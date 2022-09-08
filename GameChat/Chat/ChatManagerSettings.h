//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include "ChatManagerEvents.h"

#if TV_API

// Forward declare
namespace Microsoft { namespace Xbox { namespace GameChat { ref class ChatManager; } } }

namespace Microsoft {
namespace Xbox {
namespace GameChat {

/// <summary>
/// Indicates the level of debug messages send to ChatManager::OnDebugMessage
/// </summary>
public enum class GameChatDiagnosticsTraceLevel
{ 
    /// <summary>
    /// Output no tracing and debugging messages.
    /// </summary>
    Off = 0,

    /// <summary>
    /// Output error-handling messages.
    /// </summary>
    Error,

    /// <summary>
    /// Output warnings and error-handling messages.
    /// </summary>
    Warning,

    /// <summary>
    /// Output informational messages, warnings, and error-handling messages.
    /// </summary>
    Info,

    /// <summary>
    /// Output all debugging and tracing messages.
    /// </summary>
    Verbose
};

public ref class ChatManagerSettings sealed
{
public:
    /// <summary>
    /// Controls how often the audio thread wakes up in milliseconds.  
    /// A longer time causes the audio thread to process less often which causes larger capture buffers and thus larger voice packets.
    //  Defaults to 40ms.
    /// </summary>
    property uint32 AudioThreadPeriodInMilliseconds
    {
        uint32 get();
        void set(_In_ uint32 value);
    }

    /// <summary>
    /// Controls the audio thread's affinity mask.  
    /// Defaults to XAUDIO2_DEFAULT_PROCESSOR.  On XDK, this is set to Processor5 (0x10)
    ///
    /// For background on how thread affinity mask works:
    /// A thread affinity mask is a bit vector in which each bit represents a logical processor that a thread is allowed to run on. 
    /// A thread affinity mask must be a subset of the process affinity mask for the containing process of a thread. 
    /// A thread can only run on the processors its process can run on. 
    /// Therefore, the thread affinity mask cannot specify a 1 bit for a processor when the process affinity mask specifies a 0 bit for that processor.
    /// </summary>
    property uint32 AudioThreadAffinityMask
    {
        uint32 get();
        void set(_In_ uint32 value);
    }

    /// <summary>
    ///  Controls the audio thread's priority.
    ///  Defaults to THREAD_PRIORITY_TIME_CRITICAL so that the audio thread is not interrupted often
    /// </summary>
    property int AudioThreadPriority
    {
        int get();
        void set(_In_ int value);
    }

    /// <summary>
    /// The compression ratio used by the audio encoder (low, normal, high)
    /// Defaults to Normal
    /// </summary>
    property Windows::Xbox::Chat::EncodingQuality AudioEncodingQuality
    {
        Windows::Xbox::Chat::EncodingQuality get();
        void set(_In_ Windows::Xbox::Chat::EncodingQuality value);
    }

    /// <summary>
    /// Each remote capture source has a jitter buffer that contains a ring buffer.
    /// This is the max number of packets that can be stored in that ring buffer
    /// If this number is too low, then incoming packets will will be dropped
    /// If this number is too high, then memory will be wasted
    /// Defaults to 20.
    /// </summary>
    property uint32 JitterBufferMaxPackets
    {
        uint32 get();
        void set(_In_ uint32 value);
    }

    /// <summary>
    /// Each remote capture source has a jitter buffer.
    /// The jitter buffer dynamically adjusts the number of packets it needs before it hands out packets to avoid audio glitches.
    /// This value is called DynamicNeededPacketCount which can found on each ChatUser.
    /// DynamicNeededPacketCount automatically is adjusted based on internal jitter buffer heuristics.
    /// JitterBufferLowestNeededPacketCount is the lowest that DynamicNeededPacketCount can go.
    /// The lower this number is the better the latency will be with a potential trade-off of more audio glitches
    /// Defaults to 0.
    /// </summary>
    property uint32 JitterBufferLowestNeededPacketCount
    {
        uint32 get();
        void set(_In_ uint32 value);
    }

    /// <summary>
    /// Each remote capture source has a jitter buffer.
    /// The jitter buffer dynamically adjusts the number of packets it needs before it hands out packets to avoid audio glitches.
    /// This value is called DynamicNeededPacketCount which can found on each ChatUser.
    /// DynamicNeededPacketCount automatically is adjusted based on internal jitter buffer heuristics.
    /// JitterBufferPacketsBeforeRelaxingNeeded is the number of packets received while in the sweet spot 
    /// between DynamicNeededPacketCount and JitterBufferMaxPackets.
    /// When it reaches this target, the jitter buffer will lower the DynamicNeededPacketCount.
    /// DynamicNeededPacketCount will never go below JitterBufferLowestNeededPacketCount.
    /// Defaults to 5.
    /// </summary>
    property uint32 JitterBufferPacketsBeforeRelaxingNeeded
    {
        uint32 get();
        void set(_In_ uint32 value);
    }

    /// <summary>
    /// This enables or disables the chat performance counters.
    /// Defaults to false.
    /// </summary>
    property bool PerformanceCountersEnabled
    {
        bool get();
        void set(_In_ bool value);
    }

    /// <summary>
    /// This enables or disables combining mic data from multiple local users into a single
    /// packet as an optimization before the packet is sent to the OnOutgoingChatPacketReady event.  
    /// Defaults to true.  
    /// Some titles may wish to change this to false in order to precisely control
    /// which remote consoles receive the mic data of each local user.  
    /// For example, game logic could determine that local user A's mic data should be sent to 
    /// remote user C & D while local user B's mic data should be only be sent to remote user E.
    /// </summary>
    property bool CombineCaptureBuffersIntoSinglePacket
    {
        bool get();
        void set(_In_ bool value);
    }

    /// <summary>
    /// This enables or disables using Kinect as the capture source.
    /// Defaults to true.
    /// </summary>
    property bool UseKinectAsCaptureSource
    {
        bool get();
        void set(_In_ bool value);
    }

    /// <summary>
    /// This enables or disables a callback prior to encoding captured mic data.
    /// This allows titles to apply sound effects to the capture stream
    /// Defaults to false.
    /// </summary>
    property bool PreEncodeCallbackEnabled
    {
        bool get();
        void set(_In_ bool value);
    }

    /// <summary>
    /// This enables or disables a callback after to decoding remote chat voice data.
    /// This allows titles to apply sound effects to chat voice data
    /// Defaults to false.
    /// </summary>
    property bool PostDecodeCallbackEnabled
    {
        bool get();
        void set(_In_ bool value);
    }

    /// <summary>
    /// Indicates the level of debug messages send to ChatManager::OnDebugMessage
    /// Defaults to GameChatDiagnosticsTraceLevel::Info
    /// </summary>
    property GameChatDiagnosticsTraceLevel DiagnosticsTraceLevel 
    { 
        GameChatDiagnosticsTraceLevel get();
        void set(GameChatDiagnosticsTraceLevel value);
    }

    /// <summary>
    /// New chat session users will be auto muted if they have a bad reputation
    /// and are not a friend of a local user.
    /// Defaults to true.
    /// </summary>
    property bool AutoMuteBadReputationUsers
    {
        bool get();
        void set(_In_ bool value);
    }

internal:
    bool IsAtDiagnosticsTraceLevel( 
        _In_ GameChatDiagnosticsTraceLevel levelOfMessage
        );

    ChatManagerSettings(
        _In_ ChatManager^ chatManager
        );

private:
    Concurrency::critical_section m_chatSettingsStateLock;

    void TraceJitterBufferSettings();
    void TraceMiscSettings();
    void TraceEffectSettings();

    uint32 m_audioThreadPeriodInMilliseconds;
    uint32 m_audioThreadAffinityMask;
    Windows::Xbox::Chat::EncodingQuality m_audioEncodingQuality;
    uint32 m_jitterBufferMaxPackets;
    uint32 m_jitterBufferLowestNeededPacketCount;
    uint32 m_jitterBufferPacketsBeforeRelaxingNeeded;
    bool m_performanceCountersEnabled;
    Platform::WeakReference m_chatManager;
    int m_audioThreadPriority;
    bool m_combineCaptureBuffersIntoSinglePacket;
    bool m_useKinectAsCaptureSource;
    bool m_preEncodeCallbackEnabled;
    bool m_postDecodeCallbackEnabled;
    GameChatDiagnosticsTraceLevel m_gameChatDiagnosticsTraceLevel;
    bool m_autoMuteBadReputationUsers;
};

}}}

#endif