#pragma once

// SIMD common platform independent implementation 

#include "simd/simd.h"

namespace RBX
{

namespace simd
{

namespace details
{
    RBX_SIMD_INLINE v4f inverseEstimate0Precision( )
    {
        return splat( 3e-04f );
    }

    RBX_SIMD_INLINE v4f inverseEstimate1Precision( )
    {
        return splat( 2e-07f );
    }
    
    RBX_SIMD_INLINE v4f inverseSqrtEstimate0Precision( )
    {
        return splat( 3.3e-05f );
    }

    RBX_SIMD_INLINE v4f inverseSqrtEstimate1Precision( )
    {
        return splat( 3e-07f );
    }
}

RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b, v4fArg c )
{
    return sumAcross2( a, b, c, c );
}

RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b, v4fArg c )
{
    return sumAcross3( a, b, c, c );
}

RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b, v4fArg c )
{
    return sumAcross4( a, b, c, c );
}

RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b, v4fArg c, v4fArg d )
{
    v4f a0c0a1c1 = zipLow( a, c );
    v4f b0d0b1d1 = zipLow( b, d );
    v4f a0b0c0d0, a1b1c1d1;
    zip( a0b0c0d0, a1b1c1d1, a0c0a1c1, b0d0b1d1 );
    v4f sum = a0b0c0d0 + a1b1c1d1;
    return sum;
}

RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b, v4fArg c, v4fArg d )
{
    v4f a0c0a1c1, a2c2xxxx;
    zip( a0c0a1c1, a2c2xxxx, a, c );
    v4f b0d0b1d1, b2d2xxxx;
    zip( b0d0b1d1, b2d2xxxx, b, d );
    v4f a0b0c0d0, a1b1c1d1;
    zip( a0b0c0d0, a1b1c1d1, a0c0a1c1, b0d0b1d1 );
    v4f sum = a0b0c0d0 + a1b1c1d1;
    v4f a2b2c2d2 = zipLow( a2c2xxxx, b2d2xxxx );
    sum = sum + a2b2c2d2;
    return sum;
}

RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b, v4fArg c, v4fArg d )
{
    v4f a0c0a1c1, a2c2a3c3;
    zip( a0c0a1c1, a2c2a3c3, a, c );
    v4f b0d0b1d1, b2d2b3d3;
    zip( b0d0b1d1, b2d2b3d3, b, d );
    v4f a0b0c0d0, a1b1c1d1;
    zip( a0b0c0d0, a1b1c1d1, a0c0a1c1, b0d0b1d1 );
    v4f sum = a0b0c0d0 + a1b1c1d1;
    v4f a2b2c2d2, a3b3c3d3;
    zip( a2b2c2d2, a3b3c3d3, a2c2a3c3, b2d2b3d3 );
    sum = sum + ( a2b2c2d2 + a3b3c3d3 );
    return sum;
}

template< class T >
RBX_SIMD_INLINE void transpose( T& a, T& b, T& c, T& d, const T& x, const T& y, const T& z, const T& w )
{
    T x0z0x1z1, x2z2x3z3;
    zip( x0z0x1z1, x2z2x3z3, x, z );
    T y0w0y1w1, y2w2y3w3;
    zip( y0w0y1w1, y2w2y3w3, y, w );

    zip( a, b, x0z0x1z1, y0w0y1w1 );
    zip( c, d, x2z2x3z3, y2w2y3w3 );
}

template< class T >
RBX_SIMD_INLINE void transpose4x3( T& x, T& y, T& z, const T& a, const T& b, const T& c, const T& d )
{
    T dummy;
    transpose( x, y, z, dummy, a, b, c, d );
}

template< class T >
RBX_SIMD_INLINE void transpose3x4( T& a, T& b, T& c, T& d, const T& x, const T& y, const T& z )
{
    transpose(a, b, c, d, x, y, z, z);
}

template< class T >
RBX_SIMD_INLINE void transpose4x2( T& x, T& y, const T& a, const T& b, const T& c, const T& d )
{
    T t0 = zipLow( a, c );
    T t1 = zipLow( b, d );
    zip( x, y, t0, t1 );
}

template< class T >
RBX_SIMD_INLINE void transpose2x4( T& a, T& b, T& c, T& d, const T& x, const T& y )
{
    zip( a, c, x, y );
    b = moveHighLow( a, a );
    d = moveHighLow( c, c );
}

template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b, const T& c, const T& d )
{
    T t0 = zipLow( a, c );
    T t1 = zipLow( b, d );
    return zipLow( t0, t1 );
}

template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b, const T& c )
{
    T t0 = zipLow( a, c );
    return zipLow( t0, b );
}

template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b )
{
    return zipLow( a, b );
}

}

}
