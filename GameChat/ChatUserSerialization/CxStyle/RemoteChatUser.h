//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#if TV_API

namespace Wfc = Windows::Foundation::Collections;
namespace Wxs = Windows::Xbox::System;
namespace Wxi = Windows::Xbox::Input;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ref class RemoteChatUser sealed : public Wxs::IUser
{
public:
    RemoteChatUser(
        _In_ Platform::String^ xuid,
        _In_ bool isGuest,
        _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices
        );

    // IUser interface implementation, these are not implemented
    virtual Windows::Foundation::IAsyncOperation< Wxs::GetTokenAndSignatureResult^ >^ GetTokenAndSignatureAsync( 
        _In_ Platform::String^ xuid, 
        _In_ Platform::String^ url, 
        _In_ Platform::String^ headers 
        )
    {
        throw E_NOTIMPL;
    }

    virtual Windows::Foundation::IAsyncOperation< Wxs::GetTokenAndSignatureResult^ >^ GetTokenAndSignatureAsync( 
        _In_ Platform::String^ xuid, 
        _In_ Platform::String^ url, 
        _In_ Platform::String^ headers, 
        _In_ const Platform::Array<unsigned char>^ body 
        )
    {
        throw E_NOTIMPL;
    }

    virtual Windows::Foundation::IAsyncOperation< Wxs::GetTokenAndSignatureResult^ >^ GetTokenAndSignatureAsync( 
        _In_ Platform::String^ m_xuid, 
        _In_ Platform::String^ url, 
        _In_ Platform::String^ headers, 
        _In_ Platform::String^ body 
        )
    {
        throw E_NOTIMPL;
    }

    // These properties are not used by the Chat libraries and are left unimplemented
    virtual property Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ AudioDevices
    {
        Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ get()
        {
            return m_audioDevices.GetView();
        }
    }

    virtual property Wfc::IVectorView< Wxi::IController^ >^ Controllers
    {
        Wfc::IVectorView< Wxi::IController^ >^ get()
        {
            return nullptr;
        }
    }

    virtual property Wxs::UserDisplayInfo^ DisplayInfo
    {
        Wxs::UserDisplayInfo^ get()
        {
            throw E_NOTIMPL;
        }
    }

    virtual property Wfc::IVectorView< UINT>^ Privileges
    {
        Wfc::IVectorView< UINT >^ get()
        {
            throw E_NOTIMPL;
        }
    }

    virtual property Platform::String^ XboxUserHash
    {
        Platform::String^ get()
        {
            throw E_NOTIMPL;
        }
    }

    virtual property bool IsAdult
    {
        bool get()
        {
            throw E_NOTIMPL;
        }
    }

    virtual property Wxs::User^ Sponsor
    {
        Wxs::User^ get()
        {
            return nullptr;
        }
    }

    virtual property UINT Id
    {
        UINT get()
        {
            return m_id;
        }
    }

    // This isn't currently used, but may be used in the future
    virtual property bool IsGuest
    {
        bool get()
        {
            return m_isGuest;
        }
    }

    // Remote users are never signed in on our console
    virtual property bool IsSignedIn
    {
        bool get()
        {
            return false;
        }
    }

    // By definition, these users are always remote
    virtual property Wxs::UserLocation Location
    {
        Wxs::UserLocation get()
        {
            return Wxs::UserLocation::Remote;
        }
    }

    // XUID's are always important
    virtual property Platform::String^ XboxUserId
    {
        Platform::String^ get()
        {
            return m_xuid;
        }
    }

    virtual property Platform::String^ Gamertag
    {
        Platform::String^ get()
        {
            return m_gamertag;
        }
    }

    void AddAudioDevice( 
        _In_ Wxs::IAudioDeviceInfo^ audioDevice 
        );

    void ClearAudioDevices();

    void ResetAudioDevices(
        _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices
        );

private:
    RemoteChatUser();

    void Initialize(
        _In_ Platform::String^ xuid,
        _In_ Wfc::IVectorView< Wxs::IAudioDeviceInfo^ >^ audioDevices
        );

private:
    Platform::Collections::Vector< Wxs::IAudioDeviceInfo^ > m_audioDevices;
    Platform::String^ m_xuid;
    UINT m_id;
    bool m_isGuest;
    Platform::String^ m_gamertag;
};

}}}

#endif