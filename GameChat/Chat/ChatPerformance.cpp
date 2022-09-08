//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatPerformance.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ChatPerformanceTime::ChatPerformanceTime() :
    m_minTimeInMilliseconds( 100000.0 ),
    m_maxTimeInMilliseconds( 0.0 ),
    m_averageTimeInMilliseconds( 0.0 ),
    m_totalTimeInMilliseconds( 0.0 ),
    m_totalExecutionTimeInMilliseconds( 0.0f ),
    m_counter( 0 )
{
}

double ChatPerformanceTime::MinTimeInMilliseconds::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_minTimeInMilliseconds;
}

double ChatPerformanceTime::MaxTimeInMilliseconds::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_maxTimeInMilliseconds;
}

double ChatPerformanceTime::AverageTimeInMilliseconds::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_averageTimeInMilliseconds;
}

void
ChatPerformanceTime::Update(
    _In_ double executeTimeInMilliseconds,
    _In_ double timePassedInMilliseconds
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_counter++;

    if (executeTimeInMilliseconds < m_minTimeInMilliseconds)
    {
        m_minTimeInMilliseconds = executeTimeInMilliseconds;
    }
    if (executeTimeInMilliseconds > m_maxTimeInMilliseconds)
    {
        m_maxTimeInMilliseconds = executeTimeInMilliseconds;
    }
    m_totalExecutionTimeInMilliseconds += executeTimeInMilliseconds;
    m_totalTimeInMilliseconds += timePassedInMilliseconds;

    // Update average every so often
    if( m_totalTimeInMilliseconds > 500 )
    {
        m_averageTimeInMilliseconds = m_totalExecutionTimeInMilliseconds / m_counter;

        m_totalTimeInMilliseconds = 0.0f;
        m_totalExecutionTimeInMilliseconds = 0.0f;
        m_counter = 0;
    }
}

ChatPerformanceCounters::ChatPerformanceCounters() :
    m_incomingPacketBandwidthBitsPerSecond( 0 ),
    m_outgoingPacketBandwidthBitsPerSecond( 0 ),
    m_incomingPacketBytesIncomingCounter( 0 ),
    m_outgoingPacketBytesIncomingCounter( 0 )
{
    m_freq.QuadPart = 0;
    m_loopStartTime.QuadPart = 0;
    m_captureDoneTime.QuadPart = 0;
    m_sendDoneTime.QuadPart = 0;
    m_loopDoneTime.QuadPart = 0;
    m_incomingPacketStartTime.QuadPart = 0;
    m_incomingPacketDoneTime.QuadPart = 0;
    m_previousLoopStartTime.QuadPart = 0;

    m_captureExecutionTime = ref new ChatPerformanceTime();
    m_sendExecutionTime = ref new ChatPerformanceTime();
    m_renderExecutionTime = ref new ChatPerformanceTime();
    m_audioThreadExecutionTime = ref new ChatPerformanceTime();
    m_audioThreadPeriodTime = ref new ChatPerformanceTime();
    m_incomingPacketTime = ref new ChatPerformanceTime();

    QueryPerformanceFrequency(&m_freq);
}

ChatPerformanceTime^ ChatPerformanceCounters::CaptureExecutionTime::get()
{
    return m_captureExecutionTime;
}

ChatPerformanceTime^ ChatPerformanceCounters::SendExecutionTime::get()
{
    return m_sendExecutionTime;
}

ChatPerformanceTime^ ChatPerformanceCounters::RenderExecutionTime::get()
{
    return m_renderExecutionTime;
}

ChatPerformanceTime^ ChatPerformanceCounters::AudioThreadExecutionTime::get()
{
    return m_audioThreadExecutionTime;
}

ChatPerformanceTime^ ChatPerformanceCounters::AudioThreadPeriodTime::get()
{
    return m_audioThreadPeriodTime;
}

ChatPerformanceTime^ ChatPerformanceCounters::IncomingPacketTime::get()
{
    return m_incomingPacketTime;
}

double ChatPerformanceCounters::OutgoingPacketBandwidthBitsPerSecond::get()
{
    Concurrency::critical_section::scoped_lock lock(m_packetBytesLock);
    return m_outgoingPacketBandwidthBitsPerSecond;
}

double ChatPerformanceCounters::IncomingPacketBandwidthBitsPerSecond::get()
{
    Concurrency::critical_section::scoped_lock lock(m_packetBytesLock);
    return m_incomingPacketBandwidthBitsPerSecond;
}

double
ChatPerformanceCounters::GetDeltaInMilliseconds(
    _In_ const LARGE_INTEGER& startTime,
    _In_ const LARGE_INTEGER& endTime,
    _In_ const LARGE_INTEGER& freq
    )
{
    double deltaInSeconds = 0;

    if( startTime.QuadPart != 0 &&
        endTime.QuadPart != 0 &&
        freq.QuadPart != 0)
    {
        LARGE_INTEGER deltaTicks;
        deltaTicks.QuadPart = endTime.QuadPart - startTime.QuadPart;
        deltaInSeconds = static_cast< double >( deltaTicks.QuadPart ) / static_cast< double >( freq.QuadPart );
    }

    return deltaInSeconds * 1000.0f;
}

void ChatPerformanceCounters::QueryLoopStart()
{
    QueryPerformanceCounter(&m_loopStartTime);
}

void ChatPerformanceCounters::QueryCaptureDone()
{
    QueryPerformanceCounter(&m_captureDoneTime);
}

void ChatPerformanceCounters::QuerySendDone()
{
    QueryPerformanceCounter(&m_sendDoneTime);
}

void ChatPerformanceCounters::QueryLoopDone()
{
    QueryPerformanceCounter(&m_loopDoneTime);
    CalculateTimes();
}

void ChatPerformanceCounters::QueryIncomingPacketStart()
{
    QueryPerformanceCounter(&m_incomingPacketStartTime);
}

void ChatPerformanceCounters::QueryIncomingPacketDone()
{
    QueryPerformanceCounter(&m_incomingPacketDoneTime);
    double incomingPacketTimeInMilliseconds = GetDeltaInMilliseconds(m_incomingPacketStartTime, m_incomingPacketDoneTime, m_freq);
    m_incomingPacketTime->Update(incomingPacketTimeInMilliseconds, m_audioThreadPeriodTime->AverageTimeInMilliseconds);
}

void ChatPerformanceCounters::AddPacketBandwidth( 
    _In_ bool incomingPacket, 
    _In_ int numberOfBytes 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_packetBytesLock);
    if( incomingPacket )
    {
        m_incomingPacketBytesIncomingCounter += numberOfBytes;
    }
    else
    {
        m_outgoingPacketBytesIncomingCounter += numberOfBytes;
    }
}

void ChatPerformanceCounters::UpdatePacketBandwidth(
    _In_ double totalTimePassedInMilliseconds
    )
{
    Concurrency::critical_section::scoped_lock lock(m_packetBytesLock);
    m_packetsBytesTimeCounterInMilliseconds += totalTimePassedInMilliseconds;
    if( m_packetsBytesTimeCounterInMilliseconds > 1000.0f )
    {
        double timeElapsedInSeconds = m_packetsBytesTimeCounterInMilliseconds / 1000.0;
        double incomingBits = static_cast<double>(m_incomingPacketBytesIncomingCounter) * 8;
        double outgoingBits = static_cast<double>(m_outgoingPacketBytesIncomingCounter) * 8;

        m_incomingPacketBandwidthBitsPerSecond = incomingBits / timeElapsedInSeconds;
        m_outgoingPacketBandwidthBitsPerSecond = outgoingBits / timeElapsedInSeconds;

        m_packetsBytesTimeCounterInMilliseconds = 0.0;
        m_incomingPacketBytesIncomingCounter = 0;
        m_outgoingPacketBytesIncomingCounter = 0;
    }
}

void ChatPerformanceCounters::CalculateTimes()
{
    if ( m_previousLoopStartTime.QuadPart != 0 )
    {
        double loopTimeInMilliseconds = GetDeltaInMilliseconds(m_loopStartTime, m_loopDoneTime, m_freq);
        double captureTimeInMilliseconds = GetDeltaInMilliseconds(m_loopStartTime, m_captureDoneTime, m_freq);
        double sendTimeInMilliseconds = GetDeltaInMilliseconds(m_captureDoneTime, m_sendDoneTime, m_freq);
        double renderTimeInMilliseconds = GetDeltaInMilliseconds(m_sendDoneTime, m_loopDoneTime, m_freq);
        double audioThreadPeriodInMilliseconds = GetDeltaInMilliseconds(m_previousLoopStartTime, m_loopStartTime, m_freq);
        double totalTimePassedInMilliseconds = audioThreadPeriodInMilliseconds;

        UpdatePacketBandwidth( totalTimePassedInMilliseconds );

        m_captureExecutionTime->Update(captureTimeInMilliseconds, totalTimePassedInMilliseconds);
        m_sendExecutionTime->Update(sendTimeInMilliseconds, totalTimePassedInMilliseconds);
        m_renderExecutionTime->Update(renderTimeInMilliseconds, totalTimePassedInMilliseconds);
        m_audioThreadExecutionTime->Update(loopTimeInMilliseconds, totalTimePassedInMilliseconds);
        if( audioThreadPeriodInMilliseconds > 10.0 )
        {
            m_audioThreadPeriodTime->Update(audioThreadPeriodInMilliseconds, totalTimePassedInMilliseconds);
        }
        else
        {
            audioThreadPeriodInMilliseconds = 0;
        }
    }

    m_previousLoopStartTime = m_loopStartTime;
}


}}}

#endif