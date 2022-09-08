//
// Empty precompiled header file
//

#ifdef WIN32
#pragma once
#define _CRT_SECURE_NO_WARNINGS 1

#ifndef STRICT
#define STRICT
#endif

#define _ATL_APARTMENT_THREADED 
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x05010100	// NTDDI_WINXPSP1
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0601
#endif

#include <sdkddkver.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <atlbase.h>
#include <atlcom.h>
#include <atlutil.h>
#include <string>

#endif