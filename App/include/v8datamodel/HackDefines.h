#pragma once

// If you are looking to run a release-like build with a debugger or vs production
// you should uncomment the LOVE_ALL_ACCESS line.  This should never be set for
// actual releases!
// (Now conveniently located at the top of the file!)

//#define LOVE_ALL_ACCESS

#include "Security/RandomConstant.h"
#define LINE_RAND4 ((RBX_BUILDSEED&0x3FFFF)*__LINE__)

// This line is different due to an unexplained VS2012 issue in debug.
// __LINE__ appears to fail to evaluate to a constant in some cases.
#define LINE_RAND1 (((RBX_BUILDSEED&0xFF)*(__COUNTER__+1))&0xFC)

// HATE_FLAGS:  these are the original flags used for reporting detected exploits.

// "Impossible error" is used to help detect anything else.
// Perhaps the user is attempting to cause a game shutdown by
// attacking the networking protocol.
#define HATE_IMPOSSIBLE_ERROR 0x80000000
#define HATE_CE_ASM 0x40000000
#define HATE_NEW_AV_CHECK 0x20000000
#define HATE_HASH_FUNCTION_CHANGED 0x10000000

#define HATE_RETURN_CHECK 0x8000000
#define HATE_VERB_SNATCH 0x4000000
#define HATE_VEH_HOOK 0x2000000
#define HATE_HSCE_HASH_CHANGED 0x1000000

#define HATE_DLL_INJECTION 0x800000
#define HATE_INVALID_ENVIRONMENT 0x400000
#define HATE_SPEEDHACK 0x200000
#define HATE_LUA_VM_HOOKED 0x100000

#define HATE_OSX_MEMORY_HASH_CHANGED 0x80000
#define HATE_UNHOOKED_VEH 0x40000
#define HATE_CHEATENGINE_NEW 0x20000
#define HATE_HSCE_EBX 0x10000

#define HATE_WEAK_DM_POINTER_BROKEN 0x8000
#define HATE_LUA_HASH_CHANGED 0x4000
#define HATE_DESTROY_ALL 0x2000
#define HATE_SEH_CHECK 0x1000

#define HATE_HOOKED_GTX 0x800
#define HATE_DEBUGGER 0x400
#define HATE_LUA_SCRIPT_HASH_CHANGED 0x200
#define HATE_CATCH_EXECUTABLE_ACCESS_VIOLATION 0x100

#define HATE_CONST_CHANGED 0x80
#define HATE_INVALID_BYTECODE 0x40
#define HATE_MEMORY_HASH_CHANGED 0x20
#define HATE_ILLEGAL_SCRIPTS 0x10

#define HATE_SIGNATURE 0x8
#define HATE_NEW_HWBP 0x4
#define HATE_XXHASH_BROKEN 0x2
#define HATE_CHEATENGINE_OLD 0x1 // the image detection, hwnd scanner, and logs.

// SCORN_FLAGS:  these are the set of flags that extended the HATE_FLAGS
#define SCORN_IMPOSSIBLE_ERROR 0xFFFFF000
#define SCORN_REPLICATE_PROP 0xFFF

static const unsigned int kNoScornFlags = 0;

// These 32 bit-vectors create a basis for all 32b values.  
// eg, y = 0x455F4314 ^ 0xB108F6D2
// gives a value that corresponds to 0x00000003, assuming these are mapped to a 
// bit corresponding to the index in the table.
// (these values were generated from a sage script)
static const unsigned int kGf2EncodeLut[32] = { 
    0x455F4314, 0xB108F6D2, 0x3C297366, 0x76EFD2BB,
    0xBE165929, 0xDC284CB4, 0x69A0FE16, 0xCE19BDD9,
    0xFD1E6044, 0x0B2BD610, 0xC0BFA0AE, 0xB1004FD3,
    0x79D6A004, 0x78925BB3, 0x0E320F15, 0xDB56E1D6,
    0x685E2DC1, 0xFB5F13E1, 0x8B1571F0, 0x1E83936A,
    0xDB2AAE2A, 0xA49F0A74, 0xD0F8ADB7, 0x53D0B56E,
    0xE69C8A79, 0xD6FCEEF9, 0x80AC6B77, 0x68A47CCF,
    0x92CA6C6D, 0x7170E034, 0x4CE64F7D, 0xDB8CBB83 };

// These are the 32 bit vectors used to convert back.
// eg, innerProduct( (0x455F4314 ^ 0xB108F6D2), 0xFCEE5D30 ) = 1
//     innerProduct( (0x455F4314 ^ 0xB108F6D2), 0xDC401163 ) = 1
//     innerProduct( (0x455F4314 ^ 0xB108F6D2), 0xA295C49F ) = 0
//     ...
// inner product is parity(a & b) = (1 & popcnt(a & b))
// (these values were generated from a sage script)
static const unsigned int kGf2DecodeLut[32] = { 
    0xFCEE5D30, 0xDC401163, 0xA295C49F, 0xA73452E1,
    0x848D2984, 0x18DF419E, 0x73531D65, 0x11777D3E,
    0xD203172E, 0x5D2F07BA, 0x5A96E9DE, 0x7B80D8E4,
    0xCC969B58, 0x2B99050C, 0xC45979EB, 0xC124CFE9,
    0xF62F63BB, 0x49B0989B, 0xEC6E91C6, 0xB9C8406B,
    0x769E8FA2, 0xF3961122, 0x42038864, 0x3A31303E,
    0x21F1D62A, 0xC75CC383, 0xB9A4B34A, 0x1050D751,
    0xA10126CD, 0x3D1B091B, 0xCAC3F44F, 0x7F89722D };

#define MCC_FAKE_FFLAG_IDX 14
#define MCC_FREECONSOLE_IDX 13
#define MCC_SPEED_IDX 12

#define MCC_MCC_IDX 11
#define MCC_PMC_IDX 10
#define MCC_BAD_IDX 9
#define MCC_INIT_IDX 8

#define MCC_NULL1_IDX 7
#define MCC_NULL0_IDX 6
#define MCC_VEH_IDX 5
#define MCC_GTX_IDX 4

#define MCC_HWBP_IDX 3
#define MCC_RDATA_IDX  2
#define MCC_VMP_IDX 1
#define MCC_TEXT_IDX  0

namespace RBX
{
namespace Security
{
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
// This generates an "or" operation in a way that is closer to what a compiler would
// generate.
template<unsigned int key> __forceinline void setHackFlagVs(unsigned int& y, const unsigned int x)
{
    unsigned int dest = ((unsigned int)(&y) + key);
    int flag = x ^ _rotl(key,11);
    volatile int vFlag = flag;
    volatile unsigned int tmp = dest;
    vFlag ^= _rotl(key,11);
    *(unsigned int*)(tmp - key) |= vFlag;
}

// This performs an "or" operation in a way that blends in well with VMProtect, which
// favors "bts" over "or".
template<unsigned int key> __forceinline void setHackFlagVmp(unsigned int& y, const unsigned int x)
{
    unsigned int dest = ((unsigned int)(&y) + key);
    int flag = x ^ key ^ _rotl(key,17);
    volatile int vFlag = flag ^ key;
    volatile unsigned int tmp = dest;
    tmp -= key;
    vFlag ^= _rotl(key,17);
    unsigned long bitLoc;
    _BitScanForward(&bitLoc, vFlag);
    long* vDest = (long*)(tmp);
    _bittestandset(vDest, bitLoc);
}

// Get a hack flag location in an indirect manner.
template<unsigned int key> __forceinline unsigned int getHackFlag(const unsigned int& flag)
{
    unsigned int dest = ((unsigned int)(&flag) + key);
    volatile unsigned int tmp = dest;
    return *(unsigned int*)(tmp - key);
}

template<unsigned int key> __forceinline unsigned int getIndirectly(void* addr)
{
    unsigned int dest = ((unsigned int)(addr) + key);
    volatile unsigned int tmp = dest;
    return *(unsigned int*)(tmp - key);
}

#else

template<unsigned int key1> inline void setHackFlagVmp(unsigned int& y, const unsigned int x)
{
    y |= x;
}
template<unsigned int key1> inline void setHackFlagVs(unsigned int& y, const unsigned int x)
{
    y |= x;
}
template<unsigned int key> inline unsigned int getHackFlag(const unsigned int& flag)
{
    return flag;
}
#endif
}
}

// In order to spread these hackflags out in the linking phase, I'm placing these in
// a wide variety of places in .data .  Sadly, this means they can't be an array.
namespace RBX 
{ 
namespace Security
{
    extern unsigned int 
        hackFlag0, hackFlag1, hackFlag2, hackFlag3,
        hackFlag4, hackFlag5, hackFlag6, hackFlag7,
        hackFlag8, hackFlag9, hackFlag10, hackFlag11,
        hackFlag12;
}
}
