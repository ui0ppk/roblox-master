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

private class ErrorUtils
{
public:
    /// <summary>
    /// Converts HTTP status Code (e.g., 200, 401, 304) to HRESULT
    /// </summary>
    static HRESULT
    ConvertHttpStatusCodeToHR(
        _In_ uint32 HttpStatusCode
        );

    static HRESULT
    ConvertExceptionToHRESULT();
    
    static Platform::String^ 
    ConvertHResultToString( HRESULT hr );

    static Platform::String^ 
    ConvertHResultToErrorName( HRESULT hr );
};

}}}

#endif