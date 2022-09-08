
#if 1 // removing deprecated senders, set to 0 when FastFlag RemoveUnusedPhysicsSenders is set to true and removed (delete this file)
#pragma once

#include "PhysicsSender.h"
#include "BoostAppend.h"

#include "V8DataModel/PartInstance.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include "rbx/signal.h"

#include "boost/pool/pool_alloc.hpp"
#include "boost/intrusive/set.hpp"
using boost::multi_index_container;
using namespace boost::multi_index;


namespace RBX { 
	
	class Instance;
	class PhysicsService;
	class ModelInstance;

	namespace Network {

	class Replicator;
	class PhysicsPacketCache;

	class ErrorCompPhysicsSender : public PhysicsSender
	{

		class Nugget;

		// custom containers
		typedef std::list<shared_ptr<PartInstance>, boost::fast_pool_allocator<shared_ptr<PartInstance> > > NewMovingAssemblies;
		NewMovingAssemblies newMovingAssemblies;	// new moving assemblies for current step

		typedef boost::intrusive::set_base_hook<boost::intrusive::tag<Nugget>, boost::intrusive::link_mode<boost::intrusive::normal_link> > NuggetHook;
		typedef boost::intrusive::multiset<Nugget, boost::intrusive::compare<std::greater<Nugget> >, boost::intrusive::base_hook<NuggetHook> > SortedNuggetList;

		typedef std::list<SortedNuggetList::iterator> NuggetList;
		typedef boost::unordered_map<shared_ptr<const PartInstance>,  Nugget> NuggetMap;

		NuggetMap nuggetMap;
		NuggetList nuggetUpdateList;
		SortedNuggetList nuggetSendList;

		class Nugget : public NuggetHook
		{
		public:

			shared_ptr<const PartInstance> part;
			double error;
			int errorStepId;				     // "stepId" of the last send  (-1 means uninitialized. 0 means we just got sent and need updating)

			Nugget(shared_ptr<PartInstance> part);

			static void onSent(Nugget& nugget);

			void computeError(const CoordinateFrame& focus, const ModelInstance* focusModel, int timestamp);

			CoordinateFrame lastSent;	// the CF last sent
			bool sendDetailed;			// Should this send detailed (uncompressed) physics info
			float biggestSize;			// May change for an articulated assembly....
			float radius;				// cached assembly radius

			static float minDistance();
			static float maxDistance();
			static float minSize();
			static float maxSize();

			friend bool operator > (const Nugget &a, const Nugget& b)
			{
				return a.error > b.error;
			}

			NuggetList::iterator updateListIter; // iterator into update list
		};

		int stepId;
		shared_ptr<PhysicsService> physicsService;
		shared_ptr<PhysicsPacketCache> physicsPacketCache;

		bool sendDetailed;

		rbx::signals::scoped_connection addingAssemblyConnection;
		rbx::signals::scoped_connection removedAssemblyConnection;

	public:
		ErrorCompPhysicsSender(Replicator& replicator);
		~ErrorCompPhysicsSender();

		virtual void step();
		virtual int sendPacket(int maxPackets, PacketPriority packetPriority,
			ReplicatorStats::PhysicsSenderStats* stats);
	
	protected:
		virtual void writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression = false);

	private:
		void updateErrors();
		bool sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget);

		void addNugget(PartInstance& part);
		void addNugget2(shared_ptr<PartInstance> part);

		void onAddingAssembly(shared_ptr<Instance> assembly);
		void onRemovedAssembly(shared_ptr<Instance> assembly);
		void removeNugget(shared_ptr<const PartInstance> part);
	};


} }

#endif  // removing deprecated senders