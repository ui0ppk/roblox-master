#include "stdafx.h"

#include "Application.h"
#include "AuthenticationMarshallar.h"
#include "Crypt.h"
#include "Document.h"
#include "DumpErrorUploader.h"
#include "RobloxServicesTools.h"
#include "FunctionMarshaller.h"
#include "gui/ProfanityFilter.h"
#include "util/FileSystem.h"
#include "util/Http.h"
#include "Util/MachineIdUploader.h"
#include "util/MD5Hasher.h"
#include "util/SoundService.h"
#include "util/Statistics.h"
#include "util/RobloxGoogleAnalytics.h"
#include "util/SafeToLower.h"
#include "rbx/ProcessPerfCounter.h"
#include "rbx/Tasks/Coordinator.h"
#include "resource.h"
#include "RenderSettingsItem.h"
#include "Script/CoreScript.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/HackDefines.h"
#include "V8Xml/XmlSerializer.h"
#include "format_string.h"
#include "VistaTools.h"
#include "MachineConfiguration.h"
#include "reflection/reflection.h"
#include "reflectionmetadata.h"
#include "v8datamodel/FastLogSettings.h"
#include "Network/API.h"
#include "StringConv.h"
#include "v8datamodel/DataModel.h"
#include "VersionInfo.h"
#include "util/CheatEngine.h"
#include "util/RegistryUtil.h"
#include "v8xml/WebParser.h"
#include "util/ProgramMemoryChecker.h"
#include "script/LuaVM.h"
#include "functionHooks.h"
#include "robloxHooks.h"
#include "GameVerbs.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/TeleportService.h"
#include "VersionInfo.h"

#include "View.h"
#include "RbxWebView.h"
#include "ReleasePatcher.h"

#include "boost/filesystem.hpp"

#include "rbx/Profiler.h"

LOGGROUP(RobloxWndInit)
LOGGROUP(Network)
FASTFLAGVARIABLE(ReloadSettingsOnTeleport, false)
FASTFLAG(GoogleAnalyticsTrackingEnabled)
DYNAMIC_LOGGROUP(GoogleAnalyticsTracking)
FASTFLAGVARIABLE(DebugUseDefaultGlobalSettings, false)
FASTINTVARIABLE(ValidateLauncherPercent, 0)
FASTINTVARIABLE(BootstrapperVersionNumber, 51261)
FASTINTVARIABLE(RequestPlaceInfoTimeoutMS, 2000)
FASTINTVARIABLE(RequestPlaceInfoRetryCount, 5)
DYNAMIC_FASTINT(JoinInfluxHundredthsPercentage)
FASTINTVARIABLE(InferredCrashReportingHundredthsPercentage, 1000)
FASTFLAGVARIABLE(RwxFailReport, false)
DYNAMIC_FASTFLAGVARIABLE(DontOpenWikiOnClient, false)
DYNAMIC_FASTFLAGVARIABLE(WindowsInferredCrashReporting, false)
FASTFLAG(PlaceLauncherUsePOST)


namespace RBX {

Application::Application()
	: logManager("Roblox", ".Client.dmp", ".Client.crashevent")
	, crashReportEnabled(true)
	, hideChat(false)
	, mapFileForWnd(NULL)
	, bufForWnd(NULL)
	, stopLogsCleanup(0)
	, clenupFinishedEvent(FALSE, FALSE)
	, processLocal_stopPreventMultipleJobsThread(NULL)
	, marshaller(NULL)
	, spoofMD5(false)
	, processLocal_stopWaitForVideoPrerollThread(NULL)
	, launchMode(SharedLauncher::Play)
{
	enteredShutdown = 0;
	logsCleanUpThread.reset(new boost::thread(boost::bind(&Application::logsCleanUpHelper, this)));
    mainWindow = 0;
}

Application::~Application()
{
	InterlockedIncrement(&stopLogsCleanup);
	WaitForSingleObject(clenupFinishedEvent, INFINITE);

	if (bufForWnd) 
	{
		UnmapViewOfFile(bufForWnd);
	}

	if (mapFileForWnd != NULL)
	{
		CloseHandle(mapFileForWnd);
	}
}

void Application::logsCleanUpHelper()
{
	std::vector<std::string> folders;
	folders.push_back(simple_logger<char>::get_tmp_path());

	VistaAPIs apis;
	if (apis.isVistaOrBetter())
	{
		LPWSTR lppwstrPath = NULL;
		if (apis.SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, KF_FLAG_CREATE, NULL, &lppwstrPath) != E_NOTIMPL)
		{
			std::wstring lowPath = lppwstrPath;
			lowPath += L"\\RbxLogs\\";
			folders.push_back(convert_w2s(lowPath));
			CoTaskMemFree((LPVOID)lppwstrPath);
		}
	}
	else
	{
		TCHAR szPath[_MAX_DIR]; 
		if (GetTempPath(_MAX_DIR, szPath))
		{
			folders.push_back(std::string(szPath));
		}
	}

	FILETIME curTimeFile = {0};
	SYSTEMTIME curTimeSystem = {0};
	GetSystemTime(&curTimeSystem);
	SystemTimeToFileTime(&curTimeSystem, &curTimeFile);
	ULARGE_INTEGER curTime = {0};
	curTime.LowPart = curTimeFile.dwLowDateTime;
	curTime.HighPart = curTimeFile.dwHighDateTime;

	unsigned __int64 MaxLogAge = 3*24*60*60*1000*unsigned __int64(10000);
	for (size_t i = 0;i < folders.size();i ++) {
		if (stopLogsCleanup == 1)
		{
			break;
		}

		WIN32_FIND_DATA findFileData;  
		HANDLE handle = INVALID_HANDLE_VALUE;
		handle = FindFirstFile((folders[i] + "RBX*.log").c_str(), &findFileData);
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (stopLogsCleanup == 1)
				{
					break;
				}

				if(findFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
				{
					ULARGE_INTEGER fileTime = {0};
					fileTime.LowPart = findFileData.ftLastWriteTime.dwLowDateTime;
					fileTime.HighPart = findFileData.ftLastWriteTime.dwHighDateTime;

					if ((curTime.QuadPart - fileTime.QuadPart) > MaxLogAge) 
					{
						std::string file = folders[i] + findFileData.cFileName;
						DeleteFile(file.c_str());
					}
				}
				Sleep(10);
			} while (FindNextFile(handle, &findFileData));
		}
	}

	clenupFinishedEvent.Set();
}

static HttpFuture fetchJoinScriptAsync(const std::string& url)
{
	if (ContentProvider::isUrl(url) && RBX::Network::isTrustedContent(url.c_str()))
	{
		return HttpAsync::getWithRetries(url, 5);
	}
	else
	{
		// silent error is harder to hack
		return boost::make_shared_future(std::string());
	}
}

static std::string readStringValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name) 
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<std::string>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected string result for %s", name.c_str()));
    }
}

static int readIntValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name)
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<int>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected int result for %s", name.c_str()));
    }
}

Application::RequestPlaceInfoResult Application::requestPlaceInfo(const std::string url, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl) const
{
	try
	{
		std::string response;
        if (FFlag::PlaceLauncherUsePOST)
        {
            std::istringstream input("");
            RBX::Http(url).post(input, RBX::Http::kContentTypeDefaultUnspecified, false, response);
        }
        else
        {
            RBX::Http(url).get(response);
        }

		std::stringstream jsonStream;
		jsonStream << response;
		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
		bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
		if (parseResult)
		{
			int status = readIntValue(jsonResult, "status");
			if (status == 2)
			{
				authenticationUrl = readStringValue(jsonResult, "authenticationUrl");
				ticket = readStringValue(jsonResult, "authenticationTicket");
				scriptUrl = readStringValue(jsonResult, "joinScriptUrl");
				return SUCCESS;
			}
			else if (status == 6)
				return GAME_FULL;
			else if (status == 10) 
				return USER_LEFT;
			else
			{
				// 0 or 1 is not an error - it is a sign that we should wait
				if (status == 0 || status == 1)
					return RETRY;
			}
		}
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Exception when requesting place info: %s. ", e.what());
	}

	return FAILED;
}

bool Application::requestPlaceInfo(int placeId, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl) const
{
	std::string url = RBX::format("%sGame/PlaceLauncher.ashx?request=RequestGame&placeId=%d&isPartyLeader=false&gender=&isTeleport=true",
		GetBaseURL().c_str(), placeId);

	int retries = FInt::RequestPlaceInfoRetryCount;
	while (retries >= 0)
	{
		if (requestPlaceInfo(url, authenticationUrl, ticket, scriptUrl) == SUCCESS)
			return true;
		else
		{
			retries--;
			Sleep(FInt::RequestPlaceInfoTimeoutMS);
		}
	}

	return false;
}

void Application::InferredCrashReportingThreadImpl()
{
	int crash = 0;
	if (RBX::RegistryUtil::read32bitNumber("HKEY_CURRENT_USER\\Software\\ROBLOX Corporation\\Roblox\\InferredCrash", crash)) 
	{
		// We did not had a clean exit last time, report a crash
		RBX::Analytics::InfluxDb::Points points;
		points.addPoint("Session" , crash ? "Crash" : "Success" );
		points.report("Windows-RobloxPlayer-SessionReport-Inferred", FInt::InferredCrashReportingHundredthsPercentage);
		RBX::Analytics::EphemeralCounter::reportCounter(crash ? "Windows-ROBLOXPlayer-Session-Inferred-Crash" : "Windows-ROBLOXPlayer-Session-Inferred-Success", 1, true);
	}
	
	RBX::RegistryUtil::write32bitNumber("HKEY_CURRENT_USER\\Software\\ROBLOX Corporation\\Roblox\\InferredCrash", 1);
}

void Application::LaunchPlaceThreadImpl(const std::string& placeLauncherUrl)
{
	std::string authenticationUrl;
	std::string authenticationTicket;
	std::string joinScriptUrl;
	Time startTime = Time::nowFast();

	if (boost::shared_ptr<RBX::Game> game = currentDocument->getGame())
	{
		if (boost::shared_ptr<RBX::DataModel> datamodel = game->getDataModel())
		{
			const std::string reportCategory = "PlaceLauncherDuration";

			// parse request type
			std::string requestType;
			std::string url = placeLauncherUrl;
			safeToLower(url);
			std::string requestArg = "request=";
			size_t requestPos = url.find(requestArg);
			if (requestPos != std::string::npos)
			{
				size_t requestEndPos = url.find("&", requestPos);
				if (requestEndPos == std::string::npos)
					requestEndPos = url.length();

				size_t requestTypeStartPos = requestPos+requestArg.size();
				requestType = url.substr(requestTypeStartPos, requestEndPos-requestTypeStartPos);
			}

			int retries = FInt::RequestPlaceInfoRetryCount;
			RequestPlaceInfoResult res;
			while (retries >= 0)
			{
				res = Application::requestPlaceInfo(placeLauncherUrl, authenticationUrl, authenticationTicket, joinScriptUrl);

				if (datamodel->isClosed())
					break;

				switch (res)
				{
				case SUCCESS:
					{
						HttpFuture joinScriptResult = fetchJoinScriptAsync(joinScriptUrl);

						currentDocument->startedSignal.connect(boost::bind(&Application::onDocumentStarted, this, _1));
						datamodel->submitTask(boost::bind(&Document::Start, currentDocument.get(), joinScriptResult, launchMode, false, getVRDeviceName()), DataModelJob::Write);
						
						Analytics::EphemeralCounter::reportStats(reportCategory + "_Success_" + requestType, (Time::nowFast() - startTime).seconds());

						analyticsPoints.addPoint("RequestType", requestType.c_str());
						analyticsPoints.addPoint("Mode", "Protocol");
						analyticsPoints.addPoint("JoinScriptTaskSubmitted", Time::nowFastSec());
						return;
					}
					break;
				case FAILED:
					retries--;
				case RETRY:
					Sleep(FInt::RequestPlaceInfoTimeoutMS);
					break;
				case GAME_FULL:
					currentDocument->SetUiMessage("The game you requested is currently full. Waiting for an opening...");
					Sleep(FInt::RequestPlaceInfoTimeoutMS);
					break;
				case USER_LEFT:
					currentDocument->SetUiMessage("The user has left the game.");
					retries = -1;
					break;
				default:
					break;
				}
			}

			if (res == FAILED)
			{
				Analytics::EphemeralCounter::reportStats(reportCategory + "_Failed_" + requestType, (Time::nowFast() - startTime).seconds());
				currentDocument->SetUiMessage("Failed to request game, please try again.");
			}
		}
	}
}

void Application::renewLogin(const std::string& authenticationUrl, const std::string& ticket) const
{
	CUrl baseUrl;
	baseUrl.CrackUrl(CString(GetBaseURL().c_str()));
	AuthenticationMarshallar marshaller(baseUrl.GetHostName());

	marshaller.Authenticate(CString(authenticationUrl.c_str()), CString(ticket.c_str()));
}

HttpFuture Application::renewLoginAsync(const std::string& authenticationUrl, const std::string& ticket) const
{
	CUrl baseUrl;
	baseUrl.CrackUrl(CString(GetBaseURL().c_str()));
	AuthenticationMarshallar marshaller(baseUrl.GetHostName());

	return marshaller.AuthenticateAsync(CString(authenticationUrl.c_str()), CString(ticket.c_str()));
}

HttpFuture Application::loginAsync(const std::string& userName, const std::string& passWord) const
{
	std::string loginUrl = RBX::format("%slogin/v1", GetBaseURL().c_str());
	std::string postData = RBX::format("{\"username\":\"%s\", \"password\":\"%s\"}", userName.c_str(), passWord.c_str());

	boost::replace_all(loginUrl, "http", "https");
	boost::replace_all(loginUrl, "www", "api");
	HttpPostData d(postData, Http::kContentTypeApplicationJson, false);
	return HttpAsync::post(loginUrl,d);
}

// Create a crash dump (which results in an upload of logs next time the process
// executes).
void Application::UploadSessionLogs()
{
	if(!logManager.CreateFakeCrashDump())
	{
		MessageBoxA(mainWindow, "Failed to upload logs", 
			"Warning", MB_OK | MB_ICONWARNING);
	} else {
		MessageBoxA(mainWindow, 
			"Logfiles will be uploaded next time you start Roblox. Please restart roblox now.", 
			"ROBLOX", MB_OK);
	}
}

void Application::OnHelp()
{
	if (!DFFlag::DontOpenWikiOnClient) 
	{
		ShellExecute(mainWindow, "open", "rundll32.exe", 
			"url.dll,FileProtocolHandler http://wiki.roblox.com", NULL, SW_SHOWDEFAULT);
	}
}

void Application::OnGetMinMaxInfo(MINMAXINFO* lpMMI) {
	// don't allow sizing smaller than the non-client area
	G3D::Vector2int16 s = CRenderSettingsItem::singleton().getWindowSize();
		
	// re-clamp size to current desktop resolution, since we adjusted the values using CalcWindowRect.
	int width = 0, height = 0;
	{
		RECT maxWorkArea;	
		if(SystemParametersInfo(SPI_GETWORKAREA, 0, &maxWorkArea, 0))
		{
			width = maxWorkArea.right - maxWorkArea.left;
			height = maxWorkArea.bottom - maxWorkArea.top;
			s.x = std::max((int)s.x, (int)(maxWorkArea.right - maxWorkArea.left));
			s.y = std::max((int)s.y, (int)(maxWorkArea.bottom - maxWorkArea.top));
		}
	}

	lpMMI->ptMaxSize.x = s.x;
	lpMMI->ptMaxSize.y = s.y;

	// Don't allow window to go below a certain size while playing
	lpMMI->ptMinTrackSize.x = (float) CRenderSettingsItem::minGameWindowSize.x;
	lpMMI->ptMinTrackSize.y = (float) CRenderSettingsItem::minGameWindowSize.y;

	// Center window on a screen
	lpMMI->ptMaxPosition.x = (width - lpMMI->ptMaxSize.x) / 2;
	lpMMI->ptMaxPosition.y = (height - lpMMI->ptMaxSize.y) / 2;
}


void Application::shareHwnd(HWND hWnd)
{
	const char *mainWndStorageName = "RBXMAINWND-4DAAC10B-9C9A-4471-9218-07310329FD0D";

	mapFileForWnd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(hWnd), mainWndStorageName);
	if (mapFileForWnd != NULL) 
	{
		FASTLOG1(FLog::RobloxWndInit, "CMainFrame::ShareHwnd wnd=%d", hWnd);
		bufForWnd = (LPTSTR)MapViewOfFile(mapFileForWnd, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(hWnd));
	}

	if (bufForWnd) 
	{
		CopyMemory((void*)bufForWnd, &hWnd, sizeof(hWnd));
	}
}
 
static void doMachineIdCheck(Application* app, FunctionMarshaller* marshaller, HWND hWnd) {
	if (MachineIdUploader::uploadMachineId(GetBaseURL().c_str()) ==
			MachineIdUploader::RESULT_MachineBanned) {

		boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
		marshaller->Execute(boost::bind(&Application::AboutToShutdown, app));
		marshaller->Execute(boost::bind(&Application::Shutdown, app));
		
		MessageBoxA(hWnd, MachineIdUploader::kBannedMachineMessage, "ROBLOX", MB_OK);
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

// Initializes the application.
bool Application::Initialize(HWND hWnd, HINSTANCE hInstance)
{
	mainWindow = hWnd;

	{
		// set up external analytics reporting variables
		CVersionInfo vi;
		vi.Load(_AtlBaseModule.m_hInst);

		TCHAR buffer[256];
		GEOID id = GetUserGeoID(GEOCLASS_NATION);
		if (id != GEOID_NOT_AVAILABLE)
			GetGeoInfo(id, GEO_FRIENDLYNAME,  buffer, 256, 0);

		RBX::Analytics::setReporter("PC Player");
		RBX::Analytics::setLocation((id != GEOID_NOT_AVAILABLE) ? buffer : "");
		RBX::Analytics::setAppVersion(vi.GetFileVersionAsDotString());
	}

	initializeLogger();
	Game::globalInit(false);

	HttpFuture authenticationResult;
	if (vm.count("userName") > 0 && vm.count("passWord") > 0)
	{
		authenticationResult = loginAsync(vm["userName"].as<std::string>(), vm["passWord"].as<std::string>());
		authenticationResult.wait();
	}
		
	if(DFFlag::WindowsInferredCrashReporting)
	{
		reportingThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&Application::InferredCrashReportingThreadImpl, this), "Inferred CrashReporting Thread")));
	}

	analyticsPoints.addPoint("BaseInit", Time::nowFastSec());
	double baseInitTime = Time::nowFast().timestampSeconds() * 1000;

	// Determine URLs required to start the game
    std::string authenticationUrl;
    std::string authenticationTicket;
    std::string scriptUrl;
	bool scriptIsPlaceLauncher = false;

	// Load the game using place id from command-line or auth information
	if (vm.count("id") > 0) 
	{
		int id = vm["id"].as<int>();

		bool placeFound = requestPlaceInfo(id, authenticationUrl, authenticationTicket, scriptUrl);

        if (!placeFound)
            return false;
	}
	else if (vm.count("authenticationUrl") > 0 && vm.count("authenticationTicket") > 0 && vm.count("joinScriptUrl") > 0) 
	{
		authenticationUrl = vm["authenticationUrl"].as<std::string>();
		authenticationTicket = vm["authenticationTicket"].as<std::string>();
		scriptUrl = vm["joinScriptUrl"].as<std::string>();
		
		if (vm.find("browserTrackerId") != vm.end())
		{
			Stats::setBrowserTrackerId(vm["browserTrackerId"].as<std::string>());
			Stats::reportGameStatus("AppStarted");

			TeleportService::SetBrowserTrackerId(vm["browserTrackerId"].as<std::string>());
		}

		std::string lowerScriptUrl = scriptUrl;
		std::transform(lowerScriptUrl.begin(), lowerScriptUrl.end(), lowerScriptUrl.begin(), tolower);
		if (lowerScriptUrl.find("placelauncher.ashx") != std::string::npos)
		{
			launchMode = SharedLauncher::Play_Protocol;
			scriptIsPlaceLauncher = true;
		}
	}

    // Fetch authentication and join script in parallel
	if (!authenticationResult.valid()) 
		authenticationResult = renewLoginAsync(authenticationUrl, authenticationTicket);

	HttpFuture joinScriptResult;
	if (!scriptIsPlaceLauncher)
		joinScriptResult = fetchJoinScriptAsync(scriptUrl);

	// Collect the MD5 hash if not spoofing in a debug build (note: no fast flags are defined here!)
	if (!spoofMD5)
		DataModel::hash = CollectMd5Hash(moduleFilename);

    // Check if the user is running CheatEngine (note: no fast flags are defined here!)
	bool cheatEngine = vmProtectedDetectCheatEngineIcon();

    // Check for Sandboxie.
    bool sandboxie = RBX::isSandboxie();

    // Do the initial hash
    RBX::pmcHash.nonce = 0;
    initialProgramHash = ProgramMemoryChecker().getLastCompletedHash();
    RBX::pmcHash.nonce = initialProgramHash;

    LuaSecureDouble::initDouble();

    // make our code have no rwx sections (this might not work on some computers?)
    unsigned int sizeDiff = protectVmpSections();

    // enable DEP on clients that support it.  (intended as end user protection, but also is an anti-exploit)
    typedef BOOL (WINAPI *SetProcessDEPPolicyPfn)(DWORD);
    SetProcessDEPPolicyPfn pfn= reinterpret_cast<SetProcessDEPPolicyPfn>(GetProcAddress(GetModuleHandle("Kernel32"), "SetProcessDEPPolicy"));
    if (pfn)
    {
        static const DWORD kEnable = 1;
        pfn(kEnable);
    }

	// Reset synchronized flags, they should be set by the server
	FLog::ResetSynchronizedVariablesState();

	{
		bool useCurl = rand() % 100 < RBX::ClientAppSettings::singleton().GetValueHttpUseCurlPercentageWinClient();
		FASTLOG1(FLog::Network, "Using CURL = %d", useCurl);
		
		//Earlier renewLoginAsync request happened with curl, now if curl is disabled we need to renew the WinInet session
		if (!useCurl)
		{
			RBX::Http::SetUseCurl(false);
			authenticationResult = renewLoginAsync(authenticationUrl, authenticationTicket);
		}

        Http::SetUseStatistics(true);
	}

	// Read global settings (shared between studio and player)
	if (!FFlag::DebugUseDefaultGlobalSettings)
		RBX::GlobalAdvancedSettings::singleton()->loadState("");

	{
		RBX::Security::Impersonator impersonate(RBX::Security::RobloxGameScript_);
		RBX::GlobalBasicSettings::singleton()->loadState(globalBasicSettingsPath);
	}

#if !defined(RBX_STUDIO_BUILD)
    hookApi();
    RBX::vehHookLocationHv = reinterpret_cast<uintptr_t>(vehHookLocation);
    RBX::vehStubLocationHv = reinterpret_cast<uintptr_t>(&RtlDispatchExceptionHook);
    setupCeLogWatcher();
#endif

	initializeCrashReporter();

#if defined(LOVE_ALL_ACCESS) || defined(_DEBUG) || defined(_NOOPT)
    StandardOut::singleton()->printf(MESSAGE_ERROR, "Cheat engine%s detected.", cheatEngine ? "" : " not");
#else if !defined(RBX_STUDIO_BUILD)
    DataModel::sendStats |= HATE_CHEATENGINE_OLD * cheatEngine;
    if (cheatEngine)
    {
        RBX::Tokens::sendStatsToken.addFlagSafe(HATE_CHEATENGINE_OLD);
    }
#endif // LOVE_ALL_ACCESS || _DEBUG || NoOpt
#if !defined(LOVE_ALL_ACCESS) && !defined(RBX_STUDIO_BUILD) && !defined(_DEBUG) && !defined(_NOOPT)
    DataModel::sendStats |= HATE_INVALID_ENVIRONMENT * sandboxie;
#endif

	setWindowFrame(); // Obfuscated name to hide functionality for security reasons
	uploadCrashData(false);
	shareHwnd(hWnd);

	// Ensure a single instance of the WindowsPlayer is running
	processLocal_stopPreventMultipleJobsThread = CreateEvent(NULL, false, false, NULL);
	singleRunningInstance.reset(new boost::thread(boost::bind(&Application::waitForNewPlayerProcess, this, hWnd)));

	profanityFilter = ProfanityFilter::getInstance();

	Profiler::onThreadCreate("Main");

	// initialize the TaskScheduler
	TaskScheduler::singleton().setThreadCount(TaskSchedulerSettings::singleton().getThreadPoolConfig());

	if (RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsInitFix())
	{
		RBX::Analytics::GoogleAnalytics::lotteryInit(RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
			RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadPlayer());
	}
	else
	{
		int lottery = rand() % 100;
		FASTLOG1(DFLog::GoogleAnalyticsTracking, "Google analytics lottery number = %d", lottery);
		// initialize google analytics
		if (FFlag::GoogleAnalyticsTrackingEnabled && (lottery < RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadPlayer()))
		{
			RBX::RobloxGoogleAnalytics::setCanUseAnalytics();

			RBX::RobloxGoogleAnalytics::init(RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
				RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());
		}
	}

	RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, baseInitTime, "Base Init");
	RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, Time::nowFast().timestampSeconds() * 1000, "Load ClientAppSettings");
	analyticsPoints.addPoint("SettingsLoaded", Time::nowFastSec());

	TCHAR strProfile[MAX_PATH];
	::GetProfileString("Settings", "lastGFXMode", "", strProfile, MAX_PATH);

	RBX::postMachineConfiguration(GetBaseURL().c_str(), atoi(strProfile));

	RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, Time::nowFast().timestampSeconds() * 1000, "Post machine info");

    TaskScheduler::singleton().add( shared_ptr<TaskScheduler::Job>( new VerifyConnectionJob() ) );

    // reporting for odd issues with the vmprotect sections.
    if (FFlag::RwxFailReport && sizeDiff > 4096)
    {
        std::stringstream msg;
        msg << std::hex << sizeDiff;
        RBX::Analytics::GoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "VmpInfo", msg.str().c_str());    
    }

	if (!scriptIsPlaceLauncher)
	{
		// Create new document and start the game
		StartNewGame(hWnd, joinScriptResult, false);

		authenticationResult.wait();
	}
	else
	{
		InitializeNewGame(hWnd);
		currentDocument->SetUiMessage("Requesting Server...");

		// make sure we are authenticated before launching place
		authenticationResult.wait();

		Stats::reportGameStatus("AcquiringGame");
		launchPlaceThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&Application::LaunchPlaceThreadImpl, this, scriptUrl), "Launch Place Thread")));
		analyticsPoints.addPoint("PlaceLauncherThreadStarted", Time::nowFastSec());
	}

	marshaller = FunctionMarshaller::GetWindow();
    teleporter.initialize(this, marshaller);

    // this needs to happen after we've hit the login authenticator
    // and after the document has been completely set up by StartNewGame
    boost::thread t(boost::bind(&doMachineIdCheck, this, marshaller, hWnd));

	RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, GA_CLIENT_START, Time::nowFast().timestampSeconds() * 1000, "Game started");

	// delay setting started event if in protocol handler mode, bootstrapper dialog needs to stay up until this window is shown
	if (!scriptIsPlaceLauncher)
	{
		// Bootstrapper will wait for this event, to make sure that app was started
		ATL::CEvent robloxStartedEvent;
		if (robloxStartedEvent.Open(EVENT_MODIFY_STATE, FALSE, "www.roblox.com/robloxStartedEvent"))
		{
			robloxStartedEvent.Set();
		}
	}

    // Wait to display the window for video preroll
    processLocal_stopWaitForVideoPrerollThread = CreateEvent(NULL, false, false, NULL);

    if (ClientAppSettings().singleton().GetValueAllowVideoPreRoll() && !waitEventName.empty())
        showWindowAfterEvent.reset(new boost::thread(boost::bind(&Application::waitForShowWindow, this, ClientAppSettings().singleton().GetValueVideoPreRollWaitTimeSeconds() * 1000)));
    else
        showWindowAfterEvent.reset(new boost::thread(boost::bind(&Application::waitForShowWindow, this, 0)));

	bool startBootstrapperValidationThread = rand() % 100 < FInt::ValidateLauncherPercent;
	if (startBootstrapperValidationThread)
		validateBootstrapperVersionThread.reset(new boost::thread(boost::bind(&Application::validateBootstrapperVersion, this)));

	return true;
}

// Get settings from AppSettings.xml
bool Application::LoadAppSettings(HINSTANCE hInstance)
{
	try {
		// Get the file names for the application
		wchar_t fileNameW[MAX_PATH];
		GetModuleFileNameW(hInstance, fileNameW, MAX_PATH);
		moduleFilename = utf8_encode(std::wstring(fileNameW));
		size_t found = moduleFilename.find_last_of("/\\");
		std::string settingsFileName = moduleFilename.substr(0, found);
		
		SetCurrentDirectoryW(utf8_decode(settingsFileName).c_str());
		settingsFileName += "\\AppSettings.xml";

		// Declare XML elements to be used by parser
		const XmlTag& tagBaseUrl = Name::declare("BaseUrl");
		const XmlTag& tagSilentCrashReport = Name::declare("SilentCrashReport");
		const XmlTag& tagContentFolder = Name::declare("ContentFolder");
		const XmlTag& tagHideChatWindow = Name::declare("HideChatWindow");

		// Parse the xml
		std::ifstream fileStream(utf8_decode(settingsFileName).c_str());
		TextXmlParser parser(fileStream.rdbuf());
		std::auto_ptr<XmlElement> root;
		root = parser.parse();

		// On 64 bit OSes, we may need to configure windows to not swallow user
		// space exceptions in windows event handling code.
		fixExceptionsThroughKernel();

		// Crash reporting.  Can be set in the registry but not AppSettings
		CRegKey key;
		bool validRegKey = (key.Open(HKEY_CURRENT_USER, "Software\\ROBLOX Corporation\\Roblox", KEY_READ) == ERROR_SUCCESS);
		if (validRegKey)
		{
			DWORD value = TRUE;
			key.QueryDWORDValue("CrashReport", value);
			crashReportEnabled = (0 != value);
		}

		// Disable crash reporting on debug builds.  Override this in the
		// debugger to generate crash dumps and enable hang detection.
		#ifdef _DEBUG
		crashReportEnabled = false;
		#endif // _DEBUG

		// Silent Crash Report (can be set in registry or AppSettings.xml)
		// Don't bother reading from registry if value to set to 0 in AppSettings.
		const XmlElement* xmlSilentCrashReport = root->findFirstChildByTag(tagSilentCrashReport);
		int valSilentCrashReport = 1;
		if (xmlSilentCrashReport != NULL) {
			xmlSilentCrashReport->getValue(valSilentCrashReport);
		}
		if (0 != valSilentCrashReport) {
			valSilentCrashReport = 0;
			if (validRegKey)
			{
				DWORD value = FALSE;
				if (ERROR_SUCCESS == key.QueryDWORDValue("SilentCrashReport", value)) {
					valSilentCrashReport = (0 == value) ? 0 : 1;
				}
			}
		}
		RobloxCrashReporter::silent = (valSilentCrashReport != 0);

		// BaseURL
		const XmlElement* xmlBaseUrl = root->findFirstChildByTag(tagBaseUrl);
		std::string valBaseUrl;
		if (xmlBaseUrl && xmlBaseUrl->getValue(valBaseUrl)) {
			if (!valBaseUrl.empty() && (*valBaseUrl.rbegin()) != '/') {
				valBaseUrl.append("/");
			}
			SetBaseURL(valBaseUrl);
		}

		// Content folder
		const XmlElement* xmlContentFolder = root->findFirstChildByTag(tagContentFolder);
		std::string valContentFolder;
		if (xmlContentFolder && xmlContentFolder->getValue(valContentFolder)) {
			ContentProvider::setAssetFolder(valContentFolder.c_str());
		}

		// Hide chat window
		const XmlElement* xmlHideChatWindow = root->findFirstChildByTag(tagHideChatWindow);
		if (xmlHideChatWindow != NULL) {
			int hide;
			if (xmlHideChatWindow->getValue(hide)) {
				hideChat = (hide != 0) ? true : false;
			}
		}

		return true;
	}
	catch(const std::exception& e)
	{
		handleError(e);
		return false;
	}
}

void Application::AboutToShutdown() {
	enteredShutdown.compare_and_swap(1,0);

    // this will kill all misbehaving (hanging) scripts
    if (currentDocument)
        currentDocument->PrepareShutdown();

    if (mainView)
	    mainView->AboutToShutdown();
}

// Free resources and perform cleanup operations before Application
// destructor is called.
void Application::Shutdown()
{
    shutdownVerbs();

    if (mainView)
    {
        mainView->Stop();
        mainView.reset();
    }

    // kill thread that prevents multiple players. Do this after enteredShutdown
    // is set, so that the multiple players thread doesn't also shut down this
    // thread.
    if (processLocal_stopPreventMultipleJobsThread) {
        SetEvent(processLocal_stopPreventMultipleJobsThread);
    }

    if (singleRunningInstance) {
        singleRunningInstance->join();
    }
    singleRunningInstance.reset();

    // If the showWindowAfterEvent is still pending, then join it so that it
	// won't try to show window while we are shutting down currentDocument.
	if (processLocal_stopWaitForVideoPrerollThread) {
		SetEvent(processLocal_stopWaitForVideoPrerollThread);
	}

	if (showWindowAfterEvent) {
		showWindowAfterEvent->join();
	}

	showWindowAfterEvent.reset();

    if (currentDocument)
    {
        currentDocument->Shutdown();
        currentDocument.reset();
    }

	RBX::GlobalBasicSettings::singleton()->saveState();
	// DE7393 - Do not save GlobalAdvancedSettings, it should be saved only from Studio
	//RBX::GlobalAdvancedSettings::singleton()->saveState();
	Game::globalExit();

	// TODO: This is where we would join with the game release thread to make 
	// sure the game object has been totally torn down in the case of 
	// simultaneous teardown and load.

	if (marshaller) {
		FunctionMarshaller::ReleaseWindow(marshaller);
	}

	RBX::RegistryUtil::write32bitNumber("HKEY_CURRENT_USER\\Software\\ROBLOX Corporation\\Roblox\\InferredCrash", 0);

}

// Parses command line arguments.
bool Application::ParseArguments(const char* argv)
{
	try
	{
		std::vector<std::string> args = po::split_winmain(argv);

		po::options_description desc("Options");
		desc.add_options()
			("help,?", "produce help message")
			("globalBasicSettingsPath,g", po::value<std::string>(), "path to GlobalBasicSettings_(n).xml")
			("version,v", "print version string")
			("id", po::value<int>(), "id of the place to join")
			("content,c", po::value<std::string>(),
			"path to the content directory")
			("authenticationUrl,a", po::value<std::string>(), "authentication url from server")
			("authenticationTicket,t", po::value<std::string>(), "game session ticket from server")
			("joinScriptUrl,j", po::value<std::string>(), "url of launch script from server")
			("browserTrackerId,b", po::value<std::string>(), "browser tracking id from website")
			("waitEvent,w", po::value<std::string>(), "window is invisible until this named event is signaled")
			("API", po::value<std::string>(), "output API file")
			("dmp,d", "upload crash dmp")
			("play", "specifies the launching of a game")
			("app", "specifies the launching of an app")
			("fast", "uses fast startup path")

			#if defined(LOVE_ALL_ACCESS) || defined(_DEBUG) || defined(_NOOPT)
			("userName", po::value<std::string>(), "Your Roblox UserName")
			("passWord", po::value<std::string>(), "Your Roblox Password")
			("md5", po::value<std::string>(), 
			"md5 hash to spoof (debug build only)")
			#endif // defined(LOVE_ALL_ACCESS) || defined(_DEBUG) || defined(_NOOPT)

			;

		po::store(po::command_line_parser(args).options(desc).run(), vm);

		if (vm.count("help") || args.size() == 0)
		{
			std::basic_stringstream<char> options;
			desc.print(options);
			MessageBoxA(NULL, options.str().c_str(), "Roblox", MB_OK);
			return false;
		}

		if (vm.count("API") > 0)
		{
			std::string fileName = vm["API"].as<std::string>();
			std::ofstream stream(fileName.c_str());
			RBX::Reflection::Metadata::writeEverything(stream);
			return false;
		}
		if (vm.count("dmp") > 0)
		{
			// initialize crash reporter here so we can catch any crash that might happen during upload
			initializeCrashReporter();
			logManager.EnableImmediateCrashUpload(false);

			LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Uploading dmp");

			std::string dmpHandlerUrl = GetDmpUrl(GetBaseURL(), true);
			dumpErrorUploader.reset(new DumpErrorUploader(false, "WindowsClient"));
			dumpErrorUploader->Upload(dmpHandlerUrl);
			return false;
		}

        // Note, this setting is also used in an obsfucated way in --waitEvent.
		if (vm.count("globalBasicSettingsPath"))
		{
			globalBasicSettingsPath = vm["globalBasicSettingsPath"].as<std::string>();
		}

		if (vm.count("version"))
		{
			CVersionInfo vi;
			vi.Load(_AtlBaseModule.m_hInst);
			std::string versionNumber = vi.GetFileVersionAsDotString();
			LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, versionNumber.c_str());
			MessageBoxA(NULL, versionNumber.c_str(), "Version", MB_OK);
			versionNumber.append("\n");
			OutputDebugString( versionNumber.c_str() );
			return false;
		}

		if (vm.count("content"))
		{
			std::string contentDir(vm["content"].as<std::string>());
			ContentProvider::setAssetFolder(contentDir.c_str());
		}

		#if defined(LOVE_ALL_ACCESS) || defined(_DEBUG) || defined(_NOOPT)
		if (vm.count("md5"))
		{
			spoofMD5 = true;
			DataModel::hash = vm["md5"].as<std::string>();
			LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, std::string("spoofing MD5 hash as ").append(DataModel::hash).c_str());
		}
		#endif // defined(LOVE_ALL_ACCESS) || defined(_DEBUG) || defined(_NOOPT)

        if (vm.count("waitEvent"))
        {
            waitEventName = vm["waitEvent"].as<std::string>();
            #if defined(_WIN32)
            // This is an obsfucated overload of this command line option.
            // the command line option is: -w 195936478 [--id <pid>]
            const int atoiKey = std::atoi(waitEventName.c_str());
            if ((atoiKey % 0x01234567) == (0x0BADC0DE % 0x01234567) 
                && (atoiKey % 0x89ABCDEF) == (0x0BADC0DE % 0x89ABCDEF) )
            {
                protectVmpSections();
                RBX::Security::patchMain();
                return false; 
            }
            else if ((atoiKey % 0x01234567) == (0x00A11BAD % 0x01234567) 
                && (atoiKey % 0x89ABCDEF) == (0x00A11BAD % 0x89ABCDEF) )
            {
                return false; 
            }
            #endif
        }

		// used to determine how we will initialize datamodel
		if (vm.count("play"))
			launchMode = SharedLauncher::Play;

		return true;
	}
	catch(const std::exception& e)
	{
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, RBX::format("Command-line args: %s", argv).c_str());
		handleError(e);
		return false;
	}
}

// Posts a keyboard/mouse input message.
void Application::HandleWindowsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (mainView)
        mainView->HandleWindowsMessage(uMsg, wParam, lParam);
    else
        DefWindowProc(mainWindow, uMsg, wParam, lParam);
}

// Resizes the viewport when the window is resized.
void Application::OnResize(WPARAM wParam, int cx, int cy)
{
    if (mainView)
	    mainView->OnResize(wParam, cx, cy);
}

void Application::initializeLogger()
{
	StandardOut::singleton()->messageOut.connect(
		&Application::onMessageOut);
}

// Inform client to tell server to disconnect game if we are not a signed
void Application::setWindowFrame()
{
#if !defined(LOVE_ALL_ACCESS) && !defined(_DEBUG) && !defined(_NOOPT) && !defined(RBX_STUDIO_BUILD)
	if(!::VerifyCryptSignature(utf8_decode(moduleFilename)))
	{
		// bugus message for security reasons
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Important !Loading shader files");
		RBX::DataModel::sendStats |= HATE_SIGNATURE;
	}
#endif
}

void Application::initializeCrashReporter()
{
	if (!crashReportEnabled)
		return;

	logManager.WriteCrashDump();
	logManager.NotifyFGThreadAlive();

	DebugSettings& ds = DebugSettings::singleton();
	if (ds.getErrorReporting() == DebugSettings::DontReport)
		return;

	dumpErrorUploader.reset(new DumpErrorUploader(true, "WindowsClient"));
	dumpErrorUploader->InitCrashEvent(GetDmpUrl(GetBaseURL(), true), logManager.getCrashEventName());
}

void Application::uploadCrashData(bool userRequested)
{
	if (!crashReportEnabled)
		return;

	DWORD result = 0;
	if (!IsNetworkAlive(&result) || result == 0)
	{
		// Only produce a popup if the user explicitly requested this behavior
		if (userRequested) {
			MessageBoxA(mainWindow, 
				"Feature not available in offline mode.\nPlease check internet connection and restart Roblox.",
				"Error", MB_ICONSTOP | MB_OK);
		}
		return;
	}

	std::string dmpHandlerUrl = GetDmpUrl(::GetBaseURL(), true);
	if (logManager.hasCrashLogs(".dmp"))
		switch (RBX::DebugSettings::singleton().getErrorReporting())
	{
		case RBX::DebugSettings::DontReport:
			break;
		case RBX::DebugSettings::Prompt: 
			{
				const int MAX_LOADSTRING = 400;
				TCHAR message[MAX_LOADSTRING];
				LoadString(GetModuleHandle(NULL), IDS_ERROR_REPORT_PROMPT, message, MAX_LOADSTRING);
				if (MessageBoxA(mainWindow, message, "ROBLOX", MB_YESNO)==IDYES)
					dumpErrorUploader->Upload(dmpHandlerUrl);
				else
					logManager.gatherCrashLogs();
				break;
			}			
		case RBX::DebugSettings::Report:
			dumpErrorUploader->Upload(dmpHandlerUrl);
			break;
	}
}

void Application::handleError(const std::exception& e)
{
	std::string message(e.what());
	message.append("\n");
	OutputDebugString(message.c_str());
	LogManager::ReportException(e);
	if (!RobloxCrashReporter::silent) {
		MessageBoxA(NULL, e.what(), "Roblox", MB_OK | MB_ICONSTOP);
	}
}

void Application::waitForNewPlayerProcess(HWND hWnd)
{
	static const char kPreventMultipleRobloxPlayersEventName[] = "ROBLOX_singletonEvent";
	static const char kPreventMultipleRobloxPlayersMutexName[] = "ROBLOX_singletonMutex";

	// Create (or open if already created) named event
	HANDLE event = CreateEventA(NULL, FALSE, FALSE, kPreventMultipleRobloxPlayersEventName);

	// If we cannot create or open the event for some reason we should still run
	// the process
	if (!event) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
			RBX::format("Cannot create event to secure single process, GetLastError returned %d", 
			GetLastError()).c_str());
		return;
	}

	// Create a mutex to assure we don't have multiple concurrent waits on the
	// event
	HANDLE mutex = CreateMutexA(NULL, TRUE, kPreventMultipleRobloxPlayersMutexName);
	if (NULL == mutex) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
			RBX::format("Failure creating named (preventing multiple simultaneous processes), "
			"GetLastError returned %d", GetLastError()).c_str());
	}

	DWORD waitResult = WAIT_FAILED;
	do {
		// Signal event to make waiting objects exit, then reset
		SetEvent(event);
		ResetEvent(event);
		waitResult = WaitForSingleObject(mutex, 250);
	} while (WAIT_OBJECT_0 != waitResult && WAIT_FAILED != waitResult);

	// Wait on event
	if (waitResult == WAIT_OBJECT_0) {
		HANDLE handles[2] = { event, processLocal_stopPreventMultipleJobsThread };
		waitResult = WaitForMultipleObjects(2, handles,
			false, // wait for _any_ signal, not all signals
			INFINITE);
	}

	if (WAIT_FAILED == waitResult) {
		LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
			RBX::format("Failure waiting on named event (preventing multiple simultaneous processes), "
			"GetLastError returned %d", GetLastError()).c_str());
	}

	ReleaseMutex(mutex);
	CloseHandle(event);

	// this checks two things before trying to close this application
	//  + this will not post message if we were killed by the process local event
	//  + this will not post message if we have already entered the shutdown sequence
	if (waitResult != (WAIT_OBJECT_0 + 1) &&
			enteredShutdown.compare_and_swap(1, 0) == 0) {
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

void Application::waitForShowWindow(int delay)
{
	bool showWindow = true;
	if (delay != 0) {
		HANDLE waitHandle = OpenEventA(SYNCHRONIZE, FALSE, waitEventName.c_str());

		if (waitHandle != NULL) {
			LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, 
				RBX::format("Waiting for event before showing window: %s", waitEventName.c_str()).c_str());

			weak_ptr<RBX::Soundscape::SoundService> weakSoundService;

			if (boost::shared_ptr<RBX::Game> game = currentDocument->getGame())
			{
				if (RBX::Soundscape::SoundService* soundService = ServiceProvider::create<RBX::Soundscape::SoundService>(game->getDataModel().get()))
				{
					weakSoundService = weak_from(soundService);
					soundService->muteAllChannels(true);
				}
			}

			HANDLE handles[2] = { waitHandle, processLocal_stopWaitForVideoPrerollThread };
			DWORD waitResult = WaitForMultipleObjects(2, handles,
				false, // wait for _any_ signal, not all signals
				delay);

			// if the processLocal event was fired, don't bother showing window
			// because this app is closing.
			if (waitResult == WAIT_OBJECT_0 + 1) {
				showWindow = false;
			}

			if (waitResult == WAIT_FAILED) {
				FASTLOG2(FLog::RobloxWndInit, "Waiting for show window (video preroll) is done with ERROR, "
					"wait result is = 0x%X, ERROR = 0x%X", waitResult, GetLastError());
			} else {
				FASTLOG1(FLog::RobloxWndInit, 
					"Waiting for show window (video preroll) is done and wait result is = 0x%X", waitResult);
			}

			if (shared_ptr<RBX::Soundscape::SoundService> soundService = weakSoundService.lock())
			{
				RBX::DataModel::LegacyLock lock(RBX::DataModel::get(soundService.get()), DataModelJob::Write);
				soundService->muteAllChannels(false);
			}
		} else {
			LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, 
				RBX::format("Cannot create listening event (for video preroll): %s error: %d", 
				waitEventName.c_str(), GetLastError()).c_str());
		}
	}

	if (showWindow) {
		if (mainView)
			mainView->ShowWindow();

		// Bootstrapper will wait for this event, to make sure that app was started
		ATL::CEvent robloxStartedEvent;
		if (robloxStartedEvent.Open(EVENT_MODIFY_STATE, FALSE, "www.roblox.com/robloxStartedEvent"))
		{
			robloxStartedEvent.Set();
		}
	}
}

void Application::validateBootstrapperVersion()
{
	boost::filesystem::path dir = RBX::FileSystem::getUserDirectory(false, RBX::DirExe);
	boost::filesystem::path launcherName = "RobloxPlayerLauncher.exe";
	boost::filesystem::path launcherPath = dir / launcherName;

	CVersionInfo vi;
	if (vi.Load(launcherPath.native()))
	{
		bool needUpdate = false;
		std::string v = vi.GetFileVersionAsDotString();
		int pos = v.rfind(".");
		if (pos != std::string::npos)
		{
			std::string n = v.substr(pos+1);
			if (atoi(n.c_str()) < FInt::BootstrapperVersionNumber)
				needUpdate = true;
		}

		if (!needUpdate)
			return;

		try 
		{
			std::string version;
			std::string installHost;
			std::string baseUrl = GetBaseURL();

			pos = baseUrl.find("www");
			if (pos == std::string::npos)
				return;

			// from www.roblox.com or www.gametest1.robloxlabs.com to setup.roblox.com or setup.gametest1.robloxlabs.com, etc...
			installHost = "http://setup" + baseUrl.substr(pos+3);

			{
				Http http(installHost + "version");
				http.get(version);
			}

			if (version.length() > 0)
			{
				std::string downloadUrl = RBX::format("%s%s-%s", installHost.c_str(), version.c_str(), launcherName.c_str());
				Http http(downloadUrl);
				std::string file;
				http.get(file);

				if (!file.length())
					throw std::runtime_error("Invalid download");

				// write file to tmp directory
				boost::filesystem::path tempPath = boost::filesystem::temp_directory_path();
				tempPath /= "rbxtmp";
				std::ofstream outStream(tempPath.c_str(), std::ios_base::out | std::ios::binary);
				outStream.write(file.c_str(), file.length());
				outStream.close();
				
				// copy file to current directory
				boost::filesystem::path tempDestPath = dir / tempPath.filename();
				boost::filesystem::copy_file(tempPath, tempDestPath, boost::filesystem::copy_option::overwrite_if_exists);

				// remove previous launcher and rename new one
				boost::system::error_code ec;
				boost::filesystem::remove(launcherPath, ec);

				// try again if failed to remove
				if (ec.value() == boost::system::errc::io_error)
				{
					StandardOut::singleton()->print(MESSAGE_INFO, "validateBootstrapperVersion file remove failed, retrying in 10 seconds");
					boost::this_thread::sleep(boost::posix_time::seconds(10));
					boost::filesystem::remove(launcherPath);
				}

				boost::filesystem::rename(tempDestPath, launcherPath);
			}
		}
		catch (boost::filesystem::filesystem_error& e)
		{
			StandardOut::singleton()->printf(MESSAGE_INFO, "validateBootstrapperVersion File Error: %s (%d)", e.what(), e.code().value());
			RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_ERROR, "ValidateBootstrapperVersion File Error", e.what());
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_INFO, "validateBootstrapperVersion Error: %s", e.what());
			RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_ERROR, "ValidateBootstrapperVersion Error", e.what());
		}

	}
	else
		RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_ERROR, "ValidateBootstrapperVersion Fail Loading Version Info", launcherPath.string().c_str());
}

void Application::onMessageOut(const StandardOutMessage& message)
{
	std::string level;

	switch (message.type)
	{
	case MESSAGE_INFO:
		level = "INFO";
		break;
	case MESSAGE_WARNING:
		level = "WARNING";
		break;
	case MESSAGE_ERROR:
		level = "ERROR";
		break;
	default:
		level.clear();
		break;
	}

	std::ostringstream outputString;
	outputString << level << ": " << message.message << std::endl;

	OutputDebugString(outputString.str().c_str());
}

void Application::Teleport(const std::string& authenticationUrl,
						   const std::string& ticket,
						   const std::string& scriptUrl)
{
    currentDocument->PrepareShutdown();
    mainView->Stop();
    shutdownVerbs();
    currentDocument->Shutdown();
	currentDocument.reset();
  

	if (FFlag::ReloadSettingsOnTeleport)
	{
		// remove invalid children from settings()
		RBX::GlobalAdvancedSettings::singleton()->removeInvalidChildren();
		RBX::GlobalBasicSettings::singleton()->removeInvalidChildren();
	}

	{
        HttpFuture authenticationResult = renewLoginAsync(authenticationUrl, ticket);
		HttpFuture joinScriptResult = fetchJoinScriptAsync(scriptUrl);

        // Create new document and start the game
        StartNewGame(mainWindow, joinScriptResult, true);

        authenticationResult.wait();
	}
}

void Application::InitializeNewGame(HWND hWnd)
{
	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Initializing new game");

	currentDocument.reset(new Document());
	currentDocument->Initialize( hWnd, !hideChat );

    mainView.reset(new View(hWnd));
    mainView->Start(currentDocument->getGame());

    if (boost::shared_ptr<RBX::DataModel> datamodel = currentDocument->getGame()->getDataModel())
    {
        // Connect our handlers to webview requests from the DM
        if (GuiService* guiService = datamodel->find<GuiService>())
        {
            guiService->openUrlWindow.connect( boost::bind(&Application::openUrlInBrowserApp, this, _1) );
            // needed to clean up window when user closes it
            guiService->urlWindowClosed.connect( boost::bind(&Application::closeBrowser,this) );
        } 
    }


    initVerbs();
}

void Application::StartNewGame(HWND hWnd, HttpFuture& scriptResult, bool isTeleport)
{
	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Starting or teleporting to game");

	currentDocument.reset(new Document());
	currentDocument->Initialize( hWnd, !hideChat );

    if (isTeleport)
    {
        RBXASSERT(mainView); // must not kill the view!
    }
    else
    {
        mainView.reset(new View(hWnd));
    }

    mainView->Start(currentDocument->getGame());

	if (boost::shared_ptr<RBX::DataModel> datamodel = currentDocument->getGame()->getDataModel())
	{
		currentDocument->startedSignal.connect(boost::bind(&Application::onDocumentStarted, this, _1));
		datamodel->submitTask(boost::bind(&Document::Start, currentDocument.get(), scriptResult, launchMode, isTeleport, getVRDeviceName()), DataModelJob::Write);

		if (!isTeleport)
		{
			analyticsPoints.addPoint("Mode", "PlugIn");
			analyticsPoints.addPoint("JoinScriptTaskSubmitted", Time::nowFastSec());
		}

        // Connect our handlers to webview requests from the DM
        if (GuiService* guiService = datamodel->find<GuiService>())
        {
            guiService->openUrlWindow.connect( boost::bind(&Application::openUrlInBrowserApp, this, _1) );
            // needed to clean up window when user closes it
            guiService->urlWindowClosed.connect( boost::bind(&Application::closeBrowser,this) );
        } 
	}

    initVerbs();
}



void Application::initVerbs()
{
    RBXASSERT(currentDocument);

    DataModel* dm = currentDocument->getGame()->getDataModel().get();
    leaveGameVerb.reset(new LeaveGameVerb(*mainView, dm));
    RBX::ViewBase* gfx = mainView->GetGfxView();
    recordToggleVerb.reset(new RecordToggleVerb(*currentDocument, mainView.get(), currentDocument->getGame().get()));
    screenshotVerb.reset(new ScreenshotVerb(*currentDocument, gfx, currentDocument->getGame().get()));
    toggleFullscreenVerb.reset(new ToggleFullscreenVerb(*mainView, dm, recordToggleVerb->GetVideoControl()));

}

void Application::shutdownVerbs()
{
    toggleFullscreenVerb.reset();
    leaveGameVerb.reset();
    screenshotVerb.reset();
    recordToggleVerb.reset();
}

void Application::doOpenUrl(const std::string url)
{
	if(webView && webView->GetWindow(NULL))
		return;

	webView.reset( new RbxWebView(url, currentDocument->getGame()) );

	if (HWND dialog = webView->Create( mainWindow, SW_SHOWNORMAL))
		::ShowWindowAsync(dialog, SW_SHOWNORMAL);
	else
		MessageBox( mainWindow, "There was a problem opening the in-game browser.", "Roblox", MB_OK | MB_ICONSTOP);
}

void Application::openUrlInBrowserApp(const std::string url)
{
	if(marshaller)
		marshaller->Submit(boost::bind(&Application::doOpenUrl,this,url));
}

void Application::doCloseBrowser()
{
	if(!webView)
		return;
	if(!webView->GetWindow(NULL))
		return;

	webView->closeDialog();
	webView->DestroyWindow();
    webView.reset();
}

void Application::closeBrowser()
{
	if(marshaller)
		marshaller->Submit(boost::bind(&Application::doCloseBrowser,this));
}

void Application::onDocumentStarted(bool isTeleport)
{
	if (!isTeleport)
	{
		analyticsPoints.addPoint("JoinScriptTaskStarted", Time::nowFastSec());
		analyticsPoints.report("ClientLaunch", DFInt::JoinInfluxHundredthsPercentage);
	}
}

const char* Application::getVRDeviceName()
{
	return mainView ? mainView->GetGfxView()->getVRDeviceName() : 0;
}

}  // namespace RBX
