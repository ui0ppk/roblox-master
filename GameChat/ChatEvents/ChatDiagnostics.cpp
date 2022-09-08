//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "StringUtils.h"
#include "ChatDiagnostics.h"
#include "BufferUtils.h"
#include "ChatAudioThread.h"
#include "ChatNetwork.h"
#include "ChatManager.h"

#if TV_API

using namespace Windows::Foundation;
using namespace concurrency;

namespace Microsoft {
namespace Xbox {
namespace GameChat {

class ChatDiagnosticsRegistration
{
public:
    ChatDiagnosticsRegistration()
    {
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        EventRegisterXbox_GameChat_API();
#endif
    }

    ~ChatDiagnosticsRegistration()
    {
#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
        EventUnregisterXbox_GameChat_API();
#endif
    }
};

static ChatDiagnosticsRegistration s_chatDiagnosticsRegistration;

ChatDiagnostics::ChatDiagnostics() :
    m_diagnosticConsoleNameTracker(1)
{
}

uint32 ChatDiagnostics::GetDiagnosticNameForConsole( 
    _In_ ChatManager^ chatManager,
    _In_ Platform::Object^ uniqueConsoleIdentifier
    )
{
    if( uniqueConsoleIdentifier == nullptr )
    {
        return 0;
    }

    Concurrency::critical_section::scoped_lock lock(m_diagnosticNameOfConsolesLock);
    for each (std::shared_ptr<ChatEventConsoleNameIdentifierPair> pair in m_diagnosticNameOfConsoles)
    {
        if( chatManager->OnCompareUniqueConsoleIdentifiersHandler(pair->uniqueConsoleIdentifier, uniqueConsoleIdentifier) )
        {
            return pair->consoleName;
        }
    }

    // Record the name for next time
    std::shared_ptr<ChatEventConsoleNameIdentifierPair> pair( new ChatEventConsoleNameIdentifierPair() );
    pair->consoleName = InterlockedIncrement(&m_diagnosticConsoleNameTracker);
    pair->uniqueConsoleIdentifier = uniqueConsoleIdentifier;
    m_diagnosticNameOfConsoles.push_back( pair );

    return pair->consoleName;
}

void ChatDiagnostics::TraceChatUserAndAudioDevices( 
    _In_ Windows::Xbox::System::IUser^ user
    )
{
    if( user == nullptr )
    {
        return;
    }

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE
    TraceChatUserInfo(
        user->XboxUserId->Data(),
        user->Id,
        user->IsGuest,
        user->IsSignedIn,
        user->Sponsor != nullptr, // HasSponsor
        user->Controllers != nullptr ? user->Controllers->Size : 0, // NumControllers
        user->AudioDevices->Size // NumAudioDevices
        );

    for each (Windows::Xbox::System::IAudioDeviceInfo^ audioDeviceInfo in user->AudioDevices)
    {
        TraceChatUserAudioDevice(
            user->XboxUserId->Data(),
            audioDeviceInfo->Id->Data(),
            ConvertAudioDeviceCategoryToString( audioDeviceInfo->DeviceCategory )->Data(),
            ConvertAudioDeviceTypeToString( audioDeviceInfo->DeviceType )->Data(),
            audioDeviceInfo->IsMicrophoneMuted,
            ConvertAudioDeviceSharingToString( audioDeviceInfo->Sharing )->Data()
            );
    }
#endif
}

Platform::String^ ChatDiagnostics::ConvertAudioDeviceTypeToString(
    _In_ Windows::Xbox::System::AudioDeviceType audioDeviceType 
    )
{
    switch (audioDeviceType)
    {
        case Windows::Xbox::System::AudioDeviceType::Capture: return L"Capture";
        case Windows::Xbox::System::AudioDeviceType::Render: return L"Render";
    }

    return L"Unknown";
}

Platform::String^ ChatDiagnostics::ConvertAudioDeviceCategoryToString(
    _In_ Windows::Xbox::System::AudioDeviceCategory audioDeviceCategory
    )
{
    switch (audioDeviceCategory)
    {
        case Windows::Xbox::System::AudioDeviceCategory::Communications: return L"Communications";
        case Windows::Xbox::System::AudioDeviceCategory::Multimedia: return L"Multimedia";
        case Windows::Xbox::System::AudioDeviceCategory::Voice: return L"Voice";
    }

    return L"Unknown";
}

Platform::String^ ChatDiagnostics::ConvertAudioDeviceSharingToString(
    _In_ Windows::Xbox::System::AudioDeviceSharing audioDeviceSharing
    )
{
    switch (audioDeviceSharing)
    {
        case Windows::Xbox::System::AudioDeviceSharing::Exclusive: return L"Exclusive";
        case Windows::Xbox::System::AudioDeviceSharing::Private: return L"Private";
        case Windows::Xbox::System::AudioDeviceSharing::Shared: return L"Shared";
    }

    return L"Unknown";
}


}}}

#endif