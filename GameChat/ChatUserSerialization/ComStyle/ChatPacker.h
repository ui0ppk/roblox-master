//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include "ChatUserSerializationCommon.h"
#include "ChatReadWriteBuffer.h"
#include "AudioDeviceIDMapper.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

// Class that used to perform data serialization
class Packer 
{
public:
    Packer();
    ~Packer();

    HRESULT PackBegin();

    HRESULT PackEnd( 
        _Outptr_ BYTE** ppOut, 
        _Out_ UINT* pOut 
        );

    HRESULT PackBYTE( 
        _In_ const BYTE data 
        );

    HRESULT PackUSHORT( 
        _In_ const USHORT data 
        );

    HRESULT PackUINT( 
        _In_ const UINT data 
        );

    HRESULT PackBytes( 
        _In_reads_bytes_(size) const BYTE* const pData, 
        _In_ const UINT size 
        );

    HRESULT PackString( 
        _In_ const Mwrlw::HString& data 
        );

    HRESULT PackBuffer( 
        _In_ const Mwrl::ComPtr<Awss::IBuffer>& spData 
        );

    HRESULT PackAudioDevice( 
        _In_ const Mwrl::ComPtr<Awxs::IAudioDeviceInfo>& spData, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
        );

    HRESULT PackAudioDevices( 
        _In_ const Mwrl::ComPtr<Awfc::IVectorView<Awxs::IAudioDeviceInfo*>>& spData, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
        );

    HRESULT PackUser( 
        _In_ const Mwrl::ComPtr<Awxs::IUser>& spData, 
        _In_ AudioDeviceIDMapper^ audioDeviceIDMapper 
        );

private:
    ReadWriteBuffer m_buffer;

    static HRESULT ConvertWideToUtf8( _In_ PCWSTR wideStr, _Inout_ std::string* pUtf8Str);
};

}}}

#endif
