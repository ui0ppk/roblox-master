#pragma once

#include <string>
#include <jni.h>

namespace RBX
{
namespace JNI
{
std::string jstringToStdString(JNIEnv *jenv, jstring jstrName);
} // namespace JNI
} // namespace RBX
