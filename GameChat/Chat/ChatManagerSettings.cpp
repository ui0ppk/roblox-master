//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatManagerSettings.h"
#include "ChatAudioThread.h"
#include "ChatNetwork.h"
#include "ChatManagerEvents.h"
#include "ChatManager.h"
#include "ChatDiagnostics.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ChatManagerSettings::ChatManagerSettings(
    _In_ ChatManager^ chatManager
    ) :
    m_audioThreadPeriodInMilliseconds( 40 ), 
    m_audioThreadAffinityMask( XAUDIO2_DEFAULT_PROCESSOR ), // On XDK, this is set to Processor5 (0x10)
    m_audioEncodingQuality( Windows::Xbox::Chat::EncodingQuality::Normal ),
    m_jitterBufferMaxPackets( 20 ),
    m_jitterBufferLowestNeededPacketCount( 0 ),
    m_jitterBufferPacketsBeforeRelaxingNeeded( 5 ),
    m_performanceCountersEnabled( false ),
    m_chatManager( chatManager ),
    m_audioThreadPriority( THREAD_PRIORITY_TIME_CRITICAL ),
    m_combineCaptureBuffersIntoSinglePacket( true ),
    m_useKinectAsCaptureSource( true ),
    m_preEncodeCallbackEnabled( false ),
    m_postDecodeCallbackEnabled( false ),
    m_gameChatDiagnosticsTraceLevel( GameChatDiagnosticsTraceLevel::Info ),
    m_autoMuteBadReputationUsers( true )
{
}

uint32 ChatManagerSettings::AudioThreadPeriodInMilliseconds::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_audioThreadPeriodInMilliseconds;
}

void ChatManagerSettings::AudioThreadPeriodInMilliseconds::set( 
    _In_ uint32 value 
    )
{
    {
        Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
        m_audioThreadPeriodInMilliseconds = value;
    }
    
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        chatManager->OnChatManagerSettingsChangedHandler();
    }
}

uint32 ChatManagerSettings::AudioThreadAffinityMask::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_audioThreadAffinityMask;
}

void ChatManagerSettings::AudioThreadAffinityMask::set( 
    _In_ uint32 value 
    )
{
    {
        Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
        m_audioThreadAffinityMask = value;
    }

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        chatManager->OnChatManagerSettingsChangedHandler();
    }
}

int ChatManagerSettings::AudioThreadPriority::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_audioThreadPriority;
}

void ChatManagerSettings::AudioThreadPriority::set( 
    _In_ int value 
    )
{
    {
        Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
        m_audioThreadPriority = value;
    }

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        chatManager->OnChatManagerSettingsChangedHandler();
    }
}

Windows::Xbox::Chat::EncodingQuality ChatManagerSettings::AudioEncodingQuality::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_audioEncodingQuality;
}

void ChatManagerSettings::AudioEncodingQuality::set( 
    _In_ Windows::Xbox::Chat::EncodingQuality value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_audioEncodingQuality = value;

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatManagerSettingsAudioEncodingQuality( 
        static_cast<uint32>(m_audioEncodingQuality)
        );
#endif
}

uint32 ChatManagerSettings::JitterBufferMaxPackets::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_jitterBufferMaxPackets;
}

void ChatManagerSettings::JitterBufferMaxPackets::set( 
    _In_ uint32 value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_jitterBufferMaxPackets = value;

    TraceJitterBufferSettings();
}

uint32 ChatManagerSettings::JitterBufferLowestNeededPacketCount::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_jitterBufferLowestNeededPacketCount;
}

void ChatManagerSettings::JitterBufferLowestNeededPacketCount::set( 
    _In_ uint32 value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_jitterBufferLowestNeededPacketCount = value;

    TraceJitterBufferSettings();
}

uint32 ChatManagerSettings::JitterBufferPacketsBeforeRelaxingNeeded::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_jitterBufferPacketsBeforeRelaxingNeeded;
}

void ChatManagerSettings::JitterBufferPacketsBeforeRelaxingNeeded::set( 
    _In_ uint32 value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_jitterBufferPacketsBeforeRelaxingNeeded = value;

    TraceJitterBufferSettings();
}

bool ChatManagerSettings::PerformanceCountersEnabled::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_performanceCountersEnabled;
}

void ChatManagerSettings::PerformanceCountersEnabled::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_performanceCountersEnabled = value;

    TraceMiscSettings();
}

bool ChatManagerSettings::CombineCaptureBuffersIntoSinglePacket::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_combineCaptureBuffersIntoSinglePacket;
}

void ChatManagerSettings::CombineCaptureBuffersIntoSinglePacket::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_combineCaptureBuffersIntoSinglePacket = value;

    TraceMiscSettings();
}

bool ChatManagerSettings::UseKinectAsCaptureSource::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_useKinectAsCaptureSource;
}

void ChatManagerSettings::UseKinectAsCaptureSource::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_useKinectAsCaptureSource = value;

    TraceMiscSettings();
}

bool ChatManagerSettings::PreEncodeCallbackEnabled::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_preEncodeCallbackEnabled;
}

void ChatManagerSettings::PreEncodeCallbackEnabled::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_preEncodeCallbackEnabled = value;

    TraceEffectSettings();
}

bool ChatManagerSettings::PostDecodeCallbackEnabled::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_postDecodeCallbackEnabled;
}

void ChatManagerSettings::PostDecodeCallbackEnabled::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_postDecodeCallbackEnabled = value;

    TraceEffectSettings();
}

GameChatDiagnosticsTraceLevel ChatManagerSettings::DiagnosticsTraceLevel::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_gameChatDiagnosticsTraceLevel;
}

void ChatManagerSettings::DiagnosticsTraceLevel::set( 
    GameChatDiagnosticsTraceLevel value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_gameChatDiagnosticsTraceLevel = value;

    TraceMiscSettings();
}

bool ChatManagerSettings::AutoMuteBadReputationUsers::get()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    return m_autoMuteBadReputationUsers;
}

void ChatManagerSettings::AutoMuteBadReputationUsers::set( 
    _In_ bool value 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSettingsStateLock);
    m_autoMuteBadReputationUsers = value;

    TraceEffectSettings();
}

bool ChatManagerSettings::IsAtDiagnosticsTraceLevel( 
    _In_ GameChatDiagnosticsTraceLevel levelOfMessage 
    )
{
    GameChatDiagnosticsTraceLevel diagnosticsTraceLevel = this->DiagnosticsTraceLevel;
    return (int)diagnosticsTraceLevel >= (int)levelOfMessage;
}

void ChatManagerSettings::TraceJitterBufferSettings()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatManagerSettingsJitterBuffer(
        m_jitterBufferMaxPackets,
        m_jitterBufferLowestNeededPacketCount,
        m_jitterBufferPacketsBeforeRelaxingNeeded
        );
#endif
}

void ChatManagerSettings::TraceMiscSettings()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatManagerSettingsMisc(
        m_performanceCountersEnabled,
        m_combineCaptureBuffersIntoSinglePacket,
        m_useKinectAsCaptureSource,
        static_cast<int>(m_gameChatDiagnosticsTraceLevel)
        );
#endif
}

void ChatManagerSettings::TraceEffectSettings()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatManagerSettingsEffects(
        m_preEncodeCallbackEnabled,
        m_postDecodeCallbackEnabled
        );
#endif
}

}}}

#endif