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

namespace Wxs = Windows::Xbox::System;

ref class RemoteAudioDeviceInfo sealed : public Wxs::IAudioDeviceInfo
{
public:
    RemoteAudioDeviceInfo(
        _In_ Windows::Xbox::System::AudioDeviceCategory deviceCategory,
        _In_ Windows::Xbox::System::AudioDeviceType deviceType,
        _In_ Platform::String^ id,
        _In_ bool isMicrophoneMuted,
        _In_ Windows::Xbox::System::AudioDeviceSharing sharing
        );  

    virtual property Windows::Xbox::System::AudioDeviceCategory DeviceCategory
    {
        Windows::Xbox::System::AudioDeviceCategory get()
        {
            return m_deviceCategory;
        }
    }

    virtual property Windows::Xbox::System::AudioDeviceType DeviceType
    {
        Windows::Xbox::System::AudioDeviceType get()
        {
            return m_deviceType;
        }
    }

    virtual property Platform::String^ Id
    {
        Platform::String^ get()
        {
            return m_id;
        }
    }

    virtual property bool IsMicrophoneMuted
    {
        bool get()
        {
            return m_isMicrophoneMuted;
        }
    }

    virtual property Windows::Xbox::System::AudioDeviceSharing Sharing
    {
        Windows::Xbox::System::AudioDeviceSharing get()
        {
            return m_sharing;
        }
    }

private:
    Windows::Xbox::System::AudioDeviceCategory m_deviceCategory;
    Windows::Xbox::System::AudioDeviceType m_deviceType;
    Platform::String^ m_id;
    bool m_isMicrophoneMuted;
    Windows::Xbox::System::AudioDeviceSharing m_sharing;
};


}}}

#endif