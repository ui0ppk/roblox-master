#pragma once

#include "RbxPlatform.h"
#include "RbxAssert.h"
#include "RbxFormat.h"
#include <set>
#include <ostream>
#include <fstream>
#include <assert.h>

#if (defined(_DEBUG) && defined(_WIN32))
#include <crtdbg.h>
#endif

#ifdef __ANDROID__
#include <typeinfo>
#include <cstdlib>
#endif

#ifdef _WIN32
#undef min
#undef max
#endif

#include "rbx/Declarations.h"
#include "FastLog.h"

#ifndef _WIN32
#define __noop
inline void DebugBreak()
{
#if defined(__i386__)
	// gcc on intel
		__asm__ __volatile__ ( "int $3" ); 
#else
	// some other gcc
	::abort();
#endif
}
#endif

LOGGROUP(Asserts)

/* Overview of builds and switches:

RBXASSERT:					Standard assert.  Should be reasonably fast.  Do not do "finds" or complex stuff here.  Simple bools, simple math, a couple levels of pointer indirection, etc.
RBXASSERT_VERY_FAST:		High fr  equency, extremely fast assert.  Not in regular debug build because frequency too high.  Mostly inner engine stuff
RBXASSERT_SLOW:				Put things like "find" here.  Will always run in debug builds
RBXASSERT_IF_VALIDATING:	Very slow stuff.  Only turns on if the "validating debug" switch is turned on in debug or noOpt build
RBXASSERT_FISHING:			Usually doesn't go off, should be safe - turn on for engine testing

							RBXASSERT()			RBXASSERT_VERY_FAST()	RBXASSERT_SLOW()		RBXASSERT_IF_VALIDATING()	RBXASSERT_FISHING()
	DEBUG						X					X						X							X							-
	NoOpt						X					X						-							-							-
	ReleaseAssert				X					-						-							-							-
	Release						-					-						-							-							-

*/


#ifdef _DEBUG
	#define __RBX_VERY_FAST_ASSERT
	#define __RBX_VALIDATE_ASSERT
//	#define __RBX_SLOW_ASSERT	// TODO: Hire a physics guy to enable them
//	#define __RBX_FISHING_ASSERT
	#define __RBX_NOT_RELEASE
#endif

#ifdef _NOOPT
	#define __RBX_CRASH_ON_ASSERT
	#define __RBX_VERY_FAST_ASSERT
	#define __RBX_NOT_RELEASE
#endif

namespace RBX {

	// Used for memory leak detection and other stuff
	class Debugable
    {
	public:
		// this is here as a last chance way to debug an assert build, force assertions on, but not crash
		static volatile bool doCrashEnabled;
        
		static void doCrash();
		static void doCrash(const char*);

		static void* badMemory() {return reinterpret_cast<void*>(0x00000003);}		// set values to this when deleting to check if ever coming back
	};

}

void RBXCRASH();
void RBXCRASH(const char* message);

void ReleaseAssert(int channel, const char* msg);

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// macro to convince a compiler a variable is used while not generating instructions (useful for removing warnings)
#define RBX_UNUSED(x) (void)(sizeof((x), 0))

// This macro will cause a crash. Usually you don't call it directly. Use RBXASSERT instead
#define RBX_CRASH_ASSERT(expr) \
		((void) (!!(expr) || \
		((RBX::_internal::_debugHook != NULL) && (RBX::_internal::_debugHook(#expr, __FILE__, __LINE__))) || \
		(RBX::Debugable::doCrash(#expr), 0)))

// This macro will just log an assert string, if we will run into crash log with the assert information will be sent to us
#define RBX_LOG_ASSERT(expr) \
	((void) (FLog::Asserts && (!!(expr) || \
	((RBX::_internal::_debugHook != NULL) && (RBX::_internal::_debugHook(#expr, __FILE__, __LINE__))) || \
	(ReleaseAssert(FLog::Asserts,#expr " file: " __FILE__ " line: " TOSTRING(__LINE__)), 0))))


// LEGACY_ASSERT should be used when we have some assert bogging us and it seems like this guy is a good candidate for removal
// usage just replace RBXASSERT with LEGACY_ASSERT and it will gone by default, but if you need to see it temporary define FIRE_LEGACY_ASSERT
#undef FIRE_LEGACY_ASSERT

#ifdef FIRE_LEGACY_ASSERT
	#define LEGACY_ASSERT(expr) RBXASSERT(expr)
#else
	#define LEGACY_ASSERT(expr) ((void)0)
#endif

#define RBXASSERTENABLED

// RBXASSERT()
//
#ifdef __RBX_CRASH_ON_ASSERT
	#define RBXASSERT RBX_CRASH_ASSERT
#else
	#if (defined(_DEBUG) && defined(__APPLE__))		// Apple Debug
        #include "TargetConditionals.h"
        #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
            #define RBXASSERT RBX_LOG_ASSERT // iOS has no way to step over asserts (makes debugging hard)
        #else
            #define RBXASSERT(expr) assert(expr)
            #define RBXASSERTENABLED
        #endif
	#elif (defined(_DEBUG) && defined(_WIN32))		// Windows Debug
		#define RBXASSERT(expr) \
			((void) (!!(expr) || \
			((RBX::_internal::_debugHook != NULL) && (RBX::_internal::_debugHook(#expr, __FILE__, __LINE__))) || \
			(_ASSERTE(expr), 0)))
		#define RBXASSERTENABLED
	#else											// All Platform Release
		#define RBXASSERT RBX_LOG_ASSERT
	#endif
#endif



// RBXASSERT_VERY_FAST()
//
#ifdef __RBX_VERY_FAST_ASSERT
	#define RBXASSERT_VERY_FAST(expr) RBXASSERT(expr)
#else
	#define RBXASSERT_VERY_FAST(expr) ((void)0)
#endif


// RBXASSERT_SLOW()
//
#ifdef __RBX_SLOW_ASSERT
	#define RBXASSERT_SLOW(expr) RBXASSERT(expr)
#else
	#define RBXASSERT_SLOW(expr) ((void)0)
#endif


// RBXASSERT_FISHING)
//
#ifdef __RBX_FISHING_ASSERT
	#define RBXASSERT_FISHING(expr) RBXASSERT(expr)
#else
	#define RBXASSERT_FISHING(expr) ((void)0)
#endif


// RBXASSERT_IF_VALIDATING()
//
#ifdef __RBX_VALIDATE_ASSERT
	#define RBXASSERT_IF_VALIDATING(expr)	RBXASSERT( (expr) ) 

#else
	#define RBXASSERT_IF_VALIDATING(expr) ((void)0)
#endif


// RBXASSERT_NOT_RELEASE()		make sure this code is not being compiled in release build
#ifdef __RBX_NOT_RELEASE
	#define RBXASSERT_NOT_RELEASE()			((void)0)
#else
	#define RBXASSERT_NOT_RELEASE()			RBXCRASH()
#endif


// Same as boost::polymorphic_downcast but with an RBXASSERT
template<class T, class U>
inline T rbx_static_cast(U u) {
	RBXASSERT_SLOW(dynamic_cast<T>(u)==u);
	return static_cast<T>(u);
}
