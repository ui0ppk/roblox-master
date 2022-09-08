//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "AudioDeviceIDMapper.h"
#include "AudioDeviceInfo.h"
#include "AudioDevices.h"
#include "RemoteUser.h"
#include "ChatUnpacker.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Unpacker::Unpacker() 
{
}

Unpacker::~Unpacker() 
{
}

// Cleanup buffer and feed it with data
HRESULT Unpacker::UnpackBegin( 
    _In_reads_bytes_(size) const BYTE* const pData, 
    _In_ const UINT size ) 
{
    HRESULT hr = E_UNEXPECTED;

    CHECKHR_EXIT( m_buffer.Cleanup() );
    CHECKHR_EXIT( m_buffer.Write( pData, size ) );
exit:
    return hr;
}

HRESULT Unpacker::UnpackEnd() const 
{
    return S_OK;
}

//----------------------------------------------------------------------------------------
// Unpack functions
// 1. Unpack primitive types
//      a. No input data validation
//      b. Input data validation
// 2. Unpack complex types
//      a. Group of sub-unpack functions
//----------------------------------------------------------------------------------------
HRESULT Unpacker::UnpackBYTE( 
    _Out_ BYTE* pOut 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    BYTE* pR = nullptr;

    // No input data validation
    CHECKHR_EXIT( m_buffer.ReadFast( &pR, sizeof( BYTE ) ) );
    *pOut = *pR;
exit:
    return hr;
}

HRESULT Unpacker::UnpackUSHORT( 
    _Out_ USHORT* pOut 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    BYTE* pR = nullptr;

    // No input data validation
    CHECKHR_EXIT( m_buffer.ReadFast( &pR, sizeof( USHORT ) ) );
    *pOut = *( USHORT* )pR;
exit:
    return hr;
}

HRESULT Unpacker::UnpackUINT( 
    _Out_ UINT* pOut 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    BYTE* pR = nullptr;

    // No input data validation
    CHECKHR_EXIT( m_buffer.ReadFast( &pR, sizeof( UINT ) ) );
    *pOut = *( UINT* )pR;
exit:
    return hr;
}

// Returns pointer, no memory copy
HRESULT Unpacker::UnpackBytesFast( 
    _Outptr_result_maybenull_ BYTE** ppOut, 
    _In_ const UINT size 
    ) 
{
    if ( !ppOut )
    {
        return E_POINTER;
    }

    *ppOut = nullptr;

    HRESULT hr = S_FALSE;
    BYTE* pR = nullptr;

    // Input data validation
    if ( size ) 
    {
        CHECKHR_EXIT( m_buffer.ReadFast( &pR, size ) );
        *ppOut = pR;
        return hr;
    }
exit:
    return hr;
}

HRESULT Unpacker::UnpackString( 
    _Out_ Mwrlw::HString& out 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    UINT size = 0;
    BYTE* pBytes = nullptr;

    CHECKHR_EXIT( UnpackUINT( &size ) );
    CHECKHR_EXIT( UnpackBytesFast( &pBytes, size ) );

    {
        std::string byteString ((char*)pBytes, size);
        std::unique_ptr<WCHAR[]> pConverted( new WCHAR[size] );
        ::MultiByteToWideChar( CP_UTF8,
            0,
            &byteString[0],
            -1,
            pConverted.get(),
            size );

        CHECKHR_EXIT( WindowsCreateString( pConverted.get(), size , out.GetAddressOf() ) );
    }
exit:
    return hr;
}

// Returns pointer and size, no memory copy
HRESULT Unpacker::UnpackBufferFast( 
    _Outptr_result_maybenull_ BYTE** ppOut, 
    _Out_ UINT* pOut 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    UINT size = 0;
    BYTE* pBytes = nullptr;

    CHECKHR_EXIT( UnpackUINT( &size ) );
    CHECKHR_EXIT( UnpackBytesFast( &pBytes, size ) );

    *ppOut = pBytes;
    *pOut = size;
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we unpack an audio device, it is a "remote" audio device for a "remote" user
// 2. Id, type, sharing, use BYTE to represent enum
//----------------------------------------------------------------------------------------
HRESULT Unpacker::UnpackAudioDevice( 
    _Out_ Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spOut, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper,
    _In_ uint8 localNameOfRemoteConsole
    )
{
    HRESULT hr = E_UNEXPECTED;
    Mwrlw::HString id;
    BYTE type = 0;
    BYTE sharing = 0;
    BYTE category = 0;
    BYTE isKinect;

    DEVICE_ID deviceId;
    Platform::String^ strId; 
    CHECKHR_EXIT( UnpackBYTE( &deviceId ) );  

    CHECKHR_EXIT( UnpackBYTE( &isKinect ) );
    CHECKHR_EXIT( UnpackBYTE( &type ) );
    CHECKHR_EXIT( UnpackBYTE( &sharing ) );
    CHECKHR_EXIT( UnpackBYTE( &category ) );

    // Store the lookupID and strId correlation for later use
    LOOKUP_ID lookUpId = audioDeviceIDMapper->ConstructLookupID( localNameOfRemoteConsole, deviceId );
    strId = lookUpId.ToString();

    if (isKinect)
    {
        strId += L"postmec";
    }
    audioDeviceIDMapper->AddRemoteAudioDevice( lookUpId, strId );

    id.Set(strId->Data());
    // RemoteAudioDevice
    CHECKHR_EXIT( Mwrl::MakeAndInitialize<ComStyleRemoteAudioDeviceInfo>( &spOut,
        id.Get(),
        ( Awxs::AudioDeviceType )type,
        ( Awxs::AudioDeviceSharing )sharing,
        ( Awxs::AudioDeviceCategory )category ) );
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we unpack audio devices, it is "remote" audio devices for a "remote user"
//----------------------------------------------------------------------------------------
HRESULT Unpacker::UnpackAudioDevices( 
    _Out_ Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>& spOut, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper,
    _In_ uint8 localNameOfRemoteConsole
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    Mwrl::ComPtr<ComStyleRemoteAudioDeviceInfos> spTemp;
    UINT size = 0;

    // RemoteAudioDevices
    CHECKHR_EXIT( Mwrl::MakeAndInitialize<ComStyleRemoteAudioDeviceInfos>( &spTemp ) );

    CHECKHR_EXIT( UnpackUINT( &size ) );

    for ( UINT i = 0; i < size; i++ ) 
    {
        Mwrl::ComPtr<Awxs::IAudioDeviceInfo> spIAudioDevice;

        CHECKHR_EXIT( UnpackAudioDevice( spIAudioDevice, audioDeviceIDMapper, localNameOfRemoteConsole ) );
        spTemp->PushBack( spIAudioDevice );
    }

    CHECKHR_EXIT( spTemp.As( &spOut ) );
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we unpack an user, it is a "remote" user
//----------------------------------------------------------------------------------------
HRESULT Unpacker::UnpackUser( 
    _Out_ Mwrl::ComPtr<Awxs::IUser>& spOut, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper,
    _In_ uint8 localNameOfRemoteConsole
    ) 
{
    HRESULT             hr = E_UNEXPECTED;
    BYTE                isGuest = 0;
    Mwrlw::HString      xuid;
    Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>   spIAudioDevices;

    CHECKHR_EXIT( UnpackBYTE( &isGuest ) );
    CHECKHR_EXIT( UnpackString( xuid ) );
    CHECKHR_EXIT( UnpackAudioDevices( spIAudioDevices, audioDeviceIDMapper, localNameOfRemoteConsole ) );

    // RemoteUser
    CHECKHR_EXIT( Mwrl::MakeAndInitialize<ComStyleRemoteUser>( &spOut,
        isGuest,
        xuid.Get(),
        spIAudioDevices.Get() ) );
exit:
    return hr;
}

}}}

#endif