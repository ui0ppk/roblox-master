//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include "ChatManagerSettings.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

public ref class ChatPerformanceTime sealed
{
public:

    /// <summary>
    /// Minimum time value in milliseconds.
    //  Defaults to 100000.0ms until the first audio thread update
    /// </summary>
    property double MinTimeInMilliseconds { double get(); }

    /// <summary>
    /// Maximum time value in milliseconds.
    //  Defaults to 0.0ms until the first audio thread update
    /// </summary>
    property double MaxTimeInMilliseconds { double get(); }

    /// <summary>
    /// Rolling average time value in milliseconds.
    /// Value is refreshed every 500ms while the audio thread executes.
    //  Defaults to 0.0ms until the first audio thread update.
    /// </summary>
    property double AverageTimeInMilliseconds { double get(); }

internal:
    ChatPerformanceTime();

    void Update(
        _In_ double executeTimeInMilliseconds,
        _In_ double timePassedInMilliseconds
        );

private:
    Concurrency::critical_section m_stateLock;

    double m_minTimeInMilliseconds;
    double m_maxTimeInMilliseconds;
    double m_averageTimeInMilliseconds;
    double m_totalTimeInMilliseconds;
    double m_totalExecutionTimeInMilliseconds;
    long m_counter;
};

public ref class ChatPerformanceCounters sealed
{
public:

    /// <summary>
    /// ChatPerformanceTime object representing time spent capturing chat data
    /// </summary>
    property ChatPerformanceTime^ CaptureExecutionTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// ChatPerformanceTime object representing time spent sending chat data
    /// </summary>
    property ChatPerformanceTime^ SendExecutionTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// ChatPerformanceTime object representing time spent rendering incoming chat data
    /// </summary>
    property ChatPerformanceTime^ RenderExecutionTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// ChatPerformanceTime object representing time spent executing the audio worker thread
    /// </summary>
    property ChatPerformanceTime^ AudioThreadExecutionTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// ChatPerformanceTime object representing how often the audio thread wakes to do work
    /// </summary>
    property ChatPerformanceTime^ AudioThreadPeriodTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// ChatPerformanceTime object representing the time it takes to process incoming packets
    /// </summary>
    property ChatPerformanceTime^ IncomingPacketTime { ChatPerformanceTime^ get(); }

    /// <summary>
    /// Returns the bandwidth in bits per second of incoming packets
    /// </summary>
    property double IncomingPacketBandwidthBitsPerSecond { double get(); }

    /// <summary>
    /// Returns the bandwidth in bits per second of outgoing packets
    /// </summary>
    property double OutgoingPacketBandwidthBitsPerSecond { double get(); }

internal:
    ChatPerformanceCounters();

    void QueryLoopStart();
    void QueryCaptureDone();
    void QuerySendDone();
    void QueryLoopDone();
    void CalculateTimes();
    void QueryIncomingPacketStart();
    void QueryIncomingPacketDone();
    void AddPacketBandwidth( _In_ bool incomingPacket, _In_ int numberOfBytes );

private:
    void UpdatePacketBandwidth( _In_ double totalTimePassedInMilliseconds );

    static double
    GetDeltaInMilliseconds(
        _In_ const LARGE_INTEGER& startTime,
        _In_ const LARGE_INTEGER& endTime,
        _In_ const LARGE_INTEGER& freq
        );

    LARGE_INTEGER m_freq;
    LARGE_INTEGER m_loopStartTime;
    LARGE_INTEGER m_captureDoneTime;
    LARGE_INTEGER m_sendDoneTime;
    LARGE_INTEGER m_loopDoneTime;
    LARGE_INTEGER m_previousLoopStartTime;
    LARGE_INTEGER m_incomingPacketStartTime;
    LARGE_INTEGER m_incomingPacketDoneTime;

    ChatPerformanceTime^ m_captureExecutionTime;
    ChatPerformanceTime^ m_sendExecutionTime;
    ChatPerformanceTime^ m_renderExecutionTime;
    ChatPerformanceTime^ m_audioThreadExecutionTime;
    ChatPerformanceTime^ m_audioThreadPeriodTime;
    ChatPerformanceTime^ m_incomingPacketTime;

    Concurrency::critical_section m_packetBytesLock;
    int m_incomingPacketBytesIncomingCounter;
    int m_outgoingPacketBytesIncomingCounter;
    double m_packetsBytesTimeCounterInMilliseconds;
    double m_incomingPacketBandwidthBitsPerSecond;
    double m_outgoingPacketBandwidthBitsPerSecond;
};

}}}

#endif