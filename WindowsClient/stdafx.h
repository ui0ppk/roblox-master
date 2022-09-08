// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_NO_WARNINGS 1 // Microsoft's standard function deprecation crap: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead.

// Windows and library header files
#include <windows.h>
#include <atlsync.h>
#include <atlwin.h>
#include <atlbase.h>
#include <Sensapi.h>
#include <Shellapi.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <comutil.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ Header files
#include <fstream>

// Boost header files
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

// Web browser control (needed for ActiveX hosting of IWebBrowser2)
#import "shdocvw.dll" include("OLECMDID", "OLECMDF", "OLECMDEXECOPT", "tagREADYSTATE")
