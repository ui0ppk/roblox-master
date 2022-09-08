#pragma once

// Do not include this file directly. Include simd/simd.h

#include "simd/simd_types.h"
#include <arm_neon.h>

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
        return vld1q_f32( s );
    }

    RBX_SIMD_INLINE v4i::pod_t load( const int32_t* s )
    {
        return vld1q_s32( s );
    }
    
    RBX_SIMD_INLINE v4u::pod_t load( const uint32_t* s )
    {
        return vld1q_u32( s );
    }

    template< int Lane >
    RBX_SIMD_INLINE v4f::pod_t loadLane( const float* s, v4f::pod_t vec )
    {
        return vld1q_lane_f32( s, vec, Lane );
    }

    template< int Lane >
    RBX_SIMD_INLINE v4u::pod_t loadLane( const uint32_t* s, v4u::pod_t vec )
    {
        return vld1q_lane_u32( s, vec, Lane );
    }

    template< int Lane >
    RBX_SIMD_INLINE v4i::pod_t loadLane( const int32_t* s, v4i::pod_t vec )
    {
        return vld1q_lane_s32( s, vec, Lane );
    }

    RBX_SIMD_INLINE v4f::pod_t loadSplat( const float* s )
    {
        return vld1q_dup_f32( s );
    }

    RBX_SIMD_INLINE v4u::pod_t loadSplat( const uint32_t* s )
    {
        return vld1q_dup_u32( s );
    }

    RBX_SIMD_INLINE v4i::pod_t loadSplat( const int32_t* s )
    {
        return vld1q_dup_s32( s );
    }
    
    RBX_SIMD_INLINE float32x2_t loadD( const float* s )
    {
        return vld1_f32( s );
    }

    RBX_SIMD_INLINE int32x2_t loadD( const int32_t* s )
    {
        return vld1_s32( s );
    }

    RBX_SIMD_INLINE uint32x2_t loadD( const uint32_t* s )
    {
        return vld1_u32( s );
    }

    RBX_SIMD_INLINE float32x2_t loadSplatD( const float* s )
    {
        return vld1_dup_f32( s );
    }

    RBX_SIMD_INLINE int32x2_t loadSplatD( const int32_t* s )
    {
        return vld1_dup_s32( s );
    }

    RBX_SIMD_INLINE uint32x2_t loadSplatD( const uint32_t* s )
    {
        return vld1_dup_u32( s );
    }
    
    RBX_SIMD_INLINE float32x4_t combine( float32x2_t a, float32x2_t b )
    {
        return vcombine_f32( a, b );
    }

    RBX_SIMD_INLINE int32x4_t combine( int32x2_t a, int32x2_t b )
    {
        return vcombine_s32( a, b );
    }

    RBX_SIMD_INLINE uint32x4_t combine( uint32x2_t a, uint32x2_t b )
    {
        return vcombine_u32( a, b );
    }

    template< class Type >
    RBX_SIMD_INLINE typename Type::pod_t zero();
    
    template< >
    RBX_SIMD_INLINE v4f::pod_t zero< v4f >()
    {
        float32x2_t z = vcreate_f32( 0 );
        return vcombine_f32( z, z );
    }

    template< >
    RBX_SIMD_INLINE v4i::pod_t zero< v4i >()
    {
        int32x2_t z = vcreate_s32( 0 );
        return vcombine_s32( z, z );
    }

    template< >
    RBX_SIMD_INLINE v4u::pod_t zero< v4u >()
    {
        uint32x2_t z = vcreate_u32( 0 );
        return vcombine_u32( z, z );
    }
    
    template< class ScalarType >
    struct v2;
    
    template< >
    struct v2< float >
    {
        typedef float32x2_t pod_t;
        pod_t v;
    };

    template< >
    struct v2< int32_t >
    {
        typedef int32x2_t pod_t;
        pod_t v;
    };

    template< >
    struct v2< uint32_t >
    {
        typedef uint32x2_t pod_t;
        pod_t v;
    };

    template< class ScalarType >
    struct v2x2;
    
    template< >
    struct v2x2< float >
    {
        typedef float32x2x2_t pod_t;
        pod_t v;
    };

    template< >
    struct v2x2< int32_t >
    {
        typedef int32x2x2_t pod_t;
        pod_t v;
    };

    template< >
    struct v2x2< uint32_t >
    {
        typedef uint32x2x2_t pod_t;
        pod_t v;
    };
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
    v4< ScalarType > r;
    r.v = details::load( s );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > loadSingle( const ScalarType* s )
{
    v4< ScalarType > r;
    r.v = details::loadLane< 0 >( s, details::zero< v4< ScalarType > >() );
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
    float32x2_t xy = details::loadD(value);
    float32x2_t z = details::loadSplatD((value + 2));
    v4f r;
    r.v = details::combine(xy, z);
    return r;
}

//
// Storing
//
namespace details
{
    RBX_SIMD_INLINE void store( float* s, float32x4_t v )
    {
        vst1q_f32( s, v );
    }

    RBX_SIMD_INLINE void store( int32_t* s, int32x4_t v )
    {
        vst1q_s32( s, v );
    }

    RBX_SIMD_INLINE void store( uint32_t* s, uint32x4_t v )
    {
        vst1q_u32( s, v );
    }

    template< int lane >
    RBX_SIMD_INLINE void storeLane( float* s, float32x4_t v )
    {
        vst1q_lane_f32( s, v, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE void storeLane( int32_t* s, int32x4_t v )
    {
        vst1q_lane_s32( s, v, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE void storeLane( uint32_t* s, uint32x4_t v )
    {
        vst1q_lane_u32( s, v, lane );
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
    details::store( dst, v.v );
}

template< class ScalarType >
RBX_SIMD_INLINE void storeSingle( ScalarType* dst, const v4< ScalarType >& v )
{
    details::storeLane< 0 >( dst, v.v );
}

//
// Vector Forming
//
RBX_SIMD_INLINE v4f zerof()
{
    v4f r;
    r.v = details::zero< v4f >();
    return r;
}

RBX_SIMD_INLINE v4i zeroi()
{
    v4i r;
    r.v = details::zero< v4i >();
    return r;
}

RBX_SIMD_INLINE v4u zerou()
{
    v4u r;
    r.v = details::zero< v4u >();
    return r;
}

namespace details
{
    RBX_SIMD_INLINE float32x4_t splat( float s )
    {
        return vdupq_n_f32( s );
    }

    RBX_SIMD_INLINE int32x4_t splat( int32_t s )
    {
        return vdupq_n_s32( s );
    }

    RBX_SIMD_INLINE uint32x4_t splat( uint32_t s )
    {
        return vdupq_n_u32( s );
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
    RBX_SIMD_INLINE float32x2_t splatD( float s )
    {
        return vmov_n_f32( (s) );
    }

    RBX_SIMD_INLINE int32x2_t splatD( int32_t s )
    {
        return vmov_n_s32( (s) );
    }

    RBX_SIMD_INLINE uint32x2_t splatD( uint32_t s )
    {
        return vmov_n_u32( (s) );
    }
    
    RBX_SIMD_INLINE float32x2_t zip1( float32x2_t a, float32x2_t b )
    {
        return vzip_f32( a, b ).val[ 0 ];
    }

    RBX_SIMD_INLINE int32x2_t zip1( int32x2_t a, int32x2_t b )
    {
        return vzip_s32( a, b ).val[ 0 ];
    }

    RBX_SIMD_INLINE uint32x2_t zip1( uint32x2_t a, uint32x2_t b )
    {
        return vzip_u32( a, b ).val[ 0 ];
    }
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z, ScalarType w )
{
    typename details::v2< ScalarType >::pod_t vx = details::splatD( x );
    typename details::v2< ScalarType >::pod_t vy = details::splatD( y );
    typename details::v2< ScalarType >::pod_t vxy = details::zip1( vx, vy );

    typename details::v2< ScalarType >::pod_t vz = details::splatD( z );
    typename details::v2< ScalarType >::pod_t vw = details::splatD( w );
    typename details::v2< ScalarType >::pod_t vzw = details::zip1( vz, vw );
    
    v4< ScalarType > r;
    r.v = details::combine( vxy, vzw );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y, ScalarType z )
{
    typename details::v2< ScalarType >::pod_t vx = details::splatD( x );
    typename details::v2< ScalarType >::pod_t vy = details::splatD( y );
    typename details::v2< ScalarType >::pod_t vxy = details::zip1( vx, vy );

    typename details::v2< ScalarType >::pod_t vz = details::splatD( z );

    v4< ScalarType > r;
    r.v = details::combine( vxy, vz );
    return r;
}

template< class ScalarType >
RBX_SIMD_INLINE v4< ScalarType > form( ScalarType x, ScalarType y )
{
    typename details::v2< ScalarType >::pod_t vx = details::splatD( x );
    typename details::v2< ScalarType >::pod_t vy = details::splatD( y );
    typename details::v2< ScalarType >::pod_t vxy = details::zip1( vx, vy );

    v4< ScalarType > r;
    r.v = details::combine( vxy, vxy );
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

    v = _v;
    return el[ _i ];
}

//
// Casts: cast one type to the other preserving the bit representation
//
RBX_SIMD_INLINE v4u reinterpretAsUInt( v4fArg v )
{
    v4u r;
    r.v = vreinterpretq_u32_f32( (v.v) );
    return r;
}

RBX_SIMD_INLINE v4i reinterpretAsInt( v4fArg v )
{
    v4i r;
    r.v = vreinterpretq_s32_f32( (v.v) );
    return r;
}

RBX_SIMD_INLINE v4f reinterpretAsFloat( v4uArg v )
{
    v4f r;
    r.v = vreinterpretq_f32_u32( (v.v) );
    return r;
}

RBX_SIMD_INLINE v4f reinterpretAsFloat( v4iArg v )
{
    v4f r;
    r.v = vreinterpretq_f32_s32( (v.v) );
    return r;
}

RBX_SIMD_INLINE v4u reinterpretAsUInt( v4iArg v )
{
    v4u r;
    r.v = vreinterpretq_u32_s32( (v.v) );
    return r;
}

RBX_SIMD_INLINE v4i reinterpretAsInt( v4uArg v )
{
    v4i r;
    r.v = vreinterpretq_s32_u32( (v.v) );
    return r;
}

//
// Conversions: convert float to int and int to float
//

// Convert using rounding to closest
// vcvtq_s32_f32 uses truncate to zero so we need to do more...
RBX_SIMD_INLINE v4i convertFloat2IntNearest( v4fArg v )
{
    uint32x4_t sign = vcgeq_f32( v.v, vdupq_n_f32( 0.0f ) );
    int32x4_t t = vcvtq_s32_f32( vabsq_f32( v.v ) + vdupq_n_f32( 0.5f ) );
    
    v4i r;
    r.v = vbslq_s32( sign, t, -t );
    return r;
}

RBX_SIMD_INLINE v4i convertFloat2IntTruncate( v4fArg v )
{
    v4i r;
    r.v = vcvtq_s32_f32( v.v );
    return r;
}

RBX_SIMD_INLINE v4f convertIntToFloat( v4iArg v )
{
    v4f r;
    r.v = vcvtq_f32_s32( v.v );
    return r;
}

//
// Splat
//
namespace details
{
    RBX_SIMD_INLINE float32x2_t low( float32x4_t t )
    {
        return vget_low_f32( t );
    }

    RBX_SIMD_INLINE int32x2_t low( int32x4_t t )
    {
        return vget_low_s32( t );
    }

    RBX_SIMD_INLINE uint32x2_t low( uint32x4_t t )
    {
        return vget_low_u32( t );
    }

    RBX_SIMD_INLINE float32x2_t high( float32x4_t t )
    {
        return vget_high_f32( t );
    }

    RBX_SIMD_INLINE int32x2_t high( int32x4_t t )
    {
        return vget_high_s32( t );
    }

    RBX_SIMD_INLINE uint32x2_t high( uint32x4_t t )
    {
        return vget_high_u32( t );
    }
    
    template< int lane >
    RBX_SIMD_INLINE float32x4_t splat( float32x2_t a )
    {
        return vdupq_lane_f32( a, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE int32x4_t splat( int32x2_t a )
    {
        return vdupq_lane_s32( a, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE uint32x4_t splat( uint32x2_t a )
    {
        return vdupq_lane_u32( a, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE float32x2_t splatD( float32x2_t a )
    {
        return vdup_lane_f32( a, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE int32x2_t splatD( int32x2_t a )
    {
        return vdup_lane_s32( a, lane );
    }

    template< int lane >
    RBX_SIMD_INLINE uint32x2_t splatD( uint32x2_t a )
    {
        return vdup_lane_u32( a, lane );
    }
    
    template< class VectorType, unsigned i >
    struct SplatHelper;
    
    template< class VectorType >
    struct SplatHelper< VectorType, 0 >
    {
        static RBX_SIMD_INLINE VectorType splat( const VectorType& v )
        {
            typename v2< typename VectorType::elem_t >::pod_t h = low( v.v );
            VectorType r;
            r.v = details::splat< 0 >( h );
            return r;
        }
        
        static RBX_SIMD_INLINE v2< typename VectorType::elem_t > splatD( const VectorType& v )
        {
            v2< typename VectorType::elem_t > r;
            r.v = details::splatD< 0 >( low( v.v ) );
            return r;
        }
    };

    template< class VectorType >
    struct SplatHelper< VectorType, 1 >
    {
        static RBX_SIMD_INLINE VectorType splat( const VectorType& v )
        {
            typename v2< typename VectorType::elem_t >::pod_t h = low( v.v );
            VectorType r;
            r.v = details::splat< 1 >( h );
            return r;
        }
        
        static RBX_SIMD_INLINE v2< typename VectorType::elem_t > splatD( const VectorType& v )
        {
            v2< typename VectorType::elem_t > r;
            r.v = splatD< 1 >( low( v.v ) );
            return r;
        }

    };
    
    template< class VectorType >
    struct SplatHelper< VectorType, 2 >
    {
        static RBX_SIMD_INLINE VectorType splat( const VectorType& v )
        {
            typename v2< typename VectorType::elem_t >::pod_t h = high( v.v );
            VectorType r;
            r.v = details::splat< 0 >( h );
            return r;
        }
        
        static RBX_SIMD_INLINE v2< typename VectorType::elem_t > splatD( const VectorType& v )
        {
            v2< typename VectorType::elem_t > r;
            r.v = splatD< 0 >( high( v.v ) );
            return r;
        }

    };

    template< class VectorType >
    struct SplatHelper< VectorType, 3 >
    {
        static RBX_SIMD_INLINE VectorType splat( const VectorType& v )
        {
            typename v2< typename VectorType::elem_t >::pod_t h = high( v.v );
            VectorType r;
            r.v = details::splat< 1 >( h );
            return r;
        }
        
        static RBX_SIMD_INLINE v2< typename VectorType::elem_t > splatD( const VectorType& v )
        {
            v2< typename VectorType::elem_t > r;
            r.v = splatD< 1 >( high( v.v ) );
            return r;
        }

    };
}

template< unsigned i, class VectorType >
RBX_SIMD_INLINE VectorType splat( const VectorType& v )
{
    return details::SplatHelper< VectorType, i >::splat( v );
}

//
// Selects
//
namespace details
{
    RBX_SIMD_INLINE float32x2_t selectD( float32x2_t u, float32x2_t v, uint32x2_t s )
    {
        return vbsl_f32( s, v, u );
    }

    RBX_SIMD_INLINE int32x2_t selectD( int32x2_t u, int32x2_t v, uint32x2_t s )
    {
        return vbsl_s32( s, v, u );
    }

    RBX_SIMD_INLINE uint32x2_t selectD( uint32x2_t u, uint32x2_t v, uint32x2_t s )
    {
        return vbsl_u32( s, v, u );
    }
    
    RBX_SIMD_INLINE uint8x8_t castToUInt8( float32x2_t v )
    {
        return vreinterpret_u8_f32( v );
    }

    RBX_SIMD_INLINE uint8x8_t castToUInt8( uint32x2_t v )
    {
        return vreinterpret_u8_u32( v );
    }

    RBX_SIMD_INLINE uint8x8_t castToUInt8( int32x2_t v )
    {
        return vreinterpret_u8_s32( v );
    }

    template< class From >
    RBX_SIMD_INLINE uint8x8x2_t castToUInt8x2( From v )
    {
        uint8x8x2_t r;
        r.val[ 0 ] = details::castToUInt8( details::low( v ) );
        r.val[ 1 ] = details::castToUInt8( details::high( v ) );
        return r;
    }

    template< class From >
    RBX_SIMD_INLINE uint8x8x4_t castToUInt8x4( From u, From v )
    {
        uint8x8x4_t r;
        r.val[ 0 ] = details::castToUInt8( details::low( u ) );
        r.val[ 1 ] = details::castToUInt8( details::high( u ) );
        r.val[ 2 ] = details::castToUInt8( details::low( v ) );
        r.val[ 3 ] = details::castToUInt8( details::high( v ) );
        return r;
    }


    template< class To >
    struct CastFromUInt8;
    
    template<>
    struct CastFromUInt8< float32x4_t >
    {
        static RBX_SIMD_INLINE float32x4_t cast( uint8x8x2_t v )
        {
            float32x2_t r0 = vreinterpret_f32_u8( v.val[0] );
            float32x2_t r1 = vreinterpret_f32_u8( v.val[1] );
            return details::combine(r0, r1);
        }

        static RBX_SIMD_INLINE float32x2_t cast( uint8x8_t v )
        {
            return vreinterpret_f32_u8( v );
        }
    };
    
    template<>
    struct CastFromUInt8< uint32x4_t >
    {
        static RBX_SIMD_INLINE uint32x4_t cast( uint8x8x2_t v )
        {
            uint32x2_t r0 = vreinterpret_u32_u8( v.val[0] );
            uint32x2_t r1 = vreinterpret_u32_u8( v.val[1] );
            return details::combine(r0, r1);
        }

        static RBX_SIMD_INLINE uint32x2_t cast( uint8x8_t v )
        {
            return vreinterpret_u32_u8( v );
        }
    };

    template<>
    struct CastFromUInt8< int32x4_t >
    {
        static RBX_SIMD_INLINE int32x4_t cast( uint8x8x2_t v )
        {
            int32x2_t r0 = vreinterpret_s32_u8( v.val[0] );
            int32x2_t r1 = vreinterpret_s32_u8( v.val[1] );
            return details::combine(r0, r1);
        }

        static RBX_SIMD_INLINE int32x2_t cast( uint8x8_t v )
        {
            return vreinterpret_s32_u8( v );
        }
    };
    
    template< int a, int b >
    RBX_SIMD_INLINE uint32x2_t selectMaskD()
    {
        boost::uint64_t mask64 = ( boost::uint64_t( boost::uint32_t( -( boost::int32_t(b) & 0x1 ) ) ) << 32 )
        | boost::uint64_t( boost::uint32_t( -(boost::int32_t)( a & 0x1 ) ) );
        return vcreate_u32( mask64 );
    }
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d >
RBX_SIMD_INLINE v4u selectMask()
{
    uint32x2_t low_u = details::selectMaskD< a, b >();
    uint32x2_t high_u = details::selectMaskD< c, d >();
    v4u r;
    r.v = vcombine_u32( low_u, high_u );
    return r;
}

template< class VectorType >
RBX_SIMD_INLINE VectorType select( const VectorType& a, const VectorType& b, const v4u& mask )
{
    VectorType r;
    typename details::v2< typename VectorType::elem_t >::pod_t rlow = details::selectD( details::low( a.v ), details::low( b.v ), details::low( mask.v ) );
    typename details::v2< typename VectorType::elem_t >::pod_t rhigh = details::selectD( details::high( a.v ), details::high( b.v ), details::high( mask.v ) );
    r.v = details::combine( rlow, rhigh );
    return r;
}

#define RBX_SIMD_NEON_TBL_MASK( a ) ( ( 4 * ( a ) ) | ( ( 4 * ( a ) + 1 ) << 8 ) | ( ( 4 * ( a ) + 2 ) << 16 ) | ( ( 4 * ( a ) + 3 ) << 24 ) )

namespace details
{
    template< uint32_t a, uint32_t b, class VectorType >
    RBX_SIMD_INLINE typename details::v2< typename VectorType::elem_t >::pod_t chooseTwoElements( const VectorType& u )
    {
        uint8x8x2_t u8 = details::castToUInt8x2( u.v );
        uint64_t lowMask = uint64_t( RBX_SIMD_NEON_TBL_MASK( a ) ) | ( uint64_t( RBX_SIMD_NEON_TBL_MASK( b ) ) << 32 );
        return details::CastFromUInt8< typename VectorType::pod_t >::cast( vtbl2_u8( u8, vcreate_u8(lowMask) ) );
    }

    template< uint32_t a, uint32_t b, class VectorType >
    RBX_SIMD_INLINE typename details::v2< typename VectorType::elem_t >::pod_t chooseTwoElements( const VectorType& u, const VectorType& v )
    {
        uint8x8x4_t u8 = details::castToUInt8x4( u.v, v.v );
        uint64_t lowMask = uint64_t( RBX_SIMD_NEON_TBL_MASK( a ) ) | ( uint64_t( RBX_SIMD_NEON_TBL_MASK( b ) ) << 32 );
        return details::CastFromUInt8< typename VectorType::pod_t >::cast( vtbl4_u8( u8, vcreate_u8(lowMask) ) );
    }
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class VectorType >
RBX_SIMD_INLINE VectorType shuffle( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::combine( details::chooseTwoElements<a, b>( u ), details::chooseTwoElements<c, d>( v ) );
    return r;
}

namespace details
{
    RBX_SIMD_INLINE float32x4_t zipLow( float32x4_t a, float32x4_t b )
    {
        float32x2x2_t t = vzip_f32( low(a), low(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE uint32x4_t zipLow( uint32x4_t a, uint32x4_t b )
    {
        uint32x2x2_t t = vzip_u32( low(a), low(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE int32x4_t zipLow( int32x4_t a, int32x4_t b )
    {
        int32x2x2_t t = vzip_s32( low(a), low(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE float32x4_t zipHigh( float32x4_t a, float32x4_t b )
    {
        float32x2x2_t t = vzip_f32( high(a), high(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE uint32x4_t zipHigh( uint32x4_t a, uint32x4_t b )
    {
        uint32x2x2_t t = vzip_u32( high(a), high(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE int32x4_t zipHigh( int32x4_t a, int32x4_t b )
    {
        int32x2x2_t t = vzip_s32( high(a), high(b) );
        return combine( t.val[0], t.val[1] );
    }

    RBX_SIMD_INLINE float32x4x2_t zip( float32x4_t a, float32x4_t b )
    {
        return vzipq_f32( a, b );
    }

    RBX_SIMD_INLINE uint32x4x2_t zip( uint32x4_t a, uint32x4_t b )
    {
        return vzipq_u32( a, b );
    }

    RBX_SIMD_INLINE int32x4x2_t zip( int32x4_t a, int32x4_t b )
    {
        return vzipq_s32( a, b );
    }
}

template< class VectorType >
RBX_SIMD_INLINE VectorType zipLow( const VectorType& u, const VectorType& v )
{
    return VectorType( details::zipLow( u.v, v.v ) );
}

template< class VectorType >
RBX_SIMD_INLINE VectorType zipHigh( const VectorType& u, const VectorType& v )
{
    return VectorType( details::zipHigh( u.v, v.v ) );
}

template< class VectorType >
RBX_SIMD_INLINE void zip( VectorType& r0, VectorType& r1, const VectorType& u, const VectorType& v )
{
    auto r = details::zip( u.v, v.v );
    r0.v = r.val[0];
    r1.v = r.val[1];
}

template< class VectorType >
RBX_SIMD_INLINE VectorType moveHighLow( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::combine( details::high( v ), details::high( u ) );
    return r;
}

template< class VectorType >
RBX_SIMD_INLINE VectorType moveLowHigh( const VectorType& u, const VectorType& v )
{
    VectorType r;
    r.v = details::combine( details::low( u ), details::low( v ) );
    return r;
}

namespace details
{
    template< class T, uint32_t a, uint32_t b, uint32_t c, uint32_t d >
    struct SelectHelper
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            v4u mask = selectMask<a, b, c, d>();
            return simd::select( u, v, mask );
        }
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
            uint32x2_t mask = details::selectMaskD< 1, 0 >();
            typename details::v2< typename T::elem_t >::pod_t lowr = details::selectD( details::low( u ), details::low( v ), mask );
            T r;
            r.v = details::combine( lowr, details::high( u ) );
            return r;
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,1,1 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            return SelectHelper< T, 1, 0, 0, 0 >::select( v, u );
        }
    };

    template< class T >
    struct SelectHelper< T, 0,1,0,0 >
    {
        static RBX_SIMD_INLINE T select( const T& u, const T& v )
        {
            uint32x2_t mask = details::selectMaskD< 0, 1 >();
            typename details::v2< typename T::elem_t >::pod_t lowr = details::selectD( details::low( u ), details::low( v ), mask );
            T r;
            r.v = details::combine( lowr, details::high( u ) );
            return r;
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
            uint32x2_t mask = details::selectMaskD< 1, 0 >();
            typename details::v2< typename T::elem_t >::pod_t highr = details::selectD( details::high( u ), details::high( v ), mask );
            T r;
            r.v = details::combine( details::low( u ), highr );
            return r;
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
            uint32x2_t mask = details::selectMaskD< 0, 1 >();
            typename details::v2< typename T::elem_t >::pod_t highr = details::selectD( details::high( u ), details::high( v ), mask );
            T r;
            r.v = details::combine( details::low( u ), highr );
            return r;
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
            return details::combine( details::low(u), details::high(v) );
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
}

template< uint32_t a, uint32_t b, uint32_t c, uint32_t d, class T >
RBX_SIMD_INLINE T select( const T& u, const T& v )
{
    return details::SelectHelper< T, a, b, c, d >::select( u, v );
}

template< uint32_t index, class T >
RBX_SIMD_INLINE T replace( const T& a, const T& b )
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
    VectorType r;
    r.v = details::combine( details::chooseTwoElements<a, b>(u), details::chooseTwoElements<c, d>(u) );
    return r;
}

template< uint32_t offset, class T >
RBX_SIMD_INLINE T rotateLeft( const T& v )
{
    return shuffle< offset % 4, ( offset + 1 ) % 4, ( offset + 2 ) % 4, ( offset + 3 ) % 4 >( v, v );
}

//
// Compares
//
RBX_SIMD_INLINE v4u compareGreater( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcgtq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator>( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcgtq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u compareGreaterEqual( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcgeq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator>=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcgeq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u compareLess( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcltq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator<( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcltq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u compareLessEqual( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcleq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator<=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vcleq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vceqq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vceqq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4fArg a, v4fArg b )
{
    v4u r;
    r.v = vornq_u32( vdupq_n_u32( 0 ), vceqq_f32( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = vceqq_s32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = vceqq_s32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4iArg a, v4iArg b )
{
    v4u r;
    r.v = vornq_u32( vdupq_n_u32( 0 ), vceqq_s32( a.v, b.v ) );
    return r;
}

RBX_SIMD_INLINE v4u compare( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = vceqq_u32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator==( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = vceqq_u32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4u operator!=( v4uArg a, v4uArg b )
{
    v4u r;
    r.v = vornq_u32( vdupq_n_u32( 0 ), vceqq_u32( a.v, b.v ) );
    return r;
}

//
// Set
//
template< uint32_t a >
RBX_SIMD_INLINE v4f replaceWithZero( v4fArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    v4f r;
    r.v = vsetq_lane_f32(0.0f, v.v, a);
    return r;
}

template< uint32_t a >
RBX_SIMD_INLINE v4i replaceWithZero( v4iArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    v4i r;
    r.v = vsetq_lane_s32(0, v.v, a);
    return r;
}

template< uint32_t a >
RBX_SIMD_INLINE v4u replaceWithZero( v4uArg v )
{
    BOOST_STATIC_ASSERT_MSG( ( a & 0x3 ) == a, "a must be between 0 and 3" );
    v4u r;
    r.v = vsetq_lane_u32(0, v.v, a);
    return r;
}

//
// Float Arithmetics
//
RBX_SIMD_INLINE v4f operator+( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = vaddq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator+=( v4f& a, v4fArg b )
{
    a.v = vaddq_f32( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f operator-( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = vsubq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator-=( v4f& a, v4fArg b )
{
    a.v = vsubq_f32( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f operator*( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = vmulq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f& operator*=( v4f& a, v4fArg b )
{
    a.v = vmulq_f32( a.v, b.v );
    return a;
}

RBX_SIMD_INLINE v4f mulAdd( v4fArg a, v4fArg b, v4fArg c )
{
    v4f r;
    r.v = vmlaq_f32( a.v, b.v, c.v );
    return r;
}

RBX_SIMD_INLINE v4f operator-( v4fArg a )
{
    v4f r;
    r.v = vnegq_f32( a.v );
    return r;
}

RBX_SIMD_INLINE v4f operator/( v4fArg a, v4fArg b )
{
    return a * inverseEstimate1( b );
}

RBX_SIMD_INLINE v4f& operator/=( v4f& a, v4fArg b )
{
    a = a * inverseEstimate1( b );
    return a;
}

RBX_SIMD_INLINE v4f abs( v4fArg a )
{
    v4f r;
    r.v = vabsq_f32( a.v );
    return r;
}

//
// Inverse estimates
//
namespace details
{
    RBX_SIMD_INLINE v4f inverseEstimateIteration(const v4f& a, const v4f& estimate)
    {
        return estimate * v4f( vrecpsq_f32( a.v, estimate.v ) );
    }
}

RBX_SIMD_INLINE v4f inverseEstimate0Fast( v4fArg a )
{
    v4f estimate = v4f(vrecpeq_f32( a.v ));
    v4f estimate1 = details::inverseEstimateIteration(a, estimate);
    return estimate1;
}

RBX_SIMD_INLINE v4f inverseEstimate0( v4fArg a )
{
    v4f estimate = v4f(vrecpeq_f32( a.v ));
    v4f estimate1 = details::inverseEstimateIteration(a, estimate);

    // Handle small inputs
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );

    // Handle infinities
    v4f signedZero = vreinterpretq_f32_u32( vandq_u32( vreinterpretq_u32_f32( vdupq_n_f32(-0.0f) ), vreinterpretq_u32_f32( a.v ) ) );
    return select( estimate1, signedZero, abs( a ) == splat( std::numeric_limits< float >::infinity() ) );
}

RBX_SIMD_INLINE v4f inverseEstimate1Fast( v4fArg a )
{
    v4f estimate = v4f(vrecpeq_f32( a.v ));
    v4f estimate1 = details::inverseEstimateIteration(a, estimate);
    estimate1 = details::inverseEstimateIteration(a, estimate1);
    return estimate1;
}

RBX_SIMD_INLINE v4f inverseEstimate1( v4fArg a )
{
    v4f estimate = v4f(vrecpeq_f32( a.v ));
    v4f estimate1 = details::inverseEstimateIteration(a, estimate);
    estimate1 = details::inverseEstimateIteration(a, estimate1);

    // Handle denorms
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );

    // Handle infinities
    v4f signedZero = vreinterpretq_f32_u32( vandq_u32( vreinterpretq_u32_f32( vdupq_n_f32(-0.0f) ), vreinterpretq_u32_f32( a.v ) ) );
    return select( estimate1, signedZero, abs( a ) == splat( std::numeric_limits< float >::infinity() ) );
}

RBX_SIMD_INLINE v4f smallestInvertible( )
{
    // This is not the same as std::numeric_limints< float >::min().
    // This number is denormalized
    return splat( 2.9387359e-39f );
}

RBX_SIMD_INLINE v4f largestInvertible( )
{
    return splat( 3.40282347e+38f );
}

//
// Inverse sqrt estimate
//
namespace details
{
    RBX_SIMD_INLINE v4f inverseSqrtIteration(const v4f& v, const v4f& estimate) 
    {
        v4f estimate2 = estimate * v;
        return estimate * v4f( vrsqrtsq_f32(estimate2.v, estimate.v) );
    }
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate0( v4fArg a )
{
    v4f estimate( vrsqrteq_f32( a.v ) );
    v4f estimate1 = details::inverseSqrtIteration(a, estimate);
    
    // Handle small numbers
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );
    
    // Handle infinities
    return select( estimate1, splat(0.0f), a == splat( std::numeric_limits< float >::infinity() ) );
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate0Fast( v4fArg a )
{
    v4f estimate( vrsqrteq_f32( a.v ) );
    estimate = details::inverseSqrtIteration(a, estimate);
    return estimate;
}

RBX_SIMD_INLINE v4f inverseSqrtEstimate1( v4fArg a )
{
    v4f estimate( vrsqrteq_f32( a.v ) );
    v4f estimate1 = details::inverseSqrtIteration(a, estimate);
    estimate1 = details::inverseSqrtIteration(a, estimate1);
    
    // Handle small numbers
    estimate1 = select( estimate, estimate1, abs( estimate1 ) < splat( std::numeric_limits< float >::infinity() ) );
    
    // Handle infinities
    return select( estimate1, splat(0.0f), a == splat( std::numeric_limits< float >::infinity() ) );
}


RBX_SIMD_INLINE v4f inverseSqrtEstimate1Fast(const v4f& v) 
{
    v4f estimate( vrsqrteq_f32( v.v ) );
    estimate = details::inverseSqrtIteration(v, estimate);
    estimate = details::inverseSqrtIteration(v, estimate);
    return estimate;
}

RBX_SIMD_INLINE v4f smallestSqrtInvertible( )
{
    return splat( 1.4012985e-45f );
}

RBX_SIMD_INLINE v4f largestSqrtInvertible( )
{
    // This is the same as std::numeric_limints< float >::max()
    return splat( 3.40282347E+38f );
}

RBX_SIMD_INLINE v4f max( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = vmaxq_f32( a.v, b.v );
    return r;
}

RBX_SIMD_INLINE v4f min( v4fArg a, v4fArg b )
{
    v4f r;
    r.v = vminq_f32( a.v, b.v );
    return r;
}

//
// Inter-element vector ops
//
RBX_SIMD_INLINE v4f dotProduct( v4fArg a, v4fArg b )
{
    float32x4_t p0123 = vmulq_f32( a.v, b.v );
    float32x4_t p1032 = vrev64q_f32( p0123 );
    float32x4_t t = p0123 + p1032;
    t = t + details::combine( details::high( t ), details::low( t ) );
    return v4f( t );
}

RBX_SIMD_INLINE v4f dotProduct3( v4fArg a, v4fArg b )
{
    float32x4_t p0123 = vmulq_f32( a.v, b.v );
    p0123 = vsetq_lane_f32(0.0f, p0123, 3);
    float32x4_t p1032 = vrev64q_f32( p0123 );
    float32x4_t t = p0123 + p1032;
    t = t + details::combine( details::high( t ), details::low( t ) );
    return t;
}

RBX_SIMD_INLINE v4f sumAcross( v4fArg a )
{
    float32x4_t p0123 = a.v;
    float32x4_t p1032 = vrev64q_f32( p0123 );
    float32x4_t t = p0123 + p1032;
    t = t + details::combine( details::high( t ), details::low( t ) );
    return v4f( t );
}

RBX_SIMD_INLINE v4f sumAcross2( v4fArg a, v4fArg b )
{
    float32x2x2_t t = vtrn_f32( details::low( a.v ), details::low( b.v ) );
    float32x2_t sum = t.val[0] + t.val[1];
    return v4f( details::combine( sum, sum ) );
}

RBX_SIMD_INLINE v4f sumAcross3( v4fArg a, v4fArg b )
{
    float32x2x2_t t01 = vtrn_f32( details::low( a.v ), details::low( b.v ) );
    float32x2x2_t t2 = vtrn_f32( details::high( a.v ), details::high( b.v ) );
    float32x2_t sum = t01.val[0] + t01.val[1] + t2.val[0];
    return v4f( details::combine( sum, sum ) );
}

RBX_SIMD_INLINE v4f sumAcross4( v4fArg a, v4fArg b )
{
    float32x2x2_t t01 = vtrn_f32( details::low( a.v ), details::low( b.v ) );
    float32x2x2_t t23 = vtrn_f32( details::high( a.v ), details::high( b.v ) );
    float32x2_t sum = ( t01.val[0] + t01.val[1] ) + ( t23.val[0] + t23.val[1] );
    return v4f( details::combine( sum, sum ) );
}

//
// Packing / unpacking
//

namespace details
{
    template< class T >
    RBX_SIMD_INLINE T moveHigh( const T& a, const T& b )
    {
        return details::combine( details::low( a ), details::high( b ) );
    }

    template< class T >
    RBX_SIMD_INLINE T moveLow( const T& a, const T& b )
    {
        return details::combine( details::low( b ), details::high( a ) );
    }
}

template< class T >
RBX_SIMD_INLINE void pack3( typename T::pod_t* __restrict p, const T& a, const T& b, const T& c, const T& d )
{
    p[0] = details::combine( details::low(a), details::zip1( details::high(a), details::low(b) ) );
    p[1] = details::combine( details::chooseTwoElements<1, 2>(b), details::low(c) );
    p[2] = details::combine( details::zip1( details::high(c), details::low(d) ), details::chooseTwoElements<1, 2>( d ) );
}

template< class T >
RBX_SIMD_INLINE void unpack3( T& a, T& b, T& c, T& d, const typename T::pod_t* p )
{
    a = p[0];
    b = details::combine( details::chooseTwoElements<3, 4>(T(p[0]),T(p[1])), details::chooseTwoElements<1, 2>(T(p[1])) );
    c = details::combine( details::chooseTwoElements<2, 3>(T(p[1])), details::chooseTwoElements<0, 1>(T(p[2])));
    d = details::combine( details::chooseTwoElements<1, 2>(T(p[2])), details::chooseTwoElements<3, 3>(T(p[2])));
}

}

}
