//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatPacker.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Packer::Packer() 
{
}

Packer::~Packer() 
{
}

HRESULT Packer::PackBegin() 
{
    HRESULT hr = E_UNEXPECTED;

    CHECKHR_EXIT( m_buffer.Cleanup() );
exit:
    return hr;
}

// Returns the packed data, if there is no packed data, the behavior is E_UNEXPECTED
HRESULT Packer::PackEnd( 
    _Outptr_ BYTE** ppOut, 
    _Out_ UINT* pOut 
    ) 
{
    HRESULT hr = E_UNEXPECTED;

    CHECKHR_EXIT( m_buffer.Length( pOut ) );
    CHECKHR_EXIT( m_buffer.ReadFast( ppOut, *pOut ) );
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// Pack functions
// 1. Pack primitive types
//      a. No input data validation
//      b. Input data validation
// 2. Pack complex types
//      a. Input data validation( Not handled in sub-pack functions )
//      b. Group of sub-pack functions
//----------------------------------------------------------------------------------------
HRESULT Packer::PackBYTE( 
    _In_ const BYTE data 
    ) 
{
    HRESULT hr = E_UNEXPECTED;

    // No input data validation
    CHECKHR_EXIT( m_buffer.Write( &data, sizeof( BYTE ) ) );
exit:
    return hr;
}

HRESULT Packer::PackUSHORT( 
    _In_ const USHORT data 
    ) 
{
    HRESULT hr = E_UNEXPECTED;

    // No input data validation
    CHECKHR_EXIT( m_buffer.Write( (BYTE*)&data, sizeof( USHORT ) ) );
exit:
    return hr;
}

HRESULT Packer::PackUINT( 
    _In_ const UINT data 
    ) 
{
    HRESULT hr = E_UNEXPECTED;

    // No input data validation
    CHECKHR_EXIT( m_buffer.Write( ( BYTE* )&data, sizeof( UINT ) ) );
exit:
    return hr;
}

HRESULT Packer::PackBytes( 
    _In_reads_bytes_(size) const BYTE* const pData, 
    _In_ const UINT size 
    ) 
{
    HRESULT hr = S_FALSE; // if size if 0 or no data

    // Input data validation
    if ( pData && size ) {
        CHECKHR_EXIT( m_buffer.Write( pData, size ) );
    }
exit:
    return hr;
}

HRESULT Packer::PackString( 
    _In_ const Mwrlw::HString& data 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    UINT strLen = 0;
    UINT size = 0;

    // Input data validation
    if ( data != nullptr )
    {
        const wchar_t* pStr = data.GetRawBuffer( &strLen );
        size = strLen * sizeof( wchar_t );

        std::string pUtf8Str;
        ConvertWideToUtf8(pStr, &pUtf8Str);

        CHECKHR_EXIT( PackUINT( (UINT)pUtf8Str.size() ) );
        CHECKHR_EXIT( PackBytes( ( BYTE* )pUtf8Str.data(), (UINT)pUtf8Str.size() ) );
    }
exit:
    return hr;
}

HRESULT Packer::PackBuffer( 
    _In_ const Mwrl::ComPtr<Awss::IBuffer>& spData ) 
{
    HRESULT hr = E_UNEXPECTED;
    UINT size = 0;
    Mwrl::ComPtr<Wss::IBufferByteAccess> spIBufferByteAccess;
    BYTE* pBytes = nullptr;

    // Input data validation
    if ( spData ) {
        CHECKHR_EXIT( spData->get_Length( &size ) );
        CHECKHR_EXIT( spData.As( &spIBufferByteAccess ) );
        CHECKHR_EXIT( spIBufferByteAccess->Buffer( &pBytes ) );

        CHECKHR_EXIT( PackUINT( size ) );
        CHECKHR_EXIT( PackBytes( pBytes, size ) );
    }
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we pack an audio device, it is a "local" audio device from a "local" user
// 2. Id, type and sharing, use BYTE to represent enum
//----------------------------------------------------------------------------------------
HRESULT Packer::PackAudioDevice( 
    _In_ const Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spData, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    Mwrlw::HString id;
    Awxs::AudioDeviceType type;
    Awxs::AudioDeviceSharing sharing;
    Awxs::AudioDeviceCategory category;

    // Input data validation
    if ( spData ) 
    {
        CHECKHR_EXIT( spData->get_Id( id.GetAddressOf() ) );
        CHECKHR_EXIT( spData->get_DeviceType( &type ) );
        CHECKHR_EXIT( spData->get_Sharing( &sharing ) );
        CHECKHR_EXIT( spData->get_DeviceCategory( &category ) );

        // Save and send a deviceId 
        UINT length = 0;
        Platform::String^ strId = ref new Platform::String( id.GetRawBuffer(&length) );
        DEVICE_ID deviceId = audioDeviceIDMapper->GetLocalDeviceID(strId);
        CHECKHR_EXIT( PackBYTE( deviceId ) );

        uint8  isKinect = (wcsstr(strId->Data(), L"postmec") != nullptr);
        CHECKHR_EXIT( PackBYTE( ( BYTE )isKinect ) );
        CHECKHR_EXIT( PackBYTE( ( BYTE )type ) );
        CHECKHR_EXIT( PackBYTE( ( BYTE )sharing ) );
        CHECKHR_EXIT( PackBYTE( ( BYTE )category ) );

#ifdef DEBUG_REMOTE_AUDIO_DEVICES
        WCHAR text[1024] = {0};
        swprintf_s(
            text, ARRAYSIZE(text), 
            L"Packer::PackAudioDevice: strId = %s, deviceId = %d, isKinect = %d\r\n",
            strId->Data(),
            deviceId,
            isKinect
            );  
        OutputDebugString( text );
#endif
    }
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we pack audio devices, it is "local" audio devices from a "local" user
//----------------------------------------------------------------------------------------
HRESULT Packer::PackAudioDevices( 
    _In_ const Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>& spData, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    UINT size = 0;

    // Input data validation
    if ( spData ) {
        // Pack size
        CHECKHR_EXIT( spData->get_Size( &size ) );
        CHECKHR_EXIT( PackUINT( size ) );

        // Pack each Mwrl::ComPtr<Awxs::IAudioDeviceInfo>
        for ( UINT i = 0; i < size; i++ ) {
            Mwrl::ComPtr<Awxs::IAudioDeviceInfo> spIAudioDevice;

            CHECKHR_EXIT( spData->GetAt( i, &spIAudioDevice ) );
            CHECKHR_EXIT( PackAudioDevice( spIAudioDevice, audioDeviceIDMapper ) );
        }
    }
exit:
    return hr;
}

//----------------------------------------------------------------------------------------
// IMPORTANT
// 1. When we pack an user, it is "local" user
//----------------------------------------------------------------------------------------
HRESULT Packer::PackUser( 
    _In_ const Mwrl::ComPtr<Awxs::IUser>& spData, 
    _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
    ) 
{
    HRESULT hr = E_UNEXPECTED;
    boolean isGuest = false;
    Mwrlw::HString xuid;
    Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>> spIAudioDevices;

    // Input data validation
    if ( spData ) {
        CHECKHR_EXIT( spData->get_IsGuest( &isGuest ) );
        CHECKHR_EXIT( spData->get_XboxUserId( xuid.GetAddressOf() ) );
        CHECKHR_EXIT( spData->get_AudioDevices( &spIAudioDevices ) );

        CHECKHR_EXIT( PackBYTE( isGuest ) );
        CHECKHR_EXIT( PackString( xuid ) );
        CHECKHR_EXIT( PackAudioDevices( spIAudioDevices, audioDeviceIDMapper ) );
    }
exit:
    return hr;
}

HRESULT Packer::ConvertWideToUtf8(
    _In_ PCWSTR wideStr,
    _Inout_ std::string* pUtf8Str
    )
{
    if ( !pUtf8Str )
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    try
    {
        pUtf8Str->clear();
        if (wideStr == nullptr || wideStr[0] == L'\0')
            return S_OK;

        int wideStrLen = (int)wcslen(wideStr) + 1; // WideCharToMultiByte expects the NULL terminator to be counted

        // calculate the required size
        int nBytes = WideCharToMultiByte(
            CP_UTF8,
            0,
            wideStr,
            wideStrLen,
            nullptr,
            0,
            nullptr,
            nullptr);

        if(nBytes == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        std::unique_ptr<char[]> utf8Buffer(new char[nBytes]);

        if(WideCharToMultiByte(
            CP_UTF8,
            0,
            wideStr,
            wideStrLen,
            utf8Buffer.get(),
            nBytes,
            nullptr,
            nullptr) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        pUtf8Str->assign(utf8Buffer.get());

        hr = S_OK;
        goto Exit;
    }
    catch (std::bad_alloc)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}

}}}

#endif