//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "RemoteUser.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

#define REMOTEUSERSTART 0x01000000
#define REMOTEUSEREND 0x01100000
unsigned ComStyleRemoteUser::s_id = REMOTEUSERSTART;

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
ComStyleRemoteUser::ComStyleRemoteUser() : 
    m_id( s_id < REMOTEUSEREND ? s_id++ : (s_id = REMOTEUSERSTART, s_id++) ), // using the , operator, returns last expression
    m_isGuest( false )
{
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
ComStyleRemoteUser::~ComStyleRemoteUser() 
{
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::RuntimeClassInitialize( 
    const boolean isGuest,
    const HSTRING xuid,
    Awfc::IVectorView<Awxs::IAudioDeviceInfo*>* const pIAudioDevices ) 
{
    HRESULT hr = E_UNEXPECTED;
            
    m_isGuest = isGuest;
    CHECKHR_EXIT( m_xuid.Set( xuid ) );
    m_spIAudioDevices = pIAudioDevices;
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_Id( UINT32* id ) 
{
    if ( id == nullptr ) 
    {
        return E_POINTER;
    }
    *id = m_id;

    return S_OK;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_AudioDevices( Awfc::IVectorView<Awxs::IAudioDeviceInfo*>** audioDevices ) 
{
    if ( audioDevices == nullptr ) 
    {
        return E_POINTER;
    }

    return m_spIAudioDevices.CopyTo( audioDevices );
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_Controllers( Awfc::IVectorView<Awxi::IController*>** /* controllers */ ) 
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_DisplayInfo( Awxs::IUserDisplayInfo** /* displayInfo */ ) 
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_IsGuest( boolean* isGuest ) 
{
    if ( isGuest == nullptr ) 
    {
        return E_POINTER;
    }
    *isGuest = m_isGuest;

    return S_OK;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_IsSignedIn( boolean* isSignedIn ) 
{
    if ( isSignedIn == nullptr ) 
    {
        return E_POINTER;
    }
    *isSignedIn = true;

    return S_OK;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_Location( Awxs::UserLocation* location ) 
{
    if ( location == nullptr ) 
    {
        return E_POINTER;
    }
    *location = Awxs::UserLocation_Remote;

    return S_OK;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_Sponsor( IUser** /* sponsor */ ) 
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_XboxUserHash( HSTRING* /* userhash */ ) 
{
    return E_NOTIMPL;
}



//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::get_XboxUserId( _Out_ HSTRING* xuid ) 
{
    if ( !xuid ) 
    {
        return E_POINTER;
    }

    HRESULT hr = WindowsDuplicateString( m_xuid.Get(), xuid );
#ifdef __PREFAST__
    if ( FAILED(hr) )
    {
        *xuid = nullptr;
    }
#endif
    return  hr;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::GetTokenAndSignatureAsync( 
    HSTRING /* httpMethod */,
    HSTRING /* url */,
    HSTRING /* headers */,
    Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** /* operation */ ) 
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::GetTokenAndSignatureWithBodyAsync( 
    HSTRING /* httpMethod */,
    HSTRING /* url */,
    HSTRING /* headers */,
    UINT32 /* __bodySize */,
    BYTE* /* body */,
    Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** /* operation */ ) 
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
HRESULT ComStyleRemoteUser::GetTokenAndSignatureWithStringBodyAsync( 
    HSTRING /* httpMethod */,
    HSTRING /* url */,
    HSTRING /* headers */,
    HSTRING /* body */,
    Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** /* operation */ ) 
{
    return E_NOTIMPL;
}

}}}

#endif