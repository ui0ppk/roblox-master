/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "boost/utility.hpp"
#include "util/utilities.h"
#include "Util/G3DCore.h"
#include "rbx/Debug.h"
#include "G3D/Array.h"

namespace RBX {

/* USAGE
	class C 
	{
	private:
		int	index;

	public:
		C() : index(-1) {}
		~C() {RBXASSERT(index == -1);}

		int&	getIndex() {return index;}
	};


	main()
	{
		IndexArray<C, &C::getIndex> array;

		C* c = new C();
		C  c2;

		array.fastAppend(c);
		array.fastAppend(&c2);

		array.fastRemove(c)
		array.fastRemove(&c2);
	}
*/

	template <class Item, int& (Item::*getIndex)()>
	class IndexArray 
		: public boost::noncopyable // You can't copy elements from one array to another
		                    // since the index values can't be shared.
	{
	private:
		G3D::Array<Item*> array;

		int& indexOf(Item* item) const {
			return (item->*getIndex)();
		}

	public:
	    typedef Item** Iterator;
	    typedef const Item** ConstIterator;

		inline void fastAppend(Item* item)
		{
			RBXASSERT(item);
			RBXASSERT(indexOf(item) == -1);
			RBXASSERT_IF_VALIDATING(array.find(item) == array.end());

			indexOf(item) = array.size();
			array.append(item);
		}

		inline void fastRemove(Item* item)
		{
			RBXASSERT_IF_VALIDATING(array.find(item) != array.end());

			int removeIndex = indexOf(item);

			RBXASSERT(removeIndex >= 0);
			RBXASSERT(array[removeIndex] == item);

			// Move last item to removal index
			Item* oldLast = array.last();		// if array size == 1, this is redundant
			array[removeIndex] = oldLast;
			indexOf(oldLast) = removeIndex;
			array.pop(false);

			// Update indices
			indexOf(item) = -1;
		}

		inline void remove(Item* item)
		{
			RBXASSERT_IF_VALIDATING(array.find(item) != array.end());

			int removeIndex = indexOf(item);

			RBXASSERT(removeIndex >= 0);
			RBXASSERT(array[removeIndex] == item);

			// Move all the items back in the array. 
			for (int i = removeIndex; i < array.size() - 1; i++)
			{
				Item* nextItem = array[i + 1];
				array[i] = nextItem;
				indexOf(nextItem) = i;
			}

			array.pop(false);

			// Update indices
			indexOf(item) = -1;
		}

		inline bool fastContains(Item* item) const
		{
			bool answer = (indexOf(item) >= 0);
			RBXASSERT_IF_VALIDATING(answer == underlyingArray().contains(item));
			return answer;
		}

		G3D::Array<Item*>& underlyingArray() {
			return array;
		}

		const G3D::Array<Item*>& underlyingArray() const {
			return array;
		}

		inline Item* operator[](int n) {
			RBXASSERT(indexOf(array[n]) == n);
			return array[n];
		}

		inline Item* operator[](unsigned int n) {
			RBXASSERT(indexOf(array[n]) == n);
			return array[n];
		}

		inline Item* const operator[](int n) const {		// the pointer is const?
			RBXASSERT(indexOf(array[n]) == n);
			return array[n];
		}

		inline Item* const operator[](unsigned int n) const {	// the pointer is const?...
			RBXASSERT(indexOf(array[n]) == n);
			return array[n];
		}

		inline int size() const {
		   return array.size();
		}
	   
	};

}// namespace



