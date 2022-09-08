/**
 * stdafx.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

/*****************************************************************************/
// Windows Headers and Defines
/*****************************************************************************/

#undef INTSAFE_E_ARITHMETIC_OVERFLOW

#ifdef _WIN32
    #ifndef STRICT
    #define STRICT
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

//#define WIN32_LEAN_AND_MEAN		            // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define _ATL_APARTMENT_THREADED

// TODO: this disables support for registering COM objects
// exported by this project since the project contains no
// COM objects or typelib. If you wish to export COM objects
// from this project, add a typelib and remove this line
#define _ATL_NO_COM_SUPPORT

#define _WIN32_WINNT    0x0501
#define WINVER          _WIN32_WINNT
#define _WIN32_WINDOWS  _WIN32_WINNT 
#define NTDDI_VERSION   0x05010100	        // NTDDI_WINXPSP1
#define _WIN32_IE       0x0601

#ifdef _WIN32
    #include <sdkddkver.h>
    #include <atlbase.h>
    #include <atlstr.h>
    #include <atlstr.h>
    #include <shobjidl.h>
#endif

#define NO_SDL_MAIN
    #define G3DEXPORT __declspec(dllimport)
#endif

/*****************************************************************************/
// Pre-compiled Headers
/*****************************************************************************/

//#undef USE_PCH_HEADERS

// only include stuff here that never changes
#ifdef USE_PCH_HEADERS

// Standard C/C++ Headers
#include <string>
#include <vector>
#include <map>
#include <list>
#include <cmath>
#include <limits>
#include <exception>

// Windows Headers
#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QtCore>
#include <QtGui>
#include <QtXml>
#include <QtDebug>

// TODO - these 2 give strange compiler bugs in the DS video code
//#include <QtNetwork>
//#include <QtWebKit>

// Boost Headers
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/iostreams/copy.hpp"

// Roblox Headers
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/RenderSettings.h"
#include "Network/api.h"
#include "Network/Players.h"
#include "rbx/Boost.hpp"
#include "rbx/CEvent.h"
#include "rbx/Debug.h"
#include "rbx/ProcessPerfCounter.h"
#include "rbx/signal.h"
#include "rbx/TaskScheduler.h"
#include "rbx/TaskScheduler.Job.h"
#include "rbx/threadsafe.h"
#include "reflection/reflection.h"
#include "reflection/Type.h"
#include "script/Script.h"
#include "util/ContentId.h"
#include "util/FileSystem.h"
#include "util/G3DCore.h"
#include "util/Http.h"
#include "util/IMetric.h"
#include "util/Profiling.h"
#include "util/ScopedAssign.h"
#include "util/standardout.h"
#include "util/Statistics.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/Workspace.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8xml/Serializer.h"
#include "v8xml/XmlElement.h"
#include "v8xml/XmlSerializer.h"

#include "FastLog.h"
#include "NetworkSettings.h"
#include "ReflectionMetaData.h"

#endif
