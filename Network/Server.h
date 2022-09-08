/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Network/NetworkPacketCache.h"
#include "Network/Player.h"
#include "Peer.h"
#include "Replicator.h"

#include "Util/ProtectedString.h"
#include "Util/standardout.h"
#include "V8DataModel/PartInstance.h"
#include "V8Tree/Service.h"

#include "RakNet/Source/PluginInterface2.h"
#include <boost/bimap.hpp>
#include <boost/optional.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

namespace RBX { 
	namespace Network {

	class ServerReplicator;
    class NetworkOwnerJob;
	
	extern const char* const sServer;
	class Server 
		: public DescribedCreatable<Server, Peer, sServer, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	private:
		typedef DescribedCreatable<Server, Peer, sServer, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
		int outgoingPort;
		shared_ptr<Players> players;
		shared_ptr<TaskScheduler::Job> networkOwnerJob;
		bool isCloudEditServer;

		rbx::signals::scoped_connection itemAddedConnection;
		rbx::signals::scoped_connection workspaceLoadedConnection;
		static int lastId;

		bool isPlayerAuthenticationRequired;

		PhysicsPacketCache* physicsPacketCache;
		InstancePacketCache* instancePacketCache;

		NetworkSettings* networkSettings;

        struct LegalScript
        {
            std::string source;
            // cvx: Note that "bytecode" includes the strings tables, line info, etc...
            std::string currentBytecode;
            std::string legacyBytecode;
        };

        std::vector<LegalScript> legalScripts;
        boost::unordered_map<std::string, long> scriptsBySource;
        boost::unordered_map<std::string, long> scriptsByCurrentBytecode;
        boost::unordered_map<std::string, long> scriptsByLegacyBytecode;

		boost::mutex placeAuthenticationResultCacheMutex;
		boost::unordered_map<int, int> placeAuthenticationResultCache;

	public:
		static boost::function<shared_ptr<ServerReplicator>(RakNet::SystemAddress, Server*, NetworkSettings*)> createReplicator;	// factory

		std::set<std::string> usedTickets;
		std::set<std::string> preusedTickets;

		bool getIsPlayerAuthenticationRequired() const { return Network::isPlayerAuthenticationEnabled() && isPlayerAuthenticationRequired; }
		void setIsPlayerAuthenticationRequired(bool value) { isPlayerAuthenticationRequired = value; }

		Server();
		~Server();
		/*override*/ void onCreateRakPeer();

		rbx::signal<void(shared_ptr<Instance> peer, FilterResult result, shared_ptr<Instance> item, std::string member)> dataBasicFilteredSignal;
		rbx::signal<void(shared_ptr<Instance> peer, FilterResult result, shared_ptr<Instance> item, std::string member)> dataCustomFilteredSignal;
		rbx::signal<void(std::string, shared_ptr<Instance>)> incommingConnectionSignal;

		static bool serverIsPresent(const Instance* context, bool testInDatamodel = true);

		ServerReplicator* findClientOwner(const Vector3& p);

		void start(int port, int threadSleepTime);
		void stop(int blockDuration = 1000);
		void configureAsCloudEditServer();
		int getClientCount();
		int getPort() const { return outgoingPort; }
		bool securityKeyMatches(const std::string& key);
		bool protocolVersionMatches(int protocolVersion);

        NetworkOwnerJob* getNetworkOwnerJob();

		static void setAllowedSecurityVersions(const std::vector<std::string>& versions);
        
        static std::vector<std::string> getAllIPv4Addresses();

		// script management
		bool isLegalReceiveInstance(Instance* instance) const;
		bool isScriptLegal(Instance* instance) const;
        
		boost::optional<long> getScriptIndexForSource(const std::string& source) const;
		boost::optional<long> getScriptIndexForBytecode(const std::string& bytecode, bool isCurrentFormat = true) const;

        boost::optional<ProtectedString> getScriptSourceForIndex(long index) const;
        boost::optional<std::string> getScriptBytecodeForIndex(long index, bool isCurrentFormat = true) const;

		boost::optional<int> getPlaceAuthenticationResultForOrigin(int originPlaceId);
		void registerPlaceAuthenticationResult(int originPlaceId, int result);

		/*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet *packet);

		bool isCloudEdit() const { return isCloudEditServer; }

	protected:
		virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		bool askAddChild(const Instance* instance) const;

	private:
		void onItemAdded(shared_ptr<Instance> item);
		void onWorkspaceLoaded();

		boost::optional<std::string> getScriptSourceFromInstance(Instance* instance) const;

        void registerLegalScript(const std::string& source);
	};


} }
