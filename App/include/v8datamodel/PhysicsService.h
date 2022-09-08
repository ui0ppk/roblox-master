#pragma once

#include "V8Tree/Service.h"
#include "V8World/SendPhysics.h"
#include "Util/ConcurrencyValidator.h"
#include "RBX/Intrusive/Set.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"

namespace RBX {

	extern const char *const sPhysicsService;

	class PhysicsService 
		: public DescribedNonCreatable<PhysicsService, Instance, sPhysicsService>
		, public Service
		, public HeartbeatInstance
	{
	private:
		typedef DescribedNonCreatable<PhysicsService, Instance, sPhysicsService> Super;

	public:
		typedef RBX::Intrusive::Set< PartInstance, PhysicsService > Parts;
		typedef Parts::Iterator PartsIt;

	private:

        bool iAmServer;

		Parts parts;

		typedef boost::unordered_set<TouchPair> PartPairs;
		PartPairs touchesSendList;		// used by physics senders, swaps with receive list
		PartPairs touchesReceiveList;	// from physics receiver and world
		rbx::signals::scoped_connection touchesConnection;
		rbx::atomic<int> touchSentCounter;
		int touchResetCount;
		int touchSendListId;

		// used to determine number of physics senders
		rbx::signals::connection playersChangedConnection;

		ConcurrencyValidator concurrencyValidator;

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		rbx::signals::connection assemblyPhysicsOnConnection;
		rbx::signals::connection assemblyPhysicsOffConnection;
		void onAssemblyPhysicsOn(Primitive* primitive);		// added by engine
		void onAssemblyPhysicsOff(Primitive* primitive);

	public:
		PhysicsService() : touchSendListId(0), touchResetCount(1)
		{
			iAmServer = false;
			setName("PhysicsService");
		}
		~PhysicsService();

		// Interface
		rbx::signal<void(shared_ptr<Instance>)> assemblyAddingSignal;				// state change scripts			
		rbx::signal<void(shared_ptr<Instance>)> assemblyRemovedSignal;			// state change scripts

		int numSenders() {
			return parts.size();
		}

		PartsIt begin() {
			return parts.begin();
		}

		PartsIt end() {
			return parts.end();
		}

		/*override*/ void onHeartbeat(const Heartbeat& event);
 
		void onTouchStep(const TouchPair& other);
		size_t pendingTouchCount() { return touchesSendList.size(); }
		int getTouchesId() { return touchSendListId; }
		void getTouches(std::list<TouchPair>& out);
		void onTouchesSent();

		void onPlayersChanged(Instance::CombinedSignalType type, const ICombinedSignalData* data);
	};

} // namespace
