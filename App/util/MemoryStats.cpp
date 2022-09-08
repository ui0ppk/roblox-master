#include "util/MemoryStats.h"

#if defined(_WIN32) // should only be used with Microsoft platforms
#include <Windows.h>

#if !defined(RBX_PLATFORM_DURANGO)
#include <psapi.h>
#endif

using namespace RBX;
using namespace RBX::MemoryStats;

namespace RBX {
	namespace MemoryStats {
#if !defined(RBX_PLATFORM_DURANGO)
		MEMORYSTATUSEX globalMemoryStatusEx() {
			MEMORYSTATUSEX statex;
			statex.dwLength = sizeof(statex);
			GlobalMemoryStatusEx(&statex);
			return statex;
		}
#endif

		DWORDLONG usedMemoryBytes() {
#if !defined(RBX_PLATFORM_DURANGO)
			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
			return pmc.WorkingSetSize;
#elif defined(RBX_PLATFORM_WIN_PHONE)
			return Windows::System::MemoryManager::AppMemoryUsage;
#else
			// TODO:WinRT Windows Surface App
			return 0;
#endif
		}

		DWORDLONG freeMemoryBytes() {
#if !defined(RBX_PLATFORM_DURANGO)
			MEMORYSTATUSEX statex = globalMemoryStatusEx();
			return statex.ullAvailPhys;
#elif defined(RBX_PLATFORM_WIN_PHONE)
			return Windows::System::MemoryManager::AppMemoryUsageLimit - Windows::System::MemoryManager::AppMemoryUsage;
#else
			// TODO:WinRT Windows Store App
			return 0;
#endif

		}

		DWORDLONG totalMemoryBytes() {
#if defined (RBX_PLATFORM_DURANGO)
            _TITLEMEMORYSTATUS status;
            status.dwLength = sizeof(TITLEMEMORYSTATUS);
            TitleMemoryStatus(&status);
            return status.ullTotalMem;
#elif defined(RBX_PLATFORM_WIN_PHONE)
            return Windows::System::MemoryManager::AppMemoryUsageLimit;
#else
            MEMORYSTATUSEX statex = globalMemoryStatusEx();
            return statex.ullTotalPhys;
#endif
		}
	}
}
#endif // defined(_WIN32)
