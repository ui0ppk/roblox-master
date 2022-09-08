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
#include "ChatClient.h"
#include "Clock.h"
#include "StringUtils.h"
#include "BufferUtils.h"
#include "ChatManagerEvents.h"
#include "ChatManager.h"
#include <Audioclient.h>

#if TV_API

using namespace Windows::Xbox::Chat;
using namespace Windows::Xbox::System;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Concurrency;
using namespace Microsoft::WRL::Details;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ChatClient::ChatClient(
    _In_ uint32 chatPeriodInMilliseconds,
    _In_ ChatManager^ chatManager,
    _In_ ChatManagerSettings^ chatManagerSettings
    ) :
    m_chatManager(chatManager),
    m_chatManagerSettings( chatManagerSettings )
{
    m_userIdsToUserData = std::map<unsigned int, ChatUser^>();
    m_audioCaptureSourceIdToUserId = std::map<Platform::String^, std::hash_set<unsigned int>>();

    TimeSpan chatPeriod;
    chatPeriod.Duration = chatPeriodInMilliseconds * 10000; // milliseconds to hundred nanoseconds
    m_chatSession = ref new ChatSession(chatPeriod, chatManagerSettings->AudioThreadAffinityMask, ChatFeatures::Default);

    // Hook up the state changed handler

    Platform::WeakReference wr(this);
    m_tokenStateChangedEvent = m_chatSession->StateChangedEvent += ref new ChatSessionStateChangedHandler(
        [ wr ]( IChatSession^ chatSession, ChatSessionStateChangeReason reason )
    {
        ChatClient^ chatClient = wr.Resolve<ChatClient>();
        if( chatClient != nullptr )
        {
            chatClient->ChatSessionStateChanged(chatSession, reason);
        }
    });
}

ChatClient::~ChatClient()
{
    CHAT_LOG_INFO_MSG(L"ChatClient::~ChatClient");

    if( m_chatSession != nullptr )
    {
        m_chatSession->StateChangedEvent -= m_tokenStateChangedEvent;
    }
}

bool ChatClient::DoesChannelContainUser( 
    _In_ Windows::Foundation::Collections::IVector<Windows::Xbox::Chat::IChatParticipant^>^ participants, 
    _In_ IUser^ user 
    )
{
    for ( UINT i = 0; i < participants->Size; ++i )
    {
        if( StringUtils::IsStringEqualCaseInsenstive(participants->GetAt( i )->User->XboxUserId, user->XboxUserId) )
        {
            return true;
        }
    }

    return false;
}

void ChatClient::LogCommentFormat( 
    _In_ LPCWSTR strMsg, ... 
    )
{
    va_list args;
    va_start(args, strMsg);
    LogComment(StringUtils::GetStringFormat(strMsg, args));
    va_end(args);
}

Microsoft::Xbox::GameChat::ChatUser^ ChatClient::AddUserToChatChannel(
    _In_ uint8 channelIndex, 
    _In_ IUser^ user, 
    _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
    _In_ bool isLocal,
    _In_opt_ Windows::Xbox::Chat::IChatParticipant^ chatParticipantToUpdate
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );
    CHAT_THROW_INVALIDARGUMENT_IF( channelIndex >= NUMBER_OF_CHAT_CHANNELS );

    Microsoft::Xbox::GameChat::ChatUser^ chatUserAddedOrUpdated = nullptr;

    {
        Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

        // If the channel index doesn't exist, add 0-channelIndex channels first.
        int channelsNeeded = channelIndex + 1;
        int channelsExisting = m_chatSession->Channels->Size;
        int channelsToAdd = 0;
        if (channelsExisting < channelsNeeded)
        {
            channelsToAdd = abs(channelsExisting - channelsNeeded);
        }

        for (int i = 0; i < channelsToAdd; i++)
        {
            m_chatSession->Channels->Append( ref new ChatChannel() );
        }
        
        IChatChannel^ channel = m_chatSession->Channels->GetAt(channelIndex);
        CHAT_THROW_HR_IF( channel == nullptr, E_UNEXPECTED ); // We must have a channel for channelIndex since we just created it above

        if( chatParticipantToUpdate != nullptr )
        {
            // If the audio devices don't match, that means the remote user packet contained an updated user 
            // and we should remove the user from the chat channel and add the user back

            bool removeFromAllChannels = false;
            RemoveUserFromChatChannelHelper(channel, chatParticipantToUpdate->User, channelIndex, removeFromAllChannels);
        }

        // Check for an existing ChatUser to use
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
        {
            if (StringUtils::IsStringEqualCaseInsenstive(chatUser->User->XboxUserId, user->XboxUserId) )
            {
                chatUserAddedOrUpdated = chatUser;
                break;
            }
        }

        // Verify the user isn't already in the channel to avoid adding the user twice
        if( false == DoesChannelContainUser( channel->Participants, user ) )
        {
            // Add a ChatParticipant to the channel
            Windows::Xbox::Chat::IChatParticipant^ chatParticipant = ref new ChatParticipant( user );
            channel->Participants->Append( chatParticipant );

            if( chatUserAddedOrUpdated != nullptr )
            {
                // If the ChatUser already exists, then append the channel to his channel list.
                chatUserAddedOrUpdated->AddChannelIndexToChannelList(channelIndex);                

                if( m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Info) )
                {
                    LogCommentFormat( L"AddUserToChatChannel: AddChannelIndexToChannelList: XUID: %s chatUser: 0x%0.8x", user->XboxUserId->Data(), chatUserAddedOrUpdated );
                }
            }
            else
            {
                // No pre-existing ChatUser found, so create one 
                chatUserAddedOrUpdated = AddUserToUserDataMap(user, channelIndex, isLocal, uniqueConsoleIdentifier, chatParticipant);

                if( m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Info) )
                {
                    LogCommentFormat( L"AddUserToChatChannel: AddUserToAudioDevicesMap: XUID: %s UserId: 0x%0.8x chatUser: 0x%0.8x", user->XboxUserId->Data(), user->Id, chatUserAddedOrUpdated );
                }
            }
        }
    }

    // The user may now have a new audio device, so link all of this user's audio devices to the userId
    AddUserToAudioDevicesMap(user);
    UpdateSessionState();

    return chatUserAddedOrUpdated;
}

uint8 ChatClient::GetLocalNameOfRemoteConsole( 
    _In_ Platform::Object^ consoleIdentifier 
    )
{
    // Note that the local console will not be added to this list
    bool matchFound = false;
    uint8 localName = 0;
    Concurrency::critical_section::scoped_lock lock(m_localNameOfRemoteConsolesLock);   

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        for each (std::shared_ptr<ConsoleNameIdentifierPair> uniqueConsole in m_localNameOfRemoteConsoles)
        {
            if( chatManager->OnCompareUniqueConsoleIdentifiersHandler(uniqueConsole->uniqueConsoleIdentifier, consoleIdentifier) )
            {
                matchFound = true;
                localName = uniqueConsole->consoleName;
                break;
            }
        }
    }

    if( !matchFound )
    {
        std::shared_ptr<ConsoleNameIdentifierPair> consoleNameIdentifierPair(new ConsoleNameIdentifierPair());
        consoleNameIdentifierPair->uniqueConsoleIdentifier = consoleIdentifier;
        bool success = CallerLock_GetUnusedLocalNameOfRemoteConsole(localName);
        if( success )
        {
            consoleNameIdentifierPair->consoleName = localName;
            m_localNameOfRemoteConsoles.push_back( consoleNameIdentifierPair );
        }
    }

    return localName;
}

std::vector< std::shared_ptr<ConsoleNameIdentifierPair> > ChatClient::GetMyLocalNameOfRemoteConsolesCopy()
{
    Concurrency::critical_section::scoped_lock lock(m_localNameOfRemoteConsolesLock);
    std::vector< std::shared_ptr<ConsoleNameIdentifierPair> > myLocalNameOfRemoteConsolesCopy( m_localNameOfRemoteConsoles );
    return myLocalNameOfRemoteConsolesCopy;
}

void ChatClient::RemoveUserFromAllChatChannels(
    _In_ IUser^ user
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

    {
        Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);
        UINT numChannels = m_chatSession->Channels->Size;
        for (uint8 channelIndex = 0; channelIndex < numChannels; channelIndex++)
        {
            if (DoesChannelExist(channelIndex))
            {
                IChatChannel^ channel = m_chatSession->Channels->GetAt(channelIndex);
                bool removeFromAllChannels = true;
                RemoveUserFromChatChannelHelper(channel, user, 0, removeFromAllChannels);
            }
        }
    }

    UpdateSessionState();
}

void ChatClient::RemoveUserFromChatChannel(
    _In_ uint8 channelIndex,
    _In_ IUser^ user,
    _In_ bool updateSessionState
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );
    CHAT_THROW_INVALIDARGUMENT_IF( channelIndex >= NUMBER_OF_CHAT_CHANNELS );

    {
        Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);
        if (DoesChannelExist(channelIndex))
        {
            IChatChannel^ channel = m_chatSession->Channels->GetAt(channelIndex);
            bool removeFromAllChannels = false;
            RemoveUserFromChatChannelHelper(channel, user, channelIndex, removeFromAllChannels);
        }
    }

    if( updateSessionState )
    {
        UpdateSessionState();
    }
}

void ChatClient::RemoveUserFromChatChannelHelper( 
    _In_ IChatChannel^ channel, 
    _In_ IUser^ user,
    _In_ uint8 channelIndex,
    _In_ bool removeFromAllChannels
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( channel );

    // Note: This function does not require a lock as the functions 
    // that call his helper already have a m_chatSessionLock.

    UINT numParticipants = channel->Participants->Size;
    for (UINT j=0; j < numParticipants; )
    {
        IChatParticipant^ participant  = channel->Participants->GetAt(j);
        if( participant != nullptr && 
            participant->User != nullptr &&
            StringUtils::IsStringEqualCaseInsenstive(participant->User->XboxUserId, user->XboxUserId) )
        {
            channel->Participants->RemoveAt(j);

            // Update the UserDataMap.
            RemoveUserFromUserDataMap(user, channelIndex, removeFromAllChannels);
            break;
        }
        else
        {
            j++;
        }
    }
}

void ChatClient::RemoveLocalNameOfRemoteConsole(
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_localNameOfRemoteConsolesLock);

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        bool found = false;
        auto iter = m_localNameOfRemoteConsoles.begin();
        for( ; iter != m_localNameOfRemoteConsoles.end(); iter++ )
        {
            std::shared_ptr<ConsoleNameIdentifierPair> iterConsoleNameIdentifier = *iter;
            if( chatManager->OnCompareUniqueConsoleIdentifiersHandler(iterConsoleNameIdentifier->uniqueConsoleIdentifier, uniqueRemoteConsoleIdentifier) )
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            m_localNameOfRemoteConsoles.erase(iter);
        }
    }
}

bool ChatClient::DoesChannelExist(
    _In_ uint8 channelIndex
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF( channelIndex >= NUMBER_OF_CHAT_CHANNELS );

    // Note: This function does not require a lock as the functions 
    // that call his helper already have a m_chatSessionLock.

    if (channelIndex >= m_chatSession->Channels->Size)
    {
        return false;
    }

    IChatChannel^ channel = m_chatSession->Channels->GetAt(channelIndex);
    return channel != nullptr;
}

IChatParticipant^ ChatClient::GetChatParticipantFromUserId(
    _In_ unsigned int userId
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

    // Note that this could be turned into a lookup map for more efficient lookup 
    // The map can be updated updated every time the session state changes

    for (UINT i=0; i < m_chatSession->Channels->Size; i++)
    {
        IChatChannel^ channel = m_chatSession->Channels->GetAt(i);

        for (UINT j=0; j < channel->Participants->Size; j++)
        {
            IChatParticipant^ participant = channel->Participants->GetAt(j);
            if( participant != nullptr && 
                participant->User != nullptr && 
                participant->User->Id == userId )
            {
                return participant;
            }
        }
    }

    return nullptr;
}

UINT ChatClient::GetChatChannelIndexFromUserId(
    _In_ unsigned int userId
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

    // Note that this could be turned into a lookup map for more efficient lookup 
    // The map can be updated updated every time the session state changes

    for (UINT i=0; i < m_chatSession->Channels->Size; i++)
    {
        IChatChannel^ channel = m_chatSession->Channels->GetAt(i);

        for (UINT j=0; j < channel->Participants->Size; j++)
        {
            IChatParticipant^ participant = channel->Participants->GetAt(j);
            if( participant != nullptr && 
                participant->User != nullptr && 
                participant->User->Id == userId )
            {
                return i;
            }
        }
    }

    return NUMBER_OF_CHAT_CHANNELS;
}

void ChatClient::ChangeMuteStateForChatParticipantFromAllChannels(
    _In_ unsigned int userId,
    _In_ float volume
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

    for (UINT i=0; i < m_chatSession->Channels->Size; i++)
    {
        IChatChannel^ channel = m_chatSession->Channels->GetAt(i);

        for (UINT j=0; j < channel->Participants->Size; j++)
        {
            IChatParticipant^ participant = channel->Participants->GetAt(j);
            if( participant != nullptr && 
                participant->User != nullptr && 
                participant->User->Id == userId )
            {
                participant->Volume = volume;
                break;
            }
        }
    }
}

void ChatClient::ChatSessionStateChanged(
    _In_ IChatSession^ chatSession, 
    _In_ ChatSessionStateChangeReason reason
    )
{
    UNREFERENCED_PARAMETER( chatSession );

    if (reason == ChatSessionStateChangeReason::MicrophoneFocusGained)
    {
        ChatAudioThread^ chatAudioThread = m_chatAudioThread.Resolve<ChatAudioThread>();
        if (chatAudioThread != nullptr)
        {
            chatAudioThread->HasMicFocus = true;
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
            TraceChatMicFocus( chatAudioThread->HasMicFocus );
#endif
        }     
    }

    UpdateSessionState();
}

void ChatClient::RemoveAllUsers()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);
    m_chatSession->Channels->Clear();
}

void ChatClient::RemoveRemoteConsole( 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(uniqueRemoteConsoleIdentifier);

    RemoveLocalNameOfRemoteConsole(uniqueRemoteConsoleIdentifier);

    Windows::Foundation::Collections::IVectorView<unsigned int>^ userIds = GetAllUserIdsForConsoleId( uniqueRemoteConsoleIdentifier );
    for each (unsigned int userId in userIds)
    {
        IChatParticipant^ chatUserToRemove = GetChatParticipantFromUserId(userId);
        if( chatUserToRemove != nullptr )
        {
            IUser^ userToRemove = chatUserToRemove->User;
            if( userToRemove != nullptr )
            {
                RemoveUserFromAllChatChannels(userToRemove);
            }
        }
    }
}

IChatParticipant^ ChatClient::GetChatParticipantFromChatSession( 
    _In_ Platform::String^ xboxUserId,
    _In_ UINT channelIndex
    )
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

    if (DoesChannelExist((uint8)channelIndex))
    {
        IChatChannel^ channel = m_chatSession->Channels->GetAt(channelIndex);
        for (UINT j=0; j < channel->Participants->Size; ++j)
        {
            IChatParticipant^ participant  = channel->Participants->GetAt(j);
            if( participant != nullptr &&
                participant->User != nullptr && 
                StringUtils::IsStringEqualCaseInsenstive(participant->User->XboxUserId, xboxUserId) )
            {
                return participant;
            }
        }
    }
    return nullptr;
}

Windows::Foundation::Collections::IVectorView<unsigned int>^ ChatClient::GetAllUserIdsForConsoleId( 
    _In_ Platform::Object^ uniqueConsoleIdentifier
    )
{
    Platform::Collections::Vector<unsigned int>^ userIds = ref new Platform::Collections::Vector<unsigned int>();

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( chatManager != nullptr )
    {
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
        {
            if( chatManager->OnCompareUniqueConsoleIdentifiersHandler(chatUser->UniqueConsoleIdentifier, uniqueConsoleIdentifier) )
            {
                unsigned int userId = chatUser->User->Id;
                userIds->Append(userId);
            }
        }
    }

    return userIds->GetView();
}

Windows::Foundation::Collections::IVectorView<ChatUser^>^ ChatClient::GetChatUsers()
{
    Concurrency::critical_section::scoped_lock lock(m_userIdsToUserDataLock);    

    Platform::Collections::Vector<ChatUser^>^ chatUsers = ref new Platform::Collections::Vector<ChatUser^>();
    for(std::map<unsigned int, ChatUser^>::iterator iter = m_userIdsToUserData.begin(); iter != m_userIdsToUserData.end(); ++iter)
    {
        ChatUser^ user =  iter->second;
        if( user != nullptr )
        {
            chatUsers->Append(user);
        }
    }

    return chatUsers->GetView();
}

ChatUser^ ChatClient::GetChatUserForUserId(
    _In_ unsigned int userId
    ) 
{
    Concurrency::critical_section::scoped_lock lock(m_userIdsToUserDataLock);    
    if (m_userIdsToUserData.find(userId) != m_userIdsToUserData.end())
    {
        return m_userIdsToUserData[userId];
    }
    return nullptr;
};

std::vector<unsigned int> ChatClient::GetUserIdsForDevice(
    _In_ Platform::String^ captureSourceId
    ) 
{
    std::vector<unsigned int> ids;

    Concurrency::critical_section::scoped_lock lock(m_audioCaptureSourceIdToUserIdLock);    

    if( m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Verbose) )
    {
        LogCommentFormat( L"GetUserIdsForDevice: Size:%d", m_audioCaptureSourceIdToUserId.size() );
        for(std::map<Platform::String^, std::hash_set<unsigned int>>::iterator iter = m_audioCaptureSourceIdToUserId.begin(); iter != m_audioCaptureSourceIdToUserId.end(); ++iter)
        {
            LogCommentFormat( L"GetUserIdsForDevice: captureSourceId: %s", iter->first->Data() );
            std::hash_set<unsigned int> hashSet = iter->second;
            for each (unsigned int userId in hashSet)
            {
                LogCommentFormat( L"GetUserIdsForDevice: userId: 0x%0.8x", userId );
            }
        }
    }


    if (m_audioCaptureSourceIdToUserId.find(captureSourceId) != m_audioCaptureSourceIdToUserId.end())
    {
        ids.insert( std::begin( ids ), std::begin( m_audioCaptureSourceIdToUserId[captureSourceId] ), std::end( m_audioCaptureSourceIdToUserId[captureSourceId] ) );
    }

    return std::move( ids );
};

void ChatClient::CorrelateDeviceToUser(
    _In_ Platform::String^ deviceId, 
    _In_ unsigned int userId,
    _In_ bool isSharedDevice
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_STRING_EMPTY( deviceId );

    Concurrency::critical_section::scoped_lock lock(m_audioCaptureSourceIdToUserIdLock);    

    if( !isSharedDevice )
    {
        // Exclusive devices like headset can only have one user tied to them so clear out old users.
        // This happens when 2 users have sign-in with Kinect enabled and swap controllers
        m_audioCaptureSourceIdToUserId[deviceId].clear();
    }

    if( m_audioCaptureSourceIdToUserId[deviceId].find(userId) == m_audioCaptureSourceIdToUserId[deviceId].end() )
    {
        m_audioCaptureSourceIdToUserId[deviceId].insert( userId );
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatCorrelateAudioDeviceToUser( 
        deviceId->Data(),
        userId,
        isSharedDevice,
        false
        );
#endif
}

IChatSession^ ChatClient::GetChatSession()
{
    return m_chatSession;
}

void ChatClient::UpdateSessionState()
{
    Concurrency::critical_section::scoped_lock lock(m_chatSessionLock);

    CHAT_LOG_INFO_MSG( L"Windows::Xbox::Chat::IChatSession::GetStateAsync starting" );
    auto asyncOp = m_chatSession->GetStateAsync();

    create_task(asyncOp)
    .then([this] (task<Windows::Xbox::Chat::IChatSessionState^> t)
    {
        try
        {
            CHAT_LOG_INFO_MSG( L"Windows::Xbox::Chat::IChatSession::GetStateAsync complete" );
            IChatSessionState^ chatSessionState = t.get();

            ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
            if( chatManager != nullptr )
            {
                chatManager->OnChatSessionStateChangedHandler( chatSessionState );
            }
        }
        catch ( Platform::COMException^ ex )
        {
            if (ex->HResult == (HRESULT)Windows::Xbox::Chat::ChatErrorStatus::NoMicrophoneFocus)
            {
                ChatAudioThread^ chatAudioThread = m_chatAudioThread.Resolve<ChatAudioThread>();
                if (chatAudioThread != nullptr)
                {
                    chatAudioThread->HasMicFocus = false;
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
                    TraceChatMicFocus( chatAudioThread->HasMicFocus );
#endif
                }
            }

            // ERROR NOT FOUND is expected
            if (ex->HResult != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            {                
                if (ex->HResult == AUDCLNT_E_DEVICE_IN_USE)
                {
                    // Package.appxmanifest should contain capabilities like this:
                    //
                    // <Capabilities>
                    //    <Capability Name="internetClientServer" />
                    //    <mx:Capability Name="kinectAudio"/>
                    //    <mx:Capability Name="kinectGamechat"/>
                    // </Capabilities>
                    LogCommentWithError( L"GetStateAsync failed.  This error often indicates missing kinectGamechat in package.appxmanifest", ex->HResult );
                }
                else
                {
                    LogCommentWithError( L"GetStateAsync", ex->HResult );
                }
            }
        }
    }).wait();
}

void ChatClient::RemoveUserFromUserDataMap( 
    _In_ IUser^ user,
    _In_ uint8 channelIndex,
    _In_ bool removeFromAllChannels
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

    Concurrency::critical_section::scoped_lock lock(m_userIdsToUserDataLock);
    if( !user->XboxUserId->IsEmpty() )
    {
        if (m_userIdsToUserData.find(user->Id) != m_userIdsToUserData.end())
        {
            ChatUser^ chatUser = m_userIdsToUserData[user->Id];
            if (removeFromAllChannels || chatUser->GetAllChannels()->Size == 1)
            {
                RemoveUserIdFromAudioCaptureSourceIdMap(user);

                // If the user is only in 1 channel, then remove him completely.
                m_userIdsToUserData.erase(user->Id);
            }
            else
            {
                // else, just remove the channel index from the channel list.
                if (chatUser)
                {
                    chatUser->RemoveChannelIndexFromChannelList(channelIndex);
                }
            }
        }
    }
}

void ChatClient::RemoveUserIdFromAudioCaptureSourceIdMap(
    _In_ IUser^ user
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

    Concurrency::critical_section::scoped_lock lock(m_audioCaptureSourceIdToUserIdLock);    
    for(std::map<Platform::String^, std::hash_set<unsigned int>>::iterator iter = m_audioCaptureSourceIdToUserId.begin(); iter != m_audioCaptureSourceIdToUserId.end(); ++iter)
    {
        m_audioCaptureSourceIdToUserId[iter->first].erase( user->Id );

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        TraceChatCorrelateAudioDeviceToUser( 
            iter->first->Data(),
            user->Id,
            false,
            true
            );
#endif
    }
}

Microsoft::Xbox::GameChat::ChatUser^ 
ChatClient::AddUserToUserDataMap( 
    _In_ IUser^ user,
    _In_ uint8 channelIndex,
    _In_ bool isLocal,
    _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
    _In_ Windows::Xbox::Chat::IChatParticipant^ chatParticipant
    )
{
    CHAT_THROW_E_POINTER_IF_NULL( user );
    CHAT_THROW_E_POINTER_IF_NULL( chatParticipant );

    // Add user meta data for lookup
    Concurrency::critical_section::scoped_lock lock(m_userIdsToUserDataLock);
    if ( m_userIdsToUserData.find( user->Id ) == m_userIdsToUserData.end() )
    {
        m_userIdsToUserData[user->Id] = ref new ChatUser(user->XboxUserId, isLocal, uniqueConsoleIdentifier, chatParticipant);
    }

    ChatUser^ chatUser = m_userIdsToUserData[user->Id];
    m_userIdsToUserData[user->Id]->AddChannelIndexToChannelList(channelIndex);
    return chatUser;
}

void ChatClient::AddUserToAudioDevicesMap( 
    _In_ IUser^ user 
    )
{
    CHAT_THROW_E_POINTER_IF_NULL( user );

    auto audioDevices = user->AudioDevices;
    for (UINT i = 0; i < audioDevices->Size; i++)
    {
        // Store relation of device ids to user id
        auto device = audioDevices->GetAt(i);
        auto deviceId = device->Id;
        auto userId = user->Id;
        bool isSharedDevice = (device->Sharing == Windows::Xbox::System::AudioDeviceSharing::Shared);
        CorrelateDeviceToUser(deviceId, userId, isSharedDevice);
    }
}

std::vector<ChatUser^> ChatClient::GetChatUsersForCaptureSourceId(
    _In_ Platform::String^ captureSourceId
    )
{
    std::vector<ChatUser^> chatUsers;

    auto userIdsForDevice = GetUserIdsForDevice( captureSourceId );
    for ( auto uid : userIdsForDevice )
    {
        auto user = GetChatUserForUserId( uid );
        if (user != nullptr)
        {
            chatUsers.push_back(user);
        }
    }

    return chatUsers;
}

void ChatClient::LogComment(
    _In_ Platform::String^ message
    )
{
    LogCommentWithError(message, S_OK);
}

void ChatClient::LogCommentWithError(
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

void ChatClient::ClearTalkingModeForAllChatUsers()
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        chatUser->SetTalkingMode(ChatUserTalkingMode::NotTalking);
    }
}

bool ChatClient::CallerLock_GetUnusedLocalNameOfRemoteConsole(
    _Out_ uint8& localNameOfRemoteConsole
    )
{
    // Note that the local console will not be added to this list
    unsigned int localName = 0;

    // Not taking lock here since caller will hold the lock 
    // Concurrency::critical_section::scoped_lock lock(m_localNameOfRemoteConsolesLock);   

    // Look for the first unused name in our list of remote console names.  0 is reserved for local console.
    bool foundUnusedLocalName = true;
    for( localName=1; localName<=0xFF; localName++ )  
    {
        foundUnusedLocalName = !IsLocalNameTaken( static_cast<uint8>(localName) );

        // If we found a free slot then stop.  We will hand that unused name.  Otherwise keep looking
        if( foundUnusedLocalName )
        {
            break;
        }
    }

    if( !foundUnusedLocalName )
    {
        LogCommentWithError( L"Couldn't find unused local name for remote console.  Should not happen due limited of number of network connections allowed", E_UNEXPECTED );
        return false;
    }

    localNameOfRemoteConsole = static_cast<uint8>(localName);
    return true;
}

void ChatClient::SetChatAudioThread( 
    _In_ ChatAudioThread^ chatAudioThread 
    )
{
    m_chatAudioThread = chatAudioThread;
}

bool ChatClient::IsLocalNameTaken( uint8 localName )
{
    bool isTaken = false;
    for each (std::shared_ptr<ConsoleNameIdentifierPair> uniqueConsole in m_localNameOfRemoteConsoles)
    {
        if( uniqueConsole->consoleName == localName )
        {
            isTaken = true;
            break;
        }
    }

    return isTaken;
}

bool ChatClient::DoesRemoteUniqueConsoleIdentifierExist(
    _In_ Platform::Object^ remoteUniqueConsoleIdentifier
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( remoteUniqueConsoleIdentifier );

    ChatManager^ chatManager = m_chatManager.Resolve<ChatManager>();
    if( remoteUniqueConsoleIdentifier != nullptr &&
        chatManager != nullptr )
    {
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
        {
            if( chatManager->OnCompareUniqueConsoleIdentifiersHandler(chatUser->UniqueConsoleIdentifier, remoteUniqueConsoleIdentifier) )
            {
                return true;
            }
        }
    }

    return false;
}

}}}

#endif