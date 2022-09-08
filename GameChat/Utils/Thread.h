//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "Clock.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

delegate void ThreadDoWorkHandler();

ref class Thread sealed
{
public:
    Thread( 
        _In_ UINT sendPeriodInMilliseconds, 
        _In_ int32 threadAffinityMask,
        _In_ int threadPriority
        );

    virtual ~Thread();

    void Shutdown();

    UINT GetWorkPeriodInMilliseconds();

    void SetWorkPeriodInMilliseconds( 
        _In_ UINT sendPeriodInMilliseconds
        );

    void SetOptions( 
        _In_ int32 threadAffinityMask,
        _In_ int threadPriority
        );

    void WakeupThread();

    event ThreadDoWorkHandler^ OnDoWork;

private:
    static DWORD WINAPI StaticThreadProc(
        _In_ Thread^ networkSendThread
        );

    DWORD ThreadProc();

private:
    CRITICAL_SECTION m_threadManagementLock;
    Clock m_clock;
    HANDLE m_terminateThreadEvent;
    HANDLE m_threadHandle;
    HANDLE m_wakeupEventHandle;
    UINT m_workPeriodInMilliseconds;
    int m_threadPriority;
    uint32 m_threadAffinityMask;
};

}}}

#endif