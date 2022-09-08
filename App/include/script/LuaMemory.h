#pragma once

#include "Util/Memory.h"
#include "boost/pool/object_pool.hpp"
#include "boost/iostreams/filter/gzip.hpp"


namespace RBX
{
	class LuaAllocator
	{
	private:
		size_t heapSize;
		size_t heapCount;
		size_t maxHeapSize;
		size_t maxHeapCount;

		// memory pools
		std::vector<boost::pool<>*> memPools;

	public:
		LuaAllocator(bool usePool = false);
		~LuaAllocator();

		static size_t heapLimit;	// maximum heap size allowed. 0 == no limit

		void clearHeapMax();
		void getHeapStats(size_t& heapSize, size_t& heapCount, size_t& maxHeapSize, size_t& maxHeapCount) const;
		void getHeapStats(size_t& heapSize, size_t& heapCount) const;

		bool hasSpace(const long diff);
		virtual void* alloc(void *ptr, size_t osize, size_t nsize);
		static void * alloc(void *ud, void *ptr, size_t osize, size_t nsize);

	};
}
