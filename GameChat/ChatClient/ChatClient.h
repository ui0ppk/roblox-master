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

#define NUMBER_OF_CHAT_BUFFER_LEVELS 16
#define NUMBER_OF_CHAT_CHANNELS 256

#if TV_API

// Forward declare
namespace Microsoft { namespace Xbox { namespace GameChat { ref class ChatAudioThread; } } }
namespace Microsoft { namespace Xbox { namespace GameChat { ref class ChatManager; } } }

namespace Microsoft {
namespace Xbox {
namespace GameChat {

struct ConsoleNameIdentifierPair
{
    Platform::Object^ uniqueConsoleIdentifier;
    uint8 consoleName;
};

/// <summary>
/// ChatClient class implements the guts of the chat system capture / render interface.
/// This class handles management of the chat session, adding and removing 
/// participants from the chat channel as appropriate.
/// </summary>
ref class ChatClient sealed
{
public:
    ChatClient(
        _In_ uint32 chatPeriodInMilliseconds,
        _In_ ChatManager^ chatManager,
        _In_ ChatManagerSettings^ chatManagerSettings
        );

    Windows::Xbox::Chat::IChatSession^ GetChatSession();

    void SetChatAudioThread( 
        _In_ ChatAudioThread^ chatAudioThread
        );

    void AddNewChannel();

    Microsoft::Xbox::GameChat::ChatUser^ AddUserToChatChannel(
        _In_ uint8 channelIndex, 
        _In_ Windows::Xbox::System::IUser^ user, 
        _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
        _In_ bool isLocal,
        _In_opt_ Windows::Xbox::Chat::IChatParticipant^ chatParticipantToUpdate
        );

    Windows::Xbox::Chat::IChatParticipant^ GetChatParticipantFromChatSession(
        _In_ Platform::String^ xboxUserId,
        _In_ UINT channelIndex
        );

    void RemoveAllUsers();

    /// <summary>
    /// Remove all users from the channel that are on a given console.  Used when
    /// console connectivity is lost
    /// </summary>
    void RemoveRemoteConsole(
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier
        );

    void RemoveUserFromChatChannel(
        _In_ uint8 channelIndex,
        _In_ Windows::Xbox::System::IUser^ user,
        _In_ bool updateSessionState
        );

    void RemoveUserFromAllChatChannels(
        _In_ Windows::Xbox::System::IUser^ user
        );

    /// <summary>
    /// Takes an IUser as input, generates a ChatParitcipant and adds to the
    /// chat channel.  Note that device::user correlation is done here as well
    /// </summary>
    Windows::Foundation::Collections::IVectorView<unsigned int>^ GetAllUserIdsForConsoleId(
        _In_ Platform::Object^ uniqueConsoleIdentifier
        );

    Windows::Xbox::Chat::IChatParticipant^ GetChatParticipantFromUserId(
        _In_ unsigned int userId
        );

    /// <summary>
    /// Returns NUMBER_OF_CHAT_CHANNELS if the user doesn't exists
    /// </summary>
    UINT GetChatChannelIndexFromUserId(
        _In_ unsigned int userId
        );

    Windows::Foundation::Collections::IVectorView<ChatUser^>^ GetChatUsers();

internal:
    void LogCommentFormat(
        _In_ LPCWSTR strMsg, ...
        );

    std::vector<ChatUser^> GetChatUsersForCaptureSourceId(
        _In_ Platform::String^ captureSourceId
        );

    std::vector<unsigned int> GetUserIdsForDevice(
        _In_ Platform::String^ captureSourceId
        ); 

    void ChangeMuteStateForChatParticipantFromAllChannels(
        _In_ unsigned int userId,
        _In_ float volume
        );

    ChatUser^ GetChatUserForUserId(
        _In_ unsigned int userId
        );

    void RemoveUserFromChatChannelHelper( 
        _In_ Windows::Xbox::Chat::IChatChannel^ channel, 
        _In_ Windows::Xbox::System::IUser^ user,
        _In_ uint8 channelIndex,
        _In_ bool removeFromAllChannels
        );

    void UpdateSessionState();

    /// <summary>
    /// We set all chat users to NotTalking, and let the capture and render source
    /// set the talking mode in the AudioThreadDoWork update.
    /// </summary>
    void ClearTalkingModeForAllChatUsers();

    /// <summary>
    /// Returns the local name of a remote console.  The name is uint8
    /// </summary>
    /// <param name="consoleIdentifier">The remote console to return the local name of</param>
    uint8 GetLocalNameOfRemoteConsole( 
        _In_ Platform::Object^ consoleIdentifier 
        );
    
    /// <summary>
    /// Returns the list of names this local console has assigned to the remote consoles
    /// </summary>
    std::vector< std::shared_ptr<ConsoleNameIdentifierPair> > GetMyLocalNameOfRemoteConsolesCopy();

    bool DoesRemoteUniqueConsoleIdentifierExist(
        _In_ Platform::Object^ remoteUniqueConsoleIdentifier
        );

private:
    void LogComment(
        _In_ Platform::String^ message
        );

    void LogCommentWithError(
        _In_ Platform::String^ message, 
        _In_ HRESULT hr
        );

    bool DoesChannelContainUser(
        _In_ Windows::Foundation::Collections::IVector<Windows::Xbox::Chat::IChatParticipant^>^ participants, 
        _In_ Windows::Xbox::System::IUser^ user
        );

    void CorrelateDeviceToUser(
        _In_ Platform::String^ deviceId, 
        _In_ unsigned int userId,
        _In_ bool isSharedDevice
        ); 

    /// <summary>
    /// Called by most of the event handlers when a player or device is added or
    /// removed, updates the session state (above)
    /// </summary>
    void ChatSessionStateChanged(
        _In_ Windows::Xbox::Chat::IChatSession^ chatSession, 
        _In_ Windows::Xbox::Chat::ChatSessionStateChangeReason reason
        );

    Windows::Xbox::Chat::IChatParticipant^ AddUserToChatSessionChannel( 
        _In_ uint8 channelIndex, 
        _In_ Windows::Xbox::System::IUser^ user 
        );

    void AddUserToAudioDevicesMap( 
        _In_ Windows::Xbox::System::IUser^ user 
        );

    Microsoft::Xbox::GameChat::ChatUser^ 
    AddUserToUserDataMap( 
        _In_ Windows::Xbox::System::IUser^ user,
        _In_ uint8 channelIndex,
        _In_ bool isLocal,
        _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
        _In_ Windows::Xbox::Chat::IChatParticipant^ chatParticipant
        );

    void ChatClient::RemoveUserFromUserDataMap( 
        _In_ Windows::Xbox::System::IUser^ user,
        _In_ uint8 channelIndex,
        _In_ bool removeFromAllChannels
        );

    void RemoveUserIdFromAudioCaptureSourceIdMap( 
        _In_ Windows::Xbox::System::IUser^ user
        );

    void RemoveLocalNameOfRemoteConsole(
        _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
        );

    bool DoesChannelExist(
        _In_ uint8 channelIndex
        );

    bool CallerLock_GetUnusedLocalNameOfRemoteConsole( _Out_ uint8& localNameOfRemoteConsole );

    bool IsLocalNameTaken( uint8 localName );

private:
    ~ChatClient();
    Windows::Foundation::EventRegistrationToken m_tokenStateChangedEvent;

    Concurrency::critical_section m_chatSessionLock; // Locks down m_chatSession
    Concurrency::critical_section m_audioCaptureSourceIdToUserIdLock;
    Concurrency::critical_section m_userIdsToUserDataLock;
    Concurrency::critical_section m_localNameOfRemoteConsolesLock;

    Windows::Xbox::Chat::IChatSession^ m_chatSession;
    std::map<UINT, UINT> m_userIdsToBufferSizes;
    Platform::WeakReference m_chatManager;
    ChatManagerSettings^ m_chatManagerSettings;

    std::vector< std::shared_ptr<ConsoleNameIdentifierPair> > m_localNameOfRemoteConsoles;

    std::map<Platform::String^, std::hash_set<unsigned int>> m_audioCaptureSourceIdToUserId;
    std::map<unsigned int, ChatUser^> m_userIdsToUserData;
    Platform::WeakReference m_chatAudioThread;
};

}}}

#endif