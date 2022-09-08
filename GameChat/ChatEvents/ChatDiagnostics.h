#pragma once

#if WINAPI_FAMILY == WINAPI_FAMILY_TV_TITLE

typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
#define EVENT_CONTROL_CODE_DISABLE_PROVIDER 0
#define EVENT_CONTROL_CODE_ENABLE_PROVIDER  1
#define EVENT_CONTROL_CODE_CAPTURE_STATE    2
#include "ChatEvents.h"

#endif

// Forward declare
namespace Microsoft { namespace Xbox { namespace GameChat { ref class ChatManager; } } }

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

struct ChatEventConsoleNameIdentifierPair
{
    Platform::Object^ uniqueConsoleIdentifier;
    uint32 consoleName;
};

class ChatDiagnostics
{
public:
    ChatDiagnostics();

    uint32 GetDiagnosticNameForConsole( 
        _In_ ChatManager^ chatManager,
        _In_ Platform::Object^ uniqueConsoleIdentifier
        );

    void TraceChatUserAndAudioDevices( 
        _In_ Windows::Xbox::System::IUser^ user
        );

private:
    Platform::String^ ConvertAudioDeviceSharingToString(
        _In_ Windows::Xbox::System::AudioDeviceSharing audioDeviceSharing
        );

    Platform::String^ ConvertAudioDeviceCategoryToString(
        _In_ Windows::Xbox::System::AudioDeviceCategory audioDeviceCategory
        );

    Platform::String^ ConvertAudioDeviceTypeToString(
        _In_ Windows::Xbox::System::AudioDeviceType audioDeviceType 
        );

    std::vector< std::shared_ptr<ChatEventConsoleNameIdentifierPair> > m_diagnosticNameOfConsoles;
    Concurrency::critical_section m_diagnosticNameOfConsolesLock;
    LONG m_diagnosticConsoleNameTracker;
};

}}}

#endif
