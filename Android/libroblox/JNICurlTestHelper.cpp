#include "JNIUtil.h"

#include <string>

#include <stdint.h>
#include <jni.h>

#include <strings.h>

#include "FastLog.h"
#include "util/Http.h"

#include <android/log.h>

LOGGROUP(Android)

using namespace RBX;
using namespace RBX::JNI;

namespace RBX
{
namespace JNI
{

} // namespace JNI
} // namespace RBX

extern "C"
{

JNIEXPORT void JNICALL Java_com_roblox_client_test_CurlTestHelper_initHttpJNI( JNIEnv* jenv, jclass obj) {

    //__android_log_print(ANDROID_LOG_INFO,  "InstrumentationTest", "JNIActivity initHttpJNI()");
    Http::init(Http::WinHttp, Http::CookieSharingSingleProcessMultipleThreads);
}

JNIEXPORT jstring JNICALL Java_com_roblox_client_test_CurlTestHelper_getCookieStringJNI( JNIEnv* jenv, jclass obj) {
    std::string cookieStr;
    Http::getCookiesForDomain("roblox.com", cookieStr);

    //__android_log_print(ANDROID_LOG_INFO,  "InstrumentationTest", "JNIActivity getCookieStringJNI() %s", cookieStr.c_str());
    return jenv->NewStringUTF(cookieStr.c_str());
}

JNIEXPORT jstring JNICALL Java_com_roblox_client_test_CurlTestHelper_doCurlRequestJNI( JNIEnv* jenv, jclass obj, jstring jURL) {

    std::string url = jstringToStdString(jenv, jURL);
    //__android_log_print(ANDROID_LOG_INFO,  "InstrumentationTest", "JNIActivity doCurlRequestJNI() %s", url.c_str());
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

} // extern "C"
