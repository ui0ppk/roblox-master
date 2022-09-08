//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include "ChatUserSerializationCommon.h"
#include "ChatReadWriteBuffer.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

// Class that used to perform data deserialization
class Unpacker 
{
public:
    Unpacker();
    ~Unpacker();

    HRESULT UnpackBegin( 
        _In_reads_bytes_(size) const BYTE* const pData, 
        _In_ const UINT size 
        );

    HRESULT UnpackEnd() const;

    HRESULT UnpackBYTE( 
        _Out_ BYTE* pOut 
        );

    HRESULT UnpackUSHORT( 
        _Out_ USHORT* pOut 
        );

    HRESULT UnpackUINT( 
        _Out_ UINT* pOut 
        );

    HRESULT UnpackBytesFast( 
        _Outptr_result_maybenull_ BYTE** ppOut, 
        _In_ const UINT size 
        );

    HRESULT UnpackString( 
        _Out_ Mwrlw::HString& out 
        );

    HRESULT UnpackBufferFast( 
        _Outptr_result_maybenull_ BYTE** ppOut, 
        _Out_ UINT* pOut 
        );

    HRESULT UnpackAudioDevice( 
        _Out_ Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spOut, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper, 
        _In_ uint8 localNameOfRemoteConsole 
        );

    HRESULT UnpackAudioDevices( 
        _Out_ Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>& spOut, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper, 
        _In_ uint8 localNameOfRemoteConsole 
        );

    HRESULT UnpackUser( 
        _Out_ Mwrl::ComPtr<Awxs::IUser>& spOut, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper, 
        _In_ uint8 localNameOfRemoteConsole 
        );

private:
    ReadWriteBuffer m_buffer;
};

}}}

#endif