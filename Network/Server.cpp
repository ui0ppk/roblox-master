/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#include "Server.h"

#include "Network/API.h"
#include "Network/NetworkClusterPacketCache.h"
#include "Network/Players.h"
#include "ConcurrentRakPeer.h"
#include "NetworkSettings.h"
#include "NetworkOwnerJob.h"
#include "ServerReplicator.h"
#include "Util.h"

#include "Script/ModuleScript.h"
#include "Script/script.h"
#include "Util/http.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/Statistics.h"
#include "Util/SoundService.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/partinstance.h"
#include "V8DataModel/Workspace.h"			// TODO - move distributed physics switch somewhere else
#include "V8DataModel/message.h"
#include "V8DataModel/MegaCluster.h"
#include "V8datamodel/TimerService.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8World/Primitive.h"

#include "RakPeer.h"
#include "GetTime.h"
#include <boost/thread/xtime.hpp>
#include <sstream>

#if !defined(_WIN32)
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "FastLog.h"

#include "script/LuaVM.h"

DYNAMIC_LOGGROUP(NetworkJoin)

FASTFLAG(DebugLocalRccServerConnection)
DYNAMIC_FASTFLAG(DebugDisableTimeoutDisconnect)
DYNAMIC_FASTFLAGVARIABLE(RCCSupportCloudEdit, false)
DYNAMIC_FASTFLAGVARIABLE(CloudEditGARespectsThrottling, false)
DYNAMIC_FASTFLAGVARIABLE(CloudEditCheckClientPresent, false)

using namespace RBX;
using namespace Network;
using namespace RakNet;

const char* const Network::sServer = "NetworkServer";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Server, void(int, int)> server_startFunction(&Server::start, "Start", "port", 0, "threadSleepTime", 20, Security::Plugin);
static Reflection::BoundFuncDesc<Server, void(int)> f_disconnect(&Server::stop, "Stop", "blockDuration", 1000, Security::LocalUser);
static Reflection::BoundFuncDesc<Server, int()> f_GetClientCount(&Server::getClientCount, "GetClientCount", Security::LocalUser);
static Reflection::PropDescriptor<Server, int> prop_Port("Port", category_Data, &Server::getPort, NULL);
static Reflection::BoundFuncDesc<Server, void(bool)> func_SetIsPlayerAuthenticationRequired(&Server::setIsPlayerAuthenticationRequired, "SetIsPlayerAuthenticationRequired", "value", Security::Roblox);
static Reflection::BoundFuncDesc<Server, void()> func_ConfigureAsCloudEditServer(&Server::configureAsCloudEditServer, "ConfigureAsCloudEditServer", Security::Roblox);

static Reflection::EventDesc<Server, void(shared_ptr<Instance>, FilterResult, shared_ptr<Instance>, std::string)> desc_dataBasicFiltered(&Server::dataBasicFilteredSignal, "DataBasicFiltered", "peer", "result", "instance", "member", Security::LocalUser);
static Reflection::EventDesc<Server, void(shared_ptr<Instance>, FilterResult, shared_ptr<Instance>, std::string)> desc_dataCustomFiltered(&Server::dataCustomFilteredSignal, "DataCustomFiltered", "peer", "result", "instance", "member", Security::LocalUser);

Reflection::EventDesc<Server, void(std::string, shared_ptr<Instance>)> event_IncommingConnection(&Server::incommingConnectionSignal, "IncommingConnection", "peer", "replicator", Security::RobloxScript);
REFLECTION_END();

static const int maxClients = 128;

static shared_ptr<Network::ServerReplicator> createReplicator(RakNet::SystemAddress a, Network::Server* s, NetworkSettings* networkSettings)
{
	// Creates an ordinary ServerReplicator without security or cheat handling code
	return Creatable<Instance>::create<Network::ServerReplicator>(a, s, networkSettings);
}

boost::function<shared_ptr<ServerReplicator>(RakNet::SystemAddress, Server*, NetworkSettings*)> Server::createReplicator = ::createReplicator;

// allowedSecuirtyVersions is modified from a RCCService thread,
// we need to protect it with a mutex because this list is checked every time a new client joins the server.
static std::vector<std::string> allowedSecurityVersions;
static boost::mutex securityVersionsMutex;

struct Accumulator
{
	float total;
	int num;

	Accumulator() : total(0), num(0) {}
	void add(float value) {total += value; num++;}
	float getAvgValue() {return total / num;}
};

static void reportServerStats(weak_ptr<Server> server)
{
	shared_ptr<Server> sharedServer = server.lock();
	if (!sharedServer)
		return;

	DataModel* dm = DataModel::get(sharedServer.get());
	if (!dm)
		return;

	double totalKBytesSendPerSec = 0;
	double totalDataBytesSendPerSec = 0;
	double totalPhysicsBytesSendPerSec = 0;
	int numPlayers = 0;

	typedef boost::unordered_map<std::string, Accumulator> PacketLossPercentByPlatforms;
	PacketLossPercentByPlatforms packetLossPercentByPlatforms;

	if (sharedServer->getChildren())
	{
		Instances::const_iterator end = sharedServer->getChildren()->end();
		for (Instances::const_iterator iter = sharedServer->getChildren()->begin(); iter != end; ++iter)
		{
			if (ServerReplicator* rep = Instance::fastDynamicCast<ServerReplicator>(iter->get()))
			{
				if (Player* player = rep->getRemotePlayer())
				{
					// log stats only for player that has been in game for more then 5 mins
					int elapsedTime = (RakNet::GetTimeUS() - rep->stats().peerStats.rakStats.connectionStartTime) / 1e6f;
					if (elapsedTime > 5 * 60)
					{
						std::string osPlatform = player->getOsPlatform();

						const ReplicatorStats& stats = rep->stats();
						totalKBytesSendPerSec += stats.kiloBytesSentPerSecond;
						totalDataBytesSendPerSec += stats.dataPacketsSent.rate() * stats.dataPacketsSentSize.value();
						totalPhysicsBytesSendPerSec += stats.physicsSenderStats.physicsPacketsSent.rate() * stats.physicsSenderStats.physicsPacketsSentSize.value();
						numPlayers++;

						packetLossPercentByPlatforms[osPlatform].add(rep->stats().peerStats.maxPacketloss);
					}
				}
			}
		}
	}

	if (totalKBytesSendPerSec)
	{
		Analytics::EphemeralCounter::reportStats("ServerBytesSentPerSec", totalKBytesSendPerSec);
		Analytics::EphemeralCounter::reportStats("ServerDataBytesSentPerSec", totalDataBytesSendPerSec);
		Analytics::EphemeralCounter::reportStats("ServerPhysicsBytesSentPerSec", totalPhysicsBytesSendPerSec);

		Analytics::EphemeralCounter::reportStats("ServerBytesSentPerSecPerPlayer", totalKBytesSendPerSec / numPlayers);
		Analytics::EphemeralCounter::reportStats("ServerDataBytesSentPerSecPerPlayer", totalDataBytesSendPerSec / numPlayers);
		Analytics::EphemeralCounter::reportStats("ServerPhysicsBytesSentPerSecPerPlayer", totalPhysicsBytesSendPerSec / numPlayers);
	}
	
	for (PacketLossPercentByPlatforms::iterator i = packetLossPercentByPlatforms.begin(); i != packetLossPercentByPlatforms.end(); i++)
	{
		Analytics::EphemeralCounter::reportStats("ServerPacketLossPercent_"+i->first, i->second.getAvgValue());
	}

	dm->create<TimerService>()->delay(boost::bind(&reportServerStats, server), 10*60);
}

Server::Server(void)
	:outgoingPort(0)
	, isPlayerAuthenticationRequired(false)
	, networkSettings(&NetworkSettings::singleton())
	, isCloudEditServer(false)
{
	Security::Context::current().requirePermission(Security::Plugin, "create a NetworkServer");
	setName(sServer);

	//Allow empty script to always come in
    registerLegalScript("");
    scriptsByCurrentBytecode[""] = 0;
    scriptsByLegacyBytecode[""] = 0;
	
	FASTLOG(FLog::Network, "NetworkServer:Create");
}

Server::~Server(void)
{
	FASTLOG(FLog::Network, "NetworkServer:Destroy");
}

bool Server::serverIsPresent(const Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	RBXASSERT(!testInDatamodel || serviceProvider!=NULL);
	return ServiceProvider::find<Server>(serviceProvider)!=NULL;
}

std::vector<std::string> Server::getAllIPv4Addresses()
{
    std::vector<std::string> ipAddresses;
    
#if defined(_WIN32)
	WORD wVersionRequested;
	WSADATA wsaData;
	char name[255];
	PHOSTENT hostinfo;
	wVersionRequested = MAKEWORD( 1, 1 );
	char *ip;

	if ( WSAStartup( wVersionRequested, &wsaData ) == 0 )
	{
		if( gethostname ( name, sizeof(name)) == 0)
		{
			if((hostinfo = gethostbyname(name)) != NULL)
			{
				int nCount = 0;
				while(hostinfo->h_addr_list[nCount])
				{
					ip = inet_ntoa(*(
					struct in_addr *)hostinfo->h_addr_list[nCount]);
					ipAddresses.push_back(std::string(ip));
					nCount++;
				}
			}
		}
	}

	// ips that we get here are only external, 
	// so add a local address as well 
	// server can't connect to same machine clients otherwise
	ipAddresses.push_back("127.0.0.1");

#else
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;
    
    getifaddrs(&ifAddrStruct);
    
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            ipAddresses.push_back(std::string(addressBuffer));
        }
    }
    
    if (ifAddrStruct!=NULL)
        freeifaddrs(ifAddrStruct);
#endif

	return ipAddresses;
}

void Server::start(int port, int threadSleepTime)
{
	if (DFFlag::CloudEditCheckClientPresent && Players::clientIsPresent(this))
		throw RBX::runtime_error("Can not call server, client is present.");

	FASTLOG(FLog::Network, "NetworkServer:Start");
    
    StartupResult res = STARTUP_OTHER_FAILURE;
    std::vector<std::string> addresses;
    
#ifdef RBX_STUDIO_BUILD
    addresses = Server::getAllIPv4Addresses();
    shared_ptr<RakNet::SocketDescriptor> sdArray(new RakNet::SocketDescriptor[addresses.size()]);
        
    for (unsigned int i = 0; i < addresses.size(); i++)
    {
        sdArray.get()[i].port = port;
        strcpy(sdArray.get()[i].hostAddress, addresses[i].c_str());
    }
        
    res = rakPeer->rawPeer()->Startup(maxClients, sdArray.get(), addresses.size());
#else
	RakNet::SocketDescriptor d(port, 0);
	res = rakPeer->rawPeer()->Startup(maxClients, &d, 1);
#endif

	if (res != RakNet::RAKNET_STARTED)
		throw std::runtime_error(RBX::format("Failed to start network server, id %d", res));

#ifdef RBX_STUDIO_BUILD
	for (unsigned int i = 0; i < addresses.size(); i++)
	{
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE,"Started network server on %s|%i",addresses[i].c_str(),port);
	}
#else
	RakNet::SystemAddress address = rakPeer->rawPeer()->GetMyBoundAddress();
	outgoingPort = address.GetPort();

	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Started network server %s", RakNetAddressToString(address).c_str());
#endif
   
	DataModel *dataModel = DataModel::get(this);

	int startupMillis = static_cast<int>((Time::nowFast() - dataModel->getDataModelInitTime()).msec());
	RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_GAME, "ServerStartTime", startupMillis);

	if(DFFlag::DebugDisableTimeoutDisconnect)
		rakPeer->rawPeer()->SetTimeoutTime(10*60*1000, UNASSIGNED_SYSTEM_ADDRESS);


	dataModel->create<TimerService>()->delay(boost::bind(&reportServerStats, weak_from(this)), 5 * 60);
}

static bool isReplicator(shared_ptr<Instance> instance)
{
	return Instance::fastDynamicCast<Replicator>(instance.get())!=NULL;
}

int Server::getClientCount()
{
	if (DFFlag::CloudEditCheckClientPresent && Players::clientIsPresent(this))
		throw RBX::runtime_error("Can not call server, client is present.");

	if (getChildren())
		return std::count_if(getChildren()->begin(), getChildren()->end(), &isReplicator);
	else
		return 0;
}

void Server::stop(int blockDuration)
{
	if (DFFlag::CloudEditCheckClientPresent && Players::clientIsPresent(this))
		throw RBX::runtime_error("Can not call server, client is present.");

	FASTLOG1(FLog::Network, "NetworkServer:Stop blockDuration(%d)", blockDuration);

	// The following line will remove the Replicators
	// we have to do this first before shutting down rakpeer because replicator might hold
	// a list of unprocessed packets that was allocated from a pool inside rakpeer. rakpeer
	// clears this pool in shutdown.
	this->visitChildren(boost::bind(&Instance::unlockParent, _1));
	this->removeAllChildren();
	
	if (rakPeer->rawPeer()->IsActive())
		rakPeer->rawPeer()->Shutdown(blockDuration);
}

static void reportCloudEditGA(const char* label, int value = 0)
{
	if (DFFlag::CloudEditGARespectsThrottling)
	{
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CloudEdit", label, value);
	}
	else
	{
		RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_GAME, "CloudEdit", label, value);
	}
}

static void reportCloudEditStats(weak_ptr<Server> server)
{
	shared_ptr<Server> sharedServer = server.lock();
	if (!sharedServer)
		return;

	DataModel* dm = DataModel::get(sharedServer.get());
	if (!dm)
		return;

	RBXASSERT(dm->currentThreadHasWriteLock());

	int players = -1;
	if (Players* p = dm->find<Players>())
	{
		players = p->numChildren();
	}

	reportCloudEditGA("5 Minute Usage", players);

	dm->create<TimerService>()->delay(boost::bind(&reportCloudEditStats, server), 5*60);
}

void Server::configureAsCloudEditServer()
{
	if (DFFlag::CloudEditCheckClientPresent && Players::clientIsPresent(this))
		throw RBX::runtime_error("Can not call server, client is present.");

	if (!DFFlag::RCCSupportCloudEdit)
	{
		return;
	}

	initWithCloudEditSecurity();
	rakPeer->rawPeer()->SetIncomingPassword(Network::versionB.c_str(), Network::versionB.size());
	isCloudEditServer = true;
	reportCloudEditGA("Server Start");
	DataModel::get(this)->create<TimerService>()->delay(boost::bind(&reportCloudEditStats, weak_from(this)), 5*60);
}

void Server::onCreateRakPeer()
{	
	Super::onCreateRakPeer();
	rakPeer->rawPeer()->SetMaximumIncomingConnections(maxClients);
	if (FFlag::DebugLocalRccServerConnection)
	{
		Network::versionB = "test";
	}
	rakPeer->rawPeer()->SetIncomingPassword(Network::versionB.c_str(), Network::versionB.size());
}


void Server::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{	
	if (oldProvider)
	{
		TaskScheduler::singleton().remove(networkOwnerJob);
		networkOwnerJob.reset();

		if (players)
		{
			players->setConnection(NULL);
			stop();
			players.reset();
		}
		itemAddedConnection.disconnect();
		workspaceLoadedConnection.disconnect();
	}



	if (newProvider && Players::clientIsPresent(newProvider))
		throw RBX::runtime_error("Can not create server, client is present.");

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		players = shared_from(ServiceProvider::create<Players>(newProvider));
		players->setConnection(rakPeer.get());
	
		DataModel* dataModel = boost::polymorphic_downcast<DataModel*>(newProvider);

		if (networkSettings->usePhysicsPacketCache)
			physicsPacketCache = ServiceProvider::create<PhysicsPacketCache>(newProvider);

		if (networkSettings->useInstancePacketCache)
			instancePacketCache = ServiceProvider::create<InstancePacketCache>(newProvider);

		ServiceProvider::create<OneQuarterClusterPacketCache >(newProvider);
		ServiceProvider::create<ClusterPacketCache>(newProvider);
		
		if (networkSettings->distributedPhysicsEnabled)
		{
			networkOwnerJob = shared_ptr<NetworkOwnerJob>( new NetworkOwnerJob(shared_from(dataModel) ) );
			TaskScheduler::singleton().add(networkOwnerJob);
		}

		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "PlaceID", "none", dataModel->getPlaceID());


		if (0 == dataModel->getPlaceID())
		{
			onWorkspaceLoaded();
		}
		else
		{
			workspaceLoadedConnection = dataModel->workspaceLoadedSignal.connect(boost::bind(&Server::onWorkspaceLoaded, this));
		}
	}
}

void Server::onWorkspaceLoaded()
{
	Workspace *workspace = ServiceProvider::find<Workspace>(this);

	if (workspace->getNetworkStreamingEnabled())
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "NetworkStreamingEnabled");

	{
		MegaClusterInstance *megaCluster = Instance::fastDynamicCast<MegaClusterInstance>(workspace->getTerrain());
		if (megaCluster && megaCluster->isAllocated())
		{
			char placeId[32];
			sprintf(placeId, "%d", DataModel::get(this)->getPlaceID());

            if (megaCluster->isSmooth())
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SmoothTerrain", placeId);
			}
			else
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "LegacyTerrain", placeId);
			}
		}
	}

	StarterPlayerService* sps = ServiceProvider::create<StarterPlayerService>(this);    
	if (sps)
		sps->recordSettingsInGA();
	
	if (DataModel* dataModel = DataModel::get(this))
	{
		dataModel->visitDescendants(boost::bind(&Server::onItemAdded, this, _1));
		itemAddedConnection = dataModel->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&Server::onItemAdded, this, _1));
	}
	else
	{
		RBXASSERT(false);
	}
}

void Server::onItemAdded(shared_ptr<Instance> item)
{
    boost::optional<std::string> scriptSource = getScriptSourceFromInstance(item.get());
	
	if (scriptSource)
        registerLegalScript(*scriptSource);
}

boost::optional<std::string> Server::getScriptSourceFromInstance(Instance* instance) const
{
	if (const Script* script = Instance::fastDynamicCast<Script>(instance))
	{
		if (script->isCodeEmbedded())
		{
			return script->getEmbeddedCode().get().getSource();
		}
		else
		{
			return script->getCachedRemoteSource().getSource();
		}
	}
	else if (const ModuleScript* moduleScript = Instance::fastDynamicCast<ModuleScript>(instance))
	{
		if (moduleScript->getScriptId().isNull())
		{
			return moduleScript->getSource().getSource();
		}
		else
		{
			return moduleScript->getCachedRemoteSource().getSource();
		}
	}
	else
	{
		return boost::optional<std::string>();
	}
}

bool Server::askAddChild(const Instance* instance) const
{
	return Instance::fastDynamicCast<ServerReplicator>(instance) != NULL;
}

RakNet::PluginReceiveResult Server::OnReceive(RakNet::Packet *packet)
{
	RakNet::PluginReceiveResult result = Super::OnReceive(packet);
	if (result!=RR_CONTINUE_PROCESSING)
		return result;

	switch ((unsigned char) packet->data[0])
	{
	case ID_NEW_INCOMING_CONNECTION:
		{
			shared_ptr<ServerReplicator> proxy;
			try
			{
				FASTLOG(FLog::Network, "NetworkServer:NewIncomingConnection");
				FASTLOG1F(DFLog::NetworkJoin, "Server::OnReceive new connection @ %f s", Time::nowFastSec());
				StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "New connection from %s\n", RakNetAddressToString(packet->systemAddress).c_str());

                proxy = createReplicator(packet->systemAddress, this, networkSettings);
                proxy->setAndLockParent(this);
                incommingConnectionSignal(RakNetAddressToString(packet->systemAddress), proxy);
			}
			catch (RBX::base_exception& e)
			{
				FASTLOG1(FLog::Error, "On receive packet %d", (unsigned char) packet->data[0]);
				FASTLOGS(FLog::Error, "Error %s", e.what());
				StandardOut::singleton()->printf(MESSAGE_ERROR, "Server::OnReceive packet %d: %s", (unsigned char) packet->data[0], e.what());
				if (proxy)
				{
					// Disconnect
					proxy->unlockParent();
					proxy->setParent(NULL);
				}
			}
		}
		return RR_CONTINUE_PROCESSING;
	default:
		return RR_CONTINUE_PROCESSING;
	}
}

void Server::setAllowedSecurityVersions(const std::vector<std::string>& versions)
{
	boost::mutex::scoped_lock lock(securityVersionsMutex);
	allowedSecurityVersions = versions;
}

bool Server::isLegalReceiveInstance(Instance* instance) const
{
	return isScriptLegal(instance);
}

bool Server::isScriptLegal(Instance* instance) const
{
    boost::optional<std::string> scriptSource = getScriptSourceFromInstance(instance);
    
	return !scriptSource || scriptsBySource.find(*scriptSource) != scriptsBySource.end();
}

boost::optional<long> Server::getScriptIndexForSource(const std::string& source) const
{
    boost::unordered_map<std::string, long>::const_iterator it = scriptsBySource.find(source);

    if (it != scriptsBySource.end())
        return it->second;
    else
        return boost::optional<long>();
}

boost::optional<long> Server::getScriptIndexForBytecode(const std::string& bytecode, bool isCurrentFormat) const
{
    boost::unordered_map<std::string, long>::const_iterator it = isCurrentFormat ? scriptsByCurrentBytecode.find(bytecode)
        : scriptsByLegacyBytecode.find(bytecode);
    boost::unordered_map<std::string, long>::const_iterator end = isCurrentFormat ? scriptsByCurrentBytecode.end()
        : scriptsByLegacyBytecode.end();

    if (it != end)
        return it->second;
    else
        return boost::optional<long>();
}

boost::optional<ProtectedString> Server::getScriptSourceForIndex(long index) const
{
    if (index >= 0 && index < static_cast<long>(legalScripts.size()))
        return ProtectedString::fromTrustedSource(legalScripts[index].source);
    else
        return boost::optional<ProtectedString>();
}

boost::optional<std::string> Server::getScriptBytecodeForIndex(long index, bool isCurrentFormat) const
{
    if (index >= 0 && index < static_cast<long>(legalScripts.size()))
        return isCurrentFormat ? legalScripts[index].currentBytecode : legalScripts[index].legacyBytecode;
    else
        return boost::optional<std::string>();
}

void Server::registerLegalScript(const std::string& source)
{
    if (scriptsBySource.find(source) == scriptsBySource.end())
	{
        long index = legalScripts.size();

        legalScripts.push_back(LegalScript());
        auto& it = legalScripts.back();
        it.source = source;
        it.currentBytecode = LuaVM::compile(source);
        it.legacyBytecode = LuaVM::compileLegacy(source);
        scriptsBySource[source] = index;

        if (!it.currentBytecode.empty())
        {
            scriptsByCurrentBytecode[it.currentBytecode] = index;
            scriptsByLegacyBytecode[it.legacyBytecode] = index;
        }
	}
}

boost::optional<int> Server::getPlaceAuthenticationResultForOrigin(int originPlaceId)
{
	boost::mutex::scoped_lock lock(placeAuthenticationResultCacheMutex);

	boost::unordered_map<int, int>::const_iterator iter = placeAuthenticationResultCache.find(originPlaceId);

	if (iter != placeAuthenticationResultCache.end())
		return iter->second;
	else
		return boost::optional<int>();
}

void Server::registerPlaceAuthenticationResult(int originPlaceId, int result)
{
	boost::mutex::scoped_lock lock(placeAuthenticationResultCacheMutex);
	placeAuthenticationResultCache[originPlaceId] = result;
}

bool Server::securityKeyMatches(const std::string& key)
{
	if (FFlag::DebugLocalRccServerConnection)
	{
		return true;
	}

	if (isCloudEdit())
	{
		return true;
	}

	boost::mutex::scoped_lock lock(securityVersionsMutex);
	if (allowedSecurityVersions.empty())
		return true;

	for (unsigned int i = 0; i < allowedSecurityVersions.size(); i++)
	{
		if (key == allowedSecurityVersions[i])
			return true;
	}

	return false;
}

bool Server::protocolVersionMatches(int protocolVer)
{
    BOOST_STATIC_ASSERT(NETWORK_PROTOCOL_VERSION_MIN <= NETWORK_PROTOCOL_VERSION);

    return protocolVer >= NETWORK_PROTOCOL_VERSION_MIN && protocolVer <= NETWORK_PROTOCOL_VERSION;
}

NetworkOwnerJob* Server::getNetworkOwnerJob()
{
    if (networkOwnerJob)
    {
        return static_cast<NetworkOwnerJob*>(networkOwnerJob.get());
    }
    else
    {
        return NULL;
    }
}




