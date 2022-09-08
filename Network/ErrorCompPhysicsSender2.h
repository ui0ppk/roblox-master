#if 1 // removing deprecated senders, set to 0 when FastFlag RemoveUnusedPhysicsSenders is set to true and removed (delete this file)

#pragma once

#include "PhysicsSender.h"
#include "BoostAppend.h"
#include "V8DataModel/PartInstance.h"
#include "rbx/signal.h"

#include "boost/pool/pool_alloc.hpp"

namespace RBX { 

	class Instance;
	class PhysicsService;
	class ModelInstance;

	namespace Network {

		class Replicator;
		class PhysicsPacketCache;

		class ErrorCompPhysicsSender2 : public PhysicsSender
		{
			class Nugget;

			typedef std::list<Nugget> NuggetList;
			typedef NuggetList::iterator NuggetIterator;
			typedef std::list<NuggetIterator> NuggetUpdateList;
			typedef boost::unordered_map<shared_ptr<const PartInstance>,  NuggetUpdateList::iterator> NuggetMap;

			class Nugget
			{
			public:

				shared_ptr<const PartInstance> part;
				int errorStepId;				    // "stepId" of the last error calculation  (-1 means uninitialized. 0 means we just got sent and need updating)
				int sendStepId;						// "stepId" of the last send
				int groupId;						// which bucket this item belong to
				NuggetMap::iterator mapIter;		// iterator into hash map, for fast update without having to do look up

				Nugget(shared_ptr<PartInstance> part);
				~Nugget();

				static void onSent(Nugget& nugget);
				void onSent(int stepId);

				void computeDeltaError(const CoordinateFrame& focus, const ModelInstance* focusModel, int timestamp);

				bool sendDetailed;			// Should this send detailed (uncompressed) physics info
				float biggestSize;			// May change for an articulated assembly....

				static float minDistance();
				static float maxDistance();
				static float minSize();
				static float maxSize();
				static float midArea();
			};

			struct Bucket
			{
				NuggetIterator iter;
				NuggetList nuggetList;

				int targetSentCount;
				int sendCount;

				Bucket() : targetSentCount(0), sendCount(0) {}

				NuggetIterator push_back(shared_ptr<PartInstance> part);
				void erase(const NuggetIterator where);

				// moves a node from one bucket to another.
				// if first == iter, iter gets moved to next node if not at end, otherwise it moves to beginning
				NuggetIterator splice(const NuggetIterator where, Bucket* from, const NuggetIterator first);

				inline unsigned int size() { return nuggetList.size(); }

			};

			typedef std::vector<Bucket> NuggetSendGroups;

			NuggetMap nuggetMap;
			NuggetUpdateList nuggetUpdateList;
			NuggetSendGroups nuggetSendGroups;
			unsigned int bucketIndex;
	
			int stepId;
			shared_ptr<PhysicsService> physicsService;
			shared_ptr<PhysicsPacketCache> physicsPacketCache;

			bool sendDetailed;

			rbx::signals::scoped_connection addingAssemblyConnection;
			rbx::signals::scoped_connection removedAssemblyConnection;

			typedef std::list<shared_ptr<PartInstance>, boost::fast_pool_allocator<shared_ptr<PartInstance> > > NewMovingAssemblies;
			NewMovingAssemblies newMovingAssemblies;	// new moving assemblies for current step

		public:
			ErrorCompPhysicsSender2(Replicator& replicator);
			~ErrorCompPhysicsSender2();

			virtual void step();
			virtual int sendPacket(int maxPackets, PacketPriority packetPriority,
				ReplicatorStats::PhysicsSenderStats* stats);

		protected:
			virtual void writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression = false);

		private:

			void calculateSendCount();

			bool sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget);

			void addNugget(PartInstance& part);
			void addNugget2(shared_ptr<PartInstance> part);
			void removeNugget(shared_ptr<const PartInstance> part);

			void onAddingAssembly(shared_ptr<Instance> assembly);
			void onRemovedAssembly(shared_ptr<Instance> assembly);

			bool writeNugget(RakNet::BitStream& bitStream, Nugget& nugget);
		};


	} }

#endif // removing deprecated senders
