/**
 * BaldPtr.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include "rbx/Debug.h"

namespace RBX
{

/**
 * Wraps raw pointers with a layer of protection in debug builds.
 * Checks for bad pointers on access.
 */
template<class T>
class BaldPtr
{
public:

    /**
     * Constructor.
     *  Sets pointer to NULL.
     */
    inline BaldPtr() 
        : mPointer(NULL)
    {}

	/**
	 * Constructor.
	 *  Pointer is also validated.
	 *
	 * @param	Pointer     pointer to wrap, may be NULL
	 */
	inline BaldPtr(T* Pointer)
        : mPointer(Pointer) 
    {
        validate();
	}

	/**
	 * Shallow copies pointer.
	 *  Pointer is validated.
	 *
	 * @param	Pointer     pointer to copy, may be NULL
	 * @return  this pointer
	 */
	inline T*& operator=(T* Pointer) 
    {
		mPointer = Pointer;
		validate();
		return mPointer;
	}

    /**
     * Dereference operator.
     *  Checks for NULL and validates.
     *
     * @return  reference to pointer
     */
    inline T& operator*() const
    {
		RBXASSERT_VERY_FAST(mPointer);
        validate();
		return *mPointer;
	}

    /**
     * Class pointer cast.
     *  Validates pointer.
     *
     * @return  pointer cast to a class *, may be NULL
     */
    inline operator T*() const
    {
		validate();
		return mPointer;
	}

	/**
	 * Arrow operator that checks and returns the pointer.
	 *  Checks for NULL and validates.
	 *
	 * @return  data pointer
	 */
	inline T* operator->() const
    {
		RBXASSERT_VERY_FAST(mPointer);
		validate();
		return mPointer;
	}

    /**
     * Get the raw pointer value.
     *  Performs no checks.
     *
     * @return  data pointer, may be NULL
     */
    inline T* get() const { return mPointer; }

	/**
	 * Validates the pointer.
	 *  Various memory patterns are checked such as fence posts, 
	 *  deleted, and allocated memory.  Standard CRT patterns are
	 *  checked as well as Ogre patterns.
	 *
	 * @return 	description
	 */
	inline void validate() const
    {
#ifdef __RBX_VERY_FAST_ASSERT
        if ( !mPointer )
            return;
#endif

		RBXASSERT_VERY_FAST(
            // CRT debug allocator
            (unsigned)mPointer != 0xCCCCCCCC &&     // uninitialized stack memory
		    (unsigned)mPointer != 0xCDCDCDCD &&     // uninitialized heap memory
		    (unsigned)mPointer != 0xFDFDFDFD &&     // "no man's land" guard bytes before and after allocated heap memory
		    (unsigned)mPointer != 0xDDDDDDDD &&     // deleted heap memory
            (unsigned)mPointer != 0xFEEEFEEE &&     // deleted heap memory

            // Ogre allocator
            (unsigned)mPointer != 0xBAADF00D &&     // before "no man's land" guard bytes
            (unsigned)mPointer != 0xDEADC0DE &&     // after "no man's land" guard bytes
            (unsigned)mPointer != 0xFEEDFACE &&     // uninitialized memory
            (unsigned)mPointer != 0xDEADBEEF  );    // deleted memory
	}

private:

    T* mPointer;
};

}

// struct Foo
// {
// 	int x;
// };
// 
// template<typename Ptr>
// void test()
// {
// 	Ptr f = new Foo();
// 	Foo* f2 = f;
// 	f->x = 23;
// 	(*f).x = 32;
// 	void* v = f;
// 	BaldPtr<Foo> b = f;
// 	BaldPtr<Foo> b2(f);
// 	const void* vc = f;
// 
// 	delete f;
// }
// 
// template<typename Ptr>
// void testConst()
// {
// 	Ptr f = new Foo();
// 	const Foo* f2 = f;
// 	int x = f->x;
// 	int y = (*f).x;
// 	const void* v = f;
// 	BaldPtr<const Foo> b = f;
// 	BaldPtr<const Foo> b2(f);
// 
// 	delete f;
// }
// 
// int _tmain(int argc, _TCHAR* argv[])
// {
// 	test<BaldPtr<Foo> const>();
// 	test<Foo* const>();
// 	test<BaldPtr<Foo> >();
// 	test<Foo* >();
// 
// 	testConst<BaldPtr<const Foo> const>();
// 	testConst<const Foo* const>();
// 	testConst<BaldPtr<const Foo> >();
// 	testConst<const Foo* >();
// 
// 	return 0;
// }

