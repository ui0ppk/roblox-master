//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "AudioDeviceInfo.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ComStyleRemoteAudioDeviceInfo::ComStyleRemoteAudioDeviceInfo() : 
    m_type( Awxs::AudioDeviceType_Capture ), 
    m_sharing( Awxs::AudioDeviceSharing_Exclusive )
{
}

ComStyleRemoteAudioDeviceInfo::~ComStyleRemoteAudioDeviceInfo() 
{
}

HRESULT ComStyleRemoteAudioDeviceInfo::RuntimeClassInitialize( 
    const HSTRING id,
    const Awxs::AudioDeviceType type,
    const Awxs::AudioDeviceSharing sharing,
    const Awxs::AudioDeviceCategory category 
    ) 
{
    HRESULT hr = E_UNEXPECTED;

    CHECKHR_EXIT( m_id.Set( id ) );
    m_type = type;
    m_sharing = sharing;
    m_category = category;
exit:
    return hr;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_Id( _Out_ HSTRING* pOut ) 
{
    if ( pOut == nullptr ) 
    {
        return E_POINTER;
    }

    return WindowsDuplicateString( m_id.Get(), pOut );
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_DeviceType( _Out_ Awxs::AudioDeviceType* pOut ) 
{
    if ( pOut == nullptr ) 
    {
        return E_POINTER;
    }
    *pOut = m_type;

    return S_OK;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_Sharing( _Out_ Awxs::AudioDeviceSharing* pOut ) 
{
    if ( pOut == nullptr ) 
    {
        return E_POINTER;
    }
    *pOut = m_sharing;

    return S_OK;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_DeviceCategory( _Out_ Awxs::AudioDeviceCategory* pOut ) 
{
    if ( pOut == nullptr ) 
    {
        return E_POINTER;
    }
    *pOut = m_category;

    return S_OK;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_Volume( _Out_ float* /* pOut */ ) 
{
    return E_UNEXPECTED;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_Muted( _Out_ boolean* /* pOut */ ) 
{
    return E_UNEXPECTED;
}

HRESULT ComStyleRemoteAudioDeviceInfo::get_IsMicrophoneMuted( _Out_ boolean* /* pOut */ ) 
{
    return E_NOTIMPL;
}

}}}

#endif