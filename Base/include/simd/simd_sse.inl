#pragma once

// Do not include this file directly. Include simd/simd.h

#include "simd/simd_types.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <mmintrin.h>

namespace RBX
{

namespace simd
{

//
// Vector Loading
//
namespace details
{
    RBX_SIMD_INLINE v4f::pod_t load( const float* s )
    {
        return _mm_load_ps( s );
    }

    RBX_SIMD_INLINE v4i::pod_t load( const int32_t* s )
    {
        return _mm_load_si128( reinterpret_cast< const __m128i* >( s ) );
    }

    RBX_SIMD_INLINE v4u::pod_t load( const uint32_t* s )
    {
        return _mm_load_si128( reinterpret_cast< const __m128i* >( s ) );
    }

    RBX_SIMD_INLINE v4f::pod_t loadUnaligned( const float* s )
    {
        return _mm_loadu_ps( s );
    }

    RBX_SIMD_INLINE v4i::pod_t loadUnaligned( const int32_t* s )
    {
        return _mm_loadu_si128( reinterpret_cast< const __m128i* >( s ) );
    }

    RBX_SIMD_INLINE v4u::pod_t loadUnaligned( const uint32_t* s )
    {
        return _mm_loadu_si128( reinterpret_cast< const __m128i* >( s ) );
    }

    RBX_SIMD_INLINE v4f::pod_t loadSingle( const float* s )
    {
        return _mm_load_ss( s );
    }

    RBX_SIMD_INLINE v4i::pod_t loadSingle( const int32_t* s )
    {
        return _mm_castps_si128( _mm_load_ss( reinterpret_cast< const float* >( s ) ) );
    }

    RBX_SIMD_INLINE v4u::pod_t loadSingle( const uint32_t* s )
    {
        return _mm_castps_si128( _mm_load_ss( reinterpret_cast< const float* >( s ) ) );
    }

    RBX_SIMD_INLINE v4f::pod_t loadSplat( const float* s )
    {
        return _mm_load_ps1( s );
    }

    RBX_SIMD_INLINE v4i::pod_t loadSplat( const int32_t* s )
    {
        return _mm_castps_si128( _mm_load_ps1( reinterpret_cast< const float* >( s ) ) );
    }

    RBX_SIMD_INLINE v4u::pod_t loadSplat( const uint32_t* s )
    {
        return _mm_castps_si128( _mm_load_ps1( reinterpret_cast< const float* >( s ) ) );
    }
}
template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > load( const ScalarType* s )
{
    RBX_SIMD_ALIGN_ASSERT( s, 16 );
    v4< ScalarType > r;
    r.v = details::load( s );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadUnaligned( const ScalarType* s )
{
    RBX_SIMD_ALIGN_ASSERT( s, 16 );
    v4< ScalarType > r;
    r.v = details::loadUnaligned( s );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadSingle( const ScalarType* s )
{
    v4< ScalarType > r;
    r.v = details::loadSingle( s );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadSplat( const ScalarType* s )
{
    v4< ScalarType > r;
    r.v = details::loadSplat( s );
    return r;
}

RBX_SIMD_INLINE v4f load3( const float* value )
{
    __m128 x = _mm_load_ss(value);
    __m128 y = _mm_load_ss(value + 1);
    __m128 z = _mm_load_ss(value + 2);
    __m128 xz = _mm_unpacklo_ps(x, z);
    v4f r;
    r.v = _mm_unpacklo_ps( xz, y );
    return r;
}

//
// Storing
//
namespace details
{
    RBX_SIMD_INLINE void store( float* dst, __m128 v )
    {
        _mm_store_ps( dst, v );
    }

    RBX_SIMD_INLINE void store( int32_t* dst, __m128i v )
    {
        _mm_store_si128( reinterpret_cast< __m128i* >( dst ), v );
    }

    RBX_SIMD_INLINE void store( uint32_t* dst, __m128i v )
    {
        _mm_store_si128( reinterpret_cast< __m128i* >( dst ), v );
    }

    RBX_SIMD_INLINE void storeUnaligned( float* dst, __m128 v )
    {
        _mm_storeu_ps( dst, v );
    }

    RBX_SIMD_INLINE void storeUnaligned( int32_t* dst, __m128i v )
    {
        _mm_storeu_si128( reinterpret_cast< __m128i* >( dst ), v );
    }

    RBX_SIMD_INLINE void storeUnaligned( uint32_t* dst, __m128i v )
    {
        _mm_storeu_si128( reinterpret_cast< __m128i* >( dst ), v );
    }

    RBX_SIMD_INLINE void storeSingle( float* dst, __m128 v )
    {
        _mm_store_ss( dst, v );
    }

    RBX_SIMD_INLINE void storeSingle( int32_t* dst, __m128i v )
    {
        _mm_store_ss( reinterpret_cast< float* >( dst ), _mm_castsi128_ps( v ) );
    }

    RBX_SIMD_INLINE void storeSingle( uint32_t* dst, __m128i v )
    {
        _mm_store_ss( reinterpret_cast< float* >( dst ), _mm_castsi128_ps( v ) );
    }
}

template< class ScalarType >
RBX_SIMD_INLINE void store( ScalarType* dst, const v4< ScalarType >& v )
{
    RBX_SIMD_ALIGN_ASSERT( dst, 16 );
    details::store( dst, v.v );
}

template< class ScalarType >
RBX_SIMD_INLINE void storeUnaligned( ScalarType* dst, const v4< ScalarType >& v )
{
    details::storeUnaligned( dst, v.v );
}

template< class ScalarType >
RBX_SIMD_INLINE void storeSingle( ScalarType* dst, const v4< ScalarType >& v )
{
    details::storeSingle( dst, v.v );
}

//
// Vector Forming
//
RBX_SIMD_INLINE v4f zerof()
{
    v4f r;
    r.v = _mm_setzero_ps();
    return r;
}

namespace details
{
    RBX_SIMD_INLINE __m128 splat( float s )
    {
        return _mm_set_ps1( s );
    }

    RBX_SIMD_INLINE __m128i splat( int32_t s )
    {
        return _mm_set1_epi32( int(s) );
    }

    RBX_SIMD_INLINE __m128i splat( uint32_t s )
    {
        return _mm_set1_epi32( int(s) );
    }
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > splat( ScalarType s )
{
    v4< ScalarType > r;
    r.v = details::splat( s );
    return r;
}

namespace details
{
    RBX_SIMD_INLINE __m128 form( float x, float y, float z, float w )
    {
        return _mm_set_ps(w, z, y, x);
    }

    RBX_SIMD_INLINE __m128i form( int32_t x, int32_t y, int32_t z, int32_t w )
    {
        return _mm_set_epi32(w, z, y, x);
    }

    RBX_SIMD_INLINE __m128i form( uint32_t x, uint32_t y, uint32_t z, uint32_t w )
    {
        return _mm_set_epi32(w, z, y, x);
    }

    RBX_SIMD_INLINE __m128 setSingle( float x )
    {
        return _mm_set_ss(x);
    }

    RBX_SIMD_INLINE __m128i setSingle( int32_t x )
    {
        return _mm_set_epi32(0,0,0,x);
    }

    RBX_SIMD_INLINE __m128i setSingle( uint32_t x )
    {
        return _mm_set_epi32(0,0,0,x);
    }
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z, ScalarType w )
{
    v4< ScalarType > r;
    r.v = details::form(x,y,z,w);
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z )
{
    v4< ScalarType > r;
    r.v = zipLow( zipLow( v4< ScalarType >( details::setSingle(x) ), v4< ScalarType >( details::setSingle(z) ) ), v4< ScalarType >( details::setSingle(y) ) );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y )
{
    v4< ScalarType > r;
    r.v = zipLow( v4< ScalarType >( details::setSingle(x) ), v4< ScalarType >( details::setSingle(y) ) );
    return r;
}

//
// Insert / extract
//
template< class VectorType >
RBX_SIMD_INLINE typename VectorType::elem_t extractSlow( const VectorType& _v, uint32_t _i )
{
    union
    {
        typename VectorType::pod_t v;
        typename VectorType::elem_t el[ 4 ];
    };

    v = _v.v;
    return el[ _i ];
}

//
// Casts: cast one type to the other preserving the bit representation
//
RBX_SIMD_INLINE v4i reinterpretAsInt( v4fArg v )
{
    v4i r;
    r.v = _mm_castps_si128( v.v );
    return r;
}

RBX_SIMD_INLINE v4u reinterpretAsUInt( v4fArg v )
{
    v4u r;
    r.v = _mm_castps_si128( v.v );
    return r;
}

RBX_SIMD_INLINE v4f reinterpretAsFloat( v4iArg v )
{
    v4f r;
    r.v = _mm_castsi128_ps( v.v );
    return r;
}

RBX_SIMD_INLINE v4f reinterpretAsFloat( v4uArg v )
{
    v4f r;
    r.v = _mm_castsi128_ps( v.v );
    return r;
}

RBX_SIMD_INLINE v4u reinterpretAsUInt( v4iArg v )
{
    v4u r;
    r.v = v.v;
    return r;
}

RBX_SIMD_INLINE v4i reinterpretAsInt( v4uArg v )
{
    v4i r;
    r.v = v.v;
    return r;
}

//
// Conversions: convert float to int and int to float
//
RBX_SIMD_INLINE v4i convertFloat2IntNearest( v4fArg v )
{
    v4i r;
    r.v = _mm_cvtps_epi32( v.v );
    return r;
}

RBX_SIMD_INLINE v4i convertFloat2IntTruncate( v4fArg v )
{
    v4i r;
    r.v = _mm_cvttps_epi32( v.v );
    return r;
}

RBX_SIMD_INLINE v4f convertIntToFloat( v4iArg v )
{
    v4f r;
    r.v = _mm_cvtepi32_ps( v.v );
    return r;
}

//
// Splat
//
#define RBX_SIMD_SHUFFLE( a, b, c, d ) _MM_SHUFFLE( (d) & 0x3, (c) & 0x3, (b) & 0x3, (a) & 0x3 )

namespace details
{
    template< int a, int b, int c, int d >
    RBX_SIMD_INLINE __m128 shuffle( __m128 u )
    {
        return _mm_shuffle_ps( u, u, RBX_SIMD_SHUFFLE(a,b,c,d) );
    }

    template< int a, int b, int c, int d >
    RBX_SIMD_INLINE __m128i shuffle( __m128i u )
    {
        return _mm_shuffle_epi32( u, RBX_SIMD_SHUFFLE(a,b,c,d) );
    }

    template< int a, int b, int c, int d >
    RBX_SIMD_INLINE __m128 shuffle( __m128 u, __m128 v )
    {
        return _mm_shuffle_ps( u, v, RBX_SIMD_SHUFFLE(a,b,c,d) );
    }

    template< int a, int b, int c, int d >
    RBX_SIMD_INLINE __m128i shuffle( __m128i u, __m128i v )
    {
        __m128 uf = _mm_castsi128_ps( u );
        __m128 vf = _mm_castsi128_ps( v );
        return _mm_castps_si128( shuffle<a,b,c,d>( uf, vf ) );
    }
}

template< unsigned i, class VectorType >
RBX_SIMD_INLINE VectorType splat( const VectorType& v )
{
    VectorType r;
    r.v = details::shuffle< i & 0x3, i & 0x3, i & 0x3, i & 0x3 >( v.v );
    return r;
}

//
// Selects
//
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d >
RBX_SIMD_INLINE v4u selectMask()
{
    v4u r;
    r.v = _mm_set_epi32( -(boost::int32_t)( d & 0x1 ), -(boost::int32_t)( c & 0x1 ), -(boost::int32_t)( b & 0x1 ), -(boost::int32_t)( a & 0x1 ) );
    return r;
}

template<>
RBX_SIMD_INLINE v4f select( const v4f& a, const v4f& b, const v4u& mask )
{
    __m128 diff = _mm_xor_ps( a.v, b.v );
    __m128 mask_ps = _mm_castsi128_ps( mask.v );
    __m128 maskedDiff = _mm_and_ps( mask_ps, diff );
    v4f r;
    r.v = _mm_xor_ps( a.v, maskedDiff );
    return r;
}

template<>
RBX_SIMD_INLINE v4u select( const v4u& a, const v4u& b, const v4u& mask )
{
    __m128i aOnly = _mm_andnot_si128( mask.v, a.v );
    __m128i bOnly = _mm_and_si128( mask.v, b.v );
    v4u r;
    r.v = _mm_or_si128( aOnly, bOnly );
    return r;
}

template<>
RBX_SIMD_INLINE v4i select( const v4i& a, const v4i& b, const v4u& mask )
{
    __m128i aOnly = _mm_andnot_si128( mask.v, a.v );
    __m128i bOnly = _mm_and_si128( mask.v, b.v );
    v4i r;
    r.v = _mm_or_si128( aOnly, bOnly );
    return r;
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType shuffle( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::shuffle<a,b,c,d>( u.v, v.v );
    return r;
}

namespace details
{
    RBX_SIMD_INLINE __m128 unpackLow( __m128 u, __m128 v )
    {
        return _mm_unpacklo_ps( u, v );
    }

    RBX_SIMD_INLINE __m128i unpackLow( __m128i u, __m128i v )
    {
        return _mm_unpacklo_epi32( u, v );
    }

    RBX_SIMD_INLINE __m128 unpackHigh( __m128 u, __m128 v )
    {
        return _mm_unpackhi_ps( u, v );
    }

    RBX_SIMD_INLINE __m128i unpackHigh( __m128i u, __m128i v )
    {
        return _mm_unpackhi_epi32( u, v );
    }
}

template< class VectorType >
RBX_SIMD_INLINE VectorType zipLow( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::unpackLow( u.v, v.v );
    return r;
}

template< class VectorType >
RBX_SIMD_INLINE VectorType zipHigh( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::unpackHigh( u.v, v.v );
    return r;
}

template< class VectorType >
RBX_SIMD_INLINE void zip( VectorType& r0, VectorType& r1, const VectorType& u, const VectorType& v )
{
    r0.v = details::unpackLow( u.v, v.v );
    r1.v = details::unpackHigh( u.v, v.v );
}

namespace details
{
    RBX_SIMD_INLINE __m128 moveHighLow( __m128 u, __m128 v )
    {
        return _mm_movehl_ps( u, v );
    }

    RBX_SIMD_INLINE __m128i moveHighLow( __m128i u, __m128i v )
    {
        return _mm_castps_si128( _mm_movehl_ps( _mm_castsi128_ps( u ), _mm_castsi128_ps( v ) ) );;
    }

    RBX_SIMD_INLINE __m128 moveLowHigh( __m128 u, __m128 v )
    {
        return _mm_movelh_ps( u, v );
    }

    RBX_SIMD_INLINE __m128i moveLowHigh( __m128i u, __m128i v )
    {
        return _mm_castps_si128( _mm_movelh_ps( _mm_castsi128_ps( u ), _mm_castsi128_ps( v ) ) );;
    }
}

template< class VectorType >
RBX_SIMD_INLINE VectorType moveHighLow( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::moveHighLow( u.v, v.v );
    return r;
}

template< class VectorType >
RBX_SIMD_INLINE VectorType moveLowHigh( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::moveLowHigh( u.v, v.v );
    return r;
}

namespace details
{
    RBX_SIMD_INLINE __m128 replaceFirst( __m128 a, __m128 b )
    {
        return _mm_move_ss( a, b );
    }

    RBX_SIMD_INLINE __m128i replaceFirst( __m128i a, __m128i b )
    {
        return _mm_castps_si128( _mm_move_ss( _mm_castsi128_ps( a ), _mm_castsi128_ps( b ) ) );
    }

    template< class VectorType >
    RBX_SIMD_INLINE VectorType replaceFirst( const VectorType& a, const VectorType& b )
    {
        VectorType r;
        r.v = details::replaceFirst( a.v, b.v );
        return r;
    }
}

#ifdef RBX_SSE3
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d >
RBX_SIMD_INLINE v4f blend( v4fArg u, v4fArg v )
{
    v4f r;
    r.v = _mm_blend_ps( u, v, ( ( d << 3 ) | ( c << 2 ) | ( b << 1 ) | ( a ) ) );
    return r;
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d >
RBX_SIMD_INLINE v4i blend( v4iArg u, v4iArg v )
{
    v4i r;
    r.v = _mm_castps_si128( _mm_blend_ps( _mm_castsi128_ps( u.v ), _mm_castsi128_ps( v.v ), ( d << 3 ) | ( c << 2 ) | ( b << 1 ) | ( a ) ) );
    return r;
}
#endif

#undef RBX_SIMD_SHUFFLE

namespace details
{
    template< class T, uint32_t a, uint32_t b, uint32_t c, uint32_t d >
    struct SelectHelper
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v );
    };

    template< class T >
    struct SelectHelper< T, 0,0,0,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return u;
        }
    };

    template< class T >
    struct SelectHelper< T, 1,1,1,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return v;
        }
    };

    template< class T >
    struct SelectHelper< T, 1,0,0,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return details::replaceFirst( u, v );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,1,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return details::replaceFirst( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,0,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            T u0v1v2v3 = details::replaceFirst( v, u );
            return shuffle< 0, 1, 2, 3 >( u0v1v2v3, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,0,1,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 0, 1, 0, 0 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,0,1,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            T t = moveHighLow( u, v );
            return shuffle< 0, 1, 0, 3 >( u, t );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,1,0,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 0, 0, 1, 0 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,0,0,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            T t = moveHighLow( u, v );
            return shuffle< 0, 1, 2, 1 >( u, t );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,1,1,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 0, 0, 0, 1 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,0,1,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return shuffle< 0, 1, 2, 3 >( u, v );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,1,0,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 0, 0, 1, 1 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,0,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            T t = shuffle< 0, 2, 1, 3 >( u, v );
            return shuffle< 0, 2, 1, 3 >( t, t );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,0,1,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 0, 1, 0, 1 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 1,0,0,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            T t = shuffle< 1, 2, 0, 3 >( u, v );
            return shuffle< 2, 0, 1, 3 >( t, t );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,1,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper<T, 1, 0, 0, 1 >::select( v, u );
        }
    };
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType select( const VectorType& u, const VectorType& v )
{
    return details::SelectHelper< VectorType, a, b, c, d >::select( u, v );
}

template< uint32_t index, class VectorType >
RBX_SIMD_INLINE VectorType replace( const VectorType& a, const VectorType& b )
{
    BOOST_STATIC_ASSERT_MSG( ( index & 0x3 ) == index, "index must be between 0 and 3" );
    return select< index == 0, index == 1, index == 2, index == 3 >( a, b );
}

//
// Permutes
//
template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType permute( const VectorType& u )
{
    return shuffle< a, b, c, d >( u, u );
}

template< uint32_t offset, class VectorType >
RBX_SIMD_INLINE VectorType rotateLeft( const VectorType& v )
{
    return shuffle< offset % 4, ( offset + 1 ) % 4, ( offset + 2 ) % 4, ( offset + 3 ) % 4 >( v, v );
}

//
// Compares
//
RBX_SIMD_INLINE v4u compareGreater( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpgt_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator>( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpgt_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compareGreaterEqual( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpge_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator>=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpge_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compareLess( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmplt_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator<( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmplt_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compareLessEqual( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmple_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator<=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmple_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpeq_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpeq_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = _mm_castps_si128( _mm_cmpneq_ps( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = _mm_cmpeq_epi32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = _mm_cmpeq_epi32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = _mm_andnot_si128( splat(0xffffffff).v, _mm_cmpeq_epi32( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = _mm_cmpeq_epi32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = _mm_cmpeq_epi32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = _mm_andnot_si128( splat(0xffffffff).v, _mm_cmpeq_epi32( a.v, b.v ) );
    return r;
}

//
// Set
//
template< uint32_t a >
RBX_SIMD_INLINE v4f replaceWithZero( v4fArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    __m128i mask = _mm_set_epi32( a == 3 ? ~0 : 0, a == 2 ? ~0 : 0, a == 1 ? ~0 : 0, a == 0 ? ~0 : 0 );
    v4f r;
    r.v = _mm_andnot_ps( _mm_castsi128_ps( mask ), v.v );
    return r;
}

template< uint32_t a >
RBX_SIMD_INLINE v4i replaceWithZero( v4iArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    __m128i mask = _mm_set_epi32( a == 3 ? ~0 : 0, a == 2 ? ~0 : 0, a == 1 ? ~0 : 0, a == 0 ? ~0 : 0 );
    v4i r;
    r.v = _mm_andnot_si128( mask, v.v );
    return r;
}

template< uint32_t a >
RBX_SIMD_INLINE v4u replaceWithZero( v4uArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    __m128i mask = _mm_set_epi32( a == 3 ? ~0 : 0, a == 2 ? ~0 : 0, a == 1 ? ~0 : 0, a == 0 ? ~0 : 0 );
    v4u r;
    r.v = _mm_andnot_si128( mask, v.v );
    return r;
}

//
// Float Arithmetics
//
RBX_SIMD_INLINE v4f operator+( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_add_ps( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator+=( v4f& a, v4fArg b )
{
    a.v = _mm_add_ps( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f operator-( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_sub_ps( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator-=( v4f& a, v4fArg b )
{
    a.v = _mm_sub_ps( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f operator*( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_mul_ps( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator*=( v4f& a, v4fArg b )
{
    a.v = _mm_mul_ps( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f mulAdd( v4fArg a, v4fArg b, v4fArg c )
{
    v4f r;
    r.v = _mm_add_ps( a.v, _mm_mul_ps( b.v, c.v ) );
    return r;
}

RBX_SIMD_INLINE v4f operator-( v4fArg a )
{
    v4f r;
    r.v = _mm_sub_ps( _mm_setzero_ps(), a.v );
    return r;
}

RBX_SIMD_INLINE v4f operator/( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_div_ps( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator/=( v4f& a, v4fArg b )
{
    a.v = _mm_div_ps( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f abs( v4fArg a )
{
    __m128 signMask = _mm_castsi128_ps( _mm_set1_epi32( 0x80000000 ) );
    v4f r;
    r.v = _mm_andnot_ps( signMask, a.v );
    return r;
}

RBX_SIMD_INLINE v4f inverseEstimate0( v4fArg a )
{
    return v4f( _mm_rcp_ps( a.v ) );
}

RBX_SIMD_INLINE v4f inverseEstimate0Fast( v4fArg a )
{
    return inverseEstimate0( a );
}

namespace details
{
    using namespace simd;
    RBX_SIMD_INLINE v4f inverseEstimateIteration(const v4f& a, const v4f& estimate)
    {
        return estimate * ( simd::splat( 2.0f ) - a * estimate );
    }
}

RBX_SIMD_INLINE v4f inverseEstimate1Fast( v4fArg a )
{
    v4f estimate = inverseEstimate0Fast( a );
    return details::inverseEstimateIteration( a, estimate );
}

RBX_SIMD_INLINE v4f inverseEstimate1( v4fArg a )
{
    v4f estimate = inverseEstimate0Fast( a );
    v4f estimate1 = details::inverseEstimateIteration(a, estimate);

    // Handle denorms
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );

    // Handle infinities
    v4f signedZero = _mm_and_ps( a.v, _mm_set1_ps( -0.0f ) );
    return select( estimate1, signedZero, abs( a ) == splat( std::numeric_limits< float >::infinity() ) );
}

RBX_SIMD_INLINE v4f smallestInvertible( )
{
    return splat( std::numeric_limits< float >::min() );
}

RBX_SIMD_INLINE v4f largestInvertible( )
{
    return splat( 8.50705867e+37f );
}

namespace details
{
    RBX_SIMD_INLINE v4f inverseSqrtIteration( const v4f& v, const v4f& estimate )
    {
        return ( estimate * ( simd::splat( 1.5f ) - ( ( simd::splat( 0.5f ) * estimate ) * ( v * estimate ) ) ) );
    }
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate0Fast( v4fArg a )
{
    v4f r;
    r.v = _mm_rsqrt_ps( a.v );
    return r;
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate0( v4fArg a )
{
    v4f r;
    r.v = _mm_rsqrt_ps( a.v );
    // Correcting for the fact that negative denormals produce -inf
    return select( r, splat( std::numeric_limits< float >::quiet_NaN() ), a < zerof() );
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate1Fast( v4fArg a )
{
    v4f estimate = inverseSqrtEstimate0( a );
    estimate = details::inverseSqrtIteration( a, estimate );
    return estimate;
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate1( v4fArg a )
{
    v4f estimate = inverseSqrtEstimate0( a );
    v4f estimate1 = details::inverseSqrtIteration( a, estimate );
    
    // Handle small values
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );
    
    // Correcting for the fact that negative denormals produce -inf
    return select( estimate1, splat( std::numeric_limits< float >::quiet_NaN() ), a < zerof() );
}

RBX_SIMD_INLINE v4f smallestSqrtInvertible( )
{
    return splat( 1.17549435e-38f );
}

RBX_SIMD_INLINE v4f largestSqrtInvertible( )
{
    return splat( std::numeric_limits< float >::max() );
}

RBX_SIMD_INLINE v4f max( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_max_ps( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f min( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = _mm_min_ps( a.v, b.v );
    return r;
}

//
// Inter-element vector ops
//
RBX_SIMD_INLINE v4f dotProduct( v4fArg a, v4fArg b )
{
#ifdef RBX_SSE4
    v4f r;
    r.v = _mm_dp_ps( a.v, b.v, 0xff );
    return r;
#else
    v4f t = a * b;
    t = t + permute< 1, 0, 3, 2 >( t );
    t = t + permute< 2, 2, 0, 0 >( t );
    return t;
#endif
}

RBX_SIMD_INLINE v4f dotProduct3( v4fArg a, v4fArg b )
{
#ifdef RBX_SSE4
    v4f r;
    r.v = _mm_dp_ps( a.v, b.v, 0x77 );
    return r;
#else
    v4f t = a * b;
    t = replaceWithZero< 3 >( t );
    t = t + permute< 1, 0, 3, 2 >( t );
    t = t + permute< 2, 2, 0, 0 >( t );
    return t;
#endif
}

RBX_SIMD_INLINE v4f sumAcross( v4fArg a )
{
#ifdef RBX_SSE3
    v4f r;
    r.v = _mm_hadd_ps( a.v, a.v );
    r.v = _mm_hadd_ps( r.v, r.v );
    return r;
#else
    v4f t = a;
    t = t + permute< 1, 0, 3, 2 >( t );
    t = t + permute< 2, 2, 0, 0 >( t );
    return t;
#endif
}

RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b )
{
    v4f a0b0a1b1 = zipLow( a, b );
    v4f a1b1a1b1 = moveHighLow( a0b0a1b1, a0b0a1b1 );
    return a0b0a1b1 + a1b1a1b1;
}

RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b )
{
    v4f a0b0a1b1 = zipLow( a, b );
    v4f a1b1a1b1 = moveHighLow( a0b0a1b1, a0b0a1b1 );
    v4f a2b2a3b3 = zipHigh( a, b );
    return ( a0b0a1b1 + a1b1a1b1 ) + a2b2a3b3;
}

RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b )
{
    v4f a0b0a1b1 = zipLow( a, b );
    v4f a1b1xxxx = moveHighLow( a0b0a1b1, a0b0a1b1 );
    v4f a2b2a3b3 = zipHigh( a, b );
    v4f a3b3xxxx = moveHighLow( a2b2a3b3, a2b2a3b3 );
    return ( a0b0a1b1 + a1b1xxxx ) + ( a2b2a3b3 + a3b3xxxx );
}

//
// Packing / unpacking
//
template< class T >
RBX_SIMD_INLINE void pack3( typename T::pod_t* __restrict p, const T& a, const T& b, const T& c, const T& d )
{
    T t0 = shuffle< 1,2,0,1 >( a, b );
    T t1 = shuffle< 1,2,0,1 >( b, c );
    T t2 = shuffle< 1,2,0,1 >( c, d );
    p[0] = shuffle< 0,1,1,2 >( a, t0 );
    p[1] = t1;
    p[2] = shuffle< 1,2,1,2 >( t2, d );
}

template< class T >
RBX_SIMD_INLINE void unpack3( T& a, T& b, T& c, T& d, const typename T::pod_t* p )
{
    a = p[0];
    b = replace< 0 >( rotateLeft< 3 >( (T)p[1] ), splat< 3 >( (T)p[0] ) );
    c = replace< 2 >( rotateLeft< 2 >( (T)p[1] ), splat< 0 >( (T)p[2] ) );
    d = rotateLeft< 1 >( (T)p[2] );
}

}

}
