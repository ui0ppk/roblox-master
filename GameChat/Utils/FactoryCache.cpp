//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "FactoryCache.h"
#include "StringUtils.h"

#if TV_API

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

FactoryCache::FactoryCache()
{
    HRESULT hr;
    hr = Windows::Foundation::GetActivationFactory( Microsoft::WRL::Wrappers::HStringReference( RuntimeClass_Windows_Storage_Streams_Buffer ).Get(), &m_bufferFactory );
    CHAT_THROW_IF_HR_FAILED( hr );
    CHAT_THROW_E_POINTER_IF_NULL( m_bufferFactory );
}

}}}

#endif