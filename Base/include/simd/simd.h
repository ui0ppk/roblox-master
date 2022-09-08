#pragma once

// Include this file only if you need the SIMD functionality (which is a large include). 
// If you only need the SIMD types (v4f, v4u, v4i), include simd_types.h.

#include "simd/simd_types.h"
#include "rbx/Debug.h"
#include "boost/static_assert.hpp"

namespace RBX
{

namespace simd
{

//
// Vector Loading
//

// Load 4 elements from memory into a vector
// The address must be aligned to 16 byte boundary
// r = {s[0],s[1],s[2],s[3]}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > load( const ScalarType* s );

// Load 3 elements from memory into a vector
// The address doesn't need to be aligned
// The 4th element doesn't need to be readable.
// Only implemented for floats
// r = {s[0],s[1],s[2],undefined}
RBX_SIMD_INLINE v4f load3( const float* s );

// Load 4 elements from memory into a vector
// The address must be aligned to 4 byte boundary only
// r = {s[0],s[1],s[2],s[3]}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadUnaligned( const ScalarType* s );

// Load a single element from memory and duplicate it into all components
// r = {s[0],s[0],s[0],s[0]}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadSplat( const ScalarType* s );

// Load a single element from memory and place it in the first component
// r = {s[0],*,*,*}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadSingle( const ScalarType* s );

//
// Storing
//

// Store the 4 components of the vector into memory
// The destination address must be 16 bytes aligned
// dst[i] = r[i] for i = 0,1,2,3
template< class ScalarType >
RBX_SIMD_INLINE void store( ScalarType* dst, const v4< ScalarType >& v );

// Store the 4 components of the vector into memory
// The destination address must be 4 bytes aligned only
// dst[i] = r[i] for i = 0,1,2,3
template< class ScalarType >
RBX_SIMD_INLINE void storeUnaligned( ScalarType* dst, const v4< ScalarType >& v );

// Store the single x components of the vector into memory
// The destination address must be 4 bytes aligned only
// *dst = r[0]
template< class ScalarType >
RBX_SIMD_INLINE void storeSingle( ScalarType* dst, const v4< ScalarType >& v );

//
// Vector Forming
//

// Form a vector of floating point 0s
// r = {0.0f,0.0f,0.0f,0.0f}
RBX_SIMD_INLINE v4f zerof();

// Duplicate the scalar into all 4 components
// r = {s,s,s,s}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > splat( ScalarType s );

// Form a vector out of 4 scalars
// r = {x,y,z,w}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z, ScalarType w );

// Form a vector out of 3 scalars
// r = {x,y,z,undetermined}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z );

// Form a vector out of 2 scalars
// r = {x,y,undetermined,undetermined}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y );

//
// Insert / extract
//

// Extract the i'th component of the vector
// This a very slow operation
// r = v[i]
template< class VectorType >
RBX_SIMD_INLINE typename VectorType::elem_t extractSlow( const VectorType& v, uint32_t i );

//
// Casts: cast one type to the other preserving the bit representation
//

// Cast from float to unsigned
// r = 'reinterpret_cast< v4u >'( v )
RBX_SIMD_INLINE v4u reinterpretAsUInt( v4fArg v );

// Cast from float to int
// r = 'reinterpret_cast< v4u >'( v )
RBX_SIMD_INLINE v4i reinterpretAsInt( v4fArg v );

// Cast from unsigned to float
// r = 'reinterpret_cast< v4f >'( v )
RBX_SIMD_INLINE v4f reinterpretAsFloat( v4uArg v );

// Cast from int to float
// r = 'reinterpret_cast< v4f >'( v )
RBX_SIMD_INLINE v4f reinterpretAsFloat( v4iArg v );

// Cast from unsigned to int
// r = 'reinterpret_cast< v4i >'( v )
RBX_SIMD_INLINE v4i reinterpretAsInt( v4uArg v );

// Cast from int to unsigned
// r = 'reinterpret_cast< v4u >'( v )
RBX_SIMD_INLINE v4u reinterpretAsUInt( v4iArg v );

//
// Conversions
//

// Convert float to int rounding to the nearest
// r[i] = int( roundToNearest( v[i] ) )
RBX_SIMD_INLINE v4i convertFloat2IntNearest( v4fArg v );

// Convert float to int truncating towards 0
// r[i] = int( truncate( v[i] ) )
RBX_SIMD_INLINE v4i convertFloat2IntTruncate( v4fArg v );

// Convert int to float
// r[i] = float( v[i] )
RBX_SIMD_INLINE v4f convertIntToFloat( v4iArg v );

//
// Selects
//

// Returns a select mask:
// r[0] = ( a == 1 ) ? 0xffffffff : 0
// r[1] = ( b == 1 ) ? 0xffffffff : 0
// r[2] = ( c == 1 ) ? 0xffffffff : 0
// r[3] = ( d == 1 ) ? 0xffffffff : 0
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d >
RBX_SIMD_INLINE v4u selectMask();

// Select elements between a and b
// r[i] = ( mask[i] == 0 ) ? a[i] : ( mask[i] == 0xffffffff ) ? b[i] : undefined
template< class VectorType >
RBX_SIMD_INLINE VectorType select( const VectorType& a, const VectorType& b, const v4u& mask );

// r[0] = ( a == 1 ) ? u[0] : v[0]
// r[1] = ( b == 1 ) ? u[1] : v[1]
// r[2] = ( c == 1 ) ? u[2] : v[2]
// r[3] = ( d == 1 ) ? u[3] : v[3]
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType select( const VectorType& u, const VectorType& v );

// Replace an element with an element of the second vector
// r = a
// r[index] = b[index]
template< uint32_t index, class VectorType >
RBX_SIMD_INLINE VectorType replace( const VectorType& a, const VectorType& b );

//
// Permutes
//

// r = { a[0], b[0], a[1], b[1] }
template< class VectorType >
RBX_SIMD_INLINE VectorType zipLow( const VectorType& u, const VectorType& v );

// r = { a[2], b[2], a[3], b[3] }
template< class VectorType >
RBX_SIMD_INLINE VectorType zipHigh( const VectorType& u, const VectorType& v );

// r0 = { a[0], b[0], a[1], b[1] }
// r1 = { a[2], b[2], a[3], b[3] }
template< class VectorType >
RBX_SIMD_INLINE void zip( VectorType& r0, VectorType& r1, const VectorType& u, const VectorType& v );

// r = { v[2], v[3], u[2], u[3] }
template< class VectorType >
RBX_SIMD_INLINE VectorType moveHighLow( const VectorType& u, const VectorType& v );

// r = { u[0], u[1], v[0], v[1] }
template< class VectorType >
RBX_SIMD_INLINE VectorType moveLowHigh( const VectorType& u, const VectorType& v );

// Shuffle two vectors
// r = { u[a], u[b], v[c], v[d] }
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType shuffle( const VectorType& u, const VectorType& v );

// Duplicate single component of a vector into all components of the result
// r = {v[i],v[i],v[i],v[i]}
template< unsigned i, class VectorType >
RBX_SIMD_INLINE VectorType splat( const VectorType& v );

// Permute the components of the vector with a compile time permutation 
// r = {v[a],v[b],v[c],v[d]}
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType permute( const VectorType& v );

// Rotate the vector left by /offset/ elements
// r = {v[(offset%4)], v[((offset+1)%4)], v[((offset+2)%4)], v[((offset+3)%4)]}
template< uint32_t offset, class VectorType >
RBX_SIMD_INLINE VectorType rotateLeft( VectorType& v );

//
// Compares
//

// Return a mask resulting from the compare
// r[i] = a[i] > b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compareGreater( v4fArg a, v4fArg b );

// r[i] = a[i] > b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator>( v4fArg a, v4fArg b );

// r[i] = a[i] >= b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compareGreaterEqual( v4fArg a, v4fArg b );

// r[i] = a[i] >= b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator>=( v4fArg a, v4fArg b );

// r[i] = a[i] < b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compareLess( v4fArg a, v4fArg b );

// r[i] = a[i] < b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator<( v4fArg a, v4fArg b );

// r[i] = a[i] <= b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compareLessEqual( v4fArg a, v4fArg b );

// r[i] = a[i] <= b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator<=( v4fArg a, v4fArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compare( v4fArg a, v4fArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator==( v4fArg a, v4fArg b );

// r[i] = a[i] != b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator!=( v4fArg a, v4fArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compare( v4iArg a, v4iArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator==( v4iArg a, v4iArg b );

// r[i] = a[i] != b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator!=( v4iArg a, v4iArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u compare( v4uArg a, v4uArg b );

// r[i] = a[i] == b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator==( v4uArg a, v4uArg b );

// r[i] = a[i] != b[i] ? 0xffffffff : 0
RBX_SIMD_INLINE v4u operator!=( v4uArg a, v4uArg b );

//
// Set element to 0
//

// r = v; r[a] = 0;
template< uint32_t a >
RBX_SIMD_INLINE v4f replaceWithZero( v4fArg v );

// r = v; r[a] = 0;
template< uint32_t a >
RBX_SIMD_INLINE v4i replaceWithZero( v4iArg v );

// r = v; r[a] = 0;
template< uint32_t a >
RBX_SIMD_INLINE v4u replaceWithZero( v4uArg v );

//
// Float Arithmetics
//

// r[i] = a[i] + b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f operator+( v4fArg a, v4fArg b );

// a[i] = a[i] + b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f& operator+=( v4f& a, v4fArg b );

// r[i] = a[i] - b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f operator-( v4fArg a, v4fArg b );

// a[i] = a[i] - b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f& operator-=( v4f& a, v4fArg b );

// r[i] = a[i] * b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f operator*( v4fArg a, v4fArg b );

// a[i] = a[i] * b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f& operator*=( v4f& a, v4fArg b );

// r[i] = a[i] + b[i] * c[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f mulAdd( v4fArg a, v4fArg b, v4fArg c );

// r[i] = -a[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f operator-( v4fArg a );

// r[i] = a[i] / b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f operator/( v4fArg a, v4fArg b );

// a[i] = a[i] / b[i], i = 0,1,2,3
RBX_SIMD_INLINE v4f& operator/=( v4f& a, v4fArg b );

// r[i] = fabsf( a[i] ), i = 0,1,2,3
RBX_SIMD_INLINE v4f abs( v4fArg a );

// r[i] = max(a[i], b[i])
RBX_SIMD_INLINE v4f max( v4fArg a, v4fArg b );

// r[i] = min(a[i], b[i])
RBX_SIMD_INLINE v4f min( v4fArg a, v4fArg b );

//
// Estimates
//

// Estimate of the inverse
// About 12 bits of precision
RBX_SIMD_INLINE v4f inverseEstimate0( v4fArg a );

// This is like the normal version except for small inputs ( abs(a) < smallestInvertible() ) and large inputs
// ( abs(a) > largestInvertible() ) are not handled and will return an undefined number
RBX_SIMD_INLINE v4f inverseEstimate0Fast( v4fArg a );

// More precise version of the inverse estimate
RBX_SIMD_INLINE v4f inverseEstimate1( v4fArg a );

// This is like the normal version except for small inputs ( abs(a) < smallestInvertible() ) and large inputs
// ( abs(a) > largestInvertible() ) are not handled and will return an undefined number
RBX_SIMD_INLINE v4f inverseEstimate1Fast( v4fArg a );

// Returns the smallest floating point number r such that inverseEstimate1(r) < inf
// Before inverting, you can create a select mask: sel = r < smallestInvertible()
RBX_SIMD_INLINE v4f smallestInvertible();

// Returns the largest floating point number r such that inverseEstimate1(r) > 0.0f
// For inputs that are larger than this, the inverse will return 0.0f
RBX_SIMD_INLINE v4f largestInvertible( );

// Return an estimate of the inverse square root.
// About 12 bits of precision
RBX_SIMD_INLINE v4f inverseSqrtEstimate0( v4fArg a );

// This is like the normal version except for small inputs ( abs(a) < smallestSqrtInvertible() ) and large inputs
// ( abs(a) > largestSqrtInvertible() ) are not handled and will return an undefined number
RBX_SIMD_INLINE v4f inverseSqrtEstimate0Fast( v4fArg a );

// Return an estimate of the inverse square root.
// More than 12 bits of precision
RBX_SIMD_INLINE v4f inverseSqrtEstimate1( v4fArg a );

// This is like the normal version except for small inputs ( abs(a) < smallestSqrtInvertible() ) and large inputs
// ( abs(a) > largestSqrtInvertible() ) are not handled and will return an undefined number
RBX_SIMD_INLINE v4f inverseSqrtEstimate1Fast( v4fArg a );

RBX_SIMD_INLINE v4f smallestSqrtInvertible( );

RBX_SIMD_INLINE v4f largestSqrtInvertible( );

//
// Inter-element arithmetic ops
//

// r[i] = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3], i = 0,1,2,3
RBX_SIMD_INLINE v4f dotProduct( v4fArg a, v4fArg b );

// r[i] = a[0]*b[0] + a[1]*b[1] + a[2]*b[2], i = 0,1,2,3
RBX_SIMD_INLINE v4f dotProduct3( v4fArg a, v4fArg b );

// r[i] = a[0] + a[1] + a[2] + a[3], i = 0,1,2,3
RBX_SIMD_INLINE v4f sumAcross( v4fArg a );

// r[0] = a[0] + a[1]
// r[1] = b[0] + b[1]
// r[2] = undefined
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b );

// r[0] = a[0] + a[1] + a[2]
// r[1] = b[0] + b[1] + b[2]
// r[2] = undefined
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b );

// r[0] = a[0] + a[1] + a[2] + a[3]
// r[1] = b[0] + b[1] + b[2] + b[3]
// r[2] = undefined
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b );

// r[0] = a[0] + a[1]
// r[1] = b[0] + b[1]
// r[2] = c[0] + c[1]
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b, v4fArg c );

// r[0] = a[0] + a[1] + a[2]
// r[1] = b[0] + b[1] + b[2]
// r[3] = c[0] + c[1] + c[2]
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b, v4fArg c );

// r[0] = a[0] + a[1] + a[2] + a[3]
// r[1] = b[0] + b[1] + b[2] + b[3]
// r[3] = c[0] + c[1] + c[2] + c[3]
// r[3] = undefined
RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b, v4fArg c );

// r[0] = a[0] + a[1]
// r[1] = b[0] + b[1]
// r[3] = c[0] + c[1]
// r[3] = d[0] + d[1]
RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b, v4fArg c, v4fArg d );

// r[0] = a[0] + a[1] + a[2]
// r[1] = b[0] + b[1] + b[2]
// r[3] = c[0] + c[1] + c[2]
// r[3] = d[0] + d[1] + d[2]
RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b, v4fArg c, v4fArg d );

// r[0] = a[0] + a[1] + a[2] + a[3]
// r[1] = b[0] + b[1] + b[2] + b[3]
// r[3] = c[0] + c[1] + c[2] + c[3]
// r[3] = d[0] + d[1] + d[2] + d[3]
RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b, v4fArg c, v4fArg d );

//
// Packing / unpacking
//

// Pack 4 vector-3's into 3 vector-4's
// p[0] = {a[0],a[1],a[2],b[0]}
// p[1] = {b[1],b[2],c[0],c[1]}
// p[2] = {c[2],d[0],d[1],d[2]}
template< class T >
RBX_SIMD_INLINE void pack3( typename T::pod_t* __restrict p, const T& a, const T& b, const T& c, const T& d );

// Pack 3 vector-4's into 4 vector-3's
// a = {p[0][0],p[0][1],p[0][2],*}
// b = {p[0][3],p[1][0],p[1][1],*}
// c = {p[1][2],p[1][3],p[2][0],*}
// d = {p[2][1],p[2][2],p[2][3],*}
template< class T >
RBX_SIMD_INLINE void unpack3( T& a, T& b, T& c, T& d, const typename T::pod_t* p );

//
// Matrix permutations
//

// Gather the X components of a,b,c,d:
// r = {a[0],b[0],c[0],d[0]}
template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b, const T& c, const T& d );

// Gather the X components of a,b,c:
// r = {a[0],b[0],c[0],undetermined}
template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b, const T& c );

// Gather the X components of a,b:
// r = {a[0],b[0],undetermined,undetermined}
template< class T >
RBX_SIMD_INLINE T gatherX( const T& a, const T& b );

// Transpose a 4x4 matrix
// a = {x[0],y[0],z[0],w[0]}
// b = {x[1],y[1],z[1],w[1]}
// c = {x[2],y[2],z[2],w[2]}
// d = {x[3],y[3],z[3],w[3]}
template< class T >
RBX_SIMD_INLINE void transpose( T& a, T& b, T& c, T& d, const T& x, const T& y, const T& z, const T& w );

// Transpose a 3x4 matrix
// a = {x[0],y[0],z[0],undetermined}
// b = {x[1],y[1],z[1],undetermined}
// c = {x[2],y[2],z[2],undetermined}
// d = {x[3],y[3],z[3],undetermined}
template< class T >
RBX_SIMD_INLINE void transpose3x4( T& a, T& b, T& c, T& d, const T& x, const T& y, const T& z );

// Transpose a 4x3 matrix
// x = {a[0],b[0],c[0],d[0]}
// y = {a[1],b[1],c[1],d[1]}
// z = {a[2],b[2],c[2],d[2]}
template< class T >
RBX_SIMD_INLINE void transpose4x3( T& x, T& y, T& z, const T& a, const T& b, const T& c, const T& d );

// Transpose a 2x4 matrix
// a = {x[0],y[0],undetermined,undetermined}
// b = {x[1],y[1],undetermined,undetermined}
// c = {x[2],y[2],undetermined,undetermined}
// d = {x[3],y[3],undetermined,undetermined}
template< class T >
RBX_SIMD_INLINE void transpose2x4( T& a, T& b, T& c, T& d, const T& x, const T& y );

// Transpose a 2x4 matrix
// x = {a[0],b[0],c[0],d[0]}
// y = {a[1],b[1],c[1],d[1]}
template< class T >
RBX_SIMD_INLINE void transpose4x2( T& x, T& y, const T& a, const T& b, const T& c, const T& d );

}

}

#if defined( RBX_SIMD_USE_SSE )
#include "simd/simd_sse.inl"
#elif defined( RBX_SIMD_USE_NEON )
#include "simd/simd_neon.inl"
#endif

#include "simd/simd_common.inl"
