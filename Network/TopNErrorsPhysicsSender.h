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

	class TopNErrorsPhysicsSender : public PhysicsSender
	{
		class Nugget;

		typedef std::list<shared_ptr<PartInstance>, boost::fast_pool_allocator<shared_ptr<PartInstance> > > Assemblies;
		typedef boost::unordered_map<shared_ptr<const PartInstance>,  Nugget> NuggetMap;
		typedef std::vector<Nugget*> NuggetList;

		Assemblies newMovingAssemblies;				// new moving assemblies for current step
		NuggetMap nuggetMap;						// map holding nuggets
		NuggetList nuggetList;						// vector holding iterator into map

		class Nugget
		{
		public:

			shared_ptr<const PartInstance> part;
			float error;

			Nugget(shared_ptr<PartInstance> part);

			void onSent(const Time& t, const CoordinateFrame& cf, float accumulatedError);
			void computeError(const CoordinateFrame& focus, const ModelInstance* focusModel, int timestamp, const TopNErrorsPhysicsSender* sender);

			CoordinateFrame lastSent;	// the CF last sent
			bool sendDetailed;			// Should this send detailed (uncompressed) physics info
			bool notInService;
			float biggestSize;			// May change for an articulated assembly....
			float radius;				// cached assembly radius

            Time lastSendTime;
            float accumulatedError;

			static float minDistance();
			static float maxDistance();
			static float minSize();
			static float maxSize();

			static bool compError(const Nugget* a, const Nugget* b)
			{
				return a->error > b->error;
			}
		};
		
		shared_ptr<PhysicsService> physicsService;
		shared_ptr<PhysicsPacketCache> physicsPacketCache;
		int stepId;
		int sendId;
		int sortId;
		bool sendDetailed;

		rbx::signals::scoped_connection addingAssemblyConnection;
		rbx::signals::scoped_connection removedAssemblyConnection;

	public:
		TopNErrorsPhysicsSender(Replicator& replicator);
		~TopNErrorsPhysicsSender();

		virtual void step();
		virtual int sendPacket(int maxPackets, PacketPriority packetPriority,
			ReplicatorStats::PhysicsSenderStats* stats);
	
	protected:
        virtual void writePV(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression);

		virtual void writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression = false);
        virtual bool writeMovementHistory(RakNet::BitStream& bitStream, const MovementHistory& history, const Assembly* assembly, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, bool& hasMovement, Compressor::CompressionType compressionType, float& accumulatedError, const PartInstance* playerHead);

	private:
		void updateErrors();
		bool sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget, CoordinateFrame& outLastSendCFrame, float& accumulatedError, const PartInstance* playerHead);

		void addNugget(PartInstance& part);
		void addNugget2(shared_ptr<PartInstance> part);

		void onAddingAssembly(shared_ptr<Instance> assembly);
		void onRemovedAssembly(shared_ptr<Instance> assembly);

		void removeNugget(shared_ptr<const PartInstance> part);
		bool isSleepingRootPrimitive(const PartInstance* part);
	};

}
} // namespace RBX

