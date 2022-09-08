#pragma once

#if defined(RBX_PLATFORM_IOS) || defined(__APPLE__) || __ANDROID__
namespace rbx
{
    template<typename T>
    class atomic
    {
        T v;
    public:
        atomic(T value = 0) : v(value) {}
        
        T operator=(T v)
        {
            this->v = v;
            return v;
        }
        
        operator T() const { return v; }
        
        T compare_and_swap(T value, T comparand) {
            return __sync_val_compare_and_swap(&v, comparand, value);
        }
        
        T operator++() {
            return __sync_add_and_fetch(&v, 1);
        }
        
        T operator--() {
            return __sync_sub_and_fetch(&v, 1);
        }
        
        T operator++(int) {
            return __sync_fetch_and_add(&v, 1);
        }
        
        T operator--(int) {
            return __sync_fetch_and_sub(&v, 1);
        }
        
        T swap(T value)
        {
            return __sync_lock_test_and_set(&v, value);
        }
    };
} // namespace rbx

#elif defined(_WIN32)  // Windows

#include "rbx/debug.h"
#include "boost/detail/interlocked.hpp"
#include "boost/static_assert.hpp"
#include <cstdint>

namespace rbx
{
    template <typename T>
    class atomic
    {
        BOOST_STATIC_ASSERT(sizeof(T) == sizeof(long));
        long v;

    public:
        atomic(T value = 0) : v(value)
        {
            // http://msdn.microsoft.com/en-us/library/ms683614.aspx: The variable pointed to must be aligned on a 32-bit boundary
            RBXASSERT((((uintptr_t)(&(this->v))) & (sizeof(T) - 1)) == 0);
        }

        T operator=(T v)
        {
            this->v = v;
            return v;
        }

        operator T() const { return v; }

        T compare_and_swap(T value, T comparand) {
            return BOOST_INTERLOCKED_COMPARE_EXCHANGE(&v, value, comparand);
        }

        T operator++() {
            return BOOST_INTERLOCKED_INCREMENT(&v);
        }

        T operator--() {
            return BOOST_INTERLOCKED_DECREMENT(&v);
        }

        T operator++(int) {
            return BOOST_INTERLOCKED_INCREMENT(&v)-1;
        }

        T operator--(int) {
            return BOOST_INTERLOCKED_DECREMENT(&v)+1;
        }

        T swap(T value) {
            return BOOST_INTERLOCKED_EXCHANGE(&v, value);
        }
    };
} // namespace rbx
#else // you are using the wrong atomic
#error "not supported"
#endif
