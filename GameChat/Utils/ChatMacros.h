//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#define TEXTW(quote) _TEXTW(quote)
#define _TEXTW(quote) L##quote

#define CHAT_LOG_EXCEPTION(hr) Microsoft::Xbox::GameChat::StringUtils::GetLogExceptionDebugInfo(hr, TEXTW(__FUNCTION__), TEXTW(__FILE__), __LINE__ );
#define CHAT_LOG_INFO_MSG(msg) { if (m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Info)) { LogComment( msg ); } }
#define CHAT_LOG_ERROR_MSG(msg) { if (m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Error)) { LogComment( msg ); } }
#define CHAT_LOG_VERBOSE_MSG(msg) { if (m_chatManagerSettings->IsAtDiagnosticsTraceLevel(GameChatDiagnosticsTraceLevel::Verbose)) { LogComment( msg ); } }
#define CHAT_THROW_INVALIDARGUMENT_IF(x) if ( x ) { CHAT_LOG_EXCEPTION(E_INVALIDARG); throw ref new Platform::InvalidArgumentException(); }
#define CHAT_THROW_INVALIDARGUMENT_IF_NULL(x) if ( ( x ) == nullptr ) { CHAT_LOG_EXCEPTION(E_INVALIDARG); throw ref new Platform::InvalidArgumentException(); }
#define CHAT_THROW_E_POINTER_IF_NULL(x) if ( ( x ) == nullptr ) { CHAT_LOG_EXCEPTION(E_POINTER); throw ref new Platform::COMException(E_POINTER); }
#define CHAT_THROW_E_POINTER_IF_NULL_WITH_LOG(x,msg) if ( ( x ) == nullptr ) { CHAT_LOG_ERROR_MSG( msg ); CHAT_LOG_EXCEPTION(E_POINTER); throw ref new Platform::COMException(E_POINTER); }
#define CHAT_THROW_INVALIDARGUMENT_IF_STRING_EMPTY(x) { auto y = x; if ( y->IsEmpty() ) { CHAT_LOG_EXCEPTION(E_INVALIDARG); throw ref new Platform::InvalidArgumentException(); } }
#define CHAT_THROW_IF_HR_FAILED(hr) { HRESULT hr2 = hr; if ( FAILED( hr2 ) ) { CHAT_LOG_EXCEPTION(hr2); throw ref new Platform::COMException(hr2); } }
#define CHAT_THROW_HR(hr) { CHAT_LOG_EXCEPTION(hr); throw ref new Platform::COMException(hr); }
#define CHAT_THROW_HR_IF(x,hr) if ( x ) { CHAT_LOG_EXCEPTION(hr); throw ref new Platform::COMException(hr); }
#define CHAT_THROW_WIN32_IF(x,e) if ( x ) { HRESULT hr = __HRESULT_FROM_WIN32(e); CHAT_LOG_EXCEPTION(hr); throw ref new Platform::COMException(hr); }

#ifndef E_INVALIDARG_IF
#define E_INVALIDARG_IF(x) if ( x ) { return E_INVALIDARG; }
#endif
#ifndef E_POINTER_IF_NULL
#define E_POINTER_IF_NULL(x) if ( ( x ) == nullptr ) { return E_POINTER; }
#endif
#ifndef E_POINTER_OR_INVALIDARG_IF_STRING_EMPTY
#define E_POINTER_OR_INVALIDARG_IF_STRING_EMPTY(x) { auto y = x; E_POINTER_IF_NULL( y ); E_INVALIDARG_IF( wcslen( y ) == 0 ); }
#endif
#ifndef CHECKHR_EXIT
#define CHECKHR_EXIT(hrResult) { hr = hrResult; if ( FAILED( hr ) ) { goto exit; } }
#endif

#define TV_API (WINAPI_FAMILY == WINAPI_FAMILY_TV_APP | WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE)

#ifdef PRERELEASE
#define PUBLIC_ONLY_IN_PRERELEASE public
#define PUBLIC_ONLY_IN_PRERELEASE_INTERNAL_OTHERWISE public
#else
#define PUBLIC_ONLY_IN_PRERELEASE private
#define PUBLIC_ONLY_IN_PRERELEASE_INTERNAL_OTHERWISE internal
#endif

#ifdef INTERNAL_BUILD
#define PUBLIC_ONLY_IN_DEVKIT private
#else
#define PUBLIC_ONLY_IN_DEVKIT public
#endif
