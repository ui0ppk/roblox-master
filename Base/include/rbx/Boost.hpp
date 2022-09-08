/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <algorithm>	// defines std::min and std::max before Windows.h takes over

#include "boost/config/user.hpp"
#ifndef ROBLOX_BOOST_CONFIGS
#error	// Please re-get the full boost directory
#endif
#include "boost/shared_ptr.hpp"


#include "boost/bind.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>

// for placement_any
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/noncopyable.hpp>

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::weak_ptr;

#ifdef _WIN32
//#include <windows.h>
#else 
#include "RbxFormat.h"
#include <pthread.h>
// This is a hack. Truncates a pointer.
#define GetCurrentThreadId() (static_cast<unsigned>(reinterpret_cast<long>(pthread_self())))
#define SwitchToThread() {sched_yield();}
// We may decide to use the following instead on Mac, but we would prefer the above.
//#define SwitchToThread() {struct timespec req = {0, 1}; nanosleep(&req, NULL);}
#endif

namespace RBX
{
	// TODO: Does boost have a nicer way of doing this?
	template<class T>
	void del_fun(T* t)
	{
		delete t;
	}

	bool isFinite(double value);
	bool isFinite(int value);
}

namespace rbx
{

		
	namespace implementation
	{
		class type_holder : boost::noncopyable
        {
		public: // operations
			void (*destruct)(char* dest);
			void (*construct)(const char* src, char* dest);
        };

        template<typename ValueType>
        class typed_holder : public type_holder
        {
			typed_holder()
            {
				construct = &construct_func;
				destruct = &destruct_func;
			}

        public:
			static const typed_holder* singleton()
			{
				static typed_holder<ValueType> s;
				return &s;
			}
			
			static void construct_func(const char* src, char* dest)
			{
				const ValueType* value = reinterpret_cast<const ValueType*>(src);
				ValueType* v = reinterpret_cast<ValueType*>(dest);
				new (v) ValueType(*value);
			} 

			static void destruct_func(char* dest) 
			{
				ValueType* value = reinterpret_cast<ValueType*>(dest);
				value->~ValueType();
			}
        };
	}

	// placement_any is a reworking of boost::any that embeds the value inside of
	// itself, rather than in the heap. This eliminates new/delete operations. However,
	// you must know in advance how big the values are able to be. Also, placement_any
	// allocates enough memory for the largest possible object, even when it is void.
	// SizeType must be a class that is as large as the largest sized object that
	// will be placed inside placement_any
	template<typename SizeType>
    class placement_any
    {
    public: // structors
		placement_any() 
			: holder(0)
		{
		}

		placement_any(const placement_any& other)
			: holder(0)
		{
			if (other.holder)
				(*other.holder->construct)(other.data, data);
			holder = other.holder;	// construct didn't throw, so we can assign the holder now
		}

        template<typename ValueType>
        explicit placement_any(const ValueType& value)
			: holder(0)
        {
			// If this fails, then make ValueType the new SizeType!
			BOOST_STATIC_ASSERT((sizeof(ValueType) <= sizeof(SizeType)));

			ValueType* v = reinterpret_cast<ValueType*>(data);
			new (v) ValueType(value);
			holder = implementation::typed_holder<ValueType>::singleton();	// construct didn't throw, so we can assign it now
        }

        ~placement_any()
        {
			if (holder)
				(*holder->destruct)(data);
        }

    public: // modifiers
		placement_any& swap(placement_any& rhs)
        {
			placement_any temp(*this);
			*this = rhs;
			rhs = temp;
            return *this;
        }

        template<typename ValueType>
        placement_any& operator=(const ValueType& rhs)
        {
			const implementation::typed_holder<ValueType>* s = implementation::typed_holder<ValueType>::singleton();
			if (holder == s)
			{
				// Optimization. Is this worth it?
				ValueType* dest = reinterpret_cast<ValueType*>(data);
				*dest = rhs;
			}
			else
			{
				if (holder)
				{
					(*holder->destruct)(data);
					holder = 0;
				}

				// If this fails, then make ValueType the new SizeType!
				BOOST_STATIC_ASSERT((sizeof(ValueType) <= sizeof(SizeType)));

				ValueType* v = reinterpret_cast<ValueType*>(data);
				new (v) ValueType(rhs);

				holder = s;
			}
			return *this;
        }

        placement_any& operator=(const placement_any& rhs)
        {
			if (&rhs == this)
				return *this;

			if (holder)
			{
				(*holder->destruct)(data);
				holder = 0;
			}
			if (rhs.holder)
			{
				(*rhs.holder->construct)(rhs.data, data); 
				holder = rhs.holder;
			}
			return *this;
        }

    public: // queries

        bool empty() const
        {
            return holder == 0;
        }

        const char* getData() const
        {
			return holder ? data : NULL;
        }

        char* getData()
        {
			return holder ? data : NULL;
        }

    private: // representation

		const implementation::type_holder* holder;
		char data[sizeof(SizeType)];
    };
}
