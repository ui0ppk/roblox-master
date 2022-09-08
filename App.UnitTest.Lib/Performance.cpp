
#include "rbx/test/Performance.h"
#include <boost/test/unit_test.hpp>

#ifdef _WIN32
#include "Windows.h"
#elif defined(__APPLE__) 
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

RBX::Test::Memory::Memory()
{
	initialBytes = 0;
	initialBytes = GetBytes();
}

long RBX::Test::Memory::GetBytes()
{
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(MEMORYSTATUSEX);
	BOOST_REQUIRE(GlobalMemoryStatusEx(&status));
	// This ain't pretty, but it is notoriously hard to get a good value for bytes used by a process:
	return (long)status.ullTotalVirtual - (long)status.ullAvailVirtual - initialBytes;
#else
	
// "MACPORT: TBD for now, implementing something however we need to discuss how to handle this properly"
	int mib[2];
	uint64_t memsize;
	size_t len;
	
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE; /*uint64_t: physical ram size */
	len = sizeof(memsize);
	sysctl(mib, 2, &memsize, &len, NULL, 0);
	
	return (long)memsize;
	
#endif
}