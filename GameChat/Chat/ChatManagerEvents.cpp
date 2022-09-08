//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatManagerEvents.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {


DebugMessageEventArgs::DebugMessageEventArgs( 
    _In_ Platform::String^ message,
    _In_ int hr ) :
    m_message( message ),
    m_hresult( hr )
{
}

int DebugMessageEventArgs::ErrorCode::get()
{ 
    return m_hresult; 
}

Platform::String^ DebugMessageEventArgs::Message::get()
{ 
    return m_message;
}

ChatPacketEventArgs::ChatPacketEventArgs( 
    _In_ Windows::Storage::Streams::IBuffer^ packetBuffer, 
    _In_ Platform::Object^ uniqueTargetConsoleIdentifier, 
    _In_ bool sendPacketToAllConnectedConsoles,
    _In_ bool sendReliable,
    _In_ bool sendInOrder,
    _In_ Microsoft::Xbox::GameChat::ChatMessageType chatMessageType,
    _In_ Microsoft::Xbox::GameChat::ChatUser^ chatUser
    ) :
    m_packetBuffer( packetBuffer ),
    m_uniqueTargetConsoleIdentifier( uniqueTargetConsoleIdentifier),
    m_sendPacketToAllConnectedConsoles( sendPacketToAllConnectedConsoles ),
    m_sendReliable( sendReliable ),
    m_sendInOrder( sendInOrder ),
    m_chatMessageType( chatMessageType ),
    m_chatUser(chatUser)
{
}

Windows::Storage::Streams::IBuffer^ ChatPacketEventArgs::PacketBuffer::get() 
{ 
    return m_packetBuffer; 
}

Platform::Object^ ChatPacketEventArgs::UniqueTargetConsoleIdentifier::get() 
{ 
    return m_uniqueTargetConsoleIdentifier;
}

Microsoft::Xbox::GameChat::ChatUser^ ChatPacketEventArgs::ChatUser::get() 
{ 
    return m_chatUser;
}

bool ChatPacketEventArgs::SendPacketToAllConnectedConsoles::get() 
{ 
    return m_sendPacketToAllConnectedConsoles; 
}

bool ChatPacketEventArgs::SendReliable::get()
{
    return m_sendReliable;
}

bool ChatPacketEventArgs::SendInOrder::get()
{
    return m_sendInOrder;
}


Microsoft::Xbox::GameChat::ChatMessageType 
ChatPacketEventArgs::ChatMessageType::get()
{
    return m_chatMessageType;
}

}}}

#endif