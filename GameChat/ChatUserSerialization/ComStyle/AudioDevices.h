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
====================================================================
    This class is used to represent a vector of Mwrl::ComPtr<Awxs::IAudioDeviceInfo>

    IMPORTANT
    Implements IAudioDevices
====================================================================
*/
class ComStyleRemoteAudioDeviceInfos : public Mwrl::RuntimeClass<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>,Mwrl::FtmBase> 
{
public:
                                    ComStyleRemoteAudioDeviceInfos();
    virtual                         ~ComStyleRemoteAudioDeviceInfos();

    void                            PushBack( const Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spIAudioDevice );

    // IAudioDevices
    HRESULT                         GetAt( UINT index, Awxs::IAudioDeviceInfo** ppOut );
    HRESULT                         get_Size( UINT* pOut );
    HRESULT                         IndexOf( Awxs::IAudioDeviceInfo* /* pIAudioDevice */, UINT* /* pOutIndex */, boolean* /* pOutFound */ );

private:
    std::vector<Mwrl::ComPtr<Awxs::IAudioDeviceInfo>>   m_vector;
};

}}}

#endif