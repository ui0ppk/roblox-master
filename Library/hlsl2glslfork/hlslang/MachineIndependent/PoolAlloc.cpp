// Copyright (c) The HLSL2GLSLFork Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.txt file.


#include "../Include/PoolAlloc.h"
#include "../Include/Common.h"

#include "../Include/InitializeGlobals.h"
#include "osinclude.h"

OS_TLSIndex PoolIndex;

GlobalFreeFunction _delete = 0;
GlobalAllocateFunction _allocate = 0;
void* _user_data = 0;

void SetGlobalAllocationAllocator(GlobalAllocateFunction alloc, GlobalFreeFunction free, void* user) {
	_allocate = alloc;
	_delete = free;
	_user_data = user;
}

void InitializeGlobalPools()
{
   TThreadGlobalPools* globalPools= static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));    
   if (globalPools)
      return;

	void* poolMem, *threadPoolMem;
	TPoolAllocator *globalPoolAllocator;
	TThreadGlobalPools* threadData;
	
	if (_allocate) {
		poolMem = _allocate(sizeof(TPoolAllocator), _user_data);
		threadPoolMem = _allocate(sizeof(TThreadGlobalPools), _user_data);
		globalPoolAllocator = new (poolMem) TPoolAllocator(true);
		threadData = new (threadPoolMem) TThreadGlobalPools();
	} else {
		globalPoolAllocator = new TPoolAllocator(true);
		threadData = new TThreadGlobalPools();
	}

	threadData->globalPoolAllocator = globalPoolAllocator;

	OS_SetTLSValue(PoolIndex, threadData);     
	globalPoolAllocator->push();
}

void FreeGlobalPools()
{
   // Release the allocated memory for this thread.
   TThreadGlobalPools* globalPools= static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));    
   if (!globalPools)
      return;

	GlobalPoolAllocator.popAll();
	if (_delete) {
	   _delete(&GlobalPoolAllocator, _user_data);
	   _delete(globalPools, _user_data);
	} else {
		delete &GlobalPoolAllocator;
		delete globalPools;
	}
}

bool InitializePoolIndex()
{
   // Allocate a TLS index.
   if ((PoolIndex = OS_AllocTLSIndex()) == OS_INVALID_TLS_INDEX)
      return false;

   return true;
}

void FreePoolIndex()
{
   // Release the TLS index.
   OS_FreeTLSIndex(PoolIndex);
}

TPoolAllocator& GetGlobalPoolAllocator()
{
   TThreadGlobalPools* threadData = static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));

   return *threadData->globalPoolAllocator;
}

void SetGlobalPoolAllocatorPtr(TPoolAllocator* poolAllocator)
{
   TThreadGlobalPools* threadData = static_cast<TThreadGlobalPools*>(OS_GetTLSValue(PoolIndex));

   threadData->globalPoolAllocator = poolAllocator;
}

//
// Implement the functionality of the TPoolAllocator class, which
// is documented in PoolAlloc.h.
//
TPoolAllocator::TPoolAllocator(bool g, int growthIncrement, int allocationAlignment) : 
global(g),
pageSize(growthIncrement),
alignment(allocationAlignment),
freeList(0),
inUseList(0),
numCalls(0),
totalBytes(0)
{
   //
   // Don't allow page sizes we know are smaller than all common
   // OS page sizes.
   //
   if (pageSize < 4*1024)
      pageSize = 4*1024;

   //
   // A large currentPageOffset indicates a new page needs to
   // be obtained to allocate memory.
   //
   currentPageOffset = pageSize;

   //
   // Adjust alignment to be at least pointer aligned and
   // power of 2.
   //
   size_t minAlign = sizeof(void*);
   alignment &= ~(minAlign - 1);
   if (alignment < minAlign)
      alignment = minAlign;
   size_t a = 1;
   while (a < alignment)
      a <<= 1;
   alignment = a;
   alignmentMask = a - 1;

   //
   // Align header skip
   //
   headerSkip = minAlign;
   if (headerSkip < sizeof(tHeader))
   {
      headerSkip = (sizeof(tHeader) + alignmentMask) & ~alignmentMask;
   }
}

TPoolAllocator::~TPoolAllocator()
{
   if (!global)
   {
      //
      // Then we know that this object is not being 
      // allocated after other, globally scoped objects
      // that depend on it.  So we can delete the "in use" memory.
      //
      while (inUseList)
      {
         tHeader* next = inUseList->nextPage;
         inUseList->~tHeader();
         delete [] reinterpret_cast<char*>(inUseList);
         inUseList = next;
      }
   }

   //
   // Always delete the free list memory - it can't be being
   // (correctly) referenced, whether the pool allocator was
   // global or not.
   //
   while (freeList)
   {
      tHeader* next = freeList->nextPage;
      delete [] reinterpret_cast<char*>(freeList);
      freeList = next;
   }
}


void TPoolAllocator::push()
{
   tAllocState state = { currentPageOffset, inUseList};

   stack.push_back(state);

   //
   // Indicate there is no current page to allocate from.
   //
   currentPageOffset = pageSize;
}

//
// Do a mass-deallocation of all the individual allocations
// that have occurred since the last push(), or since the
// last pop(), or since the object's creation.
//
// The deallocated pages are saved for future allocations.
//
void TPoolAllocator::pop()
{
   if (stack.size() < 1)
      return;

   tHeader* page = stack.back().page;
   currentPageOffset = stack.back().offset;

   while (inUseList != page)
   {
      // invoke destructor to free allocation list
      inUseList->~tHeader();

      tHeader* nextInUse = inUseList->nextPage;
      if (inUseList->pageCount > 1)
         delete [] reinterpret_cast<char*>(inUseList);
      else
      {
         inUseList->nextPage = freeList;
         freeList = inUseList;
      }
      inUseList = nextInUse;
   }

   stack.pop_back();
}

//
// Do a mass-deallocation of all the individual allocations
// that have occurred.
//
void TPoolAllocator::popAll()
{
   while (stack.size() > 0)
      pop();
}

void* TPoolAllocator::allocate(size_t numBytes)
{
   size_t allocationSize = numBytes;

   //
   // Just keep some interesting statistics.
   //
   ++numCalls;
   totalBytes += numBytes;

   //
   // Do the allocation, most likely case first, for efficiency.
   // This step could be moved to be inline sometime.
   //
   if (currentPageOffset + allocationSize <= pageSize)
   {
      //
      // Safe to allocate from currentPageOffset.
      //
      unsigned char* memory = reinterpret_cast<unsigned char *>(inUseList) + currentPageOffset;
      currentPageOffset += allocationSize;
      currentPageOffset = (currentPageOffset + alignmentMask) & ~alignmentMask;

      return memory;
   }

   if (allocationSize + headerSkip > pageSize)
   {
      //
      // Do a multi-page allocation.  Don't mix these with the others.
      // The OS is efficient and allocating and free-ing multiple pages.
      //
      size_t numBytesToAlloc = allocationSize + headerSkip;
      tHeader* memory = reinterpret_cast<tHeader*>(::new char[numBytesToAlloc]);
      if (memory == 0)
         return 0;

      // Use placement-new to initialize header
      new(memory) tHeader(inUseList, (numBytesToAlloc + pageSize - 1) / pageSize);
      inUseList = memory;

      currentPageOffset = pageSize;  // make next allocation come from a new page

      return reinterpret_cast<void*>(reinterpret_cast<UINT_PTR>(memory) + headerSkip);
   }

   //
   // Need a simple page to allocate from.
   //
   tHeader* memory;
   if (freeList)
   {
      memory = freeList;
      freeList = freeList->nextPage;
   }
   else
   {
      memory = reinterpret_cast<tHeader*>(::new char[pageSize]);
      if (memory == 0)
         return 0;
   }

   // Use placement-new to initialize header
   new(memory) tHeader(inUseList, 1);
   inUseList = memory;

   unsigned char* ret = reinterpret_cast<unsigned char *>(inUseList) + headerSkip;
   currentPageOffset = (headerSkip + allocationSize + alignmentMask) & ~alignmentMask;

   return ret;
}
