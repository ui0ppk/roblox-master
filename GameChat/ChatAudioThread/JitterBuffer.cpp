//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "JitterBuffer.h"
#include "StringUtils.h"
#include "BufferUtils.h"

#if TV_API

using namespace Microsoft::WRL;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

JitterBuffer::JitterBuffer(
    _In_ UINT maxBytesInPeriod,
    _In_ UINT maxPacketsInRingBuffer,
    _In_ UINT lowestNeededPacketCount,
    _In_ UINT packetsBeforeRelaxingNeeded,
    _In_ std::shared_ptr<FactoryCache> factoryCache
    ) :
    m_maxPacketsInRingBuffer(maxPacketsInRingBuffer),
    m_maxBytesInPeriod(maxBytesInPeriod),
    m_readIndex(0),
    m_writeIndex(0),
    m_currentPacketCount(0),
    m_dynamicNeededPacketCount(lowestNeededPacketCount), // Just set to lowest and it'll automatically adjust
    m_lowestNeededPacketCount(lowestNeededPacketCount),
    m_packetsBeforeRelaxingNeeded(packetsBeforeRelaxingNeeded), 
    m_numberOfPacketsAboveNeededWhileDraining(0),
    m_state(NeedPackets),
    m_factoryCache(factoryCache)
{
    CHAT_THROW_INVALIDARGUMENT_IF_NULL(factoryCache);

    for ( UINT i=0; i < m_maxPacketsInRingBuffer; i++ )
    {
        m_ringBuffer.push_back( BufferUtils::FastBufferCreate( m_maxBytesInPeriod, m_factoryCache->GetBufferFactory() ) );
    }
}

JitterBuffer::~JitterBuffer()
{
}

HRESULT 
JitterBuffer::Push(
    _In_reads_(sourceBufferLengthInBytes) BYTE* sourceBuffer, 
    _In_ UINT sourceBufferLengthInBytes
    )
{
    CHAT_THROW_E_POINTER_IF_NULL( sourceBuffer );
    CHAT_THROW_INVALIDARGUMENT_IF( sourceBufferLengthInBytes > m_maxBytesInPeriod );

    // Fail if the caller pushed more than m_maxPacketCount
    if( m_currentPacketCount >= m_maxPacketsInRingBuffer )
    {
        return HRESULT_FROM_WIN32( ERROR_BUFFER_OVERFLOW );
    }

    IBuffer^ buffer = m_ringBuffer[ m_writeIndex ];
    BYTE* bufferBytes = NULL;
    BufferUtils::GetBufferBytes( buffer, &bufferBytes );

    errno_t err = memcpy_s( bufferBytes, buffer->Capacity, sourceBuffer, sourceBufferLengthInBytes );
    CHAT_THROW_HR_IF(err != 0, E_FAIL); // This should not happen
    buffer->Length = sourceBufferLengthInBytes;

    // Advance to the next empty buffer in the ring buffer and update the size.
    m_currentPacketCount++;
    m_writeIndex++;
    m_writeIndex %= m_maxPacketsInRingBuffer;
    
    // Heuristics for adjusting the dynamic needed packet count
    if ( m_state == NeedPackets )
    {
        if ( m_currentPacketCount >= m_dynamicNeededPacketCount )
        {
            m_state = Draining;
        }
        else
        {
            // Stay in NeedPackets
        }
    } 
    else // if ( m_state == Draining )
    {
        m_numberOfPacketsAboveNeededWhileDraining++;

        // After we get above m_packetsBeforeRelaxingNeeded, we can relax the needed packet
        if ( m_numberOfPacketsAboveNeededWhileDraining > m_packetsBeforeRelaxingNeeded )
        {
            m_numberOfPacketsAboveNeededWhileDraining = 0;
            if( m_dynamicNeededPacketCount > 0 )
            {
                m_dynamicNeededPacketCount--;
                m_dynamicNeededPacketCount = max(m_dynamicNeededPacketCount, m_lowestNeededPacketCount);
            }
        }
    } 

    return S_OK;
}

HRESULT 
JitterBuffer::GetFront(
    _Out_ Windows::Storage::Streams::IBuffer^* pBuffer
    )
{
    if( m_currentPacketCount == 0 || 
        m_state == NeedPackets )
    {
        // Return failure if we're empty or we still haven't gotten above the needed packet count
        *pBuffer = nullptr;
        return HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS );
    }
    else // if ( m_state == Draining )
    {
        *pBuffer = m_ringBuffer[ m_readIndex ];
        return S_OK;
    } 
}

HRESULT 
JitterBuffer::Pop()
{
    if( m_currentPacketCount == 0 || 
        m_state == NeedPackets )
    {
        // Return failure if we're empty or we still haven't gotten above the needed packet count
        return HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS );
    } 
    else // if ( m_state == Draining )
    {
        m_readIndex++;
        m_readIndex %= m_maxPacketsInRingBuffer;
        m_currentPacketCount--;

        if ( m_currentPacketCount < m_lowestNeededPacketCount )
        {
            // When going below the needed packet count, reset the number of packets we have gotten
            m_numberOfPacketsAboveNeededWhileDraining = 0;
        }

        if ( m_currentPacketCount == 0 )
        {
            // When we hit 0 packets, then we have fully drained and switch the state to NeedPackets
            m_state = NeedPackets;

            // We hit 0 packet, so increase the needed packet count for more safety but higher latency
            m_dynamicNeededPacketCount++;
            m_dynamicNeededPacketCount = min(m_dynamicNeededPacketCount, m_maxPacketsInRingBuffer);
        }

        return S_OK;
    } 
}

UINT
JitterBuffer::GetCurrentPacketCount()
{
    return m_currentPacketCount;
}

UINT
JitterBuffer::GetLowestNeededPacketCount()
{
    return m_lowestNeededPacketCount;
}

UINT
JitterBuffer::GetMaxPacketsInRingBuffer()
{
    return m_maxPacketsInRingBuffer;
}

UINT JitterBuffer::GetDynamicNeededPacketCount()
{
    return m_dynamicNeededPacketCount;
}


}}}

#endif