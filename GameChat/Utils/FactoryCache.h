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
    
class FactoryCache
{
public:
    FactoryCache();

    inline Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> GetBufferFactory()
    {
        return m_bufferFactory;
    }

private:
    Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBufferFactory> m_bufferFactory;
};

}}}

#endif