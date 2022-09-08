// Bootstrapper.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"
#include "Bootstrapper.h"

#include <windows.h>
#include <Psapi.h>

#include "VersionInfo.h"
#include "FileSystem.h"
#include "atlpath.h"
#include "atlsync.h"
#include "format_string.h"
#include "ShutdownRobloxApp.h"
#include "shellapi.h"
#include "ShutdownDialog.h"
#include "MD5Hasher.h"
#include "SharedLauncher.h"
#include "StringConv.h"
#include "RobloxServicesTools.h"

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <strstream>

#include <shlguid.h>
#include <shobjidl.h>
#include <iphlpapi.h>

#include "VistaTools.h"

#include "Aclapi.h"
#include "Sddl.h"
#include "processinformation.h"
#include "CookiesEngine.h"
#include "SharedHelpers.h"
#include "HttpTools.h"
#include "GoogleAnalyticsHelper.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "Sensapi.h"
#pragma comment(lib, "Sensapi.lib")

// TODO - translation/localization
using RBX::utf8_encode;

void Bootstrapper::CreateProcess(const TCHAR* module, const TCHAR* args, PROCESS_INFORMATION& pi)
{
	LOG_ENTRY2("Bootstrapper::CreateProcess module = %S, args = %S", module, args);
	const int BufSize = 2048;
	STARTUPINFO si = {0};
	si.cb = sizeof(si);

	TCHAR cmd[BufSize] = {0};
	if (module[0] != _T('\"'))
	{
		wcscpy_s(cmd, BufSize, _T("\""));
		wcscat_s(cmd, BufSize, module);
		wcscat_s(cmd, BufSize, _T("\""));
	}
	else
	{
		wcscpy_s(cmd, BufSize, module);
	}

	wcscat_s(cmd, BufSize, _T(" "));
	wcscat_s(cmd, BufSize, args);

	throwLastError(::CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi), format_string("CreateProcess %S failed", module));
}

class CSimpleModule : public CAtlModule
{
public:
	CSimpleModule() {}
	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase*) { return S_OK; }
	int WinMain(HINSTANCE hInstance, int nShowCmd, Bootstrapper*(*newBootstrapper)(HINSTANCE)) throw()
	{
		if (CAtlBaseModule::m_bInitFailed)
		{
			ATLASSERT(0);
			return -1;
		}

		int result = 0;
		{
			boost::shared_ptr<Bootstrapper> bootstrapper;
			bool encounteredProblem = false;
			try
			{
				bootstrapper = Bootstrapper::Create(hInstance, newBootstrapper);
				
				if (bootstrapper) 
				{
					bootstrapper->RegisterEvent(_T("BootstrapperStarted"));
				}
				if (bootstrapper->isWindowed())
				{
					if (bootstrapper->isSilentMode())
					{
						LLOG_ENTRY(bootstrapper->logger, "CSimpleModule::WinMain Waiting for bootstrapper to finish in silent mode");
						bool result = bootstrapper->WaitForCompletion();
						LLOG_ENTRY1(bootstrapper->logger, "CSimpleModule::WinMain - robloxReady, wait timeout = %d", result);
					}
					else
					{
						LLOG_ENTRY1(bootstrapper->logger, "CSimpleModule::WinMain current thread id = %d", ::GetCurrentThreadId());

						MSG msg;
						HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
						while (GetMessage(&msg, NULL, 0, 0))
						{
							if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
							{
								TranslateMessage(&msg);
								DispatchMessage(&msg);
							}
						}
					}

					// windowed mode runs tasks asynchronously, 
					// throw exceptions here so we can catch anything that happened in worker thread 
					if (bootstrapper->hasUserCancel())
						throw cancel_exception(true);

					if (bootstrapper->hasErrors())
						throw std::exception();
				}

				
				bootstrapper->Close();

				LLOG_ENTRY(bootstrapper->logger, "CSimpleModule::WinMain Bring app to front.");
				HWND hWnd = bootstrapper->getLaunchedAppHwnd();
				if (hWnd)
				{
					::SetFocus(hWnd);
					SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
					SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
					SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				}

				LLOG_ENTRY(bootstrapper->logger, "Left CSimpleModule::WinMain");
			}
			catch (cancel_exception&)
			{
				if (bootstrapper) 
				{
					if (bootstrapper->PerModeLoggingEnabled()) {
						encounteredProblem = true;
						bootstrapper->ReportFinishing("Cancel");
					}
					bootstrapper->RegisterEvent(_T("BootstrapperFinishedCancel"));
				}
				result = -1;
			}
			catch (std::exception& e)
			{
				if (bootstrapper)
				{
					if (bootstrapper->PerModeLoggingEnabled()) {
						encounteredProblem = true;
						bootstrapper->ReportFinishing("Error");
					}
					bootstrapper->error(e);
				
					if (bootstrapper->IsPlayMode()) 
					{
						bootstrapper->RegisterEvent(_T("BootstrapperPlayFinishedFailure"));
					}

					bootstrapper->RegisterEvent(_T("BootstrapperFinishedFailure"));
				}
				else
				{
					CString message;
					message.Format(_T("An error occured and Roblox cannot continue.\n\n%S"), e.what());
					::MessageBox(NULL, message, _T("Error"), MB_OK | MB_ICONEXCLAMATION);
				}
				result = -1;
			}

			if(bootstrapper){
				if (bootstrapper->PerModeLoggingEnabled() && !encounteredProblem) {
					bootstrapper->ReportFinishing("Success");
				}
				if (bootstrapper->hasErrors()){
					bootstrapper->postLogFile(true);
				}
				else if(bootstrapper->hasLongUserCancel()){
					if (((rand() % 10) == 0))
					{
						bootstrapper->postLogFile(false);
					}
				} else {
					if (bootstrapper->IsPlayMode()) 
					{
						bootstrapper->RegisterEvent(_T("BootstrapperPlayFinishedSuccess"));
						bootstrapper->RegisterEvent(_T("BootstrapperPlayFinishedSuccess2")); // TODO: remove
					}
					bootstrapper->RegisterEvent(_T("BootstrapperFinishedSuccess"));
					bootstrapper->RegisterEvent(_T("BootstrapperFinishedSuccess2")); // TODO: remove
					bootstrapper->ReportElapsedEvent(bootstrapper->IsPlayMode());
				}
			}

			if(bootstrapper && bootstrapper->hasReturnValue()){
				result = bootstrapper->ExitCode();
			//CString message;
			//message.Format("FailIfNotUpToDate result = %d\n",result);
			//::MessageBox(NULL, message, "Error", MB_OK | MB_ICONEXCLAMATION);
			}

			if (bootstrapper)
			{
				bootstrapper->FlushEvents();

				// one more time for good measure
				LLOG_ENTRY(bootstrapper->logger, "CSimpleModule::WinMain Bring app to front again.");
				HWND hWnd = bootstrapper->getLaunchedAppHwnd();
				if (hWnd)
				{
					::SetFocus(hWnd);
					if (!SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE))
					{
						LLOG_ENTRY1(bootstrapper->logger, "SetWindowPos TOPMOST failed: %S", (LPCTSTR)GetLastErrorDescription());
					}

					SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
					SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				}
			}
		}
		
#ifdef _DEBUG
		// Prevent false memory leak reporting. ~CAtlWinModule may be too late.
		_AtlWinModule.Term();		
#endif	// _DEBUG

		return result;
	}
};

CSimpleModule module;


int BootstrapperMain(HINSTANCE hInstance, int nCmdShow, Bootstrapper*(*newBootstrapper)(HINSTANCE))
{
	return module.WinMain(hInstance, nCmdShow, newBootstrapper);
}

bool IsWinXP()
{
	OSVERSIONINFO osver = {0};

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	if (::GetVersionEx(&osver) && osver.dwPlatformId == VER_PLATFORM_WIN32_NT && osver.dwMajorVersion == 5)
		return true;

	return false;
}

bool IsWin7()
{
	OSVERSIONINFO osver = {0};

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	if (::GetVersionEx(&osver) && osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1)
		return true;

	return false;
}

#include <atlsecurity.h>

bool IsAdminRunning(void)
{
  ATL::CAccessToken ProcToken;
  ATL::CAccessToken ImpersonationToken;
  ATL::CSid UserSid(Sids::Admins());

  ProcToken.GetEffectiveToken(TOKEN_READ | TOKEN_DUPLICATE);
  ProcToken.CreateImpersonationToken(&ImpersonationToken);
  bool result = false;
  ImpersonationToken.CheckTokenMembership(UserSid, &result);

  return result;
}

Bootstrapper::Bootstrapper(HINSTANCE hInstance)
:hInstance(hInstance),
dialog(CMainDialog::Create(hInstance)),
mainThreadId(::GetCurrentThreadId()),
robloxAppArgs(_T("-browser")),	// By default start RobloxApp.exe as a web browser
force(false),
isUninstall(false),
isNop(false),
requestedInstall(false),
performedNewInstall(false),
performedUpdateInstall(false),
performedBootstrapperUpdateAndQuit(false),
isFailIfNotUpToDate(false),
stage(0),
windowed(true),
editMode(false),
debug(false),
windowDelay(0),
userCancelled(false),
perUser(true),
adminInstallDetected(false),
exitCode(0),
fullCheck(false),
silentMode(false),
dontStartApp(false),
robloxReady(FALSE, FALSE, NULL, NULL),
noRun(false),
waitOnStart(true),
launchedAppHwnd(NULL),
influxdbLottery(0)
{
	startTickCounter = GetTickCount();
	LOG_ENTRY("Bootstrapper::Bootstrapper");
	LOG_ENTRY1("Main threadID %d", mainThreadId);
	
	SYSTEMTIME sysTime = {0};
	GetSystemTime(&sysTime);

	CString strMessage;
	strMessage.Format(_T("%02d-%02d-%d %02d:%02d:%02d"), sysTime.wMonth, sysTime.wDay, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

	LOG_ENTRY1("Start time: %S", strMessage.GetString());

	LogMACaddress();

	srand(GetTickCount());

	reportStatGuid = rand();

	{
		CString foo;
		foo.LoadString(IDS_INSTALLHOST);
		installHost = convert_w2s(foo.GetString());
		LOG_ENTRY1("installHost: %s", installHost.c_str());
	}
	{
		CString foo;
		foo.LoadString(IDS_BASEHOST);
		baseHost = convert_w2s(foo.GetString());
		LOG_ENTRY1("baseHost: %s", baseHost.c_str());
	}
}

void Bootstrapper::SetupInfluxDb()
{
	// throttle
	influxdbLottery = rand() % 10000;

	InfluxDb::init(convert_w2s(GetBootstrapperFileName()), InfluxDbUrl(), InfluxDbDatabase(), InfluxDbUser(), InfluxDbPassword());

	TCHAR buffer[256];
	GEOID id = GetUserGeoID(GEOCLASS_NATION);
	if (id != GEOID_NOT_AVAILABLE)
		GetGeoInfo(id, GEO_FRIENDLYNAME,  buffer, 256, 0);
	char loc[256];
	size_t outSize;
	wcstombs_s(&outSize, loc, 256, buffer, 256);
	InfluxDb::setLocation((id != GEOID_NOT_AVAILABLE) ? loc : "");

	CVersionInfo vi;
	vi.Load(_AtlBaseModule.m_hInst);
	std::string v = vi.GetFileVersionAsDotString();
	InfluxDb::setAppVersion(v.c_str());
}

void Bootstrapper::ReportToInfluxDbImpl(const std::string& json)
{
	if (InfluxDb::getUrlHost().empty() || InfluxDb::getUrlPath().empty())
		return;

	try
	{
		std::ostrstream result;
		HttpTools::httpPost(this, InfluxDb::getUrlHost(), InfluxDb::getUrlPath(), std::stringstream(json), "application/json",
			result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2), false);
	}
	catch(std::exception &e)
	{
		LOG_ENTRY1("Error in reporting influx: %s", e.what());
	}
}
void Bootstrapper::ReportToInfluxDb(int throttle, const std::string& json, bool blocking)
{
	if (influxdbLottery >= throttle)
		return;

	if (!blocking)
		boost::thread(boost::bind(&Bootstrapper::ReportToInfluxDbImpl, this, json));
	else
		ReportToInfluxDbImpl(json);
}

void Bootstrapper::ReportInstallMetricsToInfluxDb(bool isUpdate, const std::string& result, const std::string& message)
{
	InfluxDb points;
	points.addPoint("Type", isUpdate ? "Update" : "Install");
	points.addPoint("Result", result.c_str());
	points.addPoint("Message", message.c_str());
	points.addPoint("ElapsedTime", (GetElapsedTime() / 1000.0f));
	points.addPoint("BytesDownloaded", (int)deployer->GetBytesDownloaded());
	ReportToInfluxDb(InfluxDbInstallHundredthsPercentage(), points.getJsonStringForPosting("PCInstall"), false);
}

DWORD Bootstrapper::GetElapsedTime()
{
	return (GetTickCount() - startTickCounter);
}

void Bootstrapper::ReportElapsedEvent(bool startingGameClient)
{
	DWORD elapsedTime = GetElapsedTime();
	std::wstring eventName;
	std::wstring prefix;
	std::wstring timeDelta;
	if (startingGameClient)
	{
		prefix = _T("Play");
	}

	if (elapsedTime < 5000) 
	{
		timeDelta = _T("0_5");
	} 
	else if (elapsedTime >= 5000 && elapsedTime < 15000) 
	{
		timeDelta = _T("5_15");
	} 
	else 
	{
		timeDelta = _T("15_INF");
	}

	eventName = format_string(_T("Bootstrapper%sRun%s"), prefix.c_str(), timeDelta.c_str());
	RegisterEvent(eventName.c_str());
}

void Bootstrapper::reportDurationAndSizeEvent(const char* category, const char* result, DWORD duration, DWORD size) {
	try {

		// TODO: Remove calls to JoinRate.ashx
		std::string handlerUrlPart = format_string(
			"/Game/JoinRate.ashx?c=%s&r=%s&d=%ld&platform=Win32",
			category, result, duration);
		if (size > 0) {
			handlerUrlPart += format_string("&b=%ld", size);
		}

		std::ostrstream result;
		HttpTools::httpGet(this, baseHost, handlerUrlPart, std::string(),
			result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		////////////////////////////////


		// new stats reporting api
		char* apiPathFormatStr = "/game/report-stats?name=%s&value=%ld";
		
		std::string durationName = format_string("%s%s_Duration", category, result);
		HttpTools::httpPost(this, baseHost, format_string(apiPathFormatStr, durationName.c_str(), duration), std::stringstream(""), "*/*",
			result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));

		if (size > 0)
		{
			std::string sizeName = format_string("%s%s_Size", category, result);
			HttpTools::httpPost(this, baseHost, format_string(apiPathFormatStr, sizeName.c_str(), size), std::stringstream(""), "*/*",
				result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		}

	} catch (const std::exception& e) {
		LOG_ENTRY1("Error in reporting: %s", e.what());
	}
}

void Bootstrapper::reportFinishingHelper(const char* category, const char* result) {
	// Use bootstrapper type to differentiate reports by pre-pending Type().
	std::string builtCategory(Type());
	builtCategory += category;
	reportDurationAndSizeEvent(builtCategory.c_str(), result, GetElapsedTime(), 0/*no size metric*/);

	// Add result to category for ephemeral counter
	builtCategory += result;
	RegisterEvent(convert_s2w(builtCategory).c_str());

	// Report to influxdb
	influxDb.addPoint("Category", category);
	influxDb.addPoint("Result", result);
	influxDb.addPoint("Duration", GetElapsedTime() / 1000.0f); // report duration in seconds
	ReportToInfluxDb(InfluxDbReportHundredthsPercentage(), influxDb.getJsonStringForPosting("BootstrapperDuration"), true);

	// log client failure by cdn
	if (result != "Success" && Type() == "Client")
	{
		std::string installUrl;
		if (UseNewCdn())
			installUrl = HttpTools::getCdnHost(this);
		if (installUrl.empty())
			installUrl = HttpTools::getPrimaryCdnHost(this);

		std::string action(category);
		action += result;
		GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, action.c_str(), installUrl.empty() ? installHost.c_str() : installUrl.c_str());
	}
}

void Bootstrapper::ReportFinishing(const char* result) {
	LOG_ENTRY1("Doing final report with result = %s", result);
	
	influxDb.addPoint("ReportFinishing", GetElapsedTime() / 1000.0f);

	if (requestedInstall) {
		if (performedBootstrapperUpdateAndQuit) {
			reportFinishingHelper("BootstrapperUpdateSelf", result);
		} else {
			// Don't pollute "BootstrapperUpdate*" and "BootstrapperInstall*"
			// ephemeral counter namespaces
			if (performedUpdateInstall) {
				reportFinishingHelper("BootstrapperDetailedUpdate", result);
			} else if (performedNewInstall) {
				reportFinishingHelper("BootstrapperDetailedInstall", result);
			} else {
				reportFinishingHelper("BootstrapperDetailedUnknownInstallOrUpdate", result);
			}
		}
	}
	if (isFailIfNotUpToDate) {
		if (exitCode != 0) {
			reportFinishingHelper("BootstrapperFailIfNotUpToDateNonZeroExit", result);
		} else {
			reportFinishingHelper("BootstrapperFailIfNotUpToDateZeroExit", result);
		}
	}
	if (isNop) {
		reportFinishingHelper("BootstrapperPrePlay", result);
	}
	if (IsPlayMode()) {
		// don't pollute "BootstrapperPlay*" ephermeral counter namespace
		reportFinishingHelper("BootstrapperDetailedPlay", result);
	}
	reportFinishingHelper("BootstrapperDetailed", result);
}

void Bootstrapper::LogMACaddress(void)
{
	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen = sizeof(AdapterInfo);

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	assert(dwStatus == ERROR_SUCCESS);

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	do {
		if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET)
		{
			std::string str = format_string("%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2], pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5], pAdapterInfo->Address[6], pAdapterInfo->Address[7]);
			LOG_ENTRY2("MAC: %s Name: %s", str.c_str(), pAdapterInfo->Description);
		}
		pAdapterInfo = pAdapterInfo->Next;
	} while(pAdapterInfo);
}

void Bootstrapper::initialize()
{	
	LOG_ENTRY("Bootstrapper::initialize");

	try
	{
		createDialog();

		dialog->closeCallback = boost::bind(&Bootstrapper::userCancel, this);
        dialog->setTitle(GetFriendlyName());

		parseCmdLine();

		bool result = ComModule::Init(perUser);
		if (result != perUser)
		{
			LOG_ENTRY("::GetProcAddress(hmodOleAut, \"RegisterTypeLibForUser\") FAILED, install farced to global mode");
		}
		perUser = result;
		
		LOG_ENTRY1("IsVistaPlus = %d", IsVistaPlus());
		if (IsVistaPlus())
		{
			LOG_ENTRY1("IsElevated = %d", IsElevated());
			LOG_ENTRY1("IsUacEnabled = %d", IsUacEnabled());
		}
		else
		{
			LOG_ENTRY1("IsWinXP = %d", IsWinXP());
			if (IsWinXP())
			{
				LOG_ENTRY1("IsAdminRunning = %d", IsAdminRunning());
			}
		}

		if (perUser)
		{
			if (debug)
			{
				dialog->MessageBox(IsVistaPlus() ? _T("This is Vista") : _T("This is not Vista"), _T("Debug"), MB_OK);
				if (IsVistaPlus())
				{
					dialog->MessageBox(IsElevated() ? _T("This is Elevated") : _T("This is not Elevated"), _T("Debug"), MB_OK);
					if (IsElevated())
						perUser = dialog->MessageBox(_T("Do you want an all-user install?"), _T("DEBUG"), MB_YESNO)==IDNO;
				}
			}
			else
			{
				if (IsVistaPlus() && IsElevated())
				{
					perUser = false;
				}
			}
		}

		LOG_ENTRY1("perUser = %d", perUser);
		deployer.reset(new FileDeployer(this, perUser));
		LOG_ENTRY("Bootstrapper::initialize - calling loadPrevVersions");
		loadPrevVersions();

		std::string installedVersion = queryInstalledVersion();
		if (installedVersion.empty())
		{
			LOG_ENTRY("isUpdating = false");
		}
		else
		{
			LOG_ENTRY1("isUpdating = true, %s", installedVersion.c_str());
			if (IsPlayMode() && HasUnhideGuid())
			{
				silentMode = true;
			}
		}

		if ((!windowed && dontStartApp) || ForceNoDialogMode()) 
		{
			silentMode = true;
		}

		dialog->SetSilentMode(silentMode);

		if (!perUser)
			if (IsVistaPlus())
				if (!IsElevated())
				{
					if(IsUacEnabled())
					{
						LOG_ENTRY("re-launching elevated");
						// Uh-oh. We're want to do something elevated but we aren't elevated
						std::wstring args;
						for (int i = 1; i < __argc; i++)
						{
							if (!args.empty())
								args += _T(" ");
							args += _T('\"');
							args += __targv[i];
							args += _T('\"');
						}

						TCHAR bootstrapperFile[_MAX_PATH];
						::GetModuleFileName(NULL, bootstrapperFile, _MAX_PATH);

						SHELLEXECUTEINFO TempInfo = {0};

						TempInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						TempInfo.fMask = 0;
						TempInfo.hwnd = NULL;
						TempInfo.lpVerb = _T("runas");
						TempInfo.lpFile = bootstrapperFile;
						TempInfo.lpParameters = args.c_str();
						TempInfo.lpDirectory = NULL;
						TempInfo.nShow = SW_NORMAL;

						if (::ShellExecuteEx(&TempInfo))
						{
							LOG_ENTRY("ShellExecuteEx succeeded");
						}
						else
						{
							LOG_ENTRY1("ShellExecuteEx failed: %S", (LPCTSTR)GetLastErrorDescription());
						}
						throw installer_error_exception(installer_error_exception::VistaElevation);
					}
					else{
						if (windowed)
							dialog->DisplayError("This operation requires administrator priveleges", NULL);

						throw installer_error_exception(installer_error_exception::AdminAccountRequiredWithoutUac);
					}
				}

		
		throwHRESULT(classesKey.Create(perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\Classes")), "Failed to create HK**\\Software\\Classes");
		LOG_ENTRY1("classesKey = %s\\Software\\Classes", (perUser ? "HKEY_CURRENT_USER" : "HKEY_LOCAL_MACHINE"));

		if (CookiesEngine::getCookiesFilePath().empty())
		{
			std::wstring dir = FileSystem::getSpecialFolder(FileSystem::RobloxUserApplicationDataLow, true, NULL, NULL);
			// lets make sure that we have roblox dir at this point
			try 
			{
				createDirectory(dir.c_str());
				CookiesEngine::setCookiesFilePath(dir + _T("rbxcsettings.rbx"));
			}
			catch (...)
			{
				// we should not fail the game if our reproting is broken somehow
				LOG_ENTRY("Failure during cookie path initialization");
			}
		}
	}
	catch( CAtlException e )
	{
		error(std::runtime_error(format_string("AtlException 0x%x", e.m_hr)));
		throw;
	}
	catch (silent_exception&)
	{
		throw;
	}
	catch (std::exception& e)
	{
		error(e);
		throw;
	}
}

Bootstrapper::~Bootstrapper(void)
{
}

void Bootstrapper::postData(std::fstream &data)
{
	std::strstream result;

	CVersionInfo vi;
	vi.Load(_AtlBaseModule.m_hInst);
	std::string v = vi.GetFileVersionAsDotString();

	std::string url = format_string("/Error/InstallLog.ashx?version=%s&stage=%02d&guid=%d", v.c_str(), stage, reportStatGuid);
	HttpTools::httpPost(this, GetUseDataDomain() ? ReplaceTopSubdomain(baseHost, "data") : baseHost, url, data, "text/plain", result, true, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
	result << (char)0;
	LOG_ENTRY1("Uploading log file result: %s", result.str());
}

static bool CmpFileTime(const WIN32_FIND_DATA &left, const WIN32_FIND_DATA &right)
{
	return (CompareFileTime(&right.ftCreationTime, &left.ftCreationTime) == -1);
}

void Bootstrapper::postLogFile(bool uploadApp)
{
	LOG_ENTRY("Posting log file");
	try
	{
		const char *corrID = "CorrIDString";
		if (uploadApp)
		{
			LOG_ENTRY2("%s: %d", corrID, reportStatGuid);
		}

		try
		{
			if (uploadApp)
			{
				std::wstring searchPattern(simple_logger<wchar_t>::get_tmp_path());
				searchPattern += _T("*.rbal");

				WIN32_FIND_DATA findFileData = {};  
				HANDLE handle = INVALID_HANDLE_VALUE;
				handle = FindFirstFile(searchPattern.c_str(), &findFileData);
				std::vector<WIN32_FIND_DATA> files;

				if(handle  != INVALID_HANDLE_VALUE)
				{
					do
					{
						if(findFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
						{
							files.push_back(findFileData);
						}
					}
					while(FindNextFile(handle, &findFileData));
				}
				FindClose(handle);

				if (!files.empty())
				{
					std::sort(files.begin(), files.end(), CmpFileTime);
					std::wstring file = simple_logger<wchar_t>::get_tmp_path() + std::wstring(files[0].cFileName);
					{
						std::fstream data(file.c_str(), std::ios_base::app);
						data << "Info: RobloxAppLog" << "\n";
						data << corrID << ": " << reportStatGuid << "\n";
						data.flush();
					}
					
					std::fstream data(file.c_str(), std::ios_base::in | std::ios_base::binary);
					LOG_ENTRY("Uploading app log file");
					postData(data);
				}
			}
		}
		catch (std::exception &e)
		{
			LOG_ENTRY1("Exception during app log upload %s", e.what());
		}

		LOG_ENTRY("Uploading install log file");
		std::fstream data(logger.log_filename().c_str(), std::ios_base::in | std::ios_base::binary);
		postData(data);
	}
	catch (std::exception& e)
	{
		LOG_ENTRY1("Error uploading log file: %s", exceptionToString(e).c_str());
	}
}

void Bootstrapper::parseCmdLine()
{
	LOG_ENTRY1("Bootstrapper::parseCmdLine params counter = %d", __argc);
	if(__argc == 1){
		fullCheck = true;
	}

	// check if cmd comes from protocol handler
	wchar_t** args = __targv;
	const wchar_t* protocolScheme = GetProtocolHandlerUrlScheme();
	int protocolSchemeLen = lstrlen(protocolScheme);
	if (!StrNCmp(args[1], protocolScheme, protocolSchemeLen))
	{
		// split args by +
		std::vector<std::wstring> argList = splitOn(args[1], _T('+'));

		// create arg map so we can find values by name
		std::map<std::wstring, std::wstring> argMap;
		for (unsigned int i = 0; i < argList.size(); i++)
		{
			int separatorPos = argList[i].find(_T(":"));
			if (separatorPos != std::string::npos)
			{
				std::wstring argName = argList[i].substr(0, separatorPos);
				std::transform(argName.begin(), argName.end(), argName.begin(), tolower);
				std::wstring argValue = argList[i].substr(separatorPos + 1, std::wstring::npos);
				argMap.insert(std::make_pair<std::wstring, std::wstring>(argName, argValue));
			}
			else
			{
				std::wstring argName = argList[i];
				std::transform(argName.begin(), argName.end(), argName.begin(), tolower);
				argMap.insert(std::make_pair<std::wstring, std::wstring>(argName, _T("")));
			}
		}

		// set browser tracking id
		std::map<std::wstring, std::wstring>::const_iterator iter = argMap.find(_T("browsertrackerid"));
		if (iter != argMap.end())
		{
			browserTrackerId = convert_w2s(iter->second);
			LOG_ENTRY1("Bootstrapper::parseCmdLine: browserTracerId, value = %s", browserTrackerId.c_str());
		}

		iter = argMap.find(_T("launchtime"));
		if (iter != argMap.end())
		{
			int64_t lt = boost::lexical_cast<int64_t>(iter->second);
			
			boost::posix_time::ptime const epoch(boost::gregorian::date(1970, 1, 1));
			int64_t ct = (boost::posix_time::microsec_clock::universal_time() - epoch).total_milliseconds();

			influxDb.addPoint("LaunchTime", (ct-lt)/1000.0f);
		}

		ProcessProtocolHandlerArgs(argMap);
		influxDb.addPoint("ProtocolHandler", true);
		return;
	}

	influxDb.addPoint("ProtocolHandler", false);
	for (int i=1; i<__argc; ++i)
	{
		TCHAR* arg1 = __targv[i];
		LOG_ENTRY2("Bootstrapper::parseCmdLine paramIndex = %d, value = %S", i, arg1);

		if (CAtlModule::WordCmpI(arg1, _T("-wait"))==0) // wait before processing more commands
        {
            if ( i + 1 >= __argc )
                throw std::runtime_error(format_string("Bad argument %s", arg1));

            int sleepTime = _wtoi(__targv[i + 1]);
            ::Sleep(sleepTime);
            i++; // skip time parameter
        }
        else if (CAtlModule::WordCmpI(arg1, _T("-ide"))==0)
		{
			LOG_ENTRY("option: ide");
			windowDelay = 1;
            robloxAppArgs = convert_s2w(SharedLauncher::IDEArgument);
			if (i+1 < __argc && __targv[i+1][0]!='-')
			{
				++i;
			
				// Check to see if the next parameter is a file location
				WIN32_FIND_DATA FileInformation;             // File information
				HANDLE hFile = ::FindFirstFile(__targv[i], &FileInformation);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					robloxAppArgs += _T(" ");
					robloxAppArgs += convert_s2w(SharedLauncher::FileLocationArgument);
					robloxAppArgs += _T(" \"");
					robloxAppArgs += __targv[i];
					robloxAppArgs += _T("\"");
				}
			}
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-q"))==0)
		{
			LOG_ENTRY("option: q");
			windowed = false;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-qbg"))==0)
		{
			LOG_ENTRY("option: qbg");
			windowed = false;
			dontStartApp = true;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-qbgnorun"))==0)
		{
			LOG_ENTRY("option: qbgnorun");
			windowed = false;
			dontStartApp = true;
			noRun = true;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-alluser"))==0)
		{
			LOG_ENTRY("option: alluser");
			perUser = false;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-browser"))==0)
		{
			LOG_ENTRY("option: browser");
			windowDelay = 1;
			robloxAppArgs = _T("-browser");
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-uninstall"))==0)
		{
			LOG_ENTRY("option: Uninstall");
			isUninstall = true;
			robloxAppArgs = _T("");
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-install"))==0)
		{
			LOG_ENTRY("option: Install");
			requestedInstall = true;
			// Just update/install roblox, but don't run it
			robloxAppArgs = _T("");
			windowDelay = 10;
		}
		else if(CAtlModule::WordCmpI(arg1, _T("-failIfNotUpToDate"))==0)
		{
			LOG_ENTRY("option: FailIfNotUpToDate");
			//Just check roblox version number, and return "success (0)" if it is up to date
			isFailIfNotUpToDate = true;
			robloxAppArgs = _T("");
			windowDelay = 10;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-force"))==0)
		{
			LOG_ENTRY("option: force");
			force = true;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-debug"))==0)
		{
			LOG_ENTRY("option: debug");
			debug = true;
		}
		else if (CAtlModule::WordCmpI(arg1, _T("-prePlay"))==0)
		{
			LOG_ENTRY("option: prePlay");
			robloxAppArgs = _T("-nop");
			isNop = true;
		}
		else if (!ProcessArg(__targv, i, __argc))
        {
            if ( !robloxAppArgs.empty() )
			    robloxAppArgs += _T(" ");
            robloxAppArgs += arg1;
		}
	}
}

void Bootstrapper::Close()
{
	dialog->CloseDialog();
}

bool Bootstrapper::hasSse2()
{
    const unsigned int kCpuVendorCmd = 0;
    const unsigned int kCpuFeatures = 1;
    const unsigned int kSse2BitLoc = 26;

    // Determine if the feature register is available.  I have not idea how this
    // would fail on any computer built in the last 15 years.
    unsigned int highestFunction = 0;
    __asm {
        mov	  eax, kCpuVendorCmd
        mov   ecx, 0
        cpuid               
        mov   highestFunction, eax
    }
    if (highestFunction < kCpuFeatures)
    {
        return false;
    }

    // Check for SSE2 Support
    unsigned int featureBits;
    __asm {
        mov	  eax, kCpuFeatures
        mov   ecx, 0
        cpuid               
        mov   featureBits, edx
    }
    return ( featureBits & (1 << kSse2BitLoc));
}

boost::shared_ptr<Bootstrapper> Bootstrapper::Create(HINSTANCE hInstance, Bootstrapper*(*newBootstrapper)(HINSTANCE))
{
	boost::shared_ptr<Bootstrapper> result(newBootstrapper(hInstance));

	result->initialize();

	// Now schedule the async tasks
	if (!result->windowed)
	{
		if (!result->noRun)
		{
			if (!result->CanRunBgTask())
			{
				return result;
			}

			CMutex m1(NULL, FALSE, result->GetStartAppMutexName());
			CTimedMutexLock l1(m1);
			if (l1.Lock(1) == WAIT_TIMEOUT)
			{
				LLOG_ENTRY(result->logger, "No bg update, Roblox App is running");
				return result;
			}

			CMutex m2(NULL, FALSE, result->GetBootstrapperMutexName());
			CTimedMutexLock l2(m2);
			if (l2.Lock(1) == WAIT_TIMEOUT)
			{
				LLOG_ENTRY(result->logger, "No bg update, Bootstrapper is running");
				return result;
			}

			LLOG_ENTRY(result->logger, "Running bootstrapper in sync mode");
			result->run();		// do it synchronously
		} 
		else 
		{
			result->LoadSettings();
			result->SetupGoogleAnalytics();
			result->SetupInfluxDb();

		}

		result->RunPreDeploy();
	}
	else
	{
		boost::thread(boost::bind(&Bootstrapper::run, result));
		if (result->windowDelay)
		{
			boost::thread(boost::bind(&Bootstrapper::showWindowAfterDelay, result));
		}
		else
		{
			if (result->isUninstall)
			{
				LLOG_ENTRY1(result->logger, "Bootstrapper::Create delay showing dialog window due to uninstall, thread id = %d", ::GetCurrentThreadId());
				result->dialog->ShowWindow(CMainDialog::WindowTimeDelayShow);
			}
			else
			{
				LLOG_ENTRY1(result->logger, "Bootstrapper::Create showing dialog window, thread id = %d", ::GetCurrentThreadId());
				result->dialog->ShowWindow(CMainDialog::WindowSomethingInterestingShow);
			}
		}
	}

	return result;
}

// Downloads a file from the install site. Uses a cached file when possible
void Bootstrapper::message(const std::string& message)
{
	LOG_ENTRY1("UI Message: %s", message.c_str());
	if(dialog)
		dialog->SetMessage(message.c_str());
}

// http://blogs.msdn.com/ie/archive/2007/06/13/new-api-smoothes-extension-development-in-protected-mode.aspx
UINT __stdcall RefreshPolicies()
{
    UINT hr = ERROR_SUCCESS;
    HMODULE hDll = LoadLibrary(_T("ieframe.dll"));
    if (NULL != hDll)
    {
        typedef HRESULT (*PFNIEREFRESHELEVATIONPOLICY)();
        PFNIEREFRESHELEVATIONPOLICY pfnIERefreshElePol = (PFNIEREFRESHELEVATIONPOLICY) GetProcAddress(hDll, "IERefreshElevationPolicy");
        if (pfnIERefreshElePol)
        {
            hr = pfnIERefreshElePol();
        } else {
             DWORD error = GetLastError(); 
             hr = HRESULT_FROM_WIN32(error);
         }
        FreeLibrary(hDll);
    } else {
       DWORD error = GetLastError(); 
       hr = HRESULT_FROM_WIN32(error);
    }
    return hr;
}

void Bootstrapper::RegisterElevationPolicy(const TCHAR* appName, const TCHAR* appPath)
{
	std::string guid;
	{
		GUID g;
		::CoCreateGuid(&g);
		OLECHAR s[256];
		::StringFromGUID2(g, s, 256);
		guid = convert_w2s((LPCTSTR)CString(s));
	}

	std::wstring skey = format_string(_T("Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\%S"), guid.c_str());

	LOG_ENTRY1("RegisterElevationPolicy: %S", skey.c_str());
	CRegKey key;
	throwHRESULT(key.Create(perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, skey.c_str()), format_string("Failed to create key for %s elevation policy", appName));
	throwHRESULT(key.SetStringValue(_T("AppName"), appName), format_string("Failed to create AppName for %s elevation policy", appName));
	throwHRESULT(key.SetDWORDValue(_T("Policy"), 3), format_string("Failed to create Policy for %s elevation policy", appName));
	throwHRESULT(key.SetStringValue(_T("AppPath"), appPath), format_string("Failed to create AppPath for %s elevation policy", appName));
	key.Close();
	
	HRESULT hr = RefreshPolicies();
	if (FAILED(hr))
	{
		LOG_ENTRY1("WARNING: RefreshPolicies failed - HR=0x%X", hr);
	}
}

bool Bootstrapper::isInstalled(std::wstring productKey)
{
	CRegKey key;
	LONG lRes = key.Open(perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, productKey.c_str(), KEY_READ);
	return (lRes == 0);
}

void Bootstrapper::RegisterUninstall(const TCHAR *productName)
{
	// http://msdn.microsoft.com/en-us/library/aa372105(VS.85).aspx

	// Register Add/Remove Programs registry
	CRegKey keyProductCode = CreateKey(perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, GetProductCodeKey().c_str(), NULL);

	std::wstring uninstallString = format_string(_T("\"%s%s\" -uninstall%s"), programDirectory().c_str(), GetBootstrapperFileName().c_str(), perUser ? _T("") : _T(" -alluser"));
	throwHRESULT (keyProductCode.SetStringValue(_T("UninstallString"), uninstallString.c_str(), REG_EXPAND_SZ), "Failed to set UninstallString key");
	throwHRESULT (keyProductCode.SetStringValue(_T("Publisher"), _T("ROBLOX Corporation")), "Failed to set Publisher key");
	throwHRESULT (keyProductCode.SetStringValue(_T("URLInfoAbout"), _T("http://www.roblox.com")), "Failed to set URLInfoAbout key");
	throwHRESULT (keyProductCode.SetStringValue(_T("Comments"), convert_s2w(installVersion).c_str()), "Failed to set Comments key");
	throwHRESULT (keyProductCode.SetStringValue(_T("InstallLocation"), programDirectory().c_str()), "Failed to set InstallLocation key");
	throwHRESULT (keyProductCode.SetDWORDValue(_T("NoModify"), 1), "Failed to set NoModify key");
	throwHRESULT (keyProductCode.SetDWORDValue(_T("NoRepair"), 1), "Failed to set NoRepair key");
	std::wstring iconPath = format_string(_T("%s%s,0"), programDirectory().c_str(), GetBootstrapperFileName().c_str());
	throwHRESULT (keyProductCode.SetStringValue(_T("DisplayIcon"), iconPath.c_str()), "Failed to set IconPath key");

	SYSTEMTIME lt;
    GetLocalTime(&lt);
	throwHRESULT (keyProductCode.SetStringValue(_T("InstallDate"), format_string(_T("%04d%02d%02d"), lt.wYear, lt.wMonth, lt.wDay).c_str()), "Failed to set InstallDate key");

	std::wstring name = productName;
	if (perUser)
	{
		wchar_t buff[256];
		buff[0] = 0;
		DWORD len = 256;
		::GetUserNameW(buff, &len);
		name += L" for ";
		name += buff;
	}
	throwHRESULT (keyProductCode.SetStringValue(_T("DisplayName"), name.c_str()), "Failed to set DisplayName key");
}

void Bootstrapper::RegisterProtocolHandler(const std::wstring& protocolScheme, const std::wstring& exePath, const std::wstring& installKeyRegPath)
{
	if (protocolScheme.length() == 0)
		return;

	// query and remove what's already installed
	CRegKey installKey;
	if (ERROR_SUCCESS==installKey.Open(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, installKeyRegPath.c_str(), KEY_READ | KEY_WRITE))
	{
		std::string result;
		result = QueryStringValue(installKey, _T("protocol handler scheme"));
		LOG_ENTRY2("HKCU\\%S :protocol handler scheme = %s", GetRegistryPath().c_str(), result.c_str());

		if (!result.empty())
		{
			UnregisterProtocolHandler(convert_s2w(result));
		}
	}

	// register the protocol handler scheme
	CRegKey key = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, (_T("SOFTWARE\\Classes\\") + protocolScheme).c_str());
	throwHRESULT(key.SetStringValue(_T(""), _T("URL: Roblox Protocol")), format_string("failed to set value for protocol"));
	throwHRESULT(key.SetStringValue(_T("URL Protocol"), _T("")), format_string("failed to set value for protocol"));

	CreateKey(key, _T("DefaultIcon"), exePath.c_str());

	std::wstring value = format_string(_T("\"%s\" %%1"), exePath.c_str());
	CreateKey(key, _T("shell\\open\\command"), value.c_str());

	// update value in install key
	throwHRESULT(installKey.SetStringValue(_T("protocol handler scheme"), protocolScheme.c_str()), "Failed to set protocol handler scheme key value");

	// Do not warn the End User to launch with protocol for IE & Edge Browsers.
	if (CreateEdgeRegistry())
	{
		key = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, (_T("SOFTWARE\\Microsoft\\Internet Explorer\\ProtocolExecute\\") + protocolScheme).c_str());
		throwHRESULT(key.SetDWORDValue(_T("WarnOnOpen"), 0), format_string("Failed to set Protocol WarnOnOpen Key"));
	}
	
	// Really is the above key causing issues, delete it
	if (DeleteEdgeRegistry())
		DeleteKey(logger, isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, (_T("SOFTWARE\\Microsoft\\Internet Explorer\\ProtocolExecute\\") + protocolScheme).c_str());
}

void Bootstrapper::UnregisterProtocolHandler(const std::wstring& protocolScheme)
{
	CRegKey hk(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
	std::wstring path = _T("SOFTWARE\\Classes\\");
	path += protocolScheme;
	hk.RecurseDeleteKey(path.c_str());
}

void Bootstrapper::validateAndFixChromeState()
{
	// open Chrome Local State file
	rapidjson::Document doc;
	std::wstring folder = FileSystem::getSpecialFolder(FileSystem::AppData, false, NULL, NULL);
	folder += _T("\\Google\\Chrome\\User Data");

	// parse file
	{
		std::ifstream ifs(convert_w2s(folder + _T("\\Local State")).c_str());
		if (!ifs.is_open())
			return;

		std::stringstream localStateStr;
		localStateStr << ifs.rdbuf();

		doc.Parse<rapidjson::kParseDefaultFlags>(localStateStr.str().c_str());
	}

	rapidjson::Value& ph = doc["protocol_handler"];
	rapidjson::Value& es = ph["excluded_schemes"];

	bool found = false;

	for (rapidjson::Value::MemberIterator  iter = es.MemberBegin(); iter != es.MemberEnd(); ++iter)
	{
		if (iter->name.IsString())
		{
			std::string name = iter->name.GetString();
			if (name.find("roblox") != std::string::npos)
			{
				if (iter->value.GetBool())
					found = true;

				iter->value.SetBool(false);
			}
		}
	}


	if (found && GetLogChromeProtocolFix())
		GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "ChromeProtocolHandler", "Blocked");

	// try detect chrome and prompt shutdown if detected
	bool terminate = false;
	bool retried = false;
	while (found)
	{
		found = false;
		DWORD process_id_array[1024]; 
		DWORD bytes_returned; 
		EnumProcesses(process_id_array, 1024*sizeof(DWORD), &bytes_returned); 

		const DWORD num_processes = bytes_returned/sizeof(DWORD); 
		for(DWORD i=0; i<num_processes; i++) 
		{ 
			DWORD desiredAccess = PROCESS_QUERY_INFORMATION;
			if (terminate)
				desiredAccess |= PROCESS_TERMINATE;

			CHandle hProcess(OpenProcess( desiredAccess, FALSE, process_id_array[i] ));
			TCHAR image_name[1024]; 
			if(GetProcessImageFileName(hProcess,image_name,1024 )) 
			{ 
				image_name[1023] = 0;
				std::wstring s = image_name;
				if (s.find(_T("chrome.exe")) != std::wstring::npos)
				{
					if (terminate)
					{
						UINT exitCode = 0;
						TerminateProcess(hProcess, exitCode);
					}
					else
					{
						found = true;
						break;
					}
				}
			}
		}

		if (found)
		{
			if (!retried)
			{
				if (IDCANCEL == dialog->MessageBox(_T("Please close all instances of Google Chrome before continuing."), _T(""), MB_RETRYCANCEL))
					retried = true;
			}

			if (retried)
			{
				int res = dialog->MessageBox(
					_T("Roblox might not launch correctly if Chrome is open or running in the background during installation.\n\n")
					_T("Automatically shutdown all instances of Chrome and continue?"), _T("Warning"), MB_YESNOCANCEL);

				if (res == IDYES)
				{
					terminate = true;

					if (GetLogChromeProtocolFix())
						GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "ChromeProtocolHandler", "AutoTerminate");
				}
				else if (res == IDNO)
				{
					retried = false;
					continue;
				}
				else if (res == IDCANCEL)
				{
					if (GetLogChromeProtocolFix())
						GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "ChromeProtocolHandler", "BlockedAndCanceled");
					break;
				}
			}

			retried = true;
		}
	}

	// save updated file
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	const char* output = buffer.GetString();

	std::ofstream ofs(convert_w2s(folder + _T("\\Local State")).c_str());
	if (ofs.is_open())
	{
		ofs << output;
	}
}

std::wstring Bootstrapper::GetSelfName() const
{
	return programDirectory() + GetBootstrapperFileName();
}

void Bootstrapper::deploySelf(bool commitData)
{
	LOG_ENTRY("deploySelf");
	if (!commitData)
	{
		return;
	}

	// TODO: Worry about errors? (If so, then check to see if we are trying to copy to ourselves)
	std::wstring destFile = GetSelfName();
	LOG_ENTRY1("Bootstrapper::deploySelf - Final file path=%S", destFile.c_str());
	
	// Copy the file by hand
	// (If we use CopyFile then when IE7 is installed the file won't be trusted)
	if (!::PathFileExists(destFile.c_str()))
	{
		LOG_ENTRY("!PathFileExists");
		wchar_t srcFileLong[_MAX_PATH];
		throwLastError(::GetModuleFileNameW(NULL, srcFileLong, _MAX_PATH),"Error when getting bootstrapper filename for selfDeploy");

		std::wstring srcFile = srcFileLong;
		LOG_ENTRY1("GetModuleFileName: %S", srcFile.c_str());

		copyFile(logger, srcFile.c_str(), destFile.c_str());
		LOG_ENTRY2("Copy: from  '%S' to '%S'", srcFile.c_str(), destFile.c_str());
	}
	else
	{
		LOG_ENTRY("PathFileExists");
	}

	std::wstring dir = programDirectory();
	RegisterElevationPolicy(GetBootstrapperFileName().c_str(), dir.c_str());
}


void Bootstrapper::createDirectory(const TCHAR* dir)
{
	if (!::CreateDirectory(dir, 0))
	{
		DWORD error = ::GetLastError();
		if (error!=ERROR_ALREADY_EXISTS)
			throwLastError(FALSE, format_string("CreateDirectory '%S' failed", dir)); 
	}
}

void Bootstrapper::DoDeployComponents(const std::vector<std::pair<std::wstring, std::wstring> > &files, bool isUpdating, bool commitData)
{
    std::string text = std::string(isUpdating ? "Upgrading " : "Installing ") + convert_w2s(GetFriendlyName()) + " ...";
	message(text);
	progressBar(0, 100);

	if (PerModeLoggingEnabled()) 
	{	
		bool gotCdn = false;
		if (UseNewCdn())
		{
			if (!HttpTools::getCdnHost(this).empty())
				gotCdn = true;
			else if (!HttpTools::getPrimaryCdnHost(this).empty())
				gotCdn = true;
		}
		else
			gotCdn = !HttpTools::getPrimaryCdnHost(this).empty();

		if (!gotCdn) {
			RegisterEvent(_T("BootstrapperUpdateOrInstallUnableToResolveCdnHost"));
		} else {
			RegisterEvent(_T("BootstrapperUpdateOrInstallResolvedCdnHost"));
		}
	}

	DWORD startTickCount = GetTickCount();

	// Create a single array of all zip files that need to be deployed
	std::vector<Progress> zips(files.size());
	for (size_t i = 0;i < files.size();i ++) 
	{
		const wchar_t *subFolder = (const wchar_t*)NULL;
		if (!files[i].second.empty()) 
		{
			subFolder = files[i].second.c_str();
		}
		boost::thread(boost::bind(&FileDeployer::deployVersionedFile, deployer.get(), files[i].first.c_str(), subFolder, boost::ref(zips[i]), commitData));
	}

	int time = 0;
	while (true)
	{
		int value = 0;
		int maxValue = 0;
		int maxSetCount = 0;
		int doneCount = 0;
		for (size_t i=0; i<zips.size(); ++i)
		{
			const Progress& p = zips[i];
			if (p.done)
				doneCount++;
			if (p.maxValue>0)
			{
				maxSetCount++;
				maxValue += p.maxValue;
			}
			value += p.value;

		}

		if (doneCount==zips.size())
			break;

		if (maxSetCount==zips.size())
			progressBar(value, maxValue);

		::Sleep(10);
		if (time>=1000)
		{
			dialog->SetMarquee(false);
		}
		else
		{
			time += 10;
		}
		checkCancel();
	}

	if (PerModeLoggingEnabled()) {
		// Above loop throws an exception if the user aborts the deploy or if
		// there is a deploy problem, so at this point we know the deploy
		// succeeded.
		// TODO: explicitly enforce that this event only gets registered for the
		// primary download, and not for other downloads (e.g. bootstrapper
		// self-upgrade).
		DWORD elapsedMillis = GetTickCount() - startTickCount;
		std::string typeAndCategory(Type());
		typeAndCategory += "BootstrapperDeploy";
		reportDurationAndSizeEvent(typeAndCategory.c_str(), "Success",
			elapsedMillis, deployer->GetBytesDownloaded());

		// Track client install time using GA

		std::string installUrl;
		if (UseNewCdn())
			installUrl = HttpTools::getCdnHost(this);
		if (installUrl.empty())
			installUrl = HttpTools::getPrimaryCdnHost(this);

		if (Type() == "Client")
		{
			GoogleAnalyticsHelper::trackUserTiming(logger, GA_CATEGORY_NAME, "ClientInstallTime", elapsedMillis, installUrl.empty() ? installHost.c_str() : installUrl.c_str());
			GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "ClientInstallThroughput bytes/sec", installUrl.empty() ? installHost.c_str() : installUrl.c_str(), deployer->GetBytesDownloaded() / (elapsedMillis / 1000.0f));

			// also log new installs to influxdb
			if (!isUpdating)
			{
				InfluxDb points;
				points.addPoint("CDN", installUrl.empty() ? installHost.c_str() : installUrl.c_str());
				double sec = elapsedMillis / 1000.0f;
				points.addPoint("Duration", sec);
				points.addPoint("BytesRecv", (int)deployer->GetBytesDownloaded());
				points.addPoint("BytesPerSec", (int)deployer->GetBytesDownloaded() / sec);
				ReportToInfluxDb(InfluxDbInstallHundredthsPercentage(), points.getJsonStringForPosting("PCInstallTime"), false);
			}
		}
	}

	dialog->SetMarquee(true);
}

std::wstring Bootstrapper::programDirectory() const
{
	return programDirectory(perUser);
};

std::wstring Bootstrapper::baseProgramDirectory(bool isPerUser) const
{
	return FileSystem::getSpecialFolder(isPerUser ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxProgramFiles, true, "Versions");
}

std::wstring Bootstrapper::programDirectory(bool isPerUser) const
{
	// Prepare the program directory
	std::wstring dir = baseProgramDirectory(isPerUser);
	if (dir.empty())
		throw std::runtime_error("Failed to create programDirectory");
	dir += convert_s2w(installVersion) + _T("\\");
			
	createDirectory(dir.c_str());
#if 1
	return dir;
#else
	// http://support.microsoft.com/kb/185126
	char path[_MAX_PATH];
	throwLastError(::GetShortPathName(dir.c_str(), path, _MAX_PATH), format_string("failed GetShortPathName %s", dir.c_str()));
	return path;
#endif
};


std::string Bootstrapper::fetchVersionGuid(std::string product = std::string())
{
	// By default look up the most recent version of the process / product that is running
	if (product.empty())
		product = convert_w2s(GetGuidName());

	LOG_ENTRY("Bootstrapper::fetchVersionGuid");
	static std::map<std::string, std::string> mostRecentVersions; 
	if (mostRecentVersions.find(product) != mostRecentVersions.end())
	{
		LOG_ENTRY1("Bootstrapper::fetchVersionGuid - found in cache: %s", mostRecentVersions[product].c_str());
		return mostRecentVersions[product];
	}

	mostRecentVersions[product] = fetchVersionGuidFromWeb(product);
	LOG_ENTRY1("Bootstrapper::fetchVersionGuid - installVersion: %s",  mostRecentVersions[product].c_str());

	return mostRecentVersions[product];
}

std::string Bootstrapper::fetchVersionGuidFromWeb(std::string product)
{
	if (GetUseNewVersionFetch() && BinaryType() != "")
	{
		//std::ostrstream result;
		//HttpTools::httpGet(this, BaseHost(), "/game/ClientVersion.ashx", std::string(), result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		//result << (char) 0;
		//return result.str();

		std::string url = GetClientVersionUploadUrl(baseHost, "76e5a40c-3ae1-4028-9f10-7c62520bd94f");
		url += "&binaryType=" + BinaryType();
		std::string result = HttpTools::httpGetString(url);

		// remove quotes
		result.erase(std::remove(result.begin(), result.end(), '\"' ), result.end());

		return result;
	}
	else
	{
		std::ostrstream result;
		std::string vsubpath = "/" + product + "?guid%d";
		HttpTools::httpGet(this, installHost, format_string(vsubpath.c_str(), reportStatGuid), std::string(), result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		result << (char) 0;

		return result.str();
	}
}

bool Bootstrapper::checkBootstrapperVersion()
{
	LOG_ENTRY("checkBootstrapperVersion()");

	CVersionInfo vi;
	vi.Load(_AtlBaseModule.m_hInst);
	moduleVersionNumber = vi.GetFileVersionAsString();
	LOG_ENTRY1("module file version: %s", moduleVersionNumber.c_str());

	message("Connecting to ROBLOX...");
	try
	{
		installVersion = fetchVersionGuid(); // TODO: Why is this setting the installVersion?
	}
	catch (std::exception& e)
	{
		// it seems right now that the main reason why it is failing that often is users local internet hiccup
		// logs upload usually happens after 3-5 secs and it is working, so lets try to sleep here before doing anything else
		Sleep(3000);
		LOG_ENTRY1("fetchVersionGuid - failed with: %s retrying after 3 sec sleep", exceptionToString(e).c_str());
		installVersion = fetchVersionGuid(); // TODO: Why is this setting the installVersion?
		
	}

	std::string bootstrapperVersion;
	try
	{
		std::ostrstream result;
		HttpTools::httpGet(this, installHost, format_string("/%s-%S", installVersion.c_str(), GetVersionFileName()), std::string(), result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		result << (char) 0;
		bootstrapperVersion = result.str();
	}
	catch (cancel_exception&)
	{
		throw;
	}
	catch (installer_error_exception&)
	{
		throw;
	}
	catch (previous_error&)
	{
		return false;
	}
	catch (std::exception& e)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(e).c_str());
		return false;
	}

	if (moduleVersionNumber != bootstrapperVersion)
	{
		if(isFailIfNotUpToDate) 
			throw non_zero_exit_exception();

		std::wstring newBootstrapper;

		// Lets kill task before updating and installing
		UninstallRobloxUpdater();

		try
		{
			
			if(queryInstalledInstallHost() == installHost)
			{
				//Check that the Installed bootstrapper matches, but only if our install host matches
				newBootstrapper = getInstalledBootstrapperPath();
				if(!newBootstrapper.empty() && ATLPath::FileExists(newBootstrapper.c_str())){
					CVersionInfo vi;
					
					vi.Load(newBootstrapper);
					moduleVersionNumber = vi.GetFileVersionAsString();
					LOG_ENTRY1("installed bootstrapper file version: %s", moduleVersionNumber.c_str());
				}
			}
		}
		catch(std::exception&)
		{
			//Something went wrong, fall back to the old method of fetching from website
			moduleVersionNumber = "";
		}
		if(moduleVersionNumber != bootstrapperVersion){
			// This bootstrapper is the wrong version. Download the correct version
			dialog->ShowWindow(CMainDialog::WindowSomethingInterestingShow);

			if (debug && windowed)
				if (dialog->MessageBox(_T("Download latest bootstrapper?"), _T("DEBUG"), MB_YESNO)==IDNO)
					return true;

			try
			{
				message("Getting the latest Roblox...");

				// We could use an "exe" extension, but hiding the type isn't a bad idea?
				newBootstrapper = simple_logger<wchar_t>::get_temp_filename(_T("tmp"));
				{
					std::ofstream bootstrapperFile(newBootstrapper.c_str(), std::ios::binary);
					// This version of the downloader doesn't show progress
					HttpTools::httpGetCdn(this, installHost, format_string("/%s-%S", installVersion.c_str(), GetBootstrapperFileName().c_str()), std::string(), bootstrapperFile, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
				}

			}
			catch (...)
			{
				newBootstrapper = simple_logger<wchar_t>::get_temp_filename(_T("tmp"));		// We could use an "exe" extension, but hiding the type isn't a bad idea?
				{
					std::ofstream bootstrapperFile(newBootstrapper.c_str(), std::ios::binary);
					// this we might need during rollback, lets be on safe side
					HttpTools::httpGetCdn(this, installHost, format_string("/%s-Roblox.exe", installVersion.c_str()), std::string(), bootstrapperFile, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
				}
			}

			message("Please Wait...");
		}

		std::wstring args;
		for (int i = 1; i < __argc; i++)
		{
			if (!args.empty())
				args += _T(" ");
			args += std::wstring(_T("\"")) + __targv[i] + _T("\"");
		}

		CProcessInformation pi;
		CreateProcess(newBootstrapper.c_str(), args.c_str(), pi);

		performedBootstrapperUpdateAndQuit = true;

		throw installer_error_exception(installer_error_exception::BootstrapVersion);
	}

	return true;
}


void Bootstrapper::writeAppSettings()
{
	message("Configuring ROBLOX...");

	std::wstring appSettings(programDirectory() + _T("AppSettings.xml"));
	std::ofstream file(appSettings.c_str());

	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	file << "<Settings>\n";
	file << "\t<ContentFolder>content</ContentFolder>\n";
	file << "\t<BaseUrl>http://" << (const char *) baseHost.c_str() << "</BaseUrl>\n";
	file << "</Settings>\n";
}

void Bootstrapper::CreateShortcuts(bool forceDesktopIconCreation, const TCHAR *linkName, const TCHAR *exeName, const TCHAR *params)
{
	bool success = true;
	RegisterEvent(_T("boostrapperCreateShortcutsStarted"));
	try
	{
		LOG_ENTRY4("Bootstrapper::CreateShortcuts force = %d, link = '%S', exeName = '%S', params = '%S'", forceDesktopIconCreation, linkName, exeName, params);
		std::wstring exePath = GetSelfName();
		createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), params, false, true);
		createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), params, true, forceDesktopIconCreation);

		long start = GetTickCount();
		std::wstring desktop = FileSystem::getSpecialFolder(FileSystem::Desktop, false, NULL, NULL);
		std::wstring versionedPath = programDirectory();
		std::wstring unversionedPath = baseProgramDirectory(isPerUser());
		updateExistingRobloxShortcuts(logger, isPerUser(), desktop.c_str(), exeName, versionedPath.c_str(), unversionedPath.c_str());

		// if user pinned our R link to task bar or start menu we want update that link as well
		if (IsWin7())
		{
			std::wstring folder = FileSystem::getSpecialFolder(FileSystem::RoamingAppData, false, NULL, NULL);
		
			std::wstring taskBar = folder + _T("Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\");
			updateExistingRobloxShortcuts(logger, isPerUser(), taskBar.c_str(), exeName, versionedPath.c_str(), unversionedPath.c_str());

			std::wstring startMenu = folder + _T("Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\StartMenu\\");
			updateExistingRobloxShortcuts(logger, isPerUser(), startMenu.c_str(), exeName, versionedPath.c_str(), unversionedPath.c_str());
		}
		else if (IsWinXP()) //WinXP
		{
			std::wstring folder = FileSystem::getSpecialFolder(FileSystem::AppData, false, NULL, NULL);
			folder += _T("Microsoft\\Internet Explorer\\Quick Launch\\");
			updateExistingRobloxShortcuts(logger, isPerUser(), folder.c_str(), exeName, versionedPath.c_str(), unversionedPath.c_str());
		}
		else // Win Vista case
		{
			std::wstring folder = FileSystem::getSpecialFolder(FileSystem::RoamingAppData, false, NULL, NULL);
			folder += _T("Microsoft\\Internet Explorer\\Quick Launch\\");
			updateExistingRobloxShortcuts(logger, isPerUser(), folder.c_str(), exeName, versionedPath.c_str(), unversionedPath.c_str());
		}
		
		long end = GetTickCount();
		LOG_ENTRY1("Folders update time %d msec", end - start);
	}
	catch (std::runtime_error &e)
	{
		LOG_ENTRY(e.what());
		success = false;
	}
	if (success) 
	{
		RegisterEvent(_T("boostrapperCreateShortcutsSuccess"));
	}
	else
	{
		RegisterEvent(_T("boostrapperCreateShortcutsFailure"));
	}
}


// The LABEL_SECURITY_INFORMATION SDDL SACL to be set for low integrity
LPCWSTR LOW_INTEGRITY_SDDL_SACL_W = L"S:(ML;;NW;;;LW)";
 
// http://www.codeproject.com/KB/vista-security/PMSurvivalGuide.aspx#creatingipc
// This function lets RobloxProxy.dll access the mutex

bool SetObjectToLowIntegrity(HANDLE hObject, SE_OBJECT_TYPE type = SE_KERNEL_OBJECT)
{
bool bRet = false;
DWORD dwErr = ERROR_SUCCESS;
PSECURITY_DESCRIPTOR pSD = NULL;
PACL pSacl = NULL;
BOOL fSaclPresent = FALSE;
BOOL fSaclDefaulted = FALSE;
 
  if ( ConvertStringSecurityDescriptorToSecurityDescriptorW (
         LOW_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL ) )
    {
    if ( GetSecurityDescriptorSacl (
           pSD, &fSaclPresent, &pSacl, &fSaclDefaulted ) )
      {
      dwErr = SetSecurityInfo (
                hObject, type, LABEL_SECURITY_INFORMATION,
                NULL, NULL, NULL, pSacl );
 
      bRet = (ERROR_SUCCESS == dwErr);
      }
 
    LocalFree ( pSD );
    }
 
  return bRet;
}

void Bootstrapper::checkOSPrerequisit()
{
	LOG_ENTRY("checkOSPrerequisit");
	// http://msdn.microsoft.com/en-us/library/ms725491(VS.85).aspx

	{
		OSVERSIONINFOEX osvi = {0};
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		GetVersionEx((OSVERSIONINFO*)&osvi);
		LOG_ENTRY1("OSVERSIONINFO.dwMajorVersion %d", osvi.dwMajorVersion);
		LOG_ENTRY1("OSVERSIONINFO.wServicePackMajor %d", osvi.wServicePackMajor);
		LOG_ENTRY1("OSVERSIONINFO.wServicePackMajor %d", osvi.wServicePackMajor);
		LOG_ENTRY1("OSVERSIONINFO.wServicePackMinor %d", osvi.wServicePackMinor);
		LOG_ENTRY1("OSVERSIONINFO.dwBuildNumber %d", osvi.dwBuildNumber);
		LOG_ENTRY1("OSVERSIONINFO.dwPlatformId %d", osvi.dwPlatformId);
	}

	// Initialize the condition mask.

	OSVERSIONINFOEX osvi = {0};
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 1;
	osvi.wServicePackMajor = 1;	// Roblox requires WinXP SP1 because it links to WinHttp
	osvi.wServicePackMinor = 0;

	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL );

	// Perform the test.
	if (VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR, dwlConditionMask))
	  return;

	LOG_ENTRY("checkOSPrerequisit failed");
	if (windowed)
		dialog->DisplayError("Roblox requires Microsoft Windows XP SP1 or greater", NULL);
	throw installer_error_exception(installer_error_exception::OsPrerequisite);
}

void Bootstrapper::checkCPUPrerequisit()
{
    if (!Bootstrapper::hasSse2())
	{
		LOG_ENTRY("checkCPUPrerequisit failed");
	    if (windowed)
		    dialog->DisplayError("Roblox requires SSE2 support", NULL);
		throw installer_error_exception(installer_error_exception::CpuPrerequisite);
	}
}

void Bootstrapper::checkDirectXPrerequisit()
{
	// DirectX is not required
#if 0
	if (!isDirectXUpToDate())
	{
		log << "checkDirectXPrerequisit failed\n"; log.flush();
		if (windowed)
			dialog->DisplayError("Roblox requires DirectX 9.0 or greater", NULL);
		throw installer_error_exception(installer_error_exception::DirectxPrerequisite);
	}
#endif
}

void Bootstrapper::checkIEPrerequisit()
{
	if (!isIEUpToDate())
	{
		LOG_ENTRY("checkIEPrerequisit failed");
		if (windowed)
			dialog->DisplayError("Roblox requires Microsoft Internet Explorer 6.0 or greater", NULL);
		throw installer_error_exception(installer_error_exception::IePrerequisite);
	}
}

bool Bootstrapper::isIEUpToDate()
{
	CRegKey key;
	if (ERROR_SUCCESS!=key.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Internet Explorer"), KEY_READ))
		return false;

	TCHAR buffer[256];
	ULONG size = 256;
	if (ERROR_SUCCESS!=key.QueryStringValue(_T("Version"), buffer, &size))
		return false;

	// TODO: Parse version string properly
	// http://support.microsoft.com/kb/164539
	if (buffer[0]<'6')
		return false;

	return true;
}


bool Bootstrapper::isDirectXUpToDate()
{
	CRegKey key;
	if (ERROR_SUCCESS!=key.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DirectX"), KEY_READ))
		return false;

	TCHAR buffer[256];
	ULONG size = 256;
	if (ERROR_SUCCESS!=key.QueryStringValue(_T("Version"), buffer, &size))
		return false;

	std::vector<std::wstring> version = splitOn(buffer,_T('.'));
	if (version.size()<4)
		throw std::runtime_error(format_string("bad DirectX version %S", buffer));

	// TODO: Parse version string properly
	if ( atoi(convert_w2s(version[1]).c_str()) < 9 )
		return false;

	return true;
}

void Bootstrapper::shutdownRobloxApp(std::wstring appExeName)
{
	int time = 0;
	ShutdownRobloxApp s(hInstance, appExeName, boost::bind(&CMainDialog::GetHWnd, dialog.get()), 15, boost::bind(&Bootstrapper::shutdownProgress, this, boost::ref(time), _1, _2));
	if(s.run()){
		//If we have to close Roblox, show the "updating" dialog right away
		dialog->ShowWindow(CMainDialog::WindowSomethingInterestingShow);
	}
	dialog->SetMarquee(true);
}

bool Bootstrapper::shutdownProgress(int& time, int pos, int max)
{
	if (stopRequested())
		return false;
	time += 100;
	if (time==1000)
	{
        std::string text = "Shutting down " + convert_w2s(GetFriendlyName());
		message(text);
		dialog->SetMarquee(false);
	}
	progressBar(pos, max);
	return true;
}

std::wstring Bootstrapper::getInstalledBootstrapperPath(const std::wstring& RegistryPath)
{
	std::wstring result;
	TCHAR buffer[_MAX_PATH];
	ULONG size = _MAX_PATH;

	if (perUser)
	{
		// First check HKCU then try HKLM
		CRegKey key;
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, RegistryPath.c_str(), KEY_READ))
		{
			if (ERROR_SUCCESS == key.QueryStringValue(_T(""), buffer, &size))
			{
				result = buffer;
			}
		}
	}

	if (result.empty())
	{
		CRegKey key;
		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, RegistryPath.c_str(), KEY_READ))
		{
			if (ERROR_SUCCESS == key.QueryStringValue(_T(""), buffer, &size))
			{
				result = buffer;
			}
		}
	}

	return result;
}

std::string Bootstrapper::queryInstalledInstallHost()
{
	std::string result;

	if (perUser)
	{
		// First check HKCU then try HKLM
		CRegKey key;
		if (ERROR_SUCCESS==key.Open(HKEY_CURRENT_USER, GetRegistryPath().c_str(), KEY_READ))
		{
			result = QueryStringValue(key, _T("install host"));
			LOG_ENTRY2("HKCU\\%S :install host = %s", GetRegistryPath().c_str(), result.c_str());
		}
	}

	if (result.empty())
	{
		CRegKey key;
		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, GetRegistryPath().c_str(), KEY_READ))
		{
			result = QueryStringValue(key, _T("install host"));
			LOG_ENTRY2("HKLM\\%S :install host = %s", GetRegistryPath().c_str(), result.c_str());
		}
		adminInstallDetected = true;
	}

	return result;
}

std::string Bootstrapper::queryInstalledVersion(const std::wstring& RegistryPath)
{
	std::string result;
	CRegKey key;

	if (perUser)
	{
		// First check HKCU then try HKLM
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, RegistryPath.c_str(), KEY_READ))
		{
			result = QueryStringValue(key, _T("version"));
			LOG_ENTRY2("HKCU\\%S :version = %s", RegistryPath.c_str(), result.c_str());
		}
	}

	if (result.empty())
	{
		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, RegistryPath.c_str(), KEY_READ))
		{
			result = QueryStringValue(key, _T("version"));
			LOG_ENTRY2("HKLM\\%S :version = %s", RegistryPath.c_str(), result.c_str());
		}
		adminInstallDetected = true;
	}

	return result;
}

void Bootstrapper::checkDiskSpace()
{
    ULARGE_INTEGER freeBytesAvailableToCaller;
	::GetDiskFreeSpaceEx(programDirectory().c_str(), &freeBytesAvailableToCaller, NULL, NULL);
	if (freeBytesAvailableToCaller.QuadPart < 40*1e6)
	{
		dialog->DisplayError("There is not enough room on your disk to install Roblox. Please free up some space and try again.", NULL);
		throw installer_error_exception(installer_error_exception::DiskSpacePrerequisite);
	}
}

void Bootstrapper::run()
{
	srand(GetTickCount());
	
	if (!robloxAppArgs.empty())
		setLatestProcess();

	if (isNop)
		goto done;

	::CoInitialize(NULL);

	// TODO remove this code next iteration
	UninstallRobloxUpdater();

    try
    {
        forceUninstallMFCStudio();
    }
    catch (...)
	{
		LOG_ENTRY("Bootstrapper::run - forceUninstallMFCStudio failed");
	}


	startTime = time(NULL);

	try
	{
		try
		{
			checkCPUPrerequisit();
			checkOSPrerequisit();
			checkIEPrerequisit();
			checkDirectXPrerequisit();
		}
		catch(...)
		{
			const bool isUpdating = !queryInstalledVersion().empty();
			throw;
		}

		if (EarlyInstall(isUninstall))
		{
			goto done;
		}

		dialog->SetMarquee(true);

		DWORD result = 0;
		if (!IsNetworkAlive(&result) || result == 0)
		{
			if(isFailIfNotUpToDate){
				LOG_ENTRY("The network is done, but -FailIfNotUpToDate should not lock us up, so returning true");
				goto done;
			}

			LOG_ENTRY("Error: IsNetworkAlive failed");
			if (windowed && isLatestProcess())
			{
				CString message = _T("Roblox cannot connect to the Internet\n\nDoes your computer have a working network connection?  Is antivirus software preventing Roblox from accessing the Internet?");
				if (!robloxAppArgs.empty())
				{
					message += _T("\n\nIf you continue Roblox may not work properly.");
					// TODO: CTaskDialog should use nice command buttons
					if (dialog->MessageBox(message, _T("Error"), MB_OKCANCEL | MB_ICONEXCLAMATION)==IDOK)
					{
						installVersion = queryInstalledVersion();
						StartRobloxApp(false);
					}
				}
				else
				{
					dialog->DisplayError(convert_w2s(message.GetString()).c_str(), NULL);
				}
			}
			goto done;
		}

		LoadSettings();

		influxDb.addPoint("SettingsLoaded", GetElapsedTime() / 1000.0f); // adding points does not need setup

		SetupGoogleAnalytics();
		SetupInfluxDb();

#ifdef LOCAL_TESTING
        installVersion = fetchVersionGuid();
#else
		if (!checkBootstrapperVersion())
		{
			if(isFailIfNotUpToDate) 
				throw non_zero_exit_exception();

			if (windowed && isLatestProcess())
			{
				CString message = _T("Cannot connect to the Roblox website.\n\nIs antivirus software preventing Roblox from accessing the Internet?");
				if (!robloxAppArgs.empty())
				{
					message += _T("\n\nIf you continue Roblox may not work properly.");
					if (dialog->MessageBox(message, _T("Error"), MB_OKCANCEL | MB_ICONEXCLAMATION)==IDOK)
					{
						installVersion = queryInstalledVersion();
						StartRobloxApp(false);
					}
				}
				else
				{
					dialog->DisplayError(convert_w2s(message.GetString()).c_str(), NULL);
				}
			}
			goto done;
		}
#endif

		if(isFailIfNotUpToDate) 
		{
			if(queryInstalledVersion() != installVersion) 
				throw non_zero_exit_exception();

			LOG_ENTRY("Roblox is up to date, returning success");
			goto done;
		}

		influxDb.addPoint("SelfVersionChecked", GetElapsedTime() / 1000.0f);

		if (queryInstalledVersion() == installVersion)
		{
			if (fullCheck)
			{
				message("Performing file check...");
				LOG_ENTRY("Performing file check...");

				std::wstring manifestFileName = simple_logger<wchar_t>::get_temp_filename(_T("manifest"));
				int status_code = -1;
				{
					std::ofstream ofs(manifestFileName.c_str());

					// This version of the downloader doesn't show progress
					status_code = HttpTools::httpGetCdn(this, installHost, format_string("/%s-rbxManifest.txt", installVersion.c_str()), std::string(), ofs, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
				}

				if (status_code == 200 || status_code == 304)
				{
					std::fstream manifestFile;
					manifestFile.open(manifestFileName.c_str());
					if (manifestFile.is_open())
					{
						// try to find each file
						std::string fileName;
						std::string fileHash;
						std::ifstream ifs;
						while (!manifestFile.eof())
						{
							std::getline(manifestFile, fileName);
							if (fileName.length() == 0)
								break;

							if (*fileName.rbegin() == '\r')
								fileName.erase(fileName.length() - 1);

							ifs.open((programDirectory() + convert_s2w(fileName)).c_str(), std::ios::in | std::ios::binary);
							if (ifs.is_open())
							{
								MD5Hasher hasher;
								hasher.addData(ifs);
								std::getline(manifestFile, fileHash);
								if (fileHash.length() && *fileHash.rbegin() == '\r')
									fileHash.erase(fileHash.length() - 1);

								if (hasher.toString() != fileHash)
								{
									LOG_ENTRY1("%s corruption detected, forcing reinstall.",fileName.c_str());
									force = true;
									ifs.close();
									break;
								}

								ifs.close();
							}
							else
							{
								LOG_ENTRY1("%s not found, forcing reinstall.", fileName.c_str());
								force = true;
								ifs.close();
								break;
							}
						}

						manifestFile.close();

						LOG_ENTRY("File check complete.");
					}
				}
				else
				{
					LOG_ENTRY("Failed to get manifest file, full check skipped.");
				}
			}
			else
			{
				// if the system is indicating we have roblox installed, try and find the exe
				HANDLE robloxAppFile;
				WIN32_FIND_DATA robloxAppFileData;
				robloxAppFile = FindFirstFile((programDirectory() + GetRobloxAppFileName()).c_str(), &robloxAppFileData);

				LOG_ENTRY1("RobloxAppFileName = %S", GetRobloxAppFileName().c_str());
				LOG_ENTRY1("programDirectory = %S", programDirectory().c_str());

				// if RobloxApp.exe is not found in the indicated install folder, force uninstall/install
				if ((robloxAppFile == INVALID_HANDLE_VALUE))
				{
					LOG_ENTRY1("%S not found, forcing reinstall.", GetRobloxAppFileName().c_str());
					force = true;
				}
				else
				{
					if (!IsInstalledVersionUptodate())
					{
						// track version mismatch
						if (!GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "VersionMismatch"))
							LOG_ENTRY("WARNING: failed to log version mismatch");

						if (ValidateInstalledExeVersion())
						{
							LOG_ENTRY("RobloxApp version mismatch, forcing reinstall.");
							force = true;
						}
					}
				}

				::FindClose(robloxAppFile);
			}
		}
	
		bool isNewInstall = false;
		{
			CMutex mutex(NULL, FALSE, GetBootstrapperMutexName());

			CTimedMutexLock lock(mutex);
			bool firstTime = true;
			while (lock.Lock(100) == WAIT_TIMEOUT )
			{
				if (firstTime)
				{
					LOG_ENTRY("Waiting for mutex");
					dialog->SetMessage("Please wait...");
					firstTime = false;
				}
				checkCancel();
				if (!isUninstall && !force)
				{
					if (!isLatestProcess())
					{
						// This process has been superceded by another one, so don't bother running
						throw cancel_exception(false);
					}
				}
			}
			dialog->ShowWindow(CMainDialog::WindowNoLongerBoringShow);

			if (!perUser)
			{
				uninstall(true);	// first uninstall per-user Roblox
			}

			if (isUninstall)
			{
                std::string text = "Uninstalling " + convert_w2s(GetFriendlyName());
				message(text);
				uninstall();
				if (windowed)
				{
                    text = convert_w2s(GetFriendlyName()) + " has been uninstalled";
					dialog->FinalMessage(text.c_str());
				}
			}
			else if (force || (queryInstalledVersion() != installVersion))
			{
				cancelDelay = 2;
				if (cancelDelay)
				{
					dialog->ShowCancelButton(CMainDialog::CancelTimeDelayHide);
					boost::thread(boost::bind(&Bootstrapper::showCancelAfterDelay, this));
				}

				//Update/Install required
				checkDiskSpace();

				isNewInstall = queryInstalledVersion().empty();

				try
				{
					install();

					installVersion = queryInstalledVersion();
					if (!IsInstalledVersionUptodate())
					{
						std::string label;
						if (!isNewInstall)
							label += "Update";
						else
							label += "Install";
						if (force)
							label += "Force";

						// track version mismatch
						if (!GoogleAnalyticsHelper::trackEvent(logger, GA_CATEGORY_NAME, "VersionMismatchPostInstall", label.c_str()))
							LOG_ENTRY("WARNING: failed to log version mismatch post install");

						LOG_ENTRY("WARNING: client version not up to date after install.");
						postLogFile(false);
					}
				}
				catch (...)
				{
					if (isNewInstall) 
					{
						RegisterEvent(_T("BootstrapperInstallFailed"));
					}
					else
					{
						RegisterEvent(_T("BootstrapperUpdateFailed"));
					}
					throw;
				}
			}
			else{
				//Roblox is already installed
				//If it is installed for all users
				if(adminInstallDetected)
				{
					perUser = false;
				}
			}
		}

        std::string text = convert_w2s(GetFriendlyName()) + " is up-to-date";
		message(text);
		if (debug)
		{
			dialog->SetMarquee(false);
			for (int i = 5; i>0; --i)
			{
				dialog->SetProgress(20*(5-i+1));

                std::string text = format_string(
                    "%s is up-to-date - starting in %d seconds", 
                    convert_w2s(GetFriendlyName()).c_str(),
                    i );
				message(text);
				::Sleep(1*1000);
				checkCancel();
			}
		}

		checkCancel();
		if (!robloxAppArgs.empty() && isLatestProcess())
			StartRobloxApp(isNewInstall);

		LOG_ENTRY("Done!");
	}
	catch (non_zero_exit_exception&)
	{
		exitCode = 1;
		LOG_ENTRY("Roblox is not up to date, returning failure");
	}
	catch (silent_exception&)
	{
		// TODO: Message to user???
	}
	catch( CAtlException e )
	{
		error(std::runtime_error(format_string("AtlException 0x%x", e.m_hr)));
	}
	catch (std::exception& e)
	{
		error(e);
	}

done:
	{
		boost::lock_guard<boost::mutex> lock(mut);
		done.notify_all();
	}

	if (!isSilentMode())
	{
		if (!Dialog()->isSuccessPromptShown())
		{
			LOG_ENTRY1("WM_QUIT message to the thread id = %d", mainThreadId);
			::PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);
		}
		else
			LOG_ENTRY("Install success prompt shown, waiting for input");

	}
	else
	{
		LOG_ENTRY("Setting up finish event in silent mode");
		robloxReady.Set();
	}

	LOG_ENTRY("Fully Done!");
	::CoUninitialize();
}




bool Bootstrapper::deleteDirectory(const std::wstring &refcstrRootDirectory, bool childrenOnly)
{
	LOG_ENTRY2("deleteDirectory %S childrenOnly=%d", refcstrRootDirectory.c_str(), childrenOnly);

	bool failure = false;
	std::wstring     strFilePath;                 // Filepath
	std::wstring     strPattern;                  // Pattern
	WIN32_FIND_DATA FileInformation;             // File information

	strPattern = refcstrRootDirectory + _T("*.*");
	HANDLE hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			try
			{
				checkCancel();
			}
			catch (...)
			{
				// Close handle
				::FindClose(hFile);
				throw;
			}

			if(FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + FileInformation.cFileName;

				if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Delete subdirectory
					failure = deleteDirectory(strFilePath + _T("\\")) || failure;
				}
				else
				{
					// Set file attributes
					if(::SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					{
						failure = true;
					}
					else
					{
						// Delete file
						if(::DeleteFile(strFilePath.c_str()) == FALSE)
						{
							failure = true;
						}
					}
				}
			}
		} while(::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_NO_MORE_FILES)
		{
			failure = true;
		}
		else if (!childrenOnly)
		{
			// Set directory attributes
			if(::SetFileAttributes(refcstrRootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
			{
				failure = true;
			}
			// Delete directory
			else if(::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
			{
				failure = true;
			}
		}
	}
	
	return failure;
}


void Bootstrapper::cleanKey(const std::wstring regPathToDelete, CRegKey &hk)
{
	CRegKey key;

	if( key.Open(hk, std::wstring( _T("SOFTWARE\\") + regPathToDelete ).c_str(), KEY_READ) == ERROR_SUCCESS )
	{
		CRegKey versionKey;

		// if this is true, that means we never have gotten a version of studio, we can delete
		if( versionKey.Open(hk, std::wstring(_T("SOFTWARE\\") + regPathToDelete + _T("\\Versions")).c_str(), KEY_READ) != ERROR_SUCCESS )
		{
			CRegKey softwareKey;
			if (ERROR_SUCCESS != softwareKey.Open(hk, _T("Software"), KEY_WRITE))
			{
				LOG_ENTRY("WARNING: Failed to open HK**\\Software");
				return;
			}
			DeleteKey(logger, softwareKey, regPathToDelete.c_str());
		}
	}
}


void Bootstrapper::uninstall()
{
	uninstall(perUser);
}

void Bootstrapper::uninstall(bool isPerUser)
{
    LOG_ENTRY("Bootstrapper::uninstall");

	CRegKey hk(isPerUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
	{
		CRegKey key;
		if (ERROR_SUCCESS != key.Open(hk, GetProductCodeKey().c_str(), KEY_READ))
		{
			LOG_ENTRY1("Uninstall %s nothing to do", (isPerUser ? "per user" : "all users"));
			return;
		}
	}

	LOG_ENTRY1("Uninstall %s", (isPerUser ? "per user" : "all users"));

	shutdownRobloxApp(GetRobloxAppFileName());

	LOG_ENTRY1("Deleting HKCU\\Software\\%S", GetRegistryPath().c_str());

	CRegKey key;
	if (ERROR_SUCCESS!=key.Open(hk, _T("Software"), KEY_WRITE))
	{
		LOG_ENTRY("WARNING: Failed to open HK**\\Software");
		return;
	}

	DoUninstallApp(hk);

	std::string version = installVersion;
	installVersion = "";

	LOG_ENTRY("Remove from Add/Remove programs");
	if (FAILED(hk.DeleteSubKey(GetProductCodeKey().c_str())))
	{
		LOG_ENTRY("WARNING: failed to delete ProductCodeKey");
	}

	deleteVersionFolder(version);
    deleteLegacyShortcuts();

    // check for components still installed

	std::vector<std::wstring> components;
	components.push_back(getStudioInstallKey());
	components.push_back(getPlayerInstallKey());
	components.push_back(getQTStudioInstallKey());

	bool finalCleanup = true;
	for (size_t i = 0;i < components.size();i ++) 
	{
		if (components[i].c_str() != GetProductCodeKey() && isInstalled(components[i].c_str())) 
		{
			LOG_ENTRY1("Component %S still installed, no cleanup needed", components[i].c_str());
			finalCleanup = false;
			break;
		}
	}

	// final cleanup when we have no more components installed -- nuke all shortcuts
	if (finalCleanup) 
	{
		std::wstring folder = getRobloxProgramsFolder(logger, isPerUser);
		if (!folder.empty())
		{
			LOG_ENTRY1("Deleting roblox shortcuts folder = %S", folder.c_str());
			deleteDirectory(folder, false);
		}

		deleteVersionsDirectoryContents();

        LOG_ENTRY("Bootstrapper::uninstall - deleteDesktopShortcut");
        deleteDesktopShortcut(logger, _T(PLAYERLINKNAME_CUR));
		deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME_CUR));

        LOG_ENTRY("Bootstrapper::uninstall - deleteProgramsShortcut");
        deleteProgramsShortcut(logger, isPerUser, _T(PLAYERLINKNAME_CUR));
        deleteProgramsShortcut(logger, isPerUser, _T(STUDIOQTLINKNAME_CUR));

		// explicitly delete the Qt key since Client deletes its key
		// the Qt key stays around since it's needed by the shared launcher to install Studio for 
		//  Build and Edit modes
        DeleteKey(logger,key,getQTStudioRegistrySubPath().c_str());
		UnregisterProtocolHandler(getQTStudioProtocolScheme(BaseHost()));
	}

	LOG_ENTRY1("Uninstall %s", (isPerUser ? "per user done" : "all users done"));
}

void Bootstrapper::deleteVersionsDirectoryContents()
{
	try
	{
		// Clear out old versions as much as possible
		std::wstring dir = FileSystem::getSpecialFolder(perUser ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxProgramFiles, true, "Versions");
		deleteDirectory(dir, true);
	}
	catch (cancel_exception&)
	{
		throw;
	}
	catch (installer_error_exception&)
	{
		throw;
	}
	catch (...)
	{
		// ignore this error
	}
}

void Bootstrapper::deleteVersionFolder(std::string version)
{
	LOG_ENTRY1("Bootstrapper::deleteVersionFolder - version=%s", version.c_str());

	if (version.empty()) // probably clean install
	{
		return;
	}

	if (version == queryInstalledVersion())
	{
		LOG_ENTRY("Bootstrapper::deleteVersionFolder - WARNING trying to delete current version");
		return;
	}

	std::wstring dir = FileSystem::getSpecialFolder(perUser ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxProgramFiles, false, "Versions");
	if (dir.empty())
	{
		return;
	}

	std::wstring curDir = dir + convert_s2w(version) + _T('\\');
	try
	{
		deleteDirectory(curDir, false);
	}
	catch (cancel_exception&)
	{
		throw;
	}
	catch (installer_error_exception&)
	{
		throw;
	}
	catch (...)
	{
		// ignore this error
	}
}

void Bootstrapper::DeleteOldVersionsDirectories()
{
	try
	{
		// Clear out old versions as much as possible
		std::wstring dir = FileSystem::getSpecialFolder(perUser ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxProgramFiles, false, "Versions");
		if (dir.empty())
			return;
		WIN32_FIND_DATA FileInformation;             // File information
		HANDLE hFile = ::FindFirstFile((dir + _T("*.*")).c_str(), &FileInformation);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			try
			{
				do
				{
					if ((FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
						continue;
					if (FileInformation.cFileName[0] == _T('.'))
						continue;
					if (installVersion==convert_w2s(FileInformation.cFileName))
						continue;
					
					std::wstring curDir = dir + FileInformation.cFileName + _T('\\');
					deleteDirectory(curDir, false);
				} 
				while(::FindNextFile(hFile, &FileInformation) == TRUE);
			}
			catch (...)
			{
				// Close handle
				::FindClose(hFile);
				throw;
			}
			// Close handle
			::FindClose(hFile);
		}
	}
	catch (cancel_exception&)
	{
		throw;
	}
	catch (installer_error_exception&)
	{
		throw;
	}
	catch (...)
	{
		// ignore this error
	}
}


static bool reportValue(CookiesEngine &engine, std::string key, std::string value)
{
	int rCounter = 10;

	while (rCounter >= 0)
	{
		int result = engine.SetValue(key, value);
		if (result == 0)
		{
			break;
		}
		::Sleep(50);
		rCounter --;
	}

	return rCounter >= 0 ? true : false;
}

std::wstring Bootstrapper::prevVersionRegKey()
{
	return std::wstring(GetRegistryPath()) + _T("\\Versions");
}

std::wstring Bootstrapper::prevVersionValueKey(int index)
{
	return format_string(_T("version%d"), index);
}

void Bootstrapper::loadPrevVersions()
{
	std::wstring keyPath = prevVersionRegKey();
	CRegKey versionsKey = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, keyPath.c_str());
	LOG_ENTRY1("Bootstrapper::loadPrevVersions path = %S", keyPath.c_str());

	int i = 0;
	while (true)
	{
		std::wstring keyName = prevVersionValueKey(i);
		TCHAR verString[MAX_PATH];
		ULONG size = MAX_PATH;
		if (ERROR_SUCCESS != versionsKey.QueryStringValue(keyName.c_str(), verString, &size)) 
		{
			break;
		}

		prevVersions.push_back(convert_w2s(verString));
		LOG_ENTRY1("Bootstrapper::loadPrevVersions - key found name = %S", verString);
		i ++;
	}
}

void Bootstrapper::storePrevVersions()
{
	std::wstring keyPath = prevVersionRegKey();
	CRegKey versionsKey = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, keyPath.c_str());
	LOG_ENTRY2("Bootstrapper::storePrevVersions path = %S, perUser = %d", keyPath.c_str(), isPerUser());

	int i = 0;
	while (true)
	{
		std::wstring keyName = prevVersionValueKey(i);
		TCHAR verString[MAX_PATH];
		ULONG size = MAX_PATH;
		if (ERROR_SUCCESS != versionsKey.QueryStringValue(keyName.c_str(), verString, &size)) 
		{
			break;
		}
		
		LOG_ENTRY2("Bootstrapper::storePrevVersions deleting key = %S, value = %S", keyName.c_str(), verString);
		versionsKey.DeleteValue(keyName.c_str());
		i ++;
	}

	i = 0;
	for (std::list<std::string>::iterator j = prevVersions.begin();j != prevVersions.end();j ++) 
	{
		std::string version = *j;
		std::wstring path = baseProgramDirectory(isPerUser()) + convert_s2w(version);
		if (FileSystem::IsFolderExists(path.c_str())) 
		{
			std::wstring keyName = prevVersionValueKey(i);
			LOG_ENTRY2("Bootstrapper::storePrevVersions storing key = %S, value = %s", keyName.c_str(), version.c_str());
			throwHRESULT(versionsKey.SetStringValue(keyName.c_str(), convert_s2w(version).c_str()), "SetKeyFailed");
			versionsKey.Flush();
			i ++;
		}
	}
}


/*

The order that items are deployed are important.

First we deploy this module, so that the proxy dll can find it.
Then we deploy and configure the proxy dll. It can now launch this module again, even if it isn't fully installed
Then we deploy all the other components.
Finally, we update the version strings in the registry.
*/
void Bootstrapper::install()
{
    LOG_ENTRY("Bootstrapper::install");

	const bool isUpdating = !queryInstalledVersion().empty();

	LOG_ENTRY1("isUpdating = %d", isUpdating);
	
	CookiesEngine engine(CookiesEngine::getCookiesFilePath());
	bool reportSuccess = false;

	if (!isUpdating) 
	{
		reportSuccess = reportValue(engine, "rbx_evt_initial_install_start", "{\"os\" : \"Win32\"}");
		RegisterEvent(_T("BootstrapperInstallStarted"));
		performedNewInstall = true;
	}
	else
	{
		RegisterEvent(_T("BootstrapperUpdateStarted"));
		performedUpdateInstall = true;
	}

	//reportStat(IsVista() ? "Vista" : "XP");
	try
	{
		if (IsWinXP())
		{
			if (!perUser && !IsAdminRunning())
			{
				if (windowed)
					dialog->DisplayError("On this machine you must install Roblox using an Administrator account", NULL);
				throw installer_error_exception(installer_error_exception::AdminAccountRequired);
			}
		}

		shutdownRobloxApp(GetRobloxAppFileName());
		checkCancel();

        std::string text = "Configuring " + convert_w2s(GetFriendlyName());
		message(text);

		// set install status
		if (!browserTrackerId.empty())
		{
			try 
			{
				std::ostrstream result;
				HttpTools::httpPost(this, baseHost, format_string("/client-status/set?browserTrackerId=%s&status=BootstrapperInstalling", browserTrackerId.c_str()), std::stringstream(""), "*/*",
					result, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
			}
			catch (std::exception& ex)
			{
				LOG_ENTRY1("Error: %s", exceptionToString(ex).c_str());
			}
		}

		DoInstallApp();

        // nukes all the old versions
		for (std::list<std::string>::iterator j = prevVersions.begin();j != prevVersions.end();j ++) 
		{
			deleteVersionFolder(*j);
		}
		prevVersions.push_back(installVersion);

		storePrevVersions();

		if (reportSuccess)
		{
			reportSuccess = reportValue(engine, "rbx_evt_initial_install_success", "{\"os\" : \"Win32\"}");
		}
	}
	catch (cancel_exception& e)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(e).c_str());
		std::string message = format_string("%s: %s", e.what(), e.getErrorType().c_str());
		ReportInstallMetricsToInfluxDb(isUpdating, "Cancel", message);			
		throw;
	}
	catch(installer_error_exception& e)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(e).c_str());
		std::string message = format_string("%s: %s", e.what(), e.getErrorType().c_str());
		ReportInstallMetricsToInfluxDb(isUpdating, "Error", message);
		throw;
	}
	catch (previous_error& e)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(e).c_str());
		ReportInstallMetricsToInfluxDb(isUpdating, "Error", e.what());
		throw;
	}
	catch (std::exception& ex)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(ex).c_str());
		ReportInstallMetricsToInfluxDb(isUpdating, "Error", ex.what());
		throw;
	}

	if (!isUpdating) 
	{
		RegisterEvent(_T("BootstrapperInstallSuccess"));
	}
	else
	{
		RegisterEvent(_T("BootstrapperUpdateSuccess"));
	}

	ReportInstallMetricsToInfluxDb(isUpdating, "Success", "");
}

void Bootstrapper::checkCancel()
{
	if (userCancelled)
		throw cancel_exception(true);
	if (hasErrors()){
		std::string errorString="";
		int i = 0;
		for(std::vector<std::string>::const_iterator iter = errors.begin();
			iter != errors.end(); ++iter){
				errorString += format_string("%d: %s ", ++i, (*iter).c_str());
		}
		throw previous_error(errorString);
	}
}

void Bootstrapper::userCancel()
{
	userCancelledTime = time(NULL);
	userCancelled = true;
}

void Bootstrapper::error(const std::exception& e)
{
	LOG_ENTRY1("ERROR: %s", exceptionToString(e).c_str());
	if (errors.size() == 0 && windowed)
    {
        std::string text = "An error occurred while starting " + convert_w2s(GetFriendlyName());        
		dialog->DisplayError(text.c_str(),e.what());
	}
	errors.push_back(exceptionToString(e));
}

std::string __cdecl format(const char* fmt,...) {
    va_list argList;
    va_start(argList,fmt);
    std::string result = vformat(fmt, argList);
    va_end(argList);

    return result;
}

static std::string urlEncode(const std::string url)
{
	std::string result;

	const size_t strLen = url.size();
	for (unsigned i=0; i < strLen; i++)
	{
		char c = url[i];
		if (
			(c<=47) ||
			(c>=58 && c<=64) ||
			(c>=91 && c<=96) ||
			(c>=123)
			)
		{
			result += format("%%%2X", c);
		}
		else
			result += c;
	}

	return result;
}

void Bootstrapper::setLatestProcess()
{
	int pid = _getpid();

	const std::string latestProcessName = format_string("www.roblox.com/%s/%s/latestProcess", installHost.c_str(), Type().c_str());

	latestProcess.Attach(CreateFileMapping(
			 INVALID_HANDLE_VALUE,    // use paging file
			 NULL,                    // default security 
			 PAGE_READWRITE,          // read/write access
			 0,                       // max. object size 
			 sizeof(pid),           // buffer size  
			 convert_s2w(latestProcessName).c_str()));   // name of mapping object
	if (latestProcess == NULL) 
	{ 
		LOG_ENTRY1("Could not create file mapping object: %S", (LPCTSTR)GetLastErrorDescription());
		return;
	}

	LPVOID pBuf = MapViewOfFile(latestProcess, FILE_MAP_ALL_ACCESS, 0, 0, 2048);           
	 
	if (pBuf == NULL) 
	{ 
		LOG_ENTRY1("Could not map view of file: %S", (LPCTSTR)GetLastErrorDescription());
		latestProcess.Close();
		return;
	}

   CopyMemory((PVOID)pBuf, &pid, sizeof(pid));

   UnmapViewOfFile(pBuf);
}

bool Bootstrapper::isLatestProcess()
{
	LPVOID pBuf = MapViewOfFile(latestProcess, FILE_MAP_ALL_ACCESS, 0, 0, 2048);           

	if (pBuf == NULL) 
	{ 
		LOG_ENTRY1("Could not map view of file: %S", (LPCTSTR)GetLastErrorDescription());
		latestProcess.Close();
		return true;
	}

	DWORD pid;
	CopyMemory(&pid, (PVOID)pBuf, sizeof(pid));

	UnmapViewOfFile(pBuf);

	return pid == ::_getpid();
}

void Bootstrapper::progressBar(int pos, int max)
{
	int progress = (int)((__int64)max ? 100 * (__int64)pos / (__int64)max : 0);
	dialog->SetProgress(progress);
}

void Bootstrapper::forceUninstallMFCStudio()
{
    LOG_ENTRY("Bootstrapper::forceUninstallMFCStudio");

    // check if Studio is installed
    if ( !isInstalled(getStudioInstallKey()) )
    {
        forceMFCStudioCleanup();
        LOG_ENTRY("Bootstrapper::forceUninstallMFCStudio - not installed");
        return;
    }

    // find the uninstall registry key
    CRegKey key;
    if ( key.Open(
            perUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
            getStudioInstallKey().c_str(), 
            KEY_READ ) != ERROR_SUCCESS )
    {
        LOG_ENTRY("Bootstrapper::forceUninstallMFCStudio - failed to find install key");
        return;
    }

    // get the uninstall command
    TCHAR value[512];
	ULONG length = sizeof(value) / sizeof(TCHAR);
    if ( key.QueryStringValue(_T("UninstallString"),value,&length) != ERROR_SUCCESS )
    {
        LOG_ENTRY("Bootstrapper::forceUninstallMFCStudio - failed to find uninstall string");
        return;
    }

    // run the command
    std::wstring uninstall_path = value;
    if ( !uninstall_path.empty() )
    {
        LOG_ENTRY1(
            "Bootstrapper::forceUninstallMFCStudio - path: %S",
            uninstall_path.c_str() );

        // do this before you execute the uninstall or you'll lose the keys
        std::string version = queryInstalledVersion(getStudioRegistryPath());
        std::wstring filePath = getInstalledBootstrapperPath(getStudioRegistryPath());

        // execute command
        CProcessInformation pi;
	    CreateProcess(uninstall_path.c_str(),_T(""),pi); // Launch the uninstaller RobloxStudio.exe -uninstall

        // make sure the stub is deleted
        LOG_ENTRY1("Bootstrapper::forceUninstallMFCStudio - deleting studio stub: %S",filePath.c_str());
	    ::DeleteFile(filePath.c_str());

        // delete empty directory
        std::wstring basePath = baseProgramDirectory(isPerUser());
        std::wstring path = basePath + convert_s2w(version);
        LOG_ENTRY1("Bootstrapper::forceUninstallMFCStudio - deleting studio directory: %S",path.c_str());
        deleteVersionFolder(convert_w2s(path));
    }

    forceMFCStudioCleanup();
}

void Bootstrapper::forceMFCStudioCleanup()
{
    LOG_ENTRY("Bootstrapper::forceMFCStudioCleanup - deleteCurVersionKeys");
	deleteCurVersionKeys(logger, isPerUser(), getStudioCode().c_str());

	deleteLegacyShortcuts();

    std::wstring basePath = baseProgramDirectory(isPerUser());
    std::wstring fileName = _T(STUDIOBOOTSTAPPERNAME);
    std::wstring filePath = basePath + fileName;

    LOG_ENTRY1("Bootstrapper::forceMFCStudioCleanup - deleting studio stub: %S",filePath.c_str());
	if (FileSystem::IsFileExists(filePath.c_str())) 
		::DeleteFile(filePath.c_str());
}

bool Bootstrapper::hasLegacyStudioDesktopShortcut()
{
	return hasDesktopShortcut(logger, _T(STUDIOQTLINKNAME))
		|| hasDesktopShortcut(logger, _T(STUDIOQTLINKNAME20))
		|| hasDesktopShortcut(logger, _T(STUDIOQTLINKNAME20BETA))
		|| hasDesktopShortcut(logger, _T(STUDIOLINKNAMELEGACY));
}

void Bootstrapper::deleteLegacyShortcuts()
{
    LOG_ENTRY("Bootstrapper::deleteLegacyShortcuts - deleteDesktopShortcut");
	deleteDesktopShortcut(logger, _T(PLAYERLINKNAMELEGACY));
    deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME));
    deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME20));
    deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME20BETA));

    LOG_ENTRY("Bootstrapper::deleteLegacyShortcuts - deleteProgramsShortcut");
    deleteProgramsShortcut(logger, isPerUser(), _T(PLAYERLINKNAMELEGACY));
    deleteProgramsShortcut(logger, isPerUser(), _T(STUDIOQTLINKNAME));
    deleteProgramsShortcut(logger, isPerUser(), _T(STUDIOQTLINKNAME20));
    deleteProgramsShortcut(logger, isPerUser(), _T(STUDIOQTLINKNAME20BETA));
}

struct HandleInfo
{
	unsigned long pid;
	HWND hWnd;
};

BOOL CALLBACK FindHwndFromPID( HWND hwnd, LPARAM lParam)
{
	HandleInfo &info = *(HandleInfo*)lParam;
	DWORD pid = 0;

	if (GetWindowThreadProcessId(hwnd, &pid))
	{
		if (info.pid == pid)
		{
			// found
			info.hWnd = hwnd;
			return FALSE;
		}
	}

	return TRUE;
}

HWND Bootstrapper::GetHwndFromPID(DWORD pid)
{
	HandleInfo handleInfo;
	handleInfo.pid = pid;
	handleInfo.hWnd = 0;

	EnumWindows(FindHwndFromPID, (LPARAM)&handleInfo);

	return handleInfo.hWnd;
}
