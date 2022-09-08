#include "JNIUtil.h"

#include <string>
#include <jni.h>

#include "LogManager.h"

#include "FastLog.h"

#include "util/Http.h"
#include "util/Statistics.h"
#include "util/standardout.h"
#include "util/Http.h"
#include "util/FileSystem.h"

#include "v8datamodel/GuiBuilder.h"

#include "JNIProfiler/prof.h"

LOGGROUP(Android)

using namespace RBX;
using namespace RBX::JNI;

namespace RBX
{
namespace JNI
{
extern std::string fileSystemCacheDir;      // FileSystem.cpp
extern std::string fileSystemFilesDir;      // FileSystem.cpp
extern std::string platformUserAgent;       // LogManager.cpp
extern std::string robloxVersion;           // LogManager.cpp
extern std::string exceptionReasonFilename; // JNIMain.cpp
} // namespace JNI
} // namespace RBX

extern "C"
{
JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetBaseUrl(JNIEnv* jenv, jclass obj, jstring jsBaseUrl)
{
    std::string baseUrl = jstringToStdString(jenv, jsBaseUrl);
    FASTLOGS(FLog::Android, "Base URL: %s", baseUrl.c_str());
    SetBaseURL(baseUrl);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetCacheDirectory(JNIEnv* jenv, jclass obj, jstring jsCacheDirectory)
{
    fileSystemCacheDir = jstringToStdString(jenv, jsCacheDirectory);
    FASTLOGS(FLog::Android, "Cache Directory: %s", fileSystemCacheDir.c_str());
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetFilesDirectory(JNIEnv* jenv, jclass obj, jstring jsFilesDirectory)
{
	fileSystemFilesDir = jstringToStdString(jenv, jsFilesDirectory);
    FASTLOGS(FLog::Android, "Files Directory: %s", fileSystemFilesDir.c_str());
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetExceptionReasonFilename(JNIEnv* jenv, jclass obj, jstring jsExceptionReasonFilename)
{
    exceptionReasonFilename = jstringToStdString(jenv, jsExceptionReasonFilename);
    FASTLOGS(FLog::Android, "Exception reason filename (for terminate_handler): %s", exceptionReasonFilename.c_str());
}


JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeInitFastLog(JNIEnv* jenv, jclass obj)
{
    if (fileSystemCacheDir.empty())
    {
        throw RBX::runtime_error("Cannot initialize fastlog system.  Cache directory not set.");
    }

    StandardOut::singleton()->printf(MESSAGE_INFO, "Setting up fast log system.");
    LogManager::setupFastLog();
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeInitBreakpad(JNIEnv* jenv, jclass obj, jboolean cleanup)
{
    if (GetBaseURL().empty() || fileSystemCacheDir.empty())
    {
        throw RBX::runtime_error("Cannot initialize breakpad.  Base URL or cache directory not set.");
    }
    LogManager::setupBreakpad(cleanup);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetPlatformUserAgent(JNIEnv* jenv, jclass obj, jstring jsUserAgent)
{
    platformUserAgent = jstringToStdString(jenv, jsUserAgent);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetRobloxVersion(JNIEnv* jenv, jclass obj, jstring jsRobloxVersion)
{
    robloxVersion = jstringToStdString(jenv, jsRobloxVersion);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetHttpProxy(JNIEnv* jenv, jclass obj, jstring jsProxyPort, jlong proxyPort)
{
    std::string proxyHost = jstringToStdString(jenv, jsProxyPort);
    Http::setProxy(proxyHost, proxyPort);
    FASTLOGS(FLog::Android, "Set proxy host to %s", proxyHost.c_str());
    FASTLOG1(FLog::Android, "Set proxy port to %ld", proxyPort);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetCookiesForDomain(JNIEnv* jenv, jclass obj, jstring jsDomain, jstring jsCookies)
{
    std::string domain = jstringToStdString(jenv, jsDomain);
    std::string cookies = jstringToStdString(jenv, jsCookies);
    FASTLOGS(FLog::Android, "Setting ROBLOX cookies: %s", cookies.c_str());
    Http::setCookiesForDomain(domain, cookies);
}

JNIEXPORT void JNICALL Java_com_roblox_client_RobloxSettings_nativeSetDebugDisplay(JNIEnv* jenv, jclass obj, jint debugDisplayMode)
{
    RBX::GuiBuilder::setDebugDisplay(static_cast<RBX::GuiBuilder::Display>(debugDisplayMode));
}

// Sending down 0 writes out the data.  See https://code.google.com/p/android-ndk-profiler/
JNIEXPORT jboolean JNICALL Java_com_roblox_client_RobloxSettings_nativeEnableNDKProfiler(JNIEnv* jenv, jclass obj, jint frequency)
{
// This is GPL code and cannot be shipped.
#if 0
	if( frequency > 0 )
	{
		// Log as error as this stuff is not supposed to ship.

		char buf[512];
		sprintf( buf, "%d", frequency );
		setenv("CPUPROFILE_FREQUENCY", buf, 1);
        StandardOut::singleton()->printf(MESSAGE_ERROR, "PROFILING CPUPROFILE_FREQUENCY: %s", buf);
		sprintf( buf, "%s/gmon.out", fileSystemFilesDir.c_str() );
		setenv("CPUPROFILE", buf, 1);
        StandardOut::singleton()->printf(MESSAGE_ERROR, "PROFILING CPUPROFILE: %s", buf);
		monstartup("roblox.so");
	}
	else
	{
        StandardOut::singleton()->printf(MESSAGE_ERROR, "PROFILING cleanup");
		moncleanup();
	}
	return true; // does exist
#else
	return false; // does not exist
#endif // JNI_PROFILER
}

} // extern "C"

