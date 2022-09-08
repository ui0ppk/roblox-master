/**
 * RobloxMainWindow.cpp
 * Copyright (c) 2013 ROBLOX Corp. All rights reserved.
 */

#include "stdafx.h"
#include "RobloxMainWindow.h"

// Qt Headers
#include <QNetworkProxyFactory>
#include <QFileOpenEvent>
#include <QDateTime>
#include <QScrollBar>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDir>
#include <QComboBox>
#include <QToolTip>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QCompleter>
#include <QWidgetAction>
#include <QProcess>
#include <QSignalMapper>
#include <QSplashScreen>
#include <QSharedMemory>
#include <QShortcut>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QResource>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QtConcurrentRun>

// 3rd Party Headers
#include "boost/filesystem/path.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/array.hpp"

// Roblox Headers
#include "util/standardout.h"
#include "util/ScopedAssign.h"
#include "util/RbxStringTable.h"
#include "util/SoundService.h"
#include "script/ScriptContext.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/PartInstance.h"	
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Explosion.h"
#include "v8datamodel/PluginManager.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/Stats.h"
#include "v8world/Contact.h"
#include "v8kernel/Body.h"
#include "v8kernel/ContactConnector.h"
#include "rbx/BaldPtr.h"
#include "rbx/CEvent.h"
#include "Network/Players.h"
#include "CountersClient.h"
#include "FastLog.h"
#include "SharedLauncher.h"
#include "RobloxStudioVersion.h"
#include "RobloxQuickAccessConfig.h"
#include "RenderSettingsItem.h"
#include "RobloxServicesTools.h"

#include "v8datamodel/CSGMesh.h"
#include "../CSG/CSGKernel.h"

#ifdef _WIN32
	#include "VersionInfo.h"
	#include "WinInet.h"
    #include <atlsync.h>
#else
    #include "BreakpadCrashReporter.h"
    #include <semaphore.h>
#endif

// Roblox Studio Headers
#include "FunctionMarshaller.h"
#include "RobloxDocManager.h" 
#include "RobloxCustomWidgets.h"
#include "Roblox.h"
#include "RobloxSettings.h"
#include "RobloxSettingsDialog.h"
#include "RobloxDiagnosticsView.h"
#include "RobloxScriptReview.h"
#include "RobloxTaskScheduler.h"
#include "CommonInsertWidget.h"
#include "RobloxToolBox.h"
#include "StudioUtilities.h"
#include "RobloxTextOutputWidget.h"
#include "QtUtilities.h"
#include "AuthoringSettings.h"
#include "UpdateUIManager.h"
#include "RobloxPluginHost.h"
#include "WebDialog.h"
#include "RobloxApplicationManager.h"
#include "AuthenticationHelper.h"
#include "RobloxIDEDoc.h"
#include "RobloxInputConfigDialog.h"
#include "RobloxKeyboardConfig.h"
#include "RobloxMouseConfig.h"
#include "RobloxStudioVerbs.h"
#include "RobloxScriptDoc.h"
#include "ShortcutHelpDialog.h"
#include "RobloxUser.h"
#include "AutoSaveDialog.h"
#include "InsertServiceDialog.h"
#include "StudioIntellesense.h"
#include "RobloxWebDoc.h"
#include "RbxWorkspace.h"
#include "FindDialog.h"
#include "ScriptTextEditor.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "StudioDeviceEmulator.h"

FASTFLAGVARIABLE(StudioCheckForUpgradeEnabled, false)
FASTFLAGVARIABLE(StudioSeparateActionByActivationMethod, false)
FASTFLAGVARIABLE(StudioFixMacStartPage, false)
FASTFLAGVARIABLE(StudioSplashScreenUntilOpen, false)
FASTFLAGVARIABLE(StudioEarlyCookieConstraintCheckGlobal, false)
FASTSTRINGVARIABLE(StudioCookieConstraintUrlFragment, "fulfillconstraint")

FASTINTVARIABLE(StudioRobloxAnalyticsLoad, 100);
FASTINTVARIABLE(StudioBootstrapperVersionNumber, 52886)
FASTINTVARIABLE(StudioInsertDeletionCheckTimeMS, 8000)

FASTFLAGVARIABLE(StudioCustomStatsEnabled, false)
FASTFLAGVARIABLE(StudioSettingsGAEnabled, true)
FASTFLAGVARIABLE(AssertOnConfigurationReportToGA, true)
FASTFLAGVARIABLE(ReportBuildVSEditMode, true)
FASTFLAGVARIABLE(TeamCreateEnableDownloadLocalCopy, true)
FASTFLAGVARIABLE(StudioDoublingOnUploadFixEnabled, true)

FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAG(StudioInSyncWebKitAuthentication)
FASTFLAG(StudioDataModelIsStudioFix)
FASTFLAG(StudioShowTutorialsByDefault)
FASTFLAG(DontSwallowInputForStudioShortcuts)
FASTFLAG(StudioSettingsGAEnabled)
FASTFLAG(UseBuildGenericGameUrl)

FASTINT(StudioWebDialogMinimumWidth)
FASTINT(StudioWebDialogMinimumHeight)

DYNAMIC_LOGGROUP(GoogleAnalyticsTracking)
LOGGROUP(Network)

FASTFLAGVARIABLE(Dep, true)

static const int MaxLengthFilenameMRU   = 64;                    //!< maximum length of a filename in the MRU list
static const char* sWindow_Title        = "ROBLOX Studio";  //!< default main window title when no place selected
static const char* sDialog_Title        = "ROBLOX Studio";       //!< default child dialog title
static const int SplashTotalTime        = 3505;                  //!< minimum time to display splashscreen

bool RobloxMainWindow::sIsAppRunning = false;
const QString RobloxMainWindow::NEW_PLACE_FILENAME = "ROBLOX_NEW_PLACE";

bool RobloxMainWindow::preventActionCounterSending = false;

QString RobloxMainWindow::sWindowStateKey = "window_state";
QString RobloxMainWindow::sGeometryKey = "window_geometry";

#ifdef _WIN32
	#include "LogManager.h"
	#include "DumpErrorUploader.h"

	static MainLogManager mainLogManager("RobloxStudio", ".RobloxStudio.dmp", ".RobloxStudio.crashevent");
	static boost::scoped_ptr<DumpErrorUploader> dumpErrorUploader;
#endif

#ifdef __APPLE__
	#include "LogProvider.h"
	static LogProvider logProvider;

    #include "StudioMacUtilities.h"
#endif

RobloxMainWindow* RobloxMainWindow::get(QObject* context)
{
	while(context != NULL && dynamic_cast<RobloxMainWindow*>(context) == NULL)
	{
		context = context->parent();
	}

	return static_cast<RobloxMainWindow*>(context);
}

RobloxMainWindow::RobloxMainWindow(const QMap<QString, QString> argMap)
: RobloxRibbonMainWindow(this)
, m_pSettingsDialog(NULL)
, m_pMinutesPlayedTimer(NULL)
, m_pInsertServiceDlg(NULL)
, m_pShortcutHelpDialog(NULL)
, m_pInputConfigDialog(NULL)
, m_pPluginHost(new RobloxPluginHost(this))
, m_splashScreen(NULL)
, m_publishGameDialog(NULL)
, m_pCounterSenderTimer(NULL)
, m_AutoSaveAccum(0)
, m_IsInitialized(false)
, m_pPublishedProjectsWebDialog(NULL)
, m_isRibbon(false)
, m_fileOpenHandled(false)
, m_BuildMode(BM_ADVANCED)
, m_IgnoreCloudEditDisconnect(false)
, m_editScriptActions("EditScript", &studioAnalytics)
, m_mouseActions("ManipulatePart", &studioAnalytics)
, m_inserts("Inserts", &studioAnalytics)
, m_cookieConstraintChecker(NULL)
, m_cookieConstraintCheckDone(NULL)
{
	try
	{
		QNetworkProxyFactory::setUseSystemConfiguration(true);

		// grab all of our known args from the map (to be used later)
		parseCommandLineOptions(argMap);

        QTime splashTime = QTime::currentTime();
        if ( showEventArg.isEmpty() )
        {
            // can't set window flags in constructor because other ones are appended that we don't want
            m_splashScreen = new QSplashScreen(this,QPixmap(":/images/RobloxStudioSplash.png"));

#ifdef Q_WS_WIN32
            Qt::WindowFlags flags = Qt::SplashScreen | Qt::FramelessWindowHint;
#else
            // Qt::Tool makes the window on top of the Z order on Mac
            Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint;
#endif

            // now we can set window flags
            m_splashScreen->setWindowFlags(flags);  

            m_splashScreen->raise();
            m_splashScreen->show();
        }

		//start the bootstrapper on Mac, start the Event on Windows
		checkUpdater(true,startEventArg);
		        
		SetBaseURL(RobloxSettings::getBaseURL().toStdString());
		
		// make sure we call this first, this will initialize boost related stuff also 
		// or else we can run into raise condition which doing HTTP init as being mentioned here
		// - https://svn.boost.org/trac/boost/ticket/6320
		boost::filesystem::path::codecvt();

		RBX::Game::globalInit(true);

		// get logging going quickly, so any output is recorded
		setupLogging();
        RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxMainWindow::RobloxMainWindow - start");

		RBX::HttpFuture settingsFuture = FetchClientSettingsDataAsync(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY);
		try 
		{
			LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, settingsFuture.get(), &RBX::ClientAppSettings::singleton());
		} 
		catch(std::exception&)
		{
			FASTLOG(FLog::Error, "Failed to load settings");
		}

		AuthenticationHelper::Instance().authenticateUserAsync(urlArg, ticketArg);

        //set up ui using the .ui file
        setupUi(this);

		// set up external analytics reporting variables
		QString country = QLocale::countryToString(QLocale::system().country());
#ifdef _WIN32
		RBX::Analytics::setReporter("PC Studio");
#else
		RBX::Analytics::setReporter("Mac Studio");
#endif
		RBX::Analytics::setLocation(country.toStdString());
		RBX::Analytics::setAppVersion(RobloxSettings::getVersionString().toStdString());

		// set up influxdb reporting
		RBX::Analytics::InfluxDb::init();

        //initializing textOutputWidget early to catch any output in globalInit
        m_pTextOutput = new RobloxTextOutputWidget(dockWidgetContents_2);
		
		RBX::Http robloxRequest(AuthenticationHelper::getLoggedInUserUrl().toStdString());
		RBX::Http externalRequest("http://www.google.com");

		QSettings retentionData("Roblox", "Retention");

		static const char* const kRetentionInstallDateKey = "InstallDate";
		static const char* const kRetentionLastRunDateKey = "LastRunDate";
		QString dateString = QDate::currentDate().toString("yyyyMMdd");
		QString installDate = retentionData.value(kRetentionInstallDateKey, "").toString();

		if (installDate.isEmpty())
		{
			StudioUtilities::setIsFirstTimeOpeningStudio(true);
		}

		// Init our engine - can only use ClientAppSettings AFTER this!!!
		Roblox::globalInit(urlArg, ticketArg, settingsFuture);
		RBX::Stats::reportGameStatus("AppStarted");	// must be after gloablInit due to http initialization

        Qtitan::RibbonStyle::setStyleVersion(1);
		
		// Disable FRM in studio by default
		CRenderSettingsItem::singleton().setEnableFRM(false);
        
		// Following are the different modes launched from website 
		// (this is required since website conveys 'only' to be launched from Studio but doesn't specify the mode
		// 1) Build mode: BuildArgument == True (default mode set)
		// 2) Edit mode: BuildArgument == True && ScriptArgument has edit.ashx
		// 3) Start Page mode: BuildArgument == True && ScriptArgument is empty
		if (StudioUtilities::containsEditScript(argMap[SharedLauncher::ScriptArgument])
			|| ((argMap[SharedLauncher::BuildArgument]=="TRUE") && argMap[SharedLauncher::ScriptArgument].isEmpty()))
		{
			m_BuildMode = BM_ADVANCED;
			StudioUtilities::setAvatarMode(false);
		}
        
		{
#ifdef __APPLE__
			int useCurlPercentage = RBX::ClientAppSettings::singleton().GetValueHttpUseCurlPercentageMacStudio();
#else
			int useCurlPercentage = RBX::ClientAppSettings::singleton().GetValueHttpUseCurlPercentageWinStudio();
#endif
			bool useCurl = rand() % 100 < useCurlPercentage;
#ifdef _WIN32
			{
				INTERNET_PER_CONN_OPTION_LIST    List;
				INTERNET_PER_CONN_OPTION         Option[1];
				unsigned long                    nSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);

				Option[0].dwOption = INTERNET_PER_CONN_FLAGS;

				List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
				List.pszConnection = NULL;
				List.dwOptionCount = 1;
				List.dwOptionError = 0;
				List.pOptions = Option;

				if (InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &nSize))
				{
					if (Option[0].Value.dwValue & PROXY_TYPE_PROXY)
					{
						// As of when this was added, curl could not handle ssl over proxies
						// so fall back to win inet when proxy is detected
						useCurl = false;
					}
				}
			}
#endif
			FASTLOG1(FLog::Network, "Use CURL = %d", useCurl);
			RBX::Http::SetUseCurl(useCurl);
		}

        RBX::Http::useDefaultTimeouts = false;
		
		// init google analytics
		std::string googleAnalyticsAccountPropId = RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyID();
		if (m_BuildMode == BM_BASIC)
		{
			// Set the account property id to either the Build Mode production account or the test account
			// by checking the baseURL for the company domain "roblox.com".
			QString baseURL(RobloxSettings().getBaseURL());
			if (baseURL.endsWith("roblox.com", Qt::CaseInsensitive))
				googleAnalyticsAccountPropId = "UA-43420590-4"; // Production
			else
				googleAnalyticsAccountPropId = "UA-43420590-5"; // Test
		}

		RBX::RobloxGoogleAnalytics::lotteryInit(googleAnalyticsAccountPropId, 
			RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize(), RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadStudio(), "studio",
			FInt::StudioRobloxAnalyticsLoad, "studioSid=");

		if(StudioUtilities::isFirstTimeOpeningStudio())
		{
			installDate = dateString;
			retentionData.setValue(kRetentionInstallDateKey, dateString);
			RBX::RobloxGoogleAnalytics::trackEventWithoutThrottling("Retention-Install",
				dateString.toStdString().c_str(), "none", 1);
		}

		QString lastRun = retentionData.value(kRetentionLastRunDateKey, "").toString();
		if (lastRun != dateString)
		{
			retentionData.setValue(kRetentionLastRunDateKey, dateString);
			RBX::RobloxGoogleAnalytics::trackEventWithoutThrottling("Retention-Run",
				installDate.toStdString().c_str(), dateString.toStdString().c_str(), 1);
		}

		// Send tracking information about the application.
		if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			QString label = m_BuildMode == BM_BASIC ? "Basic" : "IDE";

			// Extract everything from scriptArg before ".ashx"
			int httpIndex = scriptArg.indexOf("http");
			int ashxIndex = scriptArg.indexOf(".ashx");
			if(httpIndex != -1 && ashxIndex > httpIndex)
				label += " " + scriptArg.mid(httpIndex, ashxIndex - httpIndex + 5);

			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Start", label.toStdString().c_str());
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Version", RobloxSettings::getVersionString().toStdString().c_str());
		}

		QtConcurrent::run(this, &RobloxMainWindow::checkInternetConnectionSendCounter, robloxRequest, externalRequest);

		if (AuthoringSettings::singleton().getUIStyle() == AuthoringSettings::Ribbon)
        {
			m_isRibbon = true;
            RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_COUNTERS, "RibbonBar", "Launches", 1);
        }

		if (FFlag::StudioSettingsGAEnabled)
		{
			RBX::Reflection::PropertyIterator iter = AuthoringSettings::singleton().properties_begin();
			RBX::Reflection::PropertyIterator end = AuthoringSettings::singleton().properties_end();

			while (iter!=end)
			{
				RBX::Reflection::Property property = *iter;
                std::string valueString;
                if (!FFlag::AssertOnConfigurationReportToGA)
                {
                    valueString = property.getStringValue();
                }
				std::string description = property.getName().str;
				std::transform(description.begin(), description.end(), description.begin(), tolower);

				if (!(description == "parent" || description == "archivable" || description == "classname" || description == "datacost" 
					|| description == "name" || description == "robloxlocked"))
				{
					description += " On Start";
                    
                    if (FFlag::AssertOnConfigurationReportToGA)
                    {
                        valueString = "N/A";
                        if (property.hasStringValue())
                        {
                            valueString = property.getStringValue();
                        }
                    }
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO_SETTINGS, description.c_str(), valueString.c_str());
				}
				++iter;
			}

			AuthoringSettings::singleton().setDoSettingsChangedGAEvents(true);
		}

		//check for crash
		RobloxSettings settings;
		if (RobloxApplicationManager::instance().getApplicationCount() == 1 && settings.contains("appClosed") && !settings.value("appClosed").toBool())
		{
			sendCounterEvent("QTStudioCrashRecorded");
			if (FFlag::GoogleAnalyticsTrackingEnabled)
			{
				RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "Crash");
			}
		}
		settings.setValue("appClosed", false);

#ifdef _WIN32
        if (FFlag::Dep)
        {
            typedef BOOL (WINAPI *SetProcessDEPPolicyPfn)(DWORD);
            SetProcessDEPPolicyPfn pfn= reinterpret_cast<SetProcessDEPPolicyPfn>(GetProcAddress(GetModuleHandle("Kernel32"), "SetProcessDEPPolicy"));
            if (pfn)
            {
                static const DWORD kEnable = 1;
                pfn(kEnable);
            }
        }
#endif

		if (FFlag::StudioCheckForUpgradeEnabled)
		{
			RobloxSettings settings;
			if(settings.value("studioVersion").toString() != RobloxSettings::getVersionString())
			{
				settings.setValue("studioVersion", RobloxSettings::getVersionString());
				sendCounterEvent("StudioUpgradedCounter", false);
                if (FFlag::GoogleAnalyticsTrackingEnabled)
				{
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Upgrade", RobloxSettings::getVersionString().toStdString().c_str());
				}
			}
		}
		
        // Set the CSGMesh Factory to use sgCore.
        RBX::CSGMeshFactory::set(new RBX::CSGMeshFactorySgCore());

        initializeUI();

		//setup WikiSearch in scripts
		ScriptTextEditor::setupWikiLookup();

		// reset the position of the toolbars. so they are identical to the old studio.
		setToolbarPosition();

		//set up command toolbar
		setupCommandToolBar();

		//create material and color tool button
		setupCustomToolButton();

		//sends out slow counters every X seconds
		setupCounterSender();
		       
		//set default commands
		UpdateUIManager::Instance().setDefaultApplicationState();

		sIsAppRunning = true;

		// Set Security context Identity
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);

		//setup statusbar and docks
		UpdateUIManager::Instance().init(this);
		
		RobloxScriptDoc::init(*this);
		
		if (isRibbonStyle())
		{
			setupRibbonBar();
		}
		else
		{
			setupViewMenu();
		}

		//set up slots
		setupSlots();
		
		 // set up shortcut key sequences
		assignAccelerators();

		updateRecentFilesUI();

        // Idempotent function - and if there is no open tab, we need to update actions (disable ones that need to be)
		UpdateUIManager::Instance().updateToolBars();

        setBuildMode(m_BuildMode);

		// save current state as default state
		saveDefaultApplicationState();

		// load the application states from memory
		loadApplicationStates();
				
        m_PropertyChangedConnection = AuthoringSettings::singleton().propertyChangedSignal.connect(
            boost::bind(&RobloxMainWindow::onPropertyChanged,this,_1));
        onPropertyChanged(NULL);

        if (FFlag::StudioEarlyCookieConstraintCheckGlobal ||
			RobloxSettings::getBaseURL().contains("robloxlabs", Qt::CaseInsensitive))
		{
			m_cookieConstraintChecker = new QWebView(NULL);
			m_cookieConstraintCheckDone = new QEventLoop(this);
			m_cookieConstraintChecker->setAttribute(Qt::WA_DeleteOnClose);
			m_cookieConstraintChecker->page()->setNetworkAccessManager(&RobloxNetworkAccessManager::Instance());
			connect(m_cookieConstraintChecker->page(), SIGNAL(loadStarted()), m_cookieConstraintChecker, SLOT(hide()));
			connect(m_cookieConstraintChecker, SIGNAL(loadFinished(bool)), this, SLOT(cookieConstraintCheckerLoadFinished(bool)));
			connect(m_cookieConstraintChecker, SIGNAL(destroyed(QObject*)), m_cookieConstraintCheckDone, SLOT(quit()));
			m_cookieConstraintChecker->load(RobloxSettings::getBaseURL());
			m_cookieConstraintCheckDone->exec();
			m_cookieConstraintCheckDone->deleteLater();
			m_cookieConstraintCheckDone = NULL;
			// if the window is destroyed from close we want to null it out here.
			m_cookieConstraintChecker = NULL;
		}
        
#ifdef _WIN32
        if ( showEventArg.isEmpty() ) 
            show();
#else
        show();
        
        // bootstrapper will wait for this event to exit
        sem_t *sem = sem_open("/robloxStudioStartedEvent", 0);
        if (sem != SEM_FAILED)
        {
            sem_post(sem);
            sem_close(sem);
        }
#endif
		
        // take the args that were passed in and act upon them
		// has to be at the very end of the constructor, or else the menu bar will behave weird.

		handleCommandLineOptions();
    	m_fileOpenHandled = false;

        //Mac will open file through finder event; need to handle open start page this way
        IRobloxDoc* currentPlayDoc = RobloxDocManager::Instance().getPlayDoc();
        if (!currentPlayDoc)
        {
            // if nothing else opened, open a start web page
            actionStartPage->setChecked(true);
        }

#ifdef _WIN32
        if ( !showEventArg.isEmpty() ) 
        {
            // Application waits for this event before showing to the user
	        ATL::CEvent showEvent;
	        if ( showEvent.Open(EVENT_MODIFY_STATE, FALSE, qPrintable(showEventArg)) )
                ::WaitForSingleObject(showEvent,30 * 1000);
        }

        show();

		if ( !readyEventArg.isEmpty() ) 
		{
			// Bootstrapper will wait for this event, to make sure that app is loaded and ready
			ATL::CEvent readyEvent;
			if ( readyEvent.Open(EVENT_MODIFY_STATE, FALSE, qPrintable(readyEventArg)) )
				::WaitForSingleObject(readyEvent,30 * 1000);
		}
#endif

		// Unfortunately has to be called after show due to windowState not loading until then.
		UpdateUIManager::Instance().updateDockActionsCheckedStates();

		viewCommandBarAction->setChecked(commandToolBar->isVisible());

		//set window layout setting according to application states.
		//it has to be after loadApplicationStates. or else full screen button will not be initialized correctly.
		setWindowLayout();

        if ( AutoSaveDialog::checkForAutoSaveFiles() )
        {
            AutoSaveDialog dialog(this);
            while ( dialog.exec() == QDialog::Rejected )
            {
                /* nothing */
            }
        }

		if ( m_splashScreen )
        {
            // show splash screen for a minimum amount of time
            int waitTime = 0;
            if ( RobloxApplicationManager::instance().getApplicationCount() == 1 )
                waitTime = qMax(0,SplashTotalTime - splashTime.elapsed());
            QTimer::singleShot(waitTime,this,SLOT(onDeleteSplashScreen()));
		}

        setupActionCounters();

        RobloxKeyboardConfig::singleton().storeDefaults(*this);
        RobloxKeyboardConfig::singleton().loadKeyboardConfig(*this);
        RobloxMouseConfig::singleton().loadMouseConfig();

		m_IsInitialized = true;
	}
	catch (std::runtime_error const& exp)
	{
		// must make sure the splash screen is hidden before popping up any errors
        try
        {
            if ( m_splashScreen )
				onDeleteSplashScreen();
        }
        catch (...)
        {
			// ignore errors
        }

		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setText(exp.what());
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok); 
		msgBox.exec();
		throw;
	}
    catch (...)
    {
        // must make sure the splash screen is hidden before popping up any errors
        try
        {
            if ( m_splashScreen )
                onDeleteSplashScreen();
        }
        catch (...)
        {
			// ignore errors
        } 
        throw;
    }

    if (FFlag::StudioSplashScreenUntilOpen && m_splashScreen)
    {
        m_splashScreen->raise();
        m_splashScreen->show();
    }

    RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxMainWindow::RobloxMainWindow - end");
}

RobloxMainWindow::~RobloxMainWindow()
{
    RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxMainWindow::~RobloxMainWindow");
    StudioUtilities::stopMobileDevelopmentDeployer();

    m_PropertyChangedConnection.disconnect();

    //remove all documents
	RobloxDocManager::Instance().shutDown();

	//stop UI update
	UpdateUIManager::Instance().shutDown();

	//do a global shut down
	Roblox::globalShutdown();

	//Set appClosed true before shutdown
	if (RobloxApplicationManager::instance().getApplicationCount() == 1)
	{
		RobloxSettings settings;
		settings.setValue("appClosed", true);
	}
}

void RobloxMainWindow::setupLogging()
{
#ifdef _WIN32
	RBX::Log::setLogProvider(&mainLogManager);
	mainLogManager.WriteCrashDump();

	dumpErrorUploader.reset(new DumpErrorUploader(true, "Studio"));
	std::string dmpHandlerUrl =  GetDmpUrl(::GetBaseURL(), true);
	dumpErrorUploader->InitCrashEvent(dmpHandlerUrl, mainLogManager.getCrashEventName());
	dumpErrorUploader->Upload(dmpHandlerUrl);
#endif

#ifdef __APPLE__
	RBX::Log::setLogProvider(&logProvider);
#endif
}

void RobloxMainWindow::causeCrash()
{
	RBXCRASH();
}

bool RobloxMainWindow::checkUpdater(bool showUpdateOptionsDialog, const QString& initDoneEventName) const
{
#ifdef Q_WS_MAC
	QString bootstrapper = QCoreApplication::applicationFilePath(); 
	if(bootstrapper.lastIndexOf("RobloxStudio") > -1)
		bootstrapper = bootstrapper.replace(bootstrapper.lastIndexOf("RobloxStudio"), 12, "RobloxStudio.app/Contents/MacOS/RobloxStudio");
	else
		return false; //can't find proper path
	
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "boostrapper path: %s", qPrintable(bootstrapper));
	
	QStringList args;
	pid_t processID = getpid();
	QString ppid;
	ppid.setNum(processID);
	QString showUpdate = QString(showUpdateOptionsDialog ? "true" : "false");
	args << "-check" << "true" << "-ppid" << ppid << "-updateUI" << showUpdate;
	QProcess::startDetached(bootstrapper, args);
#else
    if ( !initDoneEventName.isEmpty() )
    {
	    // Bootstrapper will wait for this event, to make sure that app was started
	    ATL::CEvent robloxQtStudioStartedEvent;
	    if ( robloxQtStudioStartedEvent.Open(EVENT_MODIFY_STATE, FALSE, qPrintable(initDoneEventName)) )
		    robloxQtStudioStartedEvent.Set();
    }
#endif
	return true;
}

void RobloxMainWindow::parseCommandLineOptions(const QMap<QString, QString> argMap)
{
	fileLocationArg	= argMap[SharedLauncher::FileLocationArgument];
	urlArg 			= argMap[SharedLauncher::AuthUrlArgument];
	ticketArg 		= argMap[SharedLauncher::AuthTicketArgument];
    startEventArg   = argMap[SharedLauncher::StartEventArgument];
    readyEventArg   = argMap[SharedLauncher::ReadyEventArgument];
	showEventArg    = argMap[SharedLauncher::ShowEventArgument];
	scriptArg 		= argMap[SharedLauncher::ScriptArgument];
	QString sBrowserTrackerId = argMap[SharedLauncher::BrowserTrackerId];

	QString sWidth	= argMap[StudioUtilities::StudioWidthArgument];
	QString sHeight	= argMap[StudioUtilities::StudioHeightArgument];

	// convert loadfile('http://www.roblox.com/game/join.ashx')() to just the url
	if (StudioUtilities::containsJoinScript(scriptArg) && scriptArg.contains("loadfile("))
	{
		int urlBegin = scriptArg.indexOf("(")+2; // skip over the qoute
		int urlEnd = scriptArg.indexOf(")")-1;
		scriptArg = scriptArg.mid(urlBegin, urlEnd - urlBegin);
	}

    if ( argMap[SharedLauncher::BuildArgument] == "TRUE" )
        m_BuildMode = BM_BASIC;
    else if ( argMap[SharedLauncher::IDEArgument] == "TRUE" )
        m_BuildMode = BM_ADVANCED;

    if ( argMap[SharedLauncher::TestModeArgument] == "TRUE" )
        StudioUtilities::setTestMode(true);
    if ( argMap[SharedLauncher::AvatarModeArgument] == "TRUE" )
        StudioUtilities::setAvatarMode(true);

	if ( argMap[StudioUtilities::EmulateTouchArgument] == "TRUE" )
		StudioDeviceEmulator::Instance().setIsEmulatingTouch(true);

	if ( !sWidth.isEmpty())
		StudioDeviceEmulator::Instance().setCurrentDeviceWidth(sWidth.toInt());

	if ( !sHeight.isEmpty())
		StudioDeviceEmulator::Instance().setCurrentDeviceHeight(sHeight.toInt());

	if ( !sBrowserTrackerId.isEmpty())
		RBX::Stats::setBrowserTrackerId(sBrowserTrackerId.toStdString());
}

void RobloxMainWindow::handleCommandLineOptions()
{    
	// make sure sound is disabled if it should be before we create datamodel 
	if (StudioUtilities::isTestMode() && !StudioUtilities::isAvatarMode() && StudioUtilities::containsGameServerScript(scriptArg))
	{
		if (AuthoringSettings::singleton().getTestServerAudioBehavior() == AuthoringSettings::OnlineGame)
		{
			RBX::Soundscape::SoundService::soundDisabled = true;
		}
	}

    // check if fileOpen has already been handled
    if (m_fileOpenHandled)
        return;

    bool openedPlace = handleFileOpen(fileLocationArg,IRobloxDoc::IDE,scriptArg);
    if(!openedPlace)
    {
        // if nothing else opened, open a start web page
        actionStartPage->setChecked(true);
    }
#ifdef __APPLE__
    // if Studio is opening in sever/client mode then diable App Nap on Mac (DE13936)
    else if (StudioUtilities::containsGameServerScript(scriptArg) || StudioUtilities::containsJoinScript(scriptArg))
    {
        StudioMacUtilities::disableAppNap("Server/Client Mode");
    }
#endif

	if (openedPlace && isRibbonStyle() && StudioUtilities::isTestMode() && !StudioUtilities::isAvatarMode() && StudioUtilities::containsGameServerScript(scriptArg))
	{
		// start local server, so we can cleanup processes started by 'server'
		RobloxApplicationManager::instance().startLocalServer();
		// this is required to update client server related actions
		UpdateUIManager::Instance().updateToolBars();

		RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();

		int numPlayers = RobloxSettings().value("rbxRibbonNumPlayer").toInt();
		if (numPlayers > 0)
		{
			if (pIDEDoc && pIDEDoc->getDataModel())
			{
				StartServerAndPlayerVerb* pVerb = dynamic_cast<StartServerAndPlayerVerb*>(pIDEDoc->getDataModel()->getVerb("StartServerAndPlayerVerb"));
				if (pVerb)
					pVerb->launchPlayers(numPlayers);
			}			
		}
		StudioUtilities::startMobileDevelopmentDeployer();

		if (pIDEDoc && pIDEDoc->getDataModel())
		{
			pIDEDoc->initServerAudioBehavior();
		}
	}

    if (!openedPlace)
    {
        if (m_splashScreen)
            onDeleteSplashScreen();
    }
}

void RobloxMainWindow::moveEvent(QMoveEvent * event)
{
	RobloxRibbonMainWindow::moveEvent(event);
	updateEmbeddedFindPosition();
}

bool RobloxMainWindow::eventFilter(QObject* watched,QEvent* evt)
{
	if (FFlag::StudioSeparateActionByActivationMethod && evt->type() == QEvent::Shortcut)
	{
		if (QAction* action = dynamic_cast<QAction*>(watched))
		{
			if (commonSlotShortcut(action, !action->isChecked()))
			{
				evt->accept();
				return true;
			}
		}
	}

#ifdef Q_WS_MAC
	// handle special file open command on Mac coming from Finder
	if ( evt->type() == QEvent::FileOpen && watched == qApp )
	{
		if (FFlag::StudioFixMacStartPage)
    	{
        	// Make sure the start page is the first thing opened.
        	actionStartPage->setChecked(true);
		}

		m_fileOpenHandled = handleFileOpen(static_cast<QFileOpenEvent*>(evt)->file(), IRobloxDoc::IDE);
		return true;
	}
    else if (isRibbonStyle() && (evt->type() == QEvent::Polish) && watched->inherits("QMenu"))
    {
        static_cast<QMenu*>(watched)->setFont(QApplication::font());
    }
    else
#endif
    if (evt->type() == QEvent::KeyPress || evt->type() == QEvent::ShortcutOverride)
    {
        // ignore shortcut events during busy state
        if ( UpdateUIManager::Instance().isBusy() )
        {
            evt->accept();
            return true;
        }
    }

    if (evt->type() == QEvent::NonClientAreaMouseButtonPress)
        Studio::Intellesense::singleton().deactivate();

 	if (evt->type() == QEvent::MouseMove ||
 		evt->type() == QEvent::KeyPress)
 	{
 		keepAliveAnalyticsSession();
 	}

	if ( (evt->type() == QEvent::FocusIn || evt->type() == QEvent::FocusOut) &&
		 AuthoringSettings::singleton().onlyPlayFocusWindowAudio )
	{
		if (IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
		{
			if (RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc))
			{
				ide->setMuteAllSounds(!QApplication::activeWindow());
				if (QAction* audioToggleWidget = findChild<QAction*>("audioToggleAction"))
				{
					audioToggleWidget->setChecked(!QApplication::activeWindow());
				}
			}
		}
	}
	
	if ( watched == qApp )
		return qApp->eventFilter(watched,evt);
	else
		return QMainWindow::eventFilter(watched,evt);
}

void RobloxMainWindow::setWindowLayout()
{
	actionFullScreen->setChecked(isFullScreen());
}

void RobloxMainWindow::setupViewMenu()
{
	menuView->insertAction(menuView->actions().value(1), resetViewAction);
	menuView->insertSeparator(resetViewAction);		

	// Add dock widgets to view menu
    //  Insert after Start Page.  Start Page is first (index 0).
	UpdateUIManager::Instance().setupDockWidgetsViewMenu(*menuView->actions().value(1));    

	// Set up the toolbar
	menuToolBars->addAction(standardToolBar->toggleViewAction());
	menuToolBars->addAction(advToolsToolBar->toggleViewAction());
	menuToolBars->addAction(editCameraToolBar->toggleViewAction());
	menuToolBars->addAction(commandToolBar->toggleViewAction());
	menuToolBars->addAction(runToolBar->toggleViewAction());
	menuToolBars->addAction(viewToolsToolBar->toggleViewAction());
	if (!RBX::MouseCommand::isAdvArrowToolEnabled()) // Remove this, this flag is true
		menuToolBars->addAction(oldToolsToolBar->toggleViewAction());
}

void RobloxMainWindow::toggleFullScreen(bool state)
{
	if (isFullScreen() == state)
		return;
	
	// Toggle
	setWindowState(windowState() ^ Qt::WindowFullScreen);

    // Toggle fullscreen glitches with layout on Mac for Ribbon Bar (send event to update layout)
#ifdef Q_WS_MAC
	if (isRibbonStyle())
	{
        Qtitan::RibbonPage* pPage = ribbonBar()->getPage(ribbonBar()->currentIndexPage());
        if (pPage)
        {
            QEvent evt(QEvent::LayoutRequest);
            QApplication::sendEvent(pPage, &evt);
        }
    }
#endif

	// if we have an IDE, we need to lock the datamodel
    IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
    if ( playDoc )
    {
        RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc);
        if ( ide )
        {
            RBX::DataModel::LegacyLock lock(ide->getDataModel(),RBX::DataModelJob::Write);
	        RBX::GameBasicSettings::singleton().setFullScreen(state);
            return;
        }
    }

    RBX::GameBasicSettings::singleton().setFullScreen(state);
}

bool RobloxMainWindow::commonSlotHelper(const QString& objectName, bool isChecked)
{
	bool handled = false;

	//action to be handled by the relevant document
	if (RobloxDocManager::Instance().getCurrentDoc())
		handled = RobloxDocManager::Instance().getCurrentDoc()->handleAction(objectName, isChecked);

	// Otherwise handle it ourselves
	if (!handled)
	{

		// Handle global dock actions here
		if (UpdateUIManager::Instance().getDockActionNames().contains(objectName))
		{
			handled = UpdateUIManager::Instance().toggleDockView(objectName);
		}
		else
		{
			// Check if IDE Doc can handle the action
			IRobloxDoc* pPlayDoc = RobloxDocManager::Instance().getPlayDoc(); 
			if (pPlayDoc)
				handled = pPlayDoc->handleAction(objectName, isChecked);
		}
	}

	//update toolbar status
	UpdateUIManager::Instance().updateToolBars();

	return handled;
}

void RobloxMainWindow::commonSlot(bool isChecked)
{
	const QObject *pSender = sender();
	if (!pSender || dynamic_cast<QuickAccessBarProxyAction*>(sender()))
		return;

	if (!preventActionCounterSending || !FFlag::StudioSeparateActionByActivationMethod)
		sendActionCounterEvent(pSender->objectName());

	commonSlotHelper(pSender->objectName(), isChecked);
}

bool RobloxMainWindow::commonSlotShortcut(QAction* action, bool isChecked)
{
	if (!preventActionCounterSending)
		sendActionCounterEvent(action->objectName() + "Shortcut");

	return commonSlotHelper(action->objectName(), isChecked);
}

bool RobloxMainWindow::commonSlotQuickAccess(QAction* action)
{
	if (!preventActionCounterSending)
		sendActionCounterEvent(action->objectName() + "QuickAccess");

	return commonSlotHelper(action->objectName(), !action->isChecked());
}

void RobloxMainWindow::processAppEvent(void* pClosure)
{
	// Handle the event
	if (sIsAppRunning)
		RBX::FunctionMarshaller::handleAppEvent(pClosure);
	else	
		RBX::FunctionMarshaller::freeAppEvent(pClosure); // control flow comes here means application is closed
}

bool RobloxMainWindow::requestDocClose(IRobloxDoc& doc, bool closeIfLastDoc)
{
    if ( !doc.getViewer() )
    {
		RobloxDocManager::Instance().removeDoc(doc);
        return true;
    }

	if ( StudioUtilities::isVideoUploading() || StudioUtilities::isScreenShotUploading())
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setText(QString("ROBLOX is uploading %1. Quit anyway?").arg(StudioUtilities::isVideoUploading() ? "a video" : "an image"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);

		//user has pressed NO or close button, do not close the document
		if ( msgBox.exec() != QMessageBox::Yes )
			return false;	

		// user has pressed YES, modifying the uploading status to make sure 
		//  we don't show the message multiple times
		StudioUtilities::setVideoUploading(false);
		StudioUtilities::setScreenShotUploading(false);
	}

	if ( doc.isModified() )
	{
		IRobloxDoc::RBXCloseRequest closeMode = doc.requestClose();

		if ( closeMode == IRobloxDoc::CLOSE_CANCELED )
		{
			if ( &doc != RobloxDocManager::Instance().getCurrentDoc() )
                RobloxDocManager::Instance().setCurrentDoc(&doc);
			return false;
		}
        else if ( closeMode == IRobloxDoc::REQUEST_HANDLED )
		{
            // LUA will handle it from here on out
			return false;
		}
		else if ( closeMode != IRobloxDoc::NO_SAVE_NEEDED )
		{
            // ask the user if they want to save before close
		
			QtUtilities::RBXMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Question);
			msgBox.setText("Save changes to " + doc.displayName() + " ?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Yes);
			int ret = msgBox.exec();

			//if the user wants to save
			if ( ret == QMessageBox::Yes && !fileSave(&doc) )
            {
				// User hit the Yes to Save in the Yes No Cancel MsgBox,
				// but then did a Cancel on Save Dialog, change the state to Cancel
				ret = QMessageBox::Cancel;
            }
			
			//if the user want to cancel close
			if ( ret == QMessageBox::Cancel )
			{
				if ( &doc != RobloxDocManager::Instance().getCurrentDoc() )
                    RobloxDocManager::Instance().setCurrentDoc(&doc);
				return false;
			}
		}
    }

    // special case Start Page since it can be toggled in the View menu
	if ( doc.keyName() == "StartPage" && doc.docType() == IRobloxDoc::BROWSER )
	    actionStartPage->setChecked(false);
	else if ( doc.docType() == IRobloxDoc::OBJECTBROWSER )
	    objectBrowserAction->setChecked(false);
#ifdef Q_WS_MAC
    // On Mac, if we do not make the IDE document current then it results in DE7073
    // Combination of QWidget and ViewBase creation coupled with deletion of IDE document
    // without making it current results in non responding keyboard (?)
    else if ((doc.docType() == IRobloxDoc::IDE) && (&doc != RobloxDocManager::Instance().getCurrentDoc()))
        RobloxDocManager::Instance().setCurrentDoc(&doc);
#endif

	// WARNING: Do not use doc to call any function after this (it will get deleted)
    RobloxDocManager::Instance().removeDoc(doc);

    updateWindowTitle();

	if ( RobloxDocManager::Instance().getDocCount() == 0 && closeIfLastDoc &&
        (RobloxApplicationManager::instance().getApplicationCount() > 1 || getBuildMode() == BM_BASIC))
	{
		// If there are no tabs remaining and this is another instance of Studio open, close this one
		close();
	}

    return true;
}

void RobloxMainWindow::fileNew()
{
	handleFileOpen(NEW_PLACE_FILENAME, IRobloxDoc::IDE);
}

void RobloxMainWindow::openStartPage(bool checked, QString optionalQueryParams /* = "" */)
{
	if(checked)
	{
		QString fileToOpen;

		const char* startPageUrl = RBX::ClientAppSettings::singleton().GetValueStartPageUrl();
		if (startPageUrl && startPageUrl[0] != '\0')
		{
			// Generate start page url from our recent files -- these files are put into the query string
			fileToOpen = QString::fromStdString(GetBaseURL()).append(startPageUrl).append("?");
			RobloxSettings settings;
			QStringList recentFiles = settings.value("rbxRecentFiles").toStringList();
			for(int i = 0; i < qMin(recentFiles.length(), (int)MAX_RECENT_FILES); i++)
			{
				QFileInfo info(recentFiles.at(i));
				fileToOpen.append("&filepath=" + QUrl::toPercentEncoding(info.filePath()) + "&filename=" + QUrl::toPercentEncoding(info.fileName()));	
			}				
		}
		else
			fileToOpen = QString::fromStdString(GetBaseURL() + "/My/Places.aspx");

		if (!optionalQueryParams.isEmpty())
			fileToOpen.append("&").append(optionalQueryParams);

		// make sure we are authenticated (if there's any authentication happening) before we open the browser
		AuthenticationHelper::Instance().waitForQtWebkitAuthentication();

		handleFileOpen(fileToOpen, IRobloxDoc::BROWSER);
	}
	else
	{
		IRobloxDoc* pDoc = RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER);
        requestDocClose(*pDoc, false);
	}
}

void RobloxMainWindow::fileOpen()
{
    QString fileName;
	
	fileName = QFileDialog::getOpenFileName(
		this, 
		tr("Open Roblox File"), 
		m_LastDirectory, 
		tr(RobloxIDEDoc::getOpenFileFilters()));

	if (fileName.isEmpty())
		return;

    setCurrentDirectory(fileName);
	handleFileOpen(fileName, IRobloxDoc::IDE);
}

void RobloxMainWindow::fileOpenRecentSaves()
{
	QString fileName;
	
	fileName = QFileDialog::getOpenFileName(
		this, 
		tr("Open Roblox File"), 
		AuthoringSettings::singleton().recentSavesDir.absolutePath(), 
		tr(RobloxIDEDoc::getOpenFileFilters()));

	if (fileName.isEmpty())
		return;

    setCurrentDirectory(fileName);
	handleFileOpen(fileName, IRobloxDoc::IDE);
}

void RobloxMainWindow::fileClose()
{
    IRobloxDoc* doc = RobloxDocManager::Instance().getCurrentDoc();
	if ( doc )
        requestDocClose(*doc);
}

bool RobloxMainWindow::verifyFilePermissions(const QString &fileName)
{
	bool retVal = false;

	if (!fileName.isEmpty())
	{
		QFileInfo fileInfo(fileName);
		if (fileInfo.exists() && !fileInfo.isWritable()) 
		{
			QtUtilities::RBXMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText(fileName + "\nThis file is set to read-only.\nTry again with a different name");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
		else 
		{
			retVal = true;
		}
	}
	else /// In case of Empty File return it as True as the file is not present and new file should be created
	{
		retVal = true;
	}
	return retVal;
}

bool RobloxMainWindow::fileSave(IRobloxDoc *pDoc)
{
	bool retVal = false;
	
	//if no document is provided then get the play document. 
	if (!pDoc)
		pDoc = RobloxDocManager::Instance().getPlayDoc();
	
    if (pDoc && pDoc->isModified())
    {
        if (verifyFilePermissions(pDoc->fileName()))
        {
            if ( pDoc->fileName().isEmpty() || !pDoc->save() )
                retVal = fileSaveAs(pDoc);
            else
                retVal = true;
        }
    }
	return retVal;
}

bool RobloxMainWindow::fileSaveAs(IRobloxDoc *pDoc)
{
	bool retVal = false;
	
	//if no document is provided then get the play document
	if (!pDoc)
		pDoc = RobloxDocManager::Instance().getPlayDoc();
	
	if (pDoc)
	{
		QString fileName(pDoc->fileName());
		if ( fileName.isEmpty() &&
				boost::filesystem::portable_file_name(pDoc->displayName().toStdString()) )
			fileName = m_LastDirectory + "/" + pDoc->displayName();

		fileName = QFileDialog::getSaveFileName(this,tr("Save As"),fileName,pDoc->saveFileFilters());
			
		if( !fileName.isEmpty() && verifyFilePermissions(fileName) && pDoc->saveAs(fileName))
		{
            setCurrentDirectory(fileName);
            RobloxDocManager::Instance().setDocTitle(*pDoc,pDoc->displayName(),pDoc->titleTooltip(),pDoc->titleIcon());
            updateRecentFile(fileName);

            m_AutoSaveAccum = 0;
			retVal = true;
		}
	}

	return retVal;
}

bool RobloxMainWindow::filePublishedProjects()
{
	QString url = QString(RBX::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl());
	if (url.isEmpty())
		return false;

	if (!StudioUtilities::checkNetworkAndUserAuthentication())
		return false;
	
	WebDialog dlg(
        this, 
        url.prepend(RobloxSettings::getBaseURL()),
        NULL, 
		RBX::ClientAppSettings::singleton().GetValuePublishedProjectsPageWidth(), 
		RBX::ClientAppSettings::singleton().GetValuePublishedProjectsPageHeight() );
    dlg.setMinimumSize( FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
	dlg.exec();
	
	return true;
}

bool RobloxMainWindow::openRecentFile(const QString &fileName)
{
	if(!fileName.isEmpty())
    {
		QFileInfo fileInfo(fileName);
		if(!fileInfo.exists())
        {
            updateRecentFile(fileName);

            QtUtilities::RBXMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText(fileName + "\nThis file doesn't exist.");
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
	        msgBox.exec();
            return false;
        }
    }

	return handleFileOpen(fileName, IRobloxDoc::IDE);
}

bool RobloxMainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QString fileName = action->data().toString();
	if (action)
	{
		return openRecentFile(fileName);
	}
	else
		return false;
}

void RobloxMainWindow::publishGame()
{
	if (!StudioUtilities::checkNetworkAndUserAuthentication())
		return;

	QString initialUrl = QString("%1/IDE/publishgameas").arg(RobloxSettings::getBaseURL());

	if (!m_publishGameDialog) 
    {
		m_publishGameDialog = new WebDialog(this, initialUrl, NULL);
        m_publishGameDialog->setMinimumSize( FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
    }
	else
	{
		m_publishGameDialog->load(initialUrl);
	}

	m_publishGameDialog->show();
	m_publishGameDialog->raise();
	m_publishGameDialog->activateWindow();
}

void RobloxMainWindow::closePublishGameWindow()
{
	RBXASSERT(m_publishGameDialog);
	if (m_publishGameDialog)
	{
		m_publishGameDialog->close();
	}
}

void RobloxMainWindow::updateRecentFilesUI()
{
	if (isRibbonStyle())
	{
		RibbonPageSystemRecentFileList* pRecentFileList = findChild<RibbonPageSystemRecentFileList*>("recentFileList");
		if (pRecentFileList)
			pRecentFileList->updateRecentFileActions(getRecentFiles());
	}
	else
	{
		updateRecentFilesList(getRecentFiles());
	}
}

void RobloxMainWindow::updateRecentFilesList(const QStringList& files)
{
	int numRecentFiles = qMin(files.size(), (int)MAX_RECENT_FILES_SHOWN);

	for (int i = 0; i < numRecentFiles; ++i) 
    {
        QFileInfo fileInfo(files[i]);
        QString fullFilename = fileInfo.absoluteFilePath();
		QString filename = fullFilename;

        if ( filename.startsWith(m_LastDirectory) )
            filename = filename.right(filename.length() - 1 - m_LastDirectory.length());

		bool isTextModified = false;
		if ( filename.length() > MaxLengthFilenameMRU )
		{
			QFontMetrics fm = fontMetrics();
			filename = fm.elidedText(filename,Qt::ElideMiddle,MaxLengthFilenameMRU * fm.width('0'));
		    isTextModified = true;
		}

		recentOpenedFiles[i]->setText(QString("&%1 %2").arg(i + 1).arg(filename));

		if(isTextModified)
			recentOpenedFiles[i]->setToolTip(fullFilename);

		recentOpenedFiles[i]->setData(fullFilename);
		recentOpenedFiles[i]->setStatusTip(fullFilename);
		recentOpenedFiles[i]->setVisible(true);
	}

	for (int j = numRecentFiles; j < (int)MAX_RECENT_FILES_SHOWN; ++j)
		recentOpenedFiles[j]->setVisible(false);

	separator->setVisible(numRecentFiles > 0);
}

void RobloxMainWindow::updateInternalWidgetsState(QAction* pAction, bool enabledState,bool checkedState)
{
	if (isRibbonStyle())
		RobloxRibbonMainWindow::updateInternalWidgetsState(pAction, enabledState, checkedState);
}

#define COPYRIGHT_PREFIX 0xA9

void RobloxMainWindow::about()
{
	QString aboutMsg(tr("<div style=\"text-align: center\">"));
	aboutMsg.append(tr("<p style=\"font-weight: bold\">"));
    aboutMsg.append(tr("ROBLOX Studio "));
	aboutMsg.append(tr("Version %1</p>"));

#if defined(_NOOPT) || defined(_DEBUG) 
    aboutMsg.append(tr("<br><hr><br><br>DEBUG ONLY<br><hr>"));

    QString build_type;

#ifdef _NOOPT
    build_type = "NoOpt";
#else
    build_type = "Debug";
#endif

    aboutMsg.append("Build Type: " + build_type);
    aboutMsg.append("<br>Path: ");

    const int threshold = 50;
    QString full_path = QCoreApplication::applicationFilePath();
    while ( !full_path.isEmpty() )
    {
        aboutMsg.append(full_path.left(threshold) + "<br>");
        if ( full_path.length() >= threshold )
        {
            full_path = full_path.right(full_path.length() - threshold);
            if ( !full_path.isEmpty() )
                aboutMsg.append("&nbsp;&nbsp;&nbsp;&nbsp;");
        }
        else
        {
            full_path = "";
        }
    }

    aboutMsg.append("Qt Version: ");
    aboutMsg.append(qVersion());

	// display command line argument
    aboutMsg.append("<br><br>");
    aboutMsg.append("Arguments:<br>");
    
    QStringList args = QApplication::arguments();
    for ( int i = 1 ; i < args.size() ; ++i )
        aboutMsg.append(QString::number(i) + ") '" + args[i] + "'<br>");

    aboutMsg.append("<hr><br>");
#endif

	aboutMsg = aboutMsg.arg(RobloxSettings::getVersionString());
    aboutMsg.append(tr("<p>ROBLOX, \"Online Building Toy\", characters, logos, names, and all related indicia "));
	
	QString copyrightString;
	copyrightString.sprintf("are trademarks of ROBLOX Corporation %c2014. ", COPYRIGHT_PREFIX);
							
    aboutMsg.append(copyrightString);
	aboutMsg.append(tr("US Patents 7,874,921. Patents pending.</p>"));
					   
	aboutMsg.append(tr("<p>Contact us at info@roblox.com.</p></div>"));
	
	QMessageBox::about(this, tr("Welcome to ROBLOX!"), aboutMsg);
}

void RobloxMainWindow::onlineHelp()
{
	QUrl helpUrl("http://wiki.roblox.com/index.php/Studio", QUrl::TolerantMode);
	QDesktopServices::openUrl(helpUrl);
}

void RobloxMainWindow::shortcutHelp()
{
    if ( !m_pInputConfigDialog )
        m_pInputConfigDialog = new RobloxInputConfigDialog(*this);

    if ( m_pInputConfigDialog->isVisible() )
        m_pInputConfigDialog->hide();
    else
    {
        m_pInputConfigDialog->show();
        m_pInputConfigDialog->raise();
    }
}

void RobloxMainWindow::fastLogDump()
{
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Log Dumps");
#ifdef _WIN32
	mainLogManager.CreateFakeCrashDump();
#else
	CrashReporter::CreateLogDump();
#endif
}

void RobloxMainWindow::instanceDump()
{
}

void RobloxMainWindow::openPluginsFolder()
{	
	QString userPluginPath = RobloxPluginHost::userPluginPath();
	QDir pluginDir(userPluginPath);

	if (!pluginDir.exists())
		pluginDir.mkpath(userPluginPath);

	QDesktopServices::openUrl(QUrl::fromLocalFile(userPluginPath));
}

void RobloxMainWindow::managePlugins()
{
	if (!StudioUtilities::checkNetworkAndUserAuthentication())
		return;
	
	if (!m_managePluginsDoc)
	{
		m_managePluginsDoc.reset(new RobloxWebDoc(tr("Plugin Management"), "PluginManagement"));
		RobloxWebDoc* webDoc = dynamic_cast<RobloxWebDoc*>(
			RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER));
		connect(webDoc->getWorkspace().get(), SIGNAL(PluginInstallComplete(bool, int)),
				m_managePluginsDoc.get(),       SLOT(          refreshPage()));
	}
	m_managePluginsDoc->open(this,
		QString("%1/studio/plugins/manage").arg(RobloxSettings::getBaseURL()));
	QWidget *widgetToAddInTab = m_managePluginsDoc->getViewer();
	if (widgetToAddInTab)
	{
        RobloxDocManager::Instance().configureDocWidget(*m_managePluginsDoc);
	}
}

RobloxWebDoc* RobloxMainWindow::getConfigureWebDoc()
{
	if (!m_configureGameDoc)
	{
		m_configureGameDoc.reset(new RobloxWebDoc(tr("Configure"), "GameEntityConfigure"));
        RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER);
	}
	// The widget doesn't show up in the tabs unless open() has been called at least once.
    m_configureGameDoc->open(this, "");
	QWidget *widgetToAddInTab = m_configureGameDoc->getViewer();
	if (widgetToAddInTab)
	{
        RobloxDocManager::Instance().configureDocWidget(*m_configureGameDoc);
	}

	return m_configureGameDoc.get();
}

void RobloxMainWindow::closeConfigureWebDoc()
{
	if (m_configureGameDoc)
	{
		requestDocClose(m_configureGameDoc.get());
	}
}

void RobloxMainWindow::openObjectBrowser(bool checked)
{
	if ( checked )
    {
        handleFileOpen(QString(""), IRobloxDoc::OBJECTBROWSER);
    }
    else
    {
		IRobloxDoc* pDoc = RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::OBJECTBROWSER);
        requestDocClose(*pDoc);
	}
}

void RobloxMainWindow::openSettingsDialog()
{
	if ( !m_pSettingsDialog )
	    m_pSettingsDialog = new RobloxSettingsDialog(this);
	m_pSettingsDialog->exec();

    // perform any fixup if settings changed
    assignAccelerators();
}

void RobloxMainWindow::executeScriptFile()
{
	RBXASSERT(RobloxDocManager::Instance().getCurrentDoc());
	RBXASSERT(RobloxDocManager::Instance().getCurrentDoc() == RobloxDocManager::Instance().getPlayDoc());

	//check is there's no current document then create one
	if (!RobloxDocManager::Instance().getCurrentDoc())
		fileNew();

	//still no document? can't do anything!
	if (!RobloxDocManager::Instance().getCurrentDoc())
		return;

	//open the script file
    QString fileName;
	
	fileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Script File"),
		AuthoringSettings::singleton().defaultScriptFileDir.absolutePath(),
		tr("Scripts (*.rbxs *.lua *.txt);;All Files (*.*)"));

	if (fileName.isEmpty())
		return;

    QFileInfo info(fileName);
	AuthoringSettings::singleton().defaultScriptFileDir = info.dir();

	QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

	//read file
	QTextStream inStream(&file);
	QString toExecute = inStream.readAll();
	
	if (!toExecute.isEmpty())
    {
		if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
			ideDoc->handleScriptCommand(toExecute);
    }
}

void RobloxMainWindow::onCustomToolButton(const QString& selectedItem)
{
	const QObject *pSender = sender();
	if (!pSender)
		return;

	//action to be handled by the relevant document
	if (!RobloxDocManager::Instance().getCurrentDoc())
		return;
	
	QString modifiedActionName(pSender->objectName());
	modifiedActionName.append("_"); 
    modifiedActionName.append(selectedItem);

	RobloxDocManager::Instance().getCurrentDoc()->handleAction(modifiedActionName);

	//update toolbar status
	UpdateUIManager::Instance().updateToolBars();
}

void RobloxMainWindow::onMenuActionHovered(QAction* action)
{
	if(!action)
		return;

    for (int i = 0; i < MAX_RECENT_FILES_SHOWN; ++i) 
    {
		if(action == recentOpenedFiles[i])
		{
			QString tip = action->toolTip();
			if(tip != action->text().remove(0,1))
				QToolTip::showText(QCursor::pos(), tip);
			else
				QToolTip::hideText();
			break;
		}		
	}	
}

void RobloxMainWindow::closeEvent(QCloseEvent* evt)
{
    RBX::Log::current()->writeEntry(RBX::Log::Information,"RobloxMainWindow::closeEvent");

    if ( !m_IsInitialized || UpdateUIManager::Instance().isBusy() )
    {
        evt->ignore();
        return;
    }

	if (FFlag::StudioSettingsGAEnabled)
		AuthoringSettings::singleton().setDoSettingsChangedGAEvents(false);

    Studio::Intellesense::singleton().deactivate();

	// cleanup Wiki search lookup table
	ScriptTextEditor::cleanupWikiLookup();


	// first save and close out the play doc if we're in build mode
    if ( m_BuildMode == BM_BASIC )
    {
        IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();       
        if ( playDoc && !requestDocClose(*playDoc) )
        {
            evt->ignore();
			return;
        }
    }

    // save and close the rest of the docs
    if ( !RobloxDocManager::Instance().requestCloseAllDocs() )
    {
        evt->ignore();
        return;
    }
	
	//before close the application, save its states.
	saveApplicationStates();
	// remove event filter
	if (FFlag::StudioInSyncWebKitAuthentication)
		qApp->removeEventFilter(&AuthenticationHelper::Instance());

    hide();

	studioAnalytics.savePersistentVariables();
	studioAnalytics.reportPersistentVariables();

    QMainWindow::closeEvent(evt);
}

/**
 * Close all documents and save locally.
 */
void RobloxMainWindow::saveAndClose()
{
    if ( RobloxDocManager::Instance().requestCloseAndSaveAllDocs() )
        close();
}

/**
 * Closes all documents but doesn't save.
 */
void RobloxMainWindow::forceClose()
{
    RobloxDocManager::Instance().closeAllDocs();
    close();
}

void RobloxMainWindow::contextMenuEvent(QContextMenuEvent* evt)
{
	// don't show the main window context menu if in basic mode
    if ( m_BuildMode != BM_BASIC )
        QMainWindow::contextMenuEvent(evt);
}

void RobloxMainWindow::dragEnterEvent(QDragEnterEvent *evt)
{
	const QMimeData *pMimeData = evt->mimeData();
	if (!pMimeData || !pMimeData->hasUrls()) 
		return QMainWindow::dragEnterEvent(evt);

	bool isValidFileList = false;

	QList<QUrl> urlList = pMimeData->urls();
	for (int i = 0; i < urlList.size() && i < 6; ++i) 
	{
		QString filePath = urlList.at(i).toLocalFile();
		isValidFileList =
            !filePath.isEmpty() &&
                (filePath.endsWith(".rbxl", Qt::CaseInsensitive) ||
                 filePath.endsWith(".rbxlx", Qt::CaseInsensitive) ||
                 filePath.endsWith(".rbxm", Qt::CaseInsensitive) ||
                 filePath.endsWith(".rbxmx", Qt::CaseInsensitive));

		if (!isValidFileList)
			break;
	}

	if (!isValidFileList)
		return QMainWindow::dragEnterEvent(evt);
	
	evt->acceptProposedAction();
}

void RobloxMainWindow::dragMoveEvent(QDragMoveEvent *evt)
{
	evt->acceptProposedAction();
}

void RobloxMainWindow::dropEvent(QDropEvent *evt)
{
	const QMimeData *pMimeData = evt->mimeData();

	if (!pMimeData || !pMimeData->hasUrls())
		return QMainWindow::dropEvent(evt);
	
	QList<QUrl> urlList = pMimeData->urls();
	for (int i = 0; i < urlList.size() && i < 6; ++i) 
	{
		QString fileName = urlList.at(i).toLocalFile();
		if (!fileName.isEmpty())
		{
			if(fileName.endsWith(".rbxl", Qt::CaseInsensitive) || fileName.endsWith(".rbxlx", Qt::CaseInsensitive))
				handleFileOpen(fileName, IRobloxDoc::IDE);
			else 
			{
				if(RobloxDocManager::Instance().getCurrentDoc())
					RobloxDocManager::Instance().getCurrentDoc()->handleDrop(fileName);
			}
		}
	}

	evt->acceptProposedAction();
}

void RobloxMainWindow::dragLeaveEvent(QDragLeaveEvent *evt)
{
	evt->accept();
}

void RobloxMainWindow::initializeUI()
{

    // Apply the global Roblox Studio style sheet to the entire application.
    // RobloxStudio.css is where we can tweak and tune the look-and-feel
    // of the Studio UI.  All style overrides that are application-wide
    // should be put there.
    setStyleSheet(QtUtilities::getResourceFileText(":/RobloxStudio.css"));
    
    // force height so we don't get any resizing when adding and removing controls to the status bar
    statusBar()->setFixedHeight(32);
	 // Apply the global Roblox Studio style sheet to the entire application.

	QString css;

    if ( isRibbonStyle() )
	{
        //css = QtUtilities::getResourceFileText(":/RobloxStudioRibbon.css");
	}
	else
	{
        //css = QtUtilities::getResourceFileText(":/RobloxStudio.css");

		//TODO: Move all these customizations to .ui file
		menuTools->insertAction(openPluginsFolderAction, screenShotAction);
#ifdef _WIN32
		menuTools->insertAction(openPluginsFolderAction, toggleVideoRecordAction);
#endif

		menuTools->insertSeparator(openPluginsFolderAction);

		advToolsToolBar->insertAction(advanceJointCreationManualAction, toggleCollisionCheckAction);
		advToolsToolBar->insertSeparator(advanceJointCreationManualAction);

		if (RBX::MouseCommand::isAdvArrowToolEnabled())
			removeToolBar(oldToolsToolBar);

		// Added so that we can dump Crash for Breakpad as needed
		// To enable crashing add following to AppSettings.xml
		//   <CrashMenu>1</CrashMenu>
		if(RobloxSettings::showCrashMenu())
		{
			QAction *crashAction = new QAction("&Crash", this);
			connect(crashAction, SIGNAL(triggered()), this, SLOT(causeCrash()));
			QMenu* debugMenu = menuBar()->addMenu("&Debug");
			debugMenu->addAction(crashAction);
		}

		advToolsToolBar->insertAction(glueSurfaceAction, smoothNoOutlinesAction);
	}

	//setStyleSheet(css);
    Studio::Intellesense::singleton().setStyleSheet(css);

    //support drag drop
    setAcceptDrops(true);

	// fix focus issues in Mac
	setFocusPolicy(Qt::StrongFocus);
    
    //default is max screen
    setWindowState(Qt::WindowMaximized);
    
    QTimer* minute_timer = new QTimer(this);
    connect(minute_timer,SIGNAL(timeout()),this,SLOT(onMinuteTimer()));
    minute_timer->start(60 * 1000);
    
    m_AnalyticTrackedConnection = RBX::RobloxGoogleAnalytics::analyticTrackedSignal().connect(boost::bind(&RobloxMainWindow::onAnalyticTracked, this));
    
    qApp->installEventFilter(this);

	// Initialize the doc manager
	RobloxDocManager::Instance().initialize(*this);
}

void RobloxMainWindow::setupSlots()
{
	//file menu
	connect(menuFile, SIGNAL(hovered(QAction*)), this, SLOT(onMenuActionHovered(QAction*)));
	connect(fileNewAction,    SIGNAL(triggered()), this, SLOT(fileNew()));
	connect(fileOpenAction,   SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(fileCloseAction,  SIGNAL(triggered()), this, SLOT(fileClose()));
	connect(fileSaveAction,   SIGNAL(triggered()), this, SLOT(fileSave()));
	connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(fileExitAction,   SIGNAL(triggered()), this, SLOT(close()));
	connect(publishGameAction, SIGNAL(triggered()), this, SLOT(publishGame()));
	publishGameAction->setEnabled(false);

	connect(fileOpenRecentSavesAction, SIGNAL(triggered()), this, SLOT(fileOpenRecentSaves()));
    
    // create global shortcuts that should always work even if actions are disabled

    QShortcut* shortcut = new QShortcut(fileExitAction->shortcut(),this);
    connect(shortcut,SIGNAL(activated()),this,SLOT(close()));

    shortcut = new QShortcut(toggleBuildModeAction->shortcut(),this);
    connect(shortcut,SIGNAL(activated()),this,SLOT(toggleBuildMode()));
	
    addAction(zoomExtentsAction);
    addAction(toggleLocalSpaceAction);
    addAction(quickInsertAction);
	addAction(explorerFilterAction);

	pairRbxDeviceAction->setEnabled(true);
	playRbxDeviceAction->setEnabled(true);
	
	if (RBX::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl() && RBX::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl()[0] != '\0')
	{			
		connect(filePublishedProjectsAction, SIGNAL(triggered()), this, SLOT(filePublishedProjects()));
	}
	else
	{
		filePublishedProjectsAction->setEnabled(false);
		filePublishedProjectsAction->setVisible(false);
	}

	//set up recent open files slots
    for (int i = 0; i < MAX_RECENT_FILES_SHOWN; ++i) 
    {
		if (!isRibbonStyle())
		{
			recentOpenedFiles[i] = new QAction(this);
			menuFile->insertAction(fileExitAction, recentOpenedFiles[i]);
			recentOpenedFiles[i]->setVisible(false);
			connect(recentOpenedFiles[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		}
		else
		{
			recentOpenedFiles[i] = NULL;
		}
	}

	QMenu* pMenu = isRibbonStyle() ? findChild<QMenu*>("switchWindowsMenu") : menuWindow;

	//for window menu open tabs slots
    for ( int i = 0 ; i < MAX_DOC_WINDOWS ; ++i ) 
    {
		currentOpenedfiles[i] = new QAction(this);

		if (pMenu)
			pMenu->addAction(currentOpenedfiles[i]);

		currentOpenedfiles[i]->setVisible(false);
        connect(
            currentOpenedfiles[i],SIGNAL(triggered()), 
            &RobloxDocManager::Instance(),SLOT(onActivateWindow()) );
		currentOpenedfiles[i]->setCheckable(true);
	}
		
	separator = menuFile->insertSeparator(fileExitAction);
	
	//reset view
	connect(resetViewAction, SIGNAL(triggered()), this, SLOT(loadDefaultApplicationState()));
	
	//execute script
	connect(executeScriptAction, SIGNAL(triggered()), this, SLOT(executeScriptFile()));

	// Tools->Settings....
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(openSettingsDialog()));

	// Tools Menu
	instanceDumpAction->setVisible(false);

	connect(openPluginsFolderAction, SIGNAL(triggered()), this, SLOT(openPluginsFolder()));
	connect(managePluginsAction, SIGNAL(triggered()), this, SLOT(managePlugins()));	
	
	// Help Menu
	connect(aboutRobloxAction, SIGNAL(triggered()), this, SLOT(about()));
	connect(onlineHelpAction, SIGNAL(triggered()), this, SLOT(onlineHelp()));
    connect(shortcutHelpAction, SIGNAL(triggered()), this, SLOT(shortcutHelp()));
    connect(objectBrowserAction, SIGNAL(toggled(bool)), this, SLOT(openObjectBrowser(bool)));
	connect(fastLogDumpAction, SIGNAL(triggered()), this, SLOT(fastLogDump()));

	connect(viewCommandBarAction, SIGNAL(toggled(bool)), commandToolBar, SLOT(setVisible(bool)));

    //Insert Object
	if (FFlag::StudioSeparateActionByActivationMethod)
    	connect(quickInsertAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(commonSlot()));
	else
		connect(quickInsertAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(onQuickInsertFocus()));
    
	connect(explorerFilterAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(filterExplorer()));

	//Cleaup players and servers
	connect(cleanupServersAndPlayersAction, SIGNAL(triggered()), this, SLOT(cleanupPlayersAndServers()));
    
	// Insert Service
	if (!m_pInsertServiceDlg)
		m_pInsertServiceDlg = new InsertServiceDialog(this);
	connect(insertServiceAction, SIGNAL(triggered()), m_pInsertServiceDlg, SLOT(show()));

	connect(findInScriptsAction, SIGNAL(triggered()), this, SLOT(showFindAllDialog()));

	emulateDeviceAction->setEnabled(true);
	manageEmulationDeviceAction->setEnabled(true);
	
	viewCommandBarAction->setEnabled(true);
	testCustomStatsAction->setEnabled(FFlag::StudioCustomStatsEnabled);

    QAction* commonActions[] =
    {
        rotateSelectionAction, tiltSelectionAction, groupSelectionAction, ungroupSelectionAction,
		selectChildrenAction, moveUpBrickAction, moveDownBrickAction, deleteSelectedAction, 
		selectAllAction, lockAction, glueSurfaceAction,

        // stats menu
        testStatsAction, testRenderStatsAction, testPhysicsStatsAction, testNetworkStatsAction, 
        testSummaryStatsAction, testCustomStatsAction, testClearStatsAction,

		smoothSurfaceAction, weldSurfaceAction, studsAction, inletAction, universalsAction, 
		hingeAction, anchorAction, motorRightAction, smoothNoOutlinesAction,
		dropperAction, simulationRunAction, simulationPlayAction, simulationStopAction, simulationResetAction, 
        zoomInAction, zoomOutAction, tiltUpAction, tiltDownAction,
		zoomExtentsAction, panRightAction, panLeftAction, advanceJointCreationManualAction, 
		gridToOneAction,gridToOneFifthAction, gridToOffAction, 
        actionFillColor, actionMaterial, advTranslateAction, advRotateAction, resizeAction,
		cutAction, copyAction, pasteAction, duplicateSelectionAction, 
		pasteIntoAction, undoAction, redoAction, playSoloAction, startServerAction, 
		startPlayerAction, pairRbxDeviceAction, insertModelAction, insertIntoFileAction, 
		selectionSaveToFileAction, publishToRobloxAction, publishToRobloxAsAction, publishGameAction, publishSelectionToRobloxAction, 
		saveToRobloxAction, publishAsPluginAction, createNewLinkedSourceAction, advArrowToolAction, toggleAxisWidgetAction, toggle3DGridAction,
		toggleVideoRecordAction, viewDiagnosticsAction, viewTaskSchedulerAction, viewToolboxAction,
		viewBasicObjectsAction, viewScriptPerformanceAction, viewObjectExplorerAction,
		viewPropertiesAction, viewOutputWindowAction, viewContextualHelpAction, viewFindResultsWindowAction, viewScriptAnalysisAction,
        gameExplorerAction, toggleCollisionCheckAction,
		renameObjectAction, unlockAllAction, openPluginsFolderAction, screenShotAction, toggleLocalSpaceAction,
        quickInsertAction, explorerFilterAction, exportSelectionAction, exportPlaceAction,
        unionSelectionAction, negateSelectionAction, separateSelectionAction, viewTutorialsAction, viewTeamCreateAction,

        // script actions
        goToScriptErrorAction, commentSelectionAction, uncommentSelectionAction, toggleCommentAction,
        expandAllFoldsAction, collapseAllFoldsAction, findAction, replaceAction, findNextAction, goToLineAction, 
        findPreviousAction, resetScriptZoomAction, reloadScriptAction, neverBreakOnScriptErrorsAction, breakOnAllScriptErrorsAction,
		breakOnUnhandledScriptErrorsAction, manageEmulationDeviceAction, 

		gridSizeToTwoAction, gridSizeToFourAction, gridSizeToSixteenAction, launchHelpForSelectionAction, downloadPlaceCopyAction
	};
	
	viewTutorialsAction->setVisible(FFlag::StudioShowTutorialsByDefault);
	downloadPlaceCopyAction->setVisible(false);

	int numActions = sizeof(commonActions) / sizeof(QAction*);
    for ( int i = 0; i < numActions; ++i )
    {
        RBX::BaldPtr<QAction> action = commonActions[i];
		if (!FFlag::StudioSeparateActionByActivationMethod)
       		connect(action,SIGNAL(triggered(bool)),this,SLOT(commonSlot(bool)));

        QList<QKeySequence> shortcuts = action->shortcuts();
        if ( !shortcuts.isEmpty() )
        {
			// add the shortcuts to the tooltip
            QtUtilities::setActionShortcuts(*action,shortcuts);
        }
        else
        {
            // set status tip
            if ( action->statusTip().isEmpty() )
                action->setStatusTip(action->toolTip());
        }
    }
	
	connect(&Roblox::Instance(),SIGNAL(marshallAppEvent(void*,bool)), this,SLOT(processAppEvent(void*)));
	connect(actionStartPage, SIGNAL(toggled(bool)), this, SLOT(openStartPage(bool)));
	connect(actionFullScreen, SIGNAL(toggled(bool)), this, SLOT(toggleFullScreen(bool)));

	updateShortcutSet();
}

void RobloxMainWindow::updateShortcutSet()
{
	if (FFlag::DontSwallowInputForStudioShortcuts)
	{
		shortcutSet.clear();
		QObjectList objects = UpdateUIManager::Instance().getMainWindow().children();
		for (QObjectList::iterator iter = objects.begin(); iter != objects.end() ; ++iter )
		{
			RBX::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
			if (action && !action->text().isEmpty() &&!action->text().startsWith("&"))
			{
				shortcutSet.insert(action->shortcut().toString());
			}
		}
	}
}

bool RobloxMainWindow::isShortcut(const QKeySequence& keySequence)
{
	return (shortcutSet.find(keySequence.toString()) != shortcutSet.end());
}

/**
 * Configure the shortcuts for the actions that are platform specific.
 *  Common actions have different shortcuts on different platforms.  This
 *  function will reconfigure the common action shortcuts for the platform.
 */
void RobloxMainWindow::assignAccelerators()
{
	// file menu
    QtUtilities::setActionShortcuts(*fileNewAction,QKeySequence::keyBindings(QKeySequence::New));
	QtUtilities::setActionShortcuts(*fileOpenAction,QKeySequence::keyBindings(QKeySequence::Open));
	QtUtilities::setActionShortcuts(*fileSaveAction,QKeySequence::keyBindings(QKeySequence::Save));
	QtUtilities::setActionShortcuts(*fileSaveAsAction,QKeySequence::keyBindings(QKeySequence::SaveAs));
	QtUtilities::setActionShortcuts(*fileCloseAction,QKeySequence::keyBindings(QKeySequence::Close));
	QtUtilities::setActionShortcuts(*fileExitAction,QKeySequence::keyBindings(QKeySequence::Quit));

	// edit menu
	QtUtilities::setActionShortcuts(*copyAction,QKeySequence::keyBindings(QKeySequence::Copy));
	QtUtilities::setActionShortcuts(*cutAction,QKeySequence::keyBindings(QKeySequence::Cut));
	QtUtilities::setActionShortcuts(*pasteAction,QKeySequence::keyBindings(QKeySequence::Paste));
	QtUtilities::setActionShortcuts(*redoAction,QKeySequence::keyBindings(QKeySequence::Redo));
	QtUtilities::setActionShortcuts(*undoAction,QKeySequence::keyBindings(QKeySequence::Undo));
	QtUtilities::setActionShortcuts(*selectAllAction,QKeySequence::keyBindings(QKeySequence::SelectAll));

    // set up delete action
	QList<QKeySequence> shortcuts = QKeySequence::keyBindings(QKeySequence::Delete);
    shortcuts.append(QKeySequence(Qt::Key_Backspace));
    QtUtilities::setActionShortcuts(*deleteSelectedAction,shortcuts);
    
	// script menu
	QtUtilities::setActionShortcuts(*findAction,QKeySequence::keyBindings(QKeySequence::Find));
    QtUtilities::setActionShortcuts(*replaceAction,QKeySequence::keyBindings(QKeySequence::Replace));

    shortcuts.clear();
    shortcuts.append(QKeySequence("F3"));
	QtUtilities::setActionShortcuts(*findNextAction,shortcuts);
    
    shortcuts.clear();
    shortcuts.append(QKeySequence("Shift+F3"));
    QtUtilities::setActionShortcuts(*findPreviousAction,shortcuts);
	
    // tools menu
    QtUtilities::setActionShortcuts(*settingsAction,QKeySequence::keyBindings(QKeySequence::Preferences));

	// help menu
    shortcuts.clear();
    shortcuts.append(QKeySequence("F1"));
	QtUtilities::setActionShortcuts(*onlineHelpAction, shortcuts);

    // camera toolbar
    QtUtilities::setActionShortcuts(*zoomInAction, QList<QKeySequence>() << QKeySequence(QKeySequence::ZoomIn) << QKeySequence("Ctrl+="));
    QtUtilities::setActionShortcuts(*zoomOutAction, QList<QKeySequence>() << QKeySequence(QKeySequence::ZoomOut) << QKeySequence("Ctrl+_"));

    shortcuts.clear();
#ifndef Q_WS_MAC
	// TODO: remove shortcut collision for debugger action - stepinto
	QtUtilities::setActionShortcuts(*stepIntoAction,shortcuts);
    // F11 works differently on Mac
    shortcuts.append(QKeySequence("F11"));
#endif
    QtUtilities::setActionShortcuts(*actionFullScreen,shortcuts);

	if (isRibbonStyle())
	{
		// remove shortcut set for startServerAction
		shortcuts.clear();
		QtUtilities::setActionShortcuts(*startServerAction,shortcuts);

		// add shortcut F7 for startServerAndPlayersAction
		QAction* pAction = findChild<QAction*>("startServerAndPlayersAction");
		if (pAction)
		{
			shortcuts.append(QKeySequence("F7"));
			QtUtilities::setActionShortcuts(*pAction,shortcuts);
		}
	}
}

void RobloxMainWindow::showInsertServiceDialog()
{
	if (!m_pInsertServiceDlg)
		m_pInsertServiceDlg = new InsertServiceDialog(this);
	
	if(RobloxDocManager::Instance().getCurrentDoc())
		RobloxDocManager::Instance().getCurrentDoc()->handleAction("actionUpdateInsertServiceDialog");
	
	m_pInsertServiceDlg->setVisible(true);
}

void RobloxMainWindow::showFindAllDialog()
{
	FindReplaceProvider::instance().getFindAllDialog()->show();
	FindReplaceProvider::instance().getFindAllDialog()->activateWindow();
	FindReplaceProvider::instance().getFindAllDialog()->setFocus();
}

void RobloxMainWindow::updateEmbeddedFindPosition()
{
	if (IRobloxDoc* doc = RobloxDocManager::Instance().getCurrentDoc())
	{
		if (RobloxScriptDoc* scriptDoc = dynamic_cast<RobloxScriptDoc*>(doc))
		{
			if (ScriptTextEditor* textEditor = scriptDoc->getTextEditor())
			{
				textEditor->updateEmbeddedFindPosition();
			}
		}
	}
}

void RobloxMainWindow::setupCommandToolBar()
{
    m_pScriptComboBox = new ScriptComboBox(commandToolBar);
    commandToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    commandToolBar->addWidget(m_pScriptComboBox);

    connect(
        commandToolBar,SIGNAL(topLevelChanged(bool)),
        this,SLOT(onCommandToolBarTopLevelChanged(bool)) );
}

void RobloxMainWindow::sendCounterEvent(std::string counterName, bool fireImmediately)
{
	if (!RBX::ClientAppSettings::singleton().GetValueCaptureQTStudioCountersEnabled())
		return;

#ifdef _WIN32
	// Can't initialize in global init so must do lazy init
	static boost::scoped_ptr<CountersClient> m_pCountersClient;

	if (!m_pCountersClient)
		m_pCountersClient.reset(new CountersClient(GetBaseURL().c_str(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", NULL));

	// TODO - change project settings to not have wchar as a native type.  Qt can't do wide string operations.
	std::wstring wstringCounterName = convert_s2w(counterName);
	m_pCountersClient->registerEvent(wstringCounterName, fireImmediately);
#endif
}

void RobloxMainWindow::setupCounterSender()
{
#ifdef _WIN32
	int interval = RBX::ClientAppSettings::singleton().GetValueCaptureSlowCountersIntervalInSeconds();
	m_pCounterSenderTimer = new QTimer(this);
	connect(m_pCounterSenderTimer, SIGNAL(timeout()), this, SLOT(sendOffCounters()));
	m_pCounterSenderTimer->start(interval * 1000);
#endif
}

void RobloxMainWindow::sendOffCounters()
{
#ifdef _WIN32
	boost::scoped_ptr<CountersClient> countersClient;

	if (!countersClient)
		countersClient.reset(new CountersClient(GetBaseURL().c_str(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", NULL));

	countersClient->reportEvents();
#endif
}

void RobloxMainWindow::logIntervalCounter()
{
	int interval = RBX::ClientAppSettings::singleton().GetValueCaptureCountersIntervalInMinutes();

	sendCounterEvent(QString("QTStudio_%1MinCounter").arg(interval).toStdString(), false);
}

void RobloxMainWindow::handleFileOpenSendCounter(const QString& fileName)
{
	// This person is a builder, set up a timer that starts recording every minute they're using our app
	if (!m_pMinutesPlayedTimer && RBX::ClientAppSettings::singleton().GetValueCaptureQTStudioCountersEnabled())
	{
		// Record counters every XX minutes (based on config)
		int interval = RBX::ClientAppSettings::singleton().GetValueCaptureCountersIntervalInMinutes();
		m_pMinutesPlayedTimer = new QTimer(this);
		connect(m_pMinutesPlayedTimer, SIGNAL(timeout()), this, SLOT(logIntervalCounter()));
		m_pMinutesPlayedTimer->start(interval * 60 * 1000);
	} 
	sendCounterEvent(fileName.isEmpty() ? "QTStudio_FileNew" : "QTStudio_FileOpen", false);
}
	
void RobloxMainWindow::checkInternetConnectionSendCounter(RBX::Http& robloxRequest, RBX::Http& externalRequest)
{
	bool robloxAccessible;
	std::string result;
	RobloxSettings settings;

	int unconnectedRuns = settings.value("appNoInternet", 0).toInt();

	try
	{
		robloxRequest.get(result);
		robloxAccessible = true;
	}
	catch(...)
	{
		robloxAccessible = false;
	}

	if (robloxAccessible)
	{
		if (unconnectedRuns)
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioInternetReconnection", "none", unconnectedRuns);
		
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioInternetConnection");

		settings.setValue("appNoInternet", 0);
	}
	else
	{
		bool googleAccessible;
		try
		{
			externalRequest.get(result, true);
			googleAccessible = true;
		}
		catch(...)
		{
			googleAccessible = false;
		}

		if (googleAccessible)
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "RobloxDomainBlocked");
		else
			settings.setValue("appNoInternet", unconnectedRuns + 1);
	}
	
}

bool RobloxMainWindow::handleFileOpen(const QString &fileName, IRobloxDoc::RBXDocType type, const QString &script)
{
    RBX::Log::current()->writeEntry(RBX::Log::Information, RBX::format("RobloxMainWindow::handleFileOpen %s", script.toStdString().c_str()).c_str());

	QString fileToOpen = fileName;

	if (type == IRobloxDoc::IDE)
	{
		// we aren't specifying a file to open or a script to open, we aren't opening anything
		if(fileName.isEmpty() && script.isEmpty())
			return false;

        if (fileName == NEW_PLACE_FILENAME)
        {
            fileToOpen = "";
        }

		// Gross, if it's a build button click, add the avatar (and guis)
		StudioUtilities::setAvatarMode(false);
       
		if (StudioUtilities::containsVisitScript(script) ||
			StudioUtilities::containsJoinScript(script))
			StudioUtilities::setAvatarMode(true);

		handleFileOpenSendCounter(fileToOpen);
	
		// We already have an IDE doc open, launch a new version of Studio (SDI)
		IRobloxDoc* currentPlayDoc = RobloxDocManager::Instance().getPlayDoc();
		if (currentPlayDoc)
		{
			// If we're opening a branch new place in our new process, send over the new place file name so it can differentiate
			QString fileNameToOpen = fileName.isEmpty() ? NEW_PLACE_FILENAME : fileName;
			RobloxApplicationManager::instance().createNewStudioInstance(script, fileNameToOpen);
			return true;
		}
	}

	IRobloxDoc* newDoc = RobloxDocManager::Instance().getOrCreateDoc(type);

    if ( type == IRobloxDoc::IDE )
    {
        // static cast ok because we know we made a IDEDoc due to type
        // only ideDoc needs a script, set it here before we open file
        RobloxIDEDoc* ideDoc = static_cast<RobloxIDEDoc*>(newDoc);
        ideDoc->setInitializationScript(script);

        m_AutoSaveAccum = 0;
    }
        
    if (!newDoc->open(this, fileToOpen))
    {
        QString text = tr("Error in opening file - %1").arg(fileToOpen);
        if (RBX::StandardOut::singleton() && (!fileToOpen.isEmpty()) && type == IRobloxDoc::IDE)
            RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR,qPrintable(text));

        requestDocClose(*newDoc);
        
        if ( type == IRobloxDoc::IDE )
            QMessageBox::critical(this,"Open File Failure",text);
        return false;
    }
    else
    {
        if (RBX::StandardOut::singleton() && (!fileToOpen.isEmpty()) && type == IRobloxDoc::IDE)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Successfully opened file - %s", qPrintable(fileToOpen));
    }

    if ( fileToOpen.isEmpty() && type == IRobloxDoc::IDE && script.isEmpty())
    {
		RobloxIDEDoc* idedoc = static_cast<RobloxIDEDoc*>(newDoc);
		idedoc->initializeNewPlace();
    }


	QWidget *widgetToAddInTab = newDoc->getViewer();
	if (widgetToAddInTab)
	{
        RobloxDocManager::Instance().configureDocWidget(*newDoc);

        // update build mode with new doc information
        //saveApplicationStates(); // save state in case update reloads it
        UpdateUIManager::Instance().updateBuildMode();
	}

    if ( type == IRobloxDoc::IDE && !fileToOpen.isEmpty() )
        updateRecentFile(fileToOpen);

    if (FFlag::ReportBuildVSEditMode)
    {
        RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "EditMode", script.contains("visit.ashx") ? "Build" : "Edit");
    }

	return true;
}

//for making the position of toolbar identical to old studio.
void RobloxMainWindow::setToolbarPosition()
{
	insertToolBar(runToolBar, advToolsToolBar);
	
	if (RBX::MouseCommand::isAdvArrowToolEnabled())
	{
		insertToolBar(advToolsToolBar, runToolBar);
	}
    else
	{
		insertToolBar(advToolsToolBar, oldToolsToolBar);
		insertToolBar(oldToolsToolBar, runToolBar);
	}

	insertToolBar(runToolBar, editCameraToolBar);
	insertToolBar(editCameraToolBar, viewToolsToolBar);
}

QAction* RobloxMainWindow::getActionByName(const QString& actionName)
{
	QList<QAction*> actionList = actions();
	for (QList<QAction*>::const_iterator iter = actionList.begin(); iter != actionList.end(); ++iter)
		if ((*iter)->objectName() == actionName)
			return *iter;

	return NULL;
}

void RobloxMainWindow::saveDefaultApplicationState()
{
	RobloxSettings settings;

	//save all the positions and sizes for all dockWidgets and toolbars.
	settings.setValue("default_window_state", saveState());
	settings.setValue("default_geometry_state", saveGeometry());
}

void RobloxMainWindow::saveApplicationStates()
{
    // don't save anything in basic mode
    if ( m_BuildMode == BM_BASIC )
        return;

    if ( !isVisible() )
        return;

	UpdateUIManager::Instance().saveDocksGeometry();

	RobloxSettings settings;
	// Clear legacy values
	settings.remove("geometry");
	settings.remove("windowState");
	
	//save all the positions and sizes for all dockWidgets and toolbars.	
	settings.setValue(sGeometryKey, saveGeometry());
	settings.setValue(sWindowStateKey, saveState());
	
	// Save command history from output window
	settings.setValue("rbxCommandHistory", m_pScriptComboBox->commandHistory());

	//TODO: Move this to RobloxRibbonMainWindow.cpp
	if (isRibbonStyle())
		RobloxQuickAccessConfig::singleton().saveQuickAccessConfig();
}

void RobloxMainWindow::loadDefaultApplicationState()
{
	RobloxSettings settings;
	restoreState(settings.value("default_window_state").toByteArray());
	restoreGeometry(settings.value("default_geometry_state").toByteArray());

	// since players dockwidget and chat dockwidget gets created only after saving application default state so it doesn't get restored
	QDockWidget* playersDockWidget = findChild<QDockWidget*>("playersDockWidget");
	if (playersDockWidget)
		playersDockWidget->setVisible(false);
	QDockWidget* chatDockWidget = findChild<QDockWidget*>("chatDockWidget");
	if (chatDockWidget)
		chatDockWidget->setVisible(false);

	UpdateUIManager::Instance().setDockVisibility(eDW_OBJECT_EXPLORER, true);

	saveApplicationStates();
}

void RobloxMainWindow::loadApplicationStates()
{
	if (isRibbonStyle()) // Ribbon mode saves to its own settings
	{
		sGeometryKey = sGeometryKey.append("_ribbon");
		sWindowStateKey = sWindowStateKey.append("_ribbon");
	}

	//load all the positions and sizes for all dockWidgets and toolbars from memory.
	RobloxSettings settings;
	restoreState(settings.value(sWindowStateKey).toByteArray());
	restoreGeometry(settings.value(sGeometryKey).toByteArray());

    m_pScriptComboBox->setCommandHistory(settings.value("rbxCommandHistory").toStringList());
	
	UpdateUIManager::Instance().loadDocksGeometry();

    setCurrentDirectory(settings.value("rbxl_last_directory").toString());
}

/**
 * Update the window title to show the current doc name.
 *  If a doc is not selected, show the default.
 *  
 * @see getDialogTitle
 */
void RobloxMainWindow::updateWindowTitle()
{
    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();

	// if no document or we're in basic build mode just use the simple title
    if ( !pDoc || m_BuildMode == BM_BASIC )
        setWindowTitle(sWindow_Title);
    else
    {
		setWindowTitle(pDoc->windowTitle() + " - " + QString(sWindow_Title));
    }
}

/**
 * Get the title string to be used for child dialogs.
 *  This is different than the main window's title because the main one can
 *  have the current file opened and [*] in the name.
 * 
 * @see updateWindowTitle()
 * @return default child dialog title
 */
QString RobloxMainWindow::getDialogTitle() const
{
    return sDialog_Title;
}

/**
 * Callback every minute.
 */
void RobloxMainWindow::onMinuteTimer()
{
	// prevent re-entrancy
    static bool processing = false;
    if ( processing )
        return;
    processing = true;

    // periodically save state in case we crash and lose all the layout changes
    saveApplicationStates();

	// auto-save
    if ( AuthoringSettings::singleton().autoSaveEnabled )
    {
        ++m_AutoSaveAccum;

        // correct for bad values
        const int autoSaveMinutes = qMax(AuthoringSettings::singleton().autoSaveMinutesInterval,1);
        AuthoringSettings::singleton().autoSaveMinutesInterval = autoSaveMinutes;

        if ( m_AutoSaveAccum >= autoSaveMinutes )
        {
            m_AutoSaveAccum = 0;
            RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
            if ( playDoc && !playDoc->autoSave(false) )
            {
                const int result = QMessageBox::question(
                    this,
                    tr("Failed to AutoSave"),
                    tr("Failed to AutoSave.  Do you want to temporarily disable AutoSave?"),
                    QMessageBox::Yes | QMessageBox::No );
                if ( result == QMessageBox::Yes )
                    AuthoringSettings::singleton().autoSaveEnabled = false;
            }
        }
    }

    trackUserActive();

    processing = false;
}

/**
 * Deletes the splash screen.
 *  After deleting, shows the tip of the day dialog.
 */
void RobloxMainWindow::onDeleteSplashScreen()
{
    if ( m_splashScreen )
    {
        delete m_splashScreen;
        m_splashScreen = NULL;
    }
}

void RobloxMainWindow::toggleBuildMode()
{
    if ( m_BuildMode == BM_ADVANCED )
        setBuildMode(BM_BASIC);
    else if ( m_BuildMode == BM_BASIC )
        setBuildMode(BM_ADVANCED);

	UpdateUIManager::Instance().updateBuildMode();

    // TODO - handle other build modes
}

/**
 * Sets the current mode for building.
 *  Basic mode disables all dock widgets and toolbars.
 *  Advanced mode shows everything. 
 */
void RobloxMainWindow::setBuildMode(eBuildMode buildMode)
{
    //saveApplicationStates();

    QSettings settings;

    // if using the default, set it from the settings
    if ( buildMode == BM_LASTMODE )
        buildMode = (eBuildMode)settings.value("BuildMode",BM_ADVANCED).toInt();
    
    m_BuildMode = buildMode;
    settings.setValue("BuildMode",m_BuildMode);
}

void RobloxMainWindow::closePlayDoc()
{
	IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();       
	if (playDoc)
		requestDocClose(playDoc);
}

/**
 * Callback for the command toolbar changing floating/docking state.
 *  Handles setting the maximum size of the script input combobox so it works as expected.
 */
void RobloxMainWindow::onCommandToolBarTopLevelChanged(bool topLevel)
{
    if ( topLevel )
        m_pScriptComboBox->setFixedWidth(800);
    else
    {
        m_pScriptComboBox->setMinimumWidth(300);
        m_pScriptComboBox->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    commandToolBar->layout()->invalidate();
}

void RobloxMainWindow::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
    if (AuthoringSettings::singleton().getOutputLayoutMode() == AuthoringSettings::OutputLayoutHorizontal)
    {
        setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    }
    else
    {
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    }

    if (AuthoringSettings::singleton().diagnosticsBarEnabled) 
    {
        UpdateUIManager::Instance().updateStatusBar();
    }
}

void RobloxMainWindow::setupActionCounters()
{
    if (!FFlag::GoogleAnalyticsTrackingEnabled)
        return;

    QString ignoreActions("renameObjectAction");

	QList<QAction*> actionList = findChildren<QAction*>();

	if (FFlag::StudioSeparateActionByActivationMethod)
	{
		//We should eventually subclass QAction for this
		static QString noSlotActions("startServerCB fileNewAction fileOpenAction fileCloseAction "
			"fileSaveAction fileSaveAsAction fileExitAction"); 

		for(QList<QAction*>::const_iterator iter = actionList.begin(); iter != actionList.end(); iter++)
		{
			if (noSlotActions.contains((*iter)->objectName()))
				continue;

			QAction* currentAction = *iter;
			connect(currentAction, SIGNAL(triggered(bool)), this, SLOT(commonSlot(bool)));
			currentAction->installEventFilter(this);
		}
	}
	else
	{
		QSignalMapper* signalMapper = new QSignalMapper(this);
		for(QList<QAction*>::const_iterator iter = actionList.begin(); iter != actionList.end(); iter++)
		{
			QAction* currentAction = *iter;
			if(!currentAction->objectName().isEmpty() && !ignoreActions.contains(currentAction->objectName()))
			{
				connect(currentAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
				signalMapper->setMapping(currentAction, currentAction->objectName());
			}
		}
		connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(sendActionCounterEvent(QString)));
	}
}

void RobloxMainWindow::trackToolboxInserts(RBX::ContentId contentId, const RBX::Instances& instances)
{
	std::vector<weak_ptr<RBX::Instance> > weakInstances;

	for (RBX::Instances::const_iterator iter = instances.begin(); iter != instances.end(); ++iter)
		weakInstances.push_back(weak_from(iter->get()));

	m_insertObjectItems.push(InsertObjectItem(contentId.getAssetId(), weakInstances));

	QTimer::singleShot(FInt::StudioInsertDeletionCheckTimeMS, this, SLOT(checkInsertedObjects()));
}

void RobloxMainWindow::checkInsertedObjects()
{
	if (m_insertObjectItems.size() == 0)
		return;

	InsertObjectItem insertedInstance = m_insertObjectItems.front();
	m_insertObjectItems.pop();

	bool objectsStillExist = true;

	for (std::vector<weak_ptr<RBX::Instance> >::const_iterator iter = insertedInstance.weakInstances.begin(); iter != insertedInstance.weakInstances.end(); ++iter)
	{
		if (shared_ptr<RBX::Instance> sharedInstance = iter->lock())
		{
			if (!RBX::DataModel::get(sharedInstance.get()))
			{
				objectsStillExist = false;
				break;
			}
		}
		else
		{
			objectsStillExist = false;
			break;
		}
	}

	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, objectsStillExist ? "StudioInsertRemains" : "StudioInsertDeleted", insertedInstance.contentId.c_str());
}

void RobloxMainWindow::sendActionCounterEvent(const QString& action)
{
    if (FFlag::GoogleAnalyticsTrackingEnabled)
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "StudioUserAction", action.toStdString().c_str());
}

void RobloxMainWindow::notifyCloudEditConnectionClosed()
{
	if (m_cloudEditAwaitingShutdown)
	{
		m_cloudEditAwaitingShutdown->Set();
		return;
	}

	RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc();
	if (ideDoc && !m_IgnoreCloudEditDisconnect)
	{
		QString script = ideDoc->initializationScript();
		QtUtilities::RBXMessageBox notificationDialog;
		notificationDialog.setIcon(QMessageBox::Warning);
		if (FFlag::TeamCreateEnableDownloadLocalCopy)
		{
			notificationDialog.setText("You have lost connection to the server. "
				"Do you want to reconnect or save a copy of the place to a new file?");
		}
		else
		{
			notificationDialog.setText("You have lost connection to the server. "
				"Do you want to reconnect or work locally (you have to save local work to a new file)?");
		}
		notificationDialog.addButton("Reconnect", QMessageBox::ResetRole);
		if (FFlag::TeamCreateEnableDownloadLocalCopy)
			notificationDialog.addButton("Save Local Copy", QMessageBox::ActionRole);
		else
			notificationDialog.addButton("Work Offline", QMessageBox::DestructiveRole);
		notificationDialog.exec();
		QMessageBox::ButtonRole role = notificationDialog.buttonRole(notificationDialog.clickedButton());
		if (role == QMessageBox::ResetRole)
		{
			// close the play doc if it is still open
			if (IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
			{
				requestDocClose(*playDoc, false /*closeiflastdoc*/);
			}
			handleFileOpen(fileLocationArg, IRobloxDoc::IDE, script);
		}
		else if (FFlag::TeamCreateEnableDownloadLocalCopy && (role == QMessageBox::ActionRole))
		{
			if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
			{
				// let users save a local copy
				QString fileName = playDoc->downloadPlaceCopy();
				// close the online document
				requestDocClose(*playDoc, false /*closeiflastdoc*/);
				if (!fileName.isEmpty())
					handleFileOpen(fileName, IRobloxDoc::IDE);
			}
		}
	}
}

void RobloxMainWindow::cloudEditStatusChanged(int placeId, int universeId)
{
	RBX::ScopedAssign<bool> ignoreDocActivate(m_IgnoreCloudEditDisconnect, true);

	if (IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
	{
		requestDocClose(*playDoc, false /*closeiflastdoc*/);
	}

	QString initScript;
	if (FFlag::UseBuildGenericGameUrl)
	{
		QString pathStr = QString("Game/edit.ashx?PlaceID=%1&upload=%1&testmode=false&universeId=%2").arg(placeId).arg(universeId);
		initScript = QString::fromStdString(BuildGenericGameUrl(RobloxSettings::getBaseURL().toStdString(), pathStr.toStdString()));
	}
	else
	{
		initScript = QString("%1/Game/edit.ashx?PlaceID=%2&upload=%2&testmode=false&universeId=%3").arg(RobloxSettings::getBaseURL()).arg(placeId).arg(universeId);
	}

	handleFileOpen(FFlag::StudioDoublingOnUploadFixEnabled ? "" : fileLocationArg,
		IRobloxDoc::IDE, initScript);
}

void RobloxMainWindow::shutDownCloudEditServer()
{
	using namespace RBX;
	using namespace Network;

	RBXASSERT(QThread::currentThread() == qApp->thread());

	RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
	RBXASSERT(playDoc);
	if (!playDoc) return;

	RBXASSERT(playDoc->isCloudEditSession());
	if (!playDoc->isCloudEditSession()) return;

	shared_ptr<DataModel> dm = playDoc->getEditDataModel();
	RBXASSERT(dm);
	if (!dm) return;

	long placeId, universeId;
	{
		{
			DataModel::LegacyLock lock(dm.get(), DataModelJob::Write);

			placeId = dm->getPlaceID();
			universeId = dm->getUniverseId();

			Players* players = dm->find<Players>();
			RBXASSERT(players);
			if (players)
			{
				RBXASSERT(!m_cloudEditAwaitingShutdown);
				m_cloudEditAwaitingShutdown.reset(new RBX::CEvent(true /* manual reset */));
				Players::event_requestCloudEditShutdown.replicateEvent(players);
			}
		}
		
		if (m_cloudEditAwaitingShutdown)
		{
			UpdateUIManager::Instance().waitForLongProcess("Waiting for Shutdown", boost::bind(&RBX::CEvent::Wait, m_cloudEditAwaitingShutdown.get()));
			m_cloudEditAwaitingShutdown.reset();
		}
	}

	QMetaObject::invokeMethod(this, "cloudEditStatusChanged", Qt::QueuedConnection, Q_ARG(int, placeId), Q_ARG(int, universeId));
}

void RobloxMainWindow::setupCustomToolButton()
{	
	m_pFillColorToolButton = new FillColorPickerToolButton(this);
	m_pFillColorToolButton->setObjectName("actionFillColor");
	m_pFillColorToolButton->setDefaultAction(actionFillColor);
	connect(m_pFillColorToolButton, SIGNAL(changed(const QString &)), this, SLOT(onCustomToolButton(const QString &)));

	m_pMaterialToolButton = new MaterialPickerToolButton(this);
	m_pMaterialToolButton->setObjectName("actionMaterial");
	m_pMaterialToolButton->setDefaultAction(actionMaterial);
	connect(m_pMaterialToolButton, SIGNAL(changed(const QString &)), this, SLOT(onCustomToolButton(const QString &)));

	//add tool buttons into toolbar
	QAction *fillAction = advToolsToolBar->insertWidget(dropperAction, m_pFillColorToolButton);
	fillAction->setObjectName("actionFillColor_WidgetAction");

	QAction *materialAction = advToolsToolBar->insertWidget(smoothSurfaceAction, m_pMaterialToolButton);
	materialAction->setObjectName("actionMaterial_WidgetAction");

	advToolsToolBar->insertSeparator(materialAction);
	advToolsToolBar->insertSeparator(smoothSurfaceAction);
}

void RobloxMainWindow::trackUserActive()
{
	// We want an accurate session length so we send up a special event
	// if there is any user interaction with app that is not tracked
	// by other metrics.
    
	const int eventIdleTime = 60 * 1000; // in milliseconds
	const int stillAliveTime = 20 * 1000; // in milliseconds
    
	// If it has been some time (eventIdleTime) since the last event tracked.
	if (m_lastAnalyticTrackedTime.elapsed() > eventIdleTime
        
        // And the user is not completely idle and some system is posting
        // a keep alive.
	    && m_lastAnalyticKeepAliveTime.elapsed() < stillAliveTime)
	{
        // Send an empty timing event.
        RBX::RobloxGoogleAnalytics::trackUserTiming("", "", 0);
	}
}

void RobloxMainWindow::onAnalyticTracked()
{
	m_lastAnalyticTrackedTime = QTime::currentTime();
}


void RobloxMainWindow::keepAliveAnalyticsSession()
{
	m_lastAnalyticKeepAliveTime = QTime::currentTime();
}

void RobloxMainWindow::cleanupPlayersAndServers()
{
	RobloxApplicationManager::instance().cleanupChildProcesses();
	cleanupServersAndPlayersAction->setEnabled(false);
}

void RobloxMainWindow::onIDEDocViewInitialized()
{
    if ( m_splashScreen )
        onDeleteSplashScreen();
}

void RobloxMainWindow::cookieConstraintCheckerLoadFinished(bool ok)
{
	RBXASSERT(m_cookieConstraintChecker);
	if (!m_cookieConstraintChecker)
		return;

	if (ok && m_cookieConstraintChecker->url().toString().contains(
		FString::StudioCookieConstraintUrlFragment.c_str(), Qt::CaseInsensitive))
	{
		m_cookieConstraintChecker->raise();
		m_cookieConstraintChecker->show();
	}
	else
	{
		m_cookieConstraintChecker->hide();
		disconnect(
			m_cookieConstraintChecker, SIGNAL(                     loadFinished(bool)),
			this,                      SLOT(cookieConstraintCheckerLoadFinished(bool)));
		m_cookieConstraintChecker->deleteLater();
		m_cookieConstraintChecker = NULL;
		m_cookieConstraintCheckDone->exit();
	}
}


