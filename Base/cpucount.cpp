#include "CPUCount.h"

#if defined (_WIN32)
#   include <thread>
#else
#   include "rbx/SystemUtil.h"
#endif

unsigned int RbxTotalUsableCoreCount(unsigned int defaultValue)
{
#ifdef _WIN32
    unsigned n = std::thread::hardware_concurrency();
    return n ? n : defaultValue;
#else
	return RBX::SystemUtil::getCPUCoreCount();
#endif
}
