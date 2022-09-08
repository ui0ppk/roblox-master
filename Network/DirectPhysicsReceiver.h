#pragma once

#include "PhysicsReceiver.h"
#include "ReplicatorStats.h"

namespace RBX { 

	namespace Network {

	class DirectPhysicsReceiver : public PhysicsReceiver
	{
	private:
		MechanismItem tempItem;

	public:
		DirectPhysicsReceiver(Replicator* replicator, bool isServer): PhysicsReceiver(replicator, isServer) {}
		virtual void receivePacket(RakNet::BitStream& bitsream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* stats);
	};

} }