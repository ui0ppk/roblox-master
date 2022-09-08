/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "PhysicsSender.h"
#include "Replicator.h"
#include "Compressor.h"
#include "RakNetStatistics.h"
#include "ConcurrentRakPeer.h"
#include "Util.h"

#include "util/profiling.h"
#include "util/ObscureValue.h"

#include "V8Kernel/Constants.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"

#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8DataModel/DataModel.h"
#include "RoundRobinPhysicsSender.h"
#include "V8DataModel/PhysicsService.h"

using namespace RBX;
using namespace RBX::Network;

FASTINTVARIABLE(NumPhysicsTouchPacketsPerStep, 1)
DYNAMIC_FASTINTVARIABLE(NumPhysicsPacketsPerStep, 1)
DYNAMIC_FASTINTVARIABLE(PhysicsSenderRate, 15)
DYNAMIC_FASTFLAG(CleanUpInterpolationTimestamps)

class PhysicsSender::Job : public ReplicatorJob
{
	weak_ptr<PhysicsSender> physicsSender;
	ObscureValue<float> physicsSendRate;
	PacketPriority packetPriority;
	unsigned int skipSteps;

public:
	Job(shared_ptr<PhysicsSender> physicsSender)
		:ReplicatorJob("Replicator SendPhysics", physicsSender->replicator, DataModelJob::PhysicsOut) 
		,physicsSender(physicsSender)
		,physicsSendRate(replicator->settings().getPhysicsSendRate())
		,packetPriority(replicator->settings().getPhysicsSendPriority())
		,skipSteps(0)
	{
        if (dynamic_cast<RoundRobinPhysicsSender*>(physicsSender.get()))
        {
            physicsSendRate = replicator->settings().getClientPhysicsSendRate();
        }

		// TODO: set as default in NetworkSettings
		physicsSendRate = (float)DFInt::PhysicsSenderRate;

		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessOutgoing;
	}

private:

	Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, physicsSendRate);
	}

	virtual Error error(const Stats& stats)
	{
		if (TaskScheduler::singleton().isCyclicExecutive() && DFFlag::CleanUpInterpolationTimestamps)
		{
			return 1.0f; // ALWAYS RUN
		}
		return computeStandardErrorCyclicExecutiveSleeping(stats, physicsSendRate);
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if (DFFlag::CleanUpInterpolationTimestamps)
		{
			int stepsTillRun = std::max(Constants::uiStepsPerSec()/DFInt::PhysicsSenderRate, 1);
			if (skipSteps != (stepsTillRun - 1))
			{
				skipSteps++;
				return TaskScheduler::Stepped;
			}
			else
			{
				skipSteps = 0;
			}
		}
		
		if(shared_ptr<PhysicsSender> safePhysicsSender = physicsSender.lock())
		{
			safePhysicsSender->step();
            ReplicatorStats::PhysicsSenderStats& physStats =
                safePhysicsSender->replicator.physicsSenderStats();

			const int limit = replicator->settings().sendPacketBufferLimit;
			if (limit == -1)
			{
				safePhysicsSender->sendPacket(safePhysicsSender->sendPacketsPerStep, packetPriority, &physStats);
				physStats.physicsSentThrottle.sample(0.0); // not throttled
			}
			else if (replicator)
			{
				int bufferCount = replicator->getBufferCountAvailable(limit, packetPriority);
				safePhysicsSender->sendPacket(bufferCount, packetPriority, &physStats);
				physStats.physicsSentThrottle.sample(1.0 - bufferCount / limit);
			}

			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};


class PhysicsSender::TouchJob : public ReplicatorJob
{
	weak_ptr<PhysicsSender> physicsSender;
	float sendRate;
	PacketPriority packetPriority;
public:
	TouchJob(shared_ptr<PhysicsSender> physicsSender)
		:ReplicatorJob("Replicator SendTouches", physicsSender->replicator, DataModelJob::PhysicsOut) 
		,physicsSender(physicsSender)
		,sendRate(replicator->settings().touchSendRate)
		,packetPriority(replicator->settings().getPhysicsSendPriority())
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessOutgoing;
	}

private:

	Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, sendRate);
	}

	virtual Error error(const Stats& stats)
	{
        if (canSendPacket(replicator, packetPriority))
			if(shared_ptr<PhysicsSender> safePhysicsSender = physicsSender.lock())
				if (safePhysicsSender->pendingTouchCount() > 0)
					return computeStandardErrorCyclicExecutiveSleeping(stats, sendRate);
		Error error;
		return error;
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if(shared_ptr<PhysicsSender> safePhysicsSender = physicsSender.lock()){
			safePhysicsSender->sendTouches(packetPriority);
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};

size_t PhysicsSender::pendingTouchCount()
{ 
	return physicsService->pendingTouchCount();
}

template<typename T> 
void PhysicsSender::writeTouches(RakNet::BitStream& bitStream, unsigned int maxBitStreamSize, T& touchPairList)
{
	RBX::SystemAddress addr = RakNetToRbxAddress(replicator.remotePlayerId);
	typename T::iterator iter;
	for (iter = touchPairList.begin(); iter != touchPairList.end(); ++iter)
	{
		if (iter->originator == addr)
			continue;

		const bool touched = iter->type == TouchPair::Touch;

		int byteStart = bitStream.GetNumberOfBytesUsed();

		replicator.serializeId(bitStream, iter->p1.get());
		replicator.serializeId(bitStream, iter->p2.get());
		bitStream << touched;

		if (replicator.settings().printTouches) {
			if (touched) {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
					"Replication: Touch:%s->%s >> %s, bytes: %d", 
					iter->p1->getName().c_str(), 
					iter->p2->getName().c_str(), 
					RakNetAddressToString(replicator.remotePlayerId).c_str(),
					bitStream.GetNumberOfBytesUsed()-byteStart
					);
			} else {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
					"Replication: Untouch:%s->%s >> %s, bytes: %d", 
					iter->p1->getName().c_str(), 
					iter->p2->getName().c_str(), 
					RakNetAddressToString(replicator.remotePlayerId).c_str() ,
					bitStream.GetNumberOfBytesUsed()-byteStart
					);
			}
		}

		if (bitStream.GetNumberOfBytesUsed() > maxBitStreamSize)
			break;
	}

	if (iter == touchPairList.end())	
		touchPairList.clear();
	else
		touchPairList.erase(touchPairList.begin(), iter);
}


void PhysicsSender::sendTouches(PacketPriority packetPriority)
{
	const unsigned int maxStreamSize(replicator.getPhysicsMtuSize());
	int numPackets = FInt::NumPhysicsTouchPacketsPerStep;

	// append new touches to send list
	int newTouchId = physicsService->getTouchesId();
	if (touchPairsId < newTouchId)
	{
		physicsService->getTouches(touchPairs);
		touchPairsId = newTouchId;
	}

	ReplicatorStats::PhysicsSenderStats& physStats = replicator.physicsSenderStats();
	physStats.waitingTouches.sample(touchPairs.size());

	if (touchPairs.empty())
		return;

	while (!touchPairs.empty() && (numPackets > 0))
	{
		boost::shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream(maxStreamSize));

		*bitStream << (unsigned char) ID_PHYSICS_TOUCHES;

		writeTouches(*bitStream, maxStreamSize, touchPairs);

		// Packet "end" tag
		replicator.serializeId(*bitStream, NULL);

		// Send ID_PHYSICS_TOUCHES
		replicator.rakPeer->Send(bitStream, packetPriority, RELIABLE_ORDERED, PHYSICS_CHANNEL, replicator.remotePlayerId, false);

		physStats.touchPacketsSent.sample();
		physStats.touchPacketsSentSize.sample(bitStream->GetNumberOfBytesUsed());
		numPackets--;

		if (touchPairs.empty())
			physicsService->onTouchesSent();
	}
};

PhysicsSender::PhysicsSender(Replicator& replicator) : replicator(replicator), sendPacketsPerStep(DFInt::NumPhysicsPacketsPerStep), senderStats(NULL), touchPairsId(0)
{

	physicsService = shared_from(ServiceProvider::find<PhysicsService>(&replicator));
};

void PhysicsSender::start(shared_ptr<PhysicsSender> physicsSender) 
{
	//Someone else is holding a shared_ptr to us at the moment
	physicsSender->job = shared_ptr<Job>(new Job(physicsSender));
	TaskScheduler::singleton().add(physicsSender->job);

	physicsSender->touchJob = shared_ptr<TouchJob>(new TouchJob(physicsSender));
	TaskScheduler::singleton().add(physicsSender->touchJob);
}

PhysicsSender::~PhysicsSender()
{
	TaskScheduler::singleton().remove(job);
	job.reset();
	TaskScheduler::singleton().remove(touchJob);
	touchJob.reset();
}

/*
	Format for a mechanism:

	>> MechanismAttributes
	>> PrimaryAssembly
	>> done
	while (!done)
	{
		>> ChildPart
		>> ChildAssembly
		>> done
	}
*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

Compressor::CompressionType PhysicsSender::getCompressionType(const Assembly* assembly, bool detailed)
{
	if (replicator.settings().distributedPhysicsEnabled &&
			Mechanism::isComplexMovingMechanism(assembly)) {
		return Compressor::UNCOMPRESSED;
	}
	else if (detailed) {
		return Compressor::RAKNET_COMPRESSED;
	}
	else {
		return Compressor::HEAVILY_COMPRESSED;
	}
}

void PhysicsSender::sendChildPrimitiveCoordinateFrame(Primitive *prim, RakNet::BitStream* bitStream, Replicator *replicator, Compressor::CompressionType compressionType)
{
	RBXASSERT(replicator->isStreamingEnabled());

    if (PartInstance* part = PartInstance::fromPrimitive(prim))
    {
        if (replicator->isReplicationContainer(part))
        {
            // only update the client if the part is replicated
            RakNet::BitStream& bitStreamRef = *bitStream;
            if (replicator->trySerializeId(bitStreamRef, part))
				writeCoordinateFrame(bitStreamRef, prim->getCoordinateFrame(), compressionType);
        }
    }
}

void PhysicsSender::sendMechanismCFrames(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed)
{
	RBXASSERT(replicator.isStreamingEnabled());

    int byteStart = bitStream.GetNumberOfBytesUsed();

	const Primitive* primitive = part->getConstPartPrimitive();
    const Mechanism* mechanism = Mechanism::getConstPrimitiveMechanism(primitive);
    const Assembly* assembly = primitive->getConstAssembly();

	RBXASSERT(mechanism);

	Compressor::CompressionType compressionType = getCompressionType(assembly, detailed);

	if (replicator.isReplicationContainer(part))
	{
		if (replicator.trySerializeId(bitStream, part))
			writeCoordinateFrame(bitStream, part->getCoordinateFrame(), compressionType);
	}

    // send each part's cframe if they are in streamed region
    const_cast<Mechanism*>(mechanism)->visitPrimitives(boost::bind(&PhysicsSender::sendChildPrimitiveCoordinateFrame, this, _1, &bitStream, &replicator, compressionType));

	replicator.serializeId(bitStream, NULL); // token: done

    if (senderStats)
    {
        senderStats->details.mechanismCFrame.increment();
        senderStats->details.mechanismCFrameSize.sample(bitStream.GetNumberOfBytesUsed() - byteStart);
    }
}

void PhysicsSender::sendMechanism(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, float& accumulatedError, const PartInstance* playerHead)
{
    int byteStart = bitStream.GetNumberOfBytesUsed();

	const Assembly* assembly = part->getConstPartPrimitive()->getConstAssembly();

	RBXASSERT(assembly);
	//RBXASSERT(Mechanism::isMovingAssemblyRoot(assembly));

	Compressor::CompressionType compressionType = getCompressionType(assembly, detailed);

	writeMechanismAttributes(bitStream, assembly);

    bool hasMovement = true;
    if (!writeMovementHistory(bitStream, part->getMovementHistory(), assembly, lastSendTime, lastSendCFrame, hasMovement, compressionType, accumulatedError, playerHead))
    {
        writeAssembly(bitStream, assembly, compressionType);
        lastSendCFrame = part->getCoordinateFrame();
    }

    if (hasMovement)
    {
	    assembly->visitConstDescendentAssemblies(boost::bind(&PhysicsSender::sendChildAssembly, this, &bitStream, _1, compressionType));
    }
    else if (compressionType == Compressor::UNCOMPRESSED)
    {
        assembly->visitConstDescendentAssemblies(boost::bind(&PhysicsSender::addChildRotationError, this, _1, boost::ref(accumulatedError)));
    }

	bitStream << true;		// token - done with child assemblies

    if (senderStats)
    {
        senderStats->details.mechanism.increment();
        senderStats->details.mechanismSize.sample(bitStream.GetNumberOfBytesUsed() - byteStart);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool PhysicsSender::writeMovementHistory(RakNet::BitStream& bitStream, const MovementHistory& history, const Assembly* assembly, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, bool& hasMovement, Compressor::CompressionType compressionType, float& accumulatedError, const PartInstance* playerHead)
{
    hasMovement = true;
    return false; // client will not use path based part movement
}

void PhysicsSender::writeMechanismAttributes(RakNet::BitStream& bitStream, const Assembly* assembly)
{
	bool hasState = (assembly->getNetworkHumanoidState() != 0);
	bitStream << hasState;
	if (hasState) {
		bitStream << assembly->getNetworkHumanoidState();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicsSender::sendChildAssembly(RakNet::BitStream* bitStream, const Assembly* assembly, Compressor::CompressionType compressionType)
{
	RakNet::BitStream& bitStreamRef = *bitStream;
	const PartInstance* part = PartInstance::fromConstAssembly(assembly);
	RBXASSERT(part);

	// send only if we can serialize the part id
	if (replicator.canSerializeId(part))
	{
		bitStreamRef << false;		// token - not done with child assemblies
		replicator.serializeId(bitStreamRef, part);
		writeAssembly(bitStreamRef, assembly, compressionType);
	}

}

void PhysicsSender::addChildRotationError(const Assembly* assembly, float& accumulatedError)
{
    const PartInstance* part = PartInstance::fromConstAssembly(assembly);
    RBXASSERT(part);
    accumulatedError += part->getRotationalVelocity().magnitude();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicsSender::writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
    writePV(bitStream, assembly, compressionType, crossPacketCompression);
	writeMotorAngles(bitStream, assembly, compressionType);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
void PhysicsSender::writePV(RakNet::BitStream& bitStream, const Assembly* a, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
    const PV& pv = a->getConstAssemblyPrimitive()->getPV();
    writeCoordinateFrame(bitStream, pv.position, compressionType);
    writeVelocity(bitStream, pv.velocity);
}

void PhysicsSender::writeCoordinateFrame(RakNet::BitStream& bitStream, const CoordinateFrame& cFrame, Compressor::CompressionType compressionType)
{
    int byteStart = bitStream.GetNumberOfBytesUsed();
	Compressor::writeTranslation(bitStream, cFrame.translation, compressionType);
    if (senderStats)
    {
        senderStats->details.translation.increment();
        senderStats->details.translationSize.sample(bitStream.GetNumberOfBytesUsed() - byteStart);
    }
    byteStart = bitStream.GetNumberOfBytesUsed();
    Compressor::writeRotation(bitStream, cFrame.rotation, compressionType);
    if (senderStats)
    {
        senderStats->details.rotation.increment();
        senderStats->details.rotationSize.sample(bitStream.GetNumberOfBytesUsed() - byteStart);
    }
}

void PhysicsSender::writeVelocity(RakNet::BitStream& bitStream, const Velocity& velocity)
{
	if (replicator.settings().distributedPhysicsEnabled)
	{
        int byteStart = bitStream.GetNumberOfBytesUsed();

        bitStream.WriteVector(velocity.linear.x, velocity.linear.y, velocity.linear.z);
        bitStream.WriteVector(velocity.rotational.x, velocity.rotational.y, velocity.rotational.z); 

        if (senderStats)
        {
            senderStats->details.velocity.increment();
            senderStats->details.velocitySize.sample(bitStream.GetNumberOfBytesUsed() - byteStart);
        }
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicsSender::writeMotorAngles(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType)
{
	tempMotorAngles.resize(0);
	assembly->getPhysics(tempMotorAngles);

	RBXASSERT(tempMotorAngles.size() < 255);

	unsigned char compactNum = static_cast<unsigned char>(tempMotorAngles.size());
	bitStream << compactNum;

	for (int i = 0; i < tempMotorAngles.size(); ++i)
	{
		writeCompactCFrame(bitStream, tempMotorAngles[i], compressionType);
	}
}

void PhysicsSender::writeCompactCFrame(RakNet::BitStream& bitStream, const CompactCFrame& cFrame, Compressor::CompressionType compressionType)
{
	bool hasTranslation = !cFrame.translation.isZero();
	bool isSimpleZAngle = G3D::fuzzyEq(fabs(cFrame.getAxis().z), 1.0f);
	bool hasRotation = !G3D::fuzzyEq(cFrame.getAngle(), 0.0f);

	// common simple surface normal motor
	if(!hasTranslation && hasRotation && isSimpleZAngle) 
	{
		bitStream.Write1();
		unsigned char byteAngle = Math::rotationToByte(cFrame.getAxis().z * cFrame.getAngle());
		bitStream << byteAngle;
	}
	else
	{
		bitStream.Write0();
		// two bits, has trans, and hasrot
		bitStream << hasTranslation;
		bitStream << hasRotation;

		if(hasTranslation)
		{
			Compressor::writeTranslation(bitStream, cFrame.translation, compressionType);
		}
		if(hasRotation)
		{
			Vector3 axis = cFrame.getAxis();
            bitStream.WriteVector(axis.x, axis.y, axis.z); // would be nice to use WriteNormVector, but it's broken. (gives out NANs in certain situations, it needs fixing)

			unsigned char byteAngle = Math::rotationToByte(cFrame.getAngle()); // todo: losing one bit here, angle is always positive.
			bitStream << byteAngle;
		}
	}

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool PhysicsSender::canSend(const PartInstance* part, const Assembly* assembly, RakNet::BitStream& bitStream) 
{
	RBXASSERT(	!assembly	||	!part	||	(assembly->getConstAssemblyPrimitive() == part->getConstPartPrimitive())	);

	if (assembly && part)
	{
		if (!replicator.settings().distributedPhysicsEnabled || replicator.checkDistributedSend(part))
		{
			if (replicator.isStreamingEnabled())
				return true;
			else if (!replicator.isSerializePending(part))
				return true;
		}
	}
	return false;
}

bool PhysicsSender::sendPhysicsData(RakNet::BitStream& bitStream, const PartInstance* part, bool detailed, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, float& accumulatedError, const PartInstance* playerHead)
{
	RBXASSERT(part);
	if (part && Assembly::isAssemblyRootPrimitive(part->getConstPartPrimitive()))
	{
        senderStats = NULL;
        if (replicator.settings().trackPhysicsDetails)
        {
            senderStats = &(replicator.replicatorStats.physicsSenderStats);
        }

		const Assembly* assembly = part->getConstPartPrimitive()->getConstAssembly();
		if (canSend(part, assembly, bitStream))
		{
			if (replicator.isStreamingEnabled())
			{
				bitStream << false; // token: not done with packet

				if (!replicator.isInStreamedRegions(part->getConstPartPrimitive()->getExtentsWorld()))
				{
					bitStream << true; // token: CFrame only
					sendMechanismCFrames(bitStream, part, detailed);
                    lastSendCFrame = part->getCoordinateFrame();
					return true;
				}
				else
				{
					bitStream << false; // token: (not) CFrame only
					if (replicator.trySerializeId(bitStream, part))
					{
						sendMechanism(bitStream, part, detailed, lastSendTime, lastSendCFrame, accumulatedError, playerHead);
						return true;
					}
					else
					{
						replicator.serializeId(bitStream, NULL); // token: done with this mechanism
						return false;
					}
				}
			}
			else
			{
				if (replicator.trySerializeId(bitStream, part))
				{
					sendMechanism(bitStream, part, detailed, lastSendTime, lastSendCFrame, accumulatedError, playerHead);
					return true;
				}
			}
		}
	}

	return false;
}


