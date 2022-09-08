///////////////////////////////
/* VistaTools.cxx - version 1.0

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 2006.  WinAbility Software Corporation. All rights reserved.

Author: Andrei Belogortseff [ http://www.tweak-uac.com ]

TERMS OF USE: You are free to use this file in any way you like, 
for both the commercial and non-commercial purposes, royalty-free,
AS LONG AS you agree with the warranty disclaimer above, 
EXCEPT that you may not remove or modify this or any of the 
preceeding paragraphs. If you make any changes, please document 
them in the MODIFICATIONS section below. If the changes are of general 
interest, please let us know and we will consider incorporating them in 
this file, as well.

If you use this file in your own project, an acknowledgement will be appreciated, 
although it's not required.

SUMMARY:

This file contains several Vista-specific functions helpful when dealing with the 
"elevation" features of Windows Vista. See the descriptions of the functions below
for information on what each function does and how to use it.

This file contains the Win32 stuff only, it can be used with or without other frameworks, 
such as MFC, ATL, etc.

HOW TO USE THIS FILE:

Make sure you have the latest Windows SDK (see msdn.microsoft.com for more information)
or this file may not compile!

(The above should be done once and only once per project).

The file VistaTools.cxx can be included in the VisualStudio projects, but it should be 
excluded from the build process (because its contents is compiled when it is included 
in another .cpp file with IMPLEMENT_VISTA_TOOLS defined, as shown above.)

MODIFICATIONS:
	v.1.0 (2006-Dec-16) created by Andrei Belogortseff.
	v.2.0 (2008-Aug-04) Erik Cassel: Removed most of the APIs Roblox doesn't need. Query functions remain

*/


#include "stdafx.h"




#include <assert.h>
#include <string>
#include <comdef.h>
#include <taskschd.h>


bool IsVistaPlus()
{
	OSVERSIONINFO osver = {0};

	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	
	if (	::GetVersionEx( &osver ) && 
			osver.dwPlatformId == VER_PLATFORM_WIN32_NT && 
			(osver.dwMajorVersion >= 6 ) )
		return true;

	return false;
}

bool Is64BitWindows() 
{ 
#if defined(_WIN64) 
    return true;  // 64-bit programs run only on Win64 
#elif defined(_WIN32) 
    // 32-bit programs run on both 32-bit and 64-bit Windows 
    // so must sniff 
    BOOL f64 = FALSE; 
    return IsWow64Process(GetCurrentProcess(), &f64) && f64; 
#else 
    return false; // Win64 does not support Win16 
#endif 
}

void GetElevationType( __out TOKEN_ELEVATION_TYPE * ptet )
{
	assert( IsVistaPlus() );
	assert( ptet );

	HRESULT hResult = E_FAIL; // assume an error occured
	CHandle hToken;

	if ( !::OpenProcessToken( 
				::GetCurrentProcess(), 
				TOKEN_QUERY, 
				&hToken.m_h ) )
	{
		throw std::runtime_error("GetElevationType OpenProcessToken failed");
	}

	DWORD dwReturnLength = 0;

	if ( !::GetTokenInformation(
				hToken.m_h,
				TokenElevationType,
				ptet,
				sizeof( *ptet ),
				&dwReturnLength ) )
	{
		throw std::runtime_error("GetElevationType GetTokenInformation failed");
	}
	else
	{
		assert( dwReturnLength == sizeof( *ptet ) );
	}

}

bool IsElevated()
{
	assert( IsVistaPlus() );

	bool result = false;
	CHandle hToken;

	if ( !::OpenProcessToken( 
				::GetCurrentProcess(), 
				TOKEN_QUERY, 
				&hToken.m_h ) )
	{
		throw std::runtime_error("IsElevated OpenProcessToken failed");
	}

	TOKEN_ELEVATION te = { 0 };
	DWORD dwReturnLength = 0;

	if ( !::GetTokenInformation(
				hToken.m_h,
				TokenElevation,
				&te,
				sizeof( te ),
				&dwReturnLength ) )
	{
		throw std::runtime_error("IsElevated GetTokenInformation failed");
	}
	else
	{
		assert( dwReturnLength == sizeof( te ) );

		result = (te.TokenIsElevated != 0);
	}

	return result;
}

bool IsUacEnabled()
{
    CRegKey k;
	return FAILED(k.Open(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\EnableLUA")));
}