#pragma once

#include "v8datamodel/PlatformService.h"

#include <boost/enable_shared_from_this.hpp>
#include <collection.h>

namespace RBX
{
    class VoiceChatBase 
    {

    public:
        virtual ~VoiceChatBase(){}

        enum TalkingDelta
        {
            TalkingChange_Start,
            TalkingChange_End,
            TalkingChange_NoChange
        };

        // local user 
        virtual void addLocalUserToChannel(uint8 channelIndex, Windows::Xbox::System::User^ user) = 0;
        virtual void removeLocalUserFromChannel() = 0;

        // remote users
        virtual void addConnections(Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^>^ members) = 0;
        virtual void removeConnections(Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^>^ members) = 0;

        // remote users
        virtual void multiplayerUserAdded(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member) = 0;
        virtual void multiplayerUserRemoved(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member) = 0;

        // muting
        virtual void setMuteState(bool muted, Microsoft::Xbox::GameChat::ChatUser^ chatUser) = 0;
        virtual unsigned getMuteState(unsigned memberId) { return (unsigned)VoiceChatState::voiceChatState_Available; }
        virtual void muteAll() = 0;
        virtual void unmuteAll() = 0;

        virtual void muteUser(unsigned memberId){}
        virtual void unmuteUser(unsigned memberId){}

        virtual void updateMember(unsigned memberId, float distance, TalkingDelta* talkingDeltaOut){}
    private:
    };
}