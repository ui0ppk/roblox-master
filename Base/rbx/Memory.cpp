/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

// This prevent inclusion of winsock.h in windows.h, which prevents windows redifinition errors
// Look at winsock2.h for details, winsock2.h is #included from boost.hpp & other places.
#ifdef _WIN32
#define _WINSOCKAPI_  
#endif

#include "rbx/Memory.h"


bool RBX::roblox_allocator::crashOnAllocationFailure = true;

namespace RBX 
{
    std::vector<size_t*> poolAvailabilityList;
    std::vector<releaseFunc> poolReleaseMemoryFuncList;
    std::vector<size_t*> poolAllocationList;

#ifdef RBX_MEMORY_SCALABLE_MALLOC
	char* roblox_allocator::malloc(const size_type size)
	{
		return reinterpret_cast<char *>(scalable_malloc(size > 0 ? size : 1));
	}
	void roblox_allocator::free(char * const block)
	{
		return scalable_free(block);
	}
	char* roblox_allocator::realloc(char *ptr, size_t nsize)
	{
		return reinterpret_cast<char *>(scalable_realloc(ptr, nsize));
	}
#else
	char* roblox_allocator::malloc(const size_type size)
	{
		char* result = (char*)std::malloc(size > 0 ? size : 1);
		if (!result && size && crashOnAllocationFailure)
			RBXCRASH();	// We want a nice fat crash here so that the process quits and we can log it
		return result;
	}
	void roblox_allocator::free(char * const block)
	{
		return std::free(block);
	}
	char* roblox_allocator::realloc(char *ptr, size_t nsize)
	{
		char* result = (char*)std::realloc(ptr, nsize);
		if (!result && nsize && crashOnAllocationFailure)
			RBXCRASH();	// We want a nice fat crash here so that the process quits and we can log it
		return result;
	}
#endif
}

// Globally override new/delete operators
#ifdef RBX_MEMORY_SCALABLE_MALLOC

void* operator new(size_t size)
{
	// No use of std::new_handler because it cannot be done in portable
	// and thread-safe way
	//
	// We throw std::bad_alloc() when scalable_malloc returns NULL
	//(we return NULL if it is a no-throw implementation)
	if (void* ptr = scalable_malloc(size > 0 ? size : 1))
        return ptr;
	if (RBX::roblox_allocator::crashOnAllocationFailure)
		RBXCRASH();	// We want a nice fat crash here so that the process quits and we can log it
    throw std::bad_alloc();
}

void* operator new[](size_t size)
{
    return operator new (size);
}

void* operator new(size_t size, const std::nothrow_t&) throw ()
{
    if (void* ptr = scalable_malloc(size > 0 ? size : 1))
        return ptr;
    return NULL;
}

void* operator new[](size_t size, const std::nothrow_t&) throw ( )
{
    return operator new(size, std::nothrow);
}

void operator delete(void* ptr) throw ( )
{
    if (ptr != 0) 
		scalable_free(ptr);
}

void operator delete[](void* ptr) throw ( )
{
    operator delete(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) throw ( )
{
    if (ptr != 0) 
		scalable_free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) throw ( )
{
    operator delete(ptr, std::nothrow);
}

#endif
