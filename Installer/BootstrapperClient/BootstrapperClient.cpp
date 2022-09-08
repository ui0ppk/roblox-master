// BootstrapperClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BootstrapperClient.h"
#include "ClientInstallerSettings.h"
#include <comdef.h>
#include <taskschd.h>
#include <mstask.h>
#include <boost/function.hpp>
#include <ostream>
#include <fstream>
#include <strstream>
#include <boost/bind.hpp>
#include "SettingsLoader.h"
#include <string>
#include "HttpTools.h"
#include "ProcessInformation.h"
#include "shellapi.h"
#include <iphlpapi.h>
#include "GoogleAnalyticsHelper.h"
#include "VersionInfo.h"
#include "StringConv.h"
#include "ClientProgressDialog.h"
#include "RobloxServicesTools.h"

static const TCHAR* BootstrapperFileName    = _T("RobloxPlayerLauncher.exe");
static const TCHAR* RobloxAppFileName		= _T(PLAYEREXENAME);
static const TCHAR* BootstrapperMutexName   = _T("www.roblox.com/bootstrapper");
static const TCHAR* StartRobloxAppMutex     = _T("www.roblox.com/startRobloxApp");
static const TCHAR* LauncherFileName        = _T("RobloxProxy.dll");
static const TCHAR* LauncherFileName64      = _T("RobloxProxy64.dll");
static const TCHAR* FriendlyName            = _T("ROBLOX");
static const TCHAR* CLSID_Launcher          = _T("{76D50904-6780-4c8b-8986-1A7EE0B1716D}");
static const TCHAR* CLSID_Launcher64        = _T("{DEE03C2B-0C0C-41A9-9877-FD4B4D7B6EA3}");
static const TCHAR* AppID_Launcher          = _T("{664B192B-D17A-4921-ABF9-C6F6264E5110}");

LPCWSTR robloxUpdaterTaskName = L"RobloxGameUpdater";
LPCWSTR bgCommand = L"-qbg";

static Bootstrapper* newBootstrapper(HINSTANCE hInstance)
{
	return new BootstrapperClient(hInstance);
}
int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	if(0 == lstrcmp(lpCmdLine, _T("--version")))
	{
		CVersionInfo vi;
		vi.Load(_AtlBaseModule.m_hInst);
		std::string versionNumber = vi.GetFileVersionAsDotString();
		MessageBoxA(NULL, versionNumber.c_str(), "Version", MB_OK);
		return 0;
	}
	UNREFERENCED_PARAMETER(hPrevInstance);
	return BootstrapperMain(hInstance,nCmdShow,&newBootstrapper);
}

BootstrapperClient::BootstrapperClient(HINSTANCE hInstance)
	:Bootstrapper(hInstance)
{
	LOG_ENTRY("BootstrapperClient::BootstrapperClient");

	_protocolHandlerScheme = getPlayerProtocolScheme(BaseHost());

	//Plugin depends on RobloxReg value as well, 
	//so if you ever change this guy make sure that plugins code is updates as well
	_regSubPath = _T("RobloxReg");
	_regPath = _T("SOFTWARE\\") + _regSubPath;
	_versionFileName = _T("RobloxVersion.txt");
	_versionGuidName = _T(VERSIONGUIDNAMEPLAYER);

	SYSTEMTIME sysTime = {0};

	GetSystemTime(&sysTime);
	startHour = 10; //lets schedule it for 10AM for now

	startYear = sysTime.wYear;
	endYear = startYear + 1;
	endMonth = startMonth = sysTime.wMonth;
	endDay = startDay = sysTime.wDay;
	// lets handle leap year
	if (startMonth == 2 && startDay == 29)
	{
		endDay = 28;
	}
}

std::wstring BootstrapperClient::GetRegistryPath() const
{
	return _regPath;
}

std::wstring BootstrapperClient::GetRegistrySubPath() const
{
	return _regSubPath;
}

const wchar_t* BootstrapperClient::GetProtocolHandlerUrlScheme() const
{
	return _protocolHandlerScheme.c_str();
}

const wchar_t *BootstrapperClient::GetVersionFileName() const
{
	return _versionFileName.c_str();
}

const wchar_t *BootstrapperClient::GetGuidName() const
{
	return _versionGuidName.c_str();
}

const wchar_t *BootstrapperClient::GetStartAppMutexName() const
{
	return StartRobloxAppMutex;	
}

const wchar_t *BootstrapperClient::GetBootstrapperMutexName() const
{
	return BootstrapperMutexName;
}

const wchar_t* BootstrapperClient::GetFriendlyName() const
{
    return FriendlyName;
}

std::wstring BootstrapperClient::GetProductCodeKey() const
{
	return getPlayerInstallKey();
}

std::string BootstrapperClient::GetExpectedExeVersion()const
{
	return settings.GetValueExeVersion();
}

void BootstrapperClient::LoadSettings()
{
	try
	{
		SettingsLoader loader(BaseHost());
		settings.ReadFromStream(loader.GetSettingsString("WindowsBootstrapperSettings").c_str());

		HttpTools::httpboostPostTimeout = settings.GetValueHttpboostPostTimeout();
	}
	catch (std::exception e)
	{
	}
}

void BootstrapperClient::SetupGoogleAnalytics()
{
	GoogleAnalyticsHelper::init(settings.GetValueGoogleAnalyticsAccountPropertyID());
}

bool BootstrapperClient::ValidateInstalledExeVersion()
{
	return settings.GetValueValidateInstalledExeVersion();
}

bool BootstrapperClient::IsInstalledVersionUptodate()
{
	std::string expectedVersion = GetExpectedExeVersion();
	LOG_ENTRY1("Expected client version: %s", expectedVersion.c_str());

	if (!expectedVersion.empty())
	{
		CVersionInfo vi;
		std::wstring filePath = programDirectory() + GetRobloxAppFileName();
		if (vi.Load(filePath))
		{
			std::string fileVersion = vi.GetFileVersionAsDotString();
			if (expectedVersion != fileVersion)
			{
				LOG_ENTRY2("WARNING: RobloxApp.exe version %s != %s", expectedVersion.c_str(), fileVersion.c_str());
				return false;
			}
		}
	}

	return true;
}

std::string BootstrapperClient::BuildDateTime(int y, int m, int d)
{
	std::string sm = m < 10 ? (format_string("0%d", m)) : (format_string("%d", m));
	std::string sd = d < 10 ? (format_string("0%d", d)) : (format_string("%d", d));
	
	return format_string("%d-%s-%sT%d:00:00", y, sm.c_str(), sd.c_str(), startHour);
}

HRESULT BootstrapperClient::SheduleTaskWinVista()
{
	HRESULT hr = S_OK;
	CComPtr<ITaskService> service;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&service);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<ITaskFolder> rootFolder;
	hr = service->GetFolder(_bstr_t( L"\\"), &rootFolder);
	if (FAILED(hr))
	{
		return hr;
	}

	rootFolder->DeleteTask(_bstr_t(robloxUpdaterTaskName), 0);
	CComPtr<ITaskDefinition> task;
	hr = service->NewTask(0, &task);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IRegistrationInfo> regInfo;
	hr = task->get_RegistrationInfo(&regInfo);
	if (FAILED(hr))
	{
		return hr;
	}

	TCHAR name[MAX_PATH];
	DWORD bufSize = MAX_PATH;
	GetUserName(name, &bufSize);

	regInfo->put_Author(name);
	regInfo->put_Description(L"This tasks helps Roblox game be updated.");

	CComPtr<ITriggerCollection> triggerCollection;
	hr = task->get_Triggers(&triggerCollection);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<ITrigger> trigger;
	hr = triggerCollection->Create(TASK_TRIGGER_DAILY, &trigger);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IDailyTrigger> dailyTrigger;
	hr = trigger->QueryInterface(IID_IDailyTrigger, (void**)&dailyTrigger);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = dailyTrigger->put_Id(_bstr_t(L"RBXTrigger"));
	
	hr = dailyTrigger->put_StartBoundary(_bstr_t(CA2W(BuildDateTime(startYear, startMonth, startDay).c_str())));
	hr = dailyTrigger->put_EndBoundary(_bstr_t(CA2W(BuildDateTime(endYear, endMonth, endDay).c_str())));

	hr = dailyTrigger->put_DaysInterval((short)1);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IActionCollection> actionCollection;
	hr = task->get_Actions(&actionCollection);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IAction> action;
	hr = actionCollection->Create(TASK_ACTION_EXEC, &action);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IExecAction> execAction;
	hr = action->QueryInterface(IID_IExecAction, (void**)&execAction);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = execAction->put_Path(_bstr_t(GetSelfName().c_str()));
	if (FAILED(hr))
	{
		return hr;
	}

	hr = execAction->put_Arguments(_bstr_t(bgCommand));
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IRegisteredTask> registeredTask;
	hr = rootFolder->RegisterTaskDefinition(_bstr_t(robloxUpdaterTaskName), task, TASK_CREATE_OR_UPDATE, _variant_t(L""), _variant_t(L""), 
		TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &registeredTask);

	return hr;
}

HRESULT BootstrapperClient::SheduleTaskWinXP()
{
	HRESULT hr = S_OK;
	CComPtr<ITaskScheduler> scheduler;
	hr = CoCreateInstance(CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void **)&scheduler);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = scheduler->Delete(robloxUpdaterTaskName);

	CComPtr<ITask> task;
	hr = scheduler->NewWorkItem(robloxUpdaterTaskName, CLSID_CTask, IID_ITask, (IUnknown**)&task);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IScheduledWorkItem> item;
	hr = task->QueryInterface(IID_IScheduledWorkItem, (void**)&item);
	if (FAILED(hr))
	{
		return hr;
	}

	WORD index = 0;
	CComPtr<ITaskTrigger> trigger; 
	hr = item->CreateTrigger(&index, &trigger);
	if (FAILED(hr))
	{
		return hr;
	}

	TASK_TRIGGER triggerInfo = {0};
	triggerInfo.wBeginDay = startDay;
	triggerInfo.wBeginMonth = startMonth;
	triggerInfo.wBeginYear = startYear;
	triggerInfo.wEndDay = endDay;
	triggerInfo.wEndMonth = endMonth;
	triggerInfo.wEndYear = endYear;
	triggerInfo.cbTriggerSize = sizeof(TASK_TRIGGER); 
	triggerInfo.wStartHour = startHour;
	triggerInfo.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
	triggerInfo.TriggerType = TASK_TIME_TRIGGER_DAILY;
	triggerInfo.Type.Daily.DaysInterval = 1;

	hr = trigger->SetTrigger(&triggerInfo);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = task->SetApplicationName(GetSelfName().c_str());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = task->SetParameters(bgCommand);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = item->SetFlags(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON);
	if (FAILED(hr))
	{
		return hr;
	}

	TCHAR name[MAX_PATH];
	DWORD bufSize = MAX_PATH;
	GetUserName(name, &bufSize);

	hr = item->SetAccountInformation(name, NULL);
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IPersistFile> persistFile;
	hr = task->QueryInterface(IID_IPersistFile, (void **)&persistFile);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = persistFile->Save(NULL, TRUE);
	if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT BootstrapperClient::SheduleRobloxUpdater()
{
	if (!UpdaterActive())
	{
		return S_OK;
	}

	OSVERSIONINFO osver = {0};

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	::GetVersionEx(&osver);

	if (osver.dwMajorVersion == 5)
	{
		return SheduleTaskWinXP();
	}
	else if (osver.dwMajorVersion >= 6)
	{
		return SheduleTaskWinVista();
	}

	return S_OK;
}

HRESULT BootstrapperClient::UninstallRobloxUpdater()
{
	OSVERSIONINFO osver = {0};
	HRESULT hr = S_OK;

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	::GetVersionEx(&osver);

	if (osver.dwMajorVersion == 5)
	{
		CComPtr<ITaskScheduler> scheduler;
		hr = CoCreateInstance(CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void **)&scheduler);
		if (FAILED(hr))
		{
			return hr;
		}

		hr = scheduler->Delete(robloxUpdaterTaskName);
	}
	else if (osver.dwMajorVersion >= 6)
	{

		CComPtr<ITaskService> service;
		hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&service);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
		if (FAILED(hr))
		{
			return hr;
		}

		CComPtr<ITaskFolder> rootFolder;
		hr = service->GetFolder(_bstr_t( L"\\"), &rootFolder);
		if (FAILED(hr))
		{
			return hr;
		}

		hr = rootFolder->DeleteTask(_bstr_t(robloxUpdaterTaskName), 0);
	}

	return hr;
}

bool BootstrapperClient::NeedPreDeployRun()
{
	if (!IsPreDeployOn())
	{
		LOG_ENTRY("NeedPreDeployRun = false");
		return false;
	}

	CRegKey key;
	if (SUCCEEDED(key.Open(HKEY_CURRENT_USER, _T("Software\\ROBLOX Corporation\\Roblox"), KEY_READ)))
	{
		TCHAR buf[MAX_PATH];
		ULONG bufSize = MAX_PATH;
		if (SUCCEEDED(key.QueryStringValue(_T("LastPreVersion"), buf, &bufSize)))
		{
			std::string preVersion(settings.GetValuePreVersion());
			LOG_ENTRY2("Cure PreVersion = %s, LastPreVersion = %S", preVersion.c_str(), buf);
			if (!preVersion.empty() && preVersion != convert_w2s(buf))
			{
				LOG_ENTRY("NeedPreDeployRun = true");
				return true;
			} 

			LOG_ENTRY("NeedPreDeployRun = false");
			return false;
		}
	}

	LOG_ENTRY("NeedPreDeployRun = true");
	return true;
}

bool BootstrapperClient::IsPreDeployOn()
{
	return settings.GetValueIsPreDeployOn();
}

void BootstrapperClient::RunPreDeploy()
{
	std::string preVersion = settings.GetValuePreVersion();
	if (!IsPreDeployOn() || preVersion.empty() || preVersion == queryInstalledVersion()) 
	{
		return;
	}

	if (!NeedPreDeployRun()) 
	{
		return;
	}

	std::string version = InstallVersion();
	try
	{
		setInstallVersion(preVersion);

		deploySelf(false);
		deployRobloxProxy(false);
		deployNPRobloxProxy(false);
		DeployComponents(true, false);

		CRegKey key;
		if (SUCCEEDED(key.Create(HKEY_CURRENT_USER, _T("Software\\ROBLOX Corporation\\Roblox"))))
		{
			key.SetStringValue(_T("LastPreVersion"), convert_s2w(preVersion).c_str());
			LOG_ENTRY("Setting last pre deploy version entry");
		}
		LOG_ENTRY1("Pre deployed %s version", preVersion.c_str());
	}
	catch (std::exception e)
	{
		setInstallVersion(version);
	}
	setInstallVersion(version);
}

std::wstring BootstrapperClient::GetRobloxAppFileName() const
{
	return std::wstring(RobloxAppFileName);
}

std::wstring BootstrapperClient::GetBootstrapperFileName() const
{
	return std::wstring(BootstrapperFileName);
}

static std::wstring getValue(const std::map<std::wstring, std::wstring>& argMap, const std::wstring& key)
{
	std::map<std::wstring, std::wstring>::const_iterator iter = argMap.find(key);
	if (iter != argMap.end())
		return iter->second;

	return _T("");
}

bool BootstrapperClient::ProcessProtocolHandlerArgs(const std::map<std::wstring, std::wstring>& argMap)
{
	std::wstring launchMode = getValue(argMap, _T("launchmode"));
	if (launchMode == _T("play"))
	{
		robloxAppArgs = _T("play");
		playArgs.reset(new PlayArgs());

		playArgs->launchMode = SharedLauncher::Play;
		LOG_ENTRY("BootstrapperClient::ProcessProtocolHandlerArgs - option: play");

		std::wstring placeLauncherUrl = getValue(argMap, _T("placelauncherurl"));
		if (placeLauncherUrl.length() > 0)
		{
			TCHAR decodedUrl [1024]; 
			memset (decodedUrl, 0, sizeof (decodedUrl)); 
			DWORD dwUrlLength; 
			ATL::AtlUnescapeUrl(placeLauncherUrl.c_str(), decodedUrl, &dwUrlLength, 1024);

			playArgs->script = convert_w2s(decodedUrl);
			LOG_ENTRY1("BootstrapperClient::ProcessProtocolHandlerArgs = PlaceLauncherUrl, value = %s", playArgs->script.c_str());
		}

		playArgs->authUrl = "https://" + BaseHost() + "/Login/Negotiate.ashx";
		LOG_ENTRY1("BootstrapperClient::ProcessProtocolHandlerArgs paramIndex = AuthUrl, value = %s", playArgs->authUrl.c_str());

		std::wstring authTicket = getValue(argMap, _T("gameinfo"));
		if (authTicket.length() > 0)
		{
			playArgs->authTicket = convert_w2s(authTicket);
			LOG_ENTRY1("BootstrapperClient::ProcessProtocolHandlerArgs = gameInfo, value = %s", playArgs->authTicket.c_str());
		}

		if (BrowserTrackerId().length() > 0)
		{
			playArgs->browserTrackerId = BrowserTrackerId();
			LOG_ENTRY1("BootstrapperClient::ProcessProtocolHandlerArgs = browserTrackerId, value = %s", playArgs->browserTrackerId.c_str());
		}
	}
	else if (launchMode == _T("unhide"))
	{
		// TODO:
	}

	return true;
}

bool BootstrapperClient::ProcessArg(wchar_t** args, int &pos, int count)
{
	if (CAtlModule::WordCmpI(args[pos], _T("-play"))==0)
	{
		LOG_ENTRY("BootstrapperClient::ProcessArg - started");
		RegisterEvent(_T("BootstrapperPlayStarted"));
		robloxAppArgs = _T("play");
		playArgs.reset(new PlayArgs());

		// first, lets set the kind of play we are dealing with
		playArgs->launchMode = SharedLauncher::Play;
        LOG_ENTRY("BootstrapperClient::ProcessArg - option: play");

		if (++pos >= count)
			throw std::runtime_error("play option is missing script arg");
		playArgs->script = convert_w2s(args[pos]);
		LOG_ENTRY2("BootstrapperClient::ProcessArg = %d, value = %s", pos, playArgs->script.c_str());
		if (pos+1 < count)
		{
			playArgs->authUrl = convert_w2s(args[++pos]);
			LOG_ENTRY2("BootstrapperClient::ProcessArg paramIndex = %d, value = %s", pos, playArgs->authUrl.c_str());
		}

		if (pos+1 < count)
		{
			playArgs->authTicket = convert_w2s(args[++pos]);
			LOG_ENTRY2("Bootstrapper::parseCmdLine paramIndex = %d, value = %s", pos, playArgs->authTicket.c_str());
		}

		if (pos+1 < count)
		{
			playArgs->guidName = convert_w2s(args[++pos]);
			if (playArgs->guidName == "nonSilent") 
			{
				playArgs->guidName = "";
			}
			LOG_ENTRY2("Bootstrapper::parseCmdLine paramIndex = %d, value = %s", pos, playArgs->guidName.c_str());
		}

		if (pos+1 < count)
		{
			playArgs->hiddenStartEventName = convert_w2s(args[++pos]);
			LOG_ENTRY1("Hidden start event name = %s", playArgs->hiddenStartEventName.c_str());
			if (playArgs->hiddenStartEventName == "nonHidden")
			{
				playArgs->hiddenStartEventName = "";
			}
			LOG_ENTRY2("Bootstrapper::parseCmdLine paramIndex = %d, value = %s", pos, playArgs->hiddenStartEventName.c_str());
		}

		return true;
	}

	return false;
}

void BootstrapperClient::StartRobloxApp(bool fromInstall)
{
	if (isDontStartApp()) 
	{
		LOG_ENTRY("We are in bg update, don't need to start game.");
		return;
	}

	// disable Cancel button if we already got this far
	Dialog()->ShowCancelButton(CMainDialog::CancelHide);

	CMutex mutex(NULL, FALSE, StartRobloxAppMutex);

	CTimedMutexLock lock(mutex);
	while (lock.Lock(1) == WAIT_TIMEOUT )
	{
		LOG_ENTRY("Another process is starting Roblox. Abandoning startRobloxApp");
		return;
	}

	bool waitForApp = true;

	setStage(10);

	message("Starting ROBLOX...");

	LOG_ENTRY("Creating event");
	CEvent robloxStartedEvent(NULL, TRUE, FALSE, _T("www.roblox.com/robloxStartedEvent"));
	LOG_ENTRY("Resetting event");
	robloxStartedEvent.Reset();

	CEvent gameReady;
	if (playArgs && !playArgs->guidName.empty())
	{
		LOG_ENTRY1("Play game, syncGameEventName = %s", playArgs->guidName.c_str());
		gameReady.Open(EVENT_MODIFY_STATE, FALSE, convert_s2w(playArgs->guidName).c_str());
	}

	::AllowSetForegroundWindow(ASFW_ANY);

	CProcessInformation pi;

	if (!playArgs)
	{
		bool isIDE = robloxAppArgs.find(std::wstring(_T("-ide"))) != std::wstring::npos;
		if (!isIDE)
		{
			std::wstring url;
			if (fromInstall)
				url = format_string(_T("%S%s"), BaseHost().c_str(), _T("/download/thankyou"));
			else
				url = format_string(_T("%S%s"), BaseHost().c_str(), _T("/Games.aspx"));


			bool launcherStarted = false;
			CEvent launcherStartedEvent;
			LOG_ENTRY("Checking for launcher started...");
			if (launcherStartedEvent.Open(EVENT_MODIFY_STATE, FALSE, LAUNCHER_STARTED_EVENT_NAME))
			{
				LOG_ENTRY("Launcher started.");
				launcherStarted = true;
			}

			if (settings.GetValueShowInstallSuccessPrompt() && !launcherStarted && fromInstall)
			{
				LOG_ENTRY("Showing install success prompt.");
				Dialog()->ShowSuccessPrompt();
			}
			else
			{
				if (url.find(_T("www.")) != 0 && url.find(_T("http:")) != 0) 
				{
					url = format_string(_T("http://%s"), url.c_str());
				}
				LOG_ENTRY1("Redirectings to page url=%S", url.c_str());
				ShellExecute(0, _T("open"), url.c_str(), 0, 0, 1);
			}

			waitForApp = false;
		}
		else
		{
			if (robloxAppArgs != std::wstring(_T("-ide"))) 
			{
				LOG_ENTRY("Starting windows player (beta)");
				CreateProcess((programDirectory() + std::wstring(RobloxAppFileName)).c_str(), robloxAppArgs.c_str(), pi);

			} 
			else //this is very rare corner case, we need to launch studio bootstrapper in this case
			{
				LOG_ENTRY("Starting studio bootstrapper from client bootstrapper");
				std::string fileSubpath = std::string(STUDIOBOOTSTAPPERNAMEBETA);
				std::wstring robloxPath = baseProgramDirectory(isPerUser());
				robloxPath += convert_s2w(fileSubpath);
				LOG_ENTRY1("Checking file existance %S", robloxPath.c_str());
				if (FileSystem::IsFileExists(robloxPath.c_str()))
				{
					CreateProcess(robloxPath.c_str(), robloxAppArgs.c_str(), pi);
				}
			}
		}
	}
	else
	{
		Dialog()->ShowWindow(CMainDialog::WindowSomethingInterestingShow);

		std::string gameMode = "play";
        LOG_ENTRY("Starting in play configuration");

		std::wstring startArgs = format_string(_T("--%S -a %S -t %S -j %S"), gameMode.c_str(), playArgs->authUrl.c_str(), playArgs->authTicket.c_str(), playArgs->script.c_str());
		if (!playArgs->hiddenStartEventName.empty())
		{
			startArgs.append(format_string(_T(" -w %S"), playArgs->hiddenStartEventName.c_str()));
		}
		if (!playArgs->browserTrackerId.empty())
		{
			startArgs.append(format_string(_T(" -b %S"), playArgs->browserTrackerId.c_str()));
		}
		LOG_ENTRY1("playArgs = %S", startArgs.c_str());

		LOG_ENTRY("Starting windows player (beta)");
		CreateProcess((programDirectory() + std::wstring(RobloxAppFileName)).c_str(), startArgs.c_str(), pi);
	}

	// TODO: Allow cancel of start process???
	Dialog()->ShowCancelButton(CMainDialog::CancelHide);

	// wait for Roblox to start
	// This event is set in MainFrm.cpp
	if (waitForApp) 
	{
		LOG_ENTRY("startRobloxApp waiting for robloxStartedEvent");
		influxDb.addPoint("WaitingForAppStart", GetElapsedTime() / 1000.0f);
		::WaitForSingleObject(robloxStartedEvent, 10*1000);
		influxDb.addPoint("WaitingForAppEnd", GetElapsedTime() / 1000.0f);
	}

	// find hwnd of launched process
	launchedAppHwnd = GetHwndFromPID(pi.pi.dwProcessId);

	if (playArgs && !playArgs->guidName.empty())
	{
		LOG_ENTRY("Setting game ready event");
		gameReady.Set();
	}

	Dialog()->ShowCancelButton(CMainDialog::CancelShow);
	//start Bootstrapper in HERE to do predeploy if
	if (NeedPreDeployRun())
	{
		std::wstring args = _T("-qbgnorun");
		CProcessInformation pi;
		TCHAR pathBuffer[MAX_PATH];
		::GetModuleFileName(0, pathBuffer, MAX_PATH);

		LOG_ENTRY1("Starting up pre deploy bootstrapper, path = %s", pathBuffer);
		CreateProcess(pathBuffer, args.c_str(), pi);
	}
}

// installs bootstrapper for any version of studio
void BootstrapperClient::deployExtraStudioBootstrapper(std::string exeName, TCHAR *linkName, std::wstring componentId, bool forceDesktopIconCreation, const TCHAR *registryPath)
{
	LOG_ENTRY3("BootstrapperClient::deployExtraStudioBootstrapper name = %s, linkName = %S, component = %S", exeName.c_str(), linkName, componentId.c_str());
	bool success = true;
	RegisterEvent(_T("BootstrapperDeployStudioBootstrapperStarted"));
	try
	{
		LOG_ENTRY3("BootstrapperClient::deployExtraStudioBootstrapper name = %s, linkName = %S, component = %S", exeName.c_str(), linkName, componentId.c_str());
		std::string fileSubpath = exeName;
		std::wstring exePath = baseProgramDirectory(isPerUser());
		exePath += convert_s2w(fileSubpath);

		TCHAR version[MAX_PATH] = {0};
		TCHAR url[MAX_PATH] = {0};
        // look in registry for current version
		getCurrentVersion(logger, isPerUser(), componentId.c_str(), version, sizeof(version)/sizeof(version[0]), url, sizeof(url)/sizeof(url[0]));
		LOG_ENTRY2("BootstrapperClient::deployExtraStudioBootstrapper - Studio version = %S, url = %S", version, url);

		try 
		{
			// delete 2013 shortcut if any
			deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME2013));
			deleteProgramsShortcut(logger, isPerUser(), _T(STUDIOQTLINKNAME2013));
		}
		catch (std::exception& e)
		{
			LOG_ENTRY1("BootstrapperClient::deployExtraStudioBootstrapper - failed to delete shortcut: %s", e.what());
		}
	
		bool studioOutOfDate = false;

		if (settings.GetValueCheckIsStudioOutofDate())
		{
			CVersionInfo vi;
			if (vi.Load(exePath))
			{
				std::string v = vi.GetFileVersionAsDotString();
				if (v != settings.GetValueExeVersion())
					studioOutOfDate = true;
			}
		}

		if (!std::wstring(version).empty() && !studioOutOfDate && std::wstring(url) == convert_s2w(BaseHost())) 
		{
			LOG_ENTRY("BootstrapperClient::deployExtraStudioBootstrapper - Studio bootstrapper version is present and URL is current one");
			if (FileSystem::IsFileExists(exePath.c_str())) 
			{
				LOG_ENTRY("BootstrapperClient::deployExtraStudioBootstrapper - Studio bootstrapper is present. updating links and leaving");
				createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), _T("-ide"), false, true);
				createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), _T("-ide"), true, forceDesktopIconCreation);
				return;
			}
		}

		std::wstring studioBootstrapper = simple_logger<wchar_t>::get_temp_filename(_T("tmp"));
		{
			std::ofstream bootstrapperFile(studioBootstrapper.c_str(), std::ios::binary);
			// This version of the downloader doesn't show progress
			// Fetch the most recent version of the exe
			std::string versionedExeName = format_string("/%s-%s", fetchVersionGuid(VERSIONGUIDNAMESTUDIO).c_str(), exeName.c_str());
			HttpTools::httpGetCdn(this, InstallHost(), versionedExeName.c_str(), std::string(), bootstrapperFile, false, boost::bind(&Bootstrapper::dummyProgress, _1, _2));
		}

		if (FileSystem::IsFileExists(exePath.c_str()))
		{
			LOG_ENTRY("BootstrapperClient::deployExtraStudioBootstrapper - deleting old studio bootstrapper");
			DeleteFile(exePath.c_str());
		}

		copyFile(logger, studioBootstrapper.c_str(), exePath.c_str());
		setCurrentVersion(logger, isPerUser(), componentId.c_str(), _T(""), _T(""));

        createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), _T("-ide"), false, true);
        createRobloxShortcut(logger, isPerUser(), linkName, exePath.c_str(), _T("-ide"), true, forceDesktopIconCreation);

		// create registry info, so we can launch properly from web
        std::wstring progName = convert_s2w(exeName);
		CRegKey installKey = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, registryPath, (baseProgramDirectory(isPerUser()) + progName).c_str());
		throwHRESULT(installKey.SetStringValue(_T("install host"), convert_s2w(InstallHost()).c_str()), "Failed to set install host key value");
		// update registry to point to stub studio bootstrapper
		HKEY curKey = HKEY_LOCAL_MACHINE;
		if (isPerUser())
		{
			curKey = HKEY_CURRENT_USER;
		}
		CRegKey key;
		LONG result = key.Open(curKey, registryPath, KEY_WRITE);
		if (ERROR_SUCCESS == result)
		{
			LOG_ENTRY("BootstrapperClient::deployExtraStudioBootstrapper - studio reg key opened");
			result = key.SetStringValue(_T(""), exePath.c_str());
			if (SUCCEEDED(result))
			{
				LOG_ENTRY1("BootstrapperClient::deployExtraStudioBootstrapper - studio reg key set to %S", exePath.c_str());
			}
			else
			{
				LOG_ENTRY1("BootstrapperClient::deployExtraStudioBootstrapper - studio reg key write failed, error=0x%X", result);
			}
		}
		else
		{
			LOG_ENTRY1("BootstrapperClient::deployExtraStudioBootstrapper - studio reg key open failed, error=0x%X", result);
		}

		// create protocol handler registry 
		RegisterProtocolHandler(getQTStudioProtocolScheme(BaseHost()), exePath, registryPath);
	}
	catch (std::exception &e)
	{
		success = false;
		LOG_ENTRY1("BootstrapperClient::deployStudioBootstrapper - failed exception=%s", e.what());
	}

	if (success)
	{
		RegisterEvent(_T("BootstrapperDeployStudioBootstrapperSuccess"));
	}
	else
	{
		RegisterEvent(_T("BootstrapperDeployStudioBootstrapperFailure"));
	}
}

void BootstrapperClient::deployStudioBetaBootstrapper(bool forceDesktopIconCreation)
{
	deployExtraStudioBootstrapper(
        std::string(STUDIOBOOTSTAPPERNAMEBETA),
        _T(STUDIOQTLINKNAME_CUR), 
        getQTStudioCode(), 
        forceDesktopIconCreation, 
        getQTStudioRegistryPath().c_str() );
}

void BootstrapperClient::deployRobloxProxy(bool commitData)
{
	deployer->deployVersionedFile(_T("RobloxProxy.zip"), NULL, Progress(), commitData);
	CheckCancel();

	if (!commitData)
	{
		return;
	}

	if (!isPerUser())
	{
		//Global install in process, so remove local settings to prevent conflicts
		CRegKey ckey;
		if (!FAILED(ckey.Open(HKEY_CURRENT_USER, _T("Software\\Classes"))))
			proxyModule.unregisterModule(ckey, true, logger);
	}

	proxyModule.registerModule(classesKey, programDirectory(), logger);

	// create the 64 bit keys
	CRegKey classesKey64;
	throwHRESULT(classesKey64.Create(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\Classes"), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY), "Failed to create 64 bits HK**\\Software\\Classes");
	LOG_ENTRY1("classesKey64 = %s\\Software\\Classes", (isPerUser() ? "HKEY_CURRENT_USER" : "HKEY_LOCAL_MACHINE"));
	proxyModule64.registerModule(classesKey64, programDirectory(), logger);

	{
		// IE hack to avoid DLL security warning
		// Ideally we'd just register the DLL as pre-approved, but this doesn't work in HKCU, only HKLM
		CRegKey key;
		throwHRESULT(key.Create(HKEY_CURRENT_USER, format_string(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Ext\\Stats\\%s\\iexplore"), CLSID_Launcher).c_str()), "Failed to create key for iexplorer stats");
		key.SetDWORDValue(_T("Flags"), 4);

		// For IE8:
		CRegKey allowedDomainsKey = CreateKey(key, _T("AllowedDomains"));
		CreateKey(allowedDomainsKey, _T("roblox.com"));
		CreateKey(allowedDomainsKey, _T("robloxlabs.com"));
	}

	CVersionInfo vi;
	std::wstring filePath = programDirectory() + _T("\\RobloxProxy.dll");
	if (vi.Load(filePath))
	{
		std::string version = vi.GetFileVersionAsString();
		CRegKey installKey;
		if (!FAILED(installKey.Open(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, GetRegistryPath().c_str())))
			installKey.SetStringValue(_T("Plug-in version"), convert_s2w(version).c_str());
	}
}

void BootstrapperClient::deployNPRobloxProxy(bool commitData)
{
	deployer->deployVersionedFile(_T("NPRobloxProxy.zip"), NULL, Progress(), commitData);
	CheckCancel();

	if (!commitData)
	{
		return;
	}

	unregisterFirefoxPlugin(_T(FIREFOXREGKEY), false);
	unregisterFirefoxPlugin(_T(FIREFOXREGKEY64), true);

	registerFirefoxPlugin(_T(FIREFOXREGKEY), false);
	registerFirefoxPlugin(_T(FIREFOXREGKEY64), true);
}

void BootstrapperClient::registerFirefoxPlugin(const TCHAR* id, bool is64Bits)
{
	HKEY parent = HKEY_CURRENT_USER;
	CRegKey key = CreateKey(parent, format_string(_T("SOFTWARE\\MozillaPlugins\\%s"), id).c_str(), NULL, is64Bits);

	key.SetStringValue(_T("ProductName"), _T("Launcher"));
	key.SetStringValue(_T("Description"), _T("Roblox Launcher"));
	key.SetStringValue(_T("Vendor"), _T("Roblox"));
	key.SetStringValue(_T("Version"), _T("1"));

	if (is64Bits)
		key.SetStringValue(_T("Path"), (programDirectory() + _T("\\NPRobloxProxy64.dll")).c_str());
	else
		key.SetStringValue(_T("Path"), (programDirectory() + _T("\\NPRobloxProxy.dll")).c_str());

	CreateKey(parent, format_string(_T("SOFTWARE\\MozillaPlugins\\%s\\MimeTypes"), id).c_str(), NULL, is64Bits);
	CreateKey(parent, format_string(_T("SOFTWARE\\MozillaPlugins\\%s\\MimeTypes\\application/x-vnd-roblox-launcher"), id).c_str(), NULL, is64Bits);
}

void BootstrapperClient::unregisterFirefoxPlugin(const TCHAR* id, bool is64Bits)
{
	CRegKey mozillaKey;
	REGSAM sam = KEY_READ | KEY_WRITE;
	if (is64Bits)
		sam |= KEY_WOW64_64KEY;

	if(!FAILED(mozillaKey.Open(HKEY_CURRENT_USER,_T("Software\\MozillaPlugins"), sam))){
		DeleteKey(logger, mozillaKey, id);
	}
}

void BootstrapperClient::DeployComponents(bool isUpdating, bool commitData)
{
	std::vector<std::pair<std::wstring, std::wstring> > files;
	createDirectory((programDirectory() + _T("content")).c_str());
	createDirectory((programDirectory() + _T("content\\fonts")).c_str());
	createDirectory((programDirectory() + _T("content\\music")).c_str());
	createDirectory((programDirectory() + _T("content\\particles")).c_str());
	createDirectory((programDirectory() + _T("content\\sky")).c_str());
	createDirectory((programDirectory() + _T("content\\sounds")).c_str());
	createDirectory((programDirectory() + _T("content\\textures")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc\\textures")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc\\terrain")).c_str());
	createDirectory((programDirectory() + _T("shaders")).c_str());

	files.push_back(std::pair<std::wstring, std::wstring>(_T("redist.zip"), _T("")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("RobloxApp.zip"), _T("")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("Libraries.zip"), _T("")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-fonts.zip"), _T("content\\fonts\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-music.zip"), _T("content\\music\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-particles.zip"), _T("content\\particles\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-sky.zip"), _T("content\\sky\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-sounds.zip"), _T("content\\sounds\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-textures.zip"), _T("content\\textures\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-textures2.zip"), _T("content\\textures\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-textures3.zip"), _T("PlatformContent\\pc\\textures\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-terrain.zip"), _T("PlatformContent\\pc\\terrain\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("shaders.zip"), _T("shaders\\")));

	DoDeployComponents(files, isUpdating, commitData);
}

void BootstrapperClient::DoInstallApp()
{
	LOG_ENTRY("BootstrapperClient::DoInstallApp");
	const bool isUpdating = !queryInstalledVersion().empty();

	validateAndFixChromeState();

	deploySelf(true);
	CheckCancel();

	setStage(2);
	
	// setup studio bootstrappers so we can launch studio (has to be done before we do much else, to make sure we launch right application)
	installStudioLauncher(isUpdating);

	// "install key" and "install host" must be set before RobloxProxy is deployed, because RobloxProxy uses these values
	LOG_ENTRY("set install key");
	CRegKey installKey = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, GetRegistryPath().c_str(), (programDirectory() + BootstrapperFileName).c_str());
	LOG_ENTRY("set install host key value");
	throwHRESULT(installKey.SetStringValue(_T("install host"), convert_s2w(InstallHost()).c_str()), "Failed to set install host key value");
	CheckCancel();
	
	// Now deploy RobloxProxy so that javascript clients can get going ASAP (has to go after studio bootstrappers in case we are in editMode)
	LOG_ENTRY("deployRobloxProxy()");
	deployRobloxProxy(true);
	CheckCancel();

	LOG_ENTRY("deployNPRobloxProxy()");
	deployNPRobloxProxy(true);
	CheckCancel();

	setStage(3);

	// Now do the time-consuming download of all needed components
	LOG_ENTRY("BootstrapperClient::DoInstallApp - DeployComponents()");
	DeployComponents(isUpdating, true);
	CheckCancel();

	LOG_ENTRY("writeAppSettings()");
	writeAppSettings();
	CheckCancel();

	setStage(4);

	std::wstring dir = programDirectory();
	RegisterElevationPolicy(RobloxAppFileName, dir.c_str());
	CheckCancel();

	LOG_ENTRY("BootstrapperClient::DoInstallApp - RegisterProtocolHandler()");
	RegisterProtocolHandler(GetProtocolHandlerUrlScheme(), programDirectory() + BootstrapperFileName, GetRegistryPath());

	//%%%%%%%%%%%%%%%%% POINT OF NO RETURN %%%%%%%%%%%%%%%%%%%%%
	// No cancels allowed after this point
	// TODO: Uninstall on failure???
	setStage(5);
	LOG_ENTRY("registerUninstall()");
	RegisterUninstall(_T(PLAYERLINKNAME_CUR));

    setStage(6);
	try		
	{
        LOG_ENTRY("createShortcuts()");
		CreateShortcuts(!isUpdating, _T(PLAYERLINKNAME_CUR), BootstrapperFileName, _T("-browser"));
	}
	catch(std::exception& ex)
	{
		LOG_ENTRY1("Error: %s", exceptionToString(ex).c_str());
	}

	setStage(7);

	// The very last thing to do is to set the version number
	LOG_ENTRY("set version key value");
	throwHRESULT(installKey.SetStringValue(_T("version"), convert_s2w(InstallVersion()).c_str()), "Failed to set version key value");
	setCurrentVersion(logger, isPerUser(), getPlayerCode().c_str(), convert_s2w(InstallVersion()).c_str(), convert_s2w(BaseHost()).c_str());
	setStage(8);

	HRESULT hr = SheduleRobloxUpdater();
	LOG_ENTRY1("SheduleRobloxUpdater hr = 0x%8.8x", hr);
}

void BootstrapperClient::installStudioLauncher(bool isUpdating)
{
	try
	{
		deployStudioBetaBootstrapper(!isUpdating || hasLegacyStudioDesktopShortcut());
	}
	catch (std::exception& e)
	{
		LOG_ENTRY1("Error while deployStudioBetaBootstrapper: %s", exceptionToString(e).c_str());
	}
}

void BootstrapperClient::DoUninstallApp(CRegKey &hk)
{
	LOG_ENTRY("BootstrapperClient::DoUninstallApp");

	{
		CRegKey ckey;
		if (FAILED(ckey.Open(hk, _T("Software\\Classes"))))
		{
			LOG_ENTRY("WARNING: Failed to open HK**\\Software\\Classes");
		}
		else
		{
			//unregisterFileTypes(ckey);
			proxyModule.unregisterModule(ckey, isPerUser(), logger);

			unregisterFirefoxPlugin(_T(FIREFOXREGKEY), false);
			unregisterFirefoxPlugin(_T(FIREFOXREGKEY64), true);
		}

		// unregister 64 bit proxy module
		CRegKey classesKey64;
		if (!FAILED(classesKey64.Open(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\Classes"), KEY_READ | KEY_WRITE | KEY_WOW64_64KEY)))
		{
			proxyModule64.unregisterModule(classesKey64, isPerUser(), logger);
		}
	}

	LOG_ENTRY("BootstrapperClient::DoUninstallApp - UnregisterProtocolHandler");
	UnregisterProtocolHandler(GetProtocolHandlerUrlScheme());

	LOG_ENTRY("Deleting roblox auto updater");
	UninstallRobloxUpdater();

	LOG_ENTRY("BootstrapperClient::DoUninstallApp - deleteCurVersionKeys");
	deleteCurVersionKeys(logger, isPerUser(), getPlayerCode().c_str());

	LOG_ENTRY("BootstrapperClient::DoUninstallApp - deleteDesktopShortcut");
	deleteDesktopShortcut(logger, _T(PLAYERLINKNAME_CUR));

	LOG_ENTRY("BootstrapperClient::DoUninstallApp - deleteProgramsShortcut");
	deleteProgramsShortcut(logger, isPerUser(), _T(PLAYERLINKNAME_CUR));

	// now check our studio bootstrappers, see if this needs to be removed as well (only if never installed!)
	// essentially we check the registry key to see if studio was ever installed, if not we remove the registry key
	// todo: turn this back on when we can do this in separated release! (US13812)
	//checkStudioForUninstall(hk);
	
    LOG_ENTRY("BootstrapperClient::DoUninstallApp - deleting program keys");
    CRegKey key;
	key.Open(hk, _T("Software"),KEY_WRITE);
    DeleteKey(logger, key, GetRegistrySubPath().c_str());
}

void BootstrapperClient::checkStudioForUninstall(CRegKey &hk)
{
// 	cleanKey( getStudioRegistrySubPath(), hk);
// 	cleanKey( getQTStudioRegistrySubPath(), hk );
}

void BootstrapperClient::RegisterEvent(const TCHAR *eventName)
{
	LOG_ENTRY1("BootstrapperClient::RegisterEvent event=%S", eventName);
	counters->registerEvent(eventName, settings.GetValueCountersFireImmediately());
}

void BootstrapperClient::initialize()
{
	LOG_ENTRY("BootstrapperClient::initialize");
	counters.reset(new CountersClient(BaseHost(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", &logger));

	{
		proxyModule.appName = "RobloxProxy";
		proxyModule.fileName = _T("RobloxProxy.DLL");
		proxyModule.clsid = CLSID_Launcher;
		proxyModule.typeLib = "{731B317A-E2B8-4BF7-A2C4-B47C225DDAFF}"; 
		proxyModule.typeLibVersion = "1.0"; 
		proxyModule.appID = AppID_Launcher;
		proxyModule.interfaces.push_back(ComModule::Interface("ILauncher", "{699F0898-B7BC-4DE5-AFEE-0EC38AD42240}", false));
		proxyModule.interfaces.push_back(ComModule::Interface("_ILauncherEvents", "{6E9600BE-5654-47F0-9A68-D6DC25FADC55}", true));
		proxyModule.progID = "RobloxProxy.Launcher.4";
		proxyModule.versionIndependentProgID = "RobloxProxy.Launcher";
		proxyModule.name = _T("Launcher Class");
		proxyModule.moduleName = LauncherFileName;
		proxyModule.isDll = true;
		proxyModule.is64Bits = false;

		proxyModule64 = proxyModule;
		proxyModule64.clsid = CLSID_Launcher64;
		proxyModule64.moduleName = LauncherFileName64;
		proxyModule64.fileName = _T("RobloxProxy64.DLL");
		proxyModule64.is64Bits = true;
	}

	Bootstrapper::initialize();
}

void BootstrapperClient::createDialog()
{
	dialog.reset(new CClientProgressDialog(hInstance, this));
	dialog->InitDialog();
}

void BootstrapperClient::FlushEvents()
{
	int load = settings.GetValueCountersLoad();
	if (load > 100) // this should not be the case ever
	{
		load = 100;
	}

	if (load > 0) 
	{
		if ((rand() % 100/load) == 0)
		{
			counters->reportEvents();
		}
	}
}

bool BootstrapperClient::PerModeLoggingEnabled() {
	return settings.GetValuePerModeLoggingEnabled();
}

bool BootstrapperClient::UseNewCdn() {
	return settings.GetValueUseNewCdn();
}

