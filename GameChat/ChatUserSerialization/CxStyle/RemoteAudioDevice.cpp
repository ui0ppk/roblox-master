//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "RemoteAudioDevice.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

    
RemoteAudioDeviceInfo::RemoteAudioDeviceInfo( 
    _In_ Windows::Xbox::System::AudioDeviceCategory deviceCategory, 
    _In_ Windows::Xbox::System::AudioDeviceType deviceType, 
    _In_ Platform::String^ id, 
    _In_ bool isMicrophoneMuted, 
    _In_ Windows::Xbox::System::AudioDeviceSharing sharing 
    ) :
    m_deviceCategory( deviceCategory ),
    m_deviceType( deviceType ),
    m_id( id ),
    m_isMicrophoneMuted( isMicrophoneMuted ),
    m_sharing( sharing )
{
}

}}}

#endif