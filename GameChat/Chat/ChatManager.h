//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatClient.h"
#include "ChatPerformance.h"
#include "ChatDiagnostics.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

#define STATISTICS_SERVICE_GUID "7492baca-c1b4-440d-a391-b7ef364a8d40"

/// <summary>
/// The chat session period in milliseconds.  
/// This defines how big the chat capture buffers will be.
/// Larger buffers adds latency
/// </summary>
public enum class ChatSessionPeriod
{
    /// <summary>
    /// Sets the chat session period to 20 milliseconds
    /// </summary>
    ChatPeriodOf20Milliseconds,

    /// <summary>
    /// Sets the chat session period to 40 milliseconds
    /// </summary>
    ChatPeriodOf40Milliseconds,

    /// <summary>
    /// Sets the chat session period to 80 milliseconds
    /// </summary>
    ChatPeriodOf80Milliseconds
};

public ref class ChatManager sealed
{
public:

    /// <summary>
    /// Creates the chat manager class using a default ChatSession period of 40 milliseconds
    /// </summary>
    ChatManager();

    /// <summary>
    /// To shutdown the ChatManager, simple set ChatManager^ to nullptr.
    /// This will automatically shutdown the local chat session.  
    /// It is best to simply tear down and rebuild the whole ChatManger class, 
    /// otherwise every API would have to do checks against throw if not initialized.
    /// </summary>
    virtual ~ChatManager();

    /// <summary>
    /// Creates the chat manager class using the specified ChatSession period 
    /// </summary>
    /// <param name="chatSessionPeriod">
    /// The chat session period in milliseconds.  
    /// This defines how big the chat capture buffers will be.
    /// Larger buffers adds latency
    /// </param>
    ChatManager(
        _In_ ChatSessionPeriod chatSessionPeriod
        );

    /// <summary>
    /// Set various chat manager options.  
    /// If this is not called, defaults values are used.
    /// It function can be called at any time to change the previously set options
    /// </summary>
    property ChatManagerSettings^ ChatSettings { ChatManagerSettings^ get(); }
        
    /// <summary>
    /// This event is triggered when the chat manager has a debug message.  
    /// The game can optionally listen to this event to debug failures and behavior
    /// </summary>
    event Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::DebugMessageEventArgs^>^ OnDebugMessage;

    /// <summary>
    /// This event is triggered when the chat manager has a network packet ready to send out. 
    /// The ChatPacketEventArgs provide context on the packet buffer, who the packet is for, and options around how to packet is to be sent.
    /// The packets are opaque to the game.  
    /// When the chat message is received by the remote console, it must call ProcessIncomingChatMessage() to so the chat manager can process the message.
    /// The game is required to listen to this event.
    /// When handling this event, you will need to have a thread safe network layer as this event will be called from 
    /// an internal worker thread that is real time priority by default.  
    /// The chat packet must be serviced as quickly as possible since this thread is real time priority by default.  
    /// The chat packet should also be sent to the remote console as quickly as possible to reduce latency.
    /// </summary>
    event Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::ChatPacketEventArgs^>^ OnOutgoingChatPacketReady;

    /// <summary>
    /// This delegate compares 2 uniqueRemoteConsoleIdentifiers.  They are Platform::Object^ and can be cast or unboxed to most types. 
    /// What exactly you use doesn't matter, but optimally it would be something that uniquely identifies a console on in the session. 
    /// A Windows::Xbox::Networking::SecureDeviceAssociation^ is perfect to use if you have access to it.
    /// This delegate is not optional and must return true if the uniqueRemoteConsoleIdentifier1 matches uuniqueRemoteConsoleIdentifier2
    /// </summary>
    event CompareUniqueConsoleIdentifiersHandler^ OnCompareUniqueConsoleIdentifiers;

    /// <summary>
    /// This delegate is called prior to encoding captured audio buffer.
    /// This allows titles to apply sound effects to the capture stream
    /// To use, register for this delegate and set ChatManagerSettings::PreEncodeCallbackEnabled to true
    /// </summary>
    event ProcessAudioBufferHandler^ OnPreEncodeAudioBuffer;

    /// <summary>
    /// This delegate is called after decoding a remote audio buffer.
    /// This allows titles to apply sound effects to a remote user's audio mix
    /// To use, register for this delegate and set ChatManagerSettings::PostDecodeCallbackEnabled to true
    /// </summary>
    event ProcessAudioBufferHandler^ OnPostDecodeAudioBuffer;

    /// <summary>
    /// Processes incoming chat messages from remote consoles.
    /// It must call ProcessIncomingChatMessage() to so the chat manager can process the message.
    /// </summary>
    /// <param name="chatPacket">
    /// The incoming chat packet inside an IBuffer.  
    /// This is how you would convert from byte array to a IBuffer^
    ///
    ///    Windows::Storage::Streams::IBuffer^ destBuffer = ref new Windows::Storage::Streams::Buffer( sourceByteBufferSize );
    ///    byte* destBufferBytes = nullptr;
    ///    GetBufferBytes( destBuffer, &destBufferBytes );
    ///    errno_t err = memcpy_s( destBufferBytes, destBuffer->Capacity, sourceByteBuffer, sourceByteBufferSize );
    ///    THROW_HR_IF(err != 0, E_FAIL);
    ///    destBuffer->Length = sourceByteBufferSize;
    /// </param>
    /// <param name="uniqueRemoteConsoleIdentifier">
    /// uniqueRemoteConsoleIdentifier is a Platform::Object^ and can be cast or unboxed to most types. 
    /// What exactly you use doesn't matter, but optimally it would be something that uniquely identifies a console on in the session. 
    /// A Windows::Xbox::Networking::SecureDeviceAssociation^ is perfect to use if you have access to it.
    ///
    /// This is how you would convert from an int to a Platform::Object^
    /// Platform::Object obj = (Object^)5;
    /// </param>
    /// <returns>
    /// The chat message type that was processed.  This can be used to track down networking issues, but typically can be ignored
    /// </returns>
    Microsoft::Xbox::GameChat::ChatMessageType ProcessIncomingChatMessage( 
        _In_ Windows::Storage::Streams::IBuffer^ chatPacket, 
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
        );

    /// <summary>
    /// This causes existing local users to be resent to a new remote console connection.
    /// </summary>
    /// <param name="uniqueRemoteConsoleIdentifier">
    /// uniqueRemoteConsoleIdentifier is a Platform::Object^ and can be cast or unboxed to most types. 
    /// What exactly you use doesn't matter, but optimally it would be something that uniquely identifies a console on in the session. 
    /// A Windows::Xbox::Networking::SecureDeviceAssociation^ is perfect to use if you have access to it.
    ///
    /// This is how you would convert from an int to a Platform::Object^
    /// Platform::Object obj = (Object^)5;
    /// </param>
    void HandleNewRemoteConsole(
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
        );

    /// <summary>
    /// Adds a local user to the chat channel.
    /// Note that this user should have expressed intent to play prior to calling this, because the user may 
    /// have been signed in due to Kinect automatically but may not want to play or take up a chat slot.
    /// This will automatically serialize the local user to a packet that can be sent to all connected consoles.
    /// </summary>
    /// <param name="channelIndex">The index of the chat channel</param>
    /// <param name="user">The user to add to the chat channel</param>
    Windows::Foundation::IAsyncAction^ 
    AddLocalUserToChatChannelAsync( 
        _In_ uint8 channelIndex,
        _In_  Windows::Xbox::System::IUser^ user
        ); 

    /// <summary>
    /// Adds a local user to the chat channel.
    /// Note that this user should have expressed intent to play prior to calling this, because the user may 
    /// have been signed in due to Kinect automatically but may not want to play or take up a chat slot.
    /// This will automatically serialize the local user to a packet that can be sent to all connected consoles.
    /// </summary>
    /// <param name="channelIndex">The index of the chat channel</param>
    /// <param name="users">The user to add to the chat channel</param>
    Windows::Foundation::IAsyncAction^ 
    AddLocalUsersToChatChannelAsync( 
        _In_ uint8 channelIndex,
        _In_ Windows::Foundation::Collections::IVectorView<Windows::Xbox::System::User^>^ users
        ); 

    /// <summary>
    /// Adds a remote user to the chat channel.
    /// This will automatically create a packet to inform the all connected consoles that this player should be removed.
    /// </summary>
    /// <param name="channelIndex">The index of the chat channel</param>
    /// <param name="user">The user to remove to the chat channel</param>
    Windows::Foundation::IAsyncAction^ 
    RemoveLocalUserFromChatChannelAsync(
        _In_ uint8 channelIndex,
        _In_ Windows::Xbox::System::IUser^ user
        );

    /// <summary>
    /// Remove all remote users that are attached to remote console.
    /// This is typically called when a connection to a remote console is destroyed
    /// </summary>
    /// <param name="consoleId">A consoleId of the remote console</param>
    Windows::Foundation::IAsyncAction^ 
    RemoveRemoteConsoleAsync(
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier
        ); 
    
    /// <summary>
    /// Returns a list of ChatUser objects.  
    /// The ChatUser object contains metadata about the user such as if they are talking
    /// </summary>
    /// <returns>A list of ChatUser objects</returns>
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ GetChatUsers();

    /// <summary>
    /// Mutes the user from all channels.
    /// If you mute a local user, it stops capturing packets from the capture source
    /// but does not stop you from receiving packets.
    /// </summary>
    /// <param name="user">The user to mute</param>
    void MuteUserFromAllChannels( ChatUser^ user );

    /// <summary>
    /// Mutes the user from all channels. And sets permanent mute flag
    /// </summary>
    /// <param name="user">The user to mute</param>
    void MuteUserFromAllChannelsPermanently( ChatUser^ user );

    /// <summary>
    /// Unmute a specific user from all channels.
    /// </summary>
    /// <param name="user">The user to unmute</param>
    void UnmuteUserFromAllChannels( ChatUser^ user );

    /// <summary>
    /// Mute all users in the chat session
    /// </summary>
    void MuteAllUsersFromAllChannels();

    /// <summary>
    /// Unmute all users in the chat session
    /// </summary>
    void UnmuteAllUsersFromAllChannels();

    /// <summary>
    /// Mute non-friend chat user with a poor reputation.
    /// </summary>
    /// <param name="remoteUser">The user to mute if they fail a reputation check</param>
    Windows::Foundation::IAsyncAction^
        MuteUserIfReputationIsBadAsync(
        _In_ Microsoft::Xbox::GameChat::ChatUser^ user
        );

    /// <summary>
    /// Indicates if the the title has mic focus
    /// </summary>
    property bool HasMicFocus { bool get(); }

    /// <summary>
    /// Returns the ChatPerformanceCounters object.
    /// The ChatPerformanceCounters object contains performance data for profiling.
    /// See ChatManagerSettings::PerformanceCountersEnabled to enable/disable collection of performance data.
    /// </summary>
    property Microsoft::Xbox::GameChat::ChatPerformanceCounters^ ChatPerformanceCounters { Microsoft::Xbox::GameChat::ChatPerformanceCounters^ get(); }

internal:
    std::shared_ptr<ChatDiagnostics> GetChatDiagnostics() { return m_chatDiagnostics; };

    bool DoesAudioDeviceCollectionsMatchExclusiveDevices( 
        _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices1, 
        _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices2 
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnDebugMessageHandler( 
        _In_ Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args 
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnChatPacketReadyHandler( 
        _In_ Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args 
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnChatSessionStateChangedHandler( 
        _In_ Windows::Xbox::Chat::IChatSessionState^ chatSessionState 
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnRemoteUserReadyToAddHandler( 
        _In_ uint8 channelIndex,
        _In_ Windows::Xbox::System::IUser^ remoteUser,
        _In_ Platform::Object^ remoteUniqueConsoleIdentifier,
        _In_ bool hasAddedRemoteUserToLocalChatSession
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnRemoteUserReadyToRemoveHandler( 
        _In_ uint8 channelIndex,
        _In_ Platform::String^ remoteXboxUserId
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    bool OnCompareUniqueConsoleIdentifiersHandler( 
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier1, 
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier2 
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    Windows::Storage::Streams::IBuffer^ OnPreEncodeAudioBufferHandler( 
        _In_ Windows::Storage::Streams::IBuffer^ buffer, 
        _In_ Windows::Xbox::Chat::IFormat^ audioFormat,
        _In_ Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    Windows::Storage::Streams::IBuffer^ OnPostDecodeAudioBufferHandler( 
        _In_ Windows::Storage::Streams::IBuffer^ buffer, 
        _In_ Windows::Xbox::Chat::IFormat^ audioFormat,
        _In_ Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers
        );

    /// <summary>
    /// Internal event handler
    /// </summary>
    void OnChatManagerSettingsChangedHandler();

private:

    /// <summary>
    /// Internal helper function to initialize the chat manager class using the specified ChatSession period 
    /// </summary>
    /// <param name="chatSessionPeriod">
    /// A ChatSessionPeriod enum which represents the chat session period in milliseconds.  
    /// This defines how big the chat capture buffers will be
    /// </param>
    void Initialize(
        _In_ ChatSessionPeriod chatSessionPeriod
        );

    /// <summary>
    /// Internal helper function to send all local users to a remote console
    /// </summary>
    void SendLocalUsersToRemoteConsole( _In_ Platform::Object^ remoteUniqueConsoleIdentifier );

    /// <summary>
    /// Internal helper function to log a comment
    /// </summary>
    void LogComment(
        _In_ Platform::String^ message
        );

    /// <summary>
    /// Internal helper function to log a comment with an error string
    /// </summary>
    void LogCommentWithError(
        _In_ Platform::String^ message, 
        _In_ HRESULT hr
        );

    /// <summary>
    /// Internal helper function to log a formated comment
    /// </summary>
    void LogCommentFormat( 
        _In_ LPCWSTR strMsg, ... 
        );

    /// <summary>
    /// Helper function to convert from ChatSessionPeriod to uint32 milliseconds 
    /// </summary>
    /// <param name="args">Returns a uint32 milliseconds</param>
    uint32 ConvertChatSessionPeriodToMilliseconds( _In_ ChatSessionPeriod chatSessionPeriod );

    /// <summary>
    /// Helper function to compare lists of audio devices to mismatches
    /// </summary>
    bool ChatManager::DoAudioDeviceCollectionsMatch( 
        _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices1, 
        _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices2 
        );

private:
    std::shared_ptr<FactoryCache> m_factoryCache;
    ChatClient^ m_chatClient;
    ChatAudioThread^ m_chatAudioThread;
    ChatNetwork^ m_chatNetwork;
    ChatManagerSettings^ m_chatManagerSettings;
    Platform::WeakReference m_chatManagerEventHandler;
    std::shared_ptr<ChatDiagnostics> m_chatDiagnostics;
    std::map<Platform::String^, Microsoft::Xbox::Services::Social::XboxSocialRelationshipResult^> m_socialRelationships;
};

}}}

#endif