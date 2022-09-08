#pragma once

// Do not include this file directly

#include "simd/simd_types.h"

namespace RBX
{

namespace simd
{

template< class ElemType >
RBX_SIMD_INLINE v4< ElemType >::v4( const v4& u ): v( u.v ) { }

template< class ElemType >
RBX_SIMD_INLINE v4< ElemType >::v4( const pod_t& u ): v( u ) { }

template< class ElemType >
RBX_SIMD_INLINE void v4< ElemType >::operator=( const v4& u )
{
    v = u.v;
}

template< class ElemType >
RBX_SIMD_INLINE void v4< ElemType >::operator=( const pod_t& u )
{
    v = u;
}

template< class ElemType >
RBX_SIMD_INLINE v4< ElemType >::operator pod_t() const
{
    return v;
}

}

}
