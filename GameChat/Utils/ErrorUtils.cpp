//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "ErrorUtils.h"

#if TV_API

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

enum WebErrorStatus
{
    WebErrorStatus_Unknown = 0,
    WebErrorStatus_CertificateCommonNameIsIncorrect = 1,
    WebErrorStatus_CertificateExpired = 2,
    WebErrorStatus_CertificateContainsErrors = 3,
    WebErrorStatus_CertificateRevoked = 4,
    WebErrorStatus_CertificateIsInvalid = 5,
    WebErrorStatus_ServerUnreachable = 6,
    WebErrorStatus_Timeout = 7,
    WebErrorStatus_ErrorHttpInvalidServerResponse = 8,
    WebErrorStatus_ConnectionAborted = 9,
    WebErrorStatus_ConnectionReset = 10,
    WebErrorStatus_Disconnected = 11,
    WebErrorStatus_HttpToHttpsOnRedirection = 12,
    WebErrorStatus_HttpsToHttpOnRedirection = 13,
    WebErrorStatus_CannotConnect = 14,
    WebErrorStatus_HostNameNotResolved = 15,
    WebErrorStatus_OperationCanceled = 16,
    WebErrorStatus_RedirectFailed = 17,
    WebErrorStatus_UnexpectedStatusCode = 18,
    WebErrorStatus_UnexpectedRedirection = 19,
    WebErrorStatus_UnexpectedClientError = 20,
    WebErrorStatus_UnexpectedServerError = 21,
    WebErrorStatus_MultipleChoices = 300,
    WebErrorStatus_MovedPermanently = 301,
    WebErrorStatus_Found = 302,
    WebErrorStatus_SeeOther = 303,
    WebErrorStatus_NotModified = 304,
    WebErrorStatus_UseProxy = 305,
    WebErrorStatus_TemporaryRedirect = 307,
    WebErrorStatus_BadRequest = 400,
    WebErrorStatus_Unauthorized = 401,
    WebErrorStatus_PaymentRequired = 402,
    WebErrorStatus_Forbidden = 403,
    WebErrorStatus_NotFound = 404,
    WebErrorStatus_MethodNotAllowed = 405,
    WebErrorStatus_NotAcceptable = 406,
    WebErrorStatus_ProxyAuthenticationRequired = 407,
    WebErrorStatus_RequestTimeout = 408,
    WebErrorStatus_Conflict = 409,
    WebErrorStatus_Gone = 410,
    WebErrorStatus_LengthRequired = 411,
    WebErrorStatus_PreconditionFailed = 412,
    WebErrorStatus_RequestEntityTooLarge = 413,
    WebErrorStatus_RequestUriTooLong = 414,
    WebErrorStatus_UnsupportedMediaType = 415,
    WebErrorStatus_RequestedRangeNotSatisfiable = 416,
    WebErrorStatus_ExpectationFailed = 417,
    WebErrorStatus_InternalServerError = 500,
    WebErrorStatus_NotImplemented = 501,
    WebErrorStatus_BadGateway = 502,
    WebErrorStatus_ServiceUnavailable = 503,
    WebErrorStatus_GatewayTimeout = 504,
    WebErrorStatus_HttpVersionNotSupported = 505
};

HRESULT
ErrorUtils::ConvertHttpStatusCodeToHR(
    _In_ uint32 httpStatusCode
    )
{
    HRESULT hr = HTTP_E_STATUS_UNEXPECTED;
    // 2xx are http success codes
    if ((httpStatusCode >= 200) && (httpStatusCode < 300))
    {
        hr = S_OK;
    }
    // MSXML XHR bug: get_status() returns HTTP/1223 for HTTP/204:
    // http://blogs.msdn.com/b/ieinternals/archive/2009/07/23/the-ie8-native-xmlhttprequest-object.aspx
    // treat it as success code as well
    else if ( httpStatusCode == 1223 )
    {
        hr = S_OK;
    }
    else
    {
        switch(httpStatusCode)
        {
        //
        // 101 server has switched protocols in upgrade header. Not suppose to happen in product.
        //
        case 101:
            hr = INET_E_UNKNOWN_PROTOCOL;
            break;

        //
        // 300 Multiple Choices
        //
        case WebErrorStatus::WebErrorStatus_MultipleChoices:
            hr = HTTP_E_STATUS_AMBIGUOUS;
            break;

        //
        // 301 Moved Permanently
        //
        case WebErrorStatus::WebErrorStatus_MovedPermanently:
            hr = HTTP_E_STATUS_MOVED;
            break;

        //
        // 302 Found
        //
        case WebErrorStatus::WebErrorStatus_Found:
            hr = HTTP_E_STATUS_REDIRECT;
            break;

        //
        // 303 See Other
        //
        case WebErrorStatus::WebErrorStatus_SeeOther:
            hr = HTTP_E_STATUS_REDIRECT_METHOD;
            break;

        //
        // 304 Not Modified
        //
        case WebErrorStatus::WebErrorStatus_NotModified:
            hr = HTTP_E_STATUS_NOT_MODIFIED;
            break;

        //
        // 305 Use Proxy
        //
        case WebErrorStatus::WebErrorStatus_UseProxy:
            hr = HTTP_E_STATUS_USE_PROXY;
            break;

        //
        // 307 Temporary Redirect
        //
        case WebErrorStatus::WebErrorStatus_TemporaryRedirect:
            hr = HTTP_E_STATUS_REDIRECT_KEEP_VERB;
            break;

        //
        // 400 Bad Request
        //
        case WebErrorStatus::WebErrorStatus_BadRequest:
            hr = HTTP_E_STATUS_BAD_REQUEST;
            break;

        //
        // 401 Unauthorized
        //
        case WebErrorStatus::WebErrorStatus_Unauthorized:
            hr = HTTP_E_STATUS_DENIED;
            break;

        //
        // 402 Payment Required
        //
        case WebErrorStatus::WebErrorStatus_PaymentRequired:
            hr = HTTP_E_STATUS_PAYMENT_REQ;
            break;

        //
        // 403 Forbidden.
        //
        case WebErrorStatus::WebErrorStatus_Forbidden:
            hr = HTTP_E_STATUS_FORBIDDEN;
            break;

        //
        // 404 Not Found.
        //
        case WebErrorStatus::WebErrorStatus_NotFound:
            hr = HTTP_E_STATUS_NOT_FOUND;
            break;

        //
        // 405 Method Not Allowed
        //
        case WebErrorStatus::WebErrorStatus_MethodNotAllowed:
            hr = HTTP_E_STATUS_BAD_METHOD;
            break;

        //
        // 406 Not Acceptable
        //
        case WebErrorStatus::WebErrorStatus_NotAcceptable:
            hr = HTTP_E_STATUS_NONE_ACCEPTABLE;
            break;
        //
        // 407 Proxy Authentication Required
        //
        case WebErrorStatus::WebErrorStatus_ProxyAuthenticationRequired:
            hr = HTTP_E_STATUS_PROXY_AUTH_REQ;
            break;
        //
        // 408 Request Timeout
        //
        case WebErrorStatus::WebErrorStatus_RequestTimeout:
            hr = HTTP_E_STATUS_REQUEST_TIMEOUT;
            break;

        //
        // 409 Conflict
        //
        case WebErrorStatus::WebErrorStatus_Conflict:
            hr = HTTP_E_STATUS_CONFLICT;
            break;

        //
        // 410 Gone
        //
        case WebErrorStatus::WebErrorStatus_Gone:
            hr = HTTP_E_STATUS_GONE;
            break;

        //
        // 411 Length Required
        //
        case WebErrorStatus::WebErrorStatus_LengthRequired:
            hr = HTTP_E_STATUS_LENGTH_REQUIRED;
            break;

        //
        // 412 Precondition Failed
        //
        case WebErrorStatus::WebErrorStatus_PreconditionFailed:
            hr = HTTP_E_STATUS_PRECOND_FAILED;
            break;

        //
        // 413 Request Entity Too Large
        //
        case WebErrorStatus::WebErrorStatus_RequestEntityTooLarge:
            hr = HTTP_E_STATUS_REQUEST_TOO_LARGE;
            break;

        //
        // 414 Request URI Too Long
        //
        case WebErrorStatus::WebErrorStatus_RequestUriTooLong:
            hr = HTTP_E_STATUS_URI_TOO_LONG;
            break;

        //
        // 415 Unsupported Media Type
        //
        case WebErrorStatus::WebErrorStatus_UnsupportedMediaType:
            hr = HTTP_E_STATUS_UNSUPPORTED_MEDIA;
            break;

        //
        // 416 Requested Range Not Satisfiable
        //
        case WebErrorStatus::WebErrorStatus_RequestedRangeNotSatisfiable:
            hr = HTTP_E_STATUS_RANGE_NOT_SATISFIABLE;
            break;

        //
        // 417 Expectation Failed
        //
        case WebErrorStatus::WebErrorStatus_ExpectationFailed:
            hr = HTTP_E_STATUS_EXPECTATION_FAILED;
            break;

        //
        // 449 Retry after doing the appropriate action.
        //
        case 449:
            hr = HTTP_E_STATUS_BAD_REQUEST;
            break;

        //
        // 500 Internal Server Error
        //
        case WebErrorStatus::WebErrorStatus_InternalServerError:
            hr = HTTP_E_STATUS_SERVER_ERROR;
            break;

        //
        // 501 Not Implemented
        //
        case WebErrorStatus::WebErrorStatus_NotImplemented:
            hr = HTTP_E_STATUS_NOT_SUPPORTED;
            break;

        //
        // 502 Bad Gateway
        //
        case WebErrorStatus::WebErrorStatus_BadGateway:
            hr = HTTP_E_STATUS_BAD_GATEWAY;
            break;

        //
        // 503 Service Unavailable
        //
        case WebErrorStatus::WebErrorStatus_ServiceUnavailable:
            hr = HTTP_E_STATUS_SERVICE_UNAVAIL;
            break;

        //
        // 504 Gateway Timeout
        //
        case WebErrorStatus::WebErrorStatus_GatewayTimeout:
            hr = HTTP_E_STATUS_GATEWAY_TIMEOUT;
            break;

        //
        // 505 HTTP Version Not Supported.
        //
        case WebErrorStatus::WebErrorStatus_HttpVersionNotSupported:
            hr = HTTP_E_STATUS_VERSION_NOT_SUP;
            break;

        default:
            hr = HTTP_E_STATUS_UNEXPECTED;
            break;
        }
    }

    return hr;
}

HRESULT
ErrorUtils::ConvertExceptionToHRESULT()
{
    // Default value, if there is no exception appears, return S_OK
    HRESULT hr = S_OK;

    try
    {
        throw;
    }

    // std exceptions
    catch( const std::bad_alloc& ) // is an exception
    {
        hr = E_OUTOFMEMORY;
    }
    catch( const std::invalid_argument& ) // is a logic_error
    {
        hr = E_INVALIDARG;
    }
    catch( const std::domain_error& ) // is a logic_error
    {
        hr = E_INVALIDARG;
    }
    catch( const std::out_of_range& ) // is a logic_error
    {
        hr = E_INVALIDARG;
    }
    catch( const std::length_error& ) // is a logic_error
    {
        hr = __HRESULT_FROM_WIN32( ERROR_ARITHMETIC_OVERFLOW );
    }
    catch( const std::overflow_error& ) // is a runtime_error
    {
        hr = __HRESULT_FROM_WIN32( ERROR_ARITHMETIC_OVERFLOW );
    }
    catch( const std::underflow_error& ) // is a runtime_error
    {
        hr = RPC_S_FP_UNDERFLOW;
    }
    catch( const std::range_error& ) // is a runtime_error
    {
        hr = E_INVALIDARG;
    }
    catch ( const std::logic_error& ) // is an exception
    {
        hr = E_UNEXPECTED;
    }
    catch ( const std::runtime_error& ) // is an exception
    {
        hr = E_FAIL;
    }
    catch ( const std::exception& ) // base class for standard C++ exceptions
    {
        hr = E_FAIL;
    }
    catch ( Platform::Exception^ exception )
    {
        hr = (HRESULT)exception->HResult;
    }
    catch ( HRESULT exceptionHR )
    {
        hr = exceptionHR;
    }
    catch (...) // everything else
    {
        hr = E_FAIL;
    }

    return hr;
}

Platform::String^ ErrorUtils::ConvertHResultToString( HRESULT hr )
{
    WCHAR tmp[ 1024 ];
    _snwprintf_s( tmp, _countof( tmp ), _TRUNCATE, L"0x%0.8x", hr);
    return ref new Platform::String(tmp);
}

Platform::String^ ErrorUtils::ConvertHResultToErrorName( HRESULT hr )
{
    switch( hr )
    {
        // Generic errors
        case S_OK: return L"S_OK";
        case S_FALSE: return L"S_FALSE";
        case E_OUTOFMEMORY: return L"E_OUTOFMEMORY";
        case E_ACCESSDENIED: return L"E_ACCESSDENIED";
        case E_INVALIDARG: return L"E_INVALIDARG";
        case E_UNEXPECTED: return L"E_UNEXPECTED";
        case E_ABORT: return L"E_ABORT";
        case E_FAIL: return L"E_FAIL";
        case E_NOTIMPL: return L"E_NOTIMPL";
        case E_ILLEGAL_METHOD_CALL: return L"E_ILLEGAL_METHOD_CALL";

        // Authentication specific errors
        case 0x87DD0003: return L"AM_E_XASD_UNEXPECTED";
        case 0x87DD0004: return L"AM_E_XASU_UNEXPECTED";
        case 0x87DD0005: return L"AM_E_XAST_UNEXPECTED";
        case 0x87DD0006: return L"AM_E_XSTS_UNEXPECTED";
        case 0x87DD0007: return L"AM_E_XDEVICE_UNEXPECTED";
        case 0x87DD0008: return L"AM_E_DEVMODE_NOT_AUTHORIZED";
        case 0x87DD0009: return L"AM_E_NOT_AUTHORIZED";
        case 0x87DD000A: return L"AM_E_FORBIDDEN";
        case 0x87DD000B: return L"AM_E_UNKNOWN_TARGET";
        case 0x87DD000C: return L"AM_E_NSAL_READ_FAILED";
        case 0x87DD000D: return L"AM_E_TITLE_NOT_AUTHENTICATED";
        case 0x87DD000E: return L"AM_E_TITLE_NOT_AUTHORIZED";
        case 0x87DD000F: return L"AM_E_DEVICE_NOT_AUTHENTICATED";
        case 0x87DD0010: return L"AM_E_INVALID_USER_INDEX";
        case 0x8015DC00: return L"XO_E_DEVMODE_NOT_AUTHORIZED";
        case 0x8015DC01: return L"XO_E_SYSTEM_UPDATE_REQUIRED";
        case 0x8015DC02: return L"XO_E_CONTENT_UPDATE_REQUIRED";
        case 0x8015DC03: return L"XO_E_ENFORCEMENT_BAN";
        case 0x8015DC04: return L"XO_E_THIRD_PARTY_BAN";
        case 0x8015DC05: return L"XO_E_ACCOUNT_PARENTALLY_RESTRICTED";
        case 0x8015DC06: return L"XO_E_DEVICE_SUBSCRIPTION_NOT_ACTIVATED";
        case 0x8015DC08: return L"XO_E_ACCOUNT_BILLING_MAINTENANCE_REQUIRED";
        case 0x8015DC09: return L"XO_E_ACCOUNT_CREATION_REQUIRED";
        case 0x8015DC0A: return L"XO_E_ACCOUNT_TERMS_OF_USE_NOT_ACCEPTED";
        case 0x8015DC0B: return L"XO_E_ACCOUNT_COUNTRY_NOT_AUTHORIZED";
        case 0x8015DC0C: return L"XO_E_ACCOUNT_AGE_VERIFICATION_REQUIRED";
        case 0x8015DC0D: return L"XO_E_ACCOUNT_CURFEW";
        case 0x8015DC0E: return L"XO_E_ACCOUNT_ZEST_MAINTENANCE_REQUIRED";
        case 0x8015DC0F: return L"XO_E_ACCOUNT_CSV_TRANSITION_REQUIRED";
        case 0x8015DC10: return L"XO_E_ACCOUNT_MAINTENANCE_REQUIRED";
        case 0x8015DC11: return L"XO_E_ACCOUNT_TYPE_NOT_ALLOWED";
        case 0x8015DC12: return L"XO_E_CONTENT_ISOLATION (Verify SCID / Sandbox)";
        case 0x8015DC13: return L"XO_E_ACCOUNT_NAME_CHANGE_REQUIRED";
        case 0x8015DC14: return L"XO_E_DEVICE_CHALLENGE_REQUIRED";
        case 0x8015DC20: return L"XO_E_EXPIRED_DEVICE_TOKEN";
        case 0x8015DC21: return L"XO_E_EXPIRED_TITLE_TOKEN";
        case 0x8015DC22: return L"XO_E_EXPIRED_USER_TOKEN";
        case 0x8015DC23: return L"XO_E_INVALID_DEVICE_TOKEN";
        case 0x8015DC24: return L"XO_E_INVALID_TITLE_TOKEN";
        case 0x8015DC25: return L"XO_E_INVALID_USER_TOKEN";

        // HTTP specific errors
        case WEB_E_UNSUPPORTED_FORMAT: return L"WEB_E_UNSUPPORTED_FORMAT";
        case WEB_E_INVALID_XML: return L"WEB_E_INVALID_XML";
        case WEB_E_MISSING_REQUIRED_ELEMENT: return L"WEB_E_MISSING_REQUIRED_ELEMENT";
        case WEB_E_MISSING_REQUIRED_ATTRIBUTE: return L"WEB_E_MISSING_REQUIRED_ATTRIBUTE";
        case WEB_E_UNEXPECTED_CONTENT: return L"WEB_E_UNEXPECTED_CONTENT";
        case WEB_E_RESOURCE_TOO_LARGE: return L"WEB_E_RESOURCE_TOO_LARGE";
        case WEB_E_INVALID_JSON_STRING: return L"WEB_E_INVALID_JSON_STRING";
        case WEB_E_INVALID_JSON_NUMBER: return L"WEB_E_INVALID_JSON_NUMBER";
        case WEB_E_JSON_VALUE_NOT_FOUND: return L"WEB_E_JSON_VALUE_NOT_FOUND";
        case HTTP_E_STATUS_UNEXPECTED: return L"HTTP_E_STATUS_UNEXPECTED";
        case HTTP_E_STATUS_UNEXPECTED_REDIRECTION: return L"HTTP_E_STATUS_UNEXPECTED_REDIRECTION";
        case HTTP_E_STATUS_UNEXPECTED_CLIENT_ERROR: return L"HTTP_E_STATUS_UNEXPECTED_CLIENT_ERROR";
        case HTTP_E_STATUS_UNEXPECTED_SERVER_ERROR: return L"HTTP_E_STATUS_UNEXPECTED_SERVER_ERROR";
        case HTTP_E_STATUS_AMBIGUOUS: return L"HTTP_E_STATUS_AMBIGUOUS";
        case HTTP_E_STATUS_MOVED: return L"HTTP_E_STATUS_MOVED";
        case HTTP_E_STATUS_REDIRECT: return L"HTTP_E_STATUS_REDIRECT";
        case HTTP_E_STATUS_REDIRECT_METHOD: return L"HTTP_E_STATUS_REDIRECT_METHOD";
        case HTTP_E_STATUS_NOT_MODIFIED: return L"HTTP_E_STATUS_NOT_MODIFIED";
        case HTTP_E_STATUS_USE_PROXY: return L"HTTP_E_STATUS_USE_PROXY";
        case HTTP_E_STATUS_REDIRECT_KEEP_VERB: return L"HTTP_E_STATUS_REDIRECT_KEEP_VERB";
        case HTTP_E_STATUS_BAD_REQUEST: return L"HTTP_E_STATUS_BAD_REQUEST";
        case HTTP_E_STATUS_DENIED: return L"HTTP_E_STATUS_DENIED";
        case HTTP_E_STATUS_PAYMENT_REQ: return L"HTTP_E_STATUS_PAYMENT_REQ";
        case HTTP_E_STATUS_FORBIDDEN: return L"HTTP_E_STATUS_FORBIDDEN";
        case HTTP_E_STATUS_NOT_FOUND: return L"HTTP_E_STATUS_NOT_FOUND";
        case HTTP_E_STATUS_BAD_METHOD: return L"HTTP_E_STATUS_BAD_METHOD";
        case HTTP_E_STATUS_NONE_ACCEPTABLE: return L"HTTP_E_STATUS_NONE_ACCEPTABLE";
        case HTTP_E_STATUS_PROXY_AUTH_REQ: return L"HTTP_E_STATUS_PROXY_AUTH_REQ";
        case HTTP_E_STATUS_REQUEST_TIMEOUT: return L"HTTP_E_STATUS_REQUEST_TIMEOUT";
        case HTTP_E_STATUS_CONFLICT: return L"HTTP_E_STATUS_CONFLICT";
        case HTTP_E_STATUS_GONE: return L"HTTP_E_STATUS_GONE";
        case HTTP_E_STATUS_LENGTH_REQUIRED: return L"HTTP_E_STATUS_LENGTH_REQUIRED";
        case HTTP_E_STATUS_PRECOND_FAILED: return L"HTTP_E_STATUS_PRECOND_FAILED";
        case HTTP_E_STATUS_REQUEST_TOO_LARGE: return L"HTTP_E_STATUS_REQUEST_TOO_LARGE";
        case HTTP_E_STATUS_URI_TOO_LONG: return L"HTTP_E_STATUS_URI_TOO_LONG";
        case HTTP_E_STATUS_UNSUPPORTED_MEDIA: return L"HTTP_E_STATUS_UNSUPPORTED_MEDIA";
        case HTTP_E_STATUS_RANGE_NOT_SATISFIABLE: return L"HTTP_E_STATUS_RANGE_NOT_SATISFIABLE";
        case HTTP_E_STATUS_EXPECTATION_FAILED: return L"HTTP_E_STATUS_EXPECTATION_FAILED";
        case HTTP_E_STATUS_SERVER_ERROR: return L"HTTP_E_STATUS_SERVER_ERROR";
        case HTTP_E_STATUS_NOT_SUPPORTED: return L"HTTP_E_STATUS_NOT_SUPPORTED";
        case HTTP_E_STATUS_BAD_GATEWAY: return L"HTTP_E_STATUS_BAD_GATEWAY";
        case HTTP_E_STATUS_SERVICE_UNAVAIL: return L"HTTP_E_STATUS_SERVICE_UNAVAIL";
        case HTTP_E_STATUS_GATEWAY_TIMEOUT: return L"HTTP_E_STATUS_GATEWAY_TIMEOUT";
        case HTTP_E_STATUS_VERSION_NOT_SUP: return L"HTTP_E_STATUS_VERSION_NOT_SUP";

        // WinINet specific errors
        case INET_E_INVALID_URL: return L"INET_E_INVALID_URL";
        case INET_E_NO_SESSION: return L"INET_E_NO_SESSION";
        case INET_E_CANNOT_CONNECT: return L"INET_E_CANNOT_CONNECT";
        case INET_E_RESOURCE_NOT_FOUND: return L"INET_E_RESOURCE_NOT_FOUND";
        case INET_E_OBJECT_NOT_FOUND: return L"INET_E_OBJECT_NOT_FOUND";
        case INET_E_DATA_NOT_AVAILABLE: return L"INET_E_DATA_NOT_AVAILABLE";
        case INET_E_DOWNLOAD_FAILURE: return L"INET_E_DOWNLOAD_FAILURE";
        case INET_E_AUTHENTICATION_REQUIRED: return L"INET_E_AUTHENTICATION_REQUIRED";
        case INET_E_NO_VALID_MEDIA: return L"INET_E_NO_VALID_MEDIA";
        case INET_E_CONNECTION_TIMEOUT: return L"INET_E_CONNECTION_TIMEOUT";
        case INET_E_INVALID_REQUEST: return L"INET_E_INVALID_REQUEST";
        case INET_E_UNKNOWN_PROTOCOL: return L"INET_E_UNKNOWN_PROTOCOL";
        case INET_E_SECURITY_PROBLEM: return L"INET_E_SECURITY_PROBLEM";
        case INET_E_CANNOT_LOAD_DATA: return L"INET_E_CANNOT_LOAD_DATA";
        case INET_E_CANNOT_INSTANTIATE_OBJECT: return L"INET_E_CANNOT_INSTANTIATE_OBJECT";
        case INET_E_INVALID_CERTIFICATE: return L"INET_E_INVALID_CERTIFICATE";
        case INET_E_REDIRECT_FAILED: return L"INET_E_REDIRECT_FAILED";
        case INET_E_REDIRECT_TO_DIR: return L"INET_E_REDIRECT_TO_DIR";
    }

    return L"Unknown error";
}

}}}

#endif