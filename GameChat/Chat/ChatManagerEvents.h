//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatUser.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

/// <summary>
/// Various types of chat messages sent by the GameChat system
/// </summary>
public enum class ChatMessageType
{
    /// <summary>
    /// Sends voice data along with a unique LOOKUP_ID which is built from the local name of remote console and an audio device index
    /// </summary>
    ChatVoiceDataMessage = 1,

    /// <summary>
    /// Sends byte array which is used to create an IUser on the remote console 
    /// </summary>
    UserAddedMessage = 2,

    /// <summary>
    /// Sends XboxUserId string of the user to remove
    /// </summary>
    UserRemovedMessage = 3,

    /// <summary>
    /// ProcessIncomingChatMessage was sent an invalid chat packet
    /// </summary>
    InvalidMessage = 4
};

/// <summary>
/// Event to report diagnostic and error message information
/// </summary>
public ref class DebugMessageEventArgs sealed
{
public:
    /// <summary>
    /// The debug message
    /// </summary>
    property Platform::String^ Message  { Platform::String^ get(); }

    /// <summary>
    /// The HRESULT of the message. S_OK for non-errors
    /// </summary>
    property int ErrorCode { int get(); }

internal:
    DebugMessageEventArgs(
        _In_ Platform::String^ message, 
        _In_ int hr );

private:
    Platform::String^ m_message;
    int m_hresult;
};

/// <summary>
/// Arguments for the event when a chat packet is ready to be sent
/// </summary>
public ref class ChatPacketEventArgs sealed
{
public:
    /// <summary>
    /// The buffer of the packet that should be sent to remote console(s)
    /// </summary>
    property Windows::Storage::Streams::IBuffer^ PacketBuffer { Windows::Storage::Streams::IBuffer^ get(); }

    /// <summary>
    /// Indicates if the packet should be sent with reliable UDP
    /// </summary>
    property bool SendReliable { bool get(); }

    /// <summary>
    /// Indicates if the packet should be sent sequential ordering if available
    /// </summary>
    property bool SendInOrder { bool get(); }

    /// <summary>
    /// Indicates if the packet should be sent to all connected consoles or just a single remote console.
    /// If this is false, then TargetUniqueConsoleIdentifier indicates who to send the packet to.
    /// </summary>
    property bool SendPacketToAllConnectedConsoles { bool get(); }

    /// <summary>
    /// The remote console to send the packet to.  This is nullptr if SendPacketToAllConnectedConsoles is true.
    /// </summary>
    property Platform::Object^ UniqueTargetConsoleIdentifier { Platform::Object^ get(); }

    /// <summary>
    /// Indicates the ChatMessageType message type of the packet
    /// Typically this should be ignored, but some games may need to specific handling around unique types
    /// </summary>
    property Microsoft::Xbox::GameChat::ChatMessageType ChatMessageType { Microsoft::Xbox::GameChat::ChatMessageType get(); }

    /// <summary>
    /// If this is a ChatMessageType::ChatVoiceDataMessage message, then this is the first ChatUser whose audio device that created the voice packet.
    /// For Kinect, there may be multiple ChatUsers who share the Kinect mic, so this is the first ChatUser.
    /// Otherwise, this is nullptr.
    /// Typically this can be ignored but some games may want to do specific handling around who created the voice packet.
    /// </summary>
    property Microsoft::Xbox::GameChat::ChatUser^ ChatUser { Microsoft::Xbox::GameChat::ChatUser^ get(); }

internal:
    ChatPacketEventArgs(
        _In_ Windows::Storage::Streams::IBuffer^ packetBuffer,
        _In_ Platform::Object^ uniqueTargetConsoleIdentifier,
        _In_ bool sendPacketToAllConnectedConsoles,
        _In_ bool sendReliable,
        _In_ bool sendInOrder,
        _In_ Microsoft::Xbox::GameChat::ChatMessageType chatMessageType,
        _In_ Microsoft::Xbox::GameChat::ChatUser^ chatUser
        );

private:
    Windows::Storage::Streams::IBuffer^ m_packetBuffer;
    Platform::Object^ m_uniqueTargetConsoleIdentifier;
    bool m_sendPacketToAllConnectedConsoles;
    bool m_sendReliable;
    bool m_sendInOrder;
    Microsoft::Xbox::GameChat::ChatMessageType m_chatMessageType;
    Microsoft::Xbox::GameChat::ChatUser^ m_chatUser;
};

/// <summary>
/// This delegate compares 2 uniqueRemoteConsoleIdentifiers.  They are Platform::Object^ and can be cast or unboxed to most types. 
/// What exactly you use doesn't matter, but optimally it would be something that uniquely identifies a console on in the session. 
/// A Windows::Xbox::Networking::SecureDeviceAssociation^ is perfect to use if you have access to it.
/// This delegate is not optional and must return true if the uniqueRemoteConsoleIdentifier1 matches uuniqueRemoteConsoleIdentifier2
/// </summary>
public delegate bool CompareUniqueConsoleIdentifiersHandler(
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier1, 
    _In_ Platform::Object^ uniqueRemoteConsoleIdentifier2 
    );

/// <summary>
/// This delegate enables titles to process the audio stream.  
/// To use, set either ChatManagerSettings::PreEncodeCallbackEnabled or ChatManagerSettings::PostDecodeCallbackEnabled to true 
/// </summary>
/// <param name="preEncodedRawBuffer">A buffer containing the pre-processed raw audio data</param>
/// <param name="audioFormat">The audio format of the preEncodedRawBuffer</param>
/// <param name="chatUsers">A collection of users associated with this audio buffer</param>
/// <returns>A buffer containing processed audio</returns>
public delegate Windows::Storage::Streams::IBuffer^ ProcessAudioBufferHandler(
    _In_ Windows::Storage::Streams::IBuffer^ preEncodedRawBuffer, 
    _In_ Windows::Xbox::Chat::IFormat^ audioFormat,
    _In_ Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers
    );

}}}

#endif