//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {
    
private class BufferUtils
{
public:
    static Windows::Storage::Streams::IBuffer^ FastBufferCreate(
        _In_ uint32 capacity,
        _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
        );

    /// <summary>
    /// Creates a copy of a buffer from an existing buffer
    /// </summary>
    /// <param name="source">The buffer to copy</param>
    /// <returns>A new buffer that contains the contents of the input buffer</returns>
    static Windows::Storage::Streams::IBuffer^ BufferCopy(
        _In_ Windows::Storage::Streams::IBuffer^ source,
        _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
        );

    /// <summary>
    /// Gets a byte* from a buffer
    /// </summary>
    /// <param name="buffer">The buffer to get the byte* from</param>
    /// <param name="ppOut">A pointer to a byte* that will be set</param>
    static void GetBufferBytes( 
        _In_ Windows::Storage::Streams::IBuffer^ buffer, 
        _Outptr_ byte** ppOut 
        );

    /// <summary>
    /// Creates a new buffer from a byte array
    /// </summary>
    /// <param name="sourceByteBuffer">The input byte array</param>
    /// <param name="sourceByteBufferSize">The length of the sourceByteBuffer in number of bytes</param>
    /// <returns>A new buffer that contains the contents of byte array</returns>
    static Windows::Storage::Streams::IBuffer^ CreateBufferFromBytes( 
        _In_reads_bytes_(sourceByteBufferSize) byte* sourceByteBuffer, 
        _In_ UINT sourceByteBufferSize,
        _In_ Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> bufferFactory
        );

    static Platform::String^ GetBase64String( 
        _In_ Windows::Storage::Streams::IBuffer^ buffer 
        );

    static int GetLength( 
        _In_ Windows::Storage::Streams::IBuffer^ buffer 
        );
};

}}}

#endif