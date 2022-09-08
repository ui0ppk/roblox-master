/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "boost/array.hpp"
#include "rbx/Debug.h"

namespace RBX {

/* USAGE

*/
    template<class T, std::size_t N>
    class FixedArray 
	{
	private:
		boost::array<T, N> data;
		size_t num;

	public:
		FixedArray()
			: num(0)
		{}

		void push_back(const T& x) {
			RBXASSERT_VERY_FAST(num < N);
			data[num] = x;
			++num;
		}

		void fastRemove(size_t i) {
			RBXASSERT_VERY_FAST(i < num);
			RBXASSERT_VERY_FAST(num <= N);
			data[i] = data[num - 1];
			--num;
		}

		void replace(size_t i, const T& x) {
			RBXASSERT_VERY_FAST(i < num);
			RBXASSERT_VERY_FAST(num <= N);
			data[i] = x;
		}

		void fastClear() {
			num = 0;
		}

		T operator[](size_t i) {
			RBXASSERT_VERY_FAST(i < num);
			return data[i];
		}

		const T operator[](size_t i) const {
			RBXASSERT_VERY_FAST(i < num);
			return data[i];
		}

		size_t size() const {
		   return num;
		}
        
        size_t capacity() const
        {
            return N;
        }
	};

}// namespace



