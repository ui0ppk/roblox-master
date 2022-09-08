//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatUserSerializationCommon.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

/*
===============================================================================
    This class is used to represent a remote audio device

    IMPORTANT
    1. Implements IAudioDevice
    2. We create remote audio device for remote user, based on network data
===============================================================================
*/

class ComStyleRemoteAudioDeviceInfo : public Mwrl::RuntimeClass<Awxs::IAudioDeviceInfo, Mwrl::FtmBase> 
{
public:
                                ComStyleRemoteAudioDeviceInfo();
    virtual                     ~ComStyleRemoteAudioDeviceInfo();

    HRESULT                     RuntimeClassInitialize( const HSTRING id,
                                                        const Awxs::AudioDeviceType type,
                                                        const Awxs::AudioDeviceSharing sharing,
                                                        const Awxs::AudioDeviceCategory category );

    // IAudioDevice
    HRESULT                     get_Id( _Out_ HSTRING* pOut );
    HRESULT                     get_DeviceType( _Out_ Awxs::AudioDeviceType* pOut );
    HRESULT                     get_Sharing( _Out_ Awxs::AudioDeviceSharing* pOut );
    HRESULT                     get_DeviceCategory( _Out_ Awxs::AudioDeviceCategory* pOut );
    HRESULT                     get_Volume( _Out_ float* /* pOut */ );
    HRESULT                     get_Muted( _Out_ boolean* /* pOut */ );
    HRESULT                     get_IsMicrophoneMuted( _Out_ boolean* /* pOut */ );

private:
    Mwrlw::HString              m_id;
    Awxs::AudioDeviceType       m_type;
    Awxs::AudioDeviceSharing    m_sharing;
    Awxs::AudioDeviceCategory   m_category;
};

}}}

#endif