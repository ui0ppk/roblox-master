// BootstrapperQTStudio.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BootstrapperQTStudio.h"
#include "ProcessInformation.h"
#include "SettingsLoader.h"
#include "Bootstrapper.h"
#include "HttpTools.h"
#include "StudioProgressDialog.h"
#include "VersionInfo.h"

// TODO - rename to just "Roblox Studio"
static const TCHAR* BootstrapperQTStudioFileName    = _T(STUDIOBOOTSTAPPERNAMEBETA);
static const TCHAR* RobloxStudioAppFileName         = _T(STUDIOQTEXENAME);
static const TCHAR* BootstrapperMutexName           = _T("www.roblox.com/bootstrapperQTStudio");
static const TCHAR* StartRobloxStudioAppMutex       = _T("www.roblox.com/startRobloxQTStudioApp");
static const TCHAR* FriendlyName                    = _T("ROBLOX Studio");
static const TCHAR* StartEvent                      = _T("www.roblox.com/robloxQTStudioStartedEvent");

static Bootstrapper* newBootstrapper(HINSTANCE hInstance)
{
	return new BootstrapperQTStudio(hInstance);
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

BootstrapperQTStudio::BootstrapperQTStudio(HINSTANCE hInstance)
	:Bootstrapper(hInstance)
{
    LOG_ENTRY("BootstrapperQTStudio::BootstrapperQTStudio");

	_versionFileName = _T("BootstrapperQTStudioVersion.txt");
	_versionGuidName = _T(VERSIONGUIDNAMESTUDIO);

	_protocolHandlerScheme = getQTStudioProtocolScheme(BaseHost());
}

std::wstring BootstrapperQTStudio::GetRegistryPath() const
{
	return getQTStudioRegistryPath();
}

std::wstring BootstrapperQTStudio::GetRegistrySubPath() const
{
	return getQTStudioRegistrySubPath();
}

const wchar_t *BootstrapperQTStudio::GetVersionFileName() const
{
	return _versionFileName.c_str();
}

const wchar_t *BootstrapperQTStudio::GetGuidName() const
{
	return _versionGuidName.c_str();
}

const wchar_t *BootstrapperQTStudio::GetStartAppMutexName() const
{
	return StartRobloxStudioAppMutex;
}

const wchar_t *BootstrapperQTStudio::GetBootstrapperMutexName() const
{
	return BootstrapperMutexName;
}

std::wstring BootstrapperQTStudio::GetBootstrapperFileName() const
{
	return std::wstring(BootstrapperQTStudioFileName);
}

std::wstring BootstrapperQTStudio::GetRobloxAppFileName() const 
{ 
	return std::wstring(RobloxStudioAppFileName);
}

const wchar_t* BootstrapperQTStudio::GetFriendlyName() const
{
    return FriendlyName;
}

std::wstring BootstrapperQTStudio::GetProductCodeKey() const
{
	return getQTStudioInstallKey();
}

const wchar_t* BootstrapperQTStudio::GetProtocolHandlerUrlScheme() const
{
	return _protocolHandlerScheme.c_str();
}

void BootstrapperQTStudio::LoadSettings()
{
	try
	{
		SettingsLoader loader(BaseHost());
		settings.ReadFromStream(loader.GetSettingsString("WindowsStudioBootstrapperSettings").c_str());

		HttpTools::httpboostPostTimeout = settings.GetValueHttpboostPostTimeout();
	}
	catch (std::exception e)
	{
	}
}

void BootstrapperQTStudio::RegisterEvent(const TCHAR *eventName)
{
	std::wstring event = format_string(_T("QTStudio%s"), eventName);
	LOG_ENTRY1("BootstrapperQTStudio::RegisterEvent event=%S", event.c_str());
	counters->registerEvent(event.c_str(), settings.GetValueCountersFireImmediately());
}

void BootstrapperQTStudio::FlushEvents()
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

bool BootstrapperQTStudio::PerModeLoggingEnabled() {
	return settings.GetValuePerModeLoggingEnabled();
}

void BootstrapperQTStudio::initialize()
{
	LOG_ENTRY("BootstrapperQTStudio::initialize");
	counters.reset(new CountersClient(BaseHost(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", &logger));

	Bootstrapper::initialize();
}

void BootstrapperQTStudio::createDialog()
{
	dialog.reset(new CStudioProgressDialog(hInstance, this));
	dialog->InitDialog();
}

void BootstrapperQTStudio::StartRobloxApp(bool fromInstall)
{
	LOG_ENTRY("BootstrapperQTStudio::StartRobloxApp");

	if (settings.GetValueShowInstallSuccessPrompt() && fromInstall)
	{
		dialog->ShowSuccessPrompt();
		return;
	}

    CEvent startEvent(NULL, TRUE, FALSE, StartEvent);
	startEvent.Reset();

    CEvent readyEvent;
	if ( !editArgs.readyEvent.empty() )
	{
		LOG_ENTRY1("BootstrapperStudio::StartRobloxApp - Play game, syncGameEventName = %s", editArgs.readyEvent.c_str());
		readyEvent.Open(EVENT_MODIFY_STATE, FALSE, convert_s2w(editArgs.readyEvent).c_str());
	}

    robloxAppArgs += _T(" ") + convert_s2w(SharedLauncher::StartEventArgument) + _T(" ") + StartEvent;
    robloxAppArgs += _T(" ") + SharedLauncher::generateEditCommandLine(editArgs);
 
	CProcessInformation pi;
	CreateProcess((programDirectory() + GetRobloxAppFileName()).c_str(), robloxAppArgs.c_str(), pi);

	LOG_ENTRY1("BootstrapperQTStudio::StartRobloxApp - waiting for %S",StartEvent);
	influxDb.addPoint("WaitingForAppStart", GetElapsedTime() / 1000.0f);
	::WaitForSingleObject(startEvent,10 * 1000);
	influxDb.addPoint("WaitingForAppEnd", GetElapsedTime() / 1000.0f);

    if ( !editArgs.readyEvent.empty() )
	{
		LOG_ENTRY("BootstrapperStudio::StartRobloxApp - Setting ready event");
		readyEvent.Set();
	}
}

void BootstrapperQTStudio::DeployComponents(bool isUpdating, bool commitData)
{
	std::vector<std::pair<std::wstring, std::wstring> > files;
	createDirectory((programDirectory() + _T("content")).c_str());
	createDirectory((programDirectory() + _T("content\\fonts")).c_str());
	createDirectory((programDirectory() + _T("content\\music")).c_str());
	createDirectory((programDirectory() + _T("content\\particles")).c_str());
	createDirectory((programDirectory() + _T("content\\sky")).c_str());
	createDirectory((programDirectory() + _T("content\\sounds")).c_str());
	createDirectory((programDirectory() + _T("content\\textures")).c_str());
	createDirectory((programDirectory() + _T("content\\scripts")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc\\textures")).c_str());
	createDirectory((programDirectory() + _T("PlatformContent\\pc\\terrain")).c_str());
	createDirectory((programDirectory() + _T("shaders")).c_str());
	createDirectory((programDirectory() + _T("BuiltInPlugins")).c_str());
	createDirectory((programDirectory() + _T("imageformats")).c_str());

	files.push_back(std::pair<std::wstring, std::wstring>(_T("redist.zip"), _T("")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("RobloxStudio.zip"), _T("")));
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
	files.push_back(std::pair<std::wstring, std::wstring>(_T("content-scripts.zip"), _T("content\\scripts\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("shaders.zip"), _T("shaders\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("BuiltInPlugins.zip"), _T("BuiltInPlugins\\")));
	files.push_back(std::pair<std::wstring, std::wstring>(_T("imageformats.zip"), _T("imageformats\\")));

	DoDeployComponents(files, isUpdating, commitData);
}

void BootstrapperQTStudio::registerFileTypes()
{
    // register file extensions
	{
        // rbxl
		CRegKey rbxl = CreateKey(classesKey, _T(".rbxl"), _T("Roblox.Place"));
		CRegKey roblox_place = CreateKey(rbxl, _T("Roblox.Place"), NULL);
		CreateKey(roblox_place, _T("ShellNew"), NULL);
	}

    // register file types
	{
        // Roblox.Place
		CRegKey roblox_place = CreateKey(classesKey, _T("Roblox.Place"), _T("Roblox Place"));

		CRegKey defaultIcon = CreateKey(
            roblox_place, 
            _T("DefaultIcon"), 
            format_string(_T("%s%s,0"),programDirectory().c_str(),RobloxStudioAppFileName).c_str() );

		CRegKey shell = CreateKey(roblox_place, _T("shell"), NULL);
		CRegKey open = CreateKey(shell, _T("Open"), _T("Open"));

        std::wstring commandline = format_string(
            _T("\"%s%s\" %s \"%%1\""), 
            programDirectory().c_str(), 
            GetBootstrapperFileName().c_str(),
            convert_s2w(SharedLauncher::IDEArgument).c_str() );

		CRegKey command = CreateKey(open,_T("command"),commandline.c_str());
	}
}

void BootstrapperQTStudio::unregisterFileTypes()
{
	unregisterFileTypes(classesKey);
}

void BootstrapperQTStudio::unregisterFileTypes(HKEY ckey)
{
	DeleteKey(logger, classesKey, _T(".rbxl")); 
	DeleteKey(logger, classesKey, _T("Roblox.Place")); 
}

void BootstrapperQTStudio::DoInstallApp()
{
	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp");
	const bool isUpdating = !queryInstalledVersion().empty();

	validateAndFixChromeState();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - set install key");
	CRegKey installKey = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, GetRegistryPath().c_str(), (programDirectory() + BootstrapperQTStudioFileName).c_str());
	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - set install host key value");
	throwHRESULT(installKey.SetStringValue(_T("install host"), convert_s2w(InstallHost()).c_str()), "Failed to set install host key value");
	CheckCancel();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - deploying studio bootstrapper");
	deploySelf(true);
	CheckCancel();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - DeployComponents()");
	DeployComponents(isUpdating, true);
	CheckCancel();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - writeAppSettings()");
	writeAppSettings();
	CheckCancel();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - RegisterElevationPolicy");
	std::wstring dir = programDirectory();
	RegisterElevationPolicy(RobloxStudioAppFileName, dir.c_str());
	CheckCancel();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - registerFileTypes()");
	registerFileTypes();

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - RegisterProtocolHandler()");
	RegisterProtocolHandler(GetProtocolHandlerUrlScheme(), programDirectory() + BootstrapperQTStudioFileName, GetRegistryPath());

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - registerUninstall");
	RegisterUninstall(_T(STUDIOQTLINKNAME_CUR));

	try
	{
		bool hasLegacyShortcut = hasLegacyStudioDesktopShortcut();
		deleteLegacyShortcuts();
		// Force creation of new shortcuts if they had an old one (now deleted), or if it's an install
		CreateShortcuts(!isUpdating || hasLegacyShortcut, _T(STUDIOQTLINKNAME_CUR), BootstrapperQTStudioFileName, _T("-ide"));
	}
	catch (std::exception &)
	{
		LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - CreateShortcuts failed");
	}

	//lets clean up stuff from old temp beta bootstrapper - even if it was not uninstalled
	try
	{
		LOG_ENTRY("Cleaning up old beta leftovers");

		LOG_ENTRY("Looking for old beta programs link folder");
		std::wstring shortcuts = FileSystem::getSpecialFolder(isPerUser() ? FileSystem::RobloxUserPrograms : FileSystem::RobloxCommonPrograms, false, NULL, false);
		shortcuts += _T("RobloxBeta\\");
		if (FileSystem::IsFolderExists(shortcuts.c_str())) 
		{
			deleteDirectory(shortcuts, false);
		}

		LOG_ENTRY("Looking for old beta data folder");
		std::wstring oldData = FileSystem::getSpecialFolder(isPerUser() ? FileSystem::RobloxUserApplicationData : FileSystem::RobloxProgramFiles, false, "StudioBetaVersions");
		if (FileSystem::IsFolderExists(oldData.c_str())) 
		{
			deleteDirectory(oldData, false);
		}

		LOG_ENTRY("Deleting old beta uninstall entry");
		CRegKey keyProductCode = CreateKey(isPerUser() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"), NULL);
		DeleteKey(logger, keyProductCode, _T("{6B64A6DF-79A4-418A-9A30-C7CFFAB603E9}"));
	}
	catch (std::runtime_error &e)
	{
		LOG_ENTRY1("Exception during old beta cleanup: %s", e.what());
	}

	LOG_ENTRY("BootstrapperQTStudio::DoInstallApp - set version key value");
	throwHRESULT(installKey.SetStringValue(_T("version"), convert_s2w(InstallVersion()).c_str()), "Failed to set version key value");
	setCurrentVersion(logger, isPerUser(), getQTStudioCode().c_str(), convert_s2w(InstallVersion()).c_str(), convert_s2w(BaseHost()).c_str());
}

void BootstrapperQTStudio::DoUninstallApp(CRegKey &hk)
{
	LOG_ENTRY("BootstrapperQTStudio::DoUninstallApp");

	LOG_ENTRY("BootstrapperQTStudio::DoUninstallApp - unregisterFileTypes()");
	unregisterFileTypes();

	LOG_ENTRY("BootstrapperQTStudio::DoUninstallApp - UnregisterProtocolHandler");
	UnregisterProtocolHandler(GetProtocolHandlerUrlScheme());

	LOG_ENTRY("BootstrapperQTStudio::DoUninstallApp - deleteCurVersionKeys");
	deleteCurVersionKeys(logger, isPerUser(), getQTStudioCode().c_str());

    deleteLegacyShortcuts();

	LOG_ENTRY("BootstrapperQTStudio::DoUninstallApp - redirect to original installer");

	std::wstring exePath = baseProgramDirectory(isPerUser()) + _T(STUDIOBOOTSTAPPERNAMEBETA);

	// reset shortcuts to player's studio launcher
    createRobloxShortcut(logger,isPerUser(),_T(STUDIOQTLINKNAME_CUR),exePath.c_str(),_T("-ide"),false,false);
	createRobloxShortcut(logger,isPerUser(),_T(STUDIOQTLINKNAME_CUR),exePath.c_str(),_T("-ide"),true,false);

    // reset key so that shared launcher will run player's studio launcher instead
    std::wstring progPath = baseProgramDirectory(isPerUser()) + _T(STUDIOBOOTSTAPPERNAMEBETA);
    CRegKey installKey = CreateKey(hk, getQTStudioRegistryPath().c_str(), progPath.c_str());

    // delete the current version info
    installKey.DeleteValue(_T("version"));
}

bool BootstrapperQTStudio::ProcessArg(wchar_t** args, int &pos, int count)
{
    return SharedLauncher::parseEditCommandArg(args,pos,count,editArgs);
}

void BootstrapperQTStudio::deleteLegacyShortcuts()
{
	Bootstrapper::deleteLegacyShortcuts();
	deleteDesktopShortcut(logger, _T(STUDIOQTLINKNAME2013));
	deleteProgramsShortcut(logger, isPerUser(), _T(STUDIOQTLINKNAME2013));
}

bool BootstrapperQTStudio::hasLegacyStudioDesktopShortcut()
{
	if (Bootstrapper::hasLegacyStudioDesktopShortcut())
		return true;
	return hasDesktopShortcut(logger, _T(STUDIOQTLINKNAME2013));
}

static std::wstring getValue(const std::map<std::wstring, std::wstring>& argMap, const std::wstring& key)
{
	std::map<std::wstring, std::wstring>::const_iterator iter = argMap.find(key);
	if (iter != argMap.end())
		return iter->second;

	return _T("");
}

bool BootstrapperQTStudio::ProcessProtocolHandlerArgs(const std::map<std::wstring, std::wstring>& argMap)
{
	std::wstring appArg = getValue(argMap, _T("launchmode"));
	if (appArg == _T("build"))
		editArgs.launchMode = SharedLauncher::BuildArgument;
	else if (appArg == _T("ide"))
		editArgs.launchMode = SharedLauncher::IDEArgument;

	std::wstring scriptUrl = getValue(argMap, _T("script"));
	if (scriptUrl.length() > 0)
	{
		TCHAR decodedUrl [1024]; 
		memset (decodedUrl, 0, sizeof (decodedUrl)); 
		DWORD dwUrlLength; 
		ATL::AtlUnescapeUrl(scriptUrl.c_str(), decodedUrl, &dwUrlLength, 1024);

		editArgs.script = convert_w2s(decodedUrl);
		LOG_ENTRY1("BootstrapperQTStudio::ProcessProtocolHandlerArgs: scriptUrl, value = %s", editArgs.script.c_str());
	}
	
	editArgs.authUrl = "https://" + BaseHost() + "/Login/Negotiate.ashx";
	LOG_ENTRY1("BootstrapperQTStudio::ProcessProtocolHandlerArgs: AuthUrl, value = %s", editArgs.authUrl.c_str());

	std::wstring authTicket = getValue(argMap, _T("gameinfo"));
	if (authTicket.length() > 0)
	{
		editArgs.authTicket = convert_w2s(authTicket);
		LOG_ENTRY1("BootstrapperQTStudio::ProcessProtocolHandlerArgs: gameInfo, value = %s", editArgs.authTicket.c_str());
	}

	std::wstring avatar = getValue(argMap, _T("avatar"));
	if (avatar == _T("true") || avatar == _T(""))
		editArgs.avatarMode = SharedLauncher::AvatarModeArgument;

	if (BrowserTrackerId().length() > 0)
	{
		editArgs.browserTrackerId = BrowserTrackerId();
		LOG_ENTRY1("BootstrapperQTStudio::ProcessProtocolHandlerArgs = browserTrackerId, value = %s", editArgs.browserTrackerId.c_str());
	}

	GUID g1 = {0};
	HRESULT hr = CoCreateGuid(&g1);
	TCHAR guidString[MAX_PATH];
	swprintf(guidString, MAX_PATH, _T("RBX-%d-%d-%d-%d%d%d%d%d%d%d%d"), g1.Data1, g1.Data2, g1.Data3, g1.Data4[0], g1.Data4[1], g1.Data4[2], g1.Data4[3], g1.Data4[4], g1.Data4[5], g1.Data4[6], g1.Data4[7]);
	editArgs.readyEvent = convert_w2s(guidString);
	syncGameStartEvent.Create(NULL, FALSE, FALSE, convert_s2w(editArgs.readyEvent).c_str());

	return true;
}
