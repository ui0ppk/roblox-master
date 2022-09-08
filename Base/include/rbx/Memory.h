

#ifndef _0632EE02291848e49902EAA033B8C2EA
#define _0632EE02291848e49902EAA033B8C2EA

#include "boost/pool/singleton_pool.hpp"
#include "boost/scoped_ptr.hpp"
#include <assert.h>
#include "rbx/debug.h"
#include "rbx/atomic.h"
#include <vector>

// Note - Interlock Incs, Decs are turned off on count because of possible performance issues
// TODO: Benchmark RBX_ALLOCATOR_COUNTS
#ifdef _DEBUG
#define RBX_ALLOCATOR_COUNTS
#define RBX_POOL_ALLOCATION_STATS
#endif

// TODO: Benchmark:
#ifndef _DEBUG
// Note: Using this option makes it harder to find memory leaks
#define RBX_ALLOCATOR_SINGLETON_POOL
#endif

// TODO: Benchmark:
//#define RBX_MEMORY_SCALABLE_MALLOC

namespace RBX {
#ifdef RBX_POOL_ALLOCATION_STATS
    extern std::vector<size_t*> poolAllocationList;
#endif
    typedef bool (*releaseFunc)();
    extern std::vector<size_t*> poolAvailabilityList;
    extern std::vector<releaseFunc> poolReleaseMemoryFuncList;
    
    inline void addToPool(size_t* allocatedSize, size_t* availableSize, size_t size)
    {
        if (size > *availableSize)
        {
#ifdef RBX_POOL_ALLOCATION_STATS
            (*allocatedSize)+=(size);
#endif
        }
        else
        {
            (*availableSize)-=(size);
        }
    }

    inline void removeFromPool(size_t* availableSize, size_t size)
    {
        (*availableSize)+=(size);
    }

	// You can use this allocator when using std or boost collections
	class roblox_allocator
	{
    public:
        static bool crashOnAllocationFailure;	// TODO: Put this in more places, including std allocator overrides?
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        static char* malloc(const size_type bytes);
        static void free(char* const block);
        static char* realloc(char* ptr, size_t nsize);
	};

	template<class T>
	class Allocator 
	{
#ifdef RBX_ALLOCATOR_COUNTS
		static rbx::atomic<int> count;
#endif
	public:
        static size_t allocatedSize;
        static size_t availableSize;
        static bool initialized;

        Allocator()
        {
            if (!initialized)
            {
#ifdef RBX_POOL_ALLOCATION_STATS
                poolAllocationList.push_back(&allocatedSize);
#endif
                poolAvailabilityList.push_back(&availableSize);
                bool (*pReleaseMemory)() = releaseMemory;
                poolReleaseMemoryFuncList.push_back(pReleaseMemory);
                initialized = true;
            }
        }

#ifdef RBX_ALLOCATOR_SINGLETON_POOL
		// TODO: Benchmark this allocator vs. other kinds
		void* operator new(size_t nSize) {
			assert(nSize==sizeof(T));
			void* result = boost::singleton_pool<T, sizeof(T), boost::default_user_allocator_malloc_free>::malloc();
			if (!result)
			{
				if (roblox_allocator::crashOnAllocationFailure)
					RBXCRASH();	// We want a nice fat crash here so that the process quits and we can log it
				throw std::bad_alloc();
			}
#ifdef RBX_ALLOCATOR_COUNTS
			count++;
#endif
            addToPool(&allocatedSize, &availableSize, nSize);
			return result;
		}

        void* operator new( size_t size, void* p )
        {
            addToPool(&allocatedSize, &availableSize, size);
            return p;
        }

        void operator delete(void*, void*)
        {
            removeFromPool(&availableSize, sizeof(T));
        }

        static bool releaseMemory()
		{
#ifdef RBX_POOL_ALLOCATION_STATS
            allocatedSize -= availableSize;
#endif
            availableSize = 0;
            return boost::singleton_pool<T, sizeof(T), boost::default_user_allocator_malloc_free>::release_memory();
		}

        static bool purgeMemory()
        {
            // Be very careful when calling this as this is singleton pool purge
#ifdef RBX_POOL_ALLOCATION_STATS
            allocatedSize = 0;
#endif
            availableSize = 0;
            return boost::singleton_pool<T, sizeof(T), boost::default_user_allocator_malloc_free>::purge_memory();
        }

		void operator delete(void* p) {
			boost::singleton_pool<T, sizeof(T), boost::default_user_allocator_malloc_free>::free(p);
#ifdef RBX_ALLOCATOR_COUNTS
			count--;
#endif
            removeFromPool(&availableSize, sizeof(T));
		}

/////////////////////////////////////////////////////////////////////////////////////
#else
		void* operator new(size_t nSize) {
			assert(nSize==sizeof(T));
			void* result = (void*)roblox_allocator::malloc(nSize);
			if (!result)
			{
				if (roblox_allocator::crashOnAllocationFailure)
					RBXCRASH();	// We want a nice fat crash here so that the process quits and we can log it
				throw std::bad_alloc();
			}
#ifdef RBX_ALLOCATOR_COUNTS
			count++;
#endif
			return result;
		}

		void operator delete(void* p) {
			roblox_allocator::free((char*)p);
#ifdef RBX_ALLOCATOR_COUNTS
			count--;
#endif
		}

        void* operator new( size_t size, void* p )
        {
            return p;
        }

        void operator delete(void*, void*)
        {
            // placement delete, nothing to do
        }

        static bool releaseMemory()
        {
            // pool not used, nothing to do
            return true;
        }

        static bool purgeMemory()
        {
            // pool not used, nothing to do
            return true;
        }

#endif
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef RBX_ALLOCATOR_COUNTS
		static long getCount()		{return count; }
		static long getHeapSize()	{return sizeof(T) * count; }
#endif
	};

    template<class T>
    size_t Allocator<T>::allocatedSize = 0;
    template<class T>
    size_t Allocator<T>::availableSize = 0;
    template<class T>
    bool Allocator<T>::initialized = false;

#ifdef RBX_ALLOCATOR_COUNTS
	template<class T>
	rbx::atomic<int> Allocator<T>::count;
#endif

	// This class is a wrapper for boost::pool<>. It allocates extra memory used by AutoPoolObject
	// to store a pointer back to the pool.
	class AutoMemPool
	{
		boost::scoped_ptr< boost::pool<> > pool;

	public:

		// A pool object that auto free itself from the pool it was allocated from
		// MUST use this with AutoMemPool
		class Object 
		{
		public:
			void* operator new(size_t size, AutoMemPool* pool) 
			{
				RBXASSERT(((size_t)pool->getRequestedSize()) == size + sizeof(AutoMemPool*));

				void* mem = pool->malloc();
				*(AutoMemPool**)mem = &(*pool);	// store the pool at start of memory block
				return (char*)mem + sizeof(AutoMemPool*);	// skip over the pool
			}

			void operator delete(void* p, AutoMemPool* pool)
			{
				pool->free(p);
			}

			void operator delete(void *p) 
			{
				p = (char*)p - sizeof(AutoMemPool*);
				AutoMemPool* pool = *(AutoMemPool**)p;
				pool->free(p);
			}
		};

		AutoMemPool(int requested_size)
		{
			// allocate extra bytes to store pointer to the pool
			pool.reset(new boost::pool<>(requested_size + sizeof(this)));
		}

		inline void* malloc()
		{
			return pool->malloc();
		}

		inline void free(void* p)
		{
			RBXASSERT(pool->is_from(p));
			pool->free(p);
		}

		inline int getRequestedSize()
		{
			return int(pool->get_requested_size());
		}
	};

}

#endif