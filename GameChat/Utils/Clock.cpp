//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "Clock.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Clock::Clock() : 
    m_heartbeats(0)
{
    m_timerFrequency.QuadPart = 0;
    m_timerStart.QuadPart = 0;
    m_countPerPeriod.QuadPart = 0;
}

void Clock::Initialize( UINT uPeriodMS )
{
    if (!QueryPerformanceFrequency(&m_timerFrequency))
    {
        assert(false); 
    }

    SetInterval(uPeriodMS);
}

void Clock::SetInterval( UINT uPeriodMS )
{
    m_countPerPeriod.QuadPart = ( m_timerFrequency.QuadPart * uPeriodMS ) / c_uOneSecondInMS;
}

void Clock::SleepUntilNextHeartbeat()
{
    LARGE_INTEGER timeOfNextHeartbeat = GetNextHeartbeat();
    LARGE_INTEGER timeNow;

    m_heartbeats++; // so that the next one will be 20ms (or whatever) later...

    if (!QueryPerformanceCounter(&timeNow))
    {
        assert(false); 
    }

    LONGLONG timeDelta = timeOfNextHeartbeat.QuadPart - timeNow.QuadPart;

    if ( timeDelta > 0 )
    {
        LONGLONG numberOfMilliSecondsSinceLast = c_uOneSecondInMS * timeDelta / m_timerFrequency.QuadPart;
        WCHAR text[200] = {0};
        swprintf_s( text, L"I Slept: %I64d", numberOfMilliSecondsSinceLast );
        OutputDebugString( text );
        DWORD dwSleepTime = static_cast<DWORD>(numberOfMilliSecondsSinceLast);
        Sleep( dwSleepTime );
    }
}

DWORD Clock::WaitForEventsOrHeartbeat( HANDLE hObject, HANDLE hObject2 )
{
    LARGE_INTEGER liExpected = GetNextHeartbeat();
    LARGE_INTEGER liNow;

    m_heartbeats++; // so that the next one will be 20ms (or whatever) later...

    if (!QueryPerformanceCounter(&liNow))
    {
        assert(false); 
    }

    LONGLONG llDiff = liExpected.QuadPart - liNow.QuadPart;

    HANDLE aObjects[2];
    aObjects[0] = hObject;
    aObjects[1] = hObject2;

    if ( llDiff > 0 )
    {
        DWORD dwSleepTime = static_cast<DWORD>(c_uOneSecondInMS * llDiff / m_timerFrequency.QuadPart);
        return ::WaitForMultipleObjectsEx( 2, aObjects, false, dwSleepTime, false );
    }

    // we're already late, no need to wait, but we still need to test the object
    return ::WaitForMultipleObjectsEx( 2, aObjects, false, 0, false );
}

LARGE_INTEGER Clock::GetNextHeartbeat()
{
    LARGE_INTEGER timeExpected;

    // Update Start
    if ( m_timerStart.QuadPart == 0 )
    {
        if (!QueryPerformanceCounter( &m_timerStart ))
        {
            assert(false); 
        }
        m_heartbeats = 1; // we are on the very first heartbeat
    }

    timeExpected.QuadPart = m_timerStart.QuadPart + (m_countPerPeriod.QuadPart * m_heartbeats);
    return timeExpected;
}

}}}

#endif