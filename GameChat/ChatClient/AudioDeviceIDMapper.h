//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#if TV_API

// Make this so we can make the underlying data type larger or smaller if we want
#define LOOKUP_ID  UINT16  // The type of the full lookup id (CONSOLE_ID << 8 + LOOKUP_ID)
#define DEVICE_ID  UINT8   // The type of the second half of the lookup id (unique device id)
#define CONSOLE_NAME UINT8   // The type of the first half of the lookup id (unique console id)

namespace Microsoft {
namespace Xbox {
namespace GameChat {


ref class AudioDeviceIDMapper sealed
{
public:
    AudioDeviceIDMapper();

    void Reset();

    DEVICE_ID GetLocalDeviceID(
        _In_ Platform::String^ stringID
        );

    void AddRemoteAudioDevice( 
        _In_ LOOKUP_ID deviceId, 
        _In_ Platform::String^ strId 
        );

    Platform::String^ GetRemoteAudioDevice( 
        _In_ LOOKUP_ID lookupId
        );

    LOOKUP_ID ConstructLookupID(
        _In_ CONSOLE_NAME localNameOfRemoteConsole, 
        _In_ DEVICE_ID deviceID
        );

    void RemoveRemoteConsole( 
        _In_ CONSOLE_NAME localNameOfRemoteConsole 
        );

    void DebugDumpRemoteIdToStringMap();

private:
    bool StringDeviceIDExists(
        _In_ Platform::String^ stringID
        );

    bool StringRemoteIDExists(
        _In_ Platform::String^ stringID
        );

    bool DoesLookupIDExistsForRemote(
        _In_ LOOKUP_ID lookupId
        );

private:
    Concurrency::critical_section m_stateLock; 
    std::map<Platform::String^, DEVICE_ID> m_stringToLocalDeviceID;
    std::map<Platform::String^, LOOKUP_ID> m_stringToRemoteDeviceID;
    std::map<LOOKUP_ID, Platform::String^> m_remoteIdToString;
    DEVICE_ID m_nextLocalDeviceID;
    LONG m_nextLocalDeviceIDLong;
};


}}}

#endif