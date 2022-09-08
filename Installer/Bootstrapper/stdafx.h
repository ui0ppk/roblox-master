// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>
#include <exception>
#include <string>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

class silent_exception : public std::exception
{
protected:
	silent_exception(const char* what):std::exception(what) {}
};

class cancel_exception : public silent_exception
{
public:
	cancel_exception(bool userCancel)
		: silent_exception("Cancelled")
	    , userCancel(userCancel)
	{}
	std::string getErrorType()
	{
		return userCancel ? "User Cancelled" : "Double Start Cancelled";
	}
protected:
	bool userCancel;
};
class non_zero_exit_exception: public silent_exception
{
public:
	non_zero_exit_exception()
		: silent_exception("NonZeroExitException")
	{}

};
class installer_error_exception : public silent_exception
{
public:
	enum InstallerErrorExceptionType
	{
		VistaElevation = 0,
		BootstrapVersion,
		OsPrerequisite,
		DirectxPrerequisite,
		IePrerequisite,
		DiskSpacePrerequisite,
		AdminAccountRequired,
		InstallerTimeout,
		AdminAccountRequiredWithoutUac,
		CpuPrerequisite
	};
	installer_error_exception(InstallerErrorExceptionType type)
		: silent_exception("InstallerError")
	    , type(type)
	{}

	std::string getErrorType()
	{
		switch(type){
		case VistaElevation: return "VistaElevation";
		case BootstrapVersion: return "boostrapVersion";
		case OsPrerequisite: return "OSPreRequisite";
		case DirectxPrerequisite: return "DirectXPreRequisite";
		case IePrerequisite: return "IEPreRequisite";
		case DiskSpacePrerequisite: return "DiskSpacePreRequisite";
		case AdminAccountRequired: return "AdminAccountRequired";
		case InstallerTimeout: return "InstallerTimeout";
		case AdminAccountRequiredWithoutUac: return "AdminAccountRequiredWithoutUac";
		}
		return "Unknown";
	}
protected:
	InstallerErrorExceptionType type;
};

class previous_error : public silent_exception
{
public:
	previous_error():silent_exception("previous error") {}
	previous_error(const std::string& msg):silent_exception(("previous error " + msg).c_str()) {}
};

void throwLastError(BOOL result, const std::string& message);

#include "boost/config/user.hpp"
