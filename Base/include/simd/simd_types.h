#pragma once

// Include this file if you only need the SIMD types but not the functionality (which could be a large include)

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <stdint.h>

#include "simd/simd_platform.h"

namespace RBX
{

namespace simd
{

template< class ElemType >
class v4
{
public:
    typedef typename details::VectorTypeSelect< ElemType >::type fun_t;
    typedef ElemType elem_t;
    typedef v4< elem_t > v4_t;
    typedef const v4_t& arg_t;
    typedef fun_t pod_t;

    RBX_SIMD_INLINE v4( ) { }
    RBX_SIMD_INLINE v4( const v4& u );
    RBX_SIMD_INLINE v4( const pod_t& u );
    RBX_SIMD_INLINE void operator=( const v4& u );
    RBX_SIMD_INLINE void operator=( const pod_t& u );
    RBX_SIMD_INLINE operator pod_t() const;

    fun_t v;
};

// The SIMD types
typedef v4< float > v4f;
typedef v4< int32_t > v4i;
typedef v4< uint32_t > v4u;

// The SIMD pod types
typedef v4f::pod_t v4f_pod;
typedef v4i::pod_t v4i_pod;
typedef v4u::pod_t v4u_pod;

// Shortcuts for const refs
typedef const v4f& v4fArg;
typedef const v4i& v4iArg;
typedef const v4u& v4uArg;

}

}

#include "simd/simd_types.inl"
