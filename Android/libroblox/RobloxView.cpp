#include "RobloxView.h"

#include "GfxBase/ViewBase.h"
#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/camera.h"
#include "v8datamodel/game.h"
#include "FunctionMarshaller.h"
#include "Util/StandardOut.h"
#include "Util/FileSystem.h"
#include "rbx/Tasks/Coordinator.h"
#include "Util/IMetric.h"
#include "Util/Object.h"
#include "GfxBase/RenderSettings.h"
#include "GfxBase/FrameRateManager.h"
#include "v8datamodel/UserController.h"
#include "Util/Statistics.h"
#include "v8datamodel/ContentProvider.h"
#include "script/ScriptContext.h"
#include "v8xml/Serializer.h"
#include "rbx/CEvent.h"
#include "../RobloxMac/GameVerbs.h"
#include "Network/Players.h"
#include "../ClientBase/RenderSettingsItem.h"
#include "rbx/SystemUtil.h"

#include "JNIMain.h"
#include "JNIGLActivity.h"

#include <boost/iostreams/copy.hpp>

#include "../RobloxMac/Roblox.h"
#include "V8DataModel/GameBasicSettings.h"

#include "FastLog.h"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

FASTFLAG(RenderLowLatencyLoop)

namespace RBX
{

LeaveGameVerb::LeaveGameVerb(RBX::VerbContainer* container) :
	Verb(container, "Exit")
{
}

void LeaveGameVerb::doIt(RBX::IDataState* dataState)
{
	JNI::exitGame();
}

namespace JNI
{
extern char *cacheDirectory;
} // namespace JNI
} // namespace RBX

LOGGROUP(RenderBreakdown)
FASTFLAGVARIABLE(RenderCleanupInBackground, true)
FASTFLAGVARIABLE(EnableiOSSettingsLeave, false)


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
	RBX::CEvent prepareBeginEvent;
	RBX::CEvent prepareEndEvent;
    volatile int stopped;

public:
	RenderJob(RBX::ViewBase* view, RBX::FunctionMarshaller* marshaller, shared_ptr<RBX::DataModel> dataModel)
		: RBX::BaseRenderJob( CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), dataModel)
		, view(view)
		, dataModel(dataModel)
		, marshaller(marshaller)
		, renderEvent(false)
		, prepareBeginEvent(false)
		, prepareEndEvent(false)
        , stopped(0)
	{
		cyclicExecutive = 1;
	}

	RBX::Time::Interval sleepTime(const Stats& stats)
	{
        if(isAwake)
            return computeStandardSleepTime(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
        else
            return RBX::Time::Interval::max();
	}
    
    void stop()
    {
        stopped = 1;
    }
    
	static void scheduleRender(weak_ptr<RenderJob> selfWeak, ViewBase* view, double timeJobStart)
	{
		shared_ptr<RenderJob> self = selfWeak.lock();
		if (!self) return;

		self->prepareBeginEvent.Wait();

		view->renderPrepare(self.get());

		self->prepareEndEvent.Set();

		view->renderPerform(timeJobStart);

		self->wake();
	}

    static void scheduleRenderPrepare(RenderJob* self, ViewBase* view)
    {
        if (self->stopped != 0)
            return;
            
        view->renderPrepare(self);
    }

    static void scheduleRenderPerform(RenderJob* self, ViewBase* view, double timeJobStart)
    {
        if( !self->dataModel.lock() )
            return;
        
        if ( self->stopped != 0 )
            return;
        
        if(!view)
            return;
        
        view->renderPerform(timeJobStart);
        self->wake();
    }

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		shared_ptr<RBX::DataModel> dm(dataModel.lock());
		if (!dm || stopped)
			return RBX::TaskScheduler::Done;
        
        // Initially a view does not have a data model; it gets one during bindWorkspace.
        // If bindWorkspace is launched asynchronously, there is a possibility of a race -
        // render job might start before bindWorkspace gets a chance to run.
        // It should be safe to skip the render job in this case.
        if (!view->getDataModel())
            return RBX::TaskScheduler::Stepped;
        
        double timeJobStart = Time::nowFastSec();
        
		if (FFlag::RenderLowLatencyLoop)
		{
			RBX::DataModel::scoped_write_request request(dm.get());

			const double renderDelta = timeSinceLastRender().seconds();

			lastRenderTime = RBX::Time::now<RBX::Time::Fast>();
			isAwake = false;

			marshaller->Submit(boost::bind(&scheduleRender, weak_from(this), view, timeJobStart));

			view->updateVR();

			dm->renderStep(renderDelta);

			prepareBeginEvent.Set();
			prepareEndEvent.Wait();
		}
		else
		{
			try
			{
				{
					RBX::DataModel::scoped_write_request request(dm.get());

					const double renderDelta = timeSinceLastRender().seconds();
					lastRenderTime = RBX::Time::now<RBX::Time::Fast>();

					view->updateVR();

					dm->renderStep(renderDelta);
				
					isAwake = false;
					FASTLOG(FLog::RenderBreakdown, "Trigger renderPrepare");
					marshaller->Execute(boost::bind(&scheduleRenderPrepare, this, view), &renderEvent);
					FASTLOG(FLog::RenderBreakdown, "Finished renderPrepare");
				}
				
				{
					FASTLOG(FLog::RenderBreakdown, "Trigger renderPerform");
					marshaller->Submit(boost::bind(&scheduleRenderPerform, this, view, timeJobStart));
					FASTLOG(FLog::RenderBreakdown, "Finished renderPerform");
				}
			}
			catch (RBX::base_exception& e)
			{
				RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
			}
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

        if (metric == "Render FPS")
			return averageStepsPerSecond();
        if (metric == "Render Duty")
			return averageDutyCycle();
        if (metric == "Render Job Time")
			return averageStepTime();
        if (metric == "Render Nominal FPS")
			return frm ? 1000.0 / frm->GetRenderTimeAverage() : 0.0;
        if (metric == "Delta Between Renders")
			return view->getMetricValue(metric);
        if (metric == "Total Render")
			return view->getMetricValue(metric);
        if (metric == "Present Time")
            return view->getMetricValue(metric);
        if (metric == "GPU Delay")
			return view->getMetricValue(metric);
        if (metric == "Render Prepare")
			return view->getMetricValue(metric);
        if (metric == "Video Memory MB")
			return RBX::SystemUtil::getVideoMemory() / 1e6;
		
		return 0.0;
	}


	/*override*/ std::string getMetric(const std::string& metric) const
	{
		if (! view )
			return "No View";
		if (metric == "Graphics Mode")
			return "";

		RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;

		if (metric == "FRM")
			return (frm && frm->IsBlockCullingEnabled()) ? "On" : "Off";
		if (metric == "Anti-Aliasing")
			return (frm && frm->getAntialiasingMode() == RBX::CRenderSettings::AntialiasingOn) ? "On" : "Off";

		RBXASSERT(0);
		return "";
	}
};

void RobloxView::requestStopRenderingForBackgroundMode()
{
    if (FFlag::RenderCleanupInBackground)
    {
        if (renderJob)
        {
            renderJob->abortRender();
            boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
            RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
        }
        
        // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
        // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
        // in the marshaller queue.
        // This makes sure that all pending marshalled events are processed to avoid a use after free.
        marshaller->ProcessMessages();
        
        // All render processing is complete; it's safe to reset job pointers now
        renderJob.reset();
    }
    else
    {
        if (renderJob)
        {
            renderJob->abortRender();
            renderJob->stop();
            renderJob.reset();
        }
    }
}

void RobloxView::requestResumeRendering()
{
    renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, game->getDataModel()));

    RBX::TaskScheduler::singleton().add(renderJob);
}

static RBX::ViewBase* createGameWindow(RobloxView* view, void *wnd, unsigned int width, unsigned int height)
{
	static boost::once_flag flag2 = BOOST_ONCE_INIT;
	
	// static initialization:
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&RBX::ViewBase::InitPluginModules, flag);

	RBX::OSContext context;
	context.hWnd = wnd;
    context.width = width;
    context.height = height;

	CRenderSettingsItem& settings = CRenderSettingsItem::singleton();
    
	RBX::CRenderSettings::GraphicsMode mode = RBX::CRenderSettings::OpenGL;
	RBX::ViewBase* rbxView = RBX::ViewBase::CreateView(mode, &context, &settings);

	rbxView->initResources();

	return rbxView;
}

RobloxView::RobloxView(void* wnd, unsigned int width, unsigned int height)
    :view(createGameWindow(this, wnd, width, height))
    ,marshaller(RBX::FunctionMarshaller::GetWindow())
{
}

void RobloxView::completeViewPrep(shared_ptr<RBX::Game> game)
{
    this->game = game;
    
    placeIDChangeConnection = game->getDataModel()->propertyChangedSignal.connect( boost::bind(&RobloxView::onPlaceIDChanged, this, _1) );
    
    shared_ptr<DataModel> dataModelToSubmitOn = game->getDataModel();

    {
        RBX::DataModel::LegacyLock lock(dataModelToSubmitOn.get(), RBX::DataModelJob::Write);
        if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(dataModelToSubmitOn.get()) )
        {
            userInputService->setTouchEnabled(true);
        }
    }
    
    bindWorkspace(view, game->getDataModel());
    
    // complete render jobs setup
    renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, game->getDataModel()));
    
    defineConcurrencyRules();
    
    // Important! only schedule view and render jobs after concurrency rules are defined
    RBX::TaskScheduler::singleton().add(renderJob);

   if (shared_ptr<RBX::DataModel> sharedDM = game->getDataModel())
	{
		if (RBX::DataModel* dm = sharedDM.get())
		{
			leaveGameVerb.reset(new RBX::LeaveGameVerb(dm) );
		}
	}
}

void RobloxView::replaceGame(shared_ptr<RBX::Game> game)
{
    // Shut down
    if (sequence)
    {
        if (RBX::RunService* rs = game->getDataModel()->find<RBX::RunService>())
            rs->getPhysicsJob()->removeCoordinator(sequence);
    }
    
    if (renderJob)
    {
        renderJob->abortRender();
        
        boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
        RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
    }
    
    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();
    
   
    if (boost::shared_ptr<RBX::DataModel> dataModel = game->getDataModel())
    {
        // give scripts a deadline to finish
        if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
            if (ScriptContext* scriptContext = game->getDataModel()->find<ScriptContext>())
                scriptContext->setTimeout(FLog::PlayerShutdownLuaTimeoutSeconds);
        dataModel->setIsShuttingDown(true);
    }
   
    
    {
        RBX::DataModel::LegacyLock lock(game->getDataModel().get(), RBX::DataModelJob::Write);
        
        RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(game->getDataModel().get());
        service->setHardwareDevice(NULL);
        
        view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());
        
    }

    // Setup the new game
    this->game = game;
    // Necessary to insure we have touch controls after a teleport
    if(DataModel* dm = game->getDataModel().get())
    {
        if(RBX::UserInputService* inputService = RBX::ServiceProvider::create<RBX::UserInputService>(dm))
        {
            inputService->setTouchEnabled(true);

            inputService->textBoxGainFocus.connect( boost::bind(&JNI::textBoxFocused, _1) );
            inputService->motionEventListeningStarted.connect( boost::bind(&JNI::motionEventListening, _1) );
            inputService->textBoxReleaseFocus.connect( boost::bind(&JNI::textBoxFocusLost, _1) );
        }

        leaveGameVerb.reset(new RBX::LeaveGameVerb(dm) );
    }

    placeIDChangeConnection = game->getDataModel()->propertyChangedSignal.connect( boost::bind(&RobloxView::onPlaceIDChanged, this, _1) );
    
    bindWorkspace(view, game->getDataModel());
    
    // complete render jobs setup
    renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, game->getDataModel()));
    
    defineConcurrencyRules();
    
    // Important! only schedule view and render jobs after concurrency rules are defined
    RBX::TaskScheduler::singleton().add(renderJob);
}

void RobloxView::restartDataModel()
{
    doRestartDataModel();
}

void RobloxView::doRestartDataModel()
{
//    dispatch_async( dispatch_get_main_queue(), ^{
        
        {
            if (RBX::RunService* rs = game->getDataModel()->find<RBX::RunService>())
                rs->stopTasks();
        }
        
        renderJob->abortRender();
        renderJob->stop();
        
        {
            boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
            RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
            
            if (sequence)
            {
                if (RBX::RunService* rs = game->getDataModel()->find<RBX::RunService>())
                    rs->getPhysicsJob()->removeCoordinator(sequence);
            }
        }

        // make sure all jobs have been completed, we are going to destroy game datamodel now
        marshaller->ProcessMessages();
        
        // All render processing is complete; it's safe to reset job pointers now
        renderJob.reset();
        
        {
            RBX::DataModel::LegacyLock dmlock(game->getDataModel().get(), RBX::DataModelJob::Write);
            
            RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(game->getDataModel().get());
            service->setHardwareDevice(NULL);
            
            placeIDChangeConnection.disconnect();
            
        }

		game->shutdown();
        
        setupNewDataModel();
//    });
}

void RobloxView::setupNewDataModel()
{
    if(game->getDataModel())
        return;
    
    {
        RBX::DataModel::LegacyLock dmlock(game->getDataModel(), RBX::DataModelJob::Write);
        
        view->bindWorkspace(game->getDataModel());
        view->buildGui();
        
        placeIDChangeConnection = game->getDataModel()->propertyChangedSignal.connect( boost::bind(&RobloxView::onPlaceIDChanged, this, _1) );
    }
}

void RobloxView::newGameDidStart()
{
//    dispatch_async( dispatch_get_main_queue(), ^{
        // now we can start rendering again
        requestResumeRendering();
//    });
}

void RobloxView::onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc)
{
#if !RBX_PLATFORM_IOS
//	bool placeIDChanged = desc->name=="PlaceId";
//
//	if(placeIDChanged && dataModel->getPlaceID() > 0)
//		Roblox::addBreakPadKeyValue("Place0", dataModel->getPlaceID());
#endif
}


void RobloxView::defineConcurrencyRules()
{
    RBXASSERT(renderJob);

	if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
	{
		// Force rendering and physics to happen in lock-step
		sequence.reset(new RBX::Tasks::Sequence());
		renderJob->addCoordinator(sequence);
		game->getDataModel()->create<RBX::RunService>()->getPhysicsJob()->addCoordinator(sequence);
	}
}

RobloxView::~RobloxView(void)
{
    if (sequence)
	{
		if (RBX::RunService* rs = game->getDataModel()->find<RBX::RunService>())
			rs->getPhysicsJob()->removeCoordinator(sequence);
	}
    
	if (renderJob)
	{
		renderJob->abortRender();
		
		boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
		RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
	}
    
    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();
    
    
	if (boost::shared_ptr<RBX::DataModel> dataModel = game->getDataModel())
	{
		dataModel->setIsShuttingDown(true);
	}
    
    
	{
		RBX::DataModel::LegacyLock lock(game->getDataModel().get(), RBX::DataModelJob::Write);
        
		RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(game->getDataModel().get());
		service->setHardwareDevice(NULL);
		
		view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());
        
	}
    
	RBX::FunctionMarshaller::ReleaseWindow(marshaller);
	
	// First destroy the view before closing the DataModel
	view.reset();
}

void RobloxView::bindWorkspace(boost::shared_ptr<RBX::ViewBase> view, boost::shared_ptr<RBX::DataModel> const dataModel, bool buildGUI)
{
    DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
    view->bindWorkspace(dataModel);
    if(buildGUI)
        view->buildGui();
}

void RobloxView::setBounds(unsigned int width, unsigned int height)
{
	this->width = width; this->height = height;
	if (view)
		view->onResize(width, height);
}

RobloxView *RobloxView::create_view(shared_ptr<RBX::Game> game, void* wnd, unsigned int width, unsigned int height)
{
    RobloxView* result = new RobloxView(wnd, width,height);
    result->completeViewPrep(game);
    return result;
}

void RobloxView::shutDownGraphics(shared_ptr<RBX::Game> game)
{
    if (renderJob)
    {
        renderJob->abortRender();
		renderJob->stop();
        
        boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
        RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);

		renderJob.reset();
	}

    marshaller->ProcessMessages();

	view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());

	RBX::FunctionMarshaller::ReleaseWindow(marshaller);

	view.reset();
}

void RobloxView::startUpGraphics(shared_ptr<RBX::Game> game, void *wnd, unsigned int width, unsigned int height )
{
	view = boost::shared_ptr<RBX::ViewBase>( createGameWindow(this, wnd, width, height) );
	marshaller = RBX::FunctionMarshaller::GetWindow();

	bindWorkspace(view, game->getDataModel(), false);
	
	// complete render jobs setup
    renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, game->getDataModel()));
    
    defineConcurrencyRules();
    
    // Important! only schedule view and render jobs after concurrency rules are defined
    RBX::TaskScheduler::singleton().add(renderJob);
}

