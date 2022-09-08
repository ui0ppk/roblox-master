#pragma once

#include "V8DataModel/DataModelJob.h"
#include "Util/SystemAddress.h"
#include "Util/G3DCore.h"
#include "Util/Region2.h"
#include <map>

namespace RBX { 

	class PartInstance;
	class Mechanism;
	class DataModel;

namespace Network {

	class Server;
	class ServerReplicator;

	class NetworkOwnerJob : public DataModelJob
	{
	private:
		boost::weak_ptr<DataModel> dataModel;
		float networkOwnerRate;

		class ClientLocation
		{
		public:
			Region2::WeightedPoint clientPoint;
			ServerReplicator* clientProxy;
			const Mechanism* characterMechanism;
            bool overloaded;

			ClientLocation(const Region2::WeightedPoint& clientPoint, ServerReplicator* clientProxy, const Mechanism* characterMechanism) 
				: clientPoint(clientPoint)
				, clientProxy(clientProxy)
				, characterMechanism(characterMechanism)
                , overloaded(false)
			{}
		};
		
		typedef std::map<RBX::SystemAddress, ClientLocation> ClientMap;
		typedef std::pair<RBX::SystemAddress, ClientLocation> ClientMapPair;
		typedef ClientMap::const_iterator ClientMapConstIt;
        typedef ClientMap::iterator ClientMapIt;

		ClientMap clientMap;

		void updatePlayerLocations(Server* server);
		void updateNetworkOwner(PartInstance* part);
		ClientMapConstIt findClosestClientToPart(PartInstance* part);
		ClientMapConstIt findClosestClient(const Vector2& testLocation);

		bool clientCanSimulate(PartInstance* part, ClientMapConstIt testLocation);
		bool isClientCharacterMechanism(PartInstance* part, ClientMapConstIt testLocation);
		bool switchOwners(PartInstance* part, ClientMapConstIt currentOwner, ClientMapConstIt possibleNewOwner);
		static void resetNetworkOwner(PartInstance* part, const RBX::SystemAddress value);

		// Job overrides
		/*override*/ virtual Error error(const Stats& stats);
		/*override*/ Time::Interval sleepTime(const Stats& stats);
		/*override*/ virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);


	public:
		NetworkOwnerJob(shared_ptr<DataModel> dataModel);
        void invalidateProjectileOwnership(RBX::SystemAddress addr);
	};
	
}
}

