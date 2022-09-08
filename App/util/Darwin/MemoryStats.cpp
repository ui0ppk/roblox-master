#include "MemoryStats.h"

#include <mach/mach.h>
#include <sys/types.h>
#include <sys/sysctl.h>

// memory functions from stackoverflow.com
namespace RBX {
namespace MemoryStats {
        
memsize_t usedMemoryBytes()
{
	struct task_basic_info info;
	mach_msg_type_number_t size = sizeof(info);
	kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
	return (kerr == KERN_SUCCESS) ? info.resident_size : 0;
}

memsize_t freeMemoryBytes()
{
	mach_port_t host_port = mach_host_self();
	mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
	vm_size_t pagesize;
	vm_statistics_data_t vm_stat;
	
	host_page_size(host_port, &pagesize);
	(void) host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
	return vm_stat.free_count * pagesize;
}

memsize_t totalMemoryBytes()
{
	// https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/sysctl.3.html
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    u_int namelen = sizeof(mib) / sizeof(mib[0]);
    uint64_t size;
    size_t len = sizeof(size);
    
    if (sysctl(mib, namelen, &size, &len, NULL, 0) >= 0)
        return size;

    return 0;
}

} // namespace MemoryStats
} // namespace RBX
