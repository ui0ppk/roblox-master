//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include <vector>
#include "FactoryCache.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {


enum JitterBufferState
{
    NeedPackets = 0,
    Draining
};

class IJitterBuffer 
{
public:
    virtual HRESULT Push( _In_reads_(sourceBufferLengthInBytes) BYTE* sourceBuffer, _In_ UINT sourceBufferLengthInBytes ) = 0;
    virtual HRESULT GetFront( _Out_ Windows::Storage::Streams::IBuffer^* buffer ) = 0;
    virtual HRESULT Pop() = 0;
    virtual UINT GetCurrentPacketCount() = 0;
    virtual UINT GetLowestNeededPacketCount() = 0;
    virtual UINT GetMaxPacketsInRingBuffer() = 0;
    virtual UINT GetDynamicNeededPacketCount() = 0;
};

class JitterBuffer : public IJitterBuffer
{
public:
    JitterBuffer(
        _In_ UINT maxBytesInPeriod,
        _In_ UINT maxPacketsInRingBuffer,
        _In_ UINT lowestNeededPacketCount,
        _In_ UINT packetsBeforeRelaxingNeeded,
        _In_ std::shared_ptr<FactoryCache> factoryCache
        );

    virtual ~JitterBuffer();

    // IJitterBuffer

    /// <summary>
    /// Title is pushing the next buffer of data.
    /// </summary>    
    virtual HRESULT
    Push(
        _In_reads_(sourceBufferLengthInBytes) BYTE* sourceBuffer, 
        _In_ UINT sourceBufferLengthInBytes
        );

    /// <summary>
    /// Title is asking for a reference to the next IBuffer that needs to be rendered.
    /// </summary>    
    virtual HRESULT 
    GetFront(
        _Out_ Windows::Storage::Streams::IBuffer^* buffer
        );

    /// <summary>
    /// Title is telling the JitterBuffer that it is ok to reuse the last IBuffer that was
    /// handed down by the Jitter Buffer, essentially advancing the JB ring buffer.
    /// </summary>
    virtual HRESULT Pop();

    /// <summary>
    /// Title can use this property to query the current size/latency, expressed in periods/packets.
    /// </summary>
    virtual UINT GetCurrentPacketCount();

    /// <summary>
    /// Title can use this property to query the min needed packet count, expressed in periods.
    /// </summary>
    virtual UINT GetLowestNeededPacketCount();

    /// <summary>
    /// Title can use this property to query the max packets in the ring buffer, expressed in periods.
    /// </summary>
    virtual UINT GetMaxPacketsInRingBuffer();

    /// <summary>
    /// Title can use this property to query the current dynamic needed packets, expressed in periods.
    /// </summary>
    virtual UINT GetDynamicNeededPacketCount();

private:

    // Ring buffer
    UINT m_maxPacketsInRingBuffer;
    UINT m_maxBytesInPeriod;
    std::vector<Windows::Storage::Streams::IBuffer^> m_ringBuffer;
    UINT m_readIndex;
    UINT m_writeIndex;
    UINT m_currentPacketCount;
    std::shared_ptr<FactoryCache> m_factoryCache;

    // Heuristics
    UINT m_dynamicNeededPacketCount;
    UINT m_lowestNeededPacketCount;
    UINT m_packetsBeforeRelaxingNeeded;
    UINT m_numberOfPacketsAboveNeededWhileDraining;
    JitterBufferState m_state;
};

}}}

#endif