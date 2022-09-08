//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

/// <summary>
/// Indicates the talking mode of the user
/// </summary>
public enum class ChatUserTalkingMode 
{
    /// <summary>
    /// Indicates the chat user is not talking
    /// </summary>
    NotTalking = 0x0,

    /// <summary>
    /// Indicates the chat user is talking over a headset
    /// </summary>
    TalkingOverHeadset = 0x1,

    /// <summary>
    /// Indicates the chat user is talking over the Kinect
    /// </summary>
    TalkingOverKinect = 0x2
};

public ref class ChatUser sealed
{
public:
    /// <summary>
    /// The XboxUserID of this ChatUser
    /// </summary>
    property Platform::String^ XboxUserId
    {
        Platform::String^ get();
    }

    /// <summary>
    /// The UniqueConsoleIdentifier for this ChatUser
    /// </summary>
    property Platform::Object^ UniqueConsoleIdentifier
    {
        Platform::Object^ get();
    }

    /// <summary>
    /// Indicates if the this ChatUser is talking or not
    /// </summary>
    property ChatUserTalkingMode TalkingMode
    {
        ChatUserTalkingMode get();
    }

    /// <summary>
    /// Indicates number of pending audio packets to play.  
    /// This is controlled by the jitter buffer.  
    /// The jitter buffer can be controlled by some settings found on ChatManagerSettings
    /// </summary>
    property uint32 NumberOfPendingAudioPacketsToPlay
    {
        uint32 get();
    }

    /// <summary>
    /// Indicates the current value of the dynamic needed packet count.
    /// This is the number of packets required before audio will start playing.
    /// This number is dynamically adjusted based on heuristics monitoring how often packets arrive from the network
    /// </summary>
    property uint32 DynamicNeededPacketCount
    {
        uint32 get();
    }

    /// <summary>
    /// Indicates if the chat user is restricted
    /// </summary>
    property Windows::Xbox::Chat::ChatRestriction RestrictionMode
    {
        Windows::Xbox::Chat::ChatRestriction get();
    }

    /// <summary>
    /// Returns the IUser for this ChatUser
    /// </summary>
    property Windows::Xbox::System::IUser^ User
    {
        Windows::Xbox::System::IUser^ get();
    }

    /// <summary>
    /// Returns if the user is local for the console
    /// </summary>
    property bool IsLocal
    {
        bool get();
    }

    /// <summary>
    /// Returns if the local user is muted.
    /// If local user is muted, then doesn't send capture packets to others.
    /// </summary>
    property bool IsLocalUserMuted
    {
        bool get();
    }

    /// <summary>
    /// Returns if the user is muted.
    /// </summary>
    property bool IsMuted
    {
        bool get();
    }

    /// <summary>
    /// Returns if the user is muted permanently.
    /// </summary>
    property bool IsMutedPermanently
    {
        bool get();
    }

    /// <summary>
    /// Returns a list of all channels the user is part of.
    /// </summary>
    Windows::Foundation::Collections::IVectorView<uint8>^ GetAllChannels();
    
    property Windows::Xbox::Chat::ChatParticipantTypes ParticipantType 
    {
        /// <summary>
        /// Gets the ParticipantType for this ChatUser
        /// </summary>
        Windows::Xbox::Chat::ChatParticipantTypes get();

        /// <summary>
        /// Sets the ParticipantType for this ChatUser
        /// </summary>
        void set(Windows::Xbox::Chat::ChatParticipantTypes val);
    }

    property float Volume
    {
        /// <summary>
        /// Gets the volume for this ChatUser
        /// Volume is a percentage 0.0f to 1.0f
        /// </summary>
        float get();

        /// <summary>
        /// Sets the volume for this ChatUser
        /// Volume is a percentage 0.0f to 1.0f
        /// </summary>
        void set(float val);
    }

    property float LocalRenderTargetVolume
    {
        /// <summary>
        /// Gets the volume for the render target if this is a local user
        /// Volume is a percentage 0.0f to 1.0f.
        /// Returns 0 if this is a remote user
        /// </summary>
        float get();

        /// <summary>
        /// Sets the volume for the chat render target if this is a local user. 
        /// Volume is a percentage 0.0f to 1.0f
        /// Value is ignored if this is a remote user
        /// </summary>
        void set(float val);
    }

internal:

    /// <summary>
    /// Returns the IChatParticipant if this ChatUser
    /// </summary>
    property Windows::Xbox::Chat::IChatParticipant^ ChatParticipant
    {
        Windows::Xbox::Chat::IChatParticipant^ get();
    }

    /// <summary>
    /// Mute user permanently 
    /// </summary>
    void MutePermanently();

    /// <summary>
    /// UnMute user permanently 
    /// </summary>
    void UnmutePermanently();

    /// <summary>
    /// Internal helper function.  If muting local user, then don't send capture packets to others.
    /// Else, set ChatParticipant volume to 0.
    /// </summary>
    void Mute();

    /// <summary>
    /// Internal helper function to unmute user
    /// </summary>
    void Unmute();

    /// <summary>
    /// Internal function to construct a ChatUser
    /// </summary>
    ChatUser(
        _In_ Platform::String^ xuid,
        _In_ bool isLocal,
        _In_opt_ Platform::Object^ uniqueConsoleIdentifier,
        _In_ Windows::Xbox::Chat::IChatParticipant^ chatParticipant
        );

    /// <summary>
    /// Internal function to change the talking mode of the ChatUser
    /// </summary>
    void SetTalkingMode(
        _In_ ChatUserTalkingMode mode
        );

    /// <summary>
    /// Internal function to set number of pending audio packets for this ChatUser
    /// </summary>
    void SetNumberOfPendingAudioPacketsToPlay(
        _In_ uint32 numberOfPendingAudioPacketsToPlay
        );

    /// <summary>
    /// Internal function to set the restriction mode for this chat user
    /// </summary>
    void SetRestrictionMode(
        _In_ Windows::Xbox::Chat::ChatRestriction restriction
        );

    /// <summary>
    /// Internal function to add this chat user to a specific channel
    /// </summary>
    void AddChannelIndexToChannelList( 
        _In_ uint8 channelIndex
        );

    /// <summary>
    /// Internal function to remove this chat user from a specific channel
    /// </summary>
    void RemoveChannelIndexFromChannelList( 
        _In_ uint8 channelIndex
        );

    /// <summary>
    /// Set the dynamic needed packet count for this chat user
    /// </summary>
    void SetDynamicNeededPacketCount( 
        _In_ uint32 dynamicNeededPacketCount
        );

    /// <summary>
    /// Set the Windows::Xbox::Chat::IChatRenderTarget for this chat user
    /// </summary>
    void SetChatRenderTarget( 
        _In_ Windows::Xbox::Chat::IChatRenderTarget^ chatRenderTarget
        );

private:
    Concurrency::critical_section m_stateLock;
    Platform::String^ m_xuid;
    Platform::Object^ m_uniqueConsoleIdentifier;
    Windows::Xbox::Chat::IChatParticipant^ m_chatParticipant;
    Platform::String^ m_gamertag;
    ChatUserTalkingMode m_talkingMode;
    uint32 m_numberOfPendingAudioPacketsToPlay;
    Windows::Xbox::Chat::ChatRestriction m_restrictionMode;
    bool m_isLocal;
    bool m_isMuted;
    bool m_shouldBepermanentlyMuted;
    uint32 m_dynamicNeededPacketCount;
    Windows::Xbox::Chat::IChatRenderTarget^ m_chatRenderTarget;

    /// <summary>
    /// List of channels the user is currently part of.
    /// </summary>
    std::vector<uint8> m_channels;

    /// <summary>
    /// ChatParticipant volume prior to mute.
    /// </summary>
    float m_chatParticipantVolume;
};

}}}

#endif