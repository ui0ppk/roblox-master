//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "BufferUtils.h"
#include "StringUtils.h"
#include "ErrorUtils.h"

#if TV_API

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Windows::Storage::Streams::IBuffer^ 
BufferUtils::FastBufferCreate(
    _In_ uint32 capacity,
    _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
    )
{
    // All of this can be done with this C++/CX line:
    //      Windows::Storage::Streams::IBuffer^ cxBuffer = ref new Buffer(capacity);
    // 
    // However there is a performance hit since inside this call is slow to lookup the activation factory by string.
    // The caller should cache the result of GetActivationFactory and reuse it upon sequential calls.
    //
    // Create the factory like so:
    // hr = GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference( RuntimeClass_Windows_Storage_Streams_Buffer ).Get(), &bufferFactory);

    CHAT_THROW_INVALIDARGUMENT_IF_NULL( bufferFactory );

    HRESULT hr;
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> abiBuffer;
    hr = bufferFactory->Create( capacity, &abiBuffer );
    CHAT_THROW_IF_HR_FAILED( hr );
    CHAT_THROW_E_POINTER_IF_NULL( abiBuffer );

    Windows::Storage::Streams::IBuffer^ cxBuffer = reinterpret_cast<Windows::Storage::Streams::IBuffer^>(abiBuffer.Get());
    return cxBuffer;
}


Windows::Storage::Streams::IBuffer^ 
BufferUtils::BufferCopy(
    _In_ Windows::Storage::Streams::IBuffer^ source,
    _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
    )
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(source);
    
    byte* srcBufferBytes = nullptr;
    GetBufferBytes( source, &srcBufferBytes );

    Windows::Storage::Streams::IBuffer^ destination = BufferUtils::FastBufferCreate(source->Length, bufferFactory);

    byte* destBufferBytes = nullptr;
    GetBufferBytes( destination, &destBufferBytes );

    errno_t err = memcpy_s(
        destBufferBytes,
        destination->Capacity,
        srcBufferBytes,
        source->Length
        );

    // Params were validated prior to the memcpy_s call, so this should never happen
    CHAT_THROW_HR_IF(err != 0, E_FAIL);

    destination->Length = source->Length;
    return destination;
}

void BufferUtils::GetBufferBytes( 
    _In_ Windows::Storage::Streams::IBuffer^ buffer, 
    _Outptr_ byte** ppOut 
    )
{
    if ( ppOut == nullptr || buffer == nullptr )
    {
        throw ref new Platform::InvalidArgumentException();
    }

    *ppOut = nullptr;

    Microsoft::WRL::ComPtr<IInspectable> srcBufferInspectable(reinterpret_cast<IInspectable*>( buffer ));
    Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> srcBufferByteAccess;
    HRESULT hr = srcBufferInspectable.As(&srcBufferByteAccess);
    CHAT_THROW_IF_HR_FAILED(hr);

    hr = srcBufferByteAccess->Buffer(ppOut);
    CHAT_THROW_IF_HR_FAILED(hr);
}

Windows::Storage::Streams::IBuffer^ BufferUtils::CreateBufferFromBytes(
    _In_reads_bytes_(sourceByteBufferSize) byte* sourceByteBuffer, 
    _In_ UINT sourceByteBufferSize,
    _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
    )
{
    Windows::Storage::Streams::IBuffer^ destBuffer = BufferUtils::FastBufferCreate(sourceByteBufferSize, bufferFactory);

    byte* destBufferBytes = nullptr;
    GetBufferBytes( destBuffer, &destBufferBytes );

    errno_t err = memcpy_s( destBufferBytes, destBuffer->Capacity, sourceByteBuffer, sourceByteBufferSize );
    CHAT_THROW_HR_IF(err != 0, E_FAIL);
    destBuffer->Length = sourceByteBufferSize;

    return destBuffer;
}

Platform::String^ 
BufferUtils::GetBase64String( 
    _In_ Windows::Storage::Streams::IBuffer^ buffer 
    )
{
    if( buffer == nullptr )
    {
        return L"";
    }

    UINT32 length = buffer->Length;
    if ( !length )
    {
        return L"";
    }

    // Read off the buffer, encode in base64, and push onto the string.
    auto dataReader = Windows::Storage::Streams::DataReader::FromBuffer( buffer );

    Platform::String^ base64EncodedConnectivityInfo = ref new Platform::String(L"");
    static const char s_chBase64EncodingTable[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
        'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
    while (length >= 3)
    {
        UINT8 byte0 = dataReader->ReadByte();
        UINT8 byte1 = dataReader->ReadByte();
        UINT8 byte2 = dataReader->ReadByte();
        wchar_t Digit[5];
        UCHAR index0 = (byte0 >> 2);
        UCHAR index1 = ((byte0 & 3/*00000011*/) << 4) + ((byte1 & 240/*11110000*/) >> 4);
        UCHAR index2 = ((byte2 & 192/*11000000*/) >> 6) + ((byte1 & 15/*00001111*/) << 2);
        UCHAR index3 = (byte2 & 63/*00111111*/ );
        Digit[0] = s_chBase64EncodingTable[index0];
        Digit[1] = s_chBase64EncodingTable[index1];
        Digit[2] = s_chBase64EncodingTable[index2];
        Digit[3] = s_chBase64EncodingTable[index3];
        Digit[4] = 0;
        base64EncodedConnectivityInfo += ref new Platform::String(Digit);
        length -= 3;
    }
    if(length == 1) // 2 equals
    {
        UINT8 byte0 = dataReader->ReadByte();
        UINT8 byte1 = 0;
        wchar_t Digit[5];
        UCHAR index0 = (byte0 >> 2);
        UCHAR index1 = ((byte0 & 3/*00000011*/) << 4) + ((byte1 & 240/*11110000*/) >> 4);
        Digit[0] = s_chBase64EncodingTable[index0];
        Digit[1] = s_chBase64EncodingTable[index1];
        Digit[2] = L'=';
        Digit[3] = L'=';
        Digit[4] = 0;
        base64EncodedConnectivityInfo += ref new Platform::String(Digit);
    }
    else if(length == 2) // 1 equals
    {
        UINT8 byte0 = dataReader->ReadByte();
        UINT8 byte1 = dataReader->ReadByte();
        UINT8 byte2 = 0;
        wchar_t Digit[5];
        UCHAR index0 = (byte0 >> 2);
        UCHAR index1 = ((byte0 & 3/*00000011*/) << 4) + ((byte1 & 240/*11110000*/) >> 4);
        UCHAR index2 = ((byte2 & 192/*11000000*/) >> 6) + ((byte1 & 15/*00001111*/) << 2);
        Digit[0] = s_chBase64EncodingTable[index0];
        Digit[1] = s_chBase64EncodingTable[index1];
        Digit[2] = s_chBase64EncodingTable[index2];
        Digit[3] = L'=';
        Digit[4] = 0;
        base64EncodedConnectivityInfo += ref new Platform::String(Digit);
    }
    return base64EncodedConnectivityInfo;
}

int 
BufferUtils::GetLength( 
    _In_ Windows::Storage::Streams::IBuffer^ buffer 
    )
{
    return ( buffer != nullptr ) ? buffer->Length : 0;
}


}}}

#endif