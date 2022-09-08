#pragma once
#include "shlobj.h"

bool IsVistaPlus();

bool Is64BitWindows();

/*
Use IsVistaPlus() to determine whether the current process is running under Windows Vista or 
(or a later version of Windows, whatever it will be)

Return Values:
	If the function succeeds, and the current version of Windows is Vista or later, 
		the return value is TRUE. 
	If the function fails, or if the current version of Windows is older than Vista 
		(that is, if it is Windows XP, Windows 2000, Windows Server 2003, Windows 98, etc.)
		the return value is FALSE.
*/

void GetElevationType( __out TOKEN_ELEVATION_TYPE * ptet );

/*
Use GetElevationType() to determine the elevation type of the current process.

Parameters:

ptet
	[out] Pointer to a variable that receives the elevation type of the current process.

	The possible values are:

	TokenElevationTypeDefault - User is not using a "split" token. 
		This value indicates that either UAC is disabled, or the process is started
		by a standard user (not a member of the Administrators group).

	The following two values can be returned only if both the UAC is enabled and
	the user is a member of the Administrator's group (that is, the user has a "split" token):

	TokenElevationTypeFull - the process is running elevated. 

	TokenElevationTypeLimited - the process is not running elevated.

Return Values:
	If the function succeeds, the return value is S_OK. 
	If the function fails, the return value is E_FAIL. To get extended error information, 
	call GetLastError().
*/

bool IsElevated();

/*
Use IsElevated() to determine whether the current process is elevated or not.

Parameters:

pbElevated
	[out] [optional] Pointer to a BOOL variable that, if non-NULL, receives the result.

	The possible values are:

	TRUE - the current process is elevated.
		This value indicates that either UAC is enabled, and the process was elevated by 
		the administrator, or that UAC is disabled and the process was started by a user 
		who is a member of the Administrators group.

	FALSE - the current process is not elevated (limited).
		This value indicates that either UAC is enabled, and the process was started normally, 
		without the elevation, or that UAC is disabled and the process was started by a standard user. 

Return Values
	If the function succeeds, and the current process is elevated, the return value is S_OK. 
	If the function succeeds, and the current process is not elevated, the return value is S_FALSE. 
	If the function fails, the return value is E_FAIL. To get extended error information, 
	call GetLastError().
*/


bool IsUacEnabled();

/*
Use IsUacEnabled() to determine whether UAC is enabled or not.
*/

#ifndef FOLDERID_LocalAppDataLow
DEFINE_KNOWN_FOLDER(FOLDERID_LocalAppDataLow,     0xA520A1A4, 0x1780, 0x4FF6, 0xBD, 0x18, 0x16, 0x73, 0x43, 0xC5, 0xAF, 0x16);
DEFINE_KNOWN_FOLDER(FOLDERID_LocalAppData,        0xF1B32785, 0x6FBA, 0x4FCF, 0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x91);
DEFINE_KNOWN_FOLDER(FOLDERID_Programs,            0xA77F5D77, 0x2E2B, 0x44C3, 0xA6, 0xA2, 0xAB, 0xA6, 0x01, 0x05, 0x4A, 0x51);
DEFINE_KNOWN_FOLDER(FOLDERID_ProgramData,         0x62AB5D82, 0xFDC1, 0x4DC3, 0xA9, 0xDD, 0x07, 0x0D, 0x1D, 0x49, 0x5D, 0x97);
#endif

#ifndef KF_FLAG_CREATE
#define KF_FLAG_CREATE              0x00008000
#endif

class VistaAPIs
{
	typedef HRESULT (WINAPI* GetKnownFolderPathPtr)(const GUID& rfid,
		DWORD dwFlags,
		HANDLE hToken,
		PWSTR *ppszPath);
	GetKnownFolderPathPtr getKnownFolderPath;
	HMODULE gShell32DLLInst;

public:
	VistaAPIs()
		:getKnownFolderPath(NULL)
	{
		gShell32DLLInst = LoadLibrary(TEXT("Shell32.dll"));
		if(gShell32DLLInst)
		{
			getKnownFolderPath = (GetKnownFolderPathPtr) GetProcAddress(gShell32DLLInst, "SHGetKnownFolderPath");
		}
	}

	~VistaAPIs()
	{
		if (gShell32DLLInst)
			::FreeLibrary(gShell32DLLInst);
	}

	HRESULT SHGetKnownFolderPath(const GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath)
	{
		if (getKnownFolderPath==NULL)
			return E_NOTIMPL; 
		return getKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
	}

	bool isVistaOrBetter()
	{
		//return if not Windows Vista or later
		OSVERSIONINFO osvi = {0};
		osvi.dwOSVersionInfoSize=sizeof(osvi);
		GetVersionEx (&osvi);
		if(osvi.dwMajorVersion<6)
			return false;
		return true;
	}
};
