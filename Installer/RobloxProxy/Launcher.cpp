// Launcher.cpp : Implementation of CLauncher

#include "stdafx.h"
#include "Launcher.h"
#include "vistatools.h"

#include "atlsync.h"
#include "wininet.h"
#include "ProcessInformation.h"
#include "CookiesEngine.h"
#include <vector>
#pragma comment (lib , "Wininet.lib")

#define ROBLOXREGKEY "RobloxReg"	// Can't use "Roblox" because it is used by the old legacy installer

const int MaxRetrys = 10;

namespace
{
	bool IsFileExists(const wchar_t* name)
	{
#ifdef UNICODE
		std::wstring fileName = name;
#else
		std::string fileName = convert_w2s(std::wstring(name));
#endif
		HANDLE file = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		bool exists = false;
		if (file != INVALID_HANDLE_VALUE) 
		{
			exists = true;
			CloseHandle(file);
		}

		return exists;
	}
}

// CLauncher

void CLauncher::logsCleanUpHelper()
{
	logger.write_logentry("CLauncher::logsCleanUpHelper - entering");
	std::vector<std::wstring> folders;
	folders.push_back(simple_logger<wchar_t>::get_tmp_path());

	VistaAPIs apis;
	if (apis.isVistaOrBetter())
	{
		LPWSTR lppwstrPath = NULL;
		HRESULT hr = apis.SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, KF_FLAG_CREATE, NULL, &lppwstrPath);
		if (SUCCEEDED(hr))
		{
			std::wstring lowPath = lppwstrPath;
			lowPath += L"\\RbxLogs\\";
			folders.push_back(lowPath);
			CoTaskMemFree((LPVOID)lppwstrPath);
		}
	}
	else
	{
		TCHAR szPath[_MAX_DIR]; 
		if (GetTempPath(_MAX_DIR, szPath))
		{
			folders.push_back(std::wstring(szPath));
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
	for (size_t i = 0;i < folders.size();i ++) 
	{
		if (stopLogsCleanup)
		{
			break;
		}

		WIN32_FIND_DATA findFileData;  
		HANDLE handle = INVALID_HANDLE_VALUE;
		handle = FindFirstFile((folders[i] + _T("RBX*.log")).c_str(), &findFileData);
		while (FindNextFile(handle, &findFileData))
		{
			if (stopLogsCleanup)
			{
				break;
			}

			if(findFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
			{
				ULARGE_INTEGER fileTime = {0};
				fileTime.LowPart = findFileData.ftLastWriteTime.dwLowDateTime;
				fileTime.HighPart = findFileData.ftLastWriteTime.dwHighDateTime;

				if (curTime.QuadPart - fileTime.QuadPart > MaxLogAge) 
				{
					std::wstring file = folders[i] + findFileData.cFileName;
					logger.write_logentry("CLauncher::logsCleanUpHelper - deleting log file=%S", file.c_str());
					DeleteFile(file.c_str());
				}
			}
			Sleep(100);
		} 
	}

	clenupFinishedEvent.Set();
	logger.write_logentry("CLauncher::logsCleanUpHelper - leaving");
}

unsigned int CLauncher::clenupThreadFunc(void *pThis)
{
	((CLauncher*)pThis)->logsCleanUpHelper();
	return 0;
}

void CheckResult(HRESULT hr)
{
	if(FAILED(hr))
		AtlThrow(hr);
}


// CLauncher
const CLauncher::SiteList CLauncher::rgslTrustedSites[] =
{
	{ SiteList::Allow, L"http", L"roblox.com"},
	{ SiteList::Allow, L"http", L"robloxlabs.com" },
	{ SiteList::Allow, L"https", L"roblox.com"},
	{ SiteList::Allow, L"https", L"robloxlabs.com" }
	//{ SiteList::Allow, L"http", SITELOCK_INTRANET_ZONE }
};

STDMETHODIMP CLauncher::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ILauncher
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CLauncher::StartGame(BSTR authenticationUrl, BSTR script)
{
	syncGameStartEvent.Reset();

	SharedLauncher::LaunchMode passedLaunchMode = launchMode;

	// Reset launch mode in case plugin becomes reentrant
	launchMode = SharedLauncher::Play;
	return SharedLauncher::StartGame(logger, authenticationTicket.m_str,authenticationUrl,script,GetObjectCLSID(), silenMode, guidString, startInHiddenMode, unhideEventName, passedLaunchMode);
}

STDMETHODIMP CLauncher::HelloWorld(void)
{
	::MessageBox(NULL, _T("Hello World"), _T("Roblox Proxy"), MB_OK);

	return S_OK;
}

STDMETHODIMP CLauncher::Hello(BSTR* result)
{
	CComBSTR world("World");

	*result = world.Detach();

	return S_OK;
}

CRegKey CLauncher::GetKey(CString& operation)
{
	return SharedLauncher::GetKey(operation, false);
}

STDMETHODIMP CLauncher::get_InstallHost(BSTR* pVal)
{
	return SharedLauncher::get_InstallHost(pVal,GetObjectCLSID());
}

STDMETHODIMP CLauncher::Update(void)
{
	return SharedLauncher::Update(GetObjectCLSID());
}

STDMETHODIMP CLauncher::get_IsUpToDate(VARIANT_BOOL* pVal)
{
	logger.write_logentry("CLauncher::get_IsUpToDate, create launcher started event");
	if (!launcherStartedEvent.Create(NULL, FALSE, TRUE, LAUNCHER_STARTED_EVENT_NAME))
	{
		DWORD error = GetLastError();
		if (error != ERROR_ALREADY_EXISTS)
			logger.write_logentry("CLauncher::ResetState event creation failed error=0x%X", error);
	}

	logger.write_logentry("CLauncher::get_IsUpToDate, process in use: %d", m_isUpToDateProcessInfo.InUse());
	return SharedLauncher::get_IsUpToDate(logger, pVal,m_isUpToDateProcessInfo,GetObjectCLSID());
}
STDMETHODIMP CLauncher::put_AuthenticationTicket(BSTR newVal)
{
	HRESULT hr = authenticationTicket.AssignBSTR(newVal);
	if (FAILED(hr))
	{
		CString message;
		message.Format(_T("Set AuthenticationTicket: %s"), hr, (LPCTSTR)::AtlGetErrorDescription(hr));
		return ::AtlReportError(GetObjectCLSID(), message, GUID_NULL, hr);
	}
	return S_OK;
}

STDMETHODIMP CLauncher::PreStartGame(void)
{
	return SharedLauncher::PreStartGame(GetObjectCLSID());
}

STDMETHODIMP CLauncher::get_IsGameStarted(VARIANT_BOOL* pVal)
{
	logger.write_logentry("CLauncher::get_IsGameStarted");
	DWORD dwRes = WaitForSingleObject(syncGameStartEvent, 100);
	if (WAIT_TIMEOUT == dwRes)
	{
		logger.write_logentry("CLauncher::get_IsGameStarted - game is not started");
		(*pVal) = VARIANT_FALSE;
	}
	else
	{
		logger.write_logentry("CLauncher::get_IsGameStarted - game started");
		(*pVal) = VARIANT_TRUE;
	}
	return S_OK;
}

STDMETHODIMP CLauncher::SetSilentModeEnabled(VARIANT_BOOL silentModeEnbled)
{
	silenMode = (silentModeEnbled == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP CLauncher::SetStartInHiddenMode(VARIANT_BOOL hiddenModeEnabled)
{
	startInHiddenMode = (hiddenModeEnabled == VARIANT_TRUE);
	logger.write_logentry("CLauncher::SetStartInHiddenMode startInHiddenMode = %d", startInHiddenMode);
	return S_OK;
}

STDMETHODIMP CLauncher::UnhideApp()
{
	logger.write_logentry("CLauncher::UnhideApp unhideEventName = %S", unhideEventName);
	if (!unhideEvent.Set())
	{
		logger.write_logentry("CLauncher::UnhideApp Set Event failed");
	}
	return S_OK;
}

STDMETHODIMP CLauncher::BringAppToFront()
{
	logger.write_logentry("CLauncher::BringAppToFront");
	const TCHAR *mainWndStorageName = _T("RBXMAINWND-4DAAC10B-9C9A-4471-9218-07310329FD0D");

	HANDLE hMapFile = NULL;
	LPCTSTR buf = NULL;
	HWND mainWnd = NULL;

	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mainWndStorageName);
	if (hMapFile != NULL) 
	{
		logger.write_logentry("CLauncher::BringAppToFront OpenFileMapping SUCCEEDED");
		buf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0,sizeof(HWND));
		if (buf != NULL)
		{
			CopyMemory(&mainWnd, buf, sizeof(HWND));
			logger.write_logentry("CLauncher::BringAppToFront MapViewOfFile SUCCEEDED, Wnd=0x%d", mainWnd);

			::SetForegroundWindow(mainWnd);
			::SetWindowPos(mainWnd, HWND_TOPMOST, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			::SetWindowPos(mainWnd, HWND_NOTOPMOST, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		}
	}

	if (buf != NULL)
	{
		UnmapViewOfFile(buf);
	}

	if (hMapFile != NULL)
	{
		CloseHandle(hMapFile);
	}

	return S_OK;
}

STDMETHODIMP CLauncher::ResetLaunchState()
{
	return ResetState();
}

STDMETHODIMP CLauncher::SetEditMode()
{
	logger.write_logentry("CLauncher::SetEditMode");
	launchMode = SharedLauncher::Edit;
	return S_OK;
}

STDMETHODIMP CLauncher::Get_Version(BSTR* pVal)
{
	return SharedLauncher::get_Version(pVal,GetObjectCLSID());
}

STDMETHODIMP CLauncher::GetKeyValue(BSTR key, BSTR *pVal)
{
	HRESULT hr = E_FAIL;
	int rCounter = MaxRetrys;
	CW2A converted(key);
	std::string keyString(converted);
	*pVal = NULL;

	std::wstring path = CookiesEngine::getCookiesFilePath();
	if (IsFileExists(path.c_str()))
	{
		CookiesEngine engine(path);
		while (rCounter)
		{
			bool valid = false;
			int result = 0;
			std::string value = engine.GetValue(keyString, &result, &valid);
			if (result == 0)
			{
				hr = S_OK;
				CComBSTR retVal(value.c_str());
				if (valid)
				{
					*pVal = retVal.Detach();
				}
				break;
			}

			Sleep(50);
			rCounter --;
		}
	}

	return hr;
}

STDMETHODIMP CLauncher::SetKeyValue(BSTR key, BSTR val)
{
	HRESULT hr = E_FAIL;
	int rCounter = MaxRetrys;
	CW2A cKey(key);
	CW2A cVal(val);
	std::string keyString(cKey);
	std::string valString(cVal);

	std::wstring path = CookiesEngine::getCookiesFilePath();
	logger.write_logentry("CLauncher::SetKeyValue key = %s, val = %s path = %s", keyString.c_str(), valString.c_str(), path.c_str());
	if (IsFileExists(path.c_str()))
	{
		CookiesEngine engine(path);
		while (rCounter)
		{
			int result = engine.SetValue(keyString, valString);
			if (result == 0)
			{
				logger.write_logentry("CLauncher::SetKeyValue counter = %d, result = %d", rCounter, result);
				hr = S_OK;
				break;
			}

			Sleep(50);
			rCounter --;
		}
	}

	return hr;
}

STDMETHODIMP CLauncher::DeleteKeyValue(BSTR key)
{
	HRESULT hr = E_FAIL;
	int rCounter = MaxRetrys;
	CW2A cKey(key);
	std::string keyString(cKey);

	std::wstring path = CookiesEngine::getCookiesFilePath();
	logger.write_logentry("CLauncher::DeleteKeyValue key = %s, path = %s", keyString.c_str(), path.c_str());
	if (IsFileExists(path.c_str()))
	{
		CookiesEngine engine(path);
		while (rCounter)
		{
			int result = engine.DeleteValue(keyString);
			if (result == 0)
			{
				logger.write_logentry("CLauncher::DeleteKeyValue conuter = %d, result = %d", rCounter, result);
				hr = S_OK;
				break;
			}

			Sleep(50);
			rCounter --;
		}
	}
	return hr;
}
