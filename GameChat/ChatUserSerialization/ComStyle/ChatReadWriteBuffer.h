//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "ChatUserSerializationCommon.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

#define DEFAULT_CAPACITY    8192

// Fixed capacity buffer with a read and a write pointer
class ReadWriteBuffer 
{
public:
                            ReadWriteBuffer( const unsigned int capa = DEFAULT_CAPACITY );
                            ~ReadWriteBuffer();

    HRESULT                 Cleanup();
    HRESULT                 Length( unsigned int* pOut ) const;
    HRESULT                 ReadFast( BYTE** ppOut, const unsigned int size );
    HRESULT                 Write( const BYTE* const pData, const unsigned int size );

private:
    std::unique_ptr<BYTE[]> m_pData;
    unsigned int            m_capacity;
    bool                    m_isCreated;
    BYTE*                   m_pR;
    BYTE*                   m_pW;

    bool                    IsAbleToRead( const unsigned int size ) const;
    bool                    IsAbleToWrite( const unsigned int size ) const;
};

}}}

#endif