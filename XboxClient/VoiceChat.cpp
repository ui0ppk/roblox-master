#include "VoiceChat.h"

#include "XboxUtils.h"
#include "rbx/Debug.h"
#include "async.h"

#include <map>

#if defined(RBX_PLATFORM_DURANGO)
void dprintf( const char* fmt, ... );
#endif

using namespace Microsoft::Xbox::Services::Multiplayer;

namespace RBX
{

VoiceChat::VoiceChat(unsigned char sessionConsoleId)
    : currentUser(nullptr)
{
    chatManager = ref new Microsoft::Xbox::GameChat::ChatManager();
    chatManager->ChatSettings->AudioThreadPeriodInMilliseconds = 40;

    bool dropOutOfOrderPackets = false;
    std::string whateverName = "whatever";

    try
    {
        networkMesh = ref new Microsoft::Xbox::Samples::NetworkMesh::MeshManager(sessionConsoleId, "MultiplayerUdp", ref new Platform::String(s2ws(&whateverName).data()), dropOutOfOrderPackets);
    }
    catch (Platform::Exception^ ex)
    {
        HRESULT hr = ex->HResult;
        const WCHAR* ps = ex->Message->Data();
        std::string string = ws2s(ps);

        dprintf("Cannot init network mesh: %s", string.c_str());
    }
}

VoiceChat::~VoiceChat()
{
    Windows::Xbox::System::User::AudioDeviceAdded -= onAudioAdded;
    Windows::Xbox::System::User::AudioDeviceAdded -= onAudioRemoved;
    
    if( chatManager != nullptr )
    {
        chatManager->OnDebugMessage -= onDebugMessageEvtToken;
        chatManager->OnOutgoingChatPacketReady -= onOutgoingChatPacketReadyEvtToken;
        chatManager->OnCompareUniqueConsoleIdentifiers -= onCompareUniqueConsoleIdentifiersEvtToken;
        Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged -= onResourceAvailabilityChangedEvtToken;

        chatManager = nullptr;
    }

    if ( networkMesh != nullptr)
    {
        networkMesh->GetMeshPacketManager()->OnChatMessageReceived -= onChatMessageEvtToken;
        networkMesh->OnPostHandshake -= onMeshConnectionAddedEvtToken;
        networkMesh->OnDisconnected -= onDisconnectedEvtToken;

        networkMesh->Shutdown();
        networkMesh = nullptr;
    }
}

void VoiceChat::init()
{
    boost::weak_ptr<VoiceChat> weakPtrToThis = shared_from_this();

    if (networkMesh)
    {
        onChatMessageEvtToken = networkMesh->GetMeshPacketManager()->OnChatMessageReceived += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::Samples::NetworkMesh::MeshChatMessageReceivedEvent^>( 
            [weakPtrToThis]( Platform::Object^, Microsoft::Xbox::Samples::NetworkMesh::MeshChatMessageReceivedEvent^ args )
        {
            Windows::Storage::Streams::IBuffer^ chatPacket = args->Buffer;

            boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
            if( sharedPtrToThis != nullptr )
                sharedPtrToThis->onIncomingChatMessage(chatPacket, args->Sender->GetAssociation());
        });

        onMeshConnectionAddedEvtToken = networkMesh->OnPostHandshake += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^>( 
            [weakPtrToThis]( Platform::Object^, Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^ args )
        {
            boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
            if( sharedPtrToThis != nullptr )
                sharedPtrToThis->onMeshConnectionAdded(args);
        });

        onDisconnectedEvtToken = networkMesh->OnDisconnected += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^>( 
            [weakPtrToThis]( Platform::Object^, Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^ args )
        {
            boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
            if( sharedPtrToThis != nullptr )
                sharedPtrToThis->onMeshConnectionRemoved(args);
        });
    }

    onOutgoingChatPacketReadyEvtToken = chatManager->OnOutgoingChatPacketReady += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::ChatPacketEventArgs^>( 
        [weakPtrToThis] ( Platform::Object^, Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args )
    {
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            sharedPtrToThis->onOutgoingChatPacketReady(args);
        }
    });

    onDebugMessageEvtToken = chatManager->OnDebugMessage += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::DebugMessageEventArgs^>(
        [weakPtrToThis] ( Platform::Object^, Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args )
    {
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
            sharedPtrToThis->onDebugMessageReceived(args);
    });

    onCompareUniqueConsoleIdentifiersEvtToken = chatManager->OnCompareUniqueConsoleIdentifiers += ref new Microsoft::Xbox::GameChat::CompareUniqueConsoleIdentifiersHandler( 
        [weakPtrToThis] ( Platform::Object^ obj1, Platform::Object^ obj2 ) 
    { 
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            Windows::Xbox::Networking::SecureDeviceAssociation^ sda1 = dynamic_cast<Windows::Xbox::Networking::SecureDeviceAssociation^>(obj1);
            Windows::Xbox::Networking::SecureDeviceAssociation^ sda2 = dynamic_cast<Windows::Xbox::Networking::SecureDeviceAssociation^>(obj2);

            if( sda1 != nullptr && sda2 != nullptr && sda1->RemoteSecureDeviceAddress != nullptr && sda2->RemoteSecureDeviceAddress != nullptr && sda1->RemoteSecureDeviceAddress->Compare(sda2->RemoteSecureDeviceAddress) == 0 )
            {
                return true;
            }
        }

        return false;
    });

    onAudioAdded = Windows::Xbox::System::User::AudioDeviceAdded += ref new Windows::Foundation::EventHandler<Windows::Xbox::System::AudioDeviceAddedEventArgs^>(
        [weakPtrToThis] (Platform::Object^,Windows::Xbox::System:: AudioDeviceAddedEventArgs^ args)
    {
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            if (args->AudioDevice->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && 
				args->AudioDevice->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
			{
                sharedPtrToThis->headsetConnected(args->User);
			}
        }
    });

    onAudioRemoved = Windows::Xbox::System::User::AudioDeviceRemoved += ref new Windows::Foundation::EventHandler<Windows::Xbox::System::AudioDeviceRemovedEventArgs^>(
        [weakPtrToThis, this] (Platform::Object^,Windows::Xbox::System:: AudioDeviceRemovedEventArgs^ args)
    {
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
        {
            if (args->AudioDevice->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && 
				args->AudioDevice->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
            {
                sharedPtrToThis->headsetDisconnected();
            }
        }
    });

    // Upon enter constrained mode, mute everyone.  
    // Upon leaving constrained mode, unmute everyone who was previously muted.
    onResourceAvailabilityChangedEvtToken = Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged += ref new Windows::Foundation::EventHandler< Platform::Object^ >( 
        [weakPtrToThis] (Platform::Object^, Platform::Object^ )
    {
        boost::shared_ptr<VoiceChat> sharedPtrToThis(weakPtrToThis.lock());
        if( sharedPtrToThis != nullptr )
            if (Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Constrained)
                sharedPtrToThis->muteAll();
            else if(Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Full)
                sharedPtrToThis->unmuteAll();
        
    });
}

void VoiceChat::onMeshConnectionAdded(Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^ meshConnection)
{
    if(chatManager != nullptr)
        chatManager->HandleNewRemoteConsole(meshConnection->GetAssociation());
}

void VoiceChat::onMeshConnectionRemoved(Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^ meshConnection)
{
    if (chatManager != nullptr)
    {
        async(chatManager->RemoveRemoteConsoleAsync(meshConnection->GetAssociation())).detach();
    }
}

void VoiceChat::multiplayerUserAdded(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member)
{
    // not used in this implementation
}

void VoiceChat::multiplayerUserRemoved(Microsoft::Xbox::Services::Multiplayer::MultiplayerSessionMember^ member)
{
    // not used in this implementation
}

void VoiceChat::onOutgoingChatPacketReady(Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args)
{
    if (networkMesh)
    {
        if (args->SendPacketToAllConnectedConsoles)
        {
            // Send the chat packet to all connected consoles
            Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^>^ meshMembers = networkMesh->GetConnectionsByType(Microsoft::Xbox::Samples::NetworkMesh::ConnectionStatus::PostHandshake);        
            for(int i = 0; i < meshMembers->Size; i++)
            {
                Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^ member = meshMembers->GetAt(i);
                // The simple network layer in this sample doesn't support SendInOrder, so we are ignoring that
                networkMesh->GetMeshPacketManager()->SendChatMessage(member->GetAssociation(), args->PacketBuffer, args->SendReliable);
            }
        }
        else
        {
            Windows::Xbox::Networking::SecureDeviceAssociation^ targetAssociation = dynamic_cast<Windows::Xbox::Networking::SecureDeviceAssociation^>(args->UniqueTargetConsoleIdentifier);
            if (targetAssociation != nullptr)
            {
                networkMesh->GetMeshPacketManager()->SendChatMessage(targetAssociation, args->PacketBuffer, args->SendReliable);
            }
        }
    }
}

void VoiceChat::onIncomingChatMessage(Windows::Storage::Streams::IBuffer^ chatMessage, Platform::Object^ uniqueRemoteConsoleIdentifier)
{
    if(chatManager != nullptr)
    {
        Microsoft::Xbox::GameChat::ChatMessageType chatMessageType = chatManager->ProcessIncomingChatMessage(chatMessage, uniqueRemoteConsoleIdentifier);
    }
}

void VoiceChat::setMuteState(bool muted, Microsoft::Xbox::GameChat::ChatUser^ chatUser)
{
    if (chatManager != nullptr && chatUser != nullptr)
        if (!muted)
            chatManager->UnmuteUserFromAllChannels(chatUser);
        else
            chatManager->MuteUserFromAllChannels(chatUser);
}

void VoiceChat::muteAll()
{
    if (chatManager != nullptr)
        chatManager->MuteAllUsersFromAllChannels();
}

void VoiceChat::unmuteAll()
{
    if (chatManager != nullptr)
        chatManager->UnmuteAllUsersFromAllChannels();
}

void VoiceChat::onDebugMessageReceived(Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args)
{
    dprintf("GAMECHAT debug: %s", ws2s(args->Message->Data()).c_str());
}

void VoiceChat::headsetConnected(Windows::Xbox::System::User^ user)
{
    addLocalUserToChannel(0, user);

    if (chatManager && networkMesh)
    {
        // this will ignore all members that we didnt establish connection yet. Screw those guys right?
        Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::Samples::NetworkMesh::MeshConnection^>^ meshMembers = networkMesh->GetConnectionsByType(Microsoft::Xbox::Samples::NetworkMesh::ConnectionStatus::PostHandshake);        
        for(auto member : meshMembers)
        {
            chatManager->HandleNewRemoteConsole(member->GetAssociation());
        }
    }
}

void VoiceChat::headsetDisconnected()
{
    localUserHasHeadset = false;
    removeLocalUserFromChannel();
}

void VoiceChat::addLocalUserToChannel(uint8 channelIndex, Windows::Xbox::System::User^ user)
{
    bool oldHeadserState = localUserHasHeadset;

    localUserHasHeadset = false;
    if (user)
        for (auto audio : user->AudioDevices)
            if (audio->Sharing == Windows::Xbox::System::AudioDeviceSharing::Exclusive && audio->DeviceType == Windows::Xbox::System::AudioDeviceType::Capture)
            {
                localUserHasHeadset = true;
                break;
            }

    if(chatManager != nullptr && (currentUser != user || oldHeadserState != localUserHasHeadset))
    {
        if (localUserHasHeadset)
		{
            async(chatManager->AddLocalUserToChatChannelAsync(channelIndex, user)).detach();
			currentUser = user;
		}
    }
}

void VoiceChat::removeLocalUserFromChannel()
{
    if(chatManager != nullptr && currentUser != nullptr)
    {
        async(chatManager->RemoveLocalUserFromChatChannelAsync(0, currentUser)).detach();
        currentUser = nullptr;
    }
}

void VoiceChat::addConnections(Windows::Foundation::Collections::IVectorView<MultiplayerSessionMember^>^ members)
{
    if (chatManager && networkMesh)
    {
        std::map<Platform::String^, bool> deviceTokenSeenMap;
        for( auto member : members )
        {
            if (member->Status != MultiplayerSessionMemberStatus::Active || member->SecureDeviceAddressBase64->IsEmpty() || member->IsCurrentUser)
                continue;

            if( deviceTokenSeenMap.find(member->DeviceToken) == deviceTokenSeenMap.end() )
            {
                deviceTokenSeenMap[member->DeviceToken] = true;

                Windows::Xbox::Networking::SecureDeviceAddress^ memberSecureDeviceAddress = Windows::Xbox::Networking::SecureDeviceAddress::FromBase64String( member->SecureDeviceAddressBase64 );

                if( networkMesh != nullptr )
                    networkMesh->ConnectToAddress(memberSecureDeviceAddress, member->Gamertag);
            }
        }
    }   
}

void VoiceChat::removeConnections(Windows::Foundation::Collections::IVectorView<MultiplayerSessionMember^>^ members)
{
    if (chatManager && networkMesh)
    {
        std::map<Platform::String^, bool> deviceTokenSeenMap;
        for( auto member : members )
        {
            if (member->Status != MultiplayerSessionMemberStatus::Active || member->SecureDeviceAddressBase64->IsEmpty() || member->IsCurrentUser)
                continue;

            if( deviceTokenSeenMap.find(member->DeviceToken) == deviceTokenSeenMap.end() )
            {
                deviceTokenSeenMap[member->DeviceToken] = true;

                Windows::Xbox::Networking::SecureDeviceAddress^ memberSecureDeviceAddress = Windows::Xbox::Networking::SecureDeviceAddress::FromBase64String( member->SecureDeviceAddressBase64 );

                if( networkMesh != nullptr )
                    networkMesh->DisconectFromAddress(memberSecureDeviceAddress);
            }
        }
    }   
}

}