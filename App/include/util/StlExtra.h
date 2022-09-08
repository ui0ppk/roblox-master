/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"
#include <vector>


namespace RBX {

	// only fast for very short vectors - should do no allocation
	// returns index of the item removed

	template<class T>
	void fastRemoveIndex(std::vector<T>& vec, size_t index)
	{
		RBXASSERT(index >= 0);
		RBXASSERT(index < vec.size());
		RBXASSERT(!vec.empty());
		RBXASSERT(vec.size() < 32);			// Note - possibly should be using some other container here - to find item requires N time
#ifdef _DEBUG
		size_t currentCapacity = vec.capacity();
#endif
		size_t newSize = vec.size() - 1;

		if (index < newSize)
		{
			vec[index] = vec.back();
		}
		vec.resize(newSize);				// hopefully, don't do a memory realloc/shrink

#ifdef _DEBUG
		RBXASSERT(currentCapacity == vec.capacity());		// confirm no reallocation
#endif
	}


	// only fast for very short vectors - should do no allocation
	// returns index of the item removed
	template<class T>
	size_t fastRemoveShort(std::vector<T>& vec, const T& item)
	{
		typename std::vector<T>::iterator it = std::find(vec.begin(), vec.end(), item);

		size_t answer = it - vec.begin();

		RBXASSERT(vec[answer] == item);
		RBXASSERT(it != vec.end());
		RBXASSERT(vec.size() < 32);			// Note - possibly should be using some other container here - to find item requires N time
#ifdef _DEBUG
		size_t currentCapacity = vec.capacity();
#endif

		typename std::vector<T>::iterator lastOne(vec.end());
		--lastOne;

		RBXASSERT(*lastOne == vec.back());

		if (it != lastOne) {
			*it = *lastOne;					// move back item into the place once held by item
		}
		vec.resize(vec.size() - 1);				// hopefully, don't do a memory realloc/shrink

#ifdef _DEBUG
		RBXASSERT(currentCapacity == vec.capacity());		// confirm no reallocation
#endif
		return answer;
	}



}	// namespace RBX

