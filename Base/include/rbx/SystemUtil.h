#pragma once

#include <string>

#ifdef _WIN32
typedef unsigned __int64    uint64_t;
#endif

namespace RBX
{
    namespace SystemUtil
	{
		/// CPU Related
		std::string getCPUMake();		
		uint64_t getCPUSpeed();
		uint64_t getCPULogicalCount();
		uint64_t getCPUCoreCount();
		uint64_t getCPUPhysicalCount();
		bool isCPU64Bit();
		
		/// Memory Related
		uint64_t getMBSysRAM();
		uint64_t getMBSysAvailableRAM();
		uint64_t getVideoMemory();

		/// OS Related
        std::string osPlatform();
        int osPlatformId();
        std::string osVer();
		std::string deviceName();
		
		/// GPU Related
		std::string getGPUMake();
		
		// Display Resolution
		std::string getMaxRes();
    }
}