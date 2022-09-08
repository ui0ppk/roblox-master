/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#include "Client.h"
#include "ClientReplicator.h"

#include "PacketLogger.h"
#include "Util.h"
#include "ConcurrentRakPeer.h"
#include "Network/Players.h"
#include "Network/NetworkOwner.h"
#include "Util/ProgramMemoryChecker.h"
#include "util/standardout.h"
#include "util/ProtectedString.h"
#include "Util/RbxStringTable.h"
#include "CPUCount.h"
#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/DataModel.h"
#include "V8DataModel/DebugSettings.h"
#include "v8datamodel/TeleportService.h"

#include "Script/scriptcontext.h"

#include "RakNetStatistics.h"
#include "VMProtect/VMProtectSDK.h"

LOGGROUP(US14116)
DYNAMIC_FASTFLAG(DebugDisableTimeoutDisconnect)
FASTFLAG(DebugLocalRccServerConnection)

DYNAMIC_LOGGROUP(NetworkJoin)
FASTFLAG(DebugProtocolSynchronization)

#ifndef _WIN32
// For inet_addr() call used below
#include <arpa/inet.h>
#endif

const char* const RBX::Network::sClient = "NetworkClient";

namespace RBX
{
	extern const char *const sHopper;
    class Instance;
}

using namespace RBX;
using namespace RBX::Network;
using namespace RakNet;

REFLECTION_BEGIN();
Reflection::BoundProp<std::string> Client::prop_Ticket("Ticket", "Authentication", &Client::ticket);
static Reflection::BoundFuncDesc<Client, shared_ptr<Instance>(int, std::string, int, int, int)> f_connect(&Client::playerConnect, "PlayerConnect", "userId", "server", "serverPort", "clientPort", 0, "threadSleepTime", 30, Security::Plugin);
static Reflection::BoundFuncDesc<Client, void(int)> f_disconnect(&Client::disconnect, "Disconnect", "blockDuration", 3000, Security::LocalUser);
static Reflection::BoundFuncDesc<Client, void(std::string)> func_setGameSessionID(&Client::setGameSessionID, "SetGameSessionID", "gameSessionID", Security::Roblox);
static Reflection::EventDesc<Client, void(std::string, shared_ptr<RBX::Instance>)> event_ConnectionAccepted(&Client::connectionAcceptedSignal, "ConnectionAccepted", "peer", "replicator");
static Reflection::EventDesc<Client, void(std::string)> event_ConnectionRejected(&Client::connectionRejectedSignal, "ConnectionRejected", "peer");
static Reflection::EventDesc<Client, void(std::string, int, std::string)> event_ConnectionFailed(&Client::connectionFailedSignal, "ConnectionFailed", "peer", "code", "reason");
REFLECTION_END();

Client::Client()
	: userId(-1), networkSettings(&NetworkSettings::singleton()), isCloudEditClient(false)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "create a NetworkClient");
	setName(sClient);

	FASTLOG(FLog::Network, "NetworkClient:Create");
}

Client::~Client(void)
{
	FASTLOG(FLog::Network, "NetworkClient:Remove");
}

Client* Client::findClient(const RBX::Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	RBXASSERT(!testInDatamodel || serviceProvider!=NULL);
	return ServiceProvider::find<Client>(serviceProvider);
}

bool Client::clientIsPresent(const RBX::Instance* context, bool testInDatamodel)
{
	return findClient(context, testInDatamodel) != NULL;
}

bool Client::physicsOutBandwidthExceeded(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context))
	{
		if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>())
		{
			return clientRep->isLimitedByOutgoingBandwidthLimit();
		}
	}
	return true;
}

double Client::getNetworkBufferHealth(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context))
	{
		return client->rakPeer->GetBufferHealth();
	}
	return 0.0f;
}

const RBX::SystemAddress Client::findLocalSimulatorAddress(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context, false)) {
		if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>()) {
			return RakNetToRbxAddress(clientRep->getClientAddress());
		}
	}
	return Network::NetworkOwner::Unassigned();
}

shared_ptr<Instance> Client::playerConnect(int userId, std::string server, int serverPort, int clientPort, int threadSleepTime)
{
	FASTLOG3(FLog::Network, "Client:Connect serverPort(%d) clientPort(%d) threadSleepTime(%d)", serverPort, clientPort, threadSleepTime);

	this->userId = userId;
	Players* players = ServiceProvider::create<Players>(this);
	if(!players)
		throw RBX::runtime_error("Cannot get players");

	shared_ptr<Instance> player = players->createLocalPlayer(userId, TeleportService::getPreviousPlaceId() > 0);

	if (clientPort == 0) {
		clientPort = networkSettings->preferredClientPort;
	}

	RakNet::SocketDescriptor d(clientPort, "");
	StartupResult startRes = rakPeer->rawPeer()->Startup(1, &d, 1);
	if (startRes != RakNet::RAKNET_STARTED)
    {
		if (clientPort==0)
        {
			throw RBX::runtime_error("Failed to start network client");
        }
		else
        {
			throw RBX::runtime_error("Failed to start network client on port %d", clientPort);
        }
    }

	// allow local and LAN games only.
	if (server != "localhost")
	{
		bool lansubnet = false;
		for (int i=0; !lansubnet && i<MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++i)
		{
			RakNet::SystemAddress localAddress = rakPeer->rawPeer()->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
			RakNet::SystemAddress remoteAddress(server.c_str(), serverPort);

			// match the a and b records
			lansubnet = (localAddress.GetBinaryAddress() & 0x00FF) == (remoteAddress.GetBinaryAddress() & 0x00FF);
		}
		if (!FFlag::DebugLocalRccServerConnection)
		{
			if (!lansubnet)
			{
				RBX::Security::Context::current().requirePermission(RBX::Security::Roblox, " connect to an extranet game");
			}
		}
	}
    if (FFlag::DebugLocalRccServerConnection)
    {
        //skip the security check
        Network::versionB = "test";
    }

	RakNet::ConnectionAttemptResult connectRes = rakPeer->rawPeer()->Connect(server.c_str(), serverPort, Network::versionB.c_str(), Network::versionB.size());
	if (connectRes != RakNet::CONNECTION_ATTEMPT_STARTED)
    {
		throw RBX::runtime_error("Failed to connect to server, id %d", connectRes);
    }
	FASTLOG1F(DFLog::NetworkJoin, "playerConnect connecting to server @ %f s", Time::nowFastSec());

	if(DFFlag::DebugDisableTimeoutDisconnect)
		rakPeer->rawPeer()->SetTimeoutTime(10*60*1000, UNASSIGNED_SYSTEM_ADDRESS);


	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connecting to %s:%d", server.c_str(), serverPort);

	FASTLOG2(FLog::Network, "Connecting to server, IP(inet_addr): %u Port: %u", inet_addr(server.c_str()), serverPort);

	RBX::RbxDbgInfo::SetServerIP(server.c_str());

	return player;
}



void Client::disconnect(int blockDuration)
{
	FASTLOG(FLog::Network, "Client:Disconnect");

	// The following line will remove the Replicator
	this->visitChildren(boost::bind(&Instance::unlockParent, _1));
	this->removeAllChildren();

	if (rakPeer)
	{
		rakPeer->rawPeer()->CloseConnection(this->serverId, true);
		rakPeer->rawPeer()->Shutdown(blockDuration);
	}
}

void Client::setGameSessionID(std::string value)
{
	if (value != Http::gameSessionID)
	{
		Http::gameSessionID = value;
	}
}

void Client::configureAsCloudEditClient()
{
	isCloudEditClient = true;
}

bool Client::isCloudEdit() const
{
	return isCloudEditClient;
}

void Client::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		closingConnection.disconnect();

		disconnect(); // We should have disconnected by now (in response to the Closing event)

		Players* players = ServiceProvider::find<Players>(oldProvider);
		players->setConnection(NULL);
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		//We're in multiplayer mode, so burn out the studio tools
		if(RBX::DataModel* dataModel = RBX::DataModel::get(this)){
			if(dataModel->lockVerb.get())
				dataModel->lockVerb->doIt(NULL);
		}

		Players* players = ServiceProvider::create<Players>(newProvider);
		players->setConnection(rakPeer.get());

		// Disconnect now before we start getting DescendantRemoving events
		// If we don't disconnect first, then we'll send a shower of delete messages
		// to the Server
		closingConnection = newProvider->closingSignal.connect(boost::bind(&Client::disconnect, this));
	}

}

void Client::sendVersionInfo()
{
    RakNet::BitStream bitStream;
    bitStream << (unsigned char) ID_PROTOCOL_SYNC;
    bitStream << protocolVersion;

    rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::sendTicket()
{
	RakNet::BitStream bitStream;
	bitStream << (unsigned char) ID_SUBMIT_TICKET;

	bitStream << userId;
    serializeStringCompressed(ticket, bitStream);

	serializeStringCompressed(RBX::DataModel::hash, bitStream);

	bitStream << protocolVersion;

    serializeStringCompressed(securityKey, bitStream);

	// TODO: better way to track protocol changes between versions
	// Network Protocol version 2
	serializeStringCompressed(DebugSettings::singleton().osPlatform(), bitStream);
    serializeStringCompressed(DebugSettings::singleton().getRobloxProductName(), bitStream);

    serializeStringCompressed(Http::gameSessionID, bitStream);

    unsigned int reportedGoldHash = RBX::Security::rbxGoldHash;

    bitStream << reportedGoldHash;

	encryptDataPart(bitStream);

	// Send ID_SUBMIT_TICKET
	rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

std::string rakIdToString(int id)
{
	switch (id)
	{
	case ID_INVALID_PASSWORD:
	case ID_HASH_MISMATCH:
		return "ROBLOX version is out of date. Please uninstall and try again.";
	case ID_CONNECTION_ATTEMPT_FAILED:
		return "Connection attempt failed.";
	case ID_SECURITYKEY_MISMATCH:
		return "Version not compatible with server. Please uninstall and try again.";
	default:
		return RBX::format("Network error %d", id);
	}
}

void Client::OnFailedConnectionAttempt(RakNet::Packet *packet, RakNet::PI2_FailedConnectionAttemptReason failedConnectionAttemptReason)
{
	std::string message = rakIdToString(packet->data[0]);
	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Failed to connect to %s. %s\n", RakNetAddressToString(packet->systemAddress).c_str(), message.c_str());
	connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int) packet->data[0], message);
}

// Cheat Engine StealthEdit Plugin helper. Name obscured for security.
#if !defined(RBX_STUDIO_BUILD)
static void programMemoryPermissionsHackChecker(weak_ptr<DataModel> weakDataModel) {
	static const unsigned int kSleepBetweenStealthEditChecksMillis = 2 * 1000;
	VMProtectBeginMutation("24");
	while (true) {
		shared_ptr<DataModel> dataModel = weakDataModel.lock();
		if (!dataModel) { break; }

		//FASTLOG(FLog::US14116, "Starting stealth check");
		if (ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking()) {
			//FASTLOG(FLog::US14116, "Caught stealthedit!");
            RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag7, HATE_CATCH_EXECUTABLE_ACCESS_VIOLATION);
		}
		//FASTLOG1(FLog::US14116, "Sleeping stealth for %ums", kSleepBetweenStealthEditChecksMillis);
		boost::this_thread::sleep(boost::posix_time::milliseconds(kSleepBetweenStealthEditChecksMillis));
	}
	VMProtectEnd();
}
#endif

void Client::sendPreferedSpawnName() const {
	RakNet::BitStream bitStream;

	bitStream << (unsigned char) ID_SPAWN_NAME;

    serializeStringCompressed(TeleportService::GetSpawnName(), bitStream);

	FASTLOGS(FLog::Network, "serverId: %s", RakNetAddressToString(serverId).c_str());

	rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::HandleConnection(RakNet::Packet *packet)
{
    shared_ptr<Replicator> proxy;
    try
    {
        // send previous placeId
        RakNet::BitStream bitStream;
        bitStream << (unsigned char) ID_PLACEID_VERIFICATION;

        bitStream << TeleportService::getPreviousPlaceId();

        rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);

        sendTicket();

        sendPreferedSpawnName();

        Workspace* workspace = Workspace::findWorkspace(this);
        workspace->clearTerrain();

        proxy = Creatable<Instance>::create<ClientReplicator>(packet->systemAddress, this, rakPeer->rawPeer()->GetExternalID(serverId), networkSettings);

        proxy->setAndLockParent(this);

#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO) 
        VMProtectBeginMutation("25");
		{
            weak_ptr<DataModel> weakDataModel = weak_from(DataModel::get(this));

            boost::thread t(boost::bind(&programMemoryPermissionsHackChecker, weakDataModel));
            spawnDebugCheckThreads(weakDataModel);
            if (!weakDataModel.lock())
            {
                DataModel::get(this)->addHackFlag(HATE_WEAK_DM_POINTER_BROKEN);
            }
        }
        VMProtectEnd();
#endif

        connectionAcceptedSignal(RakNetAddressToString(packet->systemAddress), proxy);
    }
    catch (RBX::base_exception& e)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in ID_CONNECTION_REQUEST_ACCEPTED: %s", e.what());
        if (proxy)
        {
            // Disconnect
            proxy->unlockParent();
            proxy->setParent(NULL);
        }
    }
}

RakNet::PluginReceiveResult Client::OnReceive(RakNet::Packet *packet)
{
	RakNet::PluginReceiveResult result = Super::OnReceive(packet);
	if (result!=RR_CONTINUE_PROCESSING)
		return result;

	switch ((unsigned char) packet->data[0])
	{
	case ID_CONNECTION_REQUEST_ACCEPTED:
		{
            StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connection accepted from %s\n", RakNetAddressToString(packet->systemAddress).c_str());

            serverId = packet->systemAddress;

            HandleConnection(packet);
            sendVersionInfo();
        }
        return RR_CONTINUE_PROCESSING;

	case ID_INVALID_PASSWORD:
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Invalid password from %s", RakNetAddressToString(packet->systemAddress).c_str());
		connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int) packet->data[0], rakIdToString(packet->data[0]));
		connectionRejectedSignal(RakNetAddressToString(packet->systemAddress));
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	
	case ID_HASH_MISMATCH:
	case ID_SECURITYKEY_MISMATCH:
		connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int) packet->data[0], rakIdToString(packet->data[0]));
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_LOST:
		RBXASSERT(packet->systemAddress==serverId);
		serverId = UNASSIGNED_SYSTEM_ADDRESS;
		return RR_CONTINUE_PROCESSING;

	default:
		return RR_CONTINUE_PROCESSING;
	}
}




