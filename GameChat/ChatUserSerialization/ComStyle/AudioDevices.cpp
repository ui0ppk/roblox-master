//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "AudioDevices.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ComStyleRemoteAudioDeviceInfos::ComStyleRemoteAudioDeviceInfos() 
{
}

ComStyleRemoteAudioDeviceInfos::~ComStyleRemoteAudioDeviceInfos() 
{
}

void ComStyleRemoteAudioDeviceInfos::PushBack( 
    const Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spIAudioDevice 
    ) 
{
    m_vector.push_back( spIAudioDevice );
}

HRESULT ComStyleRemoteAudioDeviceInfos::GetAt( UINT index, Awxs::IAudioDeviceInfo** ppOut ) 
{
    if ( ppOut == nullptr ) 
    {
        return E_POINTER;
    }

    return m_vector.at( index ).CopyTo( ppOut );
}

HRESULT ComStyleRemoteAudioDeviceInfos::get_Size( UINT* pOut ) 
{
    if ( pOut == nullptr ) 
    {
        return E_POINTER;
    }
    *pOut = ( UINT )m_vector.size();

    return S_OK;
}

HRESULT ComStyleRemoteAudioDeviceInfos::IndexOf( Awxs::IAudioDeviceInfo* /* pIAudioDevice */, UINT* /* pOutIndex */, boolean* /* pOutFound */ ) 
{
    return E_UNEXPECTED;
}
    
}}}

#endif