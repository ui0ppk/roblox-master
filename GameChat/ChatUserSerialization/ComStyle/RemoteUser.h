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
=====================================================================
    This class is used to represent a remote user, remote user is
    not created by user manager

    IMPORTANT
    1. Implements IUser
=====================================================================
*/
class ComStyleRemoteUser : public Mwrl::RuntimeClass<ABI::Windows::Xbox::System::IUser, Mwrl::FtmBase> 
{
public:
                        ComStyleRemoteUser();
    virtual             ~ComStyleRemoteUser();

    HRESULT             RuntimeClassInitialize( const boolean isGuest,
                                                const HSTRING xuid,
                                                Awfc::IVectorView<Awxs::IAudioDeviceInfo*>* const pIAudioDevices );

    HRESULT             get_Id( UINT32* id );
    HRESULT             get_AudioDevices( Awfc::IVectorView<Awxs::IAudioDeviceInfo*>** audioDevices );
    HRESULT             get_Controllers( Awfc::IVectorView<Awxi::IController*>** controllers );
    HRESULT             get_DisplayInfo( Awxs::IUserDisplayInfo** displayInfo );
    HRESULT             get_IsGuest( boolean* isGuest );
    HRESULT             get_IsSignedIn( boolean* isSignedIn );
    HRESULT             get_Location( Awxs::UserLocation* location );
    HRESULT             get_Sponsor( ABI::Windows::Xbox::System::IUser** sponsor );
    HRESULT             get_XboxUserHash( HSTRING* userhash );
    HRESULT             get_XboxUserId( _Out_ HSTRING* xuid );

    HRESULT             GetTokenAndSignatureAsync( HSTRING httpMethod,
                                                    HSTRING url,
                                                    HSTRING headers,
                                                    Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** operation );
    HRESULT             GetTokenAndSignatureWithBodyAsync( HSTRING httpMethod,
                                                            HSTRING url,
                                                            HSTRING headers,
                                                            UINT32 __bodySize,
                                                            BYTE* body,
                                                            Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** operation );
    HRESULT             GetTokenAndSignatureWithStringBodyAsync( HSTRING httpMethod,
                                                                 HSTRING url,
                                                                 HSTRING headers,
                                                                 HSTRING body,
                                                                 Awf::IAsyncOperation<Awxs::GetTokenAndSignatureResult*>** operation );

private:
    UINT                m_id;
    boolean             m_isGuest;
    Mwrlw::HString      m_xuid;
    Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>   m_spIAudioDevices;
    static UINT         s_id;
};

}}}

#endif