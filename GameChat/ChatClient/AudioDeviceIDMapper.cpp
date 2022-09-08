//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "AudioDeviceIDMapper.h"
#include "StringUtils.h"

#if TV_API

namespace Microsoft {
namespace Xbox {
namespace GameChat {

AudioDeviceIDMapper::AudioDeviceIDMapper()
{
}

void AudioDeviceIDMapper::Reset()
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_stringToLocalDeviceID.clear();
}


DEVICE_ID AudioDeviceIDMapper::GetLocalDeviceID(
    _In_ Platform::String^ stringID
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    // Only add if not already there
    if (!StringDeviceIDExists(stringID))
    {
        m_nextLocalDeviceID = (DEVICE_ID)m_nextLocalDeviceIDLong;
        m_stringToLocalDeviceID[stringID] = m_nextLocalDeviceID;
        InterlockedIncrement(&m_nextLocalDeviceIDLong);

        return m_nextLocalDeviceID;
    }
    else
    {
        return m_stringToLocalDeviceID[stringID];
    }
}

bool AudioDeviceIDMapper::StringDeviceIDExists(
    _In_ Platform::String^ stringID
    )
{
    return m_stringToLocalDeviceID.find(stringID) != m_stringToLocalDeviceID.end();
}

bool AudioDeviceIDMapper::StringRemoteIDExists(
    _In_ Platform::String^ stringID
    )
{
    return m_stringToRemoteDeviceID.find(stringID) != m_stringToRemoteDeviceID.end();
}

bool AudioDeviceIDMapper::DoesLookupIDExistsForRemote(
    _In_ LOOKUP_ID lookupId
    )
{
    return m_remoteIdToString.find(lookupId) != m_remoteIdToString.end();
}


LOOKUP_ID AudioDeviceIDMapper::ConstructLookupID(
    _In_ CONSOLE_NAME localNameOfRemoteConsole, 
    _In_ DEVICE_ID deviceID
    )
{
    return (localNameOfRemoteConsole << 8) + deviceID;
}

void AudioDeviceIDMapper::RemoveRemoteConsole( 
    _In_ CONSOLE_NAME localNameOfRemoteConsoleToRemove
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    
#ifdef DEBUG_REMOTE_AUDIO_DEVICES
    OutputDebugString( L"AudioDeviceIDMapper::RemoveRemoteConsole before\r\n" );
    DebugDumpRemoteIdToStringMap();
#endif

    for(auto iter = m_remoteIdToString.cbegin(); iter != m_remoteIdToString.cend(); )
    {
        LOOKUP_ID lookupId = iter->first;

        CONSOLE_NAME localNameOfRemoteConsole = lookupId >> 8;
        //DEVICE_ID deviceID = lookupId & 0xFF;

        if( localNameOfRemoteConsoleToRemove == localNameOfRemoteConsole )
        {
            m_remoteIdToString.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }

#ifdef DEBUG_REMOTE_AUDIO_DEVICES
    OutputDebugString( L"AudioDeviceIDMapper::RemoveRemoteConsole after\r\n" );
    DebugDumpRemoteIdToStringMap();
#endif
}


Platform::String^ AudioDeviceIDMapper::GetRemoteAudioDevice( 
    _In_ LOOKUP_ID lookUpId
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);

    // Only add if not already there
    if (DoesLookupIDExistsForRemote(lookUpId))
    {
        return m_remoteIdToString[lookUpId];
    }

    return nullptr;
}

void AudioDeviceIDMapper::AddRemoteAudioDevice( 
    _In_ LOOKUP_ID lookUpId, 
    _In_ Platform::String^ stringID 
    )
{
    Concurrency::critical_section::scoped_lock lock(m_stateLock);
    m_stringToRemoteDeviceID[stringID] = lookUpId;
    m_remoteIdToString[lookUpId] = stringID;

#ifdef DEBUG_REMOTE_AUDIO_DEVICES
    OutputDebugString( L"AudioDeviceIDMapper::AddRemoteAudioDevice\r\n" );
    DebugDumpRemoteIdToStringMap();
#endif
}

void AudioDeviceIDMapper::DebugDumpRemoteIdToStringMap()
{
#ifdef DEBUG_REMOTE_AUDIO_DEVICES
    int iterIndex = 0;
    for(std::map<LOOKUP_ID, Platform::String^>::iterator iter = m_remoteIdToString.begin(); iter != m_remoteIdToString.end(); ++iter)
    {
        LOOKUP_ID iterLookupId = iter->first;
        Platform::String^ audioStringId = iter->second;
        WCHAR text[1024];
        swprintf_s(text, ARRAYSIZE(text), L"m_remoteIdToString[%d] = %d -> %s\r\n",
            iterIndex,
            iterLookupId,
            audioStringId->Data() 
            );  
        OutputDebugString( text );

        iterIndex++;
    }
#endif
}

}}}

#endif