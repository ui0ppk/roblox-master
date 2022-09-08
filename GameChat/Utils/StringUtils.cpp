//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "StringUtils.h"
#include "ErrorUtils.h"

#if TV_API

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

Platform::String^ 
StringUtils::GetLogExceptionDebugInfo(
    _In_ HRESULT hr,
    _In_opt_ PCWSTR pwszFunction,
    _In_opt_ PCWSTR pwszFile,
    _In_ uint32 uLine
    )
{
    std::wstring info = L"[Exception]: HRESULT: ";
    info += ErrorUtils::ConvertHResultToString(hr)->Data();
    info += L"\n";
    if( pwszFunction != nullptr )
    {
        info += L"\t\tFunction: ";
        info += pwszFunction;
        info += L"\n";
    }
    if( pwszFile != nullptr )
    {
        info += L"\t\tFile:";
        info += pwszFile;
        info += L"(";
        info += uLine.ToString()->Data();
        info += L")\n";
    }

    return ref new Platform::String( info.c_str() );
}

bool StringUtils::IsStringEqualCaseInsenstive( 
    _In_ Platform::String^ val1, 
    _In_ Platform::String^ val2 
    )
{
    return ( _wcsicmp(val1->Data(), val2->Data()) == 0 );
}

Platform::String^
StringUtils::GetStringFormat( 
    _In_ LPCWSTR strMsg,
    _In_ va_list args
    )
{
    WCHAR strBuffer[2048];
    _vsnwprintf_s( strBuffer, 2048, _TRUNCATE, strMsg, args );
    strBuffer[2047] = L'\0';
    return ref new Platform::String(strBuffer);
}

Platform::String^ 
StringUtils::FormatString( LPCWSTR strMsg, ... )
{
    WCHAR strBuffer[2048];

    va_list args;
    va_start(args, strMsg);
    _vsnwprintf_s( strBuffer, 2048, _TRUNCATE, strMsg, args );
    strBuffer[2047] = L'\0';

    va_end(args);

    Platform::String^ str = ref new Platform::String(strBuffer);
    return str;
}


}}}

#endif