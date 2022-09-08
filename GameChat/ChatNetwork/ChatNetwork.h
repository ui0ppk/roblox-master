//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatUser.h"
#include "Thread.h"
#include "FactoryCache.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

// Set data alignment to be 1 byte
#pragma pack(push) 
#pragma pack(1) 

struct ChatPacketHeader
{
    /// <summary>
    /// Type of message (MessageTypeEnum)
    /// </summary>
    uint8 messageType; 

    /// <summary>
    /// Total number of bytes in the packet
    /// </summary>
    uint16 messageSize; 
};  

// Store data alignment
#pragma pack(pop)  


ref class ChatNetwork sealed
{
internal:
    /// <summary>
    /// Creates internal ChatNetwork class.  This class handles the chat network packets and talks to both the ChatClient and ChatAudioThread components.
    /// </summary>
    ChatNetwork( 
        _In_ ChatAudioThread^ chatAudioThread,
        _In_ ChatClient^ chatClient,
        _In_ ChatManager^ chatManager,
        _In_ std::shared_ptr<FactoryCache> factoryCache,
        _In_ ChatManagerSettings^ chatManagerSettings
        );

public:
    /// <summary>
    /// Shuts down the ChatNetwork
    /// </summary>
    virtual ~ChatNetwork();

internal:
    /// <summary>
    /// Handles incoming chat messages
    /// </summary>
    Microsoft::Xbox::GameChat::ChatMessageType ProcessIncomingChatMessage( 
        _In_ Windows::Storage::Streams::IBuffer^ chatPacket,
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
        _In_ ChatClient^ chatClient,
        _In_ ChatAudioThread^ chatAudioThread,
        _In_ ChatManager^ chatManager
        );

    /// <summary>
    /// Creates and raises events to send out chat voice packets.
    /// </summary>
    void CreateChatVoicePackets( 
        _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState
        );

    /// <summary>
    /// Creates and raises events to send out chat voice packets that are specific to a remote console
    /// This is called from CreateChatVoicePackets when chatManager->ChatSettings->CombineCaptureBuffersIntoSinglePacket is true.
    /// </summary>
    void CreateCombinedChatVoicePacket( 
        _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState
        );

    /// <summary>
    /// Creates and raises events to send out chat voice packets that are generic for all remote consoles
    /// This is called from CreateChatVoicePackets when chatManager->ChatSettings->CombineCaptureBuffersIntoSinglePacket is false.
    /// </summary>
    void CreateChatVoicePacketsForEachLocalCaptureSource( 
        _In_ std::shared_ptr< CHAT_AUDIO_THREAD_STATE > chatAudioThreadState 
        );

    /// <summary>
    /// This is called when a remote user packet needs to be sent out.  
    /// It creates a chat user packet and triggers the OnChatPacketReady event with it
    /// </summary>
    void CreateChatUserPacket(
        _In_ uint8 channelIndex,
        _In_ ChatUser^ localChatUser,
        _In_ Platform::Object^ uniqueTargetConsoleIdentifier
        );

    /// <summary>
    /// This is called when a remote user is removed.  
    /// It creates a chat user packet and triggers the OnChatPacketReady event with it
    /// </summary>
    void ChatNetwork::CreateChatUserRemovedPacket(
        _In_ uint8 channelIndex,
        _In_ ChatUser^ localChatUser
        );

    AudioDeviceIDMapper^ GetAudioDeviceIDMapper();

private:
    void LogComment(
        _In_ Platform::String^ message
        );

    void LogCommentWithError(
        _In_ Platform::String^ message, 
        _In_ HRESULT hr
        );

    /// <summary>
    /// This is an internal help function to create a Buffer with a specific size 
    /// and ChatMessageTypeEnum message type.
    /// </summary>
    Windows::Storage::Streams::IBuffer^ GetPacketWithHeader( 
        _In_ uint32 packetSize, 
        _In_ uint8 messageType
        );

    /// <summary>
    /// This is called when a remote console sends a chat voice packet.  
    /// It is deserialized here before and is pushed to the ChatAudioThread render buffer queue 
    /// later playback in ChatAudioThread::RenderAudioToAllRenderTargets()
    /// </summary>
    void ProcessIncomingChatVoicePacket( 
        _In_reads_bytes_(chatVoicePacketLength) byte* chatVoicePacketBytes,
        _In_ uint32 chatVoicePacketLength,
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
        _In_ ChatClient^ chatClient,
        _In_ ChatAudioThread^ chatAudioThread,
        _In_ ChatManager^ chatManager,
        _In_ Windows::Storage::Streams::IBuffer^ chatPacket
        );

    /// <summary>
    /// This is called when a remote console sends a chat user packet.  
    /// This function unpacks remote user data and creates a RemoteChatUser and associated
    /// audio devices
    /// </summary>
    void ProcessIncomingUserAddedPacket( 
        _In_reads_bytes_(chatUserPacketLength) byte* chatUserPacketBytes,
        _In_ uint32 chatUserPacketLength,
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
        _In_ Windows::Storage::Streams::IBuffer^ chatPacket
        );

    /// <summary>
    /// This is called when a remote console sends a chat user remove packet.
    /// </summary>
    void ProcessIncomingUserRemovedPacket( 
        _In_reads_bytes_(chatUserRemovedPacketLength) byte* chatUserRemovedPacketBytes, 
        _In_ uint32 chatUserRemovedPacketLength, 
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier,
        _In_ Windows::Storage::Streams::IBuffer^ chatPacket
        );

private:
    Concurrency::critical_section m_stateLock;
    Concurrency::critical_section m_userAddRemoveLock;
    Platform::WeakReference m_chatAudioThread;
    Platform::WeakReference m_chatClient;
    Platform::WeakReference m_chatManager;
    ChatManagerSettings^ m_chatManagerSettings;
    AudioDeviceIDMapper^ m_audioDeviceIDMapper;
    std::shared_ptr<FactoryCache> m_factoryCache;
};

}}}

#endif