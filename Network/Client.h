/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Peer.h"
#include "Util/SystemAddress.h"


namespace RBX { 

	namespace Network {

		extern const char* const sClient;
		class Client 
			: public DescribedCreatable<Client, Peer, sClient, Reflection::ClassDescriptor::INTERNAL_LOCAL>
			, public Service
		{
		private:
			typedef DescribedCreatable<Client, Peer, sClient, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

			rbx::signals::scoped_connection closingConnection;
			RakNet::SystemAddress serverId;
            int userId;
			std::string ticket;
			NetworkSettings* networkSettings;

			static Reflection::BoundProp<std::string> prop_Ticket;
		public:
			Client();
			~Client();

			rbx::signal<void(std::string, shared_ptr<Instance>)> connectionAcceptedSignal;
			rbx::signal<void(std::string, int, std::string)> connectionFailedSignal;
			rbx::signal<void(std::string)> connectionRejectedSignal;

			void setTicket(const std::string& t) { ticket = t; }

			shared_ptr<Instance> playerConnect(int userId, std::string server, int serverPort, int clientPort, int threadSleepTime);
			void disconnect(int blockDuration);
			void disconnect() { disconnect(3000); }
			void setGameSessionID(std::string gameSessionID);
			void configureAsCloudEditClient();
			bool isCloudEdit() const;
			
			/*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet *packet);
			/*override*/ void OnFailedConnectionAttempt(RakNet::Packet *packet, RakNet::PI2_FailedConnectionAttemptReason failedConnectionAttemptReason);
			
			static Client* findClient(const RBX::Instance* context, bool testInDatamodel = true);
			static bool clientIsPresent(const RBX::Instance* context, bool testInDatamodel = true);
			static const RBX::SystemAddress findLocalSimulatorAddress(const RBX::Instance* context);			// if Client == clientAddress, else == NULL;
			static bool physicsOutBandwidthExceeded(const RBX::Instance* context);
			static double getNetworkBufferHealth(const RBX::Instance* context);

		protected:
			virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		private:
			bool isCloudEditClient;
            void sendVersionInfo();
			void sendTicket();
			void sendPreferedSpawnName() const;
            void HandleConnection(RakNet::Packet *packet);
		};
	}
}
