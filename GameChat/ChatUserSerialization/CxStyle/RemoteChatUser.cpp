//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "RemoteChatUser.h"

#if TV_API

using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

const UINT REMOTE_USER_ID_SEED = 0x01001000;

RemoteChatUser::RemoteChatUser(
    _In_ Platform::String^ xuid,
    _In_ bool isGuest,
    _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices
    ) :
    m_isGuest(isGuest)
{
    Initialize( xuid, audioDevices );
}

RemoteChatUser::RemoteChatUser() :
    m_xuid( nullptr ),
    m_isGuest(false)
{
}

void RemoteChatUser::Initialize(
    _In_ Platform::String^ xuid,
    _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices)
{
    m_xuid = xuid;

    for( unsigned int i = 0; i < audioDevices->Size; ++i )
    {
        AddAudioDevice( audioDevices->GetAt( i ) );
    }

    static UINT remoteIdTracker = REMOTE_USER_ID_SEED;
    m_id = InterlockedIncrement(&remoteIdTracker);
}

void RemoteChatUser::AddAudioDevice( _In_ Wxs::IAudioDeviceInfo^ audioDevice )
{
    m_audioDevices.Append( audioDevice );
}

void RemoteChatUser::ClearAudioDevices()
{
    m_audioDevices.Clear();
}

void RemoteChatUser::ResetAudioDevices(
    _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices
    )
{
    m_audioDevices.Clear();

    for (auto device : audioDevices)
    {
        m_audioDevices.Append( device );
    }
}

}}}

#endif