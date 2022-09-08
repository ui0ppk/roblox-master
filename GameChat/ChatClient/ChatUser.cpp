//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatUser.h"
#include "StringUtils.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ChatUser::ChatUser(
    _In_ Platform::String^ xuid,
    _In_ bool isLocal,
    _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
    _In_ Windows::Xbox::Chat::IChatParticipant^ chatParticipant
    ) :
    m_xuid(xuid),
    m_isLocal(isLocal),
    m_uniqueConsoleIdentifier(uniqueConsoleIdentifier),
    m_chatParticipant(chatParticipant),
    m_talkingMode(ChatUserTalkingMode::NotTalking),
    m_restrictionMode(Windows::Xbox::Chat::ChatRestriction::None),
    m_chatParticipantVolume( 0.0f ),
    m_dynamicNeededPacketCount( 0 ),
    m_numberOfPendingAudioPacketsToPlay( 0 ),
    m_isMuted( false ),
    m_shouldBepermanentlyMuted( false )
{
    CHAT_THROW_E_POINTER_IF_NULL( chatParticipant );
    CHAT_THROW_INVALIDARGUMENT_IF_STRING_EMPTY( xuid );
}

Platform::String^ ChatUser::XboxUserId::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_xuid;
};


Platform::Object^ ChatUser::UniqueConsoleIdentifier::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_uniqueConsoleIdentifier;
}

Windows::Xbox::Chat::IChatParticipant^ ChatUser::ChatParticipant::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_chatParticipant;
}

Windows::Xbox::System::IUser^ ChatUser::User::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_chatParticipant->User;
}

ChatUserTalkingMode ChatUser::TalkingMode::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    ChatUserTalkingMode result = (ChatUserTalkingMode) m_talkingMode;
    return result;
}

void ChatUser::SetTalkingMode(
    _In_ ChatUserTalkingMode mode
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    m_talkingMode = mode;
};


uint32 ChatUser::NumberOfPendingAudioPacketsToPlay::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_numberOfPendingAudioPacketsToPlay;
}

void ChatUser::SetNumberOfPendingAudioPacketsToPlay(
    _In_ uint32 numberOfPendingAudioPacketsToPlay
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    m_numberOfPendingAudioPacketsToPlay = numberOfPendingAudioPacketsToPlay;
}

Windows::Xbox::Chat::ChatRestriction ChatUser::RestrictionMode::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_restrictionMode;
}

void ChatUser::SetRestrictionMode(
    _In_ Windows::Xbox::Chat::ChatRestriction restriction
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    m_restrictionMode = restriction;
};

bool ChatUser::IsLocal::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_isLocal;
}

bool ChatUser::IsMutedPermanently::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_shouldBepermanentlyMuted;
}


bool ChatUser::IsLocalUserMuted::get()
{
    // If muting myself, then don't send capture packets from that user
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_isMuted && m_isLocal;
}

void ChatUser::Mute()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_isMuted = true;
}

void ChatUser::Unmute()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    
    if (!m_shouldBepermanentlyMuted)
    {
        m_isMuted = false;

        // The restriction will be re-instantiated for the remote user on the next update.
        m_restrictionMode = Windows::Xbox::Chat::ChatRestriction::None;
    }
}

void ChatUser::MutePermanently()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_shouldBepermanentlyMuted = true;
    m_isMuted = true;
}

void ChatUser::UnmutePermanently()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_shouldBepermanentlyMuted = false;

    // The restriction will be re-instantiated for the remote user on the next update.
    m_restrictionMode = Windows::Xbox::Chat::ChatRestriction::None;
}

bool ChatUser::IsMuted::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_isMuted;
}

Windows::Foundation::Collections::IVectorView<uint8>^ ChatUser::GetAllChannels()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    Platform::Collections::Vector<uint8>^ channels = ref new Platform::Collections::Vector<uint8>();
    for each (uint8 channelIndex in m_channels)
    {
        channels->Append(channelIndex);
    }
    
    return channels->GetView();
}

void ChatUser::AddChannelIndexToChannelList( 
    _In_ uint8 channelIndex
    )
{
    bool channelIndexFound = false;

    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    for each (uint8 existingChannelIndex in m_channels)
    {
        if( existingChannelIndex == channelIndex )
        {
            channelIndexFound = true;
            break;
        }
    }

    if( !channelIndexFound )
    {
        m_channels.push_back( channelIndex );
    }
}

void ChatUser::RemoveChannelIndexFromChannelList( 
    _In_ uint8 channelIndex
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    bool found = false;
    auto iter = m_channels.begin();
    for( ; iter != m_channels.end(); iter++ )
    {
        uint8 existingChannelIndex = *iter;
        if ( existingChannelIndex == channelIndex )
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        m_channels.erase(iter);
    }
}

uint32 ChatUser::DynamicNeededPacketCount::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    return m_dynamicNeededPacketCount;
}

void ChatUser::SetDynamicNeededPacketCount(
    _In_ uint32 dynamicNeededPacketCount
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);    
    m_dynamicNeededPacketCount = dynamicNeededPacketCount;
}

Windows::Xbox::Chat::ChatParticipantTypes ChatUser::ParticipantType::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_chatParticipant->ParticipantType;
}

void ChatUser::ParticipantType::set( Windows::Xbox::Chat::ChatParticipantTypes val )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_chatParticipant->ParticipantType = val;
}

float ChatUser::Volume::get()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    return m_chatParticipant->Volume;
}

void ChatUser::Volume::set( float val )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_chatParticipant->Volume = val;
}

void ChatUser::SetChatRenderTarget( 
    _In_ Windows::Xbox::Chat::IChatRenderTarget^ chatRenderTarget
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_chatRenderTarget = chatRenderTarget;
}

float ChatUser::LocalRenderTargetVolume::get()
{
    if( !IsLocal || m_chatRenderTarget == nullptr )
    {
        return 0.0f;
    }

    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    Windows::Xbox::Chat::IChatRenderTargetVolume^ targetVolume = (Windows::Xbox::Chat::IChatRenderTargetVolume^)m_chatRenderTarget;
    return targetVolume->Volume;
}

void ChatUser::LocalRenderTargetVolume::set( float val )
{
    if( !IsLocal || m_chatRenderTarget == nullptr )
    {
        return;
    }

    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    Windows::Xbox::Chat::IChatRenderTargetVolume^ targetVolume = (Windows::Xbox::Chat::IChatRenderTargetVolume^)m_chatRenderTarget;
    targetVolume->Volume = val;
}

}}}

#endif