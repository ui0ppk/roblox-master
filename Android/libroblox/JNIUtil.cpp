#include "JNIUtil.h"

#include <string>

#include <jni.h>

namespace RBX
{
namespace JNI
{
std::string cacheDirectory;
std::string assetFolderPath;

std::string jstringToStdString(JNIEnv *jenv, jstring jstrName)
{
    jboolean isCopy;
    const char *nativeString = jenv->GetStringUTFChars(jstrName, &isCopy);
    const size_t kLength = strlen(nativeString)+1;
    char *cstr = new char[kLength];
    strncpy(cstr, nativeString, kLength);
    if (isCopy)
    {
        jenv->ReleaseStringUTFChars(jstrName, nativeString);
    }

    std::string str = cstr;
    delete [] cstr;
    return str;
}
} // JNI
} // RBX
