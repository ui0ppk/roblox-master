#pragma once

#include "rbx/rbxTime.h"
#include "RbxAssert.h"
#include "util/standardout.h"

namespace RBX
{

//
// Prints average timing after the given number of samples were taken
//
class SolverProfiler
{
public:
    SolverProfiler( int _samples, const char* _format ): maxSamples( _samples ), format( _format )
    {
        accumulator = Time::Interval::zero();
        currentSamples = 0;
    }

    void start()
    {
#ifdef ENABLE_SOLVER_PROFILER
        startTime = Time::now( Time::Precise );
#endif
    }

    void end()
    {
#ifdef ENABLE_SOLVER_PROFILER
        Time::Interval total = Time::now( Time::Precise ) - startTime;
        accumulator += total;
        currentSamples++;
#endif
    }

    void printStats()
    {
#ifdef ENABLE_SOLVER_PROFILER
        static bool enable = true;
        if( currentSamples >= maxSamples && enable )
        {
            currentSamples = 0;
            StandardOut::singleton()->printf( MESSAGE_OUTPUT, format, accumulator.msec() / maxSamples );
            accumulator = Time::Interval::zero();
        }
#endif
    }

private:
    RBX::Time startTime;
    Time::Interval accumulator;
    const char* format;
    int maxSamples;
    int currentSamples;
};

}
