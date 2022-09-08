#include "rbx/SystemUtil.h"

namespace RBX
{
    namespace SystemUtil
    {
        std::string osVer()
        {
            return "Durango";
        }

        bool isCPU64Bit()
        {
            return true;
        }

        int osPlatformId()
        {
            return -1;
        }

        std::string osPlatform()
        {
            return "Durango";
        }

        std::string getGPUMake()
        {
            return "Durango";
        }

        std::string deviceName()
        {
            return "Durango";
        }

        uint64_t getVideoMemory()
        { 
            return ((1U<<31U)-1); 
        } 
    }
}