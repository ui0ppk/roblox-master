//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "Thread.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Thread::Thread( 
    _In_ UINT workPeriodInMilliseconds, 
    _In_ int32 threadAffinityMask,
    _In_ int threadPriority
    ) :
    m_threadPriority(threadPriority),
    m_workPeriodInMilliseconds(workPeriodInMilliseconds),
    m_terminateThreadEvent(nullptr),
    m_threadAffinityMask(threadAffinityMask),
    m_threadHandle(nullptr),
    m_wakeupEventHandle(nullptr)
{
    m_wakeupEventHandle = CreateEvent( nullptr, false, false, nullptr );
    InitializeCriticalSection(&m_threadManagementLock);
    m_terminateThreadEvent = CreateEvent( nullptr, false, false, nullptr );
    if ( !m_terminateThreadEvent )
    {
        throw E_UNEXPECTED;
    }

    m_threadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Thread::StaticThreadProc, (LPVOID)this, CREATE_SUSPENDED, nullptr);
    if ( !m_threadHandle )
    {
        throw HRESULT_FROM_WIN32( GetLastError() );
    }

    SetThreadPriority(m_threadHandle, m_threadPriority);

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    SetThreadAffinityMask(m_threadHandle, m_threadAffinityMask);
#endif

    ResumeThread(m_threadHandle);
}

Thread::~Thread()
{
    Shutdown();
    DeleteCriticalSection(&m_threadManagementLock);
}

UINT Thread::GetWorkPeriodInMilliseconds()
{
    return m_workPeriodInMilliseconds;
}

void Thread::SetWorkPeriodInMilliseconds( 
    _In_ UINT sendPeriodInMilliseconds
    )
{
    m_workPeriodInMilliseconds = sendPeriodInMilliseconds;
    m_clock.SetInterval(m_workPeriodInMilliseconds);
}

void Thread::SetOptions( 
    _In_ int32 threadAffinityMask,
    _In_ int threadPriority
    )
{
    m_threadPriority = threadPriority;
    m_threadAffinityMask = threadAffinityMask;

    if( m_threadHandle != nullptr )
    {
        SetThreadPriority(m_threadHandle, m_threadPriority);

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        SetThreadAffinityMask(m_threadHandle, m_threadAffinityMask);
#endif
    }
}

void Thread::Shutdown()
{
    EnterCriticalSection(&m_threadManagementLock);
    if( m_threadHandle != nullptr )
    {
        SetEvent(m_terminateThreadEvent);
        WaitForSingleObject(m_threadHandle, INFINITE);

        CloseHandle(m_threadHandle);
        m_threadHandle = nullptr;
    }
    LeaveCriticalSection(&m_threadManagementLock);
}

DWORD WINAPI Thread::StaticThreadProc( 
    _In_ Thread^ networkSendThread 
    )
{
    return networkSendThread->ThreadProc();
}

DWORD WINAPI Thread::ThreadProc()
{
    m_clock.Initialize( m_workPeriodInMilliseconds );

    static const UINT c_uOneSecondInMS = 1000;
    LARGE_INTEGER m_timerFrequency;
    QueryPerformanceFrequency(&m_timerFrequency);

    while( m_clock.WaitForEventsOrHeartbeat( m_terminateThreadEvent, m_wakeupEventHandle ) != WAIT_OBJECT_0 )
    {
        OnDoWork();
    }

    return 0;
}

void Thread::WakeupThread()
{
    SetEvent( m_wakeupEventHandle );
}

}}}

#endif