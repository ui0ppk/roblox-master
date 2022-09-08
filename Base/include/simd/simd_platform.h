#pragma once

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <stdint.h>

#if defined( __i386__ ) || defined( _M_IX86 ) // __i386__ on clang, _M_IX86 on MVC
#define RBX_SIMD_X86 
#endif

#if defined( __x86_64__ ) || defined( _M_X64 ) // __x86_64__ on clang, _M_X64 on MVC
#define RBX_SIMD_X64
#endif

#if defined( __ARM_NEON ) || defined( __arm__ ) || defined( _M_ARM ) // __ARM_NEON on apple clang for iOS, __arm__ on GCC, _M_ARM in MVC for Windows Phone
#define RBX_SIMD_ARM
#endif

#if defined( RBX_SIMD_X86 ) || defined( RBX_SIMD_X64 )
#define RBX_SIMD_USE_SSE
#elif defined( RBX_SIMD_ARM )
#define RBX_SIMD_USE_NEON
#endif

#ifdef RBX_SIMD_USE_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#include <mmintrin.h>
#endif

#ifdef RBX_SIMD_USE_NEON
#include <arm_neon.h>
#endif

#define RBX_SIMD_ALIGN_ASSERT( p, a ) RBXASSERT_VERY_FAST( ( ( uint64_t )( p ) & ( a - 1 ) ) == 0 )

#ifdef _MSC_VER
#define RBX_SIMD_INLINE __forceinline
#elif defined( __clang__ ) || defined( __GNUC__ )
#define RBX_SIMD_INLINE inline __attribute__((always_inline))
#else
#define RBX_SIMD_INLINE inline
#endif

namespace RBX
{

namespace simd
{

namespace details
{
#if defined( RBX_SIMD_USE_SSE )
    typedef __m128 vec4f_t;
    typedef __m128i vec4i_t;
    typedef __m128i vec4u_t;
#elif defined( RBX_SIMD_USE_NEON )
    typedef float32x4_t vec4f_t;
    typedef int32x4_t vec4i_t;
    typedef uint32x4_t vec4u_t;
#endif

template< class ScalarType >
struct VectorTypeSelect;

template< >
struct VectorTypeSelect< float >
{
    typedef vec4f_t type;
};

template< >
struct VectorTypeSelect< int32_t >
{
    typedef vec4i_t type;
};

template< >
struct VectorTypeSelect< uint32_t >
{
    typedef vec4u_t type;
};

}

}

}
