// BootstrapperClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BootstrapperRccService.h"
#include "FileSystem.h"
#include "format_string.h"


#include "AtlPath.h"
#include "atlutil.h"
#include "Aclapi.h"
#include "Sddl.h"
#include "processinformation.h"

static Bootstrapper* newBootstrapper(HINSTANCE hInstance)
{
	return new BootstrapperRccService(hInstance);
}
int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	return BootstrapperMain(hInstance,nCmdShow,&newBootstrapper);
}

BootstrapperRccService::BootstrapperRccService(HINSTANCE hInstance)
:Bootstrapper(hInstance)
{
	_regSubPath = _T("RobloxReg");
	_regPath = _T("SOFTWARE\\") + _regSubPath;
	_versionGuidName = _T(VERSIONGUIDNAMERCC);

	::CoCreateGuid(&serviceGuid);
}

std::wstring BootstrapperRccService::GetRegistryPath() const
{
	return _regPath;
}

std::wstring BootstrapperRccService::GetRegistrySubPath() const
{
	return _regSubPath;
}

const wchar_t *BootstrapperRccService::GetVersionFileName() const
{
	return _versionFileName.c_str();
}

const wchar_t *BootstrapperRccService::GetGuidName() const
{
	return _versionGuidName.c_str();
}

const wchar_t *BootstrapperRccService::GetStartAppMutexName() const
{
	return _emptyString.c_str();
}

const wchar_t *BootstrapperRccService::GetBootstrapperMutexName() const
{
	return _emptyString.c_str();
}

const wchar_t* BootstrapperRccService::GetFriendlyName() const
{
    return _T("RCC Service");
}

bool BootstrapperRccService::ProcessArg(wchar_t** args, int &pos, int count)
{
	return false;
}

std::wstring BootstrapperRccService::programDirectory() const
{
	std::wstring dir = FileSystem::getSpecialFolder(FileSystem::RobloxProgramFiles, true, format_string("RCC-%08X%04X", serviceGuid.Data1, serviceGuid.Data2).c_str());
	if (dir.empty())
		throw std::runtime_error("Failed to create programDirectory");
	return dir;
}


bool BootstrapperRccService::EarlyInstall(bool isUninstall)
{
	if (isUninstall)
		uninstallService();
	else
		installService();

	return true;
}


void BootstrapperRccService::installService()
{
	deployer->deployVersionedFile(_T("RCC-redist.zip"), NULL, Progress(), true);
	deployer->deployVersionedFile(_T("RCC-Libraries.zip"), NULL, Progress(), true);
	deployer->deployVersionedFile(_T("RCC-shaders.zip"), NULL, Progress(), true);
	deployer->deployVersionedFile(_T("RCC-content.zip"), NULL, Progress(), true);
	deployer->deployVersionedFile(_T("RCC-platformcontent.zip"), NULL, Progress(), true);
	deployer->deployVersionedFile(_T("RCCService.zip"), NULL, Progress(), true);

	writeAppSettings();

	//First stop/uninstall the old RCCService.exe
	std::wstring prog = programDirectory() + _T("RCCService.exe");
	CProcessInformation pi;
	CreateProcess(prog.c_str(), _T("-stop -uninstall"), pi);
	if (WAIT_TIMEOUT==pi.WaitForSingleObject(30*1000))
		throw std::runtime_error("RCCService.exe -stop -uninstall timed out");
	
	// Write the installation dir to the registry
	const TCHAR rbxRegPath[] = _T("Software\\ROBLOX Corporation\\Roblox");
	CRegKey pathKey;
	pathKey.Create(HKEY_LOCAL_MACHINE, rbxRegPath);
	if (pathKey.m_hKey == NULL)
		throw std::runtime_error("Unable to create/open reg key: 'Software\\ROBLOX Corporation\\Roblox'");

	LONG res = pathKey.SetStringValue(_T("RccServicePath"), (LPCTSTR)ProgramDirectory().c_str());
	if (res != ERROR_SUCCESS)
		throw std::runtime_error("Failed writing to registry: 'Software\\ROBLOX Corporation\\Roblox\\RccServicePath'");
}
void BootstrapperRccService::uninstallService()
{
	CProcessInformation pi;
	CreateProcess((programDirectory() + _T("RCCService.exe")).c_str(), _T("-Stop -Uninstall"), pi);

	if (WAIT_TIMEOUT==pi.WaitForSingleObject(10*1000))
		throw std::runtime_error("RCCService.exe -Stop -Uninstall timed out");
}


#define	KEY_LEN	256
#define VERSION	3	
static bool GetDotNetFramework(TCHAR *fwVersion)
{
	HKEY hKey;
	DWORD dwIndex = 0;
	DWORD cbName = KEY_LEN;
	TCHAR sz[KEY_LEN];
	bool result = false;

	try
	{
		if(ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\.NETFramework\\policy"), 0, KEY_READ, &hKey))
		{
			// Iterate through all subkeys
			while ((ERROR_NO_MORE_ITEMS != ::RegEnumKeyEx(hKey, dwIndex, sz, &cbName, NULL, NULL, NULL, NULL)))
			{
				if(!_tcsnicmp(sz, _T("V"), 1))
					_tcsncpy_s(fwVersion, VERSION + 1, sz+1, VERSION + 1);
				dwIndex++;
				cbName = KEY_LEN;
			}
			::RegCloseKey(hKey);
			printf("Fraemwork version found: %s\n", fwVersion);
			result = true;
		}
		else
			result = false;
		return result;
	}
	catch(...)
	{
		return false;
	}
}
static bool CompareDotNetFrameWorkVersions(TCHAR *existing, TCHAR *required)
{
	int existingMajor = _wtoi(existing);
	int existingMinor = _wtoi(existing+2);
	int requiredMajor = _wtoi(required);
	int requiredMinor = _wtoi(required+2);
	bool result = false;

	// very simple comparison algorithm, really
	if(requiredMajor < existingMajor)
		result = true;
	else if(requiredMajor == existingMajor && requiredMinor <= existingMinor)
		result = true;
	else
		result = false;
	return result;
}