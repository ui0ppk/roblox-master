/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

class LcmRand
{
public:
    LcmRand() : seed(1337U) {}

    uint32_t value()
    {
        const static uint32_t a = 214013U;
        const static uint32_t c = 2531011U;
        seed = seed * a + c;
        return (seed >> 16) & 0x7FFF;
    }

	void setSeed(uint32_t newSeed)
	{
		seed = newSeed;
	}

private:
    uint32_t seed;
};