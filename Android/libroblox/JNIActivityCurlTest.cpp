#include "JNIUtil.h"

#include <string>

#include <stdint.h>
#include <jni.h>
#include <android/native_window.h>     // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer

#include <strings.h>

#include "FastLog.h"
#include "util/Http.h"
#include "RbxFormat.h"

LOGGROUP(Android)

using namespace RBX;
using namespace RBX::JNI;

// FileSystem.cpp:
namespace RBX
{
namespace JNI
{
extern std::string fileSystemCacheDir;
} // namespace JNI
} // namespace RBX

extern "C"
{

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityCurlTest_nativeOnStart(JNIEnv *jenv, jclass obj)
{
    FLog::SetValue("Android", "10", FASTVARTYPE_STATIC, true);
    FLog::SetValue("Network", "10", FASTVARTYPE_STATIC, true);
    FLog::SetValue("HttpTrace", "10", FASTVARTYPE_DYNAMIC, true);
    FLog::SetValue("HttpTraceStdout", "True", FASTVARTYPE_DYNAMIC, true);
//    FLog::SetValue("HttpTextOnly", "False", FASTVARTYPE_STATIC, true);
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityCurlTest_nativeOnResume(JNIEnv *jenv, jclass obj)
{
    FASTLOG(FLog::Android, "nativeOnResume");
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityCurlTest_nativeOnPause(JNIEnv *jenv, jclass obj)
{
    FASTLOG(FLog::Android, "nativeOnPause");
}

JNIEXPORT void JNICALL Java_com_roblox_client_ActivityCurlTest_nativeOnStop(JNIEnv *jenv, jclass obj)
{
    FASTLOG(FLog::Android, "nativeOnStop");
}

JNIEXPORT jstring JNICALL Java_com_roblox_client_ActivityCurlTest_nativeGetURL(JNIEnv *jenv, jclass obj, jstring jsURL)
{
    std::string url = jstringToStdString(jenv, jsURL);
    Http http(url.c_str());
    try
    {
        std::string response;
        bool externalRequest = url.find("roblox.com") == std::string::npos && url.find("robloxlabs.com") == std::string::npos;
        http.get(response, externalRequest);
        FASTLOGS(FLog::Android, "nativeGetURL: %s", response.c_str());
        return jenv->NewStringUTF(response.c_str());
    }
    catch (const RBX::base_exception& e)
    {
        FASTLOGS(FLog::Android, "HTTP error: %s", e.what());
        throw e;
    }
}

JNIEXPORT jstring JNICALL Java_com_roblox_client_ActivityCurlTest_nativePostAnalytics(JNIEnv *jenv, jclass obj, jstring jsCategory, jstring jsAction, jint value, jstring jsLabel)
{
    std::string category = jstringToStdString(jenv, jsCategory);
    std::string action = jstringToStdString(jenv, jsAction);
    std::string label = jstringToStdString(jenv, jsLabel);

    Http http("http://www.google-analytics.com/collect");
    std::stringstream ss;
    ss
            << "v=1"
            << "&tid=" << "UA-43420590-13"
            << "&cid=" << "1234"
            << "&t="   << "event"
            << "&ec="  << category
            << "&ea="  << action
            << "&ev="  << value
            << "&el="  << label;
    try
    {
        std::string response;
        http.post(ss, Http::kContentTypeDefaultUnspecified, true, response, true);
        FASTLOGS(FLog::Android, "nativePostAnalytics: %s", response.c_str());
        return jenv->NewStringUTF(response.c_str());
    }
    catch (const RBX::base_exception& e)
    {
        FASTLOGS(FLog::Android,"HTTP error: %s", e.what());
        throw e;
    }
}

} // extern "C"
