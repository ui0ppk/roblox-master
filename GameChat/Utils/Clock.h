//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

class Clock
{
public:
    Clock();

    void Initialize( UINT uPeriodMS );

    void SetInterval( UINT uPeriodMS );

    void SleepUntilNextHeartbeat();

    DWORD WaitForEventsOrHeartbeat(HANDLE hObject, HANDLE hObject2);

private:
    static const UINT c_uOneSecondInMS = 1000;
    
    UINT m_heartbeats;
    LARGE_INTEGER m_timerFrequency;
    LARGE_INTEGER m_timerStart;
    LARGE_INTEGER m_countPerPeriod;
    LARGE_INTEGER GetNextHeartbeat();
};

}}}

#endif