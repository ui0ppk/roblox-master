/**
 * RobloxView.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxView.h"

// Qt Headers
#include <QApplication>
#include <QCursor>
#include <QPixmapCache>
#include <QStringList>
#include <QMouseEvent>
#include <QEvent>

// Roblox Headers
#include "rbx/Tasks/Coordinator.h"
#include "rbx/CEvent.h"
#include "rbx/SystemUtil.h"
#include "Network/Players.h"
#include "Network/api.h"
#include "script/script.h"
#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/ToolMouseCommand.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/GameBasicSettings.h"
#include "util/FileSystem.h"
#include "util/standardout.h"
#include "util/Profiling.h"
#include "util/ContentId.h"
#include "v8xml/XmlSerializer.h"
#include "v8xml/Serializer.h"
#include "GfxBase/ViewBase.h"
#include "GfxBase/RenderSettings.h"
#include "GfxBase/FrameRateManager.h"
#include "RenderSettingsItem.h"
#include "tool/ICancelableTool.h"
#include "AppDraw/Draw.h"

#ifdef _WIN32
    #include "LogManager.h"
#endif

#ifdef Q_WS_MAC
    #include "BreakpadCrashReporter.h"
#endif

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "UserInput.h"
#include "UserInputUtil.h"
#include "ogrewidget.h"
#include "FunctionMarshaller.h"
#include "RobloxCustomWidgets.h"
#include "RenderStatsItem.h"
#include "StudioUtilities.h"
#include "RobloxSettings.h"
#include "UpdateUIManager.h"
#include "AuthoringSettings.h"
#include "QtUtilities.h"
#include "RobloxMouseConfig.h"
#include "RobloxTreeWidget.h"
#include "CommonInsertWidget.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"

#include "rbx/Profiler.h"

LOGGROUP(HardwareMouse)
LOGGROUP(TaskSchedulerTiming)
LOGVARIABLE(RenderRequest, 0)

FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAG(StudioSeparateActionByActivationMethod)
FASTFLAG(StudioRecordToolboxInsert)

FASTFLAGVARIABLE(DirectX11Enable, false)
FASTFLAG(UserBetterInertialScrolling)

FASTINTVARIABLE(StudioRightClickBuffer, 500)
FASTINTVARIABLE(StudioRightClickDelay, 150)
FASTINTVARIABLE(StudioRightClickMax, 700)

// should point to local file that is the default cursor for 3D window
static std::string defaultMouseCursorPath = "Textures/advCursor-default.png";

class RobloxView::RenderJobCyclic : public RBX::BaseRenderJob
{
	shared_ptr<RBX::DataModel> 					m_pDataModel;
	RobloxView                                 *m_pRbxView;
	QOgreWidget                                *m_pQOgreWidget;
	bool									    viewUpdateRequested;
	double									    m_timestamp;
	bool										isBusySkipRender;

public:
	RBX::CEvent									updateView;
	RBX::CEvent									viewUpdated;

	RenderJobCyclic(RobloxView *pRbxView, shared_ptr<RBX::DataModel> pDataModel, QOgreWidget* pQOgreWidget)
	: RBX::BaseRenderJob( CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), pDataModel)
	, m_pRbxView(pRbxView)
	, m_pDataModel(pDataModel)
	, m_pQOgreWidget(pQOgreWidget)
	, viewUpdateRequested(false)
	, updateView(false)
	, viewUpdated(false)
	, isBusySkipRender(false)
	{
	cyclicExecutive = true;
        isAwake = false;
		m_timestamp = RBX::Time::nowFastSec();
		}

	~RenderJobCyclic()
	{
	}

	double getTimestamp() {
		return m_timestamp;
	}

	void setIsBusySkipRender(bool value)
	{
		isBusySkipRender = value;
	}

	bool ogreWidgetHasFocus()
	{
		return m_pQOgreWidget->hasApplicationFocus();
	}

	bool tryJobAgain()
	{
		if (!isAwake)
		{
			if (!viewUpdateRequested)
			{
				m_timestamp = RBX::Time::nowFastSec();
				//do view update
                // Prevent Double Events
                QCoreApplication::removePostedEvents(m_pQOgreWidget, OGRE_VIEW_UPDATE);
				QApplication::postEvent(m_pQOgreWidget, new RobloxCustomEvent(OGRE_VIEW_UPDATE));	

				viewUpdateRequested = true;
			}
			return true;
		}
		return false;
	}

	// Have to define to compile
	virtual RBX::Time::Interval sleepTime(const Stats&)
	{
		return RBX::Time::Interval(0);
	}

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats&)
	{
		lastRenderTime = RBX::Time::now<RBX::Time::Fast>();

		shared_ptr<RBX::DataModel> pDataModel(m_pDataModel);
		if (!pDataModel)
			return RBX::TaskScheduler::Done;

		RBX::DataModel::scoped_write_request request(pDataModel.get());
		isAwake = false;

		// This logic has to be set by RobloxView::updateView
		// This is to prevent Deadlocks, and to not follow-through the
		// locking nature of RenderJob if the UIThread is busy.
		if (!isBusySkipRender)
		{
			//start view update
			updateView.Set();

			//wait till view is updated
			viewUpdated.Wait();
		}
		viewUpdateRequested = false;
		isBusySkipRender = false;
		return RBX::TaskScheduler::Stepped;
	}
};

//Job code has been taken from Client/Roblox/RobloxWnd.cpp
class RobloxView::RenderRequestJob : public RBX::DataModelJob
{
	RobloxView                                *m_pRbxView;
	boost::shared_ptr<RBX::Tasks::Coordinator> m_pSequence;
	shared_ptr<RBX::DataModel>                 m_pDataModel;
	QOgreWidget                               *m_pQOgreWidget;
	volatile bool                              m_bIsAwake;

	double m_timestamp;
	
public:
	RenderRequestJob(RobloxView *pRbxView, shared_ptr<RBX::DataModel> pDataModel, QOgreWidget* pQOgreWidget)
	: RBX::DataModelJob("RenderRequest", RBX::DataModelJob::Render, false, pDataModel, RBX::Time::Interval(0))
	, m_pRbxView(pRbxView)	
	, m_pDataModel(pDataModel)
	, m_pQOgreWidget(pQOgreWidget)
	, m_bIsAwake(false)
	{
		cyclicExecutive = true;

		m_timestamp = RBX::Time::nowFastSec();

// Fixed Rally: DE2449 for Mouse Performance
// Created Rally: DE2557 for Rendering performance improvements
// KLUDGE - For Mac Mouse issue, especially with Trackpad
// Qt mouse events starts throttling when using trackpad
// tieing the render job with heartbeat somehow breaks the chain & gives qt chance to process mouse trackpad events
// The disadvantage of this approach is that the Render job is now maxed out at 30 fps.
// Need to find the root cause of why the problem gets resolved with tieing to heartbeat job
// Without this fix Mouse performance is 0.5 to 2 fps on CTF Model with Trackpad, unusable
// With this fix Mouse performance is 30 to 50 fps on CTF Model with Trackpad, amazing fast
#ifndef _WIN32
		m_pSequence.reset(new RBX::Tasks::Sequence());
		addCoordinator(m_pSequence);
		m_pDataModel->create<RBX::RunService>()->getHeartbeat()->addCoordinator(m_pSequence);
#endif
		
		// Force rendering and physics to happen in lock-step
		if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
		{
#ifdef _WIN32
			m_pSequence.reset(new RBX::Tasks::Sequence());
			addCoordinator(m_pSequence);
#endif
			m_pDataModel->create<RBX::RunService>()->getPhysicsJob()->addCoordinator(m_pSequence);
		}
	}
	
	~RenderRequestJob()
	{		
		if (m_pSequence)
		{
			if (RBX::RunService* pRunService = m_pDataModel->find<RBX::RunService>())
			{
#ifndef _WIN32
				pRunService->getHeartbeat()->removeCoordinator(m_pSequence);
#endif
				if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
					pRunService->getPhysicsJob()->removeCoordinator(m_pSequence);
			}

			m_pSequence.reset();
		}
	}
	
	void wake()
	{
		m_bIsAwake = true;
		RBX::TaskScheduler::singleton().reschedule(shared_from_this());
	}
	
	virtual RBX::Time::Interval sleepTime(const Stats& stats)
	{
		if (m_bIsAwake)
        {
            float frameRate = RBX::DataModel::throttleAt30Fps ? CRenderSettingsItem::singleton().getMaxFrameRate() : 1000;

            // check for application focus
            if (UpdateUIManager::Instance().isBusy() || !m_pQOgreWidget->hasApplicationFocus())
                frameRate *= (100 - qBound(0, AuthoringSettings::singleton().renderThrottlePercentage, 100)) / 100.0f;

			return computeStandardSleepTime(stats,frameRate);
        }

		return RBX::Time::Interval::max();
	}
	
	virtual Job::Error error(const Stats& stats)
	{
		if (m_bIsAwake)
			return computeStandardError(stats, RBX::DataModel::throttleAt30Fps ? CRenderSettingsItem::singleton().getMinFrameRate() : 1000);
			
		return Job::Error();
	}

	double getTimestamp() {
		return m_timestamp;
	}
	
	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats&)
	{
		m_timestamp = RBX::Time::nowFastSec();

		m_bIsAwake = false;
		//do view update
		QApplication::postEvent(m_pQOgreWidget, new RobloxCustomEvent(OGRE_VIEW_UPDATE));	

		return RBX::TaskScheduler::Stepped;
	}
	
};

class RobloxView::RenderJob : public RBX::DataModelJob
{
	boost::weak_ptr<RBX::DataModel> m_pDataModel;
	RBX::Time                       m_LastRenderTime;
	volatile bool                   m_bIsAwake;
	
public:	
	RenderJob(RobloxView *, shared_ptr<RBX::DataModel> pDataModel)
	: RBX::DataModelJob("Render", RBX::DataModelJob::Render, false, pDataModel, RBX::Time::Interval(.02))
	, m_pDataModel(pDataModel)
	, m_bIsAwake(false)
	, updateView(false)
	, viewUpdated(false)
	{
		cyclicExecutive = true;
	}

	RBX::CEvent updateView;
	RBX::CEvent viewUpdated;
	
	void wake()
	{
		m_bIsAwake = true;
		RBX::TaskScheduler::singleton().reschedule(shared_from_this());
	}
	
	RBX::Time::Interval timeSinceLastRender() const
	{
		return RBX::Time::now<RBX::Time::Fast>() - m_LastRenderTime;
	}
	
	virtual RBX::Time::Interval sleepTime(const Stats&)
	{
		return m_bIsAwake ? RBX::Time::Interval(0) : RBX::Time::Interval::max();
	}
	
	virtual Job::Error error(const Stats& stats)
	{
		if (!m_bIsAwake)
			return Job::Error();

		Job::Error result;
		if (RBX::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive )
		{
			result = computeStandardErrorCyclicExecutiveSleeping(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
		}
		else
		{
			result = computeStandardError(stats, CRenderSettingsItem::singleton().getMinFrameRate());
		}
		return result;
	}
	
	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats&)
	{
        m_LastRenderTime = RBX::Time::now<RBX::Time::Fast>();
        
		shared_ptr<RBX::DataModel> pDataModel(m_pDataModel.lock());
		if (!pDataModel)
			return RBX::TaskScheduler::Done;
		
		RBX::DataModel::scoped_write_request request(pDataModel.get());
		m_bIsAwake = false;
		
		//start view update
		updateView.Set();

		//wait till view is updated
		viewUpdated.Wait();

		return RBX::TaskScheduler::Stepped;
	}
};

class RobloxView::WrapMouseJob : public RBX::DataModelJob
{
	RobloxView      *m_pRbxView;
	RBX::DataModel  *m_pDataModel;

public:
	WrapMouseJob(RobloxView* pRbxView, shared_ptr<RBX::DataModel> pDataModel)
	: RBX::DataModelJob("WrapMouse", RBX::DataModelJob::Write, true, pDataModel, RBX::Time::Interval(0))
	, m_pRbxView(pRbxView)
	, m_pDataModel(pDataModel.get())
	{
		cyclicExecutive = true;
	}
	
	virtual RBX::Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, 60);
	}
	
	virtual Job::Error error(const Stats& stats)
	{
		Job::Error result = computeStandardError(stats, 60);
		return result;
	}
	
	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats&)
	{
		RBX::DataModel::scoped_write_request request(m_pDataModel);

		if(m_pRbxView)
			m_pRbxView->processWrapMouse();

		return RBX::TaskScheduler::Stepped;
	}
	
};

static RBX::ViewBase* createGameWindow(QOgreWidget *pQtWrapperWindow);

boost::shared_ptr<RBX::ViewBase> RobloxView::createGameView(QOgreWidget *pQtWrapperWindow)
{
	return boost::shared_ptr<RBX::ViewBase>(createGameWindow(pQtWrapperWindow));
}

RobloxView::RobloxView(QOgreWidget *pQtWrapperWidget, boost::shared_ptr<RBX::Game> game, boost::shared_ptr<RBX::ViewBase> view, bool vr)
: m_pViewBase(view)
, m_pDataModel(game->getDataModel())
, m_pQtWrapperWidget(pQtWrapperWidget)
, m_bIgnoreCursorChange(false)
, m_hoverOverAccum(0)
, m_hoverOverLastTime(RBX::Time::nowFast())
, m_rightClickContextAllowed(true)
, m_rightClickMenuPending(false)
, m_DataModelHashNeeded(true)
, m_mouseCursorHidden(false)
, m_updateViewStepId(0)
{
    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\tRobloxView::RobloxView - start");

    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tUserInput");
    m_pUserInput.reset(new UserInput(this,game->getDataModel()));

    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tMarshaller");
    m_pMarshaller = RBX::FunctionMarshaller::GetWindow();

    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tProfilingSection");
    m_profilingSection.reset(new RBX::Profiling::CodeProfiler("Render"));

	//set bounds appropriately
	setBounds(m_pQtWrapperWidget->width(), m_pQtWrapperWidget->height());

	// set this to Qt widget. Internally this may depend on the roblox view being
	// mostly set up. In particular, this may depend on the window bounds being set
	// before this call is made.
	m_pQtWrapperWidget->setRobloxView(this);

	//CRenderSettingsItem::singleton().runProfiler(false);

	//Bind view to the Workspace
    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tBindToWorkspace");
	bindToWorkspace();

	// Enable or disable VR
	m_pViewBase->enableVR(vr);

	if (RbxViewCyclicFlagsEnabled())
	{
		RBX::Log::current()->writeEntry(RBX::Log::Information, "\t\t\tRenderJobCyclic");
		m_pRenderJobCyclic = shared_ptr<RenderJobCyclic>(new RenderJobCyclic(this, m_pDataModel, m_pQtWrapperWidget));
	}
	else
	{
		//Create the rendering jobs
		RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tRenderRequestJob");
		m_pRenderRequestJob = shared_ptr<RenderRequestJob>(new RenderRequestJob(this, m_pDataModel, m_pQtWrapperWidget));

		RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tRenderJob");
		m_pRenderJob = shared_ptr<RenderJob>(new RenderJob(this, m_pDataModel));
	}
	
    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tWrapMouseJob");
	m_pWrapMouseJob = shared_ptr<WrapMouseJob>(new WrapMouseJob(this, m_pDataModel));

    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\t\tConfigureStats");
	configureStats();
	
	if (RbxViewCyclicFlagsEnabled())
	{
		RBX::TaskScheduler::singleton().add(m_pRenderJobCyclic);
		RBX::TaskScheduler::singleton().add(m_pWrapMouseJob);
	}
	else
	{
		RBX::TaskScheduler::singleton().add(m_pRenderJob);
		RBX::TaskScheduler::singleton().add(m_pRenderRequestJob);
		RBX::TaskScheduler::singleton().add(m_pWrapMouseJob);
	}

	mouseCursorChangeConnection = m_pUserInput->cursorIdChangedSignal.connect( boost::bind(&RobloxView::handleMouseCursorChange, this) );

	if (m_pDataModel && m_pDataModel.get())
    {
		if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(m_pDataModel.get()) )
        {
			handleMouseIconEnabledEvent(userInputService->getMouseIconEnabled());

			mouseCursorEnabledConnection = userInputService->mouseIconEnabledEvent.connect(boost::bind(&RobloxView::handleMouseIconEnabledEvent, this, _1));
            m_textBoxGainFocus = userInputService->textBoxGainFocus.connect(boost::bind(&RobloxView::handleTextBoxGainFocus, this));
            m_textBoxReleaseFocus = userInputService->textBoxReleaseFocus.connect(boost::bind(&RobloxView::handleTextBoxReleaseFocus, this));
        }
    }
    
    m_lastRightClickBlockingEventDate = QDateTime::currentDateTime();

    RBX::Log::current()->writeEntry(RBX::Log::Information,"\t\tRobloxView::RobloxView - end");
}
											 
RobloxView::~RobloxView(void)
{
	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

		if(m_renderStatsItem)
		{
			m_renderStatsItem->setParent(NULL);
			m_renderStatsItem.reset();
		}
	}

	if (m_pRenderJob && !RbxViewCyclicFlagsEnabled())
	{
		m_pRenderJob->viewUpdated.Set();
		
		// viewUpdateJob can deadlock if we don't process the marshaller queue
		boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, m_pMarshaller);
		RBX::TaskScheduler::singleton().removeBlocking(m_pRenderJob, callback);
		m_pRenderJob.reset();
	}
	
	if (m_pRenderRequestJob && !RbxViewCyclicFlagsEnabled())
	{
		boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, m_pMarshaller);
		RBX::TaskScheduler::singleton().removeBlocking(m_pRenderRequestJob, callback);
		m_pRenderRequestJob.reset();
	}	
	
	if (m_pRenderJobCyclic && RbxViewCyclicFlagsEnabled())
	{
		m_pRenderJobCyclic->viewUpdated.Set();
		boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, m_pMarshaller);
		RBX::TaskScheduler::singleton().removeBlocking(m_pRenderJobCyclic, callback);
		m_pRenderJobCyclic.reset();
	}
	
	if (m_pWrapMouseJob)
	{
		boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, m_pMarshaller);
		RBX::TaskScheduler::singleton().removeBlocking(m_pWrapMouseJob, callback);
		m_pWrapMouseJob.reset();
	}
    
    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    m_pMarshaller->ProcessMessages();

	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
	
		RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(m_pDataModel.get());
		service->setHardwareDevice(NULL);
		
		m_pViewBase->bindWorkspace(boost::shared_ptr<RBX::DataModel>());

		mouseCursorChangeConnection.disconnect();
		mouseCursorEnabledConnection.disconnect();

		m_pUserInput.reset();
	}
	
	RBX::FunctionMarshaller::ReleaseWindow(m_pMarshaller);
	
	// First destroy the view before closing the DataModel
	m_pViewBase.reset();

	//TODO - ROBLOXSTUDIO
	//Roblox::relinquishGame(game);
}

void RobloxView::onEvent_playerAdded(shared_ptr<RBX::Instance> playerAdded)
{
	if( RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(m_pDataModel.get()) )
		if( RBX::Network::Player* thePlayerAdded = RBX::Instance::fastDynamicCast<RBX::Network::Player>(playerAdded.get()) )
			if(localPlayer == thePlayerAdded)
				buildGui(true);
}

void RobloxView::bindToWorkspace()
{
	// we need write locks on all datamodels at this point (about to edit them outside of normal datamodel tasks)
	
	RBXASSERT(m_pDataModel->currentThreadHasWriteLock());

	// Note that this code needs to be thread-sensitive
	CRenderSettingsItem::singleton().runProfiler(false);

	m_pViewBase->bindWorkspace(m_pDataModel);
    
	// we check when a player is added to players, in case we need to load in the gui (someone might add a player via cmd line)
	if( !StudioUtilities::isAvatarMode() )
		if ( RBX::Network::Players* players = RBX::ServiceProvider::create<RBX::Network::Players>(m_pDataModel.get()) )
			m_itemAddedToPlayersConnection = players->onDemandWrite()->childAddedSignal.connect(boost::bind(&RobloxView::onEvent_playerAdded, this, _1));
	
	RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(m_pDataModel.get());
	service->setHardwareDevice(m_pUserInput.get());
}

void RobloxView::buildGui(bool buildInGameGui)
{
	if(m_pViewBase)
		m_pViewBase->buildGui(buildInGameGui);
}

void RobloxView::configureStats()
{
	// For diagnostic view
	RBX::Stats::StatsService* pStats = RBX::ServiceProvider::create<RBX::Stats::StatsService>(m_pDataModel.get());
	m_renderStatsItem = RenderStatsItem::create(m_pViewBase->getRenderStats());
	m_renderStatsItem->setName("Render");
	m_renderStatsItem->setParent(pStats);
	m_renderStatsItem->createBoundChildItem(*m_profilingSection);
}

void RobloxView::setBounds(unsigned int width, unsigned int height)
{
	m_Width = width; m_Height = height;
	if (m_pViewBase)
		m_pViewBase->onResize(width, height);
}

void RobloxView::handleMouse(RBX::InputObject::UserInputType event,
    RBX::InputObject::UserInputState state,
    int x, int y,
    RBX::ModCode modifiers)
{
	if (!m_pUserInput)
		return;

	switch (event)
	{
		case RBX::InputObject::TYPE_MOUSEMOVEMENT:
		{
			G3D::Vector2 newPos(x, y);

            // determine delta
			x = x - m_previousCursorPos.x;
			y = y - m_previousCursorPos.y;

			// if we didn't move, no need to go any further
			if (x == 0 && y == 0)
			{
				return;
			}

			if (StudioUtilities::isAvatarMode()) 
			{
				float scale = RBX::GameBasicSettings::singleton().getMouseSensitivity();
				m_previousCursorPosFraction.x += x * scale;
				m_previousCursorPosFraction.y += y * scale;
				newPos = m_previousCursorPos + m_previousCursorPosFraction;
				x = (int) (newPos.x) - (int) (m_previousCursorPos.x);
				y = (int) (newPos.y) - (int) (m_previousCursorPos.y);
				m_previousCursorPosFraction.x -= (float) x;
				m_previousCursorPosFraction.y -= (float) y;
				setCursorPos(&newPos, m_pUserInput->isLeftMouseDown(), true, false);
				if (hasApplicationFocus() && !m_mouseCursorHidden)
					handleMouseIconEnabledEvent(false);
				else if (!hasApplicationFocus() && m_mouseCursorHidden)
					handleMouseIconEnabledEvent(true);
			}

            // remember new location
			m_previousCursorPos = newPos;
            m_rightClickContextAllowed = false;
			break;
		}
		
        case RBX::InputObject::TYPE_MOUSEBUTTON2:
		{
            if (state ==RBX::InputObject::INPUT_STATE_BEGIN)
            {
                getCursorPos(&m_mousePressedPos);
                m_rightClickContextAllowed = true;

                if (m_rightClickMenuPending)
                {
                    m_rightClickContextAllowed = false;
                    m_lastRightClickBlockingEventDate = QDateTime::currentDateTime();
                }
                else
                {
                    m_rightClickMouseDownDate = QDateTime::currentDateTime();
                }
            }
            else if (state == RBX::InputObject::INPUT_STATE_END)
            {
                #ifdef _WIN32
                // we want to show the cursor inside of the 3d view when right mouse rotating camera stops
                m_bIgnoreCursorChange = false;
                setCursorImage();
                #endif
                
				if (RobloxMouseConfig::singleton().canOpenContextMenu(event, state) &&
					m_rightClickContextAllowed &&
					!m_pUserInput->areEditModeMovementKeysDown() &&
					!RBX::Profiler::isCapturingMouseInput() &&
					!StudioUtilities::isAvatarMode())
				{
					m_rightClickPosition = QPoint(x, y);
					handleContextMenu(x,y);
					return;
				}
			}
            break;
        }
		case RBX::InputObject::TYPE_MOUSEBUTTON1:
		{
			if ((state == RBX::InputObject::INPUT_STATE_BEGIN) && ((modifiers == RBX::KMOD_LCTRL) || (modifiers == RBX::KMOD_LSHIFT)))
            {
				if(RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(m_pDataModel.get()))
				{
					// check is Ctrl or Shift is not pressed already
					RBX::KeyCode rbxKey = RBX::SDLK_UNKNOWN;
					if (!userInputService->isCtrlDown() && (modifiers == RBX::KMOD_LCTRL))
						rbxKey = RBX::SDLK_LCTRL;
					else if (!userInputService->isShiftDown() && (modifiers == RBX::KMOD_LSHIFT))
						rbxKey = RBX::SDLK_LSHIFT;

					// DE7421 - if Ctrl or Shift is not pressed already then make sure we post key press event first so multiple selections can be done
					if (rbxKey != RBX::SDLK_UNKNOWN)
						m_pUserInput->PostUserInputMessage(RBX::InputObject::TYPE_KEYBOARD, RBX::InputObject::INPUT_STATE_BEGIN, rbxKey, modifiers, NULL);
				}
			}

			if(state == RBX::InputObject::INPUT_STATE_END)
				RobloxMainWindow::get(m_pQtWrapperWidget)->m_mouseActions.increment();

			break;
		}
        default:
            break;
	}
	
	m_pUserInput->PostUserInputMessage(event, state, modifiers, MAKEXYLPARAM((uint) x, (uint) y));
}

void RobloxView::handleKey(RBX::InputObject::UserInputType event,
                           RBX::InputObject::UserInputState state,
                           RBX::KeyCode keyCode,
                           RBX::ModCode modifiers,
                           const char keyWithModifier,
						   bool processed)
{
	m_pUserInput->PostUserInputMessage(event, state, keyCode, modifiers, keyWithModifier, processed);
}

void RobloxView::handleFocus(bool focus)
{
	if (m_pUserInput)
		m_pUserInput->PostUserInputMessage(RBX::InputObject::TYPE_FOCUS, focus ? RBX::InputObject::INPUT_STATE_BEGIN : RBX::InputObject::INPUT_STATE_END, 0, 0);
}

void RobloxView::resetKeyState()
{
	if (m_pUserInput)
		m_pUserInput->resetKeyState();
}

void RobloxView::handleMouseInside(bool inside)
{
	if (!m_pUserInput)
		return;

	if (inside)
	{
		m_pUserInput->onMouseInside();
		getCursorPos(&m_previousCursorPos);

		// need to immediately set cursor to what the game has
		m_bIgnoreCursorChange = false;
		setCursorImage();
	}
	else 
	{
		// should unload any cursors we added (Qt keeps a stack of cursor changes otherwise, also some platforms need this to properly set mouse icons)
		m_pQtWrapperWidget->unsetCursor();
		m_bIgnoreCursorChange = false;

		m_pUserInput->onMouseLeave();
	}
}

void RobloxView::handleTextBoxGainFocus()
{
    m_pQtWrapperWidget->setLuaTextBoxHasFocus(true);
}

void RobloxView::handleTextBoxReleaseFocus()
{
    m_pQtWrapperWidget->setLuaTextBoxHasFocus(false);
}

void RobloxView::handleMouseIconEnabledEvent(bool iconEnabled)
{
	m_mouseCursorHidden = !iconEnabled;

	if (m_mouseCursorHidden)
	{
		m_pQtWrapperWidget->unsetCursor();
		m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
	}
	else
	{
		handleMouseCursorChange();
	}
}

void RobloxView::handleMouseCursorChange()
{
	if (!m_bIgnoreCursorChange && m_pUserInput)	
		setCursorImage();
}

void RobloxView::handleScrollWheel(float delta, int x, int y)
{
	if (m_pUserInput)
	{
		// normalize the wheel input
		if (!FFlag::UserBetterInertialScrolling)
		{
			delta = G3D::clamp(delta,-1.0f, 1.0f);
		}
		m_pUserInput->PostUserInputMessage(RBX::InputObject::TYPE_MOUSEWHEEL, RBX::InputObject::INPUT_STATE_CHANGE, delta, MAKEXYLPARAM((uint) x, (uint) y));
	}
}

shared_ptr<const RBX::Instances> RobloxView::handleDropOperation(const QString& url, int x, int y, bool &mouseCommandInvoked)
{
	if (!m_pDataModel)
		throw std::runtime_error("Can't insert at this time");

	shared_ptr<RBX::Instances> instances(new RBX::Instances());
	
	try
	{
		RBX::ContentProvider* cp = NULL;
		{
			RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
			cp = m_pDataModel.get()->create<RBX::ContentProvider>();
		}

		RBXASSERT(cp);

		// load instances in a different thread
		bool hasError = false;

		RBX::ContentId contentId(url.toStdString());

		UpdateUIManager::Instance().waitForLongProcess("Inserting...", 
			boost::bind(&RobloxView::loadContent, this, shared_from(cp), contentId, 
			boost::ref(*instances), boost::ref(hasError)));

		if (FFlag::StudioRecordToolboxInsert && !hasError)
			UpdateUIManager::Instance().getMainWindow().trackToolboxInserts(contentId, *instances);

		if (hasError || instances->size() < 1)
			throw std::runtime_error("Dragged object cannot be inserted.");

		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		RBX::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, *instances);

        if (FFlag::GoogleAnalyticsTrackingEnabled)
        {
            // Get the assetID from the url and use it as the event label.
            QStringList urlSplit = url.split("?id=");
            if (!urlSplit.empty())
            {
                QString assetID = urlSplit.last();
				RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Insert", assetID.toStdString().c_str());
				RobloxMainWindow::get(m_pQtWrapperWidget)->m_inserts.increment();
            }
        }

		RBX::InsertMode insertMode = RBX::INSERT_TO_3D_VIEW;
		if ((instances->size() == 1) && RBX::Instance::fastSharedDynamicCast<RBX::Decal>(instances->at(0)))
		{
			RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(m_pDataModel->getWorkspace());
			if ((sel->size() == 1) && RBX::Instance::fastSharedDynamicCast<RBX::PartInstance>(sel->front()))
				insertMode = RBX::INSERT_TO_TREE;
		}

		insertInstances(x, y, *instances, mouseCommandInvoked, insertMode);
	}
	// Catch any invalid asset url being dragged into Roblox Ogre Widget, this can come from different application
	catch (std::exception const& e) 
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
	}

	return instances;
}

void RobloxView::handleDropOperation(boost::shared_ptr<RBX::Instance> pInstanceToInsert, int x,  int y, bool &mouseCommandInvoked)
{
	if (!m_pDataModel || !pInstanceToInsert)
		return;
	
	try
	{
		shared_ptr<RBX::Instance> pClonedObjectToInsert = pInstanceToInsert->clone(RBX::EngineCreator);
		if (!pClonedObjectToInsert)
			return;
		
		RBX::Instances instances;
		instances.push_back(pClonedObjectToInsert);

		RBX::InsertMode insertMode = RBX::INSERT_TO_3D_VIEW;
		if (RBX::Instance::fastDynamicCast<RBX::Decal>(pClonedObjectToInsert.get()) != NULL)
			insertMode = RBX::INSERT_TO_TREE;
		insertInstances(x, y, instances, mouseCommandInvoked, insertMode);
	}
	
	catch (std::exception const& e) 
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
	}
}

void RobloxView::cancelDropOperation(bool resetMouseCommand)
{
	RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

	std::for_each(m_DraggedItems.begin(), 
				  m_DraggedItems.end(), 
				  boost::bind(&RBX::Instance::setParent, _1, (RBX::Instance*) NULL));
	m_DraggedItems.clear();
	
	if (resetMouseCommand)	
	{
		RBX::MouseCommand* pCurrentMouseCommand = m_pDataModel->getWorkspace()->getCurrentMouseCommand();
		RBX::ICancelableTool* pCancelableTool = NULL;
					
		if((pCurrentMouseCommand != NULL) && (pCancelableTool = dynamic_cast<RBX::ICancelableTool*>(pCurrentMouseCommand)))
			m_pDataModel->getWorkspace()->setMouseCommand(pCancelableTool->onCancelOperation());
	}
}

void RobloxView::insertInstances(int x, int y, const RBX::Instances &instances, bool &mouseCommandInvoked, RBX::InsertMode insertMode)
{
	RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

	try 
	{
		m_DraggedItems.clear();
		mouseCommandInvoked = false;

		std::for_each(instances.begin(), instances.end(), boost::bind(&InsertObjectWidget::SetObjectDefaultValues, _1));
	
		RBX::Instances remaining;
		{
			m_pDataModel->getWorkspace()->insertInstances(instances, m_pDataModel->getWorkspace(), insertMode, RBX::SUPPRESS_PROMPTS, NULL, &remaining);
			if (!remaining.empty())
				mouseCommandInvoked = m_pDataModel->getWorkspace()->startPartDropDrag(remaining, true);

			if ((instances.size() == 1) && RBX::Instance::fastDynamicCast<RBX::Decal>(instances[0].get()))
			{
				mouseCommandInvoked = m_pDataModel->getWorkspace()->startDecalDrag(RBX::Instance::fastDynamicCast<RBX::Decal>(instances[0].get()), RBX::INSERT_TO_3D_VIEW);
			}

	        RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(m_pDataModel->getWorkspace());
			boost::shared_ptr<RBX::RunService> runService = shared_from(m_pDataModel->create<RBX::RunService>());

			// If we're not playing a game then select the content
		    if (runService->getRunState() == RBX::RS_STOPPED)
		    {
			    sel->setSelection(instances.begin(), instances.end());
		    }
		}
		
		for (size_t ii=0; ii < instances.size(); ++ii)
			m_DraggedItems.push_back(instances[ii]);
	}
	
	catch (std::exception&)
	{
		std::for_each(instances.begin(), instances.end(), boost::bind(&RBX::Instance::setParent, _1, (RBX::Instance*) NULL));
		throw;
	}
}

void RobloxView::loadContent(boost::shared_ptr<RBX::ContentProvider> cp, RBX::ContentId contentId, RBX::Instances& instances, bool& hasError)
{
	try
	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		std::auto_ptr<std::istream> stream(cp->getContent(contentId));
		Serializer().loadInstances(*stream, instances);
		hasError = false;
	}
	catch (...)
	{
		hasError = true;
	}
}

void RobloxView::processWrapMouse()
{
	if (m_pDataModel && m_pDataModel.get())
		if( RBX::UserInputService* userInputService = RBX::ServiceProvider::create<RBX::UserInputService>(m_pDataModel.get()) )
			if(userInputService->getMouseWrapMode() == RBX::UserInputService::WRAP_HYBRID)
				m_pUserInput->ProcessUserInputMessage(RBX::InputObject::TYPE_NONE,RBX::InputObject::INPUT_STATE_NONE,0,0);
}

void RobloxView::getCursorPos(G3D::Vector2 *pPos)
{
	if (!m_pQtWrapperWidget || !pPos)
		return;

	//map global position to local widget
	QPoint viewPos  = QCursor::pos();
	QPoint localPos = m_pQtWrapperWidget->mapFromGlobal(viewPos);

	//now get the position
	pPos->x = localPos.x();
	pPos->y = localPos.y();
}

void RobloxView::setDefaultCursorImage()
{
    if ( !RBX::MouseCommand::isAdvArrowToolEnabled() )
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return;
    }

	m_pQtWrapperWidget->setCursor(Qt::ArrowCursor);
}

bool RobloxView::setCursorImageInternal_deprecated(const std::string fileName)
{
    if ( !RBX::MouseCommand::isAdvArrowToolEnabled() )
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return true;
    }
     
	QString qs = fileName.c_str();
	QPixmap pix;
    if ( !QPixmapCache::find(qs,&pix) )
    {
        pix.load(qs);
        QPixmapCache::insert(qs,pix);
    }

	if(!pix.isNull()) // we have a valid image file, lets set the cursor
		m_pQtWrapperWidget->setCursor(QCursor(pix));

	return !pix.isNull();
}

bool RobloxView::setCursorImageInternal(const RBX::ContentId& contentId, shared_ptr<const std::string> imageContent)
{
	if ( !RBX::MouseCommand::isAdvArrowToolEnabled() )
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return true;
    }
     
	QString qs = contentId.toString().c_str();
	QPixmap pix;
    if ( !QPixmapCache::find(qs,&pix) )
    {
		QImage image;
		image.loadFromData((uchar*)imageContent->c_str(), imageContent->size());
		pix = QPixmap::fromImage(image);
		QPixmapCache::insert(qs, pix);
    }

	if(!pix.isNull()) // we have a valid image file, lets set the cursor
		m_pQtWrapperWidget->setCursor(QCursor(pix));

	return !pix.isNull();
}

void RobloxView::setCursorImage(bool forceShowCursor)
{
	if (!m_pQtWrapperWidget)
		return;

	if (m_mouseCursorHidden && !forceShowCursor)
	{
		m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
		return;
	}

	RBX::ContentId cursorContent = m_pUserInput->getCurrentCursorId();

	if (cursorContent.toString().empty())
	{
		if (RBX::Network::Players::findLocalPlayer(m_pDataModel.get()) != NULL)
		{
			RBX::ToolMouseCommand* pToolMouseCommand = dynamic_cast<RBX::ToolMouseCommand*>(m_pDataModel->getWorkspace()->getCurrentMouseCommand());
			if (pToolMouseCommand != NULL)
				cursorContent = RBX::ContentId::fromAssets("Textures/ArrowCursor.png");
		}
		else
		{
			setDefaultCursorImage();
			return;
		}
	}

	bool cursorIsSet = false;

	// first, unload our current cursor (we are guaranteed to set this later)
	m_pQtWrapperWidget->unsetCursor();

	// If we want the default cursor just set with Qt arrow cursor...
	if (!cursorContent.toString().compare("rbxasset://" + defaultMouseCursorPath))
	{
		setDefaultCursorImage();
		cursorIsSet = true;
	}
	else
	{
		if (m_pDataModel)
		{
			if (RBX::DataModel* dm = m_pDataModel.get())
			{
				shared_ptr<const std::string> imageContent;
				try
				{
					imageContent = RBX::ServiceProvider::create<RBX::ContentProvider>(dm)->requestContentString(
						cursorContent, RBX::ContentProvider::PRIORITY_MFC);
				}
				catch (const std::exception& e)
				{
					FASTLOG2(FLog::HardwareMouse, "RobloxView::setCursorImage failed to load cursorContent %s because %s", cursorContent.c_str(), e.what());

					// cursor load failed (either from web or local asset)
					// make sure we still set cursor to something
					setDefaultCursorImage();
					return;
				}

				if (!imageContent || imageContent->empty() ||
					!setCursorImageInternal(cursorContent, imageContent))
				{
					setDefaultCursorImage();
				}
	
				cursorIsSet = true;
			}
		}
	}

	if (!cursorIsSet)
		setDefaultCursorImage();
}

void RobloxView::setCursorPos(G3D::Vector2 *pPos, bool isLMB, bool force, bool updatePos)
{
	if (!m_pQtWrapperWidget || !pPos || !isValidCursorPos(pPos, isLMB, force))
		return;

    // need to set previous position before the call to setPos.
    // otherwise we'll send an extra delta that we don't want.
	if (updatePos)
		m_previousCursorPos = *pPos;

	//map local to global
	QPoint localPos(pPos->x, pPos->y);
	QPoint globalPos = m_pQtWrapperWidget->mapToGlobal(localPos);

	//now set the position
	QCursor::setPos(globalPos);
}

bool RobloxView::hasApplicationFocus()
{
	if (m_pQtWrapperWidget)
		return m_pQtWrapperWidget->hasApplicationFocus();
	return false;
}

bool RobloxView::isValidCursorPos(G3D::Vector2 *pPos, bool isLMB, bool force)
{
	if (force)
		return true;
 
	if (isLMB)
	{		 
		QPoint cursorPos = m_pQtWrapperWidget->mapFromGlobal(QCursor::pos());		
		return !m_pQtWrapperWidget->rect().contains(QPoint(cursorPos.x(), cursorPos.y()), true);
	}

	return (m_mousePressedPos == *pPos);
}

void RobloxView::requestUpdateView()
{ 
	if (m_pRenderRequestJob && !(RbxViewCyclicFlagsEnabled()))
	{
		m_pRenderRequestJob->wake(); 
}
}

void RobloxView::doRender(const double timeStamp)
{
    m_pViewBase->renderPrepare(this);
}

void RobloxView::updateView()
{
	if ((!m_pRenderJob || !m_pRenderRequestJob) && 
		(!(m_pRenderJobCyclic && RbxViewCyclicFlagsEnabled())))
		return;

	
	if ( !RbxViewCyclicFlagsEnabled() && 
		(UpdateUIManager::Instance().getMainWindow().isMinimized() || 
         UpdateUIManager::Instance().isBusy() ||
         !m_pQtWrapperWidget->isVisible()) )
	{
		return;
	}

	RBX::Profiling::Mark mark(*m_profilingSection, true, true);

	FASTLOG(FLog::RenderRequest, "Got render request, waking render job");

	bool cyclicExecutiveSkipsPrep = false;

	if (RbxViewCyclicFlagsEnabled())
	{
		// Keeps track of when to Skip frames for
		// Unfocused UI
		m_updateViewStepId++;

		cyclicExecutiveSkipsPrep = (UpdateUIManager::Instance().getMainWindow().isMinimized() || 
									UpdateUIManager::Instance().isBusy() ||
									!m_pQtWrapperWidget->isVisible());

		if (!m_pRenderJobCyclic->ogreWidgetHasFocus())
		{
			if (!(m_updateViewStepId / 4))
			{
				cyclicExecutiveSkipsPrep = true;
			}
		}

		if (m_updateViewStepId >= 4)
		{
			m_updateViewStepId = 0;
		}

		// Set the RenderJob to skip all the fancy shmancy Locking and Waiting
		if (cyclicExecutiveSkipsPrep)
		{
			m_pRenderJobCyclic->setIsBusySkipRender(true);
		}
		// Wake the Rendering Job
		m_pRenderJobCyclic->wake();

		// We ignore the locking nature of the RenderJob if Busy
		if (!cyclicExecutiveSkipsPrep)
		{
			m_pRenderJobCyclic->updateView.Wait();
		}
	}
	else
	{
		m_pRenderJob->wake();
		//block the job, as we are in between of an update
		m_pRenderJob->updateView.Wait();
	}

	if (!m_pViewBase.get() || !m_pDataModel)
	{
		if (RbxViewCyclicFlagsEnabled())
		{
			if (!cyclicExecutiveSkipsPrep)
			{
				m_pRenderJobCyclic->viewUpdated.Set();
			}
		}
		else
		{
			m_pRenderJob->viewUpdated.Set();
		}
		return;
	}

	FASTLOG(FLog::RenderRequest, "Job has started, proceeding");

    // update hover over animation, hover color, and selection color
    updateHoverOver();

	RBX::Profiling::CodeProfiler* renderCodeProfiler = m_pViewBase->getRenderStats().cpuRenderTotal.get();
	{
		RBX::Profiling::Mark mark(*renderCodeProfiler, true, true);
		{			
			bool isViewUpdated = false;

			if (!cyclicExecutiveSkipsPrep)
			{
				try 
				{
					RBX::FrameRateManager* pFRMgr = m_pViewBase->getFrameRateManager();
					if(pFRMgr)
					{
						pFRMgr->configureFrameRateManager(CRenderSettingsItem::singleton().getFrameRateManagerMode(), true); // hasCharacter);
					}

					//RenderUpdate phase
					{
						RBX::DataModel::scoped_write_transfer request(m_pDataModel.get());

						m_pViewBase->updateVR();

						float secondsElapsed = m_pViewBase->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;
						m_pDataModel->renderStep(secondsElapsed);	
					}
				
					double timeStamp; 

					if (RbxViewCyclicFlagsEnabled())
					{
						timeStamp = m_pRenderJobCyclic->getTimestamp();
					}
					else
					{
						timeStamp = m_pRenderRequestJob->getTimestamp();
					}
				
					doRender(timeStamp);
				
					//done with the update, release job!
					if (RbxViewCyclicFlagsEnabled())
					{
						m_pRenderJobCyclic->viewUpdated.Set();
					}
					else
					{
						m_pRenderJob->viewUpdated.Set();
					}

					isViewUpdated = true;

					m_pViewBase->renderPerform(timeStamp);

					//this is required to convey view that we are done with screenshot
					m_pViewBase->getAndClearDoScreenshot();
				}
			
				catch (std::exception& e) 
				{
					RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
				}
				catch (...) 
				{
					RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Unknown exception occurred during view update");
				}
			}
			
			if (!isViewUpdated)
			{
				if (RbxViewCyclicFlagsEnabled())
				{
					if (!cyclicExecutiveSkipsPrep)
					{
						m_pRenderJobCyclic->viewUpdated.Set();
					}
				}
				else
				{
					m_pRenderJob->viewUpdated.Set();
				}
			}
			//TODO: add support for "isPaintMessageAware"
			requestUpdateView();
		}
	}

	//init dataModelHash
	if (m_DataModelHashNeeded)
	{
		RobloxDocManager::Instance().getPlayDoc()->initializeDataModeHash();
		m_DataModelHashNeeded = false;
	}

	FASTLOG(FLog::RenderRequest, "Render request finished");
}

/**
 * Updates the hover over color and updates the selection color.
 *  If hover over animation is enabled, the hover over color will be changed.
 */
void RobloxView::updateHoverOver()
{
    // adjust the hover over and select colors
    G3D::Color4 hoverOverColor = AuthoringSettings::singleton().hoverOverColor;
    G3D::Color4 selectColor = AuthoringSettings::singleton().selectColor;

	// animation time
    const RBX::Time currentTime = RBX::Time::nowFast();
    const double deltaTime = (currentTime - m_hoverOverLastTime).msec();
    m_hoverOverAccum += deltaTime;

	// determine length of cycle
    int animateTime;
    switch ( AuthoringSettings::singleton().hoverAnimateSpeed )
    {
        case AuthoringSettings::VerySlow:
            animateTime = 2000;             // 2 seconds
            break;
        case AuthoringSettings::Slow:
            animateTime = 1000;             // 1 seconds
            break;
        default:
        case AuthoringSettings::Medium:
            animateTime = 500;              // 1/2 second
            break;
        case AuthoringSettings::Fast:
            animateTime = 250;              // 1/4 second
            break;
        case AuthoringSettings::VeryFast:
            animateTime = 100;              // 1/10 second
            break;
    }

	// adjust time accumulator to a reasonable value
    while ( m_hoverOverAccum > animateTime * 2 )
        m_hoverOverAccum -= animateTime * 2;
    m_hoverOverLastTime = currentTime;

	// adjust colors based on animation time
    if ( AuthoringSettings::singleton().animateHoverOver )
    {
        if ( m_hoverOverAccum < animateTime )
        {
            hoverOverColor = selectColor.lerp(
                hoverOverColor,
                m_hoverOverAccum / animateTime );
        }
        else
        {
            hoverOverColor = hoverOverColor.lerp(
                selectColor,
                (m_hoverOverAccum - animateTime) / animateTime );
        } 
    }
    
	// set new colors
    RBX::Draw::setSelectColor(selectColor);
    RBX::Draw::setHoverOverColor(hoverOverColor);
}

// IMetric 
double RobloxView::getMetricValue(const std::string& metric) const
{
	RBX::FrameRateManager* pFrameRateManager = m_pViewBase ? m_pViewBase->getFrameRateManager() : NULL;
	
	if (metric == "Video Memory")
        return RBX::SystemUtil::getVideoMemory();
	
	if (RbxViewCyclicFlagsEnabled())
	{
	if (metric == "Render FPS") 
			return m_pRenderJobCyclic.get() ? m_pRenderJobCyclic->averageStepsPerSecond() : 0.0;

		if (metric == "Render Duty") 
			return m_pRenderJobCyclic.get() ? m_pRenderJobCyclic->averageDutyCycle() : 0.0;
	}
	else
	{
		if (metric == "Render FPS") 
		return m_pRenderJob.get() ? m_pRenderJob->averageStepsPerSecond() : 0.0;
	
	if (metric == "Render Duty") 
		return m_pRenderJob.get() ? m_pRenderJob->averageDutyCycle() : 0.0;
	}
	
	if (metric == "Render Nominal FPS")
		return pFrameRateManager ? 1000.0 / pFrameRateManager->GetRenderTimeAverage() : 0.0;

	if (!m_pViewBase)
		return 0.0;
	
	if (metric == "Delta Between Renders")
		return m_pViewBase->getMetricValue(metric);

	if (metric == "Total Render")
		return m_pViewBase->getMetricValue(metric);

	if (metric == "Present Time")
		return m_pViewBase->getMetricValue(metric);
	
	if (metric == "GPU Delay")
		return m_pViewBase->getMetricValue(metric);
	
	return 0.0;
}

std::string RobloxView::getMetric(const std::string& metric) const
{
	if (! m_pViewBase )
		return "No View";
	
	if (metric == "Graphics Mode")
		return RBX::Reflection::EnumDesc<RBX::CRenderSettings::GraphicsMode>::singleton().convertToString(CRenderSettingsItem::singleton().getLatchedGraphicsMode());
	
	RBX::FrameRateManager* pFrameRateManager = m_pViewBase ? m_pViewBase->getFrameRateManager() : 0;
		
	if (metric == "FRM")
		return (pFrameRateManager && pFrameRateManager->IsBlockCullingEnabled()) ? "On" : "Off";
	
	if (metric == "Anti-Aliasing")
		return (pFrameRateManager && pFrameRateManager->getAntialiasingMode() == RBX::CRenderSettings::AntialiasingOn) ? "On" : "Off";
	
	RBXASSERT(0);
	return "";
}

void RobloxView::handleContextMenu()
{
    m_rightClickMenuPending = false;
    if(!m_rightClickContextAllowed)
        return;

    int x = m_rightClickPosition.x();
    int y = m_rightClickPosition.y();

    handleContextMenu(x, y);
}

/**
 * Displays a context menu at a given screen location.
 *  If nothing is selected, a left click will be simulated in order to attempt to select
 *  an object under the cursor.
 */
void RobloxView::handleContextMenu(int x, int y)
{
    m_pQtWrapperWidget->setFocus(Qt::MouseFocusReason);

    // send the right click to the engine first
    m_pUserInput->ProcessUserInputMessage(RBX::InputObject::TYPE_MOUSEBUTTON2,
        RBX::InputObject::INPUT_STATE_END,
        0,
        MAKEXYLPARAM((uint)x,(uint)y));
    
    RBX::Instance* parent;

    {
        RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

        // fire a left click to make sure whatever is under the cursor is selected
        m_pUserInput->ProcessUserInputMessage(RBX::InputObject::TYPE_MOUSEBUTTON1,
            RBX::InputObject::INPUT_STATE_BEGIN,
            0,
            MAKEXYLPARAM((uint)x,(uint)y));
        m_pUserInput->ProcessUserInputMessage(RBX::InputObject::TYPE_MOUSEBUTTON1,
            RBX::InputObject::INPUT_STATE_END,
            0,
            MAKEXYLPARAM((uint)x,(uint)y));

        // get parent object now that we might have selected something
        RBX::Selection* selection = RBX::ServiceProvider::create<RBX::Selection>(m_pDataModel.get());
        parent = selection->front().get();
    }

    if (!parent)
        parent = m_pDataModel->getWorkspace();
    
    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
    QMenu menu(m_pQtWrapperWidget);

    menu.addAction(mainWindow.cutAction);
    menu.addAction(mainWindow.copyAction);
    menu.addAction(mainWindow.pasteAction);
    menu.addAction(mainWindow.pasteIntoAction);
	menu.addAction(mainWindow.duplicateSelectionAction);
    menu.addAction(mainWindow.deleteSelectedAction);
    menu.addSeparator();

    menu.addAction(mainWindow.groupSelectionAction);
	menu.addAction(mainWindow.ungroupSelectionAction);
    
	menu.addAction(mainWindow.unionSelectionAction);
	menu.addAction(mainWindow.negateSelectionAction);
	menu.addAction(mainWindow.separateSelectionAction);

    menu.addAction(mainWindow.selectChildrenAction);
    menu.addAction(mainWindow.zoomExtentsAction);
    menu.addSeparator();

    menu.addAction(
        QIcon(QtUtilities::getPixmap(":/images/ClassImages.PNG",1)),    // part icon 
        "Insert Part",
        this,
        SLOT(onInsertPart()) );

    QMenu* insertObjectMenu = InsertObjectWidget::createMenu(parent,this,SLOT(onInsertObject()));
    menu.addMenu(insertObjectMenu);

	menu.addSeparator();
	menu.addAction(mainWindow.launchHelpForSelectionAction);

    menu.connect(&menu,SIGNAL(aboutToShow()),&UpdateUIManager::Instance(),SLOT(onMenuShow()));
    menu.connect(&menu,SIGNAL(aboutToHide()),&UpdateUIManager::Instance(),SLOT(onMenuHide()));

    QPoint p = m_pQtWrapperWidget->mapToGlobal(QPoint(x,y));

    menu.exec(p);
}

bool RobloxView::rightClickContextMenuAllowed()
{
    if (m_rightClickContextAllowed &&
        !m_pUserInput->areEditModeMovementKeysDown() &&
        !RBX::Profiler::isCapturingMouseInput() &&
        !StudioUtilities::isAvatarMode() &&
        //if there is not a right click pending
        !m_rightClickMenuPending &&
        //checking if maximum hold time has been hit
        m_rightClickMouseDownDate.time().msecsTo(QDateTime::currentDateTime().time()) < FInt::StudioRightClickMax)
    {
        //return if buffer time is less than time between last right click blocking event and now
        return (FInt::StudioRightClickBuffer < m_lastRightClickBlockingEventDate.time().msecsTo(QDateTime::currentDateTime().time()));
    }
    return false;
}

/**
 * Callback for user clicking on insert part in context menu menu.
 */
void RobloxView::onInsertPart()
{
    InsertObjectWidget::InsertObject("Part",m_pDataModel, InsertObjectWidget::InsertMode_ContextMenu, &m_rightClickPosition);
	if (FFlag::StudioSeparateActionByActivationMethod)
		UpdateUIManager::Instance().getMainWindow().sendActionCounterEvent("insertPartMainContext");
}

/**
 * Callback for user clicking on insert basic object in context menu sub menu.
 */
void RobloxView::onInsertObject()
{
    QAction* action = static_cast<QAction*>(sender());
    QString className = action->text();
    InsertObjectWidget::InsertObject(className,m_pDataModel, InsertObjectWidget::InsertMode_ContextMenu, &m_rightClickPosition);
	
	if (FFlag::StudioSeparateActionByActivationMethod)
		UpdateUIManager::Instance().getMainWindow().sendActionCounterEvent(QString("insert%1Context").arg(className));
}

bool RobloxView::eventFilter(QObject * watched, QEvent * evt)
{
    if (evt->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(evt);
        if (mouseEvent->button() == Qt::RightButton)
        {
            QMenu *menu = static_cast<QMenu *>(watched);
            m_pQtWrapperWidget->setFocus(Qt::MouseFocusReason);
            menu->resize(0,0);
            menu->move(0,0);
            m_pUserInput->PostUserInputMessage( RBX::InputObject::TYPE_MOUSEBUTTON1, 
                                                RBX::InputObject::INPUT_STATE_BEGIN,
                                                mouseEvent->modifiers(),
                                                MAKEXYLPARAM((uint) mouseEvent->x(), (uint) mouseEvent->y()));
            return true;
        }
    }
    else if (evt->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(evt);
        if (mouseEvent->button() == Qt::RightButton)
        {
            QMenu *menu = static_cast<QMenu *>(watched);
            menu->hide();
            m_pUserInput->PostUserInputMessage( RBX::InputObject::TYPE_MOUSEBUTTON1, 
                                                RBX::InputObject::INPUT_STATE_BEGIN, 
                                                mouseEvent->modifiers(), 
                                                MAKEXYLPARAM((uint) mouseEvent->x(), (uint) mouseEvent->y()));
            return true;
        }
    }
    return watched->event(evt);
}

bool RobloxView::RbxViewCyclicFlagsEnabled()
{
	return (RBX::TaskScheduler::singleton().isCyclicExecutive());
}

static RBX::ViewBase* createGameWindow(QOgreWidget *pQtWrapperWindow)
{
	if (!pQtWrapperWindow)
		return NULL;

	//Ogre Plugin Initialization
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&RBX::ViewBase::InitPluginModules, flag);

	RBX::ViewBase* pRbxView = NULL;

	std::vector<RBX::CRenderSettings::GraphicsMode> modes;

	const char* graphicsModeName;
	RBX::CRenderSettings::GraphicsMode graphicsMode = CRenderSettingsItem::singleton().getLatchedGraphicsMode();
	switch(graphicsMode)
	{
	case RBX::CRenderSettings::NoGraphics:
		break;
	case RBX::CRenderSettings::OpenGL:
		graphicsModeName = "gfx_gl";
		modes.push_back(graphicsMode);
		break;
#ifdef _WIN32
	case RBX::CRenderSettings::Direct3D9:
		graphicsModeName = "gfx_d3d9";
		modes.push_back(graphicsMode);
		break;
    case RBX::CRenderSettings::Direct3D11:
        graphicsModeName = "gfx_d3d11";
        modes.push_back(graphicsMode);
        break;
#endif
	default:
		graphicsModeName = "gfx_all";
#ifdef _WIN32
        if (FFlag::DirectX11Enable)
            modes.push_back(RBX::CRenderSettings::Direct3D11);
		modes.push_back(RBX::CRenderSettings::Direct3D9);
#endif
		modes.push_back(RBX::CRenderSettings::OpenGL);
		break;
	}

	//View creation parameters
	RBX::OSContext context;
	context.hWnd   = reinterpret_cast<void*>(pQtWrapperWindow->winId());
	context.width  = 800;
	context.height = 600;

	bool success = false;
	size_t modei = 0;
	while(!success && modei < modes.size())
	{
		graphicsMode = modes[modei];
		try
		{
			RBX::CRenderSettings& renderSettings   = CRenderSettingsItem::singleton();
			
			//Create Ogre view
			pRbxView = RBX::ViewBase::CreateView(graphicsMode, &context, &renderSettings);
			success = true;
		}
		catch (std::exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Mode %d failed: \"%s\"", graphicsMode, e.what());

			modei++;
			if(modei < modes.size())
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Trying mode %d...", modes[modei]);
			}
		}
	}


	if(modes.size() != 0 && !success)
	{
		RobloxSettings settings;
		settings.setValue("lastGFXMode", -1);
		throw std::runtime_error("Your graphics drivers seem to be too old for Roblox to use.\n\nVisit http://www.roblox.com/drivers for info on how to perform a driver upgrade.");
	}

	if(success && pRbxView)
	{
		RobloxSettings settings;
		settings.setValue("lastGFXMode", graphicsMode);

		//Initialize resources
		pRbxView->initResources();
	}

	return pRbxView;
}
