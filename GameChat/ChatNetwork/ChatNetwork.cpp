//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "StringUtils.h"
#include "ChatManagerEvents.h"
#include "ChatPacker.h"
#include "ChatUnPacker.h"
#include "AudioDeviceInfo.h"
#include "RemoteChatUser.h"
#include "RemoteAudioDevice.h"
#include "AudioDeviceIDMapper.h"
#include "ChatAudioThread.h"
#include "ChatClient.h"
#include "ChatNetwork.h"
#include "BufferUtils.h"
#include "ChatPacker.h"
#include "ChatManagerEvents.h"
#include "ChatManager.h"
#include "BufferUtils.h"

#if TV_API

using namespace Windows::Xbox::System;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

typedef struct MIX_DATA
{
    DEVICE_ID captureSourceDeviceId;
    Windows::Storage::Streams::IBuffer^ captureBuffer;
} MIX_DATA;

ChatNetwork::ChatNetwork( 
    _In_ ChatAudioThread^ chatAudioThread,
    _In_ ChatClient^ chatClient,
    _In_ ChatManager^ chatManager,
    _In_ std::shared_ptr<FactoryCache> factoryCache,
    _In_ ChatManagerSettings^ chatManagerSettings
    ) :
    m_chatManager( chatManager ),
    m_factoryCache( factoryCache ),
    m_chatManagerSettings( chatManagerSettings )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatAudioThread);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatClient);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatManager);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(factoryCache);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatManagerSettings);

    m_chatAudioThread = Platform::WeakReference(chatAudioThread);
    m_chatClient = Platform::WeakReference(chatClient);
    m_audioDeviceIDMapper = ref new AudioDeviceIDMapper();
}

ChatNetwork::~ChatNetwork()
{
    CHAT_LOG_INFO_MSG(L"ChatNetwork::~ChatNetwork");
}

void ChatNetwork::LogComment(
    _In_ Platform::String^ message
    )
{
    LogCommentWithError(message, S_OK);
}

void ChatNetwork::LogCommentWithError(
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

Microsoft::Xbox::GameChat::ChatMessageType ChatNetwork::ProcessIncomingChatMessage( 
    _In_ Windows::Storage::Streams::IBuffer^ chatPacket,
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
    _In_ ChatClient^ chatClient,
    _In_ ChatAudioThread^ chatAudioThread,
    _In_ ChatManager^ chatManager
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatPacket);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(uniqueRemoteConsoleIdentifier);
    if ( chatPacket->Length == 0 )
    {
        return ChatMessageType::InvalidMessage;
    }

    byte* chatPacketBytes = nullptr;
    BufferUtils::GetBufferBytes( chatPacket, &chatPacketBytes );

    if ( chatPacket->Length < sizeof(ChatPacketHeader) )
    {
        // Ignore invalid packets
        CHAT_LOG_ERROR_MSG( L"Ignoring invalid packet" );
        return ChatMessageType::InvalidMessage;
    }

    bool performanceCountersEnabled = chatManager->ChatSettings->PerformanceCountersEnabled;
    if( performanceCountersEnabled ) { chatManager->ChatPerformanceCounters->QueryIncomingPacketStart(); }

    ChatPacketHeader& chatPacketHeader = (ChatPacketHeader&)*chatPacketBytes;
    ChatMessageType messageType = static_cast<ChatMessageType>(chatPacketHeader.messageType);

    switch (messageType)
    {
    case ChatMessageType::ChatVoiceDataMessage:
        {
            byte* dataPacketBufferBytes = chatPacketBytes + sizeof(ChatPacketHeader);
            uint32 dataPacketLength = chatPacket->Length - sizeof(ChatPacketHeader);
            ProcessIncomingChatVoicePacket( 
                dataPacketBufferBytes, 
                dataPacketLength, 
                uniqueRemoteConsoleIdentifier,
                chatClient,
                chatAudioThread,
                chatManager,
                chatPacket
                );
            break;
        }

    case ChatMessageType::UserAddedMessage:
        {
            // Must pass the IBuffer here to keep the reference on the packet buffer
            Concurrency::create_async( [this, chatPacket, uniqueRemoteConsoleIdentifier]()
            {
                byte* chatPacketBytes = nullptr;
                BufferUtils::GetBufferBytes( chatPacket, &chatPacketBytes );
                byte* dataPacketBufferBytes = chatPacketBytes + sizeof(ChatPacketHeader);
                uint32 dataPacketLength = chatPacket->Length - sizeof(ChatPacketHeader);

                ProcessIncomingUserAddedPacket( 
                    dataPacketBufferBytes, 
                    dataPacketLength, 
                    uniqueRemoteConsoleIdentifier,
                    chatPacket
                    );
            });
            break;
        }

    case ChatMessageType::UserRemovedMessage:
        {
            // Must pass the IBuffer here to keep the reference on the packet buffer
            Concurrency::create_async( [this, chatPacket, uniqueRemoteConsoleIdentifier]()
            {
                byte* chatPacketBytes = nullptr;
                BufferUtils::GetBufferBytes( chatPacket, &chatPacketBytes );
                byte* dataPacketBufferBytes = chatPacketBytes + sizeof(ChatPacketHeader);
                uint32 dataPacketLength = chatPacket->Length - sizeof(ChatPacketHeader);

                ProcessIncomingUserRemovedPacket( 
                    dataPacketBufferBytes, 
                    dataPacketLength, 
                    uniqueRemoteConsoleIdentifier,
                    chatPacket
                    );
            });
            break;
        }

    default:
        {
            // Ignore invalid packets
            CHAT_LOG_ERROR_MSG( L"Ignoring invalid packet type" );
            messageType = ChatMessageType::InvalidMessage;

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
            uint32 diagnosticConsoleId = chatManager->GetChatDiagnostics()->GetDiagnosticNameForConsole( chatManager, uniqueRemoteConsoleIdentifier );
            TraceChatIncomingChatInvalidPacket( 
                diagnosticConsoleId,
                BufferUtils::GetBase64String( chatPacket )->Data(),
                BufferUtils::GetLength( chatPacket )
                );
#endif
            break;
        }
    }

    if( performanceCountersEnabled ) 
    { 
        chatManager->ChatPerformanceCounters->AddPacketBandwidth( true, chatPacket->Length );
        chatManager->ChatPerformanceCounters->QueryIncomingPacketDone(); 
    }

    return messageType;
}

IBuffer^ ChatNetwork::GetPacketWithHeader( 
    _In_ uint32 packetSize, 
    _In_ uint8 messageType
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF(packetSize < sizeof(ChatPacketHeader));

    IBuffer^ networkPacketBuffer = BufferUtils::FastBufferCreate( packetSize, m_factoryCache->GetBufferFactory() );
    networkPacketBuffer->Length = packetSize;
    byte* messageBufferPtr = nullptr;
    BufferUtils::GetBufferBytes( networkPacketBuffer, &messageBufferPtr );

    // Fill out ChatPacketHeader
    ChatPacketHeader& packet = (ChatPacketHeader&)*messageBufferPtr;
    packet.messageType = messageType;
    packet.messageSize = (uint16)(packetSize);

    return networkPacketBuffer;
}


void ChatNetwork::CreateChatVoicePackets(
    _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState
    )
{
    ChatAudioThread^ chatAudioThread = m_chatAudioThread.Resolve<ChatAudioThread>();
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatAudioThread == nullptr || 
        chatManager == nullptr )
    {
        return;
    }

    if( chatManager->ChatSettings->CombineCaptureBuffersIntoSinglePacket )
    {
        CreateCombinedChatVoicePacket(chatAudioThreadState);
    }
    else
    {
        CreateChatVoicePacketsForEachLocalCaptureSource(chatAudioThreadState);
    }
}

void ChatNetwork::CreateCombinedChatVoicePacket( 
    _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState
    )
{
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager == nullptr )
    {
        return;
    }

    std::vector< MIX_DATA > captureBuffers;
    DWORD chatBufferSize = 0; 
    errno_t err;

    // Collect the capture source buffers and count how
    // large the network packet will need to be
    for each (std::shared_ptr<CHAT_AUDIO_THREAD_CAPTURE_SOURCE> chatAudioThreadCaptureSource in chatAudioThreadState->captureSources)
    {
        for (;;)
        {
            IBuffer^ captureBuffer = nullptr;
            if ( chatAudioThreadCaptureSource->audioBufferQueue.size() > 0 )
            {
                captureBuffer = chatAudioThreadCaptureSource->audioBufferQueue.front();
                chatAudioThreadCaptureSource->audioBufferQueue.pop();
            }
            if ( captureBuffer == nullptr )
            {
                break;
            }

            MIX_DATA md;
            md.captureSourceDeviceId = chatAudioThreadCaptureSource->captureSourceDeviceId;
            md.captureBuffer = captureBuffer;
            captureBuffers.push_back(md);

            // each chat message will contain a DEVICE ID that's local to this console.  When combined with the console name will create the LOOKUP_ID
            chatBufferSize += (DWORD) sizeof(DEVICE_ID); 

            chatBufferSize += (DWORD) sizeof(USHORT);
            chatBufferSize += captureBuffer->Length;
        }
    }

    if (chatBufferSize == 0)
    {
        return;
    }

    if( chatBufferSize > USHRT_MAX )
    {
        // The packet uses an unsigned short to sort the length of this buffer, so USHRT_MAX is max size.  
        // Normally its less than 100 bytes so this should be fine.
        CHAT_LOG_ERROR_MSG( L"Chat buffer too big" );
        return;
    }

    // Allocate a network packet buffer and pack all the chat packets into it.
    UINT packetSize = sizeof(ChatPacketHeader) + chatBufferSize;
    Windows::Storage::Streams::IBuffer^ packetBuffer = GetPacketWithHeader(packetSize, (uint8)ChatMessageType::ChatVoiceDataMessage );
    byte* packetBufferBytes = nullptr;
    BufferUtils::GetBufferBytes( packetBuffer, &packetBufferBytes );
    byte* networkPacketBufferBytes = packetBufferBytes  + sizeof(ChatPacketHeader);

    DWORD len = 0;

    for( auto it = captureBuffers.begin(); it != captureBuffers.end(); it++ )
    {
        // The network packet contains an lookup id, which we use to find the string that we should use
        networkPacketBufferBytes[len] = it->captureSourceDeviceId;
        len += sizeof(DEVICE_ID);

        // serialize the buffer
        byte* captureBufferBytes = nullptr;
        BufferUtils::GetBufferBytes( it->captureBuffer, &captureBufferBytes );

        unsigned short captureBufferSize = static_cast<unsigned short>(it->captureBuffer->Length);
        err = memcpy_s( &networkPacketBufferBytes[len], packetBuffer->Capacity-len, &captureBufferSize, sizeof(captureBufferSize));
        CHAT_THROW_HR_IF(err != 0, E_FAIL);
        len += sizeof(captureBufferSize);

        err = memcpy_s( &networkPacketBufferBytes[len], packetBuffer->Capacity-len, captureBufferBytes, captureBufferSize);
        CHAT_THROW_HR_IF(err != 0, E_FAIL);
        len += captureBufferSize;
    }

    CHAT_THROW_HR_IF(len != chatBufferSize, E_FAIL);

    bool sendPacketToAllConnectedConsoles = true;
    bool sendReliable = false;
    bool sendInOrder = true;
    ChatPacketEventArgs^ args = ref new ChatPacketEventArgs( 
        packetBuffer, 
        nullptr, 
        sendPacketToAllConnectedConsoles, 
        sendReliable, 
        sendInOrder,
        ChatMessageType::ChatVoiceDataMessage,
        nullptr
        );
    chatManager->OnChatPacketReadyHandler( args );

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatOutgoingChatVoiceDataPacket( 
        L"n/a", // Local user
        BufferUtils::GetBase64String( packetBuffer )->Data(),
        BufferUtils::GetLength( packetBuffer ),
        sendReliable,
        sendPacketToAllConnectedConsoles,
        sendInOrder,
        captureBuffers.size() > 0 ? captureBuffers[0].captureSourceDeviceId : 0
        );
#endif
}

void ChatNetwork::CreateChatVoicePacketsForEachLocalCaptureSource( 
    _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState 
    )
{
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager == nullptr )
    {
        return;
    }

    errno_t err;

    // Collect the capture source buffers and count how
    // large the network packet will need to be
    for each (std::shared_ptr<CHAT_AUDIO_THREAD_CAPTURE_SOURCE> chatAudioThreadCaptureSource in chatAudioThreadState->captureSources)
    {
        for (;;)
        {
            IBuffer^ captureBuffer = nullptr;
            if ( chatAudioThreadCaptureSource->audioBufferQueue.size() > 0 )
            {
                captureBuffer = chatAudioThreadCaptureSource->audioBufferQueue.front();
                chatAudioThreadCaptureSource->audioBufferQueue.pop();
            }
            if ( captureBuffer == nullptr )
            {
                break;
            }

            MIX_DATA md;
            md.captureSourceDeviceId = chatAudioThreadCaptureSource->captureSourceDeviceId;
            md.captureBuffer = captureBuffer;

            // each chat message will contain a DEVICE ID that's local to this console.  When combined with the console name will create the LOOKUP_ID
            DWORD chatBufferSize = (DWORD) sizeof(DEVICE_ID); 

            chatBufferSize += (DWORD) sizeof(USHORT);
            chatBufferSize += captureBuffer->Length;

            if (chatBufferSize == 0)
            {
                continue;
            }

            if( chatBufferSize > USHRT_MAX )
            {
                // The packet uses an unsigned short to sort the length of this buffer, so USHRT_MAX is max size.  
                // Normally its less than 100 bytes so this should be fine.
                CHAT_LOG_ERROR_MSG( L"Chat buffer too big" );
                continue;
            }

            // Allocate a network packet buffer and pack all the chat packets into it.
            UINT packetSize = sizeof(ChatPacketHeader) + chatBufferSize;
            Windows::Storage::Streams::IBuffer^ packetBuffer = GetPacketWithHeader(packetSize, (uint8)ChatMessageType::ChatVoiceDataMessage );
            byte* packetBufferBytes = nullptr;
            BufferUtils::GetBufferBytes( packetBuffer, &packetBufferBytes );
            byte* networkPacketBufferBytes = packetBufferBytes  + sizeof(ChatPacketHeader);

            DWORD len = 0;

            // The network packet contains an lookup id, which we use to find the string that we should use
            networkPacketBufferBytes[len] = md.captureSourceDeviceId;
            len += sizeof(DEVICE_ID);

            // serialize the buffer
            byte* captureBufferBytes = nullptr;
            BufferUtils::GetBufferBytes( md.captureBuffer, &captureBufferBytes );

            unsigned short captureBufferSize = static_cast<unsigned short>(md.captureBuffer->Length);
            err = memcpy_s( &networkPacketBufferBytes[len], packetBuffer->Capacity-len, &captureBufferSize, sizeof(captureBufferSize));
            CHAT_THROW_HR_IF(err != 0, E_FAIL);
            len += sizeof(captureBufferSize);

            err = memcpy_s( &networkPacketBufferBytes[len], packetBuffer->Capacity-len, captureBufferBytes, captureBufferSize);
            CHAT_THROW_HR_IF(err != 0, E_FAIL);
            len += captureBufferSize;

            CHAT_THROW_HR_IF(len != chatBufferSize, E_FAIL);

            ChatUser^ localChatUser = nullptr;
            ChatUserTalkingMode talkingMode = chatAudioThreadCaptureSource->talkingMode;
            if (talkingMode == ChatUserTalkingMode::TalkingOverHeadset &&
                chatAudioThreadCaptureSource->chatUsers->Size == 1)
            {
                localChatUser = chatAudioThreadCaptureSource->chatUsers->GetAt(0);
            }

            bool sendPacketToAllConnectedConsoles = true; 
            bool sendReliable = false;
            bool sendInOrder = true;
            ChatPacketEventArgs^ args = ref new ChatPacketEventArgs( 
                packetBuffer, 
                nullptr,
                sendPacketToAllConnectedConsoles,
                sendReliable, 
                sendInOrder,
                ChatMessageType::ChatVoiceDataMessage,
                localChatUser
                );
            chatManager->OnChatPacketReadyHandler( args );

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
            TraceChatOutgoingChatVoiceDataPacket( 
                localChatUser != nullptr ? localChatUser->XboxUserId->Data() : L"n/a",
                BufferUtils::GetBase64String( packetBuffer )->Data(),
                BufferUtils::GetLength( packetBuffer ),
                sendReliable,
                sendPacketToAllConnectedConsoles,
                sendInOrder,
                md.captureSourceDeviceId
                );
#endif
        }
    }
}

void ChatNetwork::ProcessIncomingChatVoicePacket( 
    _In_reads_bytes_(chatVoicePacketLength) byte* chatVoicePacketBytes,
    _In_ uint32 chatVoicePacketLength,
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
    _In_ ChatClient^ chatClient,
    _In_ ChatAudioThread^ chatAudioThread,
    _In_ ChatManager^ chatManager,
    _In_ Windows::Storage::Streams::IBuffer^ chatPacket
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatVoicePacketBytes);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(uniqueRemoteConsoleIdentifier);

    DWORD offset = 0;
    CONSOLE_NAME localNameOfRemoteConsole;
    
    localNameOfRemoteConsole = chatClient->GetLocalNameOfRemoteConsole(uniqueRemoteConsoleIdentifier);

    while( offset < chatVoicePacketLength )
    {
        // Deserialize the lookup id
        DEVICE_ID deviceID;
        if( chatVoicePacketLength < offset + sizeof(deviceID) )
        {
            CHAT_LOG_ERROR_MSG( L"Ignoring invalid voice packet" );
            return;
        }
        deviceID = chatVoicePacketBytes[offset];
        offset += sizeof(deviceID);

        LOOKUP_ID lookupId = m_audioDeviceIDMapper->ConstructLookupID(localNameOfRemoteConsole, deviceID);

        // Deserialize the audio data
        // part 1: size of buffer in unsigned short
        if( chatVoicePacketLength < offset + sizeof(unsigned short) )
        {
            CHAT_LOG_ERROR_MSG( L"Ignoring invalid voice packet" );
            return;
        }
        unsigned short audioBufferSize = *reinterpret_cast<unsigned short *>(&chatVoicePacketBytes[offset]); 
        offset += sizeof(unsigned short);

        // Deserialize the audio data
        // part 2: audio data byte array
        if( chatVoicePacketLength < offset + audioBufferSize )
        {
            CHAT_LOG_ERROR_MSG( L"Ignoring invalid voice packet" );
            return;
        }

        Platform::String^ captureSourceId = m_audioDeviceIDMapper->GetRemoteAudioDevice(lookupId);
        if( !captureSourceId->IsEmpty() ) // Ignore audio devices that have not been remembered during a previous USER_ADDED packet
        {
            // Hand off the audio buffer to the audio thread's render queue
            chatAudioThread->PushRemoteCaptureAudioBuffer(
                lookupId, 
                captureSourceId, 
                &chatVoicePacketBytes[offset], 
                audioBufferSize
                );
        }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        uint32 diagnosticConsoleId = chatManager->GetChatDiagnostics()->GetDiagnosticNameForConsole( chatManager, uniqueRemoteConsoleIdentifier );
        TraceChatIncomingChatVoiceDataPacket( 
            diagnosticConsoleId,
            BufferUtils::GetBase64String( chatPacket )->Data(),
            BufferUtils::GetLength( chatPacket ),
            localNameOfRemoteConsole, 
            deviceID,
            captureSourceId->Data()
            );
#endif

        offset += audioBufferSize;
    }
}

void ChatNetwork::CreateChatUserPacket(
    _In_ uint8 channelIndex,
    _In_ ChatUser^ localChatUser,
    _In_ Platform::Object^ uniqueTargetConsoleIdentifier
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(localChatUser);

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    ChatClient^ chatClient = m_chatClient.Resolve<ChatClient>();
    if( chatManager == nullptr || chatClient == nullptr )
    {
        return;
    }

    Mwrl::ComPtr<Awxs::IUser> currentUser = reinterpret_cast<ABI::Windows::Xbox::System::IUser*>(localChatUser->User);
    BYTE* userBufferBytes = nullptr;
    UINT userBufferSize = 0;

    // Packing the user to send across the network into a set of bytes
    Packer packer;
    HRESULT hr = packer.PackBegin();
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = packer.PackUser( currentUser, m_audioDeviceIDMapper );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = packer.PackEnd( &userBufferBytes, &userBufferSize );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    bool hasAddedRemoteUserToLocalChatSession = false;
    // Check if the uniqueTargetConsoleIdentifier already exists in our chat session.
    if(chatClient->DoesRemoteUniqueConsoleIdentifierExist(uniqueTargetConsoleIdentifier))
    {
        hasAddedRemoteUserToLocalChatSession = true;
    }

    UINT packetSize = sizeof(ChatPacketHeader) + userBufferSize + sizeof(channelIndex) + sizeof(hasAddedRemoteUserToLocalChatSession);
    Windows::Storage::Streams::IBuffer^ packetBuffer = GetPacketWithHeader(packetSize, (uint8)ChatMessageType::UserAddedMessage );
    byte* packetBufferBytes = nullptr;
    BufferUtils::GetBufferBytes( packetBuffer, &packetBufferBytes );

    // Fill out a userBufferBytes, which appears after the ChatPacketHeader
    BYTE* userBufferBytesPtr = packetBufferBytes + sizeof(ChatPacketHeader);
    errno_t err = memcpy_s(userBufferBytesPtr, packetSize - sizeof(ChatPacketHeader), userBufferBytes, userBufferSize);
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    // Serialize a channelIndex, which appears after the ChatPacketHeader and the userBuffer
    BYTE* channelIndexPtr = packetBufferBytes + sizeof(ChatPacketHeader) + userBufferSize;
    err = memcpy_s(channelIndexPtr, packetSize - sizeof(ChatPacketHeader) - userBufferSize , &channelIndex, sizeof(channelIndex));
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    // Serialize the hasAddedRemoteUserToLocalChatSession flag, which appears after the ChatPacketHeader, the userBuffer and channelIndex
    BYTE* hasAddedRemoteUserToLocalChatSessionPtr = packetBufferBytes + sizeof(ChatPacketHeader) + userBufferSize + sizeof(channelIndex);
    err = memcpy_s(hasAddedRemoteUserToLocalChatSessionPtr, 
        packetSize - sizeof(ChatPacketHeader) - userBufferSize - sizeof(channelIndex), 
        &hasAddedRemoteUserToLocalChatSession, 
        sizeof(hasAddedRemoteUserToLocalChatSession)
        );
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    bool sendPacketToAllConnectedConsoles = false;
    bool sendReliable = true;
    bool sendInOrder = false;
    
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    uint32 diagnosticConsoleId = chatManager->GetChatDiagnostics()->GetDiagnosticNameForConsole( chatManager, uniqueTargetConsoleIdentifier );
    TraceChatOutgoingChatUserAddedPacket( 
        localChatUser != nullptr ? localChatUser->XboxUserId->Data() : L"n/a",
        BufferUtils::GetBase64String( packetBuffer )->Data(),
        BufferUtils::GetLength( packetBuffer ),
        diagnosticConsoleId,
        sendReliable,
        sendPacketToAllConnectedConsoles,
        sendInOrder
        );
#endif

    ChatPacketEventArgs^ args = ref new ChatPacketEventArgs( 
        packetBuffer, 
        uniqueTargetConsoleIdentifier, 
        sendPacketToAllConnectedConsoles, 
        sendReliable, 
        sendInOrder,
        ChatMessageType::UserAddedMessage,
        localChatUser
        );
    chatManager->OnChatPacketReadyHandler( args );
}

void ChatNetwork::CreateChatUserRemovedPacket(
    _In_ uint8 channelIndex,
    _In_ ChatUser^ localChatUser
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(localChatUser);

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager == nullptr )
    {
        return;
    }

    Platform::String^ xuid = localChatUser->XboxUserId;
    UINT xuidSizeInChars = xuid->Length();
    UINT xuidSizeInBytes = xuidSizeInChars * 2;

    UINT packetSize = sizeof(ChatPacketHeader) + xuidSizeInBytes + sizeof(channelIndex);
    Windows::Storage::Streams::IBuffer^ packetBuffer = GetPacketWithHeader(packetSize, (uint8)ChatMessageType::UserRemovedMessage );
    byte* packetBufferBytes = nullptr;
    BufferUtils::GetBufferBytes( packetBuffer, &packetBufferBytes );

    // Serialize a channelIndex, which appears after the ChatPacketHeader 
    BYTE* channelIndexPtr = packetBufferBytes + sizeof(ChatPacketHeader);
    errno_t err = memcpy_s(channelIndexPtr, packetSize - sizeof(ChatPacketHeader), &channelIndex, sizeof(channelIndex));
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    // Serialize the xuid, which appears after the ChatPacketHeader and the channelIndex
    BYTE* xuidPtr = packetBufferBytes + sizeof(ChatPacketHeader) + sizeof(channelIndex);
    err = memcpy_s(xuidPtr, packetSize - sizeof(ChatPacketHeader) - sizeof(channelIndex), xuid->Data(), xuidSizeInBytes);
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    bool sendPacketToAllConnectedConsoles = true;
    bool sendReliable = true;
    bool sendInOrder = false;

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatOutgoingChatUserRemovedPacket( 
        BufferUtils::GetBase64String( packetBuffer )->Data(),
        BufferUtils::GetLength( packetBuffer ),
        sendReliable,
        sendPacketToAllConnectedConsoles,
        sendInOrder,
        xuid->Data()
        );
#endif

    ChatPacketEventArgs^ args = ref new ChatPacketEventArgs( 
        packetBuffer, 
        nullptr, 
        sendPacketToAllConnectedConsoles,
        sendReliable, 
        sendInOrder,
        ChatMessageType::UserRemovedMessage,
        localChatUser
        );
    chatManager->OnChatPacketReadyHandler( args );
}

void ChatNetwork::ProcessIncomingUserAddedPacket( 
    _In_reads_bytes_(chatUserPacketLength) byte* chatUserPacketBytes,
    _In_ uint32 chatUserPacketLength,
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
    _In_ Windows::Storage::Streams::IBuffer^ chatPacket
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(chatUserPacketBytes);
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(uniqueRemoteConsoleIdentifier);
    
    Concurrency::critical_section::scoped_lock lock(m_userAddRemoveLock);

    ChatClient^ chatClient = m_chatClient.Resolve<ChatClient>();
    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if ( chatClient == nullptr || chatManager == nullptr )
    {
        return;
    }

    uint8 localNameOfRemoteConsole = chatClient->GetLocalNameOfRemoteConsole(uniqueRemoteConsoleIdentifier);

    Mwrl::ComPtr<Awxs::IUser> user = nullptr;
    Unpacker unpacker;
    HRESULT hr;

    // This packet contains a set of bytes that represent a remote user.
    // Convert those bytes into an IUser
    // uint8 is for the channelIndex; bool is for the hasAddedRemoteUserToLocalChatSession flag
    hr = unpacker.UnpackBegin( chatUserPacketBytes, chatUserPacketLength - sizeof(uint8) - sizeof(bool));
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = unpacker.UnpackUser( user, m_audioDeviceIDMapper, localNameOfRemoteConsole );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = unpacker.UnpackEnd();
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }
    
    bool hasAddedRemoteUserToLocalChatSession = *reinterpret_cast<const bool *>(&chatUserPacketBytes[chatUserPacketLength-sizeof(bool)]); 
    uint8 channelIndex = *reinterpret_cast<const uint8 *>(&chatUserPacketBytes[chatUserPacketLength-sizeof(uint8)-sizeof(bool)]); 
    
    Platform::String^ chatUserXuid;
    HSTRING userXuid;
    unsigned int userId;
    boolean isGuestBoolean;

    hr = user->get_XboxUserId( &userXuid );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    chatUserXuid = ref new Platform::String( userXuid );

    WindowsDeleteString( userXuid );
    userXuid = nullptr;

    if( chatUserXuid->IsEmpty() )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = user->get_Id( &userId );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    hr = user->get_IsGuest( &isGuestBoolean );
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }
    bool isGuest = (isGuestBoolean != 0);

    // Create a set of audio devices that this remote user will contain
    Windows::Foundation::Collections::IVector< Wxs::IAudioDeviceInfo^ >^ audioDevices = ref new Platform::Collections::Vector< Wxs::IAudioDeviceInfo^ >();
    Microsoft::WRL::ComPtr< ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Xbox::System::IAudioDeviceInfo*> > devices;
    hr = user->get_AudioDevices(&devices);
    if( FAILED(hr) )
    {
        // Ignoring invalid chat user packet
        return;
    }

    UINT size = 0;
    hr = devices->get_Size(&size);
    if( FAILED(hr) || size == 0 )
    {
        // Ignoring invalid chat user packet
        return;
    }

    for (UINT i = 0; i < size; i++)
    {
        ABI::Windows::Xbox::System::IAudioDeviceInfo* currentAudioDeviceInfo = nullptr;
        ABI::Windows::Xbox::System::AudioDeviceCategory audioDeviceCategory = ABI::Windows::Xbox::System::AudioDeviceCategory::AudioDeviceCategory_Communications;
        ABI::Windows::Xbox::System::AudioDeviceType audioDeviceType = ABI::Windows::Xbox::System::AudioDeviceType::AudioDeviceType_Capture;
        ABI::Windows::Xbox::System::AudioDeviceSharing audioDeviceSharing = ABI::Windows::Xbox::System::AudioDeviceSharing::AudioDeviceSharing_Exclusive;
        HSTRING audioDeviceInfoID = nullptr;
        boolean audioDeviceInfoIsMicMuted = false;

        hr = devices->GetAt(i, &currentAudioDeviceInfo);
        if( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        hr = currentAudioDeviceInfo->get_DeviceCategory(&audioDeviceCategory);
        if( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        hr = currentAudioDeviceInfo->get_DeviceType(&audioDeviceType);
        if( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        hr = currentAudioDeviceInfo->get_Id(&audioDeviceInfoID); 
        if( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        // Cleanup the HSTRING
        Platform::String^ strAudioDeviceInfoID = ref new Platform::String( audioDeviceInfoID );
        WindowsDeleteString( audioDeviceInfoID );
        audioDeviceInfoID = nullptr;

        hr = currentAudioDeviceInfo->get_IsMicrophoneMuted(&audioDeviceInfoIsMicMuted);
        if ( hr == E_NOTIMPL )
        {
            audioDeviceInfoIsMicMuted = false;
        }
        else if ( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        hr = currentAudioDeviceInfo->get_Sharing(&audioDeviceSharing);
        if( FAILED(hr) )
        {
            // Ignoring invalid chat user packet
            return;
        }

        auto dInfo = ref new RemoteAudioDeviceInfo(
            static_cast<Windows::Xbox::System::AudioDeviceCategory>(audioDeviceCategory), 
            static_cast<Windows::Xbox::System::AudioDeviceType>(audioDeviceType), 
            strAudioDeviceInfoID, 
            audioDeviceInfoIsMicMuted ? true : false, 
            (Windows::Xbox::System::AudioDeviceSharing)audioDeviceSharing);

        audioDevices->Append( dInfo);
    }

    IUser^ remoteUser = nullptr;

    // Users can be added to multiple channels so check if we already know about this one
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = chatManager->GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        if (chatUser != nullptr && StringUtils::IsStringEqualCaseInsenstive(chatUser->XboxUserId, chatUserXuid) )
        {
            remoteUser = chatUser->User;
        }
    }

    if (remoteUser == nullptr)
    {
        remoteUser = ref new RemoteChatUser( chatUserXuid, isGuest, audioDevices->GetView() );
    }
    else
    {
        auto existingUser = dynamic_cast<RemoteChatUser^>(remoteUser);
        if (existingUser != nullptr)
        {
            existingUser->ResetAudioDevices( audioDevices->GetView() );
            remoteUser = existingUser;
        }
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    uint32 diagnosticConsoleId = chatManager->GetChatDiagnostics()->GetDiagnosticNameForConsole( chatManager, uniqueRemoteConsoleIdentifier );
    TraceChatIncomingChatUserAddedPacket( 
        diagnosticConsoleId,
        BufferUtils::GetBase64String( chatPacket )->Data(),
        chatUserPacketLength
        );
    chatManager->GetChatDiagnostics()->TraceChatUserAndAudioDevices( remoteUser );
#endif

    chatManager->OnRemoteUserReadyToAddHandler( channelIndex, remoteUser, uniqueRemoteConsoleIdentifier, hasAddedRemoteUserToLocalChatSession );
}

void ChatNetwork::ProcessIncomingUserRemovedPacket(
    _In_reads_bytes_(chatUserRemovedPacketLength) byte* chatUserRemovedPacketBytes, 
    _In_ uint32 chatUserRemovedPacketLength, 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
    _In_ Windows::Storage::Streams::IBuffer^ chatPacket
    )
{
    Concurrency::critical_section::scoped_lock lock(m_userAddRemoveLock);

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager == nullptr )
    {
        return;
    }

    if( chatUserRemovedPacketLength < sizeof(uint8) )
    {
        CHAT_LOG_ERROR_MSG( L"Ignoring invalid user removed packet" );
        return;
    }

    uint32 offset = 0;
    uint8 channelIndex = *reinterpret_cast<const uint8 *>(&chatUserRemovedPacketBytes[offset]); 
    offset += sizeof(uint8);

    // Deserialize the user xuid.
    uint32 bytesRemaining = chatUserRemovedPacketLength - offset;
    if( bytesRemaining == 0 || bytesRemaining > 25*sizeof(WCHAR) ) // 20 chars is max length for max XUID in decimal string form
    {
        CHAT_LOG_ERROR_MSG( L"Ignoring invalid user removed packet" );
        return;
    }

    BYTE* userXuidPtr = chatUserRemovedPacketBytes + offset;
    uint32 sizeOfXboxUserIdInChars = bytesRemaining / sizeof(WCHAR);
    Platform::String^ userXuid = ref new Platform::String((WCHAR*)userXuidPtr, sizeOfXboxUserIdInChars);

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    uint32 diagnosticConsoleId = chatManager->GetChatDiagnostics()->GetDiagnosticNameForConsole( chatManager, uniqueRemoteConsoleIdentifier );
    TraceChatIncomingChatUserRemovedPacket( 
        diagnosticConsoleId,
        BufferUtils::GetBase64String( chatPacket )->Data(),
        chatUserRemovedPacketLength
        );
#endif

    chatManager->OnRemoteUserReadyToRemoveHandler( channelIndex, userXuid );
}

AudioDeviceIDMapper^ ChatNetwork::GetAudioDeviceIDMapper()
{
    return m_audioDeviceIDMapper;
}

}}}

#endif