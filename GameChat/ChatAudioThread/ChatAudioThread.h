//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatUser.h"
#include "Thread.h"
#include "ChatManagerSettings.h"
#include "ChatClient.h"
#include "AudioDeviceIDMapper.h"
#include "JitterBuffer.h"
#include "ChatPerformance.h"
#include "FactoryCache.h"

#if TV_API


// Forward declare
namespace Microsoft { namespace Xbox { namespace GameChat { ref class ChatNetwork; } } }

namespace Microsoft {
namespace Xbox {
namespace GameChat {

#define KINECTDESCRIPTOR L"postmec"

struct CHAT_AUDIO_THREAD_CAPTURE_SOURCE
{
    Windows::Xbox::Chat::IChatCaptureSource^ chatCaptureSource;
    Windows::Xbox::Chat::ChatEncoder^ chatEncoder;
    ChatUserTalkingMode talkingMode; 
    Windows::Foundation::Collections::IVectorView<ChatUser^>^ chatUsers;
    std::queue<Windows::Storage::Streams::IBuffer^> audioBufferQueue;
    DEVICE_ID captureSourceDeviceId;
    Windows::Xbox::Chat::IFormat^ audioFormat;
};

struct CHAT_AUDIO_THREAD_RENDER_TARGET
{
    Windows::Xbox::Chat::IChatRenderTarget^ chatRenderTarget;
};

struct CHAT_AUDIO_THREAD_STATE
{
    Windows::Xbox::Chat::IChatSessionState^ chatSessionState;
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ allChatUsers;
    std::vector< std::shared_ptr<CHAT_AUDIO_THREAD_CAPTURE_SOURCE> > captureSources;
    std::vector< std::shared_ptr<CHAT_AUDIO_THREAD_RENDER_TARGET> > renderTargets;
    bool chatPerformanceCountersEnabled;
    bool preEncodeCallbackEnabled;
    bool postDecodeCallbackEnabled;
};

struct CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE
{
    Platform::String^ captureSourceId;
    Windows::Xbox::Chat::ChatDecoder^ chatDecoder;
    Windows::Storage::Streams::IBuffer^ cachedDecodedBuffer;
    std::shared_ptr<IJitterBuffer> jitterBuffer;
    Concurrency::critical_section jitterBufferLock;
    ChatUserTalkingMode talkingMode; 
    Windows::Foundation::Collections::IVectorView<ChatUser^>^ chatUsers;
    Windows::Storage::Streams::IBuffer^ postDecodeAudioBuffer;
};

/// <summary>
/// This class handles a real-time thread pumped at a specific frequency to capture chat
/// packets to push to a network queue and poll an incoming queue for packets to render.
/// </summary>
ref class ChatAudioThread sealed
{
internal:
    ChatAudioThread(
        _In_ ChatManagerSettings^ chatManagerSettings,
        _In_ ChatClient^ chatClient,
        _In_ ChatManager^ chatManager,
        _In_ std::shared_ptr<FactoryCache> factoryCache
        );

    void Shutdown();

    void SetChatNetwork( 
        _In_ ChatNetwork^ chatNetwork 
        );

    /// <summary>
    /// Indicates if the the title has mic focus
    /// </summary>
    property bool HasMicFocus 
    { 
        bool get(); 
        void set(bool val);
    }

    /// <summary>
    /// Push a chat packet that has been received from the network transport layer
    /// to the render buffer
    /// </summary>
    void PushRemoteCaptureAudioBuffer( 
        _In_ LOOKUP_ID lookupId,
        _In_ Platform::String^ captureSourceId, 
        _In_reads_(sourceBufferLengthInBytes) BYTE* sourceBuffer, 
        _In_ UINT sourceBufferLengthInBytes
        );

    /// <summary>
    /// Updates the chat session after a ChatParticipant has been added or removed.  
    /// This clears the capture and render queues so we don't send or render packets 
    /// without a user attached, as well as potentially shutting down the processing
    /// thread if there are no more active ChatParticipants
    /// </summary>
    void UpdateSessionState();

    void SetChatSessionState( 
        _In_ Windows::Xbox::Chat::IChatSessionState^ chatSessionState,
        _In_ ChatClient^ chatClient
        );

    void ClearChatSessionState();

    void OnChatManagerSettingsChangedHandler();


    property Microsoft::Xbox::GameChat::ChatPerformanceCounters^ ChatPerformanceCounters
    {
        Microsoft::Xbox::GameChat::ChatPerformanceCounters^ get();
    }

    void RemoveRemoteConsole(
        _In_ CONSOLE_NAME localNameOfRemoteConsole 
        );

private:
    std::shared_ptr< CHAT_AUDIO_THREAD_STATE > GetChatSessionState();

    void LogCommentFormat( 
        _In_ LPCWSTR strMsg, ... 
        );

    /// <summary>
    /// Starts the processing thread
    /// </summary>
    void StartAudioThread();

    /// <summary>
    /// Terminates the processing thread
    /// </summary>
    void ShutdownAudioThread();

    /// <summary>
    // This is called periodically by the audio thread execution loop 
    // This is the workhorse component of the audio thread, rendering
    // chat packets from the incoming rendering queue and capturing new packets from
    // audio hardware and pushing them to the capture queue.
    /// </summary>
    void AudioThreadDoWork();

    /// <summary>
    /// Render all audio from each CaptureSource to all RenderTargets.  The systems will handle
    /// whether a render target should or should not render audio from a particular target.  The
    /// overhead of sending capture data to a render target that isn't rendered is trivial.
    /// Audio sent to a render target may not be rendered if the source/target relationship
    /// doesn't allow it for some reason, such as the render target being associated solely with
    /// a child account.
    /// </summary>
    void RenderAudioToAllRenderTargets( _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState );

    void CaptureDataFromLocalCaptureSources( _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState );

    /// <summary>
    /// Pushes an incoming chat buffer to the tail of the capture buffer queue
    /// </summary>
    void PushCaptureBuffer(
        _In_ Platform::String^ captureSourceId, 
        _In_ Windows::Storage::Streams::IBuffer^ buffer
        );

    void LogComment(
        _In_ Platform::String^ message
        );

    void LogCommentWithError(
        _In_ Platform::String^ message, 
        _In_ HRESULT hr
        );

    Platform::String^ ConvertChatUserTalkingModeToString(
        _In_ ChatUserTalkingMode chatUserTalkingMode 
        );

private:
    std::map< LOOKUP_ID, std::shared_ptr<CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE> > m_remoteCaptureSources;
    std::shared_ptr<FactoryCache> m_factoryCache;

    Thread^ m_audioThread;
    Platform::WeakReference m_chatClient;
    Platform::WeakReference m_chatNetwork;
    ChatManagerSettings^ m_chatManagerSettings;
    Platform::WeakReference m_chatManager;

    Concurrency::critical_section m_audioThreadStateLock;
    Concurrency::critical_section m_remoteCaptureSourcesLock;
    std::shared_ptr< CHAT_AUDIO_THREAD_STATE > m_audioThreadState;

    bool m_hasMicFocus;

    Windows::Foundation::EventRegistrationToken m_tokenOnChatManagerSettingsChanged;
    Microsoft::Xbox::GameChat::ChatPerformanceCounters^ m_chatPerformanceCounters;
};

}}}

#endif