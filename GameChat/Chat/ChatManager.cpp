//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "StringUtils.h"
#include "AudioDeviceIDMapper.h"
#include "ChatManagerEvents.h"
#include "ChatAudioThread.h"
#include "ChatClient.h"
#include "ChatNetwork.h"
#include "ChatManager.h"

#if TV_API

using namespace Windows::Foundation;
using namespace concurrency;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

void SetHardwareOffload(bool enabled)
{
    PCWSTR chatPath = L"C:\\windows\\system32\\Chat.dll";
    HMODULE ChatModule = LoadLibraryEx(chatPath, NULL, 0);
             
    if (ChatModule == NULL)
        return;
             
    void (*SetHardwareOffloadEnabled)(bool) = 
                (void (__cdecl *)(bool))GetProcAddress(ChatModule, "SetHardwareCodecEnabled");
             
    SetHardwareOffloadEnabled(enabled);
             
    return;
}

ChatManager::ChatManager() 
{
    Initialize( ChatSessionPeriod::ChatPeriodOf40Milliseconds );
}

ChatManager::ChatManager(
    _In_ ChatSessionPeriod chatSessionPeriod
    ) 
{
    Initialize( chatSessionPeriod );
}

void ChatManager::Initialize(
    _In_ ChatSessionPeriod chatSessionPeriod
    )
{
    // Enable hardware offloaded encoding/decoding by default
    SetHardwareOffload(TRUE);

    m_factoryCache.reset( new FactoryCache() );
    m_chatDiagnostics.reset( new ChatDiagnostics() );

    m_chatManagerSettings = ref new ChatManagerSettings(
        this
        );

    uint32 chatSessionPeriodInMilliseconds = ConvertChatSessionPeriodToMilliseconds(chatSessionPeriod);
    m_chatClient = ref new ChatClient(
        chatSessionPeriodInMilliseconds,
        this,
        m_chatManagerSettings
        );

    m_chatAudioThread = ref new ChatAudioThread(
        m_chatManagerSettings, 
        m_chatClient, 
        this,
        m_factoryCache
        );

    m_chatNetwork = ref new ChatNetwork( 
        m_chatAudioThread,
        m_chatClient,
        this,
        m_factoryCache,
        m_chatManagerSettings
        );

    m_chatAudioThread->SetChatNetwork( m_chatNetwork );
    m_chatClient->SetChatAudioThread( m_chatAudioThread );
}

ChatManagerSettings^ ChatManager::ChatSettings::get()
{
    return m_chatManagerSettings;
}

ChatManager::~ChatManager()
{
    CHAT_LOG_INFO_MSG(L"ChatManager::~ChatManager");

    m_chatAudioThread->Shutdown();

    m_chatAudioThread = nullptr;
    m_chatClient = nullptr;
    m_chatNetwork = nullptr;
}

void ChatManager::LogComment(
    _In_ Platform::String^ message
    )
{
    LogCommentWithError(message, S_OK);
}

void ChatManager::LogCommentWithError(
    _In_ Platform::String^ message, 
    _In_ HRESULT hr
    )
{
    DebugMessageEventArgs^ args = ref new DebugMessageEventArgs(message, hr);
    OnDebugMessage( this, args );
}

void ChatManager::LogCommentFormat( 
    _In_ LPCWSTR strMsg, ... 
    )
{
    va_list args;
    va_start(args, strMsg);
    LogComment(StringUtils::GetStringFormat(strMsg, args));
    va_end(args);
}

void ChatManager::OnChatPacketReadyHandler( 
    _In_ Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args 
    )
{
    if( m_chatManagerSettings->PerformanceCountersEnabled ) 
    { 
        m_chatAudioThread->ChatPerformanceCounters->AddPacketBandwidth( false, args->PacketBuffer->Length );
    }

    OnOutgoingChatPacketReady( this, args );
}

void ChatManager::OnDebugMessageHandler( 
    _In_ Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args 
    )
{
    OnDebugMessage( this, args );
}

Windows::Foundation::IAsyncAction^ 
    ChatManager::RemoveLocalUserFromChatChannelAsync( 
    _In_ uint8 channelIndex,
    _In_ Windows::Xbox::System::IUser^ user 
    )
{
    return create_async( [this, channelIndex, user]()
    {
        CHAT_LOG_INFO_MSG(L"ChatManager::RemoveLocalUserFromChatChannel");
        CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

        ChatUser^ chatUser = m_chatClient->GetChatUserForUserId(user->Id);
        if( chatUser != nullptr )
        {
            m_chatClient->RemoveUserFromChatChannel(channelIndex, user, true);
            m_chatNetwork->CreateChatUserRemovedPacket(channelIndex, chatUser);
        }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        TraceChatRemoveLocalUserFromChatChannel(
            user == nullptr ? L"" : user->XboxUserId->Data(),
            channelIndex
            );
        m_chatDiagnostics->TraceChatUserAndAudioDevices( user );
#endif
    });
}

Windows::Foundation::IAsyncAction^
    ChatManager::RemoveRemoteConsoleAsync( 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier
    )
{
    return create_async( [this, uniqueRemoteConsoleIdentifier]()
    {
        LogComment(L"ChatManager::RemoveAllUsersWithConsoleId");
        CONSOLE_NAME localNameOfRemoteConsole = m_chatClient->GetLocalNameOfRemoteConsole(uniqueRemoteConsoleIdentifier);
        m_chatAudioThread->RemoveRemoteConsole(localNameOfRemoteConsole);
        m_chatNetwork->GetAudioDeviceIDMapper()->RemoveRemoteConsole(localNameOfRemoteConsole);
        m_chatClient->RemoveRemoteConsole(uniqueRemoteConsoleIdentifier);

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        TraceChatRemoveRemoteConsole(
            EventEnabledChatRemoveRemoteConsole() ? m_chatDiagnostics->GetDiagnosticNameForConsole( this, uniqueRemoteConsoleIdentifier ) : 0
            );
#endif
    });
}

Windows::Foundation::IAsyncAction^ 
    ChatManager::AddLocalUserToChatChannelAsync( 
    _In_ uint8 channelIndex,
    _In_ Windows::Xbox::System::IUser^ user
    )
{
    return create_async( [this, channelIndex, user]()
    {
        CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

        bool isLocal = true;
        Platform::Object^ uniqueConsoleIdentifier = nullptr;

        ChatUser^ chatUser = m_chatClient->AddUserToChatChannel(channelIndex, user, uniqueConsoleIdentifier, isLocal, nullptr);
        if( chatUser != nullptr )
        {
            // For each that I know of, send them a UserAddedMessage packet.
            std::vector< std::shared_ptr<ConsoleNameIdentifierPair> > myLocalNameOfRemoteConsolesCopy = m_chatClient->GetMyLocalNameOfRemoteConsolesCopy();
            for each (std::shared_ptr<ConsoleNameIdentifierPair> consoleNameIdentifierPair in myLocalNameOfRemoteConsolesCopy)
            {
                m_chatNetwork->CreateChatUserPacket(channelIndex, chatUser, consoleNameIdentifierPair->uniqueConsoleIdentifier);
            }
        }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        TraceChatAddLocalUserToChatChannel(
            user == nullptr ? L"" : user->XboxUserId->Data(),
            channelIndex
            );
        m_chatDiagnostics->TraceChatUserAndAudioDevices( user );
#endif
    });
}

void ChatManager::HandleNewRemoteConsole( 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
    )
{
    SendLocalUsersToRemoteConsole(uniqueRemoteConsoleIdentifier);

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatHandleNewRemoteConsole(
        EventEnabledChatHandleNewRemoteConsole() ? m_chatDiagnostics->GetDiagnosticNameForConsole( this, uniqueRemoteConsoleIdentifier ) : 0
        );
#endif
}

Microsoft::Xbox::GameChat::ChatMessageType ChatManager::ProcessIncomingChatMessage( 
    _In_ Windows::Storage::Streams::IBuffer^ chatPacket, 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier 
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( uniqueRemoteConsoleIdentifier );

    Microsoft::Xbox::GameChat::ChatMessageType chatMessageType = m_chatNetwork->ProcessIncomingChatMessage(
        chatPacket, 
        uniqueRemoteConsoleIdentifier,
        m_chatClient,
        m_chatAudioThread,
        this
        );

    return chatMessageType;
}

void ChatManager::OnChatSessionStateChangedHandler( 
    _In_ Windows::Xbox::Chat::IChatSessionState^ chatSessionState 
    )
{
    m_chatAudioThread->SetChatSessionState(chatSessionState, m_chatClient);
}

void ChatManager::OnRemoteUserReadyToAddHandler( 
    _In_ uint8 channelIndex,
    _In_ Windows::Xbox::System::IUser^ remoteUser,
    _In_ Platform::Object^ remoteUniqueConsoleIdentifier,
    _In_ bool hasAddedRemoteUserToLocalChatSession
    )
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        // Look at each non-user who doesn't match the XboxUserId and see if they have the same exclusive audio devices.
        // If the do, then remove them since non-shared devices aren't allowed to have more than one user
        if (chatUser != nullptr && !StringUtils::IsStringEqualCaseInsenstive(chatUser->XboxUserId, remoteUser->XboxUserId) )
        {
            bool foundExclusiveDeviceMatch = DoesAudioDeviceCollectionsMatchExclusiveDevices( remoteUser->AudioDevices, chatUser->User->AudioDevices );
            if( foundExclusiveDeviceMatch )
            {
                // Remove this user 
                m_chatClient->RemoveUserFromChatChannel(
                    channelIndex, 
                    chatUser->User, 
                    false // don't bother updating the session state yet as it will be done during the AddUserToChatChannel call
                    );
            }
        }
    }

    Microsoft::Xbox::GameChat::ChatUser^ chatUser = nullptr;
    bool isLocal = false; 

    // Now that we have a IUser, pass it to the chat client to handle
    chatUser = m_chatClient->AddUserToChatChannel(
        channelIndex,
        remoteUser,
        remoteUniqueConsoleIdentifier,
        isLocal,
        m_chatClient->GetChatParticipantFromChatSession( remoteUser->XboxUserId, (UINT)channelIndex )
        );

    // Rebuild the list of CHAT_AUDIO_THREAD_STATE objects associated with this remote console since the chatUsers on this console has changed
    CONSOLE_NAME localNameOfRemoteConsole = m_chatClient->GetLocalNameOfRemoteConsole(remoteUniqueConsoleIdentifier);
    m_chatAudioThread->RemoveRemoteConsole(localNameOfRemoteConsole);  
    LogCommentFormat( L"OnRemoteUserReadyToAddHandler: AddingUser: XboxUserId %s: localNameOfRemoteConsole: %d", remoteUser->XboxUserId->Data(), localNameOfRemoteConsole );

    // If the remote user does not have our user data, then send it to him.
    if (hasAddedRemoteUserToLocalChatSession == false)
    {
        SendLocalUsersToRemoteConsole(remoteUniqueConsoleIdentifier);
    }

    if( m_chatManagerSettings->AutoMuteBadReputationUsers )
    {
        // Anonymous (non-friend) users with a bad reputation should be muted by default
        MuteUserIfReputationIsBadAsync(chatUser);
    }
}

Windows::Foundation::IAsyncAction^
    ChatManager::MuteUserIfReputationIsBadAsync(
    _In_ Microsoft::Xbox::GameChat::ChatUser^ remoteUser
    )
{
    return create_async( [this, remoteUser] ()
    {
        CHAT_THROW_INVALIDARGUMENT_IF_NULL( remoteUser );
        CHAT_LOG_INFO_MSG(L"ChatManager::MuteUserIfReputationIsBadAsync");

        std::vector<Microsoft::Xbox::GameChat::ChatUser^> localUsers;

        // Find the local users connected to chat.  This list will be used to verify if a new remote user is a Favorite.
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
        {
            if (chatUser != nullptr && chatUser->IsLocal)
            {
                if (StringUtils::IsStringEqualCaseInsenstive(chatUser->XboxUserId, remoteUser->XboxUserId))
                {
                    // The new user is also a local user which shouldn't happen.
                    assert(false);
                    return;
                }

                localUsers.push_back(chatUser);
            }
        }

        if (localUsers.size() == 0)
        {
            // There should always be at least one local user
            assert(false);
            return;
        }

        // Check local users' Favorites for the inclusion of the new user
        for (std::vector<Microsoft::Xbox::GameChat::ChatUser^>::iterator itr = localUsers.begin(); itr != localUsers.end(); itr++)
        {
            Microsoft::Xbox::Services::Social::XboxSocialRelationshipResult^ socialResult = nullptr;
            std::map<Platform::String^, Microsoft::Xbox::Services::Social::XboxSocialRelationshipResult^>::iterator search;

            // Check if we've already performed the social query for this user
            search = m_socialRelationships.find((*itr)->XboxUserId);
            if (search != m_socialRelationships.end())
            {
                socialResult = (*search).second;
            }
            else
            {
                Microsoft::Xbox::Services::XboxLiveContext^ context = ref new Microsoft::Xbox::Services::XboxLiveContext(static_cast<Windows::Xbox::System::User^>((*itr)->User));

                // Query the Social service for the list of social relationships
                create_task(
                    context->SocialService->GetSocialRelationshipsAsync(
                        Microsoft::Xbox::Services::Social::SocialRelationship::All
                        )
                    ).then( [&, this] (task<Microsoft::Xbox::Services::Social::XboxSocialRelationshipResult^> taskResult)
                {
                    try
                    {
                        socialResult = taskResult.get();
                        m_socialRelationships[(*itr)->XboxUserId] = socialResult;
                    }
                    catch(Platform::Exception^ ex)
                    {
                        // The social request failed so skip this user and assume they're not friends
                        // Fallthrough
                    }
                }).wait();
            }

            // Check the social graph results
            if (socialResult != nullptr && socialResult->TotalCount > 0)
            {
                Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Social::XboxSocialRelationship^>^ results = socialResult->Items;

                // Check for our local users in the list of Favorites
                for each(Microsoft::Xbox::Services::Social::XboxSocialRelationship^ relation in results)
                {
                    if (StringUtils::IsStringEqualCaseInsenstive(relation->XboxUserId, remoteUser->XboxUserId))
                    {
                        // The new user is a friend of a local user so we don't need to make the reputation check
                        if (relation->IsFollowingCaller)
                        {
                            return;
                        }
                    }
                }
            }
        }

        bool shouldMute = false;
        Microsoft::Xbox::Services::XboxLiveContext^ context = ref new Microsoft::Xbox::Services::XboxLiveContext(static_cast<Windows::Xbox::System::User^>(localUsers[0]->User));

        // Now that we know this is an anonymous user we need to check their reputation.
        // We use the context of a local user to make the request so we have proper permissions.
        create_task(
            context->UserStatisticsService->GetSingleUserStatisticsAsync(
                remoteUser->XboxUserId,   // User's XUID
                STATISTICS_SERVICE_GUID,  // System constant GUID for Statistics service
                "OverallReputationIsBad"  // Stat to query
                )
            ).then( [&, this] (task<Microsoft::Xbox::Services::UserStatistics::UserStatisticsResult^> taskResult)
        {
            try
            {
                Microsoft::Xbox::Services::UserStatistics::UserStatisticsResult^ result = taskResult.get();
                Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::UserStatistics::ServiceConfigurationStatistic^>^ values = result->ServiceConfigurationStatistics;

                // Look through returned statistic values for the overall reputation
                for each(Microsoft::Xbox::Services::UserStatistics::ServiceConfigurationStatistic^ serviceStat in values)
                {
                    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::UserStatistics::Statistic^>^ stats = serviceStat->Statistics;
                    for each(Microsoft::Xbox::Services::UserStatistics::Statistic^ stat in stats)
                    {
                        if (StringUtils::IsStringEqualCaseInsenstive(stat->StatisticName, "OverallReputationIsBad") && 
                            StringUtils::IsStringEqualCaseInsenstive(stat->Value, "1"))
                        {
                            // The user has a bad rep so we'll mute them by default
                            shouldMute = true;
                            return;
                        }
                    }
                }
            }
            catch(Platform::Exception^ ex)
            {
                // There was an error querying the Statistics service.  Assume the new user does not need to be muted
                // Fallthrough
            }
        }).wait();

        // So the final check for a non-favorite with a bad rep
        if (shouldMute)
        {
            // Since this is run async and requires multiple service calls, the user may
            // be gone or disconnected so make sure they're still a current user.
            Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
            for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
            {
                if (chatUser && StringUtils::IsStringEqualCaseInsenstive(chatUser->XboxUserId, remoteUser->XboxUserId))
                {
                    MuteUserFromAllChannelsPermanently(chatUser);
                    break;
                }
            }
        }
    });
}

bool ChatManager::DoAudioDeviceCollectionsMatch( 
    _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices1, 
    _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices2 
    )
{
    if( audioDevices1 == nullptr || audioDevices2 == nullptr )
    {
        // Shouldn't happen but if either is null then treat as not matching
        return false;
    }

    if( audioDevices1->Size != audioDevices2->Size )
    {
        // Different number of audio devices means the audio device lists don't match
        return false;
    }

    for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDeviceInfo1 in audioDevices1)
    {
        // See audioDeviceInfo->Id is in audioDevices2
        bool found = false;
        for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDeviceInfo2 in audioDevices2)
        {
            if( StringUtils::IsStringEqualCaseInsenstive(audioDeviceInfo1->Id, audioDeviceInfo2->Id) )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            // Did not find audioDeviceInfo1 in audioDevices2, so these audio device lists don't match
            return false;
        }
    }

    // Everything matched
    return true;
}

bool ChatManager::DoesAudioDeviceCollectionsMatchExclusiveDevices( 
    _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices1, 
    _In_ Windows::Foundation::Collections::IVectorView< Windows::Xbox::System::IAudioDeviceInfo^ >^ audioDevices2 
    )
{
    if( audioDevices1 == nullptr || audioDevices2 == nullptr )
    {
        // Shouldn't happen but if either is null then treat as not matching
        return false;
    }

    for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDeviceInfo1 in audioDevices1)
    {
        bool isSharedDevice = (audioDeviceInfo1->Sharing == Windows::Xbox::System::AudioDeviceSharing::Shared);
        if( !isSharedDevice ) 
        {
            // See audioDeviceInfo1->Id is in audioDevices2
            bool found = false;
            for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDeviceInfo2 in audioDevices2)
            {
                if( StringUtils::IsStringEqualCaseInsenstive(audioDeviceInfo1->Id, audioDeviceInfo2->Id) )
                {
                    found = true;
                    break;
                }
            }

            if( found )
            {
                // Found audioDeviceInfo1 in audioDevices2, so return true
                return true;
            }
        }

    }

    // No match found 
    return false;
}

void ChatManager::OnRemoteUserReadyToRemoveHandler( 
    _In_ uint8 channelIndex,
    _In_ Platform::String^ remoteUserXuid
    )
{
    if (remoteUserXuid->IsEmpty() == false)
    {
        ChatUser^ userToRemove = nullptr;
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
        for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
        {
            if (chatUser != nullptr && chatUser->XboxUserId == remoteUserXuid)
            {
                Windows::Foundation::Collections::IVectorView<uint8>^ allChannels = chatUser->GetAllChannels();
                for each (uint8 currentChannelIndex in allChannels)
                {
                    if (currentChannelIndex == channelIndex)
                    {
                        userToRemove = chatUser;
                        break;
                    }
                }
            }

            if (userToRemove != nullptr)
            {
                break;
            }
        }

        if (userToRemove != nullptr)
        {
            m_chatClient->RemoveUserFromChatChannel(
                channelIndex, 
                userToRemove->User,
                true
                );
        }
    }
}

bool ChatManager::OnCompareUniqueConsoleIdentifiersHandler( 
    _In_ Platform::Object^ object1, 
    _In_ Platform::Object^ object2 
    )
{
    if (object1 == nullptr || object2 == nullptr)
    {
        return false;
    }

    return OnCompareUniqueConsoleIdentifiers(object1, object2);
}

Windows::Storage::Streams::IBuffer^ 
    ChatManager::OnPostDecodeAudioBufferHandler( 
    _In_ Windows::Storage::Streams::IBuffer^ buffer, 
    _In_ Windows::Xbox::Chat::IFormat^ audioFormat,
    _In_ Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers
    )
{
    return OnPostDecodeAudioBuffer( buffer, audioFormat, chatUsers );
}

Windows::Storage::Streams::IBuffer^ 
    ChatManager::OnPreEncodeAudioBufferHandler( 
    _In_ Windows::Storage::Streams::IBuffer^ buffer, 
    _In_ Windows::Xbox::Chat::IFormat^ audioFormat,
    _In_ Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers
    )
{
    return OnPreEncodeAudioBuffer( buffer, audioFormat, chatUsers );
}

Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ ChatManager::GetChatUsers()
{
    return m_chatClient->GetChatUsers();
}

void ChatManager::MuteUserFromAllChannelsPermanently( ChatUser^ user )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( user );

    user->MutePermanently();
    MuteUserFromAllChannels(user);
}

void ChatManager::MuteUserFromAllChannels( ChatUser^ chatUser )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( chatUser );

    chatUser->Mute();
    if (!chatUser->IsLocal)
    {
        // If muting remote user, also set the volume to 0.
        // GameChat will internally set ChatRestriction to Muted when volume equals 0.
        float volume = 0.0f;
        m_chatClient->ChangeMuteStateForChatParticipantFromAllChannels(
            chatUser->User->Id,
            volume);
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatMuteUserFromAllChannels( 
        chatUser == nullptr ? L"" : chatUser->XboxUserId->Data()
        );
#endif
}

void ChatManager::UnmuteUserFromAllChannels( ChatUser^ chatUser )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL( chatUser );

    if (!chatUser->IsMutedPermanently)
    {
        chatUser->Unmute();
        if (!chatUser->IsLocal)
        {
            float volume = 1.0f;
            m_chatClient->ChangeMuteStateForChatParticipantFromAllChannels(
                chatUser->User->Id,
                volume);
        }
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatUnmuteUserFromAllChannels(
        chatUser == nullptr ? L"" : chatUser->XboxUserId->Data()
        );
#endif
}

void ChatManager::MuteAllUsersFromAllChannels()
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        if (chatUser != nullptr)
        {
            MuteUserFromAllChannels(chatUser);
        }
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatMuteAllUsersFromAllChannels();
#endif
}

void ChatManager::UnmuteAllUsersFromAllChannels()
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        if (chatUser != nullptr)
        {
            UnmuteUserFromAllChannels(chatUser);
        }
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatUnmuteAllUsersFromAllChannels();
#endif
}

uint32 ChatManager::ConvertChatSessionPeriodToMilliseconds( 
    _In_ ChatSessionPeriod chatSessionPeriod 
    )
{
    switch (chatSessionPeriod)
    {
    case ChatSessionPeriod::ChatPeriodOf20Milliseconds: return 20;
    case ChatSessionPeriod::ChatPeriodOf40Milliseconds: return 40; 
    case ChatSessionPeriod::ChatPeriodOf80Milliseconds: return 80;
    }

    throw ref new Platform::InvalidArgumentException();
}

void ChatManager::SendLocalUsersToRemoteConsole( 
    _In_ Platform::Object^ remoteUniqueConsoleIdentifier 
    )
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
    for each (Microsoft::Xbox::GameChat::ChatUser^ chatUser in chatUsers)
    {
        if (chatUser->IsLocal)
        {
            Windows::Foundation::Collections::IVectorView<uint8>^ allChannels = chatUser->GetAllChannels();
            for each (uint8 channelIndex in allChannels)
            {
                m_chatNetwork->CreateChatUserPacket((uint8)channelIndex, chatUser, remoteUniqueConsoleIdentifier);
            }
        }
    }
}

Windows::Foundation::IAsyncAction^ 
    ChatManager::AddLocalUsersToChatChannelAsync( 
    _In_ uint8 channelIndex, 
    _In_ Windows::Foundation::Collections::IVectorView<Windows::Xbox::System::User^>^ users 
    )
{
    return create_async( [this, channelIndex, users]()
    {
        for each (Windows::Xbox::System::User^ user in users)
        {
            auto asyncOp = AddLocalUserToChatChannelAsync(channelIndex, user);
            create_task(asyncOp).wait();
        }
    });
}

void ChatManager::OnChatManagerSettingsChangedHandler()
{
    m_chatAudioThread->OnChatManagerSettingsChangedHandler();
}

bool ChatManager::HasMicFocus::get()
{
    return m_chatAudioThread->HasMicFocus;
}

ChatPerformanceCounters^ ChatManager::ChatPerformanceCounters::get()
{
    return m_chatAudioThread->ChatPerformanceCounters;
}

}}}

#endif