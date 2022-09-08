/**
 * RobloxIDEDoc.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxIDEDoc.h"

// Qt Headers
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDomDocument>
#include <QDomElement>

// 3rd Party Headers
#include "boost/iostreams/copy.hpp"

// Roblox Headers
#include "v8world/World.h"
#include "v8datamodel/PartOperation.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/CommonVerbs.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/PluginManager.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/Light.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/ServerStorage.h"
#include "v8datamodel/ReplicatedStorage.h"
#include "v8datamodel/Visit.h"
#include "v8xml/Serializer.h"
#include "v8xml/SerializerBinary.h"
#include "rbx/BaldPtr.h"
#include "AppDraw/Draw.h"
#include "script/ScriptContext.h"
#include "script/LuaMemory.h"
#include "script/DebuggerManager.h"
#include "script/ModuleScript.h"
#include "script/Script.h"
#include "network/Players.h"
#include "util/ProtectedString.h"
#include "util/Statistics.h"
#include "util/Profiling.h"
#include "util/MD5Hasher.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "util/ScopedAssign.h"
#include "Util/LuaWebService.h"
#include "Util/Hash.h"
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/ViewBase.h"
#include "GfxBase/IAdornableCollector.h"
#include "RenderSettingsItem.h"
#include "Tool/ToolsArrow.h"
#include "V8Xml/WebParser.h"
#include "v8datamodel/BasicPartInstance.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "v8datamodel/MegaCluster.h"
#include "StringConv.h"

#include "script/ScriptAnalyzer.h"

// Roblox Studio Headers
#include "RobloxView.h"
#include "RobloxTreeWidget.h"
#include "ogrewidget.h"
#include "RobloxMainWindow.h"
#include "RobloxStudioVerbs.h"
#include "RobloxPropertyWidget.h"
#include "RobloxSettings.h"
#include "StudioUtilities.h"
#include "RobloxDiagnosticsView.h"
#include "RobloxGameExplorer.h"
#include "RobloxScriptReview.h"
#include "CommonInsertWidget.h"
#include "RobloxToolBox.h"
#include "RobloxTutorials.h"
#include "RobloxDocManager.h"
#include "WebDialog.h"
#include "IExternalHandler.h"
#include "RobloxCustomWidgets.h"
#include "QtUtilities.h"
#include "RobloxScriptDoc.h"
#include "UpdateUIManager.h"
#include "AuthoringSettings.h"
#include "StudioSerializerHelper.h"
#include "InsertServiceDialog.h"
#include "DebuggerClient.h"
#include "RobloxPluginHost.h"
#include "PluginAction.h"
#include "RobloxUser.h"
#include "DebuggerWidgets.h"
#include "RobloxApplicationManager.h"
#include "ScriptAnalysisWidget.h"
#include "StudioDeviceEmulator.h"
#include "KeepSavedChangesDialog.h"
#include "AuthenticationHelper.h"
#include "RobloxTextOutputWidget.h"
#include "RobloxChatWidget.h"
#include "PlayersDataManager.h"
#include "RobloxTeamCreateWidget.h"

#include "RobloxServicesTools.h"

using namespace RBX;
using namespace boost;

LOGGROUP(AutoSave)

FASTFLAG(LuaDebugger)
FASTFLAG(StudioSeparateActionByActivationMethod)
FASTFLAG(StudioShowTutorialsByDefault)
FASTFLAG(StudioEmbeddedFindDialogEnabled)
FASTFLAG(StudioCustomStatsEnabled)
FASTFLAG(UserBetterInertialScrolling)
FASTFLAG(StudioEnableGameAnimationsTab)
DYNAMIC_FASTFLAG(ScriptExecutionContextApi)
FASTFLAG(MaterialPropertiesEnabled)

FASTFLAGVARIABLE(AnalyzerGroupUIEnabled, false)

FASTFLAGVARIABLE(PrefetchResourcesEnabled, false)

FASTFLAGVARIABLE(SearchToolboxByDataModelSearchString, false)
FASTFLAGVARIABLE(EnablePhysicalPropertiesByDefault, false)

FASTFLAGVARIABLE(PGSSolverDefaultOnNewPlaces, false)
FASTFLAGVARIABLE(NoPromptOnAlreadySavedPlace, true)

FASTFLAG(UseBuildGenericGameUrl)
FASTFLAG(CSGPhysicsLevelOfDetailEnabled)
DYNAMIC_FASTSTRINGVARIABLE(WorkspaceMessageText, std::string("Test Me - Please Remove"))
DYNAMIC_FASTSTRINGVARIABLE(WorkspaceMessageLink, std::string("http://www.roblox.com"))
DYNAMIC_FASTFLAGVARIABLE(WorkspaceNotificationMasterEnable, false)

// Level 1 = Show it to people with DEFAULT Physical Properties settings
// Level 2 = Show it to people with LEGACY and DEFAULT Physical properties settings
DYNAMIC_FASTINTVARIABLE(StudioWorkspaceNotificationLevel, 0) 
FASTFLAGVARIABLE(CLI10590_FixConversionDialog, true)
FASTFLAGVARIABLE(RetryWhenCloudEditEnabledEndpointFails, false)

DYNAMIC_FASTFLAG(UseR15Character)

Q_DECLARE_METATYPE(boost::function<void(int)>);
Q_DECLARE_METATYPE(boost::function<void(std::string)>);

static const char* DraggerGridModeSetting = "DraggerGridMode";
static const char* GridSizeSetting        = "GridSize";
static const char* StatsVerb            = "Stats";
static const char* RenderStatsVerb      = "RenderStats";
static const char* PhysicsStatsVerb     = "PhysicsStats";
static const char* NetworkStatsVerb     = "NetworkStats";
static const char* SummaryStatsVerb     = "SummaryStats";
static const char* CustomStatsVerb      = "CustomStats";
static const char* kMaterialPropertiesConvertDialogDisabled = "MaterialPropertiesConvertDialogDisabled";
static const char* kMaterialPropertiesConvertByDefault = "MaterialPropertiesConvertByDefault";

static const char* PlaySoloDebugFileExt = "_NPS"; // extension to be added to rdbg file for New Play Solo mode

static const char* kPublishedPlacePrefix = "Published as ";
static const char* kCloudEditPlacePrefix = "Team Create: ";
static const char* kNewPlaceWindowTitle  = "Untitled";

#define RBX_MANUAL_JOINT_CREATION_KEY "RbxManualJointCreationMode"

int RobloxIDEDoc::sIDEDocCount = 1;

RobloxIDEDoc::RobloxIDEDoc(RobloxMainWindow* pMainWindow)
: m_WrapperWidget(NULL)
, m_pQOgreWidget(NULL)
, m_pDiagViewWidget(NULL)
, m_pMainWindow(pMainWindow)
, m_pFileWatcher(NULL)
, m_fileName("")
, m_displayName(QString("Place%1").arg(sIDEDocCount))
, m_bIsLocalDocument(true)
, m_initializationScript("")
, m_IsAutoSaveLoad(false)
, m_TestModeEnabled(true)
, m_ReopenLastSavedPlace(true)
, m_IOMutex(QMutex::NonRecursive)
, m_RequiresSave(false)
, m_CurrentGame(&m_EditGame)
, m_initialDataModelHash("")
, m_AnnouncementWidget(NULL)
{ 
#ifdef _WIN32
	m_pRecordToggle = NULL;
#endif
	
	InitializeDefaultsFromSettings();
}

RobloxIDEDoc::~RobloxIDEDoc()
{
    clearPlayModifiedScriptList();
}

void RobloxIDEDoc::applyScriptChanges()
{
	// Get all scripts and apply changes
	const tScriptDocs& openScriptDocs = RobloxDocManager::Instance().getOpenScriptDocs();
	tScriptDocs::const_iterator iter = openScriptDocs.begin();
	for (; iter != openScriptDocs.end() ; ++iter)
		(*iter)->applyEditChanges();
}

void RobloxIDEDoc::InitializeDefaultsFromSettings()
{
	RobloxSettings settings;

	// Set our default camera speeds
	RBX::Camera::CameraKeyMoveFactor = AuthoringSettings::singleton().getCameraKeyMoveSpeed();
	RBX::Camera::CameraShiftKeyMoveFactor = AuthoringSettings::singleton().getCameraShiftMoveSpeed();
	RBX::Camera::CameraMouseWheelMoveFactor = AuthoringSettings::singleton().getCameraMouseWheelMoveSpeed();

	// Turn on or off the dragger grid
	RBX::ArrowToolBase::showDraggerGrid = AuthoringSettings::singleton().getShowDraggerGrid();
    RBX::Draw::showHoverOver(AuthoringSettings::singleton().getShowHoverOver());
}

// services that don't necessarily exist in a game, but a user may want to put into their game, so we show them in studio
void RobloxIDEDoc::createOptionalDMServices()
{
    boost::shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();

	dataModel->create<RBX::ContentProvider>()->setAssetFetchMediator(
		&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER));

	RBX::ServiceProvider::create<RBX::ServerScriptService>(dataModel.get());
	RBX::ServiceProvider::create<RBX::ServerStorage>(dataModel.get());
	RBX::ServiceProvider::create<RBX::ReplicatedStorage>(dataModel.get());

	if (m_CurrentGame->m_Game != m_PlayGame.m_Game &&
		!StudioUtilities::isAvatarMode() &&
		!StudioUtilities::isTestMode())
		dataModel->setIsStudio(true);

	dataModel->create<RBX::LuaWebService>()->setCheckApiAccessBecauseInStudio();
	dataModel->create<RBX::Network::Players>()->setNonSuperSafeChatForAllPlayersEnabled(true);

	if (AuthoringSettings::singleton().onlyPlayFocusWindowAudio)
	{
		setMuteAllSounds(!QApplication::activeWindow());
		if (m_pMainWindow)
		{
			if (QAction* audioToggleWidget = m_pMainWindow->findChild<QAction*>("audioToggleAction"))
			{
				audioToggleWidget->setChecked(!QApplication::activeWindow());
			}
		}
	}

	dataModel->getWorkspace()->getAdornableCollector()->onRenderableDescendantAdded(
		&m_CurrentGame->m_SelectionHighlightAdornable);
}

void RobloxIDEDoc::displayWorkspaceMessage()
{
	// Physical Properties Specific display of dialog.
	if (DFInt::StudioWorkspaceNotificationLevel && DFFlag::MaterialPropertiesEnabled )
	{
		bool messageEnabled = false;
		RBX::PhysicalPropertiesMode currentMode = this->getDataModel()->getWorkspace()->getPhysicalPropertiesMode();
		switch (DFInt::StudioWorkspaceNotificationLevel)
		{
		case 1:
			messageEnabled = currentMode == RBX::PhysicalPropertiesMode_Default;
			break;
		case 2:
			messageEnabled = currentMode <= RBX::PhysicalPropertiesMode_Default;
			break;
		default:
			break;
		}

		if ( messageEnabled)
		{
			// Should only display if Roblox user hasn't set his play to Legacy Mode
			// or hasn't upgraded to new mode.
			m_AnnouncementWidget->showText();
		}
		else
		{
			m_AnnouncementWidget->hideText();
		}
	}

	if (DFFlag::WorkspaceNotificationMasterEnable)
	{
		m_AnnouncementWidget->showText();
	}
}

void RobloxIDEDoc::initializeNewPlace()
{
    // disable studio mode.  studio mode wants to do some fancy camera focus stuff
    //  that causes a crash because we haven't initialized the network client yet
    bool oldStudioMode = RBX::GameBasicSettings::singleton().inStudioMode();
    RBX::GameBasicSettings::singleton().setStudioMode(false);

    // create instance for base plate
    boost::shared_ptr<RBX::Instance> instance = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
    RBXASSERT(instance);

	// set up part instance
    RBX::BaldPtr<RBX::PartInstance> partInstance = static_cast<RBX::PartInstance*>(instance.get());
    partInstance->setName("BasePlate");

	if (shared_ptr<RBX::BasicPartInstance> basicPartInstance = RBX::Instance::fastSharedDynamicCast<RBX::BasicPartInstance>(instance))
		basicPartInstance->setFormFactorXml(RBX::PartInstance::SYMETRIC);

	partInstance->setPartSizeUi(RBX::Vector3(512, 20, 512));
	partInstance->setTranslationUi(RBX::Vector3(0,-10,0));
    partInstance->setAnchored(true);
    partInstance->setPartLocked(true);
    partInstance->setColor(RBX::BrickColor::brickDarkGray());

    shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
    RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
	
	if (DFFlag::MaterialPropertiesEnabled && FFlag::EnablePhysicalPropertiesByDefault)
	{
		// All new places will have Physical Material Properties enabled
		dataModel->getWorkspace()->setPhysicalPropertiesMode(PhysicalPropertiesMode_NewPartProperties);
	}
	if (DFFlag::MaterialPropertiesEnabled)
	{
		RBXASSERT(!m_workspacePropertyChangedConnection.connected());
		m_workspacePropertyChangedConnection = getEditDataModel()->getWorkspace()->propertyChangedSignal.connect(boost::bind(&RobloxIDEDoc::onWorkspacePropertyChanged, this, _1));
	}

	RBX::Instances instances;
	instances.push_back(instance);
	dataModel->getWorkspace()->insertInstances(
        instances,
        NULL,
        RBX::INSERT_TO_TREE,
        RBX::SUPPRESS_PROMPTS);

    RBX::GameBasicSettings::singleton().setStudioMode(oldStudioMode);

    // zoom to a good spot
    RBX::Vector3 boxSize(15,15,15);
    RBX::Extents extents(-boxSize,boxSize);
    RBX::Camera* camera = dataModel->getWorkspace()->getCamera();
    camera->zoomExtents(extents,RBX::Camera::ZOOM_IN_OR_OUT);

	if(!FFlag::UserBetterInertialScrolling)
	{
		// adjust focus - zoom extents sets us in orbit cam
		RBX::CoordinateFrame frame = camera->getCameraCoordinateFrame();
		RBX::Vector3 look = frame.lookVector();
		camera->setCameraFocus(frame.translation + look * 2.0f);
	}

	// merge baseplate with change history baseline so we don't get an undo
    RBX::ChangeHistoryService* chs = dataModel->create<RBX::ChangeHistoryService>();
    chs->resetBaseWaypoint();

	// In addition to setting the replicated flag "expSolverEnabled_Replicate",
	// this also sets "experimentalSolverEnabled" (serialized flag) to true
	if (FFlag::PGSSolverDefaultOnNewPlaces)
		dataModel->getWorkspace()->setExpSolverEnabled_Replicate(true); 

	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		RBXASSERT(!m_dataModelItemChangedConnection.connected());
		m_dataModelItemChangedConnection = getEditDataModel()->itemChangedSignal.connect(boost::bind(&RobloxIDEDoc::onDMItemChanged, this, _1, _2));
	}
}

void RobloxIDEDoc::initializeDataModeHash()
{
	if (m_initialDataModelHash.isEmpty() && getDataModel())
	{
		RBX::DataModel::LegacyLock lock(getDataModel(), RBX::DataModelJob::Read);
		m_initialDataModelHash = StudioSerializerHelper::getDataModelHash(getDataModel());
	}
}

void RobloxIDEDoc::initializeRobloxView()
{
    try
    {
        RBX::DataModel::LegacyLock lock(m_CurrentGame->m_Game->getDataModel(), RBX::DataModelJob::Write);

        m_GameView = RobloxView::createGameView(m_pQOgreWidget);
        m_CurrentGame->m_View = new RobloxView(m_pQOgreWidget,m_CurrentGame->m_Game, m_GameView);
        
        m_CurrentGame->m_View->buildGui(StudioUtilities::isAvatarMode() || isCloudEditSession());

		// view has been created, initialize video recording
		initializeVideoRecording(*m_CurrentGame);
		UpdateUIManager::Instance().updateAction(*m_pMainWindow->toggleVideoRecordAction);
    }
    catch(CreatePopupException &e)
    {
        RBX::Log::current()->writeEntry(RBX::Log::Information,RBX::format("\tPopup Exception : %s",qPrintable(e.m_message)).c_str());

        QtUtilities::RBXMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setText(e.m_message);
        msgBox.exec();
    }
    catch (std::exception& e)
    {
        RBX::Log::current()->writeEntry(RBX::Log::Information,RBX::format("\tException : %s",e.what()).c_str());
        RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
    }

    m_pMainWindow->onIDEDocViewInitialized();
}

bool RobloxIDEDoc::open(RobloxMainWindow *pMainWindow, const QString &fileName)
{
    m_pMainWindow = pMainWindow;
    return openFile(fileName, false);
}

bool RobloxIDEDoc::openFile(const QString& fileName, bool asNew)
{
    if (!fileName.isEmpty())
    {
        QByteArray ba = fileName.toUtf8();
        std::string fileNameUtf8(ba.constData(), ba.size());
        std::ifstream stream(RBX::utf8_decode(fileNameUtf8).c_str(), std::ios_base::in | std::ios_base::binary);

        if (!stream.is_open())
            return false;

        return openStream(fileName, &stream, asNew);
    }

    return openStream(fileName, NULL, asNew);
}

static void doCloudEditFetch(const QString& scriptArg, int placeId, shared_ptr<EntityProperties> config,
	bool* abort, QDialog* dialog)
{
	StudioUtilities::reportCloudEditJoinEvent("Hitting PlaceLauncher");

    std::string url;
    if (FFlag::UseBuildGenericGameUrl)
    {
        url = BuildGenericGameUrl(RobloxSettings::getBaseURL().replace("http","https").toStdString(), QString("game/PlaceLauncher.ashx?request=CloudEdit&placeId=%1").arg(placeId).toStdString());
    }
    else
    {
        url = (RobloxSettings::getBaseURL()
              .replace("http", "https")
              + QString("/game/PlaceLauncher.ashx?request=CloudEdit&placeId=%1").arg(placeId)).toStdString();
    }
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Connecting to server...");
	while (!*abort)
	{
		EntityProperties p;
		p.setFromJsonFuture(RBX::HttpAsync::get(url));
		if (boost::optional<int> status = p.get<int>("status"))
		{
			// PlaceLauncher status code 2 == server is ready
			if (status.get() == 2 && !*abort)
			{
				config->setFromValueTable(p.get<shared_ptr<const RBX::Reflection::ValueTable> >("settings").get());
				QMetaObject::invokeMethod(dialog, "done", Qt::QueuedConnection, Q_ARG(int, QDialog::Accepted));
				break;
			}
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(500));
	}
}

static boost::optional<int> parseUrlParameter(QString parameterName, QString url)
{
	QRegExp matcher(QString("%1=(\\d+)").arg(parameterName));
	matcher.setCaseSensitivity(Qt::CaseInsensitive);
	if (matcher.indexIn(url) != -1)
	{
		return matcher.cap(1).toInt();
	}
	return boost::optional<int>();
}

shared_ptr<EntityProperties> RobloxIDEDoc::cloudEditDetectionAndPlaceLaunch()
{
	boost::optional<int> placeId;
	if ((RobloxUser::singleton().getUserId() > 0) && StudioUtilities::containsEditScript(m_initializationScript))
	{
		placeId = parseUrlParameter("placeid", m_initializationScript);
		RBXASSERT(placeId);

		boost::optional<int> universeId = parseUrlParameter("universeid", m_initializationScript);
		if (placeId && !universeId)
		{
			EntityProperties p;
			std::string url = ReplaceTopSubdomain(RobloxSettings::getBaseURL().toStdString(), "api") +
				format("/universes/get-universe-containing-place?placeId=%d", placeId.get());
			p.setFromJsonFuture(RBX::HttpAsync::get(url));
			if (boost::optional<int> placeUniverseId = p.get<int>("UniverseId"))
			{
				universeId = placeUniverseId.get();
			}
		}
		RBXASSERT(universeId);

		if (placeId && universeId)
		{
			std::string url = ReplaceTopSubdomain(RobloxSettings::getBaseURL().toStdString(), "api") +
				format("/universes/%d/cloudeditenabled", universeId.get());
			
			EntityProperties p;
			bool done = false;
			while (!done)
			{
				p.setFromJsonFuture(RBX::HttpAsync::get(url));
				if (boost::optional<bool> enabled = p.get<bool>("enabled"))
				{
					done = true;
					m_IsCloudEditSession = enabled.get();
				}
				else
				{
					if (FFlag::RetryWhenCloudEditEnabledEndpointFails)
					{
						StudioUtilities::reportCloudEditJoinEvent("Failed universes/cloudeditenabled");
						QtUtilities::RBXMessageBox warning;
						warning.setText("Failed to look up place's edit mode");
						warning.addButton("Check Again", QMessageBox::ResetRole);
						warning.addButton("Edit Alone", QMessageBox::AcceptRole);
						warning.exec();
						QMessageBox::ButtonRole role = warning.buttonRole(warning.clickedButton());
						if (role == QMessageBox::AcceptRole)
						{
							done = true;
							m_IsCloudEditSession = false;
						}
					}
					else
					{
						done = true;
						m_IsCloudEditSession = false;
					}
				}
			}
		}
		else
		{
			StudioUtilities::reportCloudEditJoinEvent("Unable to read universeid/placeid from edit script");
			StandardOut::singleton()->print(MESSAGE_ERROR, "Unable to determine place/universe, proceeding with Edit alone");
			placeId = 0;
			universeId = 0;
			m_IsCloudEditSession = false;
		}
	}
	else
	{
		// if it isn't an edit script it can't be a cloud edit session
		m_IsCloudEditSession = false;
	}
	RBXASSERT(m_IsCloudEditSession.is_initialized());

	shared_ptr<EntityProperties> cloudEditJoinConfig(new EntityProperties);
	if (isCloudEditSession())
	{
		QtUtilities::RBXMessageBox connectingDialog;
		connectingDialog.setText("Requesting access...");
		connectingDialog.setStandardButtons(QMessageBox::Cancel);
		bool abort = false;
		boost::thread httpThread(boost::bind(&doCloudEditFetch,
			boost::ref(m_initializationScript), placeId.get(), cloudEditJoinConfig, &abort, &connectingDialog));
		int status = connectingDialog.exec();
		if (status != QDialog::Accepted)
		{
			m_initializationScript = "";
			abort = true;
			StudioUtilities::reportCloudEditJoinEvent("User Aborted PlaceLauncher");
			httpThread.join();
		}
	}

	return cloudEditJoinConfig;
}

bool RobloxIDEDoc::openStream(const QString& fileName, std::istream* stream, bool asNew)
{
    RobloxMainWindow *pMainWindow = m_pMainWindow;
	clearOutputWidget();

	shared_ptr<EntityProperties> cloudEditJoinConfig = cloudEditDetectionAndPlaceLaunch();

	// Initialize our plugin host
	pMainWindow->getPluginHost()->init();

    QMutexLocker locker(&m_IOMutex);

    RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxIDEDoc::open - start");

	bool success = true;

    UpdateUIManager::Instance().setBusy(true);

    const static int ProgressStages = 7;
    RBX::BaldPtr<QProgressBar> progressBar = UpdateUIManager::Instance().showProgress("Opening Place",0,ProgressStages);
    bool isRemoteEditSession = false;
	
	try 
	{
        // Stage 0 - initialize

		m_pMainWindow = pMainWindow; 
		QString baseUrl = RobloxSettings::getBaseURL();
		if (!baseUrl.endsWith("/"))
		{
			baseUrl = baseUrl + "/";
		}

#ifdef STUDIO_ADMIN_BUILD
		if (fileName.indexOf("Join.ashx") != -1)
		{
			m_EditGame.m_Game.reset(new RBX::SecurePlayerGame(NULL, baseUrl.toStdString().c_str(), false));
		}
		else
        {
			m_EditGame.m_Game.reset(new RBX::UnsecuredStudioGame(NULL, baseUrl.toStdString().c_str(), StudioUtilities::isAvatarMode()));
        }
#else
        m_EditGame.m_Game.reset(new RBX::UnsecuredStudioGame(NULL,baseUrl.toStdString().c_str(), true));
#endif

		RBX::Log::current()->writeEntry(RBX::Log::Information,"\tLocking DataModel");
		RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);

		createOptionalDMServices();

        // Stage 1 - initialize widgets/graphics/view
        UpdateUIManager::Instance().updateProgress(progressBar,1);

		// TODO - when opening up a place from the website, get the real place name for
		//  the display name

		m_EditGame.m_Game->getDataModel()->setName(displayName().toStdString());

		m_WrapperWidget = new QWidget;      //hack to display Ogre window in a tab
		QWidget *pDocWidgetToAdd = NULL;

		RBX::CRenderSettings::GraphicsMode graphicsMode = CRenderSettingsItem::singleton().getLatchedGraphicsMode();
		if (graphicsMode != RBX::CRenderSettings::NoGraphics)
		{
            RBX::Log::current()->writeEntry(RBX::Log::Information,"\tInitializing Ogre");
		    m_pQOgreWidget = new QOgreWidget("QOgreWidget", m_WrapperWidget);

            if (m_pMainWindow->getBuildMode() == BM_BASIC)
                m_WrapperWidget->setMinimumSize(QSize(800, 600));
            else
                m_WrapperWidget->setMinimumSize(m_pQOgreWidget->minimumSize());

#ifdef _WIN32
			//if the application is minimized then make sure m_WrapperWidget is shown (fix for DE5666)
			if (m_pMainWindow->windowState() & Qt::WindowMinimized)
			{
				m_WrapperWidget->move(m_pMainWindow->width(), m_pMainWindow->height());
				m_WrapperWidget->setVisible(true);
			}
#endif

            RBX::Log::current()->writeEntry(RBX::Log::Information,"\tInitializing View");

			pDocWidgetToAdd = m_pQOgreWidget;
		}
		else
		{
			m_pDiagViewWidget = new RobloxDiagnosticsView(true);
			RBX::Profiling::setEnabled(true);
			pDocWidgetToAdd = m_pDiagViewWidget;
		}


		// Stage 2 - initialize controls, layout
		if (DFInt::StudioWorkspaceNotificationLevel || DFFlag::WorkspaceNotificationMasterEnable)
		{
			UpdateUIManager::Instance().updateProgress(progressBar,2);
			m_AnnouncementWidget = new WorkspaceAnnouncementTooltip(m_WrapperWidget);

			QVBoxLayout *renderWindowLayout = new QVBoxLayout(m_WrapperWidget);
			renderWindowLayout->setSpacing(0);
			renderWindowLayout->setContentsMargins(0,0,0,0);
			renderWindowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Maximum,QSizePolicy::Maximum));
			renderWindowLayout->addWidget(m_AnnouncementWidget);
			renderWindowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Maximum,QSizePolicy::Maximum));
			renderWindowLayout->addWidget(pDocWidgetToAdd);
			m_WrapperWidget->setLayout(renderWindowLayout);
		}
		else
		{
			UpdateUIManager::Instance().updateProgress(progressBar,2);

			QHBoxLayout *renderWindowLayout = new QHBoxLayout(m_WrapperWidget);
			renderWindowLayout->setSpacing(0);
			renderWindowLayout->setContentsMargins(0,0,0,0);
			renderWindowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Maximum,QSizePolicy::Maximum));
			renderWindowLayout->addWidget(pDocWidgetToAdd);
			renderWindowLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Maximum,QSizePolicy::Maximum));

			m_WrapperWidget->setLayout(renderWindowLayout);
		}
		if (StudioUtilities::isAvatarMode())
			StudioDeviceEmulator::Instance().activateCurrentDevice();

        createActionIDVerbMap(m_EditGame);

        RBX::Log::current()->writeEntry(RBX::Log::Information,"\tInitializing Explorer");
		m_EditGame.m_TreeWidget = new RobloxTreeWidget(m_EditGame.m_Game->getDataModel());

		UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).setCurrentWidget(m_EditGame.m_TreeWidget);

        // Stage 3 - load file
        UpdateUIManager::Instance().updateProgress(progressBar,3);

        m_RequiresSave = false;

        RBX::Log::current()->writeEntry(RBX::Log::Information,RBX::format("\tLoading File : %s",qPrintable(fileName)).c_str());

        if (m_initializationScript.isEmpty())
        {
            QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
                                      "nonGameLoaded", Qt::QueuedConnection);
        }

        if (fileName.isEmpty() && m_initializationScript.isEmpty())
        {
            //init debugger data
            initializeDebuggerData(fileName, m_EditGame.m_Game->getDataModel());
            sIDEDocCount++;
        }
        else
        {
            if (!asNew)
                m_fileName = fileName;

            loadFromStream(stream);

            if (!m_initializationScript.isEmpty())
            {
                sIDEDocCount++;
                m_bIsLocalDocument = false;

                if (m_initializationScript.contains("testmode=false"))
                    m_TestModeEnabled = false;

				// make sure we are authenticated before loading initialization scripts				
				AuthenticationHelper::Instance().waitForHttpAuthentication();

				if (isCloudEditSession())
				{
					// Reset synchronized flags, they should be set by the server
					FLog::ResetSynchronizedVariablesState();
                        
					boost::thread(boost::bind(&StudioUtilities::joinCloudEditSession, 
												m_EditGame.m_Game, 
												cloudEditJoinConfig));

					initCloudEditObjects();
				}
				else if (RBX::ContentProvider::isUrl(m_initializationScript.toStdString()))
                {
					 if (StudioUtilities::containsEditScript(m_initializationScript))
                    {
						// if in EDIT mode, connect with workspaceLoadedSignal, so we can initialize debugger data
                        m_contentDataLoadedConnection = m_EditGame.m_Game->getDataModel()->workspaceLoadedSignal.connect(boost::bind(
                                                                                                                                     &RobloxIDEDoc::onContentDataLoaded,
                                                                                                                                     this));
                        isRemoteEditSession = true;
                    }

					if (StudioUtilities::containsJoinScript(m_initializationScript))
                    {
                        // enable FRM in play mode
                        CRenderSettingsItem::singleton().setEnableFRM(true);

                        // Reset synchronized flags, they should be set by the server
                        FLog::ResetSynchronizedVariablesState();

						if (StudioUtilities::isTestMode())
						{
							if (RBX::Network::Players* players = RBX::ServiceProvider::create<RBX::Network::Players>(getDataModel()))
							{
								m_localPlayerConnection = players->onDemandWrite()->childAddedSignal.connect( 
									                                                boost::bind(&RobloxIDEDoc::onLocalPlayerAdded, this, _1) );
							}
						}
                        
                        boost::thread(boost::bind(
                                                  &StudioUtilities::executeURLJoinScript, 
                                                  m_EditGame.m_Game, 
                                                  m_initializationScript.toStdString()));
                    }
                    else
                    {
                        boost::thread(boost::bind(
                                                  &StudioUtilities::executeURLScript, 
                                                  m_EditGame.m_Game->getDataModel(), 
                                                  m_initializationScript.toStdString()));
                    }
                }
                else
                {
                    handleScriptCommand(m_initializationScript);
                }
            }
        }
        
		CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(getDataModel());
		NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(getDataModel());
		dictionaryService->refreshRefCount();
		nrDictionaryService->refreshRefCount();

        // Stage 4 - initialize terrain/undo
        UpdateUIManager::Instance().updateProgress(progressBar,4);

		// enable dock views (must be done before enabling undo/redo)
		setDockViewsEnabled();
		setTeamCreateWidgetEnabled(true);

		if (m_bIsLocalDocument)
		{
			RBX::Log::current()->writeEntry(RBX::Log::Information,"\tInitializing Terrain");
			m_EditGame.m_Game->getDataModel()->getWorkspace()->createTerrain();

			// initialize PlayerScripts
			StarterPlayerService* starterPlayerService = ServiceProvider::find<StarterPlayerService>(m_EditGame.m_Game->getDataModel()->getWorkspace());    
			if (starterPlayerService)
				starterPlayerService->setupPlayerScripts();

            RBX::Log::current()->writeEntry(RBX::Log::Information,"\tInitializing Undo");
            enableUndo(m_pMainWindow->getBuildMode() == BM_ADVANCED);

			// enable dynamic lighting for new places
			if (fileName.isEmpty())
			{
				if (RBX::Lighting* lighting = RBX::ServiceProvider::create<RBX::Lighting>(m_EditGame.m_Game->getDataModel().get()))
				{
					lighting->setGlobalShadows(true);
					lighting->setGlobalAmbient(G3D::Color3(0,0,0));
				}
			}

            // enable smooth terrain for new places
            if (fileName.isEmpty())
            {
                RBX::MegaClusterInstance* terrain = boost::polymorphic_downcast<RBX::MegaClusterInstance*>(m_EditGame.m_Game->getDataModel()->getWorkspace()->getTerrain());

                terrain->convertToSmooth();
            }
		}

		// Stage 5 - load plugins
        UpdateUIManager::Instance().updateProgress(progressBar,5);

		// finally load in plugins (we want these in local and online docs)
        RBX::Log::current()->writeEntry(RBX::Log::Information,"\tLoading Plugins");
		RobloxPluginHost::loadPlugins(m_EditGame.m_Game->getDataModel().get());

        // Stage 6 - finish initialization
        UpdateUIManager::Instance().updateProgress(progressBar,6);

		m_EditGame.m_Game->getDataModel()->setDirty(false);
        m_RequiresSave = false;

		m_pRSFHandler.reset(new RecentlySavedFilesHandler());

		// restore in-game stats display
		RobloxSettings settings;
        setStatsEnabled(StatsVerb,settings.value(StatsVerb,false).toBool(), m_pMainWindow->testStatsAction);
        setStatsEnabled(RenderStatsVerb,settings.value(RenderStatsVerb,false).toBool(), m_pMainWindow->testRenderStatsAction);
        setStatsEnabled(PhysicsStatsVerb,settings.value(PhysicsStatsVerb,false).toBool(), m_pMainWindow->testPhysicsStatsAction);
        setStatsEnabled(NetworkStatsVerb,settings.value(NetworkStatsVerb,false).toBool(), m_pMainWindow->testNetworkStatsAction);
        setStatsEnabled(SummaryStatsVerb,settings.value(SummaryStatsVerb,false).toBool(), m_pMainWindow->testSummaryStatsAction);
		if (FFlag::StudioCustomStatsEnabled)
		{
			setStatsEnabled(CustomStatsVerb,settings.value(CustomStatsVerb,false).toBool(), m_pMainWindow->testCustomStatsAction);
		}
	
		// for now assume qt docs always have access to keyboard/mouse (we should handle this somewhere else in the future)
		RBX::UserInputService* userInputService = m_EditGame.m_Game->getDataModel()->find<RBX::UserInputService>();
		if (userInputService != NULL)
		{
			userInputService->setMouseEnabled(true);
			userInputService->setKeyboardEnabled(true);
		}

		enableDownloadPlaceCopy(isCloudEditSession());

		QDateTime lastDateTimeOpen = settings.value("LastIDEDocOpen").toDateTime();
		if (lastDateTimeOpen.isNull())
        {
            UpdateUIManager::Instance().setDockVisibility(eDW_TOOLBOX, true);
			UpdateUIManager::Instance().setDockVisibility(eDW_TUTORIALS, FFlag::StudioShowTutorialsByDefault);
		}
		settings.setValue("LastIDEDocOpen", QDateTime::currentDateTime());

		// Fire off pre-fetching
		if (FFlag::PrefetchResourcesEnabled)
        {
            RBX::Log::current()->writeEntry(RBX::Log::Information,"\tPrefetch Resources");
			preFetchResources();
        }

        RBXASSERT(!m_bIsLocalDocument || !isRemoteEditSession);
        if (m_bIsLocalDocument || isRemoteEditSession)
        {
            RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "BeginEdit",
                m_bIsLocalDocument ? "LocalDocument" : "RemoteBackedDocument");
        }

		if (m_AnnouncementWidget)
		{
			displayWorkspaceMessage();
		}

        // Stage 7 - Done!
        UpdateUIManager::Instance().updateProgress(progressBar,7);

        RBX::Log::current()->writeEntry(RBX::Log::Information,"\tOpen Success");
	}
	catch(CreatePopupException &e)
	{
        RBX::Log::current()->writeEntry(RBX::Log::Information,RBX::format("\tPopup Exception : %s",qPrintable(e.m_message)).c_str());

		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.setText(e.m_message);
		msgBox.exec();

		success = false;
	}
	catch (std::exception& e)
	{
        RBX::Log::current()->writeEntry(RBX::Log::Information,RBX::format("\tException : %s",e.what()).c_str());
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		success = false;
	}

    UpdateUIManager::Instance().hideProgress(progressBar);
    UpdateUIManager::Instance().setBusy(false);

	// Be sure not to do this while studio is busy, or the enabled state will get lost
	if (m_initializationScript.isEmpty())
    {
        UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER).nonGameLoaded();
    }

    // make sure that we're not dirty, especially if we get any sort of error
    // if it's dirty, the user will be prompted to save
	m_RequiresSave = false;
    if (m_EditGame.m_Game && m_EditGame.m_Game->getDataModel())
        m_EditGame.m_Game->getDataModel()->setDirty(false);

    if (!m_pFileWatcher && CRenderSettingsItem::singleton().getDebugReloadAssets())
    {
        m_pFileWatcher = new FW::FileWatcher();
        // assets
        m_pFileWatcher->addWatch(ContentProvider::assetFolder(), this, true);
        // platform dependent assets
        m_pFileWatcher->addWatch(ContentProvider::platformAssetFolder(), this, true);
        // and shaders, Oh yeah!
        std::string shaderFolder = QDir::cleanPath(QString((ContentProvider::assetFolder() + "/../shaders/source").c_str())).toStdString();
        m_pFileWatcher->addWatch(shaderFolder, this, false);
    }

    RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxIDEDoc::open - end");
	return success;
}

void RobloxIDEDoc::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
{
    // we dont care about deleted file, we'll keep current data in that case
    if (action == FW::Actions::Delete)
        return;

    std::string simpleFilename = QDir::fromNativeSeparators(QString(filename.c_str())).toStdString();
    std::string absoluteDir = QDir::fromNativeSeparators(QString(dir.c_str())).toStdString();
    std::string contentFolder = QDir::fromNativeSeparators(QString(ContentProvider::assetFolder().c_str())).toStdString();
    std::string platformContentFolder = QDir::fromNativeSeparators(QString(ContentProvider::platformAssetFolder().c_str())).toStdString();

    std::string pathStr;

    if (absoluteDir.find(contentFolder) == 0 || absoluteDir.find(platformContentFolder) == 0)
        pathStr = "rbxasset://" + simpleFilename;
    else
        pathStr = simpleFilename;

    RBX::ViewBase* pView = (&m_CurrentGame->m_View->getViewBase());
    pView->queueAssetReload(pathStr);
}

void RobloxIDEDoc::exportSelection(std::string filePath, ExporterSaveType exportType)
{
	RBX::ViewBase* pView = (&m_CurrentGame->m_View->getViewBase());

	QString fileName = filePath.c_str();

	if (fileName.isEmpty())
		fileName = QFileDialog::getSaveFileName(&UpdateUIManager::Instance().getMainWindow(), tr("Export Place"), "", "Object Model (*.obj)");

	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);

		if (fileInfo.suffix() == "obj")
		{
			QString exportAction = "Exporting Place";
			if (exportType == ExporterSaveType_Selection)
				exportAction = "Exporting Selection";

			UpdateUIManager::Instance().setBusy(true);
			RBX::BaldPtr<QProgressBar> progressBar = UpdateUIManager::Instance().showProgress(exportAction);
			UpdateUIManager::Instance().updateProgress(progressBar);
			pView->exportScene(fileInfo.absolutePath().toStdString() + "/" + fileInfo.baseName().toStdString(), exportType, ExporterFormat_Obj);
			UpdateUIManager::Instance().hideProgress(progressBar);
			UpdateUIManager::Instance().setBusy(false);
			
		}
		else
		{
			QMessageBox::critical(
				m_pMainWindow,
				"Export Failure",
				QString("Obj filetype expected. '%1' received.").arg(fileInfo.suffix()));
		}
	}
}

void RobloxIDEDoc::onDMItemChanged(shared_ptr<RBX::Instance> item, const RBX::Reflection::PropertyDescriptor* descriptor)
{
	if (descriptor == &RBX::PartOperation::prop_CollisionFidelity)
	{
		if (PartOperation* partOp = item->fastDynamicCast<PartOperation>())
		{
			if (CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(partOp)))
			{
				if (!RBX::DataModel::get(partOp)->isRunMode())
				{
					partOp->createPhysicsData(partOp->getMesh().get());
				}
			}
		}
	}
}

void RobloxIDEDoc::enableUndo(bool enable)
{
    //undo redo manager.
	RBX::ChangeHistoryService* chs = m_CurrentGame->m_Game->getDataModel()->create<RBX::ChangeHistoryService>();
	chs->setEnabled(true);

    if (enable)
        m_CurrentGame->m_ChangeHistoryConnection = chs->waypointChangedSignal.connect(boost::bind(&RobloxIDEDoc::onWaypointChanged,this));
    else
        m_CurrentGame->m_ChangeHistoryConnection.disconnect();
}

IRobloxDoc::RBXCloseRequest RobloxIDEDoc::requestClose()
{
	RBX::DataModel::RequestShutdownResult shutdownRequestResult;
	{
		RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);
		// If we're in full screen build mode, call back to LUA for save as opposed to doing QT save dialog
		shutdownRequestResult = m_EditGame.m_Game->getDataModel()->requestShutdown(m_pMainWindow->getBuildMode() == BM_BASIC);
	}
	
	switch (shutdownRequestResult)
	{
		case RBX::DataModel::CLOSE_REQUEST_HANDLED:
			return IRobloxDoc::REQUEST_HANDLED;
		case RBX::DataModel::CLOSE_NOT_HANDLED:
			break;
		case RBX::DataModel::CLOSE_NO_SAVE_NEEDED:
            if (!m_TestModeEnabled)
                return IRobloxDoc::LOCAL_SAVE_NEEDED;
			return IRobloxDoc::NO_SAVE_NEEDED;
		case RBX::DataModel::CLOSE_LOCAL_SAVE:
			return IRobloxDoc::LOCAL_SAVE_NEEDED;
	}

	KeepSavedChangesDialog* keepSavedChangesDialog = new KeepSavedChangesDialog(m_pMainWindow);
	keepSavedChangesDialog->exec();
	KeepSavedChangesDialog::DialogResult result = keepSavedChangesDialog->getResult();
	delete keepSavedChangesDialog;

	switch (result)
	{
	case KeepSavedChangesDialog::Fail:
		{                
			QtUtilities::RBXMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.setText("Failed to save place online.  Check your connection and try again.");
			msgBox.exec();
			return IRobloxDoc::CLOSE_CANCELED;
		}
	case KeepSavedChangesDialog::Discard:
	case KeepSavedChangesDialog::Save:
		{
			return IRobloxDoc::NO_SAVE_NEEDED;
		}
	case KeepSavedChangesDialog::Cancel:
	default:
		{
			return IRobloxDoc::CLOSE_CANCELED;
		}
    }

    return IRobloxDoc::NO_SAVE_NEEDED;
}

bool RobloxIDEDoc::doClose()
{
	MobileDevelopmentDeployer::singleton().disconnectClientsAndShutDown();

	//reset debugger data
	if (FFlag::LuaDebugger)
	{
		RBX::Scripting::DebuggerManager::singleton().setDataModel(NULL);
		DebuggerClientManager::Instance().setDataModel(boost::shared_ptr<RBX::DataModel>());
		//make sure we remove debugger file if we are in test mode
		if (StudioUtilities::isTestMode())
			 QFile::remove(getDebugInfoFile());
		m_EditDebuggersMap.clear();
	}

	if (m_pRSFHandler)
    {
		shared_ptr<RBX::DataModel> dataModel = getEditDataModel();
		if (dataModel && dataModel->isDirty())
		{
			QString fileToSave = getRecentSaveFile();
			if (!fileToSave.isEmpty() && StudioSerializerHelper::saveAsIfModified(m_initialDataModelHash, dataModel.get(), fileToSave))
				m_pRSFHandler->addFile(fileToSave);
		}
	}

	if (m_AnnouncementWidget)
	{
		m_AnnouncementWidget->deleteLater();
		m_AnnouncementWidget = NULL;
	}

	if (m_pQOgreWidget)
	{
	    m_pQOgreWidget->setRobloxView(NULL);
	    m_pQOgreWidget->deleteLater();
	    m_pQOgreWidget = NULL;
	}

	if (m_pDiagViewWidget)
	{
		m_pDiagViewWidget->setDataModel(boost::shared_ptr<RBX::DataModel>());
		m_pDiagViewWidget->deleteLater();
		m_pDiagViewWidget = NULL;
	}

    if (m_WrapperWidget)
    {
		m_WrapperWidget->deleteLater();
		m_WrapperWidget = NULL;
    }

	setTeamCreateWidgetEnabled(false);

    cleanupGameState(m_PlayGame);
    cleanupGameState(m_EditGame);

	UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).setCurrentWidget(NULL);

    // only delete autosave if we have an autosave and we did not load from an autosave
	//  if we loaded from an autosave, we're going to keep using the same autosave but we're
	//  not going to blow it away on close.
	FASTLOGS(FLog::AutoSave, "Autosave name: %s", m_AutoSaveName.toStdString());
	FASTLOG1(FLog::AutoSave, "Is auto save loaded: %u", m_IsAutoSaveLoad);

    if (!m_AutoSaveName.isEmpty() && !m_IsAutoSaveLoad)
    {
		FASTLOG(FLog::AutoSave, "Removing auto save on successful exit");
        QFile::remove(m_AutoSaveName);
		//also remove the auto save debug file
		if (FFlag::LuaDebugger)
			 QFile::remove(getDebugInfoFile(true));
        m_AutoSaveName.clear();
        m_IsAutoSaveLoad = false;
    }

	enableDownloadPlaceCopy(false);

	UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER).onCloseIdeDoc();

    if (m_pFileWatcher)
    {
        delete m_pFileWatcher;
        m_pFileWatcher = NULL;
    }

	//self deletion (maybe we must use reference counting!)
	delete this;
	return true;
}

void RobloxIDEDoc::cleanupGameState(GameState& gameState)
{
    m_CurrentGame = &gameState;

    gameState.m_ChangeHistoryConnection.disconnect();

	if (gameState.m_Game && gameState.m_Game->getDataModel())
    {
		// cleanup all dock views        
		setDockViewsEnabled(false);

        // clean up verbs
        shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
		RBX::DataModel::LegacyLock lock(dataModel,RBX::DataModelJob::Write);	
		QVector<RBX::Verb*>::iterator iter = gameState.m_Verbs.begin();
		while (iter != gameState.m_Verbs.end())
		{
			delete *iter;
			++iter;
		}

        gameState.m_Verbs.clear();
		gameState.m_UndoVerb = NULL;
		gameState.m_RedoVerb = NULL;
#ifdef _WIN32
		gameState.m_RecordToggleVerb = NULL;
#endif
		dataModel->getWorkspace()->getAdornableCollector()->onRenderableDescendantRemoving(&gameState.m_SelectionHighlightAdornable);
	}

	// remove tree view
	if (gameState.m_TreeWidget)
	{
		m_pMainWindow->treeWidgetStack().removeWidget(gameState.m_TreeWidget);
		gameState.m_TreeWidget->requestDelete();
		gameState.m_TreeWidget = NULL;
		UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).setCurrentWidget(NULL);
	}

    // shut down UI and plugins
    if (gameState.m_Game && gameState.m_Game->getDataModel())
	{
        shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
		{
			RBX::DataModel::LegacyLock lock(dataModel,RBX::DataModelJob::Write);
			
			if (gameState.m_View)
			{
				delete gameState.m_View;
				gameState.m_View = NULL;
			}

		}
		
		gameState.m_Game->shutdown();

		// Clean up plugins after the datamodel has closed (closing the datamodel ensures that
		// no more plugin scripts are running)
		RBX::PluginManager::singleton()->deletePlugins(dataModel.get());

		gameState.m_Game.reset();
	}
}

bool RobloxIDEDoc::loadFromStream(std::istream* stream)
{
    if (!stream)
        return false;

    RBX::Security::Impersonator impersonate(RBX::Security::COM);
	shared_ptr<DataModel> dm = m_EditGame.m_Game->getDataModel();
    RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);
    Serializer serializer;
    serializer.load(*stream, dm.get());

	// For live games, the gameserver script sets the place id and universe id
	// before calling game:Load(placeid). loadFromStream() is taking the place of
	// game:Load(), and game loading needs to happen before gameserver.ashx finishes,
	// so as a workaround we use this attempt at url parsing to pull the place and
	// universe id out of the gameserver script request so that they are set in the
	// data model before we attempt load linked scripts.
	QRegExp universeIdInGameServer(".*gameserver\\.ashx.*\\((\\d+).*(\\d+)\\).*");
	universeIdInGameServer.setCaseSensitivity(Qt::CaseInsensitive);
	if (universeIdInGameServer.indexIn(m_initializationScript) != -1)
	{
		int placeId = universeIdInGameServer.cap(1).toInt();
		int universeId = universeIdInGameServer.cap(2).toInt();
		dm->setPlaceID(placeId, false);
		dm->setUniverseId(universeId);
	}

	LuaSourceContainer::blockingLoadLinkedScripts(dm->create<ContentProvider>(), dm.get());

    //init debugger data (in local document or play solo mode)
	if (m_initializationScript.isEmpty() || StudioUtilities::containsVisitScript(m_initializationScript))
        initializeDebuggerData(getDebugInfoFile(), dm);

    dm->processAfterLoad();


	if (DFFlag::MaterialPropertiesEnabled)
	{
		RBXASSERT(!m_workspacePropertyChangedConnection.connected());
		m_workspacePropertyChangedConnection = getEditDataModel()->getWorkspace()->propertyChangedSignal.connect(boost::bind(&RobloxIDEDoc::onWorkspacePropertyChanged, this, _1));
	}

    //Migrating CSG childData
    CSGDictionaryService* dictionaryService = dm->create<CSGDictionaryService>();
    ChangeHistoryService* changeHistory = dm->create<ChangeHistoryService>();

	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		RBXASSERT(!m_dataModelItemChangedConnection.connected());
		m_dataModelItemChangedConnection = getEditDataModel()->itemChangedSignal.connect(boost::bind(&RobloxIDEDoc::onDMItemChanged, this, _1, _2));
	}

    dictionaryService->reparentAllChildData();

    changeHistory->resetBaseWaypoint();

	searchToolboxByDefaultSearchString();

	return true;
}

void RobloxIDEDoc::preFetchResources()
{
    RBX::Verb* verb = getVerb(m_EditGame,"publishToRobloxAsAction");
	if(verb) {
		PublishToRobloxAsVerb* publishVerb = static_cast<PublishToRobloxAsVerb*>(verb);
		publishVerb->initDialog();
	}
}

QString RobloxIDEDoc::displayName() const
{
	if (m_fileName.isEmpty())
		return m_displayName;

	if (StudioUtilities::isTestMode() && !StudioUtilities::isAvatarMode() && StudioUtilities::containsGameServerScript(m_initializationScript))		
		return "Server";

	return QFileInfo(m_fileName).fileName();
}

QString RobloxIDEDoc::windowTitle() const 
{
	if (m_windowTitlePrefix.isEmpty() && m_initializationScript.isEmpty())
	{
		// local file
		if (!m_fileName.isEmpty())
			return QDir::toNativeSeparators(m_fileName);

		// new file
		if (m_initializationScript.isEmpty())
			return kNewPlaceWindowTitle;
	}
	
	QString title = displayName();
	title.prepend(m_windowTitlePrefix);
	// remove appended time stamp if any from the title
	int index = title.lastIndexOf(" @ ");
	if (index > 0)
		title = title.left(index);
	return title;
}

QWidget* RobloxIDEDoc::getViewer()
{
	return m_WrapperWidget;
}

/**
 * Checks if the data model is dirty or if we require a save.
 *  The data model might not be dirty if we auto-saved.
 */
bool RobloxIDEDoc::isModified()
{
    applyScriptChanges();

	if (StudioUtilities::isTestMode())
		return false;
    else if (m_RequiresSave)
        return true;

	// do not show save as dialog for cloud edit
	if (isCloudEditSession())
		return false;

	shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
	if (!dataModel)
	{
		RBXASSERT(false);
		return false;
	}

	RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Read);
	return dataModel->isDirty() && m_initialDataModelHash != StudioSerializerHelper::getDataModelHash(dataModel.get());
}

/**
 * Checks if the game is currently stopped.
 *  Paused is still considered "running".
 */
bool RobloxIDEDoc::isSimulating()
{
	if (!m_PlayGame.m_Game || !m_PlayGame.m_Game->getDataModel() || !m_PlayGame.m_Game->getDataModel()->create<RBX::RunService>())
		return false;

	RBX::RunState runState = m_PlayGame.m_Game->getDataModel()->create<RBX::RunService>()->getRunState();
	return runState != RBX::RS_STOPPED;
}

bool RobloxIDEDoc::save()
{
    bool result = saveAs(m_fileName);
    m_EditGame.m_Game->getDataModel()->setDirty(!result);
	return result;
}

void RobloxIDEDoc::createScriptMapping(shared_ptr<RBX::Instance> instance)
{
	if (RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance) || RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance))
	{
        addScriptMapping(instance, m_PlayGame.m_Game->getDataModel());
	}
}

void RobloxIDEDoc::restoreEditModelScripts()
{
    RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);

	// Copy the open script docs list because each script doc will have its script
	// reset, and resetting the script changes the master script doc list (mutating the list
	// while iterating through it).
	const tScriptDocs openScriptDocs = RobloxDocManager::Instance().getOpenScriptDocs();
    
	for (tScriptDocs::const_iterator iter = openScriptDocs.begin(); iter != openScriptDocs.end() ; ++iter)
	{
		shared_ptr<RBX::Instance> playInstance = (*iter)->getCurrentScript().toInstance();
		if (playInstance == NULL)
		{
			continue;
		}

		shared_ptr<RBX::Instance> instance = getEditScriptByPlayInstance(playInstance.get());
        
		if (instance && instance->isDescendantOf(m_EditGame.m_Game->getDataModel().get()))
		{
			LuaSourceBuffer buffer = LuaSourceBuffer::fromInstance(instance);
            
			(*iter)->setScript(m_EditGame.m_Game->getDataModel(), buffer);
		}
		else
		{
			// use RobloxDocManager::removeDoc for proper cleanup of documents
			if ((*iter) == RobloxDocManager::Instance().getCurrentDoc())
			{
				// if script doc is current then make sure we ignore IDE doc activate while removing script doc
				// IDE doc will be activated in RobloxIDEDoc::closePlayDataModel
				RBX::ScopedAssign<bool> ignoreDocActivate(m_bActive, true);
				RobloxDocManager::Instance().removeDoc(**iter);
			}
			else
			{
				RobloxDocManager::Instance().removeDoc(**iter);
			}
		}
	}
    
    clearScriptMap();
}

void RobloxIDEDoc::patchPlayModelScripts()
{
    // Build a script mapping for each script in the Edit Data Model.
    RBX::DataModel* dm = m_EditGame.m_Game->getDataModel().get();
    dm->visitDescendants(boost::bind(&RobloxIDEDoc::createScriptMapping, this, _1));
   
    // Replace Edit scripts with Play scripts.
	// Copy the open script docs list because each script doc will have its script
	// reset, and resetting the script changes the master script doc list (mutating the list
	// while iterating through it).
	const tScriptDocs openScriptDocs = RobloxDocManager::Instance().getOpenScriptDocs();

	for (tScriptDocs::const_iterator iter = openScriptDocs.begin(); iter != openScriptDocs.end() ; ++iter)
	{
		shared_ptr<RBX::Instance> backingInstance = (*iter)->getCurrentScript().toInstance();
		if (backingInstance == NULL)
		{
			continue;
		}

		std::vector<int> indexHierarchy;
		std::string serviceName = getScriptIndexHierarchy(backingInstance, indexHierarchy);
		shared_ptr<RBX::Instance> instance = getScriptByIndexHierarchy(m_PlayGame.m_Game->getDataModel(), indexHierarchy, serviceName);

		if (instance)
		{
			LuaSourceBuffer buffer = LuaSourceBuffer::fromInstance(instance);
			if (!buffer.empty())
				(*iter)->setScript(m_PlayGame.m_Game->getDataModel(), buffer);
		}
	}
}

bool RobloxIDEDoc::saveScriptsChangedWhileTesting()
{
	bool saveChanges = true;
    tPlayModifiedScriptList& playModifiedScriptList = getPlayModifiedScriptList();
    
    if (playModifiedScriptList.size() > 0 && !isCloudEditSession())
    {
		saveChanges = AuthoringSettings::singleton().alwaysSaveScriptChangesWhileRunning;
        
        if (!saveChanges)
        {
            UpdateUIManager::Instance().setBusy(true, false);
            QtUtilities::RBXMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setText("You've made script changes while playing.  Would you like to keep these changes?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            QPushButton *alwaysButton = msgBox.addButton(tr("Always Save"), QMessageBox::ActionRole);
            int ret = msgBox.exec();
            UpdateUIManager::Instance().setBusy(false, false);

            if (msgBox.clickedButton() == alwaysButton)
            {
                AuthoringSettings::singleton().alwaysSaveScriptChangesWhileRunning = true;
                RBX::GlobalAdvancedSettings::singleton()->saveState();
                RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_COUNTERS, "LiveCoding", "SaveDialogAlways", 1);
                saveChanges = true;
            }

            if (ret == QMessageBox::Yes)
            {
                RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_COUNTERS, "LiveCoding", "SaveDialogYes", 1);
                saveChanges = true;
            }
        }
        
        if (saveChanges)
        {
            RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_COUNTERS, "LiveCoding", "Saves", 1);
            applyScriptChanges();
            
            RBX::DataModel::LegacyLock editLock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);
            
            for (tPlayModifiedScriptList::iterator iter = playModifiedScriptList.begin(); iter != playModifiedScriptList.end(); ++iter)
            {
                shared_ptr<RBX::Instance> instance = getEditScriptByPlayInstance((*iter)->instance.get());
                
                if (!instance)
                    continue;
                
                LuaSourceBuffer sourceBuffer = LuaSourceBuffer::fromInstance((*iter)->instance);
                LuaSourceBuffer destBuffer = LuaSourceBuffer::fromInstance(instance);
                
                destBuffer.setScriptText(sourceBuffer.getScriptText());
            }
        }
        clearPlayModifiedScriptList();
    }

	return saveChanges;
}

bool RobloxIDEDoc::saveAs(const QString& fileName)
{
    QMutexLocker locker(&m_IOMutex);

    saveScriptsChangedWhileTesting();

	const bool useBinaryFormat = !(fileName.endsWith(".rbxlx", Qt::CaseInsensitive));

	const bool isAutoSave = fileName == m_AutoSaveName;
	if(isAutoSave)
	{
		FASTLOGS(FLog::AutoSave, "Saving AutoSave, filename: %s", fileName.toStdString());
	}

	shared_ptr<RBX::Game> game = m_EditGame.m_Game;

	QString errorMessage;
    bool result = StudioSerializerHelper::saveAs(
            fileName,
            isAutoSave ? "Auto-Saving" : "Saving",
            isAutoSave,
            useBinaryFormat,
            game->getDataModel().get(),
            errorMessage,
            !isAutoSave);

	if (result)
	{
		// modify dirty flag only for local document, we need to force saving in case of Build/Edit (DE4302)
		if (m_bIsLocalDocument)
        {
            if (game->getDataModel()->isDirty())
                m_RequiresSave = (fileName == m_AutoSaveName);

            if (!FFlag::NoPromptOnAlreadySavedPlace)
            {
                if (game->getDataModel()->isDirty() && (fileName != m_AutoSaveName))
                    resetDirty(game->getDataModel().get());
            }
        }
        
        if (FFlag::NoPromptOnAlreadySavedPlace)
        {
            if (game->getDataModel()->isDirty() && (fileName != m_AutoSaveName))
                resetDirty(game->getDataModel().get());
        }

		// if we changed file names and it's not the autosave file name
		if (fileName != m_fileName && fileName != m_AutoSaveName && !isCloudEditSession())
        {
			FASTLOGS(FLog::AutoSave, "Filename changed, remove auto save. New file: %s", fileName.toStdString());
			FASTLOGS(FLog::AutoSave, "Old file: %s", m_fileName.toStdString());
			FASTLOGS(FLog::AutoSave, "Old autoSave file: %s", m_AutoSaveName.toStdString());

			// if we have an autosave, we can delete it now
            if (!m_AutoSaveName.isEmpty())
            {
				FASTLOG(FLog::AutoSave, "Removing old autosave");
                QFile::remove(m_AutoSaveName);
                m_AutoSaveName.clear();

				// if we loaded from an autosave, we have a real file name now and can delete the autosave at end
                m_IsAutoSaveLoad = false;
            }

            m_fileName = fileName;
            RobloxDocManager::Instance().setDocTitle(*this,displayName(),titleTooltip(),titleIcon());
        }

        if (isAutoSave)
            m_pMainWindow->statusBar()->showMessage("Auto-Save Success",3000);
        else
            m_pMainWindow->statusBar()->showMessage("Save File Success",5000);

		if (isAutoSave && m_pRSFHandler)
		{
			QString recentSaveFile = getRecentSaveFile();
			if (m_fileName == recentSaveFile)
				m_pRSFHandler->removeFile(recentSaveFile);
		}

        return true;
	}
    else
    {
        QMessageBox::critical(
            m_pMainWindow,
			"Save File Failure",
            errorMessage);

        m_pMainWindow->statusBar()->showMessage("Save File Failure",5000);
        return false;
    }
}

void RobloxIDEDoc::activate()
{
	if (m_bActive)
		return;
	
	if (m_pQOgreWidget)
        m_pQOgreWidget->activate();

    shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();

	if (m_pDiagViewWidget)
		m_pDiagViewWidget->setDataModel(dataModel);

	if ( !m_pMainWindow->isRibbonStyle() ) // We never hide the plugins on ribbon mode
	{
        RBX::PluginManager::singleton()->disableStudioUI(dataModel.get(), false);
	}
	RBX::PluginManager::singleton()->setCurrentDataModel(dataModel.get());

    // Init Find Replace dialogs

	UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).setCurrentWidget(m_CurrentGame->m_TreeWidget);	

	if (dataModel)
	{
		RBX::Workspace* workspace = dataModel->getWorkspace();
		if (workspace)
		{
			workspace->setShowAxisWidget(UpdateUIManager::Instance().get3DAxisEnabled());
			workspace->setShow3DGrid(UpdateUIManager::Instance().get3DGridEnabled());
		}
		m_pMainWindow->insertServiceDialog().setDataModel(dataModel);
		m_pMainWindow->insertServiceAction->setEnabled(m_pMainWindow->insertServiceDialog().isAvailable());

	}

	 // load settings
    QSettings settings;

	 // load the grid mode setting
	setDraggerGridMode((RBX::DRAG::DraggerGridMode)settings.value(DraggerGridModeSetting, RBX::DRAG::ONE_STUD).toInt());
	setGridSize(settings.value(GridSizeSetting, 4).toInt());

	//update toobars
	UpdateUIManager::Instance().updateToolBars();

    updateUndoRedo();	

	m_bActive = true;
}

void RobloxIDEDoc::deActivate()
{
	if (!m_bActive)
		return;
	
#ifdef _WIN32
	if (m_CurrentGame->m_RecordToggleVerb && m_CurrentGame->m_RecordToggleVerb->isRecording())
		m_CurrentGame->m_RecordToggleVerb->stopRecording(false);
#endif
	
	if (m_pQOgreWidget)
        m_pQOgreWidget->deActivate();

	if (m_pDiagViewWidget)
		m_pDiagViewWidget->setDataModel(boost::shared_ptr<RBX::DataModel>());

	if (!m_pMainWindow->isRibbonStyle()) // We never hide the plugins on ribbon mode
	{
        RBX::PluginManager::singleton()->disableStudioUI(m_CurrentGame->m_Game->getDataModel().get(),true);
	}

	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		FindReplaceProvider::instance().getEmbeddedFindDialog()->hide();
	}
	else
	{
		FindReplaceProvider::instance().getFindDialog()->hide();
		FindReplaceProvider::instance().getReplaceDialog()->hide();
	}

	// save settings
    QSettings settings;
    settings.setValue(DraggerGridModeSetting,RBX::AdvArrowTool::advGridMode);
	settings.setValue(GridSizeSetting, (int)m_CurrentGame->m_Game->getDataModel()->getWorkspace()->gridSizeModifier);

    // reset the tooltips for undo/redo
    m_pMainWindow->undoAction->setToolTip(tr("Undo"));
    QtUtilities::setActionShortcuts(*m_pMainWindow->undoAction,QKeySequence::keyBindings(QKeySequence::Undo));

    m_pMainWindow->redoAction->setToolTip(tr("Redo"));
    QtUtilities::setActionShortcuts(*m_pMainWindow->redoAction,QKeySequence::keyBindings(QKeySequence::Redo));

	m_bActive = false;
}

void RobloxIDEDoc::setDockViewsEnabled(bool state)
{
	// update tree widget
    if (m_CurrentGame->m_TreeWidget)
		state ? m_CurrentGame->m_TreeWidget->activate() : m_CurrentGame->m_TreeWidget->deActivate();

	// update rest of the views
    shared_ptr<RBX::DataModel> dataModel = state ? m_CurrentGame->m_Game->getDataModel() : shared_ptr<RBX::DataModel>();

	UpdateUIManager& uiManager = UpdateUIManager::Instance();
    RobloxPropertyWidget& property_view = uiManager.getViewWidget<RobloxPropertyWidget>(eDW_PROPERTIES);
	property_view.setDataModel(dataModel);

    RobloxDiagnosticsView& diagnostics_view = uiManager.getViewWidget<RobloxDiagnosticsView>(eDW_DIAGNOSTICS);
	diagnostics_view.setDataModel(dataModel);

    RobloxScriptReview& script_view = uiManager.getViewWidget<RobloxScriptReview>(eDW_SCRIPT_REVIEW);
	script_view.setDataModel(dataModel);

    RobloxToolBox& toolbox_view = uiManager.getViewWidget<RobloxToolBox>(eDW_TOOLBOX);
	toolbox_view.setDataModel(dataModel);

    InsertObjectWidget& basic_view = uiManager.getViewWidget<InsertObjectWidget>(eDW_BASIC_OBJECTS);
	basic_view.setDataModel(dataModel);

	ScriptAnalysisWidget& analysis_view = uiManager.getViewWidget<ScriptAnalysisWidget>(eDW_SCRIPT_ANALYSIS);
	analysis_view.setDataModel(dataModel);

	if (!state)
	{
		RobloxFindWidget& find_view = uiManager.getViewWidget<RobloxFindWidget>(eDW_FIND);
		find_view.dataModelClosing();
	}
}

void RobloxIDEDoc::onWaypointChanged()
{
    QMetaObject::invokeMethod(this,"updateUndoRedo",Qt::QueuedConnection);
}

void RobloxIDEDoc::updateUndoRedo()
{
    RBXASSERT(m_CurrentGame->m_Game); 
    RBXASSERT(m_CurrentGame->m_Game->getDataModel());

	if (!m_CurrentGame->m_UndoVerb || !m_CurrentGame->m_RedoVerb) // Possible on closing down Play Doc
		return;

    m_pMainWindow->undoAction->setEnabled(m_CurrentGame->m_UndoVerb->isEnabled());
    m_pMainWindow->redoAction->setEnabled(m_CurrentGame->m_RedoVerb->isEnabled());
    
    QString tooltip;
    QList<QKeySequence> shortcuts;
    
    shortcuts = m_pMainWindow->undoAction->shortcuts();
    tooltip = m_CurrentGame->m_UndoVerb->getText().c_str();
    tooltip += " (";
    for (int i = 0 ; i < shortcuts.size() ; ++i)
    {
        if (i > 0)
            tooltip += ", ";
        tooltip += shortcuts[i].toString(QKeySequence::NativeText);
    }
    tooltip += ")";
    m_pMainWindow->undoAction->setToolTip(tooltip);

    shortcuts = m_pMainWindow->redoAction->shortcuts();
    tooltip = m_CurrentGame->m_RedoVerb->getText().c_str();
    tooltip += " (";
    for (int i = 0 ; i < shortcuts.size() ; ++i)
    {
        if (i > 0)
            tooltip += ", ";
        tooltip += shortcuts[i].toString(QKeySequence::NativeText);
    }
    tooltip += ")";
    m_pMainWindow->redoAction->setToolTip(tooltip);
}

void RobloxIDEDoc::mapActionIDWithVerb(GameState& gameState,const QString &actionID, const RBX::Name &verbName)
{
	gameState.m_VerbMap[actionID] = &verbName;
}

void RobloxIDEDoc::mapActionIDWithVerb(GameState& gameState,const QString &actionID, const RBX::Verb *pVerb)
{
	if (pVerb)
		mapActionIDWithVerb(gameState, actionID, pVerb->getName());
}

void RobloxIDEDoc::mapActionIDWithVerb(GameState& gameState,const QString &actionID, const char *verbName)
{
	if (verbName)
		mapActionIDWithVerb(gameState, actionID, RBX::Name::lookup(verbName));
}

bool RobloxIDEDoc::handleDrop(const QString &fileName)
{
	if (fileName.isEmpty())
		return false;

	if (fileName.endsWith(".rbxm", Qt::CaseInsensitive) || fileName.endsWith(".rbxmx", Qt::CaseInsensitive))
	{
		try 
        {
			StudioUtilities::insertModel(m_CurrentGame->m_Game->getDataModel(), fileName, false);
		}
		catch (std::exception& e)
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		}
	}

	return true;
}

bool RobloxIDEDoc::doHandleAction(const QString& actionID, bool isChecked)
{
	// TODO: Specific actions -- should this be moved to a separate function if more such actions?
	UpdateUIManager& uiManager = UpdateUIManager::Instance();

    if (uiManager.isLongProcessInProgress())
        return false;
	
    shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
	
	if (actionID == "startPlayerAction" || 
         actionID == "startServerAction" || 
		 actionID == "startServerAndPlayersAction" || 
         actionID == "playSoloAction")
	{
		applyScriptChanges();
	}
	else if (actionID == "viewDiagnosticsAction")
	{
        if (dataModel)
		{
            RobloxDiagnosticsView& diagnostics_view = uiManager.getViewWidget<RobloxDiagnosticsView>(eDW_DIAGNOSTICS);
	        diagnostics_view.setDataModel(dataModel);
        }
	}
	else if (actionID == "viewScriptPerformanceAction")
	{
        if (dataModel)
		{
		    RobloxScriptReview& script_view = uiManager.getViewWidget<RobloxScriptReview>(eDW_SCRIPT_REVIEW);
	        script_view.setDataModel(dataModel);
        }
	}
	else if (actionID == "actionUpdateStatusBar")
	{
		if (dataModel)
		{
			double time = dataModel->getGameTime();
			QString timeLabel;
			timeLabel.sprintf("t %.4g", time);

			QString fpsLabel;
            if (m_CurrentGame->m_View && m_CurrentGame->m_View->getViewBase().getFrameRateManager())
            {
                double fps = m_CurrentGame->m_View->getViewBase().getFrameRateManager()->GetFrameTimeAverage();

                if (AuthoringSettings::singleton().renderThrottlePercentage >= 100 && !m_pQOgreWidget->hasApplicationFocus())
			        fpsLabel.sprintf("0fps");
                else
                    fpsLabel.sprintf("%.3gfps",1000.0 / fps);
            }

			UpdateUIManager::Instance().setStatusLabels(timeLabel, fpsLabel);
		}
	}
	else if (actionID == "viewToolboxAction")
	{
        if (dataModel)
		{
		    RobloxToolBox& toolbox_view = uiManager.getViewWidget<RobloxToolBox>(eDW_TOOLBOX);
	        toolbox_view.setDataModel(dataModel);
        }
	}
	else if (actionID == "viewBasicObjectsAction")
	{
		if (dataModel)
		{
		    InsertObjectWidget& basic_view = uiManager.getViewWidget<InsertObjectWidget>(eDW_BASIC_OBJECTS);
	        basic_view.setDataModel(dataModel);
        }
	}
    else if (actionID == "testStatsAction")
    {
        toggleStats(StatsVerb);
        return true;
    }
    else if (actionID == "testRenderStatsAction")
    {
        toggleStats(RenderStatsVerb);
        return true;
    }
    else if (actionID == "testPhysicsStatsAction")
    {
        toggleStats(PhysicsStatsVerb);
        return true;
    }
    else if (actionID == "testNetworkStatsAction")
    {
        toggleStats(NetworkStatsVerb);
        return true;
    }
    else if (actionID == "testSummaryStatsAction")
    {
        toggleStats(SummaryStatsVerb);
        return true;
    }
    else if (actionID == "testCustomStatsAction" && FFlag::StudioCustomStatsEnabled)
    {
        toggleStats(CustomStatsVerb);
        return true;
    }
    else if (actionID == "testClearStatsAction")
    {
        setStatsEnabled(StatsVerb,false, m_pMainWindow->testStatsAction);
        setStatsEnabled(RenderStatsVerb,false, m_pMainWindow->testRenderStatsAction);
        setStatsEnabled(PhysicsStatsVerb,false, m_pMainWindow->testPhysicsStatsAction);
        setStatsEnabled(NetworkStatsVerb,false, m_pMainWindow->testNetworkStatsAction);
        setStatsEnabled(SummaryStatsVerb,false, m_pMainWindow->testSummaryStatsAction);
        setStatsEnabled(CustomStatsVerb,false, m_pMainWindow->testCustomStatsAction);
    }
	else if (FFlag::StudioSeparateActionByActivationMethod && actionID == "minimizeRibbonAction" && UpdateUIManager::Instance().getMainWindow().isRibbonStyle())
	{
		if (RobloxRibbonMainWindow* mainWindow = static_cast<RobloxRibbonMainWindow*>(&UpdateUIManager::Instance().getMainWindow()))
		{
			mainWindow->toggleRibbonMinimize();
			return true;
		}		
	}
	else if (FFlag::StudioSeparateActionByActivationMethod && actionID == "quickInsertAction")
	{
		UpdateUIManager::Instance().onQuickInsertFocus();
		return true;
	}
	else if (actionID == "materialActionAsTool")
	{
		StudioMaterialVerb::sMaterialActionActAsTool = isChecked;
		RobloxSettings().setValue("rbxMaterialActionActAsTool", isChecked);
		// reset mouse command if material tool is active
		if (!isChecked)
			resetMouseCommand("Material");
		return true;
	}
	else if (actionID == "colorFillActionAsTool")
	{
		StudioColorVerb::sColorActionActAsTool = isChecked;
		RobloxSettings().setValue("rbxColorActionActAsTool", isChecked);
		// reset mouse command if fill tool is active
		if (!isChecked)
			resetMouseCommand("Fill");
		return true;
	}
	else if (isCloudEditSession() && actionID == "downloadPlaceCopyAction")
	{
		downloadPlaceCopy();
		return true;
	}

	QString actionIDToLookFor(actionID);

	//split the QString to check if it need preProcessVerb
	QStringList tokens = actionID.split("_");
	//if it is a color change action, do the preProcessVerb to set rgb values, and change action to Fill automatically.
	if (tokens.length() > 1)
	{
		preProcessVerb(tokens);
		actionIDToLookFor = tokens[0];
	}

    if (!m_CurrentGame->m_VerbMap.contains(actionIDToLookFor))
	{
		IExternalHandler* pHandler = getExternalHandler(actionID);
		if (pHandler)
		{
			RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
			pHandler->setDataModel(dataModel);
			return pHandler->handle();
		}
		return false;
	}

	try
	{
		// DE2875: Doing a lock here leads to a hang with modal dialogs, if a lock is must then it has to be inside the verb!
		static QString noLockVerbs(
            "insertModelAction insertIntoFileAction selectionSaveToFileAction playSoloAction "
            "startServerAction startPlayerAction publishToRobloxAction publishToRobloxAsAction "
            "publishSelectionToRobloxAction saveToRobloxAction undoAction redoAction "
			"publishAsPluginAction openToolBoxWithOptions pairRbxDeviceAction pairRbxDevDeviceAction manageEmulationDeviceAction "
			"createNewLinkedSourceAction unionSelectionAction separateSelectionAction");

		static QString noLockVerbsPlaySoloOptimization("simulationRunAction simulationPlayAction "
                                                       "simulationRunSelectorSim simulationRunSelectorTest simulationRunSelectorDebug");

		if (noLockVerbs.contains(actionIDToLookFor) || (noLockVerbsPlaySoloOptimization.contains(actionIDToLookFor)) || (actionIDToLookFor == "simulationResetAction"))
		{
			RBX::Verb *pVerb = dataModel->getWorkspace()->getVerb(*m_CurrentGame->m_VerbMap[actionIDToLookFor]);
			if (pVerb && pVerb->isEnabled())
				pVerb->doIt(dataModel.get());			
		}
		else 
		{
			RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
			RBX::Verb *pVerb = dataModel->getWorkspace()->getVerb(*m_CurrentGame->m_VerbMap[actionIDToLookFor]);
			if (pVerb && pVerb->isEnabled())
				pVerb->doIt(dataModel.get());
		}
	}
	catch (std::exception& e)
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(e.what());
		msgBox.exec();
	}
	
	if (actionID == "advanceJointCreationManualAction")
	{
		RobloxSettings settings;
		settings.setValue(RBX_MANUAL_JOINT_CREATION_KEY, RBX::AdvArrowTool::advManualJointMode);
	}
	
	return true;
}

void RobloxIDEDoc::preProcessVerb(const QStringList &tokens)
{
	//if it is a color changes, get the last three tokens, which will be the rgb value respectily. check the name of the color actions for detail.
	if (tokens[0] == "actionFillColor")
	{	
		QByteArray ba = tokens[tokens.length()-1].toAscii();
		const char *c_str = ba.data();

		RBX::FillTool::color.set(RBX::BrickColor::parse(c_str));
	}
	//if it is a material change, the second token is the name of the material. check the material action for detail.
	else if (tokens[0] == "actionMaterial")
	{
		RBX::MaterialTool::material = RBX::MaterialVerb::parseMaterial(tokens[1].toStdString());
	}
}

bool RobloxIDEDoc::handlePluginAction(void *pNotifier, void *pAction) 
{
	if (!pAction || !pNotifier)
		return false;
	static_cast<RBX::IHostNotifier*>(pNotifier)->buttonClick(m_CurrentGame->m_Game->getDataModel().get(), pAction);
	return true;
}

RBX::Reflection::Variant RobloxIDEDoc::evaluateCommandBarItem(const char* itemToEvaluate, shared_ptr<RBX::LuaSourceContainer> script)
{
    shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
	RBX::DataModel::LegacyLock lock(dataModel,RBX::DataModelJob::Write);

    boost::shared_ptr<RBX::ScriptContext> pScriptContext = RBX::shared_from(dataModel->create<RBX::ScriptContext>());
    if(pScriptContext)
    {
        return pScriptContext->evaluateStudioCommandItem(itemToEvaluate, script);
    }
    return RBX::Reflection::Variant();
}

void RobloxIDEDoc::handleScriptCommand(const QString &execCommand)
{
    if (execCommand.isEmpty())
        return;

    shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
	RBX::DataModel::LegacyLock lock(dataModel,RBX::DataModelJob::Write);

	try
	{
		std::string command = execCommand.toStdString();

		bool isJoinScript = false;

		if (RBX::ContentProvider::isHttpUrl(command))
		{ 
			if (StudioUtilities::containsJoinScript(execCommand))
				isJoinScript = true;

			std::auto_ptr<std::istream> stream = 
                RBX::ServiceProvider::create<RBX::ContentProvider>(dataModel.get())->getContent(RBX::ContentId(command));
            command = std::string(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
		}

		if (isJoinScript)
		{
			// enable FRM in play mode
			CRenderSettingsItem::singleton().setEnableFRM(true);

			// Reset synchronized flags, they should be set by the server
			FLog::ResetSynchronizedVariablesState();

			int firstNewLineIndex = command.find("\r\n");
			if (command[firstNewLineIndex+2] == '{')
			{
				m_CurrentGame->m_Game->configurePlayer(RBX::Security::CmdLine_, command.substr(firstNewLineIndex+2));
				return;
			}
		}

		boost::shared_ptr<RBX::ScriptContext> pScriptContext = RBX::shared_from(dataModel->create<RBX::ScriptContext>());
		if (pScriptContext)
			pScriptContext->executeInNewThread(RBX::Security::CmdLine_, RBX::ProtectedString::fromTrustedSource(command.c_str()), command.c_str());
	}
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
	}
}

bool RobloxIDEDoc::actionState(const QString &actionID, bool &enableState, bool &checkedState)
{		
	if (isCloudEditSession())
	{
		// disable save and publish related actions
		static QString cloudEditDeactiveActions(
			"publishToRobloxAsAction fileSaveAction fileSaveAsAction publishToRobloxAction ");

		if (cloudEditDeactiveActions.contains(actionID))
		{
			checkedState = false;
			enableState = false;
			return true;
		}

		static QString cloudEditActiveActions(
			"downloadPlaceCopyAction ");

		if (cloudEditActiveActions.contains(actionID))
		{
			checkedState = false;
			enableState = true;
			return true;
		}
	}

	static QString defaultActiveActions(
		"fileSaveAction fileSaveAsAction fileCloseAction "
		"executeScriptAction publishToRobloxAsAction testClearStatsAction ");

	if (defaultActiveActions.contains(actionID))
	{
		checkedState = false;
		enableState = true;
		return true;
	}

	if (m_pMainWindow->isRibbonStyle())
	{
        static QString defaultActiveRibbonActions(
            "setVehiclesAction setNPCsAction setBuildingsAction setGearAction "
            "setWeaponsAction setMyModelsAction setSearchAction "
            "simulationScriptsEnabledAction simulationPlayerEnabledAction simulationPhysicsEnabledAction "
            "actionParticleEffects actionSelectMenu "			
            "PartLargeAction SphereLargeAction WedgeLargeAction CylinderLargeAction startLocationAction lockMenu jointCreationMode "
            "ms_Plastic ms_Wood ms_Slate ms_Concrete ms_CorrodedMetal ms_DiamondPlate ms_Foil ms_Grass ms_Ice ms_Brick ms_Sand "
            "ms_Fabric ms_Granite ms_Marble ms_Pebble ms_SmoothPlastic ms_Neon ms_WoodPlanks ms_Cobblestone ms_Metal emulateDeviceAction "
            "placePartAction playGameAction ScriptLargeAction LocalScriptLargeAction ModuleScriptLargeAction gridGroup __qtn_Action_Option ");
        if ( defaultActiveRibbonActions.contains(actionID))
        {
            QAction* action = UpdateUIManager::Instance().getAction(actionID);
            if (action)
            {
                checkedState = action->isChecked();
                enableState = true;
            }
            return true;
        }

		static QString clientAndServerActions("playersMode startServerCB startServerAndPlayersAction cleanupServersAndPlayersAction pairRbxDevDeviceAction");
		if (clientAndServerActions.contains(actionID))
		{
			if (!m_CurrentGame || !m_CurrentGame->m_Game || !m_CurrentGame->m_Game->getDataModel() || (
				!RobloxIDEDoc::isEditMode(m_CurrentGame->m_Game->getDataModel().get()) && !isCloudEditSession()))
			{
				enableState = false;
				return true;
			}
			else if (actionID == "pairRbxDevDeviceAction")
			{
				enableState = !RobloxApplicationManager::instance().hasChildProcesses();
				return true;
			}
			else if (actionID == "cleanupServersAndPlayersAction")
			{
				enableState = RobloxApplicationManager::instance().hasChildProcesses();
				return true;
			}
			else if (actionID == "playersMode")
			{
				enableState = true;
				return true;
			}
		}

		static QString defaultRibbonDeactiveAction("startServerAction addWatchAction ");
		if (defaultRibbonDeactiveAction.contains(actionID))
		{
			checkedState = false;
			enableState = false;
			return true;
		}

		if (actionID == "actionSurface")
		{
			static QStringList surfaceActionsList = (QStringList() << "glueSurfaceAction" << "smoothSurfaceAction" << "studsAction" 
				                                                   << "inletAction" << "universalsAction" << "weldSurfaceAction");
			for (int ii = 0; ii < surfaceActionsList.size(); ++ii)
			{
				actionState(surfaceActionsList[ii], enableState, checkedState);
				if (checkedState)
					return true;
			}

			enableState = true;
			return true;
		}

		if (actionID == "materialActionAsTool")
		{
			checkedState = StudioMaterialVerb::sMaterialActionActAsTool;
			enableState = true;
			return true;
		}

		if (actionID == "colorFillActionAsTool")
		{
			checkedState = StudioColorVerb::sColorActionActAsTool;
			enableState = true;
			return true;
		}

		if (FFlag::LuaDebugger)
		{
			if (actionID == "toggleAllBreakpointsStateAction")
			{
				enableState = DebuggerViewsManager::Instance().getViewWidget<BreakpointsWidget>(DebuggerViewsManager::eDV_BREAKPOINTS).numBreakpoints() > 0;
				return true;
			}
		}

		// get status from plugin host (this is required only for ribbon as we do not hide plugin page)
		if (m_pMainWindow->pluginHost().actionState(actionID, enableState, checkedState))
			return true;
	}

    // TODO - fix this (see below)
    static QString disabledActions =
        "commentSelectionAction uncommentSelectionAction toggleCommentAction "
        "expandAllFoldsAction collapseAllFoldsAction replaceAction resetScriptZoomAction "
        "reloadScriptAction goToLineAction downloadPlaceCopyAction ";
    if (disabledActions.contains(actionID))
	{
		enableState = false;
		return true;
	}

	if (UpdateUIManager::Instance().getDockActionNames().contains(actionID))
	{
		enableState = true;
		checkedState = UpdateUIManager::Instance().getAction(actionID)->isChecked();
		return true;
	}

    if (m_CurrentGame->m_VerbMap.contains(actionID))
	{	
		RBX::Verb* pVerb = m_CurrentGame->m_Game->getDataModel()->getWorkspace()->getVerb(*m_CurrentGame->m_VerbMap[actionID]);
		if (pVerb)
		{
			checkedState = pVerb->isChecked();
			enableState = pVerb->isEnabled();
			return true;
		}
	}	

    // TODO - all the enabled/active states should be enabled otherwise default disabled
    // enabledState = false;
	
	return false;	
}

void RobloxIDEDoc::createActionIDVerbMap(GameState& gameState)
{
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	//define mapping - action name with the verb name!
	mapFilesMenu(gameState);
	mapEditMenu(gameState);
	mapViewMenu(gameState);
	mapFormatMenu(gameState);
	mapToolMenu(gameState);
	mapInsertMenu(gameState);
	mapAdvToolsToolbar(gameState);
	mapRunToolbar(gameState);
	mapCameraToolbar(gameState);
	mapStandardToolbar(gameState);
	mapAdvanceBuildToolbar(gameState);

	//cut, copy and paste
	verbs.append(new CutVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"cutAction",verbs.back());

	verbs.append(new CopyVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"copyAction",verbs.back());

	verbs.append(new PasteVerb(dataModel.get(), false));
	mapActionIDWithVerb(gameState,"pasteAction",verbs.back());

	verbs.append(new PasteVerb(dataModel.get(), true));
	mapActionIDWithVerb(gameState,"pasteIntoAction",verbs.back());

	verbs.append(new DuplicateSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"duplicateSelectionAction",verbs.back());

	verbs.append(new RBX::BoundVerb(dataModel.get(), "renameObjectVerb",
		boost::bind(&RobloxIDEDoc::onObjectRename, this), boost::bind(&RobloxIDEDoc::isObjectRenameEnabled, this)));
	mapActionIDWithVerb(gameState,"renameObjectAction",verbs.back());

	verbs.append(new InsertModelVerb(dataModel.get(), true));
	mapActionIDWithVerb(gameState,"insertIntoFileAction",verbs.back());

	verbs.push_back(new SelectionSaveToFileVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "selectionSaveToFileAction", verbs.back());

	verbs.append(new ExportSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "exportSelectionAction", verbs.back());

	if (m_pMainWindow->isRibbonStyle())
	{
		verbs.push_back(new OpenToolBoxWithOptionsVerb(dataModel.get()));
		mapActionIDWithVerb(gameState, "openToolBoxWithOptions", verbs.back());	

		verbs.push_back(new InsertBasicObjectVerb(dataModel.get()));
		mapActionIDWithVerb(gameState, "insertBasicObject", verbs.back());			
	}
	
	// no mapping required
	verbs.push_back(new ShutdownClientVerb(dataModel.get(), this));
    verbs.push_back(new ShutdownClientAndSaveVerb(dataModel.get(), this));
	verbs.push_back(new LeaveGameVerb(dataModel.get(), this));	
	verbs.push_back(new ToggleFullscreenVerb(dataModel.get()));
    
    verbs.push_back(new ToggleLocalSpaceVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "toggleLocalSpaceAction", verbs.back());

	verbs.push_back(new LaunchHelpForSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "launchHelpForSelectionAction", verbs.back());

    verbs.push_back(new AnalyzePhysicsToggleVerb(dataModel.get()));
    mapActionIDWithVerb(gameState, "analyzePhysicsToggleAction", verbs.back());
}

void RobloxIDEDoc::mapFilesMenu(GameState& gameState)
{
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	verbs.append(new PublishToRobloxVerb(dataModel.get(), m_pMainWindow));
	mapActionIDWithVerb(gameState,"publishToRobloxAction",verbs.back());

	verbs.append(new PublishToRobloxAsVerb(dataModel.get(), m_pMainWindow));
	mapActionIDWithVerb(gameState,"publishToRobloxAsAction",verbs.back());

	verbs.append(new PublishSelectionToRobloxVerb(dataModel.get(), m_pMainWindow));
	mapActionIDWithVerb(gameState,"publishSelectionToRobloxAction",verbs.back());
	
	/// This goes into the Tree View
	mapActionIDWithVerb(gameState,"saveToRobloxAction",verbs.back());

	verbs.push_back(new CreateNewLinkedSourceVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "createNewLinkedSourceAction", verbs.back());

	verbs.push_back(new PublishAsPluginVerb(dataModel.get(), m_pMainWindow));
	mapActionIDWithVerb(gameState, "publishAsPluginAction", verbs.back());

	verbs.append(new ExportPlaceVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "exportPlaceAction", verbs.back());
}

void RobloxIDEDoc::mapEditMenu(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	mapActionIDWithVerb(gameState,"selectAllAction", &commonVerbs->selectAllCommand);
	mapActionIDWithVerb(gameState,"deleteSelectedAction", &commonVerbs->deleteSelectionVerb);
	mapActionIDWithVerb(gameState,"lockAction", &commonVerbs->lockToolVerb);
	mapActionIDWithVerb(gameState,"unlockAllAction", &commonVerbs->unlockAllVerb);

	gameState.m_UndoVerb = new UndoVerb(dataModel.get());
	verbs.append(gameState.m_UndoVerb);
	mapActionIDWithVerb(gameState,"undoAction",gameState.m_UndoVerb);

	gameState.m_RedoVerb = new RedoVerb(dataModel.get());
	verbs.append(gameState.m_RedoVerb);
	mapActionIDWithVerb(gameState,"redoAction",gameState.m_RedoVerb);

    mapActionIDWithVerb(gameState,"rotateSelectionAction",&commonVerbs->rotateSelectionVerb);
	mapActionIDWithVerb(gameState,"tiltSelectionAction",&commonVerbs->tiltSelectionVerb);
    mapActionIDWithVerb(gameState,"moveUpBrickAction",&commonVerbs->moveUpBrickVerb);
	mapActionIDWithVerb(gameState,"moveDownBrickAction",&commonVerbs->moveDownSelectionVerb);
}

void RobloxIDEDoc::mapViewMenu(GameState& gameState)
{
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	verbs.append(new ToggleAxisWidgetVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"toggleAxisWidgetAction",verbs.back());

	verbs.append(new Toggle3DGridVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"toggle3DGridAction",verbs.back());
}

void RobloxIDEDoc::mapFormatMenu(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	mapActionIDWithVerb(gameState,"rotateSelectionAction", &commonVerbs->rotateSelectionVerb);
	mapActionIDWithVerb(gameState,"tiltSelectionAction", &commonVerbs->tiltSelectionVerb);

	verbs.append(new GroupSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"groupSelectionAction", verbs.back());

    verbs.append(new UngroupSelectionVerb(dataModel.get()));
    mapActionIDWithVerb(gameState,"ungroupSelectionAction", verbs.back());

    verbs.append(new UnionSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "unionSelectionAction", verbs.back());

    verbs.append(new NegateSelectionVerb(dataModel.get()));
    mapActionIDWithVerb(gameState, "negateSelectionAction", verbs.back());

	verbs.append(new SeparateSelectionVerb(dataModel.get()));
	mapActionIDWithVerb(gameState, "separateSelectionAction", verbs.back());

	mapActionIDWithVerb(gameState,"selectChildrenAction", &commonVerbs->selectChildrenVerb);
}

void RobloxIDEDoc::mapToolMenu(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;
	
	verbs.append(new PlaySoloVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"playSoloAction",verbs.back());
	
	verbs.append(new StartServerVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"startServerAction",verbs.back());

	verbs.append(new StartPlayerVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"startPlayerAction",verbs.back());

	mapActionIDWithVerb(gameState,"testStatsAction", &commonVerbs->statsCommand);
	mapActionIDWithVerb(gameState,"testRenderStatsAction", &commonVerbs->renderStatsCommand);
	mapActionIDWithVerb(gameState,"testNetworkStatsAction", &commonVerbs->networkStatsCommand);
	mapActionIDWithVerb(gameState,"testPhysicsStatsAction", &commonVerbs->physicsStatsCommand);
	mapActionIDWithVerb(gameState,"testSummaryStatsAction", &commonVerbs->summaryStatsCommand);
	if (FFlag::StudioCustomStatsEnabled)
	{
		mapActionIDWithVerb(gameState,"testCustomStatsAction", &commonVerbs->customStatsCommand);
	}

	if (m_pQOgreWidget)
	{
		verbs.append(new ScreenshotVerb(dataModel.get()));
		mapActionIDWithVerb(gameState, "screenShotAction", verbs.back());
	}

	if (m_pMainWindow->isRibbonStyle())
	{
		verbs.push_back(new InsertAdvancedObjectViewVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"actionInsertAdvancedObject",verbs.back());

		verbs.push_back(new CreatePluginVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"createPluginAction",verbs.back());

		verbs.push_back(new StartServerAndPlayerVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"startServerAndPlayersAction",verbs.back());

		verbs.push_back(new ServerPlayersStateInitVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"startServerCB",verbs.back());

		verbs.push_back(new PairRbxDevVerb(dataModel.get(), NULL));
		mapActionIDWithVerb(gameState,"pairRbxDevDeviceAction",verbs.back());

		verbs.push_back(new ManageEmulationDevVerb(dataModel.get(), NULL));
		mapActionIDWithVerb(gameState, "manageEmulationDeviceAction", verbs.back());

		verbs.push_back(new AudioToggleVerb(dataModel.get()));
		mapActionIDWithVerb(gameState, "audioToggleAction", verbs.back());

        //verbs.push_back(new LaunchInstancesVerb(dataModel.get()));
		//mapActionIDWithVerb(gameState,"launchInstancesAction",verbs.back());
	}
}

void RobloxIDEDoc::mapInsertMenu(GameState& gameState)
{
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	//insert
	verbs.append(new InsertModelVerb(dataModel.get(), false));
	mapActionIDWithVerb(gameState,"insertModelAction",verbs.back());
}

void RobloxIDEDoc::mapAdvToolsToolbar(GameState& gameState)
{
	boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
	shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
	QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	mapActionIDWithVerb(gameState,"glueSurfaceAction", &commonVerbs->glueToolVerb);
	mapActionIDWithVerb(gameState,"smoothSurfaceAction", &commonVerbs->flatToolVerb);
	mapActionIDWithVerb(gameState,"weldSurfaceAction", &commonVerbs->weldToolVerb);
	mapActionIDWithVerb(gameState,"studsAction", &commonVerbs->studsToolVerb);
	mapActionIDWithVerb(gameState,"inletAction", &commonVerbs->inletToolVerb);
	mapActionIDWithVerb(gameState,"universalsAction", &commonVerbs->universalToolVerb);
	mapActionIDWithVerb(gameState,"hingeAction", &commonVerbs->hingeToolVerb);
	mapActionIDWithVerb(gameState,"smoothNoOutlinesAction", &commonVerbs->smoothNoOutlinesToolVerb);
	mapActionIDWithVerb(gameState,"motorRightAction", &commonVerbs->rightMotorToolVerb);

	if (m_pMainWindow->isRibbonStyle())
	{
		// if we need to have sticky command then just remove these assignments
		RBX::RightMotorTool::isStickyVerb = false;
		RBX::HingeTool::isStickyVerb = false;
		
		mapActionIDWithVerb(gameState,"anchorAction", &commonVerbs->anchorVerb);		
		verbs.push_back(new StudioColorVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"actionColorSelector", verbs.back());
		verbs.push_back(new StudioMaterialVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"actionMaterialSelector", verbs.back());
	}
	else
	{
		mapActionIDWithVerb(gameState,"anchorAction", &commonVerbs->anchorToolVerb);
		mapActionIDWithVerb(gameState,"actionFillColor", &commonVerbs->fillToolVerb);
		mapActionIDWithVerb(gameState,"actionMaterial", &commonVerbs->materialToolVerb);
	}

	mapActionIDWithVerb(gameState,"dropperAction", &commonVerbs->dropperToolVerb);
	mapActionIDWithVerb(gameState,"advArrowToolAction", &commonVerbs->advArrowToolVerb);
}

void RobloxIDEDoc::mapRunToolbar(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	//Run
	verbs.append(new RBX::BoundVerb(
        dataModel.get(), 
        "IDE Run",
		boost::bind(&RobloxIDEDoc::onIdeRun, this, false), 
        boost::bind(&RobloxIDEDoc::isIdeRunEnabled, this, false)));
	mapActionIDWithVerb(gameState,"simulationRunAction", verbs.back());

	verbs.append(new RBX::BoundVerb(
		dataModel.get(), 
		"IDE Play",
		boost::bind(&RobloxIDEDoc::onIdeRun, this, true), 
		boost::bind(&RobloxIDEDoc::isIdeRunEnabled, this, true)));
	mapActionIDWithVerb(gameState,"simulationPlayAction", verbs.back());

	verbs.append(new RBX::BoundVerb(
		dataModel.get(), 
		"IDE Pause",
		boost::bind(&RobloxIDEDoc::onIdePause, this), 
		boost::bind(&RobloxIDEDoc::isIdePauseEnabled, this)));  
	mapActionIDWithVerb(gameState,"simulationStopAction", verbs.back());
	
	verbs.append(new RBX::BoundVerb(
        dataModel.get(), 
        "IDE Reset",
		boost::bind(&RobloxIDEDoc::onIdeReset, this),
        boost::bind(&RobloxIDEDoc::isIdeResetEnabled, this)));
	mapActionIDWithVerb(gameState,"simulationResetAction", verbs.back());
}

void RobloxIDEDoc::mapCameraToolbar(GameState& gameState)
{
	mapActionIDWithVerb(gameState,"zoomInAction", "CameraZoomIn");
	mapActionIDWithVerb(gameState,"zoomOutAction", "CameraZoomOut");

    // Camera Tilt Commands apparently use an inverted pitch.  We don't want 
    //  to change anything that relies on that behavior so in Studio we're 
    //  going to switch them.  Inverted pitch might make sense playing a 
    //  game but not when editing.
	mapActionIDWithVerb(gameState,"tiltUpAction", "CameraTiltDown");
	mapActionIDWithVerb(gameState,"tiltDownAction", "CameraTiltUp");

	mapActionIDWithVerb(gameState,"panRightAction", "CameraPanRight");
	mapActionIDWithVerb(gameState,"panLeftAction", "CameraPanLeft");
	mapActionIDWithVerb(gameState,"lookAtObjectAction", "CameraCenter");
	mapActionIDWithVerb(gameState,"zoomExtentsAction", "CameraZoomExtents"); // Only used in context menu on part
}

void RobloxIDEDoc::mapStandardToolbar(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;

	mapActionIDWithVerb(gameState,"moveUpBrickAction", &commonVerbs->moveUpBrickVerb);
	mapActionIDWithVerb(gameState,"moveDownBrickAction", &commonVerbs->moveDownSelectionVerb);
}

void RobloxIDEDoc::mapAdvanceBuildToolbar(GameState& gameState)
{
    boost::shared_ptr<RBX::CommonVerbs> commonVerbs = gameState.m_Game->commonVerbs;
    shared_ptr<RBX::DataModel> dataModel = gameState.m_Game->getDataModel();
    QVector<RBX::Verb*>& verbs = gameState.m_Verbs;

	//Advance build relate
	if (!m_pMainWindow->isRibbonStyle())
	{
		RobloxSettings settings;
		RBX::AdvArrowTool::advManualJointMode = settings.value(RBX_MANUAL_JOINT_CREATION_KEY, false).toBool();

		mapActionIDWithVerb(gameState,"advanceJointCreationManualAction", &commonVerbs->turnOnManualJointCreationVerb);
	}
	else
	{
		verbs.push_back(new JointCreationModeVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"jointCreationMode", verbs.back());

		mapActionIDWithVerb(gameState,"gridSizeToTwoAction", &commonVerbs->setGridSizeToTwoVerb);
		mapActionIDWithVerb(gameState,"gridSizeToFourAction", &commonVerbs->setGridSizeToFourVerb);
		mapActionIDWithVerb(gameState,"gridSizeToSixteenAction", &commonVerbs->setGridSizeToSixteenVerb);

		verbs.push_back(new JointToolHelpDialogVerb(dataModel.get()));
		mapActionIDWithVerb(gameState,"advancedJointCreationHelpAction",verbs.back());
	}
	mapActionIDWithVerb(gameState, "gridToOneAction", &commonVerbs->setDragGridToOneVerb);
	mapActionIDWithVerb(gameState, "gridToOneFifthAction", &commonVerbs->setDragGridToOneFifthVerb);
	mapActionIDWithVerb(gameState, "gridToOffAction", &commonVerbs->setDragGridToOffVerb);

	mapActionIDWithVerb(gameState,"advTranslateAction", &commonVerbs->advMoveToolVerb);
	mapActionIDWithVerb(gameState,"advRotateAction", &commonVerbs->advRotateToolVerb);
	mapActionIDWithVerb(gameState,"resizeAction", &commonVerbs->resizeToolVerb);

	verbs.push_back(new ToggleCollisionCheckVerb(dataModel.get()));
	mapActionIDWithVerb(gameState,"toggleCollisionCheckAction", verbs.back());	
}

void RobloxIDEDoc::onIdePause()
{
	if (m_PlayGame.m_Game)
	{
		shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel();
		RBX::DataModel::LegacyLock lock(playDataModel,RBX::DataModelJob::Write);
		RBX::RunService* pRS = playDataModel->create<RBX::RunService>();
		pRS->pause();
	}
	else
	{
		shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();
		RBX::DataModel::LegacyLock lock(dataModel,RBX::DataModelJob::Write);
		RBX::RunService* pRS = dataModel->create<RBX::RunService>();
		pRS->pause();
	}
}

bool RobloxIDEDoc::isIdePauseEnabled()
{
	if (m_PlayGame.m_Game)
	{
		shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel();
		RBX::RunService* pRS = playDataModel->create<RBX::RunService>();
		RBX::RunState runState = pRS->getRunState();
		return runState == RBX::RS_RUNNING;	
	}

	shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();
	RBX::RunService* pRS = dataModel->create<RBX::RunService>();
	RBX::RunState runState = pRS->getRunState();
	return (runState == RBX::RS_RUNNING);	

}

void RobloxIDEDoc::teleportToURL(QString url, bool play)
{
    if (StudioUtilities::isTestMode())
    {
        QtUtilities::RBXMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setText("Teleports not yet supported in Test Server or Create Player.");
        msgBox.exec();
        return;
    }
    
    closePlayDataModel();

    // Check to see if the url fetched returns anything at all.
    {
        shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
		std::string command = url.toStdString();
        std::auto_ptr<std::istream> stream = RBX::ServiceProvider::create<RBX::ContentProvider>(dataModel.get())->getContent(RBX::ContentId(command));
        std::string scriptStr(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
        if (scriptStr.empty())
        {
            QtUtilities::RBXMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setText("Could not teleport to place.");
            msgBox.exec();
            return;
        }
    }

    loadPlayDataModel(url, play, false, true);
}




void RobloxIDEDoc::loadPlayDataModel(QString url, bool play, bool cloneDataModel, bool isTeleport)
{
    shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();

    if (!dataModel)
        return;

	{
		// this will be called in case of old play solo
		RBX::RunService* pRS = dataModel->create<RBX::RunService>();
		RBX::DataModel::LegacyLock editLock(dataModel,RBX::DataModelJob::Write);
		if (!pRS->isEditState())
		{
			applyScriptChanges();
			pRS->run();
			return; 
		}
	}
        
    // check if we're already running
    if (m_PlayGame.m_Game &&
        m_PlayGame.m_Game->getDataModel() &&
        m_PlayGame.m_Game->getDataModel()->create<RBX::RunService>())
    {
		{	
			RBX::DataModel::LegacyLock lock(m_PlayGame.m_Game->getDataModel(),RBX::DataModelJob::Write);
			RBX::RunState runState = m_PlayGame.m_Game->getDataModel()->create<RBX::RunService>()->getRunState();
			if (runState == RBX::RS_PAUSED)
			{
				RBX::ServiceProvider::create<RBX::RunService>(m_PlayGame.m_Game->getDataModel().get())->run();
				return;
			}
		}
    }
        
    // If the Edit game has been run and is currently paused, then resume it.
    if (m_EditGame.m_Game &&
        m_EditGame.m_Game->getDataModel())
    {
		{
			RBX::RunService* pRunState = m_EditGame.m_Game->getDataModel()->find<RBX::RunService>();
			RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(),RBX::DataModelJob::Write);
			if(pRunState && pRunState->getRunState() == RBX::RS_PAUSED)
			{				
				pRunState->run();
				return;
			}
		}
    }

	if (!isTeleport)
	{
		clearOutputWidget();
		updateOnSimulationStateChange(true);
	}

    autoSave(true);
    
    UpdateUIManager::Instance().setBusy(true);
    static const int ProgressStages = 6;
    RBX::BaldPtr<QProgressBar> progressBar = UpdateUIManager::Instance().showProgress("Play Game",0,ProgressStages);
    
    // Stage 0 - initialize game
    UpdateUIManager::Instance().updateProgress(progressBar,0);
    
    deActivate();
    
    // We never hide plugins in ribbon mode but it is requierd for NPS
    if (m_pMainWindow->isRibbonStyle())
    {
        RBX::PluginManager::singleton()->hideStudioUI(m_EditGame.m_Game->getDataModel().get(),true);
    }

    if (m_EditGame.m_View)
    {
		// remove video recording verb since it's associated with view
		cleanupVideoRecording(m_EditGame);

        delete m_EditGame.m_View;
        m_EditGame.m_View = NULL;
    }
    
    QString debugDataFile = getDebugInfoFile(false, PlaySoloDebugFileExt);
    m_CurrentGame = &m_PlayGame;
    
    if (play)
    {
        StudioUtilities::setAvatarMode(true);
    }
    
    if (!m_PlayGame.m_Game)
    {
        if (FFlag::LuaDebugger)
		{
			// save edit model debugger data so it can be merged with play model changes later
			saveEditModelDebuggerData();
		}
        
        QString baseUrl = RobloxSettings::getBaseURL() + "/";
        m_PlayGame.m_Game.reset(new RBX::UnsecuredStudioGame(NULL,baseUrl.toStdString().c_str()));
        
        shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel();
        RBX::DataModel::LegacyLock lock(m_PlayGame.m_Game->getDataModel(), RBX::DataModelJob::Write);

		createOptionalDMServices();
        
        playDataModel->setName(displayName().toStdString());
        
        m_PlayGame.m_View = new RobloxView(m_pQOgreWidget,m_PlayGame.m_Game, m_GameView, /* vr = */ true);
        
        m_PlayGame.m_View->buildGui(StudioUtilities::isAvatarMode());
        
        createActionIDVerbMap(m_PlayGame);
		// initialize video recording
		initializeVideoRecording(m_PlayGame);
        
        m_PlayGame.m_TreeWidget = new RobloxTreeWidget(playDataModel);

        UpdateUIManager::Instance().getViewWidget<RobloxExplorerWidget>(eDW_OBJECT_EXPLORER).setCurrentWidget(m_PlayGame.m_TreeWidget);

        // restore in-game stats display
        QSettings settings;
        setStatsEnabled(StatsVerb,settings.value(StatsVerb,false).toBool(), m_pMainWindow->testStatsAction);
        setStatsEnabled(RenderStatsVerb,settings.value(RenderStatsVerb,false).toBool(), m_pMainWindow->testRenderStatsAction);
        setStatsEnabled(PhysicsStatsVerb,settings.value(PhysicsStatsVerb,false).toBool(), m_pMainWindow->testPhysicsStatsAction);
        setStatsEnabled(NetworkStatsVerb,settings.value(NetworkStatsVerb,false).toBool(), m_pMainWindow->testNetworkStatsAction);
        setStatsEnabled(SummaryStatsVerb,settings.value(SummaryStatsVerb,false).toBool(), m_pMainWindow->testSummaryStatsAction);
		if (FFlag::StudioCustomStatsEnabled)
		{
			setStatsEnabled(CustomStatsVerb,settings.value(CustomStatsVerb,false).toBool(), m_pMainWindow->testCustomStatsAction);
		}
        
        RBX::UserInputService* userInputService = playDataModel->find<RBX::UserInputService>();
        userInputService->setMouseEnabled(true);
        userInputService->setKeyboardEnabled(true);
    }
    
    m_pQOgreWidget->setRobloxView(m_PlayGame.m_View);
    setDockViewsEnabled(true);
    activate();
    
    // Stage 1 - save edit world
    UpdateUIManager::Instance().updateProgress(progressBar, 1);
    
    std::stringstream ss;
	int placeId, universeId;
    if (cloneDataModel)
    {
        RBX::DataModel::LegacyLock editLock(dataModel,RBX::DataModelJob::Write);
        applyScriptChanges();
        RBX::SerializerBinary::serialize(ss, dataModel.get());
		placeId = dataModel->getPlaceID();
		universeId = dataModel->getUniverseId();
    }
    
    // Stage 2 - load play world
    UpdateUIManager::Instance().updateProgress(progressBar, 2);

    shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel();
    {
        RBX::DataModel::LegacyLock playLock(playDataModel,RBX::DataModelJob::Write);
        RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
        
        bool oldStudioMode = RBX::GameBasicSettings::singleton().inStudioMode();

        RBX::GameBasicSettings::singleton().setStudioMode(false);

        if (cloneDataModel)
        {
            RBX::SerializerBinary::deserialize(ss, playDataModel.get());       
            playDataModel->processAfterLoad();
        }

		// only need to load linked scripts here when we are cloning (not teleporting)
		if (cloneDataModel)
		{
			playDataModel->setPlaceID(placeId, false /*not a roblox place*/);
			playDataModel->setUniverseId(universeId);
			LuaSourceContainer::blockingLoadLinkedScripts(
				playDataModel->create<ContentProvider>(), playDataModel.get());

		}

		tDebuggersMap playModelDebuggers;
		for (tDebuggersMap::const_iterator iter = m_EditDebuggersMap.begin(); iter != m_EditDebuggersMap.end(); ++iter)
		{
			std::vector<int> indexHierarchy;
			std::string serviceName = getScriptIndexHierarchy(iter->first, indexHierarchy);
			shared_ptr<RBX::Instance> instance = getScriptByIndexHierarchy(playDataModel, indexHierarchy, serviceName);

			if (instance)
				playModelDebuggers[instance] = iter->second;
		}

		initializeDebuggerData("", playDataModel, playModelDebuggers);

		RobloxPluginHost::loadPlugins(playDataModel.get());

    	RBX::GameBasicSettings::singleton().setStudioMode(oldStudioMode);

	}

	if (DFFlag::UseR15Character && playDataModel->getUniverseDataRequested())
	{
		playDataModel->universeDataLoaded.get_future().wait();
	}

	// make sure we patch scripts before calling RunService 
	// otherwise it will create a 'Player1' instance inside Workspace, leading to a mismatch in index hierarchy
    patchPlayModelScripts();

    // Stage 3 - set up local player
    UpdateUIManager::Instance().updateProgress(progressBar, 3);


    if (play)
    {
        RBXASSERT(m_CurrentGame == &m_PlayGame);
        
        if (!url.isEmpty())
        {
            handleScriptCommand(url);
        }
    }
    else
    {
        RBX::DataModel::LegacyLock playLock(playDataModel,RBX::DataModelJob::Write);
        RBX::ServiceProvider::create<RBX::RunService>(playDataModel.get())->run();
    }

    UpdateUIManager::Instance().hideProgress(progressBar);
    UpdateUIManager::Instance().setBusy(false);

    m_pQOgreWidget->setVisible(true);
	RobloxDocManager::Instance().setCurrentDoc(this);

    UpdateUIManager::Instance().updateToolBars();
}

void RobloxIDEDoc::closePlayDataModel()
{
    if (!m_PlayGame.m_Game.get())
        return;

    QString debugDataFile = getDebugInfoFile(false, PlaySoloDebugFileExt);

    applyScriptChanges();
    if (saveScriptsChangedWhileTesting() && FFlag::LuaDebugger)
        updateEditModelDebuggerData();
    restoreEditModelScripts();

	cleanupVideoRecording(m_PlayGame);
    deActivate();
    m_pQOgreWidget->setRobloxView(m_EditGame.m_View);
    
    if (FFlag::LuaDebugger)
		DebuggerClientManager::Instance().onRunTransition(RBX::RunTransition(RBX::RS_STOPPED, RBX::RS_STOPPED));
    
    cleanupGameState(m_PlayGame);
    
    {
        RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);
        
        RBXASSERT(!m_EditGame.m_View);
        m_EditGame.m_View = new RobloxView(m_pQOgreWidget,m_EditGame.m_Game, m_GameView);

		//since we are creating a new view we will be required to reinitialize video recording
		initializeVideoRecording(m_EditGame);
    }
    
    m_CurrentGame = &m_EditGame;
    
    setDockViewsEnabled(true);
    activate();
    
    // We never hide plugins in ribbon mode but it is requierd for NPS
    if (m_pMainWindow->isRibbonStyle())
    {
        RBX::PluginManager::singleton()->hideStudioUI(m_CurrentGame->m_Game->getDataModel().get(), false);
    }
    
    m_CurrentGame->m_View->requestUpdateView();
	if (isCloudEditSession())
	{
		m_CurrentGame->m_TreeWidget->requestUpdate();
	}
    
    StudioUtilities::setAvatarMode(false);
    
    {
        RBX::DataModel::LegacyLock lock(m_EditGame.m_Game->getDataModel(), RBX::DataModelJob::Write);
        
        RBX::GameBasicSettings::singleton().setStudioMode(true);
        
        // Turn off the frame rate manager when we're editing.
        CRenderSettingsItem::singleton().setEnableFRM(false);
    }
    
    // again reinitialize the data for original data model
    if (FFlag::LuaDebugger)
    {
		initializeDebuggerData("", m_EditGame.m_Game->getDataModel(), m_EditDebuggersMap);
		m_EditDebuggersMap.clear();
        
        if (RobloxDocManager::Instance().getCurrentDoc() != this)
            RobloxDocManager::Instance().getCurrentDoc()->activate();
    }
}

void RobloxIDEDoc::onIdeRun(bool play)
{
	shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();

	// We receive a crash down the stack with m_GameView equal to NULL. One explanation is somehow user pressed onIdeRun before first paint message made it
	// Lets try to catch it with assert

	RBXASSERT(m_GameView);
	if(m_GameView == NULL)
		return;
    
    QString scriptStr;
    if (FFlag::UseBuildGenericGameUrl)
    {
        QString pathStr = QString("game/visit.ashx?IsPlaySolo=1&UserID=%1&PlaceID=%2&universeId=%3")
            .arg(RobloxUser::singleton().getUserId())
            .arg(m_EditGame.m_Game->getDataModel()->getPlaceID())
            .arg(m_EditGame.m_Game->getDataModel()->getUniverseId());
    
        scriptStr = QString("loadfile(\"%1\")()\n")
            .arg(QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(),pathStr.toStdString())));
    }
    else
    {
        scriptStr = QString("loadfile(\"%1/game/visit.ashx?IsPlaySolo=1&UserID=%2&PlaceID=%3&universeId=%4\")()\n")
            .arg(RobloxSettings::getBaseURL())
            .arg(RobloxUser::singleton().getUserId())
            .arg(m_EditGame.m_Game->getDataModel()->getPlaceID())
            .arg(m_EditGame.m_Game->getDataModel()->getUniverseId());
    }
	loadPlayDataModel(scriptStr, play, true);
	if (DFFlag::ScriptExecutionContextApi)
	{
		if (!play)
		{
			if (shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel())
			{
				playDataModel->setIsRunMode(true);
			}
		}
	}
}

bool RobloxIDEDoc::isIdeRunEnabled(bool play)
{
	if (m_PlayGame.m_Game) 
	{
        if (play != StudioUtilities::isAvatarMode())
        {
            return false;
        }
        
		shared_ptr<RBX::DataModel> playDataModel = m_PlayGame.m_Game->getDataModel();
		RBX::RunService* pRS = playDataModel->create<RBX::RunService>();

		return pRS->getRunState() != RBX::RS_RUNNING;
	}
    else
    {
        shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();
        RBX::RunService* pRS = dataModel->create<RBX::RunService>();
		bool clientIsPresent = RBX::Network::Players::clientIsPresent(dataModel.get()) &&
			!isCloudEditSession();
	    return !clientIsPresent && (pRS->getRunState() != RBX::RS_RUNNING);
    }
}

void RobloxIDEDoc::onIdeReset()
{
	if (m_PlayGame.m_Game)
	{
		closePlayDataModel();
	}
	else
	{
		shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		dataModel->create<RBX::RunService>()->stop();
		dataModel->create<RBX::ChangeHistoryService>()->reset();
	}

	updateOnSimulationStateChange(false);
}

bool RobloxIDEDoc::isIdeResetEnabled()
{
	return m_PlayGame.m_Game.get();
}

bool RobloxIDEDoc::isCloudEditSession()
{
	return m_IsCloudEditSession.get();
}

bool RobloxIDEDoc::getIsCloudEditSession()
{
	if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
	{
		return ideDoc->isCloudEditSession();
	}
	return false;
}

bool RobloxIDEDoc::isObjectRenameEnabled()
{
	QList<QTreeWidgetItem *> selectedItems = m_CurrentGame->m_TreeWidget->selectedItems();
	if (selectedItems.size() == 1)
		return true;

	return false;
}

void RobloxIDEDoc::onObjectRename()
{
	if (m_CurrentGame->m_TreeWidget->currentItem())
		m_CurrentGame->m_TreeWidget->editItem(m_CurrentGame->m_TreeWidget->currentItem());
}

RBX::Verb* RobloxIDEDoc::getVerb(const QString& name) const
{
	RBXASSERT(m_CurrentGame);
	return getVerb(*m_CurrentGame, name);
}

RBX::Verb* RobloxIDEDoc::getVerb(GameState& gameState,const QString& name) const
{
    std::string stdname = name.toStdString();
	for (int i = 0; i < gameState.m_Verbs.size() ; ++i)
	{
		if (gameState.m_Verbs[i]->getName().str == stdname)
			return gameState.m_Verbs[i];
	}

	return NULL;
}

bool RobloxIDEDoc::autoSave(bool force)
{
	FASTLOG1(FLog::AutoSave, "Auto-saving, force: %u", force);
    // don't save while we're running
    if (isSimulating())
        return true;

	// don't save if running Play Solo, Start Server, Start Player
    if (StudioUtilities::isTestMode())
        return true;

	// don't save unless we need to

	if (!m_EditGame.m_Game || !m_EditGame.m_Game->getDataModel() || !m_EditGame.m_Game->getDataModel()->isDirty())
	{
		FASTLOG(FLog::AutoSave, "DM is not dirty, skipping");
		return true;
	}

    if (force)
    {
        // force wait for a lock - this can cause a lockup if we're already loading or saving
		FASTLOG(FLog::AutoSave, "Locking mutex");
        m_IOMutex.lock();
    }
    else if (!force && !m_IOMutex.tryLock())
    {
        // if we're saving or loading, just skip this auto-save for now
		FASTLOG(FLog::AutoSave, "Mutex is locked, skipping");
        return true;
    }

	QByteArray currentDataModelHash;
    try
    {
		currentDataModelHash = StudioSerializerHelper::getDataModelHash(m_EditGame.m_Game->getDataModel().get());
		if (m_autoSaveDataModelHash == currentDataModelHash)
		{
			FASTLOG(FLog::AutoSave, "DM hash is same, skipping");
			m_IOMutex.unlock();
			return true;
		}			

        if (m_AutoSaveName.isEmpty())
        {
            QString fileName;
            if (m_fileName.isEmpty() || !QFile::exists(m_fileName))
                fileName = "Place";
            else
                fileName = QFileInfo(m_fileName).baseName();

			FASTLOGS(FLog::AutoSave, "AutoSave name is empty, constructing filename: %s", fileName.toStdString());

            QDir dir;
            QString autoSavePath = AuthoringSettings::singleton().autoSaveDir.absolutePath();
            if (!dir.mkpath(autoSavePath))
            {
				FASTLOGS(FLog::AutoSave, "Can't make path: %s", autoSavePath.toStdString());
                m_IOMutex.unlock();
                return false;
            }

            QString fullFileName = QString("%1/%2_AutoSave_%3.rbxl").
                arg(autoSavePath).
                arg(fileName);

            int count = 0;
            while (QFile::exists(fullFileName.arg(count)) && count < 100)
                ++count;

			FASTLOGS(FLog::AutoSave, "Full filename: %s", fullFileName.toStdString());

            RBXASSERT(count < 100);
            m_AutoSaveName = fullFileName.arg(count);
        }

        m_IOMutex.unlock();
    }
    catch(...)
    {
		FASTLOG(FLog::AutoSave, "Error on filename construction");
        m_IOMutex.unlock();
        return false;
    }

    try
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"Auto-Saving...");
        bool result = saveAs(m_AutoSaveName);
		if (result)
			m_autoSaveDataModelHash = currentDataModelHash;
		return result;
    }
    catch(...)
    {
		FASTLOG(FLog::AutoSave, "Error on AutoSave");
    	return false;
    }
}

/**
 * Set flag indicating that this place was loaded from an autosave file.
 *  The autosave file will not be deleted at close unless the file is saved with a different
 *  file name than the autosave name.
 */
void RobloxIDEDoc::setAutoSaveLoad()
{
	FASTLOGS(FLog::AutoSave, "File was loaded from autosave, name: %s", m_fileName.toStdString());
    m_AutoSaveName = m_fileName;
    m_fileName.clear();
    m_IsAutoSaveLoad = true;
}

void RobloxIDEDoc::resetDirty(RBX::DataModel* dataModel)
{
	if (!dataModel)
		return;
	
	m_initialDataModelHash = StudioSerializerHelper::getDataModelHash(dataModel);
	dataModel->setDirty(false);
	m_RequiresSave = false;
}

/**
 * Return the list of file filters for Open/Save dialogs
 */

const char* RobloxIDEDoc::getOpenFileFilters()
{
	return "All Roblox Place Files (*rbxl *rbxlx);;Roblox Place Files (*.rbxl);;Roblox XML Place Files (*.rbxlx)";
}

const char* RobloxIDEDoc::getSaveFileFilters()
{
	return "Roblox Place Files (*.rbxl);;Roblox XML Place Files (*.rbxlx)";
}

/**
 * Toggle in-game stats.
 * 
 * @param   stat    stats verb name
 */
void RobloxIDEDoc::toggleStats(const QString& stats)
{
    RBX::Verb* verb = m_CurrentGame->m_Game->getDataModel()->getVerb(stats.toStdString());
    setStatsEnabled(stats,!verb->isChecked());
}

/**
 * Enable/disable in-game stats displays.
 * 
 * @param   stat        stats verb name
 * @param   enabled     true to show
 */
void RobloxIDEDoc::setStatsEnabled(const QString& stats, bool enabled, QAction* verbAction)
{
    RBX::Verb* verb = m_CurrentGame->m_Game->getDataModel()->getVerb(stats.toStdString());
    if (verb->isChecked() != enabled)
    {
        verb->doIt(m_CurrentGame->m_Game->getDataModel().get());
        if (verbAction)
            verbAction->setChecked(verb->isChecked());
    }

    QSettings settings;
    settings.setValue(stats,enabled);
}

bool RobloxIDEDoc::displayAskConvertPlaceToNewMaterialsIfInsertNewModel()
{
	bool conversionDialogDisabled = RobloxSettings().value(kMaterialPropertiesConvertDialogDisabled, false).toBool();
	if (!conversionDialogDisabled)
	{
		// Create Dialog Box
		std::string labelMessage = std::string("The model you are inserting was designed to work with the new Physical Properties system.") + 
			"\n\n To make this model work with your place, would you like to convert the place to work with new Physical Properties?\n";
		QtUtilities::RBXConfirmationMessageBox msgBox(
			QObject::tr(labelMessage.c_str()),
			QObject::tr("Convert Place"),
			QObject::tr("Do Not Convert")
			);
		msgBox.setMinimumWidth(350);
		msgBox.setMinimumHeight(90);

		// Execute Dialog Box
		if (FFlag::CLI10590_FixConversionDialog)
		{
			UpdateUIManager::Instance().setBusy(true, false);
			QApplication::setOverrideCursor(Qt::ArrowCursor);
			msgBox.exec();
			QApplication::restoreOverrideCursor();
			UpdateUIManager::Instance().setBusy(false, false);
		}
		else
		{
			msgBox.exec();
		}

		// Process Dialog Response
		bool convertResponse = false;
		if (msgBox.getClickedYes())
		{
			convertResponse = true;
		}

		if (msgBox.getCheckBoxState())
		{
			RobloxSettings().setValue(kMaterialPropertiesConvertDialogDisabled, true);
			RobloxSettings().setValue(kMaterialPropertiesConvertByDefault, convertResponse);
		}

		return convertResponse;
	}

	return RobloxSettings().value(kMaterialPropertiesConvertByDefault, false).toBool();
}

void RobloxIDEDoc::onWorkspacePropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
	if (pDescriptor->name == RBX::Workspace::prop_physicalPropertiesMode.name)
	{
		switch (getEditDataModel()->getWorkspace()->getPhysicalPropertiesMode())
		{
		case PhysicalPropertiesMode_Legacy:
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Physical Properties Mode", "Legacy"); 
			break;
		case PhysicalPropertiesMode_Default:
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Physical Properties Mode", "Default");
			break;
		case PhysicalPropertiesMode_NewPartProperties:
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Physical Properties Mode", "New");
			break;
		}

		if (DFFlag::WorkspaceNotificationMasterEnable || DFInt::StudioWorkspaceNotificationLevel)
			displayWorkspaceMessage();

		if (getEditDataModel()->getWorkspace()->getUsingNewPhysicalProperties())
		{
			if (DataModel* dm = DataModel::get(getEditDataModel()->getWorkspace()))
			{
				RBX::PartInstance::convertToNewPhysicalPropRecursive(dm);
			}
		}
	}
}

static void createDebuggerClient(shared_ptr<RBX::Instance> instance, RBX::Scripting::DebuggerManager* pDebuggerManager)
{
	if (!instance || instance->isDescendantOf(RBX::ServiceProvider::find<RBX::CoreGuiService>(instance.get())))
		return;

	shared_ptr<RBX::Script> pScript = RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance);
	if (pScript)
	{
		//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Script Name: %s", pScript->getName().c_str());
		pDebuggerManager->addDebugger(pScript.get());
	}
	else
	{
		shared_ptr<RBX::ModuleScript> pModuleScript = RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance);
		if (pModuleScript)
			pDebuggerManager->addDebugger(pModuleScript.get());
	}
}

void RobloxIDEDoc::initializeDebuggerData(const QString &debugFile, const shared_ptr<RBX::DataModel> dm, const tDebuggersMap& debuggersMap)
{
	if (!FFlag::LuaDebugger || (m_pMainWindow->getBuildMode() == BM_BASIC))
		return;

	RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
	
	// before setting data model make sure we 'cleanup' client manager to avoid receiving unnecessary signals being called
	if (dm != DebuggerClientManager::Instance().getDataModel())
		DebuggerClientManager::Instance().setDataModel(shared_ptr<RBX::DataModel>());

	//set data model (so we can resolve references and save debugger data)
	debuggerManager.setDataModel(dm.get());

	//connect with debugger client manager also
	DebuggerClientManager::Instance().setDataModel(dm);

	//load debugger data
	if (debugFile.isEmpty())
	{
		updateDebuggerManager(debuggersMap);
	}
	else
	{
		QFileInfo debugFileInfo(debugFile);
		if (debugFileInfo.exists())
		{
			//deserialize data
			std::ifstream stream(qPrintable(debugFile), std::ios_base::in | std::ios_base::binary);

			try
			{
				RBX::SerializerBinary::deserialize(stream, &debuggerManager);
			}

			catch (std::runtime_error const&)
			{			
				stream.close();
				QFile::remove(debugFile); //remove invalid debug info file

				//remove all children from debug manager and again set datamodel
				debuggerManager.setDataModel(NULL);			
				debuggerManager.setDataModel(dm.get());
			}
		}
	}

	dm->visitDescendants(boost::bind(&createDebuggerClient, _1, &debuggerManager));
}

/**
 * Sets the current grid snap mode.
 * 
 * @param   gridMode    new grid snap mode
 */
void RobloxIDEDoc::setDraggerGridMode(RBX::DRAG::DraggerGridMode gridMode)
{
	if ( gridMode == RBX::DRAG::ONE_STUD )
		m_pMainWindow->gridToOneAction->activate(QAction::Trigger);
	else if ( gridMode == RBX::DRAG::QUARTER_STUD )
		m_pMainWindow->gridToOneFifthAction->activate(QAction::Trigger);
	else
		m_pMainWindow->gridToOffAction->activate(QAction::Trigger);
}

/**
 * Sets the current 3d grid size.
 * 
 * @param   gridSize    new 3d grid size
 */
void RobloxIDEDoc::setGridSize(int gridSize)
{
	if ( gridSize == 2 )
		m_pMainWindow->gridSizeToTwoAction->activate(QAction::Trigger);
	else if ( gridSize == 16 )
		m_pMainWindow->gridSizeToSixteenAction->activate(QAction::Trigger);
	else
		m_pMainWindow->gridSizeToFourAction->activate(QAction::Trigger);
}

static void placeNameLoadedHelperSuccess(QPointer<RobloxIDEDoc> doc, std::string json)
{
	if (doc.isNull() || json.empty()) return;
	QMetaObject::invokeMethod(doc, "placeNameLoaded", Qt::QueuedConnection,
		Q_ARG(QString, QString::fromStdString(json)));
}

static void placeNameLoadedHelperError(QPointer<RobloxIDEDoc> doc, std::string error)
{
	// didn't get name, just do nothing for now
}

void RobloxIDEDoc::placeNameLoaded(QString json)
{
	RBX::Reflection::Variant v;
	if (RBX::WebParser::parseJSONObject(json.toStdString(), v))
	{
		m_windowTitlePrefix = isCloudEditSession() ? kCloudEditPlacePrefix : kPublishedPlacePrefix;
		updateDisplayName(QString::fromStdString(v.cast<shared_ptr<const Reflection::ValueTable> >()->at("Name").cast<std::string>()));
	}
}


void RobloxIDEDoc::searchToolboxByDefaultSearchString()
{
	RBXASSERT(m_EditGame.m_Game->getDataModel()->write_requested);

	if (FFlag::SearchToolboxByDataModelSearchString)
		if (Instance* defaultToolboxSearchInstance = m_EditGame.m_Game->getDataModel()->findFirstChildByName("DefaultToolboxSearch"))
			if (StringValue* stringValue = RBX::Instance::fastDynamicCast<StringValue>(defaultToolboxSearchInstance))
			{
				QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxToolBox>(eDW_TOOLBOX),
					"loadUrl", Qt::QueuedConnection, Q_ARG(QString, 
					QString("%1/%2%3").arg(RobloxSettings::getBaseURL()).arg("ide/clienttoolbox?searchset=").arg(stringValue->getValue().c_str())));
			}
}

/**
 * Slot called on emit of "workspaceLoadedSignal" signal
 */
void RobloxIDEDoc::onContentDataLoaded()
{
	// required only at the time of loading
	m_contentDataLoadedConnection.disconnect();


	if (DFFlag::MaterialPropertiesEnabled)
	{
		RBXASSERT(!m_workspacePropertyChangedConnection.connected());
		m_workspacePropertyChangedConnection = getEditDataModel()->getWorkspace()->propertyChangedSignal.connect(boost::bind(&RobloxIDEDoc::onWorkspacePropertyChanged, this, _1));
	}

	//Migrating CSG childData
	CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(m_EditGame.m_Game->getDataModel().get());
	ChangeHistoryService* changeHistory = ServiceProvider::create<ChangeHistoryService>(m_EditGame.m_Game->getDataModel().get());

	dictionaryService->reparentAllChildData();

	changeHistory->resetBaseWaypoint();

	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		RBXASSERT(!m_dataModelItemChangedConnection.connected());
		m_dataModelItemChangedConnection = getEditDataModel()->itemChangedSignal.connect(boost::bind(&RobloxIDEDoc::onDMItemChanged, this, _1, _2));
	}

	// delay initialization of debugger data
	// while loading of contentData "Message" instance is created and once loading is done is gets deleted, 
	// but it's presence affects the child count in turn resulting in error on getting script from child number
	QTimer::singleShot(0, this, SLOT(onDelayedInitializeDebuggerData()));

    updateFromPlaceID(getDataModel()->getPlaceID());

	QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
		"openGameFromPlaceId", Qt::QueuedConnection, Q_ARG(int, m_EditGame.m_Game->getDataModel()->getPlaceID()));

	searchToolboxByDefaultSearchString();
}

/**
 * Slot called on emit of "saveFinishedSignal" signal
 */
void RobloxIDEDoc::onContentDataSaved()
{
	// save debugger data
	StudioSerializerHelper::saveDebuggerData(getDebugInfoFile());	
}

/**
 * Delayed initialization of debugger data
 */
void RobloxIDEDoc::onDelayedInitializeDebuggerData()
{
	shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();

	// initialize debugger data
	initializeDebuggerData(getDebugInfoFile(), dataModel);
	// connect with saved data signal, so we can save the debug data
	dataModel->saveFinishedSignal.connect(boost::bind(&RobloxIDEDoc::onContentDataSaved, this));
}

/**
 * Gets upload URL for current datamodel
 */
QString RobloxIDEDoc::getUploadURL()
{
	if (m_CurrentGame->m_Game && m_CurrentGame->m_Game->getDataModel())
	{
		if(RBX::Visit* pVisit = RBX::ServiceProvider::find<RBX::Visit>(m_CurrentGame->m_Game->getDataModel().get()))
			return pVisit->getUploadUrl().c_str();
	}
	return QString();
}

/**
 * Gets debug info file
 * 
 * @param   isAutoSaveFile    true, if we need debug info file corresponding to auto save file
 * @param   debuggerFileExt   extension to be added with the file name
 */
QString RobloxIDEDoc::getDebugInfoFile(bool isAutoSaveFile, const QString& debuggerFileExt)
{
	QString fileName;

	if (isAutoSaveFile)
	{
        // if we want to have autosave file name
		fileName = m_AutoSaveName;
	}
	else 
	{
		QString uploadURL = getUploadURL();
		if (!uploadURL.isEmpty())
		{
			// get name based on the URL and assetID (used in EDIT mode)
			fileName = uploadURL;
		}
		else if (m_fileName.isEmpty())
		{
			// if fileName is empty then get the display name
			fileName = displayName();
		}
		else
		{
			// local file on disk
			fileName = m_fileName;
		}
	}

	return StudioUtilities::getDebugInfoFile(fileName, debuggerFileExt);
}

void RobloxIDEDoc::notifyScriptEdited(shared_ptr<RBX::Instance> script)
{
	if (script)
	{
		m_playModifiedScriptList.push_back(new PlayModifiedScriptItem());
		PlayModifiedScriptItem& item = *m_playModifiedScriptList.back();
		item.instance = script;
		item.ancestryChangedConnection = script->ancestryChangedSignal.connect(
			boost::bind(&RobloxIDEDoc::onPlayModifiedAncestryChanged, this, script, _2));
	}
}

void RobloxIDEDoc::onPlayModifiedAncestryChanged(shared_ptr<RBX::Instance> script, shared_ptr<RBX::Instance> newParent)
{
	if (newParent==NULL)
	{
        for (tPlayModifiedScriptList::iterator iter = m_playModifiedScriptList.begin(); iter != m_playModifiedScriptList.end(); ++iter)
        {
            if ((*iter)->instance.get() == script.get())
            {
                delete (*iter);
                m_playModifiedScriptList.erase(iter);
                return;
            }
        }
    }
}

void RobloxIDEDoc::clearPlayModifiedScriptList()
{
    for (tPlayModifiedScriptList::iterator iter = m_playModifiedScriptList.begin(); iter != m_playModifiedScriptList.end(); ++iter)
    {
        delete (*iter);
    }
    
    m_playModifiedScriptList.clear();
}

void RobloxIDEDoc::addScriptMapping(shared_ptr<RBX::Instance> script, shared_ptr<RBX::DataModel> dataModel)
{
	if (!script)
		return;
    
    std::vector<int> indexHierarchy;
    std::string serviceName = getScriptIndexHierarchy(script, indexHierarchy);
    
    shared_ptr<RBX::Instance> instance = getScriptByIndexHierarchy(dataModel, indexHierarchy, serviceName);
	if (instance)
		m_scriptMap[instance.get()] = script;
}

std::string RobloxIDEDoc::getScriptIndexHierarchy(shared_ptr<RBX::Instance> script,  std::vector<int>& indexHierarchy)
{
	if (!script)
		return "";
    
    indexHierarchy.clear();
    
	const RBX::Instance* pParent = script->getParent();
	const RBX::Instance* pChild = script.get();
   
	while (pParent)
	{
		indexHierarchy.push_back(pParent->findChildIndex(pChild));

        // if this parent is a service and a child of the DataModel.
        if (pParent && dynamic_cast<const RBX::Service*>(pParent) &&
            pParent->getParent() &&
            pParent->getParent()->getParent() == NULL)
        {
            return pParent->getClassNameStr();
        }

		pChild = pParent;
		pParent = pChild->getParent();
	}

    return "";
}

shared_ptr<RBX::Instance> RobloxIDEDoc::getScriptByIndexHierarchy(shared_ptr<RBX::DataModel> dataModel, const std::vector<int>& indexHierarchy, const std::string& serviceName)
{
    if (serviceName == "")
        return shared_ptr<RBX::Instance>();

    shared_ptr<RBX::Instance> pParent = shared_from(dataModel->findFirstChildOfType(serviceName));
	shared_ptr<RBX::Instance> pChild;
    
    if (indexHierarchy.size() == 0)
        return shared_ptr<RBX::Instance>();
    
	for (int i = indexHierarchy.size() - 1; i >= 0; i--)
	{
		if (!pParent)
			return shared_ptr<RBX::Instance>();
        
        int childNum = indexHierarchy[i];
		if (childNum >= (int)pParent->numChildren())
			return shared_ptr<RBX::Instance>();
        
		pChild = shared_from(pParent->getChild(childNum));
		pParent = pChild;
	}

	if (pChild && (pChild->isA<RBX::Script>() || pChild->isA<RBX::ModuleScript>()))
		return pChild;
	return shared_ptr<RBX::Instance>();
}

boost::shared_ptr<RBX::Instance> RobloxIDEDoc::getEditScriptByPlayInstance(RBX::Instance* instance)
{
    tScriptMap::iterator iter = m_scriptMap.find(instance);
    
    if (iter != m_scriptMap.end())
        return iter->second;
    
    return boost::shared_ptr<RBX::Instance>();
}

void RobloxIDEDoc::initializeVideoRecording(GameState& gameState)
{
#ifdef _WIN32
	if (gameState.m_View && m_pQOgreWidget)
	{
		RBXASSERT(!gameState.m_RecordToggleVerb);

		gameState.m_RecordToggleVerb = new RecordToggleVerb(gameState.m_Game->getDataModel().get(), &gameState.m_View->getViewBase());
		gameState.m_Verbs.append(gameState.m_RecordToggleVerb);
		mapActionIDWithVerb(*m_CurrentGame, "toggleVideoRecordAction", gameState.m_RecordToggleVerb);
	}
#endif
}

void RobloxIDEDoc::cleanupVideoRecording(GameState& gameState)
{
#ifdef _WIN32
	if (gameState.m_RecordToggleVerb)
	{
		if (gameState.m_RecordToggleVerb->isRecording())
			gameState.m_RecordToggleVerb->stopRecording(false);

		gameState.m_Verbs.remove(gameState.m_Verbs.indexOf(gameState.m_RecordToggleVerb.get()));
		delete gameState.m_RecordToggleVerb;
		gameState.m_RecordToggleVerb = NULL;
	}
#endif
}

void RobloxIDEDoc::saveEditModelDebuggerData()
{
	if (m_EditGame.m_Game && m_EditGame.m_Game->getDataModel())
	{
		RBX::DataModel* dm = m_EditGame.m_Game->getDataModel().get();
		dm->visitDescendants(boost::bind(&RobloxIDEDoc::addToDebuggersMap, this, _1, boost::ref(m_EditDebuggersMap)));
	}
}

void RobloxIDEDoc::updateEditModelDebuggerData()
{
	if (m_PlayGame.m_Game && m_PlayGame.m_Game->getDataModel())
	{
		RBX::DataModel* dm = m_PlayGame.m_Game->getDataModel().get();
		dm->visitDescendants(boost::bind(&RobloxIDEDoc::updateDebuggersMap, this, _1, boost::ref(m_EditDebuggersMap)));
	}
}

void RobloxIDEDoc::updateDebuggerManager(const tDebuggersMap& debuggers)
{
	// recreate all debuggers based upon the saved mapping
	RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
	boost::shared_ptr<RBX::Scripting::ScriptDebugger> debugger;
	boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> breakpoint;

	for (tDebuggersMap::const_iterator iter = debuggers.begin(); iter != debuggers.end(); ++iter)
	{
		debugger = debuggerManager.addDebugger(iter->first.get());
		if (debugger)
		{
			const std::list<DebuggerData::BreakpointData>& breakpoints = iter->second.breakpoints;
			for (std::list<DebuggerData::BreakpointData>::const_iterator bp_iter = breakpoints.begin(); bp_iter != breakpoints.end(); ++bp_iter)
			{
				breakpoint = debugger->setBreakpoint(bp_iter->line);
				if (breakpoint)
					RBX::Scripting::DebuggerBreakpoint::prop_Enabled.setValue(breakpoint.get(), bp_iter->enabled);
			}

			const std::list<std::string>& watches = iter->second.watches;
			for (std::list<std::string>::const_iterator w_iter = watches.begin(); w_iter != watches.end(); ++w_iter)
			{
				debugger->addWatch(*w_iter);
			}
		}
	}
}

void RobloxIDEDoc::addToDebuggersMap(shared_ptr<RBX::Instance> instance, tDebuggersMap& debuggersMap)
{
	if (RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance) || RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance))
	{
		RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
		if (RBX::Scripting::ScriptDebugger* debugger = debuggerManager.findDebugger(instance.get()))
			debuggersMap[instance] = getDebuggerData(debugger);
	}
}

void RobloxIDEDoc::updateDebuggersMap(shared_ptr<RBX::Instance> instance, tDebuggersMap& debuggersMap)
{
	if (RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance) || RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance))
	{
		shared_ptr<RBX::Instance> editScript = getEditScriptByPlayInstance(instance.get());
		if (editScript)
		{
			RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
			if (RBX::Scripting::ScriptDebugger* debugger = debuggerManager.findDebugger(instance.get()))
				debuggersMap[editScript] = getDebuggerData(debugger);
		}
	}
}

RobloxIDEDoc::DebuggerData RobloxIDEDoc::getDebuggerData(RBX::Scripting::ScriptDebugger* debugger)
{
	DebuggerData data;

	const Scripting::ScriptDebugger::Breakpoints& breakpoints = debugger->getBreakpoints();
	for (RBX::Scripting::ScriptDebugger::Breakpoints::const_iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
	{
		DebuggerData::BreakpointData bp;
		bp.line = (iter->second->getLine());
		bp.enabled = (iter->second->isEnabled());
		data.breakpoints.push_back(bp);
	}

	data.breakpoints.sort(RobloxIDEDoc::DebuggerData::BreakpointLineNumberOrder());

	const Scripting::ScriptDebugger::Watches& watches = debugger->getWatches();
	for (RBX::Scripting::ScriptDebugger::Watches::const_iterator iter = watches.begin(); iter != watches.end(); ++iter)
	{
		data.watches.push_back((*iter)->getCondition());
	}

	return data;
}

bool RobloxIDEDoc::isEditMode(const RBX::Instance* context)
{
	// serverIsPresent can return 'true' as soon as query for 'NetworkServer' service happens
	// so explictly checking if we've actually launched Studio in server mode?
	if (RobloxApplicationManager::instance().hasLocalServer())
		return false;
	return !RBX::Network::Players::clientIsPresent(context) && !RBX::Network::Players::findConstLocalPlayer(context);
}

void RobloxIDEDoc::forceViewSize(QSize viewSize)
{
	if (!m_pQOgreWidget)
		return;

	if (!viewSize.isNull())
		m_pQOgreWidget->setFixedSize(viewSize);
	else
	{
		m_pQOgreWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		m_pQOgreWidget->setMinimumSize(100, 100);
		m_pQOgreWidget->setMaximumSize(20000, 20000);
	}

	m_WrapperWidget->setMinimumSize(m_pQOgreWidget->minimumSize());
}

void RobloxIDEDoc::updateDisplayName(const QString& newDisplayName)
{
	if (m_displayName == newDisplayName)
		return;

	m_displayName = newDisplayName;
	refreshDisplayName();
}

void RobloxIDEDoc::refreshDisplayName()
{
	RobloxDocManager::Instance().setDocTitle(*this, displayName(), titleTooltip(), titleIcon());
}

void RobloxIDEDoc::forceReloadImages(const QStringList& contentIds)
{
	if (!m_CurrentGame->m_View || !m_CurrentGame->m_Game || !m_CurrentGame->m_Game->getDataModel()) return;

	RBX::ViewBase* pView = (&m_CurrentGame->m_View->getViewBase());
	shared_ptr<RBX::DataModel> dm(m_CurrentGame->m_Game->getDataModel());
	{
		RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);
		
		// Garbage collect calls clear content if the visual engine still has a ContentProvider,
		// but the visual engine's ContentProvider is cleared when the workspace is set to null.
		if (ContentProvider* cp = dm->find<ContentProvider>())
		{
			for (QStringList::const_iterator itr = contentIds.begin(); itr != contentIds.end(); ++itr)
			{
				cp->invalidateCache(ContentId(itr->toStdString()));
			}
		}

		for (int i = 0; i < contentIds.size(); ++i)
		{
			std::string contentId = contentIds.at(i).toStdString();
			pView->immediateAssetReload(contentId);
		}

		// This will trigger a full regeneration of geometry and refetching of textures.
		// Calling immediateAssetReload also causes the reload to retry failed textures.
		// Ideally immediateAssetReload could do a more targeted invalidation/regeneration eventually.
		pView->bindWorkspace(shared_ptr<RBX::DataModel>());
		pView->garbageCollect();
		pView->bindWorkspace(dm);
	}
}

const QIcon& RobloxIDEDoc::titleIcon() const
{
    static QIcon icon;
    if (icon.isNull())
        icon.addFile(QString::fromUtf8(":/images/onlinePlaceIcon.png"), QSize(), QIcon::Normal, QIcon::Off);
	
	if (DataModel* dm = getDataModel())
	{
		if (dm->getPlaceID() != 0)
		{
			return icon;
		}
	}

    return m_emptyIcon;
}

void RobloxIDEDoc::updateFromPlaceID(int placeId)
{
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(getDataModel()))
	{
		m_bIsLocalDocument = false;
		apiService->getAsync( RBX::format("marketplace/productinfo?assetId=%d", placeId),false, RBX::PRIORITY_DEFAULT,
			boost::bind(&placeNameLoadedHelperSuccess, QPointer<RobloxIDEDoc>(this), _1),
			boost::bind(&placeNameLoadedHelperError, QPointer<RobloxIDEDoc>(this), _1));
	}
}

const QString& RobloxIDEDoc::titleTooltip() const
{
    if (!m_fileName.isEmpty())
        return m_fileName;

    return m_displayName;
}

void RobloxIDEDoc::resetMouseCommand(const char* mouseCommandToReset)
{
	shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel();
	if (dataModel)
	{
		RBX::Workspace* workspace = dataModel->getWorkspace();
		RBX::MouseCommand* mouseCommand = workspace->getCurrentMouseCommand();
		if (mouseCommand)
		{
			if (strcmp(mouseCommand->getName().c_str(), mouseCommandToReset) == 0)
				workspace->setDefaultMouseCommand();
		}
	}
}
void RobloxIDEDoc::setMuteAllSounds(bool mute)
{
	if (!m_CurrentGame)
	{
		return;
	}

	if (!m_CurrentGame->m_Game)
	{
		return;
	}

	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	QAction* pAction = mainWindow.findChild<QAction*>("audioTurnOffAction");
	if (pAction && pAction->isChecked())
	{
		return;
	}

	if (boost::shared_ptr<RBX::DataModel> dataModel = m_CurrentGame->m_Game->getDataModel())
	{
		if (RBX::Soundscape::SoundService* soundService = RBX::ServiceProvider::create<RBX::Soundscape::SoundService>(dataModel.get()))
		{
			if (soundService->isMuted() != mute)
			{
				soundService->muteAllChannels(mute);
				if (pAction)
				{
					pAction->setChecked(mute);
				}
			}
		}
	}
}

void RobloxIDEDoc::initServerAudioBehavior()
{
	if (AuthoringSettings::singleton().getTestServerAudioBehavior() == AuthoringSettings::Enabled)
	{
		return;
	}

	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	
	// we want sound to behave like an online game, so completely disable sound
	// also make sure the radio buttons do not work
	if (AuthoringSettings::singleton().getTestServerAudioBehavior() == AuthoringSettings::OnlineGame)
	{
		RBX::Soundscape::SoundService::soundDisabled = true;
		const QString disabledAudioText = "Audio has been disabled. Please go to File->Settings->Studio and then Server Audio Behavior to change this.";

		if (QWidget* testMenu = mainWindow.findChild<QWidget*>("Test"))
		{
			if (QWidget* audioGroup = testMenu->findChild<QWidget*>("Audio"))
			{
				audioGroup->setEnabled(false);
				audioGroup->setToolTip(disabledAudioText);
				audioGroup->setStatusTip(disabledAudioText);
				if (QAction* audioToggleWidget = mainWindow.findChild<QAction*>("audioToggleAction"))
				{
					audioToggleWidget->setToolTip(disabledAudioText);
					audioToggleWidget->setStatusTip(disabledAudioText);
				}
			}
		}
	}
	// just mute the instance for now
	else if (AuthoringSettings::singleton().getTestServerAudioBehavior() == AuthoringSettings::Muted)
	{
		setMuteAllSounds(true);
		if (QAction* audioToggleWidget = mainWindow.findChild<QAction*>("audioToggleAction"))
		{
			audioToggleWidget->setChecked(true);
		}
	}
}

void RobloxIDEDoc::promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{	
	QString assetTypeStr(assetType.c_str());
	if (assetTypeStr.toLower() != "animation")
	{
		errorFunction("Requested asset type is not supported.");
		return;
	}

	// make sure we launch dialog in Main Thread
	QVariant resumeFunctionVar, errorFunctionVar;
	resumeFunctionVar.setValue<boost::function<void(int)> >(resumeFunction);
	errorFunctionVar.setValue<boost::function<void(std::string)> >(errorFunction);
	QMetaObject::invokeMethod(this, "showImportAssetDialog", Qt::QueuedConnection, 
		                      Q_ARG(QVariant, resumeFunctionVar), Q_ARG(QVariant, errorFunctionVar));
}

void RobloxIDEDoc::showImportAssetDialog(QVariant resumeFunctionVar, QVariant errorFunctionVar)
{
	if (!StudioUtilities::checkNetworkAndUserAuthentication())
	{
		boost::function<void(std::string)> errorFunction = errorFunctionVar.value<boost::function<void(std::string)> >();
		RBXASSERT(errorFunction);
		errorFunction("User not logged in.");
		return;
	}

	boost::function<void(int)> resumeFunction = resumeFunctionVar.value<boost::function<void(int)> >();
	RBXASSERT(resumeFunction);

	QString importUrl(QString("%1/studio/animations/import").arg(RobloxSettings::getBaseURL()));	

	if (FFlag::StudioEnableGameAnimationsTab)
	{
		RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
		if (gameExplorer.getCurrentGameId() > 0)
			importUrl.append(QString("?universeId=%1").arg(gameExplorer.getCurrentGameId()));
	}

	// show import dialog
	ImportAssetDialog importDlg(m_pMainWindow, importUrl, getDataModel());
	importDlg.exec();

	// resume function returning the selected asset id
	resumeFunction(importDlg.getAssetId());
}

QString RobloxIDEDoc::getRecentSaveFile()
{
	QString fileToSave;
	QString recentSavesPath = AuthoringSettings::singleton().recentSavesDir.absolutePath();
	if (QDir().mkpath(recentSavesPath))
	{
		QFileInfo fileInfo(displayName());
		QString baseName = fileInfo.baseName();
		// make sure file name doesn't have invalid characters, replace them with '_'
		baseName.replace( QRegExp( "[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]" ), QString( "_" ) );
		// differentiate between files with same name
		//    for local documents use hash created using file path 
		//    for online documents use place id
		if (m_bIsLocalDocument)
		{
			fileToSave = recentSavesPath+"/"+baseName+"_"+ QString::number(RBX::Hash::hash(QFileInfo(fileName()).absoluteFilePath().toStdString()))+".rbxl";	
		}
		else
		{
			fileToSave = recentSavesPath+"/"+baseName+"_"+QString::number(getDataModel()->getPlaceID())+".rbxl";
		}
	}
	return fileToSave;
}

void RobloxIDEDoc::clearOutputWidget()
{
	if (AuthoringSettings::singleton().clearOutputOnStart)
		m_pMainWindow->getOutputWidget()->clear();
}

void RobloxIDEDoc::onLocalPlayerAdded(shared_ptr<RBX::Instance> newPlayer)
{	
	m_localPlayerConnection = newPlayer->propertyChangedSignal.connect(boost::bind(&RobloxIDEDoc::onLocalPlayerNameChanged, this, _1));	
}

void RobloxIDEDoc::onLocalPlayerNameChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
	if (*pDescriptor == RBX::Instance::desc_Name)
	{
		QMetaObject::invokeMethod(this, "syncPlayerName", Qt::QueuedConnection);
	}
}

void RobloxIDEDoc::syncPlayerName()
{
	RBX::DataModel* datamodel = getDataModel();
	if (!datamodel)
		return;

	{
		RBX::DataModel::LegacyLock lock(datamodel, RBX::DataModelJob::Read);
		if (RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(datamodel))
			m_displayName = localPlayer->getName().c_str();
	}

	// update window title and tab title
	m_pMainWindow->updateWindowTitle();
	refreshDisplayName();
}

void RobloxIDEDoc::appendTimeStampToDisplayName()
{
	QString updatedDisplayName = displayName();
	updatedDisplayName.append(" @ ");
	updatedDisplayName.append(QDateTime::currentDateTime().toString("dd MMM yyyy hh:mm"));
	updateDisplayName(updatedDisplayName);
}

void RobloxIDEDoc::removeTimeStampFromDisplayName()
{
	QString updatedDisplayName = displayName();
	int index = updatedDisplayName.lastIndexOf(" @ ");
	if (index > 0)
	{
		updatedDisplayName = updatedDisplayName.left(index);
		updateDisplayName(updatedDisplayName);
	}
}

void RobloxIDEDoc::updateOnSimulationStateChange(bool run)
{
	if (isCloudEditSession())
		run ? appendTimeStampToDisplayName() : removeTimeStampFromDisplayName();
}

void RobloxIDEDoc::setTeamCreateWidgetEnabled(bool enable)
{
	boost::shared_ptr<RBX::DataModel> dataModel;
	boost::shared_ptr<PlayersDataManager> playersDataManager;

	// enable only for local doc and edit mode
	if (enable && !m_initializationScript.isEmpty())
		enable = StudioUtilities::containsEditScript(m_initializationScript);

	if (enable)
	{
		dataModel = m_EditGame.m_Game->getDataModel();
		playersDataManager = m_EditGame.m_PlayerDataManager;
	}
		
	RobloxTeamCreateWidget& pTeamCreateWidget = UpdateUIManager::Instance().getViewWidget<RobloxTeamCreateWidget>(eDW_TEAM_CREATE);
	pTeamCreateWidget.setDataModel(dataModel, playersDataManager);

	if (enable)
	{
		// make sure we raise the dock widget to make sure it's on top if part of a tab
		QWidget* pParentWidget = qobject_cast<QWidget*>(pTeamCreateWidget.parent());
		if (pParentWidget && pParentWidget->isVisible())
			QMetaObject::invokeMethod(pParentWidget, "raise", Qt::QueuedConnection);
	}
}

void RobloxIDEDoc::initCloudEditObjects()
{
	if (!isCloudEditSession())
	{
		RBXASSERT(false);
		return;
	}

	shared_ptr<RBX::DataModel> dataModel = m_EditGame.m_Game->getDataModel();
	m_EditGame.m_PlayerDataManager.reset(new PlayersDataManager(this, dataModel));

	shared_ptr<PlayersDataManager> playersDataManager = m_EditGame.m_PlayerDataManager;
	m_CloudEditAdornable.attach(dataModel, playersDataManager);

	// since we want the chat widget to be enabled in Play solo session also, widget is not part of game state
	QStackedWidget* pChatWidgetStack = UpdateUIManager::Instance().getChatWidgetStack();
	if (pChatWidgetStack)
	{
		m_ChatWidget.reset(new RobloxChatWidget(pChatWidgetStack, dataModel, playersDataManager));
		pChatWidgetStack->addWidget(m_ChatWidget.get());
		pChatWidgetStack->setCurrentWidget(m_ChatWidget.get());
	}

	m_EditGame.m_TreeWidget->setPlayersDataManager(playersDataManager);
}

QString RobloxIDEDoc::downloadPlaceCopy()
{
	RobloxSettings settings;
	QString rbxmLastDir = settings.value("rbxm_last_directory").toString();
	if (rbxmLastDir.isEmpty())
		rbxmLastDir = RobloxMainWindow::getDefaultSavePath();

	QString fileName = QFileDialog::getSaveFileName(&UpdateUIManager::Instance().getMainWindow(),
		                                            "Download a Copy",
		                                            rbxmLastDir,
		                                            saveFileFilters());

	if (!fileName.isEmpty())
	{
		settings.setValue("rbxm_last_directory", QFileInfo(fileName).absolutePath());

		bool result = saveAs(fileName);
		if (result)
		{
			QString message = QString("Successfully downloaded a copy to '%1'").arg(QDir::toNativeSeparators(fileName));
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, qPrintable(message));
			return fileName;
		}
	}

	return QString();
}

void RobloxIDEDoc::enableDownloadPlaceCopy(bool state)
{
	// show only downloadPlaceCopyAction, reset of the actions must be invisible for CloudEdit
	UpdateUIManager::Instance().getMainWindow().downloadPlaceCopyAction->setVisible(state);
	UpdateUIManager::Instance().getMainWindow().publishToRobloxAsAction->setVisible(!state);
	UpdateUIManager::Instance().getMainWindow().fileSaveAction->setVisible(!state);
	UpdateUIManager::Instance().getMainWindow().fileSaveAsAction->setVisible(!state);
	UpdateUIManager::Instance().getMainWindow().publishToRobloxAction->setVisible(!state);
}

static const int kMaxRecentlySavedFile = 5;

static QString recentlySavedFilesFile()
{
	QString xmlFilePath(AuthoringSettings::singleton().recentSavesDir.absolutePath());
	xmlFilePath.append("/");
    xmlFilePath.append("RecentSaves.xml");
	return xmlFilePath;
}

RecentlySavedFilesHandler::RecentlySavedFilesHandler()
{ readFileList(); }

RecentlySavedFilesHandler::~RecentlySavedFilesHandler()
{ saveFileList(); }

void RecentlySavedFilesHandler::addFile(const QString& fileName)
{
	removeFile(fileName);
	m_RecentlySavedFiles.prepend(fileName);
}

void RecentlySavedFilesHandler::removeFile(const QString& fileName)
{
	if (m_RecentlySavedFiles.contains(fileName))
		m_RecentlySavedFiles.removeAll(fileName);
}

void RecentlySavedFilesHandler::updateFileList()
{
	while (m_RecentlySavedFiles.size() > kMaxRecentlySavedFile)
	{
		QFile::remove(m_RecentlySavedFiles.last());
		m_RecentlySavedFiles.removeLast();
	}
}

void RecentlySavedFilesHandler::saveFileList()
{
	QFile file(recentlySavedFilesFile());
	if (file.open(QFile::WriteOnly | QFile::Text))
	{
		//update list
		updateFileList();

		// create dom document
		QDomDocument doc;
		QDomElement root = doc.createElement("RecentSaves");
		doc.appendChild(root);
		for (int ii = 0; ii < m_RecentlySavedFiles.size(); ii++)
		{
			QDomElement newElement = doc.createElement("file");
			newElement.setAttribute("name", m_RecentlySavedFiles.at(ii));
			root.appendChild(newElement);
		}
		// save dom document to file
		QTextStream out(&file);
		out << doc.toString();
		file.close();
	}
}

void RecentlySavedFilesHandler::readFileList()
{
	m_RecentlySavedFiles.clear();

	QFile file(recentlySavedFilesFile());
	if (file.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(&file)) 
		{
			QDomElement domElement = doc.documentElement();
			for (QDomElement iter = domElement.firstChildElement(); !iter.isNull(); iter = iter.nextSiblingElement()) 
			{
				if (iter.tagName().toLower() == "file")
				{
					QString fileName = iter.attribute("name");
					if (QFileInfo(fileName).exists())
						m_RecentlySavedFiles.append(fileName);
				}
			}
		}
	}
}

const char* attentionMessageStyleSheet = "QFrame{background:rgb(246, 136, 2);}:";
static const char* kWorkspaceNotificationMessageAlreadyClosed = "WorkspaceNotificationMessageAlreadyClosed";


std::string getNotificationId()
{
	return DFString::WorkspaceMessageText + boost::to_string(DFInt::StudioWorkspaceNotificationLevel);
}

WorkspaceAnnouncementTooltip::WorkspaceAnnouncementTooltip(QWidget* parent)
{
	resize(QSize(1,1));
	setAttribute(Qt::WA_ShowWithoutActivating);
	setFocusPolicy(Qt::NoFocus);
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::ToolTip | Qt::FramelessWindowHint);
	QSize parentSize = parent->size();
	show();
	setVisible(false);
	resize(parentSize.width(), 50);

	QHBoxLayout* mainLayout   = new QHBoxLayout();

	toolTipLabel = new QLabel(this);
	toolTipLabel->setGeometry(4, 4, width(), 20);
	toolTipLabel->setOpenExternalLinks(true);
	toolTipLabel->setWordWrap(true);
	toolTipLabel->setAlignment(Qt::AlignCenter);
	toolTipLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	toolTipLabel->setTextFormat(Qt::RichText);
	toolTipLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

	closeButton = new QPushButton(this);
	closeButton->setIcon(QIcon(":/RibbonBar/images/Studio 2.0 icons/32x32/x_white_32.png"));
	closeButton->setIconSize(QSize(32, 32));
	closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	closeButton->setFlat(true);


	mainLayout->addWidget(toolTipLabel, 8);
	mainLayout->addWidget(closeButton, 1);

	setLayout(mainLayout);
	setMaximumHeight(50);
	setAutoFillBackground(true);
	setStyleSheet(attentionMessageStyleSheet);
	setParent(parent);

	setLastClosedMessage(RobloxSettings().value(kWorkspaceNotificationMessageAlreadyClosed, "").toString());
	connect(closeButton, SIGNAL (clicked()), this, SLOT (closeText()));
}

void WorkspaceAnnouncementTooltip::showText()
{
	if (getLastClosedMessage() != QString(getNotificationId().c_str()))
	{
		if (!isVisible())
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "WorkspaceNotification - ShowText", getNotificationId().c_str());
			QString customTextHyperlinkFormat;
			QFont boldFont = toolTipLabel->font();
			boldFont.setPointSize(10);
			boldFont.setBold(true);
			if (DFString::WorkspaceMessageLink == std::string(""))
			{
				// No Link
				customTextHyperlinkFormat = QString("<a style='color: white;'>%2</a>");
				toolTipLabel->setText(customTextHyperlinkFormat.arg( 
					DFString::WorkspaceMessageText.c_str()));
			}
			else
			{
				customTextHyperlinkFormat = QString("<a href=%1 style='color: white;'>%2</a>");
				toolTipLabel->setText(customTextHyperlinkFormat.arg(
					DFString::WorkspaceMessageLink.c_str(), 
					DFString::WorkspaceMessageText.c_str()));
			}
			toolTipLabel->setFont(boldFont);
		}


		QPoint ownerPosition = parentWidget()->pos();
		setGeometry(ownerPosition.x(), ownerPosition.y(), parentWidget()->size().width(), 50);
		setVisible(true);
	}
}

void WorkspaceAnnouncementTooltip::hideText()
{
	if (isVisible())
	{
		setVisible(false);
	}
}

void WorkspaceAnnouncementTooltip::closeText()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "WorkspaceNotification - CloseText", getNotificationId().c_str());
	setVisible(false);
	RobloxSettings().setValue(kWorkspaceNotificationMessageAlreadyClosed, getNotificationId().c_str());
	setLastClosedMessage(getNotificationId().c_str());
}



