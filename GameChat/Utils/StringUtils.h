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
    
private class StringUtils
{
public:
    static Platform::String^ 
    GetLogExceptionDebugInfo(
        _In_ HRESULT hr,
        _In_opt_ PCWSTR pwszFunction,
        _In_opt_ PCWSTR pwszFile,
        _In_ uint32 uLine
        );

    static bool IsStringEqualCaseInsenstive( 
        _In_ Platform::String^ val1, 
        _In_ Platform::String^ val2 
        );

    static Platform::String^
    GetStringFormat( 
        _In_ LPCWSTR strMsg, 
        _In_ va_list args
        );

    static Platform::String^ 
    FormatString( LPCWSTR strMsg, ... );
};

}}}

#endif