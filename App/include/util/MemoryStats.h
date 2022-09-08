#pragma once

#include <stddef.h>
#include <boost/cstdint.hpp>
#include "standardout.h"
#include "FastLog.h"

namespace RBX {

// Utility functions for determining used/free/total memory.
namespace MemoryStats {

enum MemoryLevel
{
    MEMORYLEVEL_ALL_CRITICAL_LOW,
    MEMORYLEVEL_ONLY_PHYSICAL_CRITICAL_LOW,
    MEMORYLEVEL_ALL_LOW,
    MEMORYLEVEL_ONLY_PHYSICAL_LOW,
    MEMORYLEVEL_LIMITED,
    MEMORYLEVEL_OK
};

typedef boost::uint64_t memsize_t;

memsize_t usedMemoryBytes();
memsize_t freeMemoryBytes();
memsize_t totalMemoryBytes();
size_t slowGetMemoryPoolAllocation();
size_t slowGetMemoryPoolAvailability();
void releaseAllPoolMemory();
MemoryLevel slowCheckMemoryLevel(memsize_t extraMemoryUsed);

} // namespace MemoryStats
} // namespace RBX
