#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <collection.h>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <wrl.h>
#include <robuffer.h>
#include <ppl.h>

#include "p2p.h"
#include "rbx/threadsafe.h"
#include "VoiceChatBase.h"

using namespace Microsoft::Xbox::Services;

namespace RBX
{
    struct VoicePacket : public Xp2p::PacketHeader
    {
        unsigned int getDataSize()
        {
            return size - sizeof(Xp2p::PacketHeader);
        }

        unsigned char data[Xp2p::PacketStorage::MaxPayloadSize];
    };

    struct ControlVoicePacket : public Xp2p::PacketHeader
    {
        uint8 subscribe;
    };

    class VoiceChatMaxNet : public boost::enable_shared_from_this<VoiceChatMaxNet>, public VoiceChatBase
    {
    private: 
        VoiceChatMaxNet(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ currentUser, XboxLiveContext^ xboxLiveContext);

    public:

        static boost::shared_ptr<VoiceChatBase> create(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ currentUser, XboxLiveContext^ xboxLiveContext)
        {
            boost::shared_ptr<VoiceChatMaxNet> sp(new VoiceChatMaxNet(currentUser, xboxLiveContext));
            sp->init();
            return sp;
        }

        virtual ~VoiceChatMaxNet();

        typedef Platform::Box<unsigned int> ConsoleId;
        typedef Platform::Box<bool> TalkingState;

        // local user 
                void headsetConnected(Windows::Xbox::System::User^ user);
        virtual void addLocalUserToChannel(uint8 channelIndex, Windows::Xbox::System::User^ user);
        virtual void removeLocalUserFromChannel();

        // remote users
        virtual void addConnections(Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^>^ members);
        virtual void removeConnections(Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^>^ members);

        // remote users
        virtual void multiplayerUserAdded(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member);
        virtual void multiplayerUserRemoved(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member);

        // receive/send
        void onOutgoingChatPacketReady(Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args);
        void onDebugMessageReceived(Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args);

        // muting
        virtual void setMuteState(bool muted, Microsoft::Xbox::GameChat::ChatUser^ chatUser);
        virtual unsigned getMuteState(unsigned memberId);
        virtual void muteAll();
        virtual void unmuteAll();
        virtual void muteUser(unsigned memberId);
        virtual void unmuteUser(unsigned memberId);

        virtual void updateMember(unsigned memberId, float distance, TalkingDelta* talkingDeltaOut);

        void receive();
        void sendVoiceControlPacket(Xp2p::PeerPtr peer, bool subscribe);
        void unsubscribeToChatMessages_NoLock();
        void subscribeToChatMessages_NoLock();
        void handleNewRemoteConnection(ConsoleId^ consoleId);

    private:

        enum PacketType
        {
            PacketType_VoicePacket = 1,
            PacketType_ControlPacket = 2
        };

        void init();
        void removeLocalUserFromChannel_NoLock();
        Microsoft::Xbox::GameChat::ChatUser^ getUserByMemberId(unsigned id);

        Microsoft::Xbox::GameChat::ChatManager^ chatManager;

        int currentChannel;
        Windows::Xbox::System::User^ currentUser;

        void sendData(Xp2p::PeerPtr peer, Windows::Storage::Streams::IBuffer^ buffer, unsigned int flags);

        boost::scoped_ptr<Xp2p::Network> network;
        XboxLiveContext^ xbLiveContext;

        RBX::mutex channelsLock;
        RBX::mutex peersLock;
        RBX::mutex subscribedPeersLock;
        boost::unordered_map<unsigned int, Xp2p::PeerPtr> peers;
        boost::unordered_set<unsigned int> subscibedPeers;

        Windows::Foundation::EventRegistrationToken onDebugMessageEvtToken;
        Windows::Foundation::EventRegistrationToken onOutgoingChatPacketReadyEvtToken;
        Windows::Foundation::EventRegistrationToken onCompareUniqueConsoleIdentifiersEvtToken;
        Windows::Foundation::EventRegistrationToken onResourceAvailabilityChangedEvtToken;
    
        Windows::Foundation::EventRegistrationToken onAudioAdded;
        Windows::Foundation::EventRegistrationToken onAudioRemoved;
    };
}