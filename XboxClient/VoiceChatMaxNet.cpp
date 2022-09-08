#include "VoiceChatMaxNet.h"

#include "XboxUtils.h"
#include "rbx/Debug.h"
#include "async.h"

#include "g3d/g3dmath.h"

#include <map>

#if defined(RBX_PLATFORM_DURANGO)
void dprintf( const char* fmt, ... );
#endif

using namespace Microsoft::Xbox::Services::Multiplayer;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;

namespace RBX
{

VoiceChatMaxNet::VoiceChatMaxNet(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ currentUser, XboxLiveContext^ xboxLiveContext)
    : xbLiveContext(xboxLiveContext)
{
    chatManager = ref new Microsoft::Xbox::GameChat::ChatManager();
    chatManager->ChatSettings->AudioThreadPeriodInMilliseconds = 40;
    chatManager->ChatSettings->AudioThreadAffinityMask = 0xff;

    network.reset(new Xp2p::Network(currentUser));
}

VoiceChatMaxNet::~VoiceChatMaxNet()
{
    Windows::Xbox::System::User::AudioDeviceAdded -= onAudioAdded;
    Windows::Xbox::System::User::AudioDeviceRemoved -= onAudioRemoved;

    if( chatManager != nullptr )
    {
        chatManager->OnDebugMessage -= onDebugMessageEvtToken;
        chatManager->OnOutgoingChatPacketReady -= onOutgoingChatPacketReadyEvtToken;
        chatManager->OnCompareUniqueConsoleIdentifiers -= onCompareUniqueConsoleIdentifiersEvtToken;
        Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged -= onResourceAvailabilityChangedEvtToken;
        chatManager = nullptr;
    }
}

void VoiceChatMaxNet::init()
{
    boost::weak_ptr<VoiceChatMaxNet> weakPtrToThis = shared_from_this();

    onOutgoingChatPacketReadyEvtToken = chatManager->OnOutgoingChatPacketReady += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::ChatPacketEventArgs^>( 
        [weakPtrToThis] ( Platform::Object^, Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args )
    {
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            sharedPtrToThis->onOutgoingChatPacketReady(args);
        }
    });

    onDebugMessageEvtToken = chatManager->OnDebugMessage += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::DebugMessageEventArgs^>(
        [weakPtrToThis] ( Platform::Object^, Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args )
    {
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
            sharedPtrToThis->onDebugMessageReceived(args);
    });

    onCompareUniqueConsoleIdentifiersEvtToken = chatManager->OnCompareUniqueConsoleIdentifiers += ref new Microsoft::Xbox::GameChat::CompareUniqueConsoleIdentifiersHandler( 
        [weakPtrToThis] ( Platform::Object^ obj1, Platform::Object^ obj2 ) 
    { 
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            RBXASSERT(dynamic_cast<ConsoleId^>(obj1) != nullptr);
            RBXASSERT(dynamic_cast<ConsoleId^>(obj2) != nullptr);
            return (obj1->Equals(obj2));
        }
        else
            return false;
    });

    // Upon enter constrained mode, mute everyone.  
    // Upon leaving constrained mode, unmute everyone who was previously muted.
    onResourceAvailabilityChangedEvtToken = Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged += ref new Windows::Foundation::EventHandler< Platform::Object^ >( 
        [weakPtrToThis] (Platform::Object^, Platform::Object^ )
    {
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
            if (Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Constrained)
                sharedPtrToThis->muteAll();
            else if(Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Full)
                sharedPtrToThis->unmuteAll();
        
    });

    onAudioAdded = Windows::Xbox::System::User::AudioDeviceAdded += ref new Windows::Foundation::EventHandler<Windows::Xbox::System::AudioDeviceAddedEventArgs^>(
        [weakPtrToThis, this] (Platform::Object^,Windows::Xbox::System:: AudioDeviceAddedEventArgs^ args)
    {
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            if (args->AudioDevice->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && args->AudioDevice->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
            {
                RBX::mutex::scoped_lock lock(peersLock);
                sharedPtrToThis->subscribeToChatMessages_NoLock();
                sharedPtrToThis->headsetConnected(args->User);
            }
            
        }
    });

    onAudioRemoved = Windows::Xbox::System::User::AudioDeviceRemoved += ref new Windows::Foundation::EventHandler<Windows::Xbox::System::AudioDeviceRemovedEventArgs^>(
        [weakPtrToThis, this] (Platform::Object^,Windows::Xbox::System:: AudioDeviceRemovedEventArgs^ args)
    {
        boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            RBX::mutex::scoped_lock lock(peersLock);
            if (args->AudioDevice->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && args->AudioDevice->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
                sharedPtrToThis->unsubscribeToChatMessages_NoLock();
        }
    });
    
    boost::thread([weakPtrToThis]()
    {
        while (boost::shared_ptr<VoiceChatMaxNet> sharedPtrToThis = weakPtrToThis.lock() )
        {
            sharedPtrToThis->receive();
            Sleep(30);
        }
    }).detach();
}

void VoiceChatMaxNet::updateMember(unsigned memberId, float distance, TalkingDelta* talkingDeltaOut)
{
    if (chatManager)
        for (auto user : chatManager->GetChatUsers())
        {
            Xp2p::PeerPtr peer = peers[memberId];
            if (user->UniqueConsoleIdentifier && peer.get())
            {
                ConsoleId^ conId = dynamic_cast<ConsoleId^>(user->UniqueConsoleIdentifier);
                TalkingState^ currentTalkState = dynamic_cast<TalkingState^>(peer->userObject);
                RBXASSERT(conId != nullptr && currentTalkState != nullptr);

                bool isTalkingNow = user->NumberOfPendingAudioPacketsToPlay > 0;
                if (currentTalkState->Value != isTalkingNow)
                {
                    *talkingDeltaOut = isTalkingNow ? TalkingChange_Start : TalkingChange_End;
                    peer->userObject = ref new TalkingState(isTalkingNow);
                }
                else
                    *talkingDeltaOut = TalkingChange_NoChange;
                
                if (!user->IsMuted)
                {
                    if (user->GetAllChannels()->Size > 0 && user->GetAllChannels()->GetAt(0) == 0)
                    {
                        ConsoleId^ conId = dynamic_cast<ConsoleId^>(user->UniqueConsoleIdentifier);
                        if (conId != nullptr && (unsigned)conId->Value == memberId)
                        {
                            static const float fadeOutDistance = 100;     // last distance[studs] that you hear fully
                            static const float slopeInv = 160;            // sounds decreases from fadeOutDistance to (fadeOutDistance + slope) to complete zero

                            user->Volume = G3D::clamp(((fadeOutDistance - distance) / slopeInv) + 1, 0, 1);
                            break;
                        }
                    }
                    else
                        user->Volume = 1;
                }
                else
                    user->Volume = 0;
            } 
        }
}

void VoiceChatMaxNet::handleNewRemoteConnection(ConsoleId^ consoleId)
{
    chatManager->HandleNewRemoteConsole(consoleId);
}

void VoiceChatMaxNet::multiplayerUserAdded(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member)
{
    RBX::mutex::scoped_lock lock(peersLock);
    RBXASSERT(peers.find(member->MemberId) == peers.end());

    if (peers.find(member->MemberId) == peers.end())
    {
        boost::weak_ptr<VoiceChatMaxNet> weakPtrToThis = shared_from_this();
            
        if (member->Status == MultiplayerSessionMemberStatus::Active && !member->SecureDeviceAddressBase64->IsEmpty() && !member->IsCurrentUser)
        {
            Microsoft::Xbox::GameChat::ChatUser^ localUser;
            for(auto user : chatManager->GetChatUsers())
                if (user->IsLocal)
                {
                    localUser = user;
                    break;
                }

            RBXASSERT(localUser);
            if (!localUser)
                return;

            peers[member->MemberId] = network->createPeer(member, 
            [weakPtrToThis, localUser, this](Xp2p::PeerPtr peer)
            {
                peer->userObject = ref new TalkingState(false);
                boost::shared_ptr<VoiceChatMaxNet> voiceChat = weakPtrToThis.lock();
                if (voiceChat != nullptr)
                {
                    for (auto audio : localUser->User->AudioDevices)
                    {
                        if (audio->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && audio->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
                        {
                            RBX::mutex::scoped_lock lock(peersLock);
                            voiceChat->handleNewRemoteConnection(ref new ConsoleId(peer->getPeerId()));
                            voiceChat->subscribeToChatMessages_NoLock();
                            break;
                        }
                    }
                }
            });
        }
    }
}

void VoiceChatMaxNet::multiplayerUserRemoved(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member)
{
    RBX::mutex::scoped_lock lock(peersLock);
    RBXASSERT(peers.find(member->MemberId) != peers.end());

    if (peers.find(member->MemberId) != peers.end())
    {
        network->destroyPeer(peers[member->MemberId]);
        peers.erase(member->MemberId);
    }
}

void VoiceChatMaxNet::sendData(Xp2p::PeerPtr peer, Windows::Storage::Streams::IBuffer^ buffer, unsigned int flags)
{
    if (!peer) return;

    VoicePacket packet;
    packet.type = PacketType_VoicePacket;
    packet.size = sizeof(Xp2p::PacketHeader) + buffer->Length;

    ComPtr<IBufferByteAccess> bufferByteAccess;
    reinterpret_cast<IInspectable*>(buffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
    unsigned char* dataPtr; 
    bufferByteAccess->Buffer(&dataPtr);
    memcpy(packet.data, dataPtr, buffer->Length);

    Xp2p::NetResult result;
    if (peer)
        do
        {
            result = network->sendPacket(peer, &packet, 10, flags);
            if (result == Xp2p::Net_Retry)
                Sleep(1);
        } 
        while(result == Xp2p::Net_Retry);
}

void VoiceChatMaxNet::onOutgoingChatPacketReady(Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args)
{
    if (network.get())
    {
        unsigned flags = Xp2p::Packet_Default;
        flags |= args->SendReliable  ? Xp2p::Packet_Reliable : 0;
        flags |= args->SendInOrder ? Xp2p::Packet_Ordered : 0;

        if (args->SendPacketToAllConnectedConsoles)
        {
            RBX::mutex::scoped_lock lock(peersLock);
            for (auto peer : peers)
            {
                RBX::mutex::scoped_lock subscriptionLock(subscribedPeersLock);
                if (subscibedPeers.find(peer.first) != subscibedPeers.end())
                    sendData(peer.second, args->PacketBuffer, flags);
            }
        }
        else
        {
            RBX::mutex::scoped_lock lock(peersLock);
            unsigned int memId = dynamic_cast<ConsoleId^>(args->UniqueTargetConsoleIdentifier)->Value;
            Xp2p::PeerPtr peer = peers[memId];
            sendData(peer, args->PacketBuffer, flags);
        }
    }
}

void VoiceChatMaxNet::receive()
{
    Xp2p::PacketStorage storage;
    Xp2p::NetResult res = network->recvPacket(&storage, 1);

    if (res == Xp2p::Net_Ok)
    {
        Xp2p::PeerPtr peer; 
        do
        {
            res = network->processPacket(peer, &storage);

            if (res == Xp2p::Net_Ok)
            {
                switch (storage.type)
                {
                case PacketType_VoicePacket:
                    {
                        VoicePacket* packet = reinterpret_cast<VoicePacket*>(&storage);
                        ConsoleId^ consoleId = ref new ConsoleId(peer->getPeerId());

                        DataWriter ^writer = ref new DataWriter();
                        writer->WriteBytes(Platform::ArrayReference<unsigned char>(packet->data, packet->getDataSize()));
                        IBuffer ^buffer = writer->DetachBuffer();

                        chatManager->ProcessIncomingChatMessage(buffer, consoleId);
                        break;
                    }
                case PacketType_ControlPacket:
                    {
                        ControlVoicePacket* packet = reinterpret_cast<ControlVoicePacket*>(&storage);
                        ConsoleId^ consoleId = ref new ConsoleId(peer->getPeerId());


                        if (packet->subscribe > 0)
                        {
                            if (Microsoft::Xbox::GameChat::ChatUser^ user = getUserByMemberId(peer->getPeerId()))
                            {
                                boost::weak_ptr<VoiceChatMaxNet> weakPtrToThis = shared_from_this();

                                async(xbLiveContext->PrivacyService->CheckPermissionWithTargetUserAsync(Microsoft::Xbox::Services::Privacy::PermissionIdConstants::CommunicateUsingVoice, user->XboxUserId)).complete(
                                [weakPtrToThis, peer](Privacy::PermissionCheckResult^ result)
                                {
                                    boost::shared_ptr<VoiceChatMaxNet> sharedThis = weakPtrToThis.lock();
                                    if (sharedThis && result->IsAllowed)
                                    {
                                        RBX::mutex::scoped_lock lock(sharedThis->subscribedPeersLock);
                                        sharedThis->subscibedPeers.insert(peer->getPeerId());
                                    }
                                }).detach();
                            }
                        }
                        else
                        {
                            RBX::mutex::scoped_lock lock(subscribedPeersLock);
                            subscibedPeers.erase(peer->getPeerId());
                        }
                        break;
                    }
                default: 
                    RBXASSERT(false);
                }
            }

            if (res == Xp2p::Net_Retry)
                Sleep(5);
        }
        while (res == Xp2p::Net_Retry);
    }
}

void VoiceChatMaxNet::setMuteState(bool muted, Microsoft::Xbox::GameChat::ChatUser^ chatUser)
{
    if (chatManager != nullptr && chatUser != nullptr)
        if (!muted)
            chatManager->UnmuteUserFromAllChannels(chatUser);
        else
            chatManager->MuteUserFromAllChannels(chatUser);
}

unsigned VoiceChatMaxNet::getMuteState(unsigned memberId)
{
    if (Microsoft::Xbox::GameChat::ChatUser^ user = getUserByMemberId(memberId))
    {
        if (subscibedPeers.find(memberId) != subscibedPeers.end())
            return user->IsMuted ? VoiceChatState::voiceChatState_Available : VoiceChatState::voiceChatState_Muted;
        else
            return VoiceChatState::voiceChatState_NotInChat;
    }
    return VoiceChatState::voiceChatState_UnknownUser;
}

void VoiceChatMaxNet::muteAll()
{
    if (chatManager != nullptr)
        chatManager->MuteAllUsersFromAllChannels();
}

void VoiceChatMaxNet::unmuteAll()
{
    if (chatManager != nullptr)
        chatManager->UnmuteAllUsersFromAllChannels();
}

Microsoft::Xbox::GameChat::ChatUser^ VoiceChatMaxNet::getUserByMemberId(unsigned id)
{
    if (chatManager)
    {
        for (auto user : chatManager->GetChatUsers())
        {
            ConsoleId^ conId = dynamic_cast<ConsoleId^>(user->UniqueConsoleIdentifier);
            if (conId != nullptr && (unsigned)conId->Value == id)
            return user;
        }
    }
    
    return nullptr;
}

void VoiceChatMaxNet::muteUser(unsigned memberId)
{
    if (Microsoft::Xbox::GameChat::ChatUser^ user = getUserByMemberId(memberId))
        chatManager->MuteUserFromAllChannels(user);
}

void VoiceChatMaxNet::unmuteUser(unsigned memberId)
{
    if (Microsoft::Xbox::GameChat::ChatUser^ user = getUserByMemberId(memberId))
        chatManager->UnmuteUserFromAllChannels(user);
}

void VoiceChatMaxNet::onDebugMessageReceived(Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args)
{
    dprintf("GAMECHAT debug: %s", ws2s(args->Message->Data()).c_str());
}

void VoiceChatMaxNet::headsetConnected(Windows::Xbox::System::User^ user)
{
    addLocalUserToChannel(currentChannel, user);

    if (chatManager != nullptr)
    {
        // this will ignore all members that we didnt establish connection yet. Screw those guys right?
        RBX::mutex::scoped_lock lock(peersLock);
        for(auto peer : peers)
        {
            chatManager->HandleNewRemoteConsole(ref new ConsoleId(peer.first));
        }
    }
}

void VoiceChatMaxNet::addLocalUserToChannel(uint8 channelIndex, Windows::Xbox::System::User^ user)
{    
    RBX::mutex::scoped_lock lock(channelsLock);

    if(chatManager != nullptr && (currentChannel != channelIndex || currentUser != user))
    {
        async(chatManager->AddLocalUserToChatChannelAsync(channelIndex, user))
        .except( [](Platform::Exception^ e) -> void
        {
            dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
        })
        .join();

        removeLocalUserFromChannel_NoLock();

        currentChannel = channelIndex;
        currentUser = user;
    }
}

void VoiceChatMaxNet::removeLocalUserFromChannel()
{
    RBX::mutex::scoped_lock lock(channelsLock);
    removeLocalUserFromChannel_NoLock();    
}

void VoiceChatMaxNet::removeLocalUserFromChannel_NoLock()
{
    if(chatManager != nullptr && currentUser != nullptr && currentChannel >= 0 )
    {
        async(chatManager->RemoveLocalUserFromChatChannelAsync(currentChannel, currentUser))
            .except( [](Platform::Exception^ e) -> void
        {
            dprintf( "%s exception [0x%x]: %S\n", __FUNCTION__, (int)e->HResult, e->Message->Data() );
        }).join();

        currentChannel = -1;
        currentUser = nullptr;
    }
}

void VoiceChatMaxNet::addConnections(Windows::Foundation::Collections::IVectorView<MultiplayerSessionMember^>^ members)
{
    // we do something smarter in max net
}

void VoiceChatMaxNet::removeConnections(Windows::Foundation::Collections::IVectorView<MultiplayerSessionMember^>^ members)
{
    // we do something smarter in max net
}

void VoiceChatMaxNet::unsubscribeToChatMessages_NoLock()
{
    for (auto peer : peers)
    {
        sendVoiceControlPacket(peer.second, false);
    }
}

void VoiceChatMaxNet::subscribeToChatMessages_NoLock()
{
    for (auto peer : peers)
    {
        sendVoiceControlPacket(peer.second, true);
    }
}

void VoiceChatMaxNet::sendVoiceControlPacket(Xp2p::PeerPtr peer, bool subscribe)
{
    if (!peer) return;

    ControlVoicePacket packet;
    packet.type = PacketType_ControlPacket;
    packet.size = sizeof(ControlVoicePacket);
    packet.subscribe = subscribe ? 1 : 0;

    unsigned flags = Xp2p::Packet_Reliable;

    Xp2p::NetResult result;
    if (peer)
        do
        {
            result = network->sendPacket(peer, &packet, 10, flags);
            if (result == Xp2p::Net_Retry)
                Sleep(1);
        } 
        while(result == Xp2p::Net_Retry);
}

}