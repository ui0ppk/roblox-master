//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "Thread.h"
#include "ChatManagerEvents.h"
#include "ChatAudioThread.h"
#include "Clock.h"
#include "StringUtils.h"
#include "BufferUtils.h"
#include "ChatClient.h"
#include "ChatUser.h"
#include "ChatNetwork.h"
#include "ChatManagerEvents.h"
#include "ChatManager.h"
#include "ChatManagerSettings.h"
#include "ChatDiagnostics.h"

#if TV_API

using namespace Windows::Xbox::Chat;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Concurrency;
using namespace Microsoft::WRL::Details;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ChatAudioThread::ChatAudioThread(
    _In_ ChatManagerSettings^ chatManagerSettings,
    _In_ ChatClient^ chatClient,
    _In_ ChatManager^ chatManager,
    _In_ std::shared_ptr<FactoryCache> factoryCache
    ) : 
m_chatManagerSettings( chatManagerSettings ),
    m_chatClient( chatClient ),
    m_hasMicFocus( true ),
    m_chatManager( chatManager ),
    m_factoryCache( factoryCache )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatManagerSettings);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatClient);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatManager);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(factoryCache);

    m_chatPerformanceCounters = ref new Microsoft::Xbox::GameChat::ChatPerformanceCounters();
    StartAudioThread();
}

void ChatAudioThread::Shutdown()
{
    CHAT_LOG_INFO_MSG(L"ChatAudioThread::Shutdown");
    ShutdownAudioThread();
}

void ChatAudioThread::SetChatNetwork( 
    _In_ ChatNetwork^ chatNetwork
    )
{
    m_chatNetwork = chatNetwork;
}

void ChatAudioThread::RemoveRemoteConsole( 
    _In_ CONSOLE_NAME localNameOfRemoteConsoleToRemove 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_remoteCaptureSourcesLock);

    for(auto iter = m_remoteCaptureSources.cbegin(); iter != m_remoteCaptureSources.cend(); )
    {
        LOOKUP_ID lookupId = iter->first;

        CONSOLE_NAME localNameOfRemoteConsole = lookupId >> 8;

        if( localNameOfRemoteConsoleToRemove == localNameOfRemoteConsole )
        {
            m_remoteCaptureSources.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }
}


void ChatAudioThread::PushRemoteCaptureAudioBuffer(
    _In_ LOOKUP_ID lookupId,
    _In_ Platform::String^ remoteCaptureSourceId, 
    _In_reads_(sourceBufferLengthInBytes) BYTE* sourceBuffer, 
    _In_ UINT sourceBufferLengthInBytes
    )
{
    std::shared_ptr<CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE> remoteCaptureSource;
    {
        std::shared_ptr< CHAT_AUDIO_THREAD_STATE > audioThreadState = GetChatSessionState();

        Concurrency::critical_section::scoped_lock lock(m_remoteCaptureSourcesLock);
        auto iter = m_remoteCaptureSources.find(lookupId);
        if (iter != m_remoteCaptureSources.end())
        {
            remoteCaptureSource = iter->second;
        }

        if ( remoteCaptureSource == nullptr && audioThreadState != nullptr )
        {
            ChatClient^ chatClient = m_chatClient.Resolve<ChatClient>();
            if( chatClient == nullptr )
            {
                // Ignore during shutdown
                return;
            }

            std::vector<ChatUser^> chatUsers = chatClient->GetChatUsersForCaptureSourceId(remoteCaptureSourceId);
            if( chatUsers.size() == 0 )
            {
                // Ignore chat packets from users who aren't yet added.  
                // Since processing a remote user is async, a few chat voice packets may come in before it is done so skip those
                return;
            }

            // Check if we already have a decoder for this remote capture source.  
            // If not, create one
            ChatDecoder^ chatDecoder = ref new ChatDecoder();
            bool isKinect = (wcsstr(remoteCaptureSourceId->Data(), KINECTDESCRIPTOR) != nullptr);
            ChatUserTalkingMode talkingMode = isKinect ? ChatUserTalkingMode::TalkingOverKinect : ChatUserTalkingMode::TalkingOverHeadset;

            Platform::Collections::Vector<ChatUser^>^ chatUsersVector = ref new Platform::Collections::Vector<ChatUser^>(chatUsers);
            Windows::Foundation::Collections::IVectorView<ChatUser^>^ chatUsersView = chatUsersVector->GetView();

            remoteCaptureSource.reset( new CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE() );
            remoteCaptureSource->captureSourceId = remoteCaptureSourceId;
            remoteCaptureSource->chatDecoder = chatDecoder;
            remoteCaptureSource->talkingMode = talkingMode;
            remoteCaptureSource->chatUsers = chatUsersView;

            UINT maxBytesInPeriod = (UINT)(m_chatManagerSettings->AudioThreadPeriodInMilliseconds / 20.0f) * 256; // max of 256 bytes per 20ms
            UINT maxPacketsInRingBuffer = m_chatManagerSettings->JitterBufferMaxPackets;
            UINT lowestNeededPacketCount = m_chatManagerSettings->JitterBufferLowestNeededPacketCount;
            UINT packetsBeforeRelaxingNeeded = m_chatManagerSettings->JitterBufferPacketsBeforeRelaxingNeeded;
            remoteCaptureSource->jitterBuffer.reset( new JitterBuffer(
                maxBytesInPeriod,
                maxPacketsInRingBuffer,
                lowestNeededPacketCount,
                packetsBeforeRelaxingNeeded,
                m_factoryCache
                ) );
            m_remoteCaptureSources[lookupId] = remoteCaptureSource;

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
            TraceChatRemoteCaptureSource( 
                chatUsersView->Size > 0 ? chatUsersView->GetAt(0)->XboxUserId->Data() : L"n/a",
                remoteCaptureSourceId->Data(),
                lookupId,
                chatUsersView->Size
                );
#endif
        }
    }

    if( remoteCaptureSource != nullptr )
    {
        Concurrency::critical_section::scoped_lock lock(remoteCaptureSource->jitterBufferLock);
        remoteCaptureSource->jitterBuffer->Push(
            sourceBuffer, 
            sourceBufferLengthInBytes
            );
    }
}

bool ChatAudioThread::HasMicFocus::get()
{
    return m_hasMicFocus;
}

void ChatAudioThread::HasMicFocus::set( 
    bool val 
    )
{
    m_hasMicFocus = val;
}

void ChatAudioThread::StartAudioThread()
{
    if ( m_audioThread == nullptr )
    {
        m_audioThread = ref new Thread(
            m_chatManagerSettings->AudioThreadPeriodInMilliseconds, 
            m_chatManagerSettings->AudioThreadAffinityMask, 
            m_chatManagerSettings->AudioThreadPriority
            ); 
        Platform::WeakReference wr(this);
        m_audioThread->OnDoWork += ref new ThreadDoWorkHandler( [wr]()
        {
            ChatAudioThread^ audioThread = wr.Resolve<ChatAudioThread>();
            if( audioThread != nullptr )
            {
                audioThread->AudioThreadDoWork();
            }
        });
    }
}

void ChatAudioThread::ShutdownAudioThread()
{
    // Need to terminate the processing thread.
    if ( m_audioThread != nullptr )
    {
        m_audioThread->Shutdown();
        m_audioThread = nullptr;
    }
}

void ChatAudioThread::AudioThreadDoWork()
{
    std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState = GetChatSessionState();
    if (chatAudioThreadState == nullptr)
    {
        return;
    }

    bool perfEnabled = chatAudioThreadState->chatPerformanceCountersEnabled;
    if( perfEnabled ) { m_chatPerformanceCounters->QueryLoopStart(); }

    ChatClient^ chatClient = m_chatClient.Resolve<ChatClient>();
    ChatNetwork^ chatNetwork = m_chatNetwork.Resolve<ChatNetwork>();

    if (chatClient != nullptr && chatNetwork != nullptr)
    {
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatAudioThreadState->allChatUsers)
        {
            chatUser->SetTalkingMode(ChatUserTalkingMode::NotTalking);
        }

        // Capture
        CaptureDataFromLocalCaptureSources( chatAudioThreadState );
        if( perfEnabled ) { m_chatPerformanceCounters->QueryCaptureDone(); }

        // Send
        // Now that we have captured the data, send out the network packets
        chatNetwork->CreateChatVoicePackets( chatAudioThreadState );
        if( perfEnabled ) { m_chatPerformanceCounters->QuerySendDone(); }

        // Render
        // Play any network packets that have come in
        RenderAudioToAllRenderTargets( chatAudioThreadState );
        if( perfEnabled ) { m_chatPerformanceCounters->QueryLoopDone(); }
    }
}


void ChatAudioThread::LogCommentFormat(
    _In_ LPCWSTR strMsg, ...
    )
{
    va_list args;
    va_start(args, strMsg);
    LogComment(StringUtils::GetStringFormat(strMsg, args));
    va_end(args);
}


void ChatAudioThread::SetChatSessionState(
    _In_ IChatSessionState^ chatSessionState,
    _In_ ChatClient^ chatClient
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( chatSessionState );
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( chatClient );

    // Using the new IChatSessionState, create a CHAT_AUDIO_THREAD_STATE
    // which precomputes everything needed for the audio thread
    std::shared_ptr<CHAT_AUDIO_THREAD_STATE> audioThreadState( new CHAT_AUDIO_THREAD_STATE() );
    audioThreadState->chatSessionState = chatSessionState;

    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ allChatUsers = chatClient->GetChatUsers();
    audioThreadState->allChatUsers = allChatUsers;

    for ( UINT i = 0; i < chatSessionState->CaptureSources->Size; ++i )
    {
        IChatCaptureSource^ chatCaptureSource = chatSessionState->CaptureSources->GetAt( i );

        ChatNetwork^ chatNetwork = m_chatNetwork.Resolve<ChatNetwork>();
        if( chatNetwork == nullptr)
        {
            // Ignore during shutdown
            return;
        }

        audioThreadState->preEncodeCallbackEnabled = m_chatManagerSettings->PreEncodeCallbackEnabled;
        audioThreadState->postDecodeCallbackEnabled = m_chatManagerSettings->PostDecodeCallbackEnabled;

        bool isKinect = (wcsstr(chatCaptureSource->Id->Data(), KINECTDESCRIPTOR) != nullptr);
        if ( isKinect &&
            m_chatManagerSettings->UseKinectAsCaptureSource == false )
        {
            continue;
        }

        ChatEncoder^ chatEncoder = ref new ChatEncoder( chatCaptureSource->Format, m_chatManagerSettings->AudioEncodingQuality );
        ChatUserTalkingMode talkingMode = isKinect ? ChatUserTalkingMode::TalkingOverKinect : ChatUserTalkingMode::TalkingOverHeadset;
        std::vector<ChatUser^> chatUsers = chatClient->GetChatUsersForCaptureSourceId(chatCaptureSource->Id);

        Platform::Collections::Vector<ChatUser^>^ chatUsersVector = ref new Platform::Collections::Vector<ChatUser^>(chatUsers);
        if( m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Info) )
        {
            LogCommentFormat( L"SetChatSessionState: Size: %d chatCaptureSourceId: %s", chatUsersVector->Size, chatCaptureSource->Id->Data() );
            for each (ChatUser^ user in chatUsersVector)
            {
                LogCommentFormat( L"SetChatSessionState: ChatUser: 0x%0.8x XUID: %s UserId: 0x%0.8x", user, user->XboxUserId->Data(), user->User->Id );
            }
        }

        Windows::Foundation::Collections::IVectorView<ChatUser^>^ chatUsersView = chatUsersVector->GetView();

        DEVICE_ID captureSourceDeviceId = chatNetwork->GetAudioDeviceIDMapper()->GetLocalDeviceID( chatCaptureSource->Id );

        std::shared_ptr<CHAT_AUDIO_THREAD_CAPTURE_SOURCE> chatAudioThreadCaptureSource( new CHAT_AUDIO_THREAD_CAPTURE_SOURCE() );
        chatAudioThreadCaptureSource->chatCaptureSource = chatCaptureSource;
        chatAudioThreadCaptureSource->audioFormat = chatCaptureSource->Format;
        chatAudioThreadCaptureSource->chatEncoder = chatEncoder;
        chatAudioThreadCaptureSource->talkingMode = talkingMode;
        chatAudioThreadCaptureSource->chatUsers = chatUsersView;
        chatAudioThreadCaptureSource->captureSourceDeviceId = captureSourceDeviceId;

        audioThreadState->captureSources.push_back( chatAudioThreadCaptureSource );

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        ChatUser^ firstChatUser = chatUsersView->Size > 0 ? chatUsersView->GetAt(0) : nullptr;
        TraceChatLocalCaptureSource( 
            firstChatUser != nullptr ? firstChatUser->XboxUserId->Data() : L"n/a",
            chatCaptureSource->Id->Data(),
            captureSourceDeviceId,
            ConvertChatUserTalkingModeToString( chatAudioThreadCaptureSource->talkingMode )->Data()
            );
#endif
    }

    for ( UINT i = 0; i < chatSessionState->RenderTargets->Size; ++i )
    {
        IChatRenderTarget^ chatRenderTarget = chatSessionState->RenderTargets->GetAt( i );

        std::shared_ptr<CHAT_AUDIO_THREAD_RENDER_TARGET> chatAudioThreadRenderTarget( new CHAT_AUDIO_THREAD_RENDER_TARGET() );
        chatAudioThreadRenderTarget->chatRenderTarget = chatRenderTarget;

        Platform::String^ renderTargetId = chatRenderTarget->Id;

        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in allChatUsers)
        {
            if( !chatUser->IsLocal )
            {
                continue;
            }

            auto audioDevices = chatUser->User->AudioDevices;
            for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDevice in audioDevices)
            {
                if( StringUtils::IsStringEqualCaseInsenstive(audioDevice->Id, renderTargetId) )
                {
                    chatUser->SetChatRenderTarget( chatRenderTarget );
                    break; // Stop processing this user but keep looking for other users with this ID
                }
            }
        }

        audioThreadState->renderTargets.push_back( chatAudioThreadRenderTarget );
    }

    audioThreadState->chatPerformanceCountersEnabled = m_chatManagerSettings->PerformanceCountersEnabled;

    {
        Concurrency::critical_section::scoped_lock lock(m_audioThreadStateLock);
        m_audioThreadState = audioThreadState;
    }
}

Platform::String^ ChatAudioThread::ConvertChatUserTalkingModeToString(
    _In_ ChatUserTalkingMode chatUserTalkingMode 
    )
{
    switch (chatUserTalkingMode)
    {
    case ChatUserTalkingMode::NotTalking: return L"NotTalking";
    case ChatUserTalkingMode::TalkingOverHeadset: return L"TalkingOverHeadset";
    case ChatUserTalkingMode::TalkingOverKinect: return L"TalkingOverKinect";
    }

    return L"Unknown";
}

std::shared_ptr< CHAT_AUDIO_THREAD_STATE > ChatAudioThread::GetChatSessionState()
{
    Concurrency::critical_section::scoped_lock lock(m_audioThreadStateLock);
    return m_audioThreadState;
}

void ChatAudioThread::CaptureDataFromLocalCaptureSources( 
    _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState 
    )
{
    for each (std::shared_ptr<CHAT_AUDIO_THREAD_CAPTURE_SOURCE> chatAudioThreadCaptureSource in chatAudioThreadState->captureSources)
    {
        IBuffer^ captureBuffer = nullptr;
        Windows::Xbox::Chat::CaptureBufferStatus status = chatAudioThreadCaptureSource->chatCaptureSource->GetNextBuffer( &captureBuffer );

        switch (status)
        {
        case Windows::Xbox::Chat::CaptureBufferStatus::Filled: __fallthrough;
        case Windows::Xbox::Chat::CaptureBufferStatus::Incomplete:
            {
                m_hasMicFocus = true;

                IBuffer^ bufferToEncode;
                if( chatAudioThreadState->preEncodeCallbackEnabled )
                {
                    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
                    if( chatManager != nullptr )
                    {
                        bufferToEncode = chatManager->OnPreEncodeAudioBufferHandler(
                            captureBuffer, 
                            chatAudioThreadCaptureSource->audioFormat, 
                            chatAudioThreadCaptureSource->chatUsers
                            );
                        if( bufferToEncode == nullptr )
                        {
                            // Skip this capture source
                            continue;
                        }
                    }
                    else
                    {
                        bufferToEncode = captureBuffer;
                    }
                }
                else
                {
                    bufferToEncode = captureBuffer;
                }

                IBuffer^ encodedBuffer = nullptr;
                chatAudioThreadCaptureSource->chatEncoder->Encode( bufferToEncode, &encodedBuffer );

                // The encoder can return a zero length buffer.
                if ( encodedBuffer != nullptr && encodedBuffer->Length > 0 )
                {
                    // Encoder or Capture buffers need to be copied as they will
                    // not hang around more than until the next pass.

                    IBuffer^ destBuffer = BufferUtils::BufferCopy( encodedBuffer, m_factoryCache->GetBufferFactory() );

                    bool isCaptureSourceMuted = false;
                    for each (ChatUser^ user in chatAudioThreadCaptureSource->chatUsers)
                    {
                        if( user->IsLocalUserMuted )
                        {
                            isCaptureSourceMuted = true;
                        }
                    }

                    if( !isCaptureSourceMuted )
                    {
                        chatAudioThreadCaptureSource->audioBufferQueue.push( destBuffer );

                        for each (ChatUser^ user in chatAudioThreadCaptureSource->chatUsers)
                        {
                            user->SetTalkingMode( chatAudioThreadCaptureSource->talkingMode );
                        }
                    }
                }
            }
            break;

        case Windows::Xbox::Chat::CaptureBufferStatus::NoMicrophoneFocus:
            {
                // At least one source does not have microphone focus. 
                // For simplicity, all rendering will stop until we regain focus. 
                m_hasMicFocus = false;
                ClearChatSessionState();

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
                TraceChatMicFocus( m_hasMicFocus );
#endif

                // Because we have cleared the session, we should return until
                // a state change brings us an updated session state.
                return;
            }
            break;

        case Windows::Xbox::Chat::CaptureBufferStatus::NotTalking:
            {
                if (chatAudioThreadCaptureSource->chatEncoder->IsDataInFlight)
                {
                    IBuffer^ encodedBuffer = nullptr;
                    chatAudioThreadCaptureSource->chatEncoder->Encode( nullptr, &encodedBuffer );

                    // The encoder can return a zero length buffer.
                    if ( encodedBuffer != nullptr && encodedBuffer->Length > 0 )
                    {
                        // Encoder or Capture buffers need to be copied as they will
                        // not hang around more than until the next pass.

                        IBuffer^ destBuffer = BufferUtils::BufferCopy( encodedBuffer, m_factoryCache->GetBufferFactory() );

                        bool isCaptureSourceMuted = false;
                        for each (ChatUser^ user in chatAudioThreadCaptureSource->chatUsers)
                        {
                            if( user->IsLocalUserMuted )
                            {
                                isCaptureSourceMuted = true;
                            }
                        }

                        if( !isCaptureSourceMuted )
                        {
                            chatAudioThreadCaptureSource->audioBufferQueue.push( destBuffer );

                            for each (ChatUser^ user in chatAudioThreadCaptureSource->chatUsers)
                            {
                                user->SetTalkingMode( chatAudioThreadCaptureSource->talkingMode );
                            }
                        }
                    }
                }
            }
        default:
            // Nothing needs to be done
            break;
        }
    }
}

void ChatAudioThread::RenderAudioToAllRenderTargets( 
    _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState
    )
{
    IChatSessionState^ chatSessionState = chatAudioThreadState->chatSessionState;

    std::map< LOOKUP_ID, std::shared_ptr<CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE> > remoteCaptureSourcesCopy;
    {
        Concurrency::critical_section::scoped_lock lock(m_remoteCaptureSourcesLock);
        remoteCaptureSourcesCopy = m_remoteCaptureSources;
    }

    // Nothing to do if we haven't yet got data from any remote capture sources
    if( remoteCaptureSourcesCopy.size() == 0 )
    {
        return;
    }

    IBuffer^ decodedBuffer;
    for each (std::shared_ptr<CHAT_AUDIO_THREAD_RENDER_TARGET> chatAudioThreadRenderTarget in chatAudioThreadState->renderTargets)
    {
        IChatRenderTarget^ target = chatAudioThreadRenderTarget->chatRenderTarget;
        try
        {
            target->BeginMix();

            for each ( auto it in remoteCaptureSourcesCopy)
            {
                std::shared_ptr<CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE> remoteCaptureSource = it.second;
                Concurrency::critical_section::scoped_lock lock(remoteCaptureSource->jitterBufferLock);

                int currentPacketCount = remoteCaptureSource->jitterBuffer->GetCurrentPacketCount();
                if ( !(remoteCaptureSource->chatDecoder->IsDataInFlight) && currentPacketCount == 0 )
                {
                    // Skip this remote capture source if there's no data from this remote capture source
                    continue;
                }

                // If data might be in flight (if hardware encoding is being used), pull out the last packet by calling decode with nullptr
                IBuffer^ queuedBuffer = nullptr;
                HRESULT hr = S_OK;
                if (currentPacketCount > 0)
                {
                    hr = remoteCaptureSource->jitterBuffer->GetFront( &queuedBuffer );
                }

                if( SUCCEEDED(hr) )
                {
                    try
                    {
                        if (remoteCaptureSource->cachedDecodedBuffer == nullptr)
                        {
                            remoteCaptureSource->chatDecoder->Decode( queuedBuffer, &decodedBuffer ); // hang on to the decoded buffer reference until after SubmitMix() returns 
                            remoteCaptureSource->cachedDecodedBuffer = decodedBuffer;
                        }

                        decodedBuffer = remoteCaptureSource->cachedDecodedBuffer;

                        if (decodedBuffer != nullptr)
                        {
                            IBuffer^ mixBuffer = nullptr;
                            if( chatAudioThreadState->postDecodeCallbackEnabled )
                            {
                                ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
                                if( chatManager != nullptr )
                                {
                                    mixBuffer = chatManager->OnPostDecodeAudioBufferHandler(
                                        decodedBuffer, 
                                        remoteCaptureSource->chatDecoder->Format, 
                                        remoteCaptureSource->chatUsers
                                        );
                                    if( mixBuffer == nullptr )
                                    {
                                        // Skip this remote capture source
                                        continue;
                                    }
                                    remoteCaptureSource->postDecodeAudioBuffer = mixBuffer; // hang on to the reference to the new mix buffer until after SubmitMix() returns
                                }
                                else
                                {
                                    mixBuffer = decodedBuffer;
                                }
                            }
                            else
                            {
                                mixBuffer = decodedBuffer;
                            }

                            ChatRestriction restriction = target->AddMixBuffer( 
                                remoteCaptureSource->captureSourceId, 
                                remoteCaptureSource->chatDecoder->Format, 
                                mixBuffer
                                );

                            // Take update any users who are associated with this remote capture source
                            for each (ChatUser^ user in remoteCaptureSource->chatUsers)
                            {
                                user->SetTalkingMode( remoteCaptureSource->talkingMode );
                                user->SetRestrictionMode( restriction );
                            }
                        }
                    }
                    catch( Platform::Exception^ )
                    {
                        // Skip buffer if Decode or AddMixBuffer throws error
                    }
                }
            }

            target->SubmitMix();
        }
        catch( Platform::Exception^ )
        {
            try
            {
                target->ResetMix();
            }
            catch (Platform::Exception^ ex)
            {
                if (ex->HResult == (HRESULT)Windows::Xbox::Chat::ChatErrorStatus::RenderGraphError)
                {
                    ChatClient^ chatClient = m_chatClient.Resolve<ChatClient>();
                    if( chatClient != nullptr )
                    {
                        chatClient->UpdateSessionState(); 
                    }
                }

            }
        }
    }

    for each ( auto it in remoteCaptureSourcesCopy )
    {
        std::shared_ptr<CHAT_AUDIO_THREAD_REMOTE_CAPTURE_SOURCE> remoteCaptureSource = it.second;
        Concurrency::critical_section::scoped_lock lock(remoteCaptureSource->jitterBufferLock);

        // Clear out all saved decoded buffers
        remoteCaptureSource->cachedDecodedBuffer = nullptr;

        if ( remoteCaptureSource->jitterBuffer->GetCurrentPacketCount() != 0 )
        {
            // Pop the first buffer that we've just rendered to all render targets.
            remoteCaptureSource->jitterBuffer->Pop();
        }

        // For UI & debugging only: This shows the number of pending audio packets that the render queue still has.
        for each (ChatUser^ user in remoteCaptureSource->chatUsers)
        {
            user->SetNumberOfPendingAudioPacketsToPlay( (uint32)remoteCaptureSource->jitterBuffer->GetCurrentPacketCount() );
            user->SetDynamicNeededPacketCount( (uint32)remoteCaptureSource->jitterBuffer->GetDynamicNeededPacketCount() );
        }
    }
}

void ChatAudioThread::ClearChatSessionState()
{
    // Need to drain the render and capture queues on session state changes.
    {
        Concurrency::critical_section::scoped_lock lock(m_audioThreadStateLock);
        m_audioThreadState.reset();
    }

    {
        Concurrency::critical_section::scoped_lock lock(m_remoteCaptureSourcesLock);
        m_remoteCaptureSources.clear();
    }
}

void ChatAudioThread::OnChatManagerSettingsChangedHandler()
{
    if( m_audioThread )
    {
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        TraceChatManagerSettingsAudioThread( 
            m_chatManagerSettings->AudioThreadPeriodInMilliseconds, 
            m_chatManagerSettings->AudioThreadAffinityMask, 
            m_chatManagerSettings->AudioThreadPriority 
            );
#endif

        m_audioThread->SetWorkPeriodInMilliseconds( m_chatManagerSettings->AudioThreadPeriodInMilliseconds );
        m_audioThread->SetOptions( m_chatManagerSettings->AudioThreadAffinityMask, m_chatManagerSettings->AudioThreadPriority );
    }
}

void ChatAudioThread::LogComment(
    _In_ Platform::String^ message
    )
{
    LogCommentWithError(message, S_OK);
}

void ChatAudioThread::LogCommentWithError(
    _In_ Platform::String^ message, 
    _In_ HRESULT hr
    )
{
    DebugMessageEventArgs^ args = ref new DebugMessageEventArgs(message, hr);
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        chatManager->OnDebugMessageHandler( args );
    }
}

ChatPerformanceCounters^ ChatAudioThread::ChatPerformanceCounters::get()
{
    return m_chatPerformanceCounters;
}

}}}

#endif