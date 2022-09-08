// this file meant to include basic OS .h dependencies.
// obviously, it only supports windows at this time.
#pragma once

#ifdef WIN32
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#ifndef STRICT
#define STRICT 1
#endif

#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN 1
#   endif

#ifndef NOMINMAX
#   define NOMINMAX 1
#endif	
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN
#   undef NOMINMAX

#   undef _G3D_INTERNAL_HIDE_WINSOCK_
#   undef _WINSOCKAPI_

#include <intrin.h>

#endif

#include "RbxBase.h"