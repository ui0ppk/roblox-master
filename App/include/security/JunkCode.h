#pragma once

#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
#include "Security/RandomConstant.h"

// This is junk code generation.  All of this is intended to be inlined
// and generally do nothing.

// there are 15*17 different seeds from this.
// (RBX_BUILDSEED%15 + 1) is between 1 and 16, and is coprime to 17.
#define RBX_JUNK (junk< ((RBX_BUILDSEED%15 + 1)*__LINE__ + RBX_BUILDSEED) % 17>())

template <int N> inline void junk() {}

#define RBX_NOP0 __asm _emit 0x0f __asm _emit 0x1f __asm _emit 0x00
#define RBX_NOP1 __asm _emit 0x8d __asm _emit 0x76 __asm _emit 0x00
#define RBX_NOP2 __asm _emit 0x0f __asm _emit 0x1f __asm _emit 0x40 __asm _emit 0x00
#define RBX_NOP3 __asm _emit 0x8d __asm _emit 0x74 __asm _emit 0x26 __asm _emit 0x00
#define RBX_NOP4 __asm _emit 0x90 __asm _emit 0x8d __asm _emit 0x74 __asm _emit 0x26 __asm _emit 0x00
#define RBX_NOP5 __asm _emit 0x66 __asm _emit 0x0f __asm _emit 0x1f __asm _emit 0x44 __asm _emit 0x00 __asm _emit 0x00
#define RBX_NOP6 __asm _emit 0x8d __asm _emit 0xb6 __asm _emit 0x00 __asm _emit 0x00 __asm _emit 0x00 __asm _emit 0x00
template<> inline void junk<0>()
{
    _asm
    {
        RBX_NOP0;
    }
}


template<> inline void junk<1>()
{
    _asm
    {
        RBX_NOP1;
    }
}

template<> inline void junk<2>()
{
    _asm
    {
        RBX_NOP2;
    }
}

template<> inline void junk<3>()
{
    _asm
    {
        RBX_NOP3;
    }
}

template<> inline void junk<4>()
{
    _asm
    {
        RBX_NOP4;
    }
}

template<> inline void junk<5>()
{
    _asm
    {
        RBX_NOP5;
    }
}

template<> inline void junk<6>()
{
    _asm
    {
        RBX_NOP6;
    }
}

template<> inline void junk<7>()
{
    _asm
    {
        RBX_NOP0;
        RBX_NOP1;
    }
}

template<> inline void junk<8>()
{
    _asm
    {
        RBX_NOP1;
        RBX_NOP2;
    }
}

template<> inline void junk<9>()
{
    _asm
    {
        RBX_NOP1;
        RBX_NOP3;   
    }
}

template<> inline void junk<10>()
{
    _asm
    {
        RBX_NOP4;
        RBX_NOP0;
    }
}

template<> inline void junk<11>()
{
    _asm
    {
        RBX_NOP3;
        RBX_NOP5;
    }
}

template<> inline void junk<12>()
{
    _asm
    {
        RBX_NOP2;
        RBX_NOP5;
    }
}

template<> inline void junk<13>()
{
    _asm
    {
        RBX_NOP6;
        RBX_NOP0;
    }
}

template<> inline void junk<14>()
{
    _asm
    {
        RBX_NOP2;
        RBX_NOP1;
    }
}

template<> inline void junk<15>()
{
    _asm
    {
        RBX_NOP4;
        RBX_NOP5;
    }
}

template<> inline void junk<16>()
{
    _asm
    {
        RBX_NOP3;
        RBX_NOP2;
    }
}
#else
#define RBX_JUNK
#endif
