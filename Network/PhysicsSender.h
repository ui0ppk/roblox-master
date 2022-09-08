#pragma once

#include "Compressor.h"
#include "rbx/Declarations.h"
#include "Util/G3DCore.h"
#include "Util/PV.h"
#include "rbx/boost.hpp"
#include "rbx/signal.h"
#include "rbx/rbxTime.h"
#include <vector>
#include "CompactCFrame.h"
#include <boost/unordered_set.hpp>
#include "PacketPriority.h"
#include "rbx/RunningAverage.h"
#include "ReplicatorStats.h"

namespace RakNet {
	class BitStream;
}

namespace RBX { 
	
	class Assembly;
	class Primitive;
	struct TouchPair;
	class PartInstance;
    class MovementHistory;
	class PhysicsService;

	namespace Network {

	class Replicator;

	class RBXBaseClass PhysicsSender : boost::noncopyable
	{
	private:
		class Job;	// The Job that sends physics
		shared_ptr<Job> job;
		class TouchJob;	// The Job that sends Touches
		shared_ptr<TouchJob> touchJob;

		std::list<TouchPair> touchPairs;
		int touchPairsId;

	private:

		G3D::Array<CompactCFrame> tempMotorAngles;

		void writeMechanismAttributes(RakNet::BitStream& bitStream, const Assembly* assembly);

		void sendChildAssembly(RakNet::BitStream* bitStream, const Assembly* assembly, Compressor::CompressionType compressionType);
		void sendChildPrimitiveCoordinateFrame(Primitive *prim, RakNet::BitStream* bitStream, Replicator *replicator, Compressor::CompressionType compressionType);
		
		void writeCompactCFrame(RakNet::BitStream& bitStream, const CompactCFrame& cFrame, Compressor::CompressionType compressionType);
        void writeCoordinateFrame(RakNet::BitStream& bitStream, const CoordinateFrame& cFrame, Compressor::CompressionType compressionType);

		Compressor::CompressionType getCompressionType(const Assembly* assembly, bool detailed);

        void addChildRotationError(const Assembly* assembly, float& accumulatedError);

	protected:
		Replicator& replicator;
        ReplicatorStats::PhysicsSenderStats* senderStats;

		shared_ptr<PhysicsService> physicsService;

		RunningAverage<int> itemsPerPacket;
		int sendPacketsPerStep;

        Time currentStepTimestamp;

		bool canSend(const PartInstance* part, const Assembly* assembly, RakNet::BitStream& bitStream);
		bool sendPhysicsData(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, float& accumulatedError, const PartInstance* playerHead);
		
		void sendMechanism(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, float& accumulatedError, const PartInstance* playerHead);
		void sendMechanismCFrames(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed);
        void writeVelocity(RakNet::BitStream& bitStream, const Velocity& velocity);
        void writeMotorAngles(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType);

        virtual void writePV(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression);

		virtual void writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression = false);
        virtual bool writeMovementHistory(RakNet::BitStream& bitStream, const MovementHistory& history, const Assembly* assembly, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, bool& hasMovement, Compressor::CompressionType compressionType, float& accumulatedError, const PartInstance* playerHead);
		
	public:
		PhysicsSender(Replicator& replicator);
		static void start(shared_ptr<PhysicsSender> physicsSender);
		virtual ~PhysicsSender();

		virtual void step() = 0;
		void sendTouches(PacketPriority packetPriority);
		
		template <typename T> 
		void writeTouches(RakNet::BitStream& bitstream, unsigned int maxBitStreamSize, T& touchPairList);

		virtual int sendPacket(int maxPackets, PacketPriority packetPriority,
			ReplicatorStats::PhysicsSenderStats* stats) = 0;

		size_t pendingTouchCount();
	};

} }
