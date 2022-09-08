#include "stdafx.h"

#ifdef _WIN32
    #include "SharedLauncher.h"
    #include "vistatools.h"
    #include "atlutil.h"
    #include "wininet.h"
    #include "atlsync.h"
    #include "ProcessInformation.h"
#endif

#define ROBLOXREGKEY        "RobloxReg"	        // Can't use "Roblox" because it is used by the old legacy installer
#define STUDIOQTROBLOXREG   "StudioQTRobloxReg" // Technical debt we need to make this two names globally unique and shared

//Pull in the win32 version Library
#pragma comment(lib, "version.lib")

namespace SharedLauncher
{

#ifdef _WIN32

static void CheckResult(HRESULT hr)
{
	if(FAILED(hr))
		AtlThrow(hr);
}

CRegKey GetKey(CString& out_operation, bool isStudioKey, bool is64bits)
{
	CRegKey key;

    // Very important to do KEY_READ - DON'T WRITE TO VIRTUAL REGISTRY!!!!
	REGSAM regSam = KEY_READ;
	regSam |= is64bits ? KEY_WOW64_64KEY  : KEY_WOW64_32KEY;
	
	if ( !IsVistaPlus() || !IsElevated() )
	{
		// First check HKCU then try HKLM
		if ( isStudioKey ) 
		{
            out_operation = _T("Open HKEY_CURRENT_USER\\Software\\") _T(STUDIOQTROBLOXREG) _T(" key");
			if ( SUCCEEDED(key.Open(HKEY_CURRENT_USER, _T("Software\\") _T(STUDIOQTROBLOXREG), regSam)) && key.m_hKey )
			    return key;
		}
		else
		{
            out_operation = _T("Open HKEY_CURRENT_USER\\Software\\") _T(ROBLOXREGKEY) _T(" key");
            if ( SUCCEEDED(key.Open(HKEY_CURRENT_USER, _T("Software\\") _T(ROBLOXREGKEY), regSam)) && key.m_hKey )
			    return key;
		}
	}

	// fallback to HKLM (used by "WinXP SP1" and "UAC off")
	if ( isStudioKey ) 
	{
		out_operation = _T("Open HKEY_LOCAL_MACHINE\\Software\\") _T(STUDIOQTROBLOXREG) _T(" key");
		CheckResult(key.Open(HKEY_LOCAL_MACHINE, _T("Software\\") _T(STUDIOQTROBLOXREG), regSam));
	}
	else
	{
		out_operation = _T("Open HKEY_LOCAL_MACHINE\\Software\\") _T(ROBLOXREGKEY) _T(" key");
		CheckResult(key.Open(HKEY_LOCAL_MACHINE, _T("Software\\") _T(ROBLOXREGKEY), regSam));
	}

    if ( !key.m_hKey )
        AtlThrow(E_FAIL);

	return key;
}

	class CHINTERNET
	{
	HINTERNET handle;
	public:
	CHINTERNET(HINTERNET handle):handle(handle) {}
	CHINTERNET():handle(0) {}
	CHINTERNET& operator = (HINTERNET handle)
	{
		::InternetCloseHandle(handle);
		this->handle = handle;
		return *this;
	}
	operator bool() { return handle!=0; }
	operator HINTERNET() { return handle; }
	~CHINTERNET()
	{
		::InternetCloseHandle(handle);
	}
	};

	static bool IsRunningVistaIE()
	{
	//return if not Windows Vista or later
	OSVERSIONINFO osvi = {0};
	osvi.dwOSVersionInfoSize=sizeof(osvi);
	GetVersionEx (&osvi);
	if(osvi.dwMajorVersion<6)
		return false;
	else
		return true;
	}

	static CString authenticate(BSTR authenticationUrl)
	{
	if (!IsRunningVistaIE())
		return _T("");

	//if (!Http::trustCheck(url.c_str()))
	//	throw std::runtime_error(G3D::format("trust check failed for %s", url.c_str()));
	
	CUrl u;
	u.CrackUrl(CString(authenticationUrl));

	if (u.GetHostNameLength()==0)
		return _T("");

	// Initialize the User Agent
	CHINTERNET session = InternetOpen(_T("RobloxProxy"), PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
	CHINTERNET connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 1); 

	//   1. Open HTTP Request (pass method type [get/post/..] and URL path (except server name))
	CString s = u.GetUrlPath();
	s += u.GetExtraInfo();
	CHINTERNET request = ::HttpOpenRequest(
		connection, _T("GET"), s, NULL, NULL, NULL, 
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_EXISTING_CONNECT |
		INTERNET_FLAG_NEED_FILE, // ensure that it gets cached
		1); 
	if (!request)
		return _T("");

	CString additionalHeaders = _T("RBXAuthenticationNegotiation:");
	additionalHeaders += u.GetHostName();
	additionalHeaders += "\r\n";
	if (!::HttpAddRequestHeaders(request, (LPCTSTR)additionalHeaders, additionalHeaders.GetLength(), HTTP_ADDREQ_FLAG_ADD))
		return _T("");

	if (!HttpSendRequest(request, NULL, 0, 0, 0))
		return _T("");
	
	// Check the return HTTP Status Code
	DWORD statusCode;
	{
		TCHAR szBuffer[80];
		DWORD dwLen = _countof(szBuffer);
		if (!HttpQueryInfo(request, HTTP_QUERY_STATUS_CODE, szBuffer, &dwLen, NULL))
			return _T("");
		statusCode = (DWORD) _ttol(szBuffer);
	}
		
	DWORD numBytes;
	if (!::InternetQueryDataAvailable(request, &numBytes, 0, 0))
		numBytes = 0;
	if (numBytes==0)
		return _T("");

	if (statusCode!=HTTP_STATUS_OK)
		return _T("");

	DWORD bytesRead;
	TCHAR ticket[2048];
	if (!::InternetReadFile(request, (LPVOID) ticket, 2048, &bytesRead))
		return _T("");

	ticket[bytesRead] = 0;
	return ticket;
	}

	static ATL::CPath loadRobloxPath(CString& out_operation, bool isStudio)
	{
	ATL::CPath path;
	CRegKey key = GetKey(out_operation, isStudio);
	out_operation = "Query Roblox default key";
	DWORD length = _MAX_PATH+1;
	CheckResult(key.QueryStringValue(NULL,path.m_strPath.GetBuffer(length),&length));
	path.m_strPath.ReleaseBuffer();
	return path;
	}

	static void launchRoblox(TCHAR cmd[2048])
	{
	CProcessInformation pi;
	STARTUPINFO si = {0};
	si.cb = sizeof(si);
	if (!::CreateProcess(NULL, cmd, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, pi))
		AtlThrowLastWin32();
	}

	template<class CHARTYPE>
	HRESULT StartGame(simple_logger<CHARTYPE> &logger, BSTR authenTicket, BSTR authenticationUrl, BSTR script, const CLSID& clsid, bool silentMode, TCHAR *guidName, bool startInHiddenMode, TCHAR *unhideEventName, LaunchMode launchMode)
	{
		CString operation;

		HRESULT hr = S_OK;
		try
		{
			if ( wcslen(authenTicket) == 0)
			{
				operation = "Authenticate";
				// TODO: Nuke this???
				CComBSTR authenticationTicket = CComBSTR(authenticate(authenticationUrl));
				authenTicket = (BSTR)authenticationTicket;
			}
			bool isStudioLaunch = launchMode == Edit || launchMode == Build;

			ATL::CPath path;

			try
			{
				path = loadRobloxPath(operation, isStudioLaunch);
			}
			catch( CAtlException )
			{
			}
			
			// If studio doesn't exist (they've uninstalled), try launching in play mode
			if ( (path.m_strPath.IsEmpty() || !path.FileExists()) && isStudioLaunch )
			{
				path = loadRobloxPath(operation, false);
				launchMode = Play;
			}

			operation = "Start Roblox.exe";

			TCHAR cmd[2048] = {0};

			// Edit and build mode both launch studio.  Studio handles arguments differently from player
			// Thus you see the formatting in here different than in the else statement
			if (launchMode == Edit || launchMode == Build)
			{
				EditArgs editArgs;
				
				// For now all web launching should launch build mode, not edit.
				// TODO: Have web launch Build vs EDIT ?
				launchMode = Build;

				editArgs.launchMode = launchMode == Build ? BuildArgument : IDEArgument;  
				editArgs.script = convert_w2s(script);
				editArgs.authUrl = convert_w2s(authenticationUrl);
				editArgs.authTicket = convert_w2s(authenTicket);
				editArgs.avatarMode = launchMode == Build ? AvatarModeArgument : "";  // build and play solo have an avatar, edit does not
					
	#ifdef UNICODE
					if ( silentMode )
						editArgs.readyEvent = convert_w2s(guidName);
					if ( startInHiddenMode )
						editArgs.showEventName = convert_w2s(unhideEventName);
	#else
					if ( silentMode )
						editArgs.readyEvent = guidName;
					if ( startInHiddenMode )
						editArgs.showEventName = unhideEventName;
	#endif

				std::wstring editCommandLine = generateEditCommandLine(editArgs); 

	#ifdef UNICODE
					swprintf_s(
						cmd,
						2048,
						_T("\"%s\" %s"),
						(LPCTSTR)path.m_strPath,
						(LPCTSTR)editCommandLine.c_str() );
	#else
					sprintf_s(
						cmd,
						2048,
						_T("\"%S\" %S"),
						(LPCTSTR)path.m_strPath,
						(LPCTSTR)editCommandLine.c_str() );
	#endif
			}
			else
			{
				BSTR launchType;
                launchType = SysAllocString(L"-play");
	#ifdef UNICODE
				swprintf(cmd, 2048, _T("\"%s\" %s %s %s %s %s %s"), (LPCTSTR)path.m_strPath, launchType,  script, authenticationUrl, authenTicket, silentMode ? guidName : _T("nonSilent"), startInHiddenMode ? unhideEventName : _T("nonHidden"));
	#else 
				sprintf_s(cmd, 2048, _T("\"%s\" %S %S %S %S %s %s"), (LPCTSTR)path.m_strPath, launchType, script, authenticationUrl, authenTicket, silentMode ? guidName : _T("nonSilent"), startInHiddenMode ? unhideEventName : _T("nonHidden"));
	#endif
			}

			logger.write_logentry("Final cmd = %s, unideEventName = %s, hidden = %d", cmd, unhideEventName, startInHiddenMode);
			
			operation = cmd;
			launchRoblox(cmd);
		}
		catch( CAtlException e )
		{
			logger.write_logentry("SharedLauncher::StartGame try failed, in catch");
			hr = e.m_hr;
		}
		if (FAILED(hr))
		{
			CString message;
			message.Format(_T("%s (hr=0x%8.8x). operation: %s"), (LPCTSTR)::AtlGetErrorDescription(hr), hr, (LPCTSTR)operation);
			logger.write_logentry("SharedLauncher::StartGame try failed, error: %S", message.GetString());
			return ::AtlReportError(clsid, message, GUID_NULL, hr);
		}
		return S_OK;
	}

	HRESULT StartGame(simple_logger<char> &logger, BSTR authenTicket, BSTR authenticationUrl, BSTR script, const CLSID& clsid, bool silentMode, TCHAR *guidName, bool startInHiddenMode, TCHAR *unhideEventName, LaunchMode launchMode)
	{
	return StartGame<char>(logger, authenTicket, authenticationUrl, script, clsid, silentMode, guidName, startInHiddenMode, unhideEventName, launchMode);
	}

	HRESULT StartGame(simple_logger<wchar_t> &logger, BSTR authenTicket, BSTR authenticationUrl, BSTR script, const CLSID& clsid, bool silentMode, TCHAR *guidName, bool startInHiddenMode, TCHAR *unhideEventName, LaunchMode launchMode)
	{
	return StartGame<wchar_t>(logger, authenTicket, authenticationUrl, script, clsid, silentMode, guidName, startInHiddenMode, unhideEventName, launchMode);
	}

	HRESULT PreStartGame(const CLSID& clsid)
	{
	CString operation;
	HRESULT hr = S_OK;
	try
	{
		ATL::CPath path = loadRobloxPath(operation, false);
		operation = "Start Roblox.exe";

		TCHAR cmd[2048];
#ifdef UNICODE
		swprintf(cmd, 2048, _T("\"%s\" -prePlay"), (LPCTSTR)path.m_strPath);
#else
		sprintf_s(cmd, 2048, "\"%s\" -install", (LPCTSTR)path.m_strPath);
#endif
		operation = cmd;
		launchRoblox(cmd);
	}
	catch( CAtlException e )
	{
		hr = e.m_hr;
	}
	if (FAILED(hr))
	{
		CString message;
		message.Format(_T("%s (hr=0x%8.8x). operation: %s"), (LPCTSTR)::AtlGetErrorDescription(hr), hr, (LPCTSTR)operation);
		return ::AtlReportError(clsid, message, GUID_NULL, hr);
	}
	return S_OK;
	}

	HRESULT get_InstallHost(BSTR* pVal, const CLSID& clsid)
	{
	CString operation;

	HRESULT hr = S_OK;
	try
	{
		CRegKey key = GetKey(operation, false);
		operation = _T("Query Software\\") _T(ROBLOXREGKEY) _T(" key");
		DWORD length = 256;
		CString host;
		CheckResult(key.QueryStringValue(_T("install host"), host.GetBuffer(length), &length));
		host.ReleaseBuffer();
		CComBSTR result(host);
		*pVal = result.Detach();
	}
	catch( CAtlException e )
	{
		hr = e.m_hr;
	}
	if (FAILED(hr))
	{
		CString message;
		message.Format(_T("%s (hr=0x%8.8x). operation: %s"), (LPCTSTR)::AtlGetErrorDescription(hr), hr, (LPCTSTR)operation);
		return ::AtlReportError(clsid, message, GUID_NULL, hr);
	}
	return S_OK;	
	}

	HRESULT get_Version(BSTR* pVal, const CLSID& clsid)
	{

		CString operation;

		HRESULT hr = S_OK;
		try
		{
			CRegKey key = GetKey(operation, false);
			operation = _T("Query Software\\") _T(ROBLOXREGKEY) _T(" key");
			DWORD length = 256;
			CString host;
			CheckResult(key.QueryStringValue(_T("Plug-in version"), host.GetBuffer(length), &length));
			host.ReleaseBuffer();
			if (host.GetLength() == 0)
				host = "-1";
			CComBSTR result(host);
			*pVal = result.Detach();
		}
		catch( CAtlException e )
		{
			hr = e.m_hr;
		}
		if (FAILED(hr))
		{
			// return -1 on errors
			CString sVersion = CString(_T("-1"));
			CComBSTR result(sVersion);
			*pVal = result.Detach();
		}
	
		return S_OK;
	}

	HRESULT Update(const CLSID& clsid)
	{
	CString operation;

	HRESULT hr = S_OK;
	try
	{
		operation = "Update Roblox.exe";

		ATL::CPath path;
		{
			CRegKey key = GetKey(operation, false);
			operation = "Query Roblox default key";
			DWORD length = _MAX_PATH+1;
			CheckResult(key.QueryStringValue(NULL, path.m_strPath.GetBuffer(length), &length));
			path.m_strPath.ReleaseBuffer();
		}

		TCHAR cmd[2048];
#ifdef UNICODE
		swprintf(cmd, 2048, _T("\"%s\" -install"), (LPCTSTR)path.m_strPath);
#else
		sprintf_s(cmd, 2048, _T("\"%s\" -install"), (LPCTSTR)path.m_strPath);
#endif
		operation = cmd;

		CProcessInformation pi;
		STARTUPINFO si = {0};
		si.cb = sizeof(si);
		if (!::CreateProcess(NULL, cmd, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, pi))
			AtlThrowLastWin32();
	}
	catch( CAtlException e )
	{
		hr = e.m_hr;
	}
	if (FAILED(hr))
	{
		CString message;
		message.Format(_T("Error code: 0x%x, %s. operation: %s"), hr, (LPCTSTR)::AtlGetErrorDescription(hr), (LPCTSTR)operation);
		return ::AtlReportError(clsid, message, GUID_NULL, hr);
	}
	return S_OK;
	}
	HRESULT get_IsUpToDate(simple_logger<wchar_t> &logger, VARIANT_BOOL* pVal, CProcessInformation& isUpToDateProcessInfo, const CLSID& clsid)
	{
	CString operation;

	HRESULT hr = S_OK;
	(*pVal) = VARIANT_FALSE;
	try
	{
		operation = "IsUpToDate Roblox.exe";

		ATL::CPath path;
		{
			CRegKey key = GetKey(operation, false);
			operation = "Query Roblox default key";
			DWORD length = _MAX_PATH+1;
			CheckResult(key.QueryStringValue(NULL, path.m_strPath.GetBuffer(length), &length));
			path.m_strPath.ReleaseBuffer();
		}

		TCHAR cmd[2048];
#ifdef UNICODE
		swprintf(cmd, 2048, _T("\"%s\" -failIfNotUpToDate"), (LPCTSTR)path.m_strPath);
#else
		sprintf_s(cmd, 2048, _T("\"%s\" -failIfNotUpToDate"), (LPCTSTR)path.m_strPath);
#endif
		operation = cmd;

		if(!isUpToDateProcessInfo.InUse()){
			STARTUPINFO si = {0};
			si.cb = sizeof(si);
			if (!::CreateProcess(NULL, cmd, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, isUpToDateProcessInfo))
				AtlThrowLastWin32();
		}
		else
		{
			logger.write_logentry("SharedLauncher::get_IsUpToDate process in use");
		}
		switch(isUpToDateProcessInfo.WaitForSingleObject(1000)){
			case WAIT_TIMEOUT:
			{
				(*pVal) = VARIANT_FALSE;
				logger.write_logentry("SharedLauncher::get_IsUpToDate time out");
				break;
			}
			case WAIT_ABANDONED:
			{
				(*pVal) = VARIANT_FALSE;
				isUpToDateProcessInfo.CloseProcess();

				logger.write_logentry("SharedLauncher::get_IsUpToDate abandoned");
				break;

			}
			case WAIT_FAILED:
			{
				(*pVal) = VARIANT_FALSE;
				isUpToDateProcessInfo.CloseProcess();

				logger.write_logentry("SharedLauncher::get_IsUpToDate wait failed");
				//CString message;
				//message.Format("Failed error=%d\n",GetLastError());
				//::MessageBox(NULL, message, "Error", MB_OK | MB_ICONEXCLAMATION);

				break;
			}
			case WAIT_OBJECT_0:
			{
				//Non-zero return value means it is *not* up to date
				DWORD exitCode;
				if(isUpToDateProcessInfo.GetExitCode(exitCode)){
					//CString message;
					//message.Format("ExitCode = %d\n",exitCode);
					//::MessageBox(NULL, message, "Error", MB_OK | MB_ICONEXCLAMATION);

					(*pVal) = (exitCode == 0) ? VARIANT_TRUE : VARIANT_FALSE;
					logger.write_logentry("SharedLauncher::get_IsUpToDate process exit code %d", exitCode);
				}
				else
				{
					logger.write_logentry("SharedLauncher::get_IsUpToDate failed to get process exit code");
				}
				
				isUpToDateProcessInfo.CloseProcess();
				break;
			}
		}
	}
	catch( CAtlException e )
	{
		hr = e.m_hr;
	}
	if (FAILED(hr))
	{
		CString message;
		message.Format(_T("Error code: 0x%x, %s. operation: %s"), hr, (LPCTSTR)::AtlGetErrorDescription(hr), (LPCTSTR)operation);
		//::MessageBox(NULL, message, "Error", MB_OK | MB_ICONEXCLAMATION);
		return ::AtlReportError(clsid, message, GUID_NULL, hr);
	}

	return S_OK;
	}

#endif

	std::wstring generateEditCommandLine(const EditArgs& editArgs)
	{
    std::string args = editArgs.launchMode;
	
	if ( !editArgs.authUrl.empty() )
        args += std::string(" ") + SharedLauncher::AuthUrlArgument + " " + editArgs.authUrl;
    if ( !editArgs.authTicket.empty() )
        args += std::string(" ") + SharedLauncher::AuthTicketArgument + " " + editArgs.authTicket;
    if ( !editArgs.script.empty() )
        args += std::string(" ") + SharedLauncher::ScriptArgument + " " + editArgs.script;
    if ( !editArgs.readyEvent.empty() )
		args += std::string(" ") + SharedLauncher::ReadyEventArgument + " " + editArgs.readyEvent;
    if ( !editArgs.showEventName.empty() )
        args += std::string(" ") + SharedLauncher::ShowEventArgument + " " + editArgs.showEventName;
	if ( !editArgs.avatarMode.empty() )
		args += std::string(" ") + SharedLauncher::AvatarModeArgument;
	if ( !editArgs.browserTrackerId.empty() )
		args += std::string(" ") + SharedLauncher::BrowserTrackerId + " " + editArgs.browserTrackerId;

    return convert_s2w(args);
	}

	bool parseEditCommandArg(wchar_t** args,int& index,int count,EditArgs& editArgs)
	{
    std::string arg = convert_w2s(args[index]);
    std::string nextArg;
    if ( (index + 1) < count )
        nextArg = convert_w2s(args[index + 1]);

    if ( arg == BuildArgument )
	    editArgs.launchMode = BuildArgument;
    else if ( arg == IDEArgument )
        editArgs.launchMode = IDEArgument;
	else if ( arg == AvatarModeArgument )
		editArgs.avatarMode = AvatarModeArgument;
    else if ( arg == FileLocationArgument )
	{
        editArgs.fileName = nextArg;
		index++;
	}
    else if ( arg == ScriptArgument )
	{
        editArgs.script = nextArg;
		index++;
	}
    else if ( arg == AuthUrlArgument )
	{
        editArgs.authUrl = nextArg;
		index++;
	}
    else if ( arg == AuthTicketArgument )
	{
        editArgs.authTicket = nextArg;
		index++;
	}
    else if ( arg == ReadyEventArgument )
	{
        editArgs.readyEvent = nextArg;
		index++;
	}
    else if ( arg == ShowEventArgument )
	{
        editArgs.showEventName = nextArg;
		index++;
	}
    else
        return false;


    return true;
	}

}

