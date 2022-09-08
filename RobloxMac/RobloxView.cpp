#include "RobloxView.h"
#include "Roblox.h"

#include "GfxBase/ViewBase.h"
#include "v8datamodel/datamodel.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/camera.h"
#include "v8datamodel/game.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/GuiService.h"
#include "FunctionMarshaller.h"
#include "Util/StandardOut.h"
#include "Util/FileSystem.h"
#include "rbx/SystemUtil.h"
#include "rbx/Tasks/Coordinator.h"
#include "UserInput.h"
#include "Util/IMetric.h"
#include "Util/Object.h"
#include "GfxBase/RenderSettings.h"
#include "GfxBase/FrameRateManager.h"
#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/UserInputService.h"
#include "Util/Statistics.h"
#include "v8datamodel/ContentProvider.h"
#include "script/ScriptContext.h"
#include "v8xml/Serializer.h"
#include "rbx/CEvent.h"
#include "GameVerbs.h"
#include "Network/Players.h"
#include "../ClientBase/RenderSettingsItem.h"


#include <boost/iostreams/copy.hpp>

#include "V8DataModel/GameBasicSettings.h"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

// This job calls ViewBase::render(), which needs to be done exclusive to the DataModel.
// This is why it has the RBX::DataModelJob::Render enum, which prevents concurrent writes to DataModel.
// It also needs to run in the view's thread for OpenGL
// TODO: Can Ogre be modified to not require the thread?
class RobloxView::RenderJob : public RBX::BaseRenderJob
	, public RBX::IMetric
{
	RBX::FunctionMarshaller* marshaller;
	weak_ptr<RBX::DataModel> dataModel;
	RBX::ViewBase* view;
	RBX::CEvent renderEvent;

public:
	RenderJob(RBX::ViewBase* view, RBX::FunctionMarshaller* marshaller, shared_ptr<RBX::DataModel> dataModel)
		:RBX::BaseRenderJob(CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), dataModel),
		view(view),
		dataModel(dataModel),
		marshaller(marshaller),
		renderEvent(false)
	{
        cyclicExecutive = true;
    }
    

	RBX::Time::Interval sleepTime(const Stats& stats)
	{
        if (isAwake)
            return computeStandardSleepTime(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
        else
            return RBX::Time::Interval::max();
	}


    static void scheduleRenderPerform(const weak_ptr<RenderJob>& selfWeak, ViewBase* view, double timeJobStart)
    {
        if (shared_ptr<RenderJob> self = selfWeak.lock())
        {
            view->renderPerform(timeJobStart);
            self->wake();
        }
    }

    void doDataModelRenderStep(DataModel* dm, const float secondsElapsed)
    {
        dm->renderStep(secondsElapsed);
        isAwake = false;
        marshaller->Execute(boost::bind(&RBX::ViewBase::renderPrepare, view, this), &renderEvent);
    }

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
        DataModel* dm = view->getDataModel();
		if (!dm)
			return RBX::TaskScheduler::Done;

        double timeJobStart = RBX::Time::nowFastSec();
        try
        {
            const float secondsElapsed = view->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;
            lastRenderTime = RBX::Time::now<RBX::Time::Fast>();
            // TODO: Can we fix Ogre so that it can be called from this thread, rather than marshalled?
            
            RBX::DataModel::scoped_write_request request(dm);
            doDataModelRenderStep(dm, secondsElapsed);
            
            marshaller->Submit(boost::bind(&scheduleRenderPerform, weak_from(this), view, timeJobStart));
        }
        catch (std::exception& e)
        {
            RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
        }
        return RBX::TaskScheduler::Stepped;
	}
	
	void abortRender()
	{
		renderEvent.Set();
	}

	// IMetric 
	/*override*/ double getMetricValue(const std::string& metric) const
	{
        RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;

		if (metric == "Render FPS") {
			return averageStepsPerSecond();
		}

		if (metric == "Render Duty") {
			return averageDutyCycle();
		}
		
		if (metric == "Render Nominal FPS") {
			return frm ? 1000.0 / frm->GetRenderTimeAverage() : 0.0;
		}

		if(metric == "Delta Between Renders")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Total Render")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Present Time")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "GPU Delay")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Video Memory")
        {
			return (double)RBX::SystemUtil::getVideoMemory();
		}
		
		return 0.0;
	}


	/*override*/ std::string getMetric(const std::string& metric) const
	{
		if (! view ) {
			return "No View";
		}

		if (metric == "Graphics Mode") {
			return ""; // RBX::Reflection::EnumDesc<CRenderSettings::GraphicsMode>::singleton().convertToString(graphicsMode);
		}

		RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;


		if (metric == "FRM") {
			return (frm && frm->IsBlockCullingEnabled()) ? "On" : "Off";
		}

		if (metric == "Anti-Aliasing") {
			return (frm && frm->getAntialiasingMode() == RBX::CRenderSettings::AntialiasingOn) ? "On" : "Off";
		}

		RBXASSERT(0);
		return "";
	}
};

 class RobloxView::UserInputJob : public RBX::DataModelJob
{
	RobloxView* wnd;
	RBX::DataModel* dataModel;
public:
	UserInputJob(RobloxView* wnd, shared_ptr<RBX::DataModel> dataModel)
		:RBX::DataModelJob("UserInput", RBX::DataModelJob::Write, true, dataModel, RBX::Time::Interval(0)),
		wnd(wnd),
		dataModel(dataModel.get())
	{
	}
	
	RBX::Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, 60);
	}

	virtual Job::Error error(const Stats& stats)
	{
		Job::Error result = computeStandardError(stats, 60);
		return result;
	}

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		RBX::DataModel::scoped_write_request request(dataModel);
		if(wnd)
			wnd->processInput();
		return RBX::TaskScheduler::Stepped;
	}

};

static RBX::InputObject::UserInputType viewEventToUserInputType(RobloxView::EventType event)
{
    switch (event)
	{
		case RobloxView::MOUSE_MOVE:
			return RBX::InputObject::TYPE_MOUSEMOVEMENT;
		case RobloxView::MOUSE_LEFT_BUTTON_DOWN:
        case RobloxView::MOUSE_LEFT_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON1;
		case RobloxView::MOUSE_RIGHT_BUTTON_DOWN:
		case RobloxView::MOUSE_RIGHT_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON2;
		case RobloxView::MOUSE_MIDDLE_BUTTON_DOWN:
		case RobloxView::MOUSE_MIDDLE_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON3;
		case RobloxView::KEY_UP:
		case RobloxView::KEY_DOWN: // intentional fall thru
			return RBX::InputObject::TYPE_KEYBOARD;
		default:
			return RBX::InputObject::TYPE_NONE;
	}
}

static RBX::InputObject::UserInputState viewEventToUserInputState(RobloxView::EventType event)
{
    switch (event)
	{
		case RobloxView::MOUSE_MOVE:
			return RBX::InputObject::INPUT_STATE_CHANGE;
		case RobloxView::MOUSE_LEFT_BUTTON_DOWN:
		case RobloxView::MOUSE_RIGHT_BUTTON_DOWN:
		case RobloxView::MOUSE_MIDDLE_BUTTON_DOWN:
        case RobloxView::KEY_DOWN: // intentional fall thru
            return RBX::InputObject::INPUT_STATE_BEGIN;
        case RobloxView::MOUSE_LEFT_BUTTON_UP:
		case RobloxView::MOUSE_RIGHT_BUTTON_UP:
		case RobloxView::MOUSE_MIDDLE_BUTTON_UP:
        case RobloxView::KEY_UP: // intentional fall thru
			return RBX::InputObject::INPUT_STATE_END;
		default:
			return RBX::InputObject::INPUT_STATE_NONE;
	}
}

void RobloxView::processInput()
{
	/*if (userInput && userInput.get())
		if(userInput->getWrapMode() == RBX::UserInputBase::WRAP_HYBRID)
			userInput->ProcessUserInputMessage(RBX::UIEvent::NO_EVENT,0,0);*/
}

// Request a shutdown of the client
Boolean RobloxView::requestShutdownClient()
{
	if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
	{
        // give scripts a deadline to finish
        if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
            if (ScriptContext* scriptContext = game->getDataModel()->find<ScriptContext>())
                scriptContext->setTimeout(FLog::PlayerShutdownLuaTimeoutSeconds);
    
		DataModel::RequestShutdownResult requestResult = dataModel->requestShutdown();
		return requestResult == DataModel::CLOSE_REQUEST_HANDLED;
	}	
	
	return true;
}

void RobloxView::handleUserMessage(EventType event, unsigned int wParam, unsigned int lParam)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), wParam, lParam);
    }
}

void RobloxView::handleFocus(bool focus)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(RBX::InputObject::TYPE_FOCUS, focus ? RBX::InputObject::INPUT_STATE_BEGIN : RBX::InputObject::INPUT_STATE_END, 0, 0);
    }
}

void RobloxView::handleMouseInside(bool inside)
{
	if (userInput)
	{
		if (inside)
			userInput->onMouseInside();
		else
			userInput->onMouseLeave();
	}
}

void RobloxView::handleMouse(EventType event, int x, int y, unsigned int modifiers)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), modifiers, MAKEXYLPARAM((uint) x, (uint) y));
    }
}

void RobloxView::handleKey(EventType event, RBX::KeyCode keyCode, RBX::ModCode modifiers)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), keyCode, modifiers);
    }
}

void RobloxView::handleScrollWheel(float delta, int x, int y)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(RBX::InputObject::TYPE_MOUSEWHEEL, RBX::InputObject::INPUT_STATE_CHANGE, delta, MAKEXYLPARAM((uint) x, (uint) y));
    }
}

void RobloxView::leaveGame()
{
	marshaller->Submit(boost::bind(&RobloxView::handleLeaveGame, this));
}

void RobloxView::handleLeaveGame()
{
	Roblox::handleLeaveGame(appWindow);
}

void RobloxView::shutdownClient()
{
	marshaller->Submit(boost::bind(&RobloxView::handleShutdownClient, this));
}

void RobloxView::handleShutdownClient()
{
	Roblox::handleShutdownClient(appWindow);
}

void RobloxView::toggleFullScreen()
{
    if(getDataModel())
        getDataModel()->submitTask(boost::bind(&RobloxView::handleToggleFullScreen, this), RBX::DataModelJob::Write);
}

void RobloxView::handleToggleFullScreen()
{
	Roblox::handleToggleFullScreen(appWindow);
	RBX::GameBasicSettings::singleton().setFullScreen(Roblox::inFullScreenMode(appWindow));
}


extern std::string macBundlePath();

static RBX::ViewBase* createGameWindow(void *wnd)
{
	// static initialization:
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&RBX::ViewBase::InitPluginModules, flag);

	RBX::OSContext context;
    context.hWnd = wnd;
	context.width = 800;
	context.height = 600;

	RBX::CRenderSettings& settings = CRenderSettingsItem::singleton();
	RBX::CRenderSettings::GraphicsMode mode = RBX::CRenderSettings::OpenGL;
	RBX::ViewBase* rbxView =  RBX::ViewBase::CreateView(mode, &context, &settings);
	rbxView->initResources();

	return rbxView;
}

class DummyVerb : public RBX::Verb {
public:
	DummyVerb(VerbContainer* container, const char* name)
	:Verb(container, name)
	{
	}
	virtual bool isEnabled() const {return false;}
	
	virtual void doIt(RBX::IDataState* dataState) {}
};

RobloxView::RobloxView(void *viewwnd, void *appwnd, boost::shared_ptr<RBX::Game> game)
	:view(createGameWindow(viewwnd))
	,appWindow(appwnd)
	,viewWindow(viewwnd)
    ,userInput(new UserInput(this, game))
	,game(game)
	,leaveGameVerb(new LeaveGameVerb(this, game->getDataModel().get()))
	,fullscreenVerb(new ToggleFullscreenVerb(this, game->getDataModel().get(), NULL))
	,studioVerb(new DummyVerb(game->getDataModel().get(), "TogglePlayMode"))
	,screenshotVerb(new DummyVerb(game->getDataModel().get(), "Screenshot"))
	,shutdownClientVerb(new ShutdownClientVerb(this, game->getDataModel().get()))
	,marshaller(RBX::FunctionMarshaller::GetWindow())
{
    shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
    
    placeIDChangeConnection = dataModel->propertyChangedSignal.connect(boost::bind(&RobloxView::onPlaceIDChanged, this, _1));
	
	// Bind to the Workspace
	bindToWorkspace();
	
	// Create the rendering jobs
	renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, dataModel));
    
    // Create the user input job
    userInputJob = shared_ptr<UserInputJob>(new UserInputJob(this, dataModel));
	
	defineConcurrencyRules();

	RBX::TaskScheduler::singleton().add(renderJob);
    if (userInputJob)
    {
        RBX::TaskScheduler::singleton().add(userInputJob);
    }
}


void RobloxView::onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc)
{	
	bool placeIDChanged = desc->name=="PlaceId";
    
	if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
        if(placeIDChanged && dataModel->getPlaceID() > 0)
            Roblox::addBreakPadKeyValue("Place0", dataModel->getPlaceID());
}


void RobloxView::defineConcurrencyRules()
{
	{
		// Force viewUpdateJob and renderJob to happen serially
		boost::shared_ptr<RBX::Tasks::Coordinator> sequence(new RBX::Tasks::ExclusiveSequence());
		renderJob->addCoordinator(sequence);
	}

	if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
	{
		// Force rendering and physics to happen in lock-step
		sequence.reset(new RBX::Tasks::Sequence());
		renderJob->addCoordinator(sequence);
        if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
            dataModel->create<RBX::RunService>()->getPhysicsJob()->addCoordinator(sequence);
	}
}

void RobloxView::doUnbindWorkspace()
{
    if(view)
        view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());
}

void RobloxView::initializeInput()
{
    userInput.reset(new UserInput(this, game));

    if(userInput)
	{
		DataModel::LegacyLock lock(game->getDataModel(), DataModelJob::Write);
        
		ControllerService* service = ServiceProvider::create<ControllerService>(game->getDataModel().get());
		service->setHardwareDevice(userInput.get());
	}
}

void RobloxView::stopJobs()
{
    if (renderJob)
    {
        renderJob->abortRender();
        
        boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
        RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
    }
    
    if (userInputJob)
	{
		boost::function<void()> callback = boost::bind(&FunctionMarshaller::ProcessMessages, marshaller);
		TaskScheduler::singleton().removeBlocking(userInputJob, callback);
	}

    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();

    // All render processing is complete; it's safe to reset job pointers now
    renderJob.reset();
    userInputJob.reset();
}

void RobloxView::doShutdownDataModel()
{
    if(game)
    {
		{
			RBX::DataModel::LegacyLock lock(game->getDataModel(), RBX::DataModelJob::Write);
        
			marshaller->Submit(boost::bind(&RobloxView::doUnbindWorkspace, this));
			marshaller->ProcessMessages();
        
			if(shared_ptr<DataModel> dataModel = game->getDataModel())
			{
				if (RBX::RunService* runService = dataModel->find<RBX::RunService>())
					runService->stopTasks();
            
				if(RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get()))
					service->setHardwareDevice(NULL);
			}
        
			if (sequence)
			{
				if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
					if (RBX::RunService* rs = dataModel->find<RBX::RunService>())
						rs->getPhysicsJob()->removeCoordinator(sequence);
			}
        
			stopJobs();
        
			marshaller->ProcessMessages();
        
			if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
			{
				userInput.reset();
            
				leaveGameVerb.reset();
				fullscreenVerb.reset();
				studioVerb.reset();
				screenshotVerb.reset();
				shutdownClientVerb.reset();
			}

			game->shutdown();
		}
        
		 game->shutdown();
    }
    
    // marshaller has not finished unbinding workspace, wait until this is finished
    if(view->getDataModel())
    {
        while (view->getDataModel())
        {
            sleep(0.016667f);
        }
    }
    
    RBXASSERT(!view->getDataModel());
}

RobloxView::~RobloxView(void)
{
	if (sequence)
	{
        if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
            if (RBX::RunService* rs = dataModel->find<RBX::RunService>())
                rs->getPhysicsJob()->removeCoordinator(sequence);
	}
    
    stopJobs();

    if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
	
		RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get());
		service->setHardwareDevice(NULL);
		
		view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());

		userInput.reset();
		
		leaveGameVerb.reset();
		fullscreenVerb.reset();
		studioVerb.reset();
		screenshotVerb.reset();
		shutdownClientVerb.reset();
		
	}
	
	RBX::FunctionMarshaller::ReleaseWindow(marshaller);
	
	// First destroy the view before closing the DataModel
	view.reset();
	
	Roblox::relinquishGame(game);
}

void RobloxView::bindToWorkspace()
{
    shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
    if(!dataModel)
        return;

	// Note that this code needs to be thread-sensitive
	RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);

	view->bindWorkspace(dataModel);
	view->buildGui();

    if (userInput)
    {
        RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get());
        service->setHardwareDevice(userInput.get());
    }
}

void RobloxView::setBounds(unsigned int width, unsigned int height)
{
	this->width = width; this->height = height;
	if (view)
	{
		view->onResize(width, height);
	}
}

static void executeScript(boost::shared_ptr<RBX::Game> game, std::string urlScript, bool isApp, bool isFromProtocolHandler)
{
	RBX::Security::Impersonator impersonate(RBX::Security::COM);
	
	std::ostringstream data;
	if (RBX::ContentProvider::isUrl(urlScript))
	{
		RBX::DataModel::LegacyLock lock(game->getDataModel(), RBX::DataModelJob::Write);
		std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(game->getDataModel().get())->getContent(RBX::ContentId(urlScript.c_str())));
		boost::iostreams::copy(*stream, data);
	}
	else
		return;	// silent failure is harder to hack :)
	
    RBX::ProtectedString verifiedSource;

	try
    {
        verifiedSource = ProtectedString::fromTrustedSource(data.str());
        ContentProvider::verifyScriptSignature(verifiedSource, true);
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(std::exception&)
	{
		return;
	}
	
	shared_ptr<RBX::DataModel> dm = game->getDataModel();
    
    if(!dm)
        return;
    
	RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);
	
	if (dm->isClosed())
		return;
	
    if (urlScript.find("join.ashx"))
    {
        std::string data = verifiedSource.getSource();
        int firstNewLineIndex = data.find("\r\n");
        if (data[firstNewLineIndex+2] == '{')
        {
            // TODO: create shared enum between PC and Mac
            // Values taken from SharedLauncher::LaunchMode
            int launchMode = 0;
            if (isFromProtocolHandler)
                launchMode = 1;
            
            game->configurePlayer(RBX::Security::COM, data.substr(firstNewLineIndex+2), launchMode);
            return;
        }
    }
    
	RBX::ScriptContext* context = dm->create<RBX::ScriptContext>();
	context->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Game");
}

boost::shared_ptr<RBX::Game> RobloxView::startGame(std::string urlScript, const bool isApp) {
	
	boost::shared_ptr<RBX::Game> game(Roblox::getpreloadedGame(isApp));
	boost::thread(boost::bind(&executeScript, game, urlScript, isApp, false));
	return game;
}

RobloxView *RobloxView::start_game(void *wnd, void *appwnd, std::string urlScript, const bool isApp)
{
    boost::shared_ptr<RBX::Game> game = startGame(urlScript, isApp);
    
    return new RobloxView(wnd, appwnd, game);
}

RobloxView *RobloxView::init_game(void *wnd, void* appwnd, const bool isApp)
{
    boost::shared_ptr<RBX::Game> game(Roblox::getpreloadedGame(isApp));
    return new RobloxView(wnd, appwnd, game);
}

void RobloxView::executeJoinScript(const std::string& urlScript, const bool isApp, const bool isFromProtocolHandler)
{
    boost::thread(boost::bind(&executeScript, game, urlScript, isApp, isFromProtocolHandler));
}

void RobloxView::setUIMessage(const std::string& message)
{
    if (shared_ptr<DataModel> dm = game->getDataModel())
        dm->submitTask(boost::bind(&RobloxView::setUIMessage_, this, message), DataModelJob::Write);
}

void RobloxView::setUIMessage_(const std::string& message)
{
    if (shared_ptr<DataModel> dm = game->getDataModel())
    {
        if (message.length() > 0)
        {
            dm->setUiMessage(message);
        }
        else
        {
            dm->clearUiMessage();
        }
        
        if (GuiService*gs = dm->create<GuiService>())
            gs->setUiMessage(GuiService::UIMESSAGE_INFO, message);
    }
}


