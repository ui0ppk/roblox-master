#include "rbx/SystemUtil.h"
#include "util/StreamHelpers.h"

#include <android/api-level.h>

#include <fstream>
#include <string>

#include <unistd.h>

using namespace RBX;

namespace RBX
{
namespace SystemUtil
{
std::string mOSVersion; // Set in JNIGLActivity.cpp
std::string mDeviceName;
    
std::string getCPUMake()
{
#ifdef __arm__
    return "ARM";
#elif __i386__
    return "Intel";
#else
#error Unsupported platform.
#endif
}

uint64_t getCPUSpeed()
{
    return 0;
}

uint64_t getCPULogicalCount()
{
    return getCPUCoreCount();
}

uint64_t getCPUCoreCount()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

uint64_t getCPUPhysicalCount()
{
    return getCPUCoreCount();
}

bool isCPU64Bit()
{
#ifdef __arm64__
    return true;
#else
    return false;
#endif
}

uint64_t getMBSysRAM()
{
    return 900;
}

uint64_t getMBSysAvailableRAM()
{
    return 0; // Does not appear to be used.
}

uint64_t getVideoMemory()
{
    return 67108864; // 64Mb, hard coded same as iOS.
}

std::string osPlatform()
{
    return "Android";
}

int osPlatformId()
{
    return __ANDROID_API__;
}
    
std::string osVer()
{
    return mOSVersion;
}
    
std::string deviceName()
{
    return RBX::SystemUtil::mDeviceName;
}

std::string getGPUMake()
{
    return "Android";
}

std::string getMaxRes()
{
    return "";
}
} // SystemUtil
} // RBX
