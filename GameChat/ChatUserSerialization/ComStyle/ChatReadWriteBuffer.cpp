//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ChatReadWriteBuffer.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

ReadWriteBuffer::ReadWriteBuffer( const unsigned int capa ) : 
    m_pData( nullptr ), 
    m_capacity( 0 ), 
    m_isCreated( false ), 
    m_pR( nullptr ),
    m_pW( nullptr )
{
    if ( capa ) 
    {
        m_pData.reset(new BYTE[capa]);
        if ( m_pData ) 
        {
            m_capacity = capa;
            m_isCreated = true;
            Cleanup();
        }
    }
}

ReadWriteBuffer::~ReadWriteBuffer() 
{
}

HRESULT ReadWriteBuffer::Cleanup() 
{
    if ( m_isCreated ) 
    {
        m_pR = m_pW = m_pData.get();
        return S_OK;
    }
    return E_UNEXPECTED;
}

HRESULT ReadWriteBuffer::Length( unsigned int* pOut ) const 
{
    if ( m_isCreated ) 
    {
        *pOut = ( unsigned int )( m_pW - m_pR );
        return S_OK;
    }
    return E_UNEXPECTED;
}

// IMPORTANT
// 1. When m_pR == m_pW, m_pR is pointing to garbage data, or it is out of boundary
// 2. When m_pW == ( m_pData + m_capacity ), m_pW is out of boundary
// We treat size zero as unexpected behaviors when performing read and write

// ReadFast does not copy memory out, just return the start address
HRESULT ReadWriteBuffer::ReadFast( BYTE** ppOut, const unsigned int size ) 
{
    if ( size != 0 && IsAbleToRead( size ) ) 
    {
        *ppOut = m_pR;
        m_pR += size;
        return S_OK;
    }
    return E_UNEXPECTED;
}

HRESULT ReadWriteBuffer::Write( const BYTE* const pData, const unsigned int size ) 
{
    if ( size != 0 && IsAbleToWrite( size ) ) 
    {
        if ( !memcpy_s( m_pW, size, pData, size ) ) 
        {
            m_pW += size;
            return S_OK;
        }
    }
    return E_UNEXPECTED;
}

bool ReadWriteBuffer::IsAbleToRead( const unsigned int size ) const {
    if ( m_isCreated ) {
        return ( m_pR + size ) <= ( m_pW );
    }
    return false;
}

bool ReadWriteBuffer::IsAbleToWrite( const unsigned int size ) const 
{
    if ( m_isCreated ) 
    {
        return ( m_pW + size ) <= ( m_pData.get() + m_capacity );
    }
    return false;
}

}}}

#endif