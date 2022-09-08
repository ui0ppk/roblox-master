#include "stdafx.h"

#include "security/SecurityContext.h"
#include "Script/LuaMemory.h"
#include "lua/lua.hpp"
#include "lobject.h"
#include <algorithm>

#include "rbx/Profiler.h"

using namespace RBX;

#define MEM_POOL_INCREMENT 4
#define MAX_NUM_MEM_POOLS 16

LOGGROUP(LuaMemoryPool);
FASTINTVARIABLE(LuaMemoryBonus, 0)

// helper function
int getMemPoolIndex(int size)
{
	if (size % MEM_POOL_INCREMENT == 0)
	{
		int index = ((size - sizeof(Udata)) / MEM_POOL_INCREMENT) - 1;
		return (index < MAX_NUM_MEM_POOLS) ? index : -1;
	}
	
	return -1;
}

size_t LuaAllocator::heapLimit = 0;

LuaAllocator::LuaAllocator(bool usePool)
	: heapSize(0)
	, heapCount(0)
	, maxHeapSize(0)
	, maxHeapCount(0)
{
	if (usePool)
	{
		// initialize memory pools with sizes that are multiples of 4
		for (int i = 1; i <= MAX_NUM_MEM_POOLS; i++)
			memPools.push_back(new boost::pool<>(sizeof(Udata) + (i * MEM_POOL_INCREMENT)));
	}
}

LuaAllocator::~LuaAllocator()
{
	if (!memPools.empty())
	{
		// clear memory pool
		for (unsigned int i = 0; i < memPools.size(); i++)
		{
			delete memPools[i]; 
			memPools[i] = NULL;
		}
		memPools.clear();
	}
}

void LuaAllocator::clearHeapMax()
{
	this->maxHeapSize = 0;
	this->maxHeapCount = 0;
}

void LuaAllocator::getHeapStats(size_t& heapSize, size_t& heapCount, size_t& maxHeapSize, size_t& maxHeapCount) const
{
	getHeapStats(heapSize, heapCount);
	maxHeapSize = this->maxHeapSize;
	maxHeapCount = this->maxHeapCount;
}

void LuaAllocator::getHeapStats(size_t& heapSize, size_t& heapCount) const
{
	heapSize = this->heapSize;
	heapCount = this->heapCount;
}

void* LuaAllocator::alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	LuaAllocator* allocator = reinterpret_cast<LuaAllocator*>(ud);
	return allocator->alloc(ptr, osize, nsize);
}

bool LuaAllocator::hasSpace(const long diff)
{
	if (heapLimit > 0 && diff > 0 && diff + heapSize > heapLimit &&
		(RBX::Security::Context::current().identity == RBX::Security::GameScript_ || RBX::Security::Context::current().identity == RBX::Security::RobloxGameScript_) )
		return false;
	return true;
}

void* LuaAllocator::alloc(void *ptr, size_t osize, size_t nsize)
{
	const long diff = nsize - osize;
    
	if(!hasSpace(diff))
	{
		FASTLOG(FLog::LuaMemoryPool, "lua alloc has no space");
		return NULL;
	}

	void* result;

	if (!memPools.empty())
	{
		if (nsize == 0)
		{
			int index = getMemPoolIndex(osize);
			if (index > -1)
			{
				RBXASSERT(osize == memPools[index]->get_requested_size());
				memPools[index]->free(ptr);
			}
			else
				free(ptr);

			result = NULL;
		}
		else // allocate new memory
		{
			int index1 = getMemPoolIndex(nsize);
			if (index1 > -1)
			{
				RBXASSERT(nsize == memPools[index1]->get_requested_size());

				// copy data to new pool
				result = memPools[index1]->malloc();
				memcpy(result, ptr, std::min(nsize, osize));

				// remove from old pool
				int index2 = getMemPoolIndex(osize);
				if (index2 > -1)
				{
					RBXASSERT(osize == memPools[index2]->get_requested_size());

					memPools[index2]->free(ptr);
				}
			}
			else
			{
				int index2 = getMemPoolIndex(osize);
				if (index2 > -1)
				{
					RBXASSERT(osize == memPools[index2]->get_requested_size());

					// was in a pool but does not belong to any new pools
					result = malloc(nsize);
					memcpy(result, ptr, std::min(osize, nsize));

					// remove from old pool
					memPools[index2]->free(ptr);
				}
				else
				{
					// does not belong to any pool
					result = realloc(ptr, nsize);
				}
			}
		}
	}
	else
	{
		if (nsize == 0)
		{
			free(ptr);
			result = NULL;
		}
		else
		{
			result = realloc(ptr, nsize + FInt::LuaMemoryBonus);
		}
	}

	heapSize += diff;

	if (osize==0)
		++heapCount;
	if (nsize==0)
		--heapCount;

	maxHeapSize = std::max<size_t>(maxHeapSize, heapSize);
	maxHeapCount = std::max<size_t>(maxHeapCount, heapCount);

    RBXPROFILER_COUNTER_ADD("memory/lua/heap", diff);

	return result;
}
