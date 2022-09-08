/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "PhysicsReceiver.h"

#include "Compressor.h"
#include "RakPeerInterface.h"
#include "NetworkSettings.h"

#include "Streaming.h"
#include "Replicator.h"
#include "Util.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PhysicsService.h"

#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/Joint.h"
#include "v8world/Mechanism.h"

#include "util/standardout.h"

#include "PhysicsSender.h"
#include "NetworkProfiler.h"

#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include <stack>
#include "util/MovementHistory.h"
#include "util/VarInt.h"

using namespace RBX;
using namespace RBX::Network;

DYNAMIC_FASTFLAG(HumanoidFloorPVUpdateSignal)
DYNAMIC_FASTINTVARIABLE(DebugMovementPathNumTotalWayPoint, 1000)
DYNAMIC_FASTFLAG(SimpleHermiteSplineInterpolate)

namespace RBX { namespace Network {
	namespace PathBasedMovementDebug
	{
		NodeDebugInfo FirstNode = {Color3::blue(), 13, false};
		NodeDebugInfo CompressedBaselineCFrame = {Color3::orange(), 12, true};
		NodeDebugInfo PathNode = {Color3::yellow(), 6, true};
		NodeDebugInfo XPacketCompressedPathNode = {Color3::green(), 6, true};
		NodeDebugInfo InterpolateCFrameFromNode = {Color3::white(), 10, true};
		NodeDebugInfo InterpolateCFrame = {Color3::white(), 11, true};
		NodeDebugInfo VelocityNode = {Color3::red(), 5, false};
		NodeDebugInfo OldNode = {Color3::red(), 6, true};
		NodeDebugInfo CFrame = {Color3::white(), 5, true}; // regular cframe not from path based movement
	}
}}

void DeserializedTouchItem::process(Replicator& replicator)
{
	if (replicator.physicsReceiver)
	{
		for (auto& tp: touchPairs)
		{
			replicator.physicsReceiver->processTouchPair(tp);
		}
	}
}

PhysicsReceiver::PhysicsReceiver(Replicator* replicator, bool isServer)
: replicator(replicator), iAmServer(isServer), stats(NULL), movementWaypointList(DFInt::DebugMovementPathNumTotalWayPoint)
{
	setTime(Time::nowFast());
}

void PhysicsReceiver::setTime(Time now_)
{
    now = now_;
}

/*
	Format for a mechanism:

	>> MechanismAttributes
	>> PrimaryAssembly
	>> done
	while (!done)
	{
		>> ChildAssembly
		>> done
	}
*/

/*
void logItem(MechanismItem& item, RakNet::Time timeStamp)
{
	float y = item.getAssemblyItem(0).pv.position.translation.y;
	
	G3D::Log::common()->printf("%u\t", timeStamp);
	G3D::Log::common()->printf("%f", y);
	G3D::Log::common()->println("");
}
*/

void PhysicsReceiver::receiveMechanismCFrames(RakNet::BitStream& bitStream, RakNet::Time timeStamp, const RBX::RemoteTime& sendTime)
{
    NETPROFILE_START("receiveMechanismCFrames", &bitStream);
    int bitStart = bitStream.GetReadOffset();
	shared_ptr<PartInstance> part;
	while (receivePart(part, bitStream))
	{
		if (part)
		{
			if (part->raknetTime > timeStamp) {
				if (replicator->settings().printPhysicsErrors) {
					RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Physics-in old packet");
				}
				part.reset();
			}
		}
		CoordinateFrame cf;
		Velocity vel;
		readCoordinateFrame(bitStream, cf);
		if(DFFlag::SimpleHermiteSplineInterpolate)
		{
			readVelocity(bitStream, vel);
		}

		if (part)
		{
			part->setPhysics(cf);
			part->addInterpolationSample(cf, vel, sendTime, now, 0);
		}
	}
    if (stats)
    {
        stats->details.mechanismCFrame.increment();
        stats->details.mechanismCFrameSize.sample((bitStream.GetReadOffset() - bitStart)/8);
    }
    NETPROFILE_END("receiveMechanismCFrames", &bitStream);
}

void PhysicsReceiver::receiveMechanism(RakNet::BitStream& bitStream, PartInstance* rootPart, MechanismItem& item, RemoteTime remoteSendTime, int& numNodesInHistory)
{
    if (NetworkSettings::singleton().getTotalNumMovementWayPoint() == 123456) // magic number to clear all after each packet
    {
        movementWaypointList.clear();
    }

    NETPROFILE_START("receiveMechanism", &bitStream);
    int bitStart = bitStream.GetReadOffset();
	item.reset();

	readMechanismAttributes(bitStream, item);
    readMovementHistory(bitStream, remoteSendTime, rootPart, item, numNodesInHistory);
    if (iAmServer) // client always sends assembly only
    {
	    readAssembly(bitStream, rootPart, item, false);
    }
	bool done;

	bitStream >> done;

	while (!done)
	{
		shared_ptr<PartInstance> part;
		bool ok = receivePart(part, bitStream);
		RBXASSERT(ok);

		readAssembly(bitStream, part.get(), item, false);

		bitStream >> done;
	}

    if (stats)
    {
        stats->details.mechanism.increment();
        stats->details.mechanismSize.sample((bitStream.GetReadOffset() - bitStart)/8);
    }
    NETPROFILE_END("receiveMechanism", &bitStream);
}

void PhysicsReceiver::readMovementHistory(RakNet::BitStream& bitStream, RemoteTime remoteSendTime, PartInstance* rootPart, MechanismItem& mechanismItem, int& numNodesInHistory)
{
    numNodesInHistory = 0;

    if (iAmServer)
    {
        // client always sends assembly only
        return;
    }
    NETPROFILE_START("readMovementHistory", &bitStream);
    bool hasMovement;
    unsigned int numNode;
    uint8_t precisionLevel, timeInterval2Ms;
    int8_t x, y, z;
    float timeInterval, timeToEnd;
    bitStream >> hasMovement;
    if (hasMovement)
    {
        CoordinateFrame cf;
        bool crossPacketCompression;
        bitStream >> crossPacketCompression;
        readAssembly(bitStream, rootPart, mechanismItem, crossPacketCompression);
        if (rootPart)
        {
            if (crossPacketCompression)
            {
                cf = rootPart->getLastCFrame(rootPart->getCoordinateFrame());
                cf.rotation = mechanismItem.getAssemblyItem(mechanismItem.numAssemblies()-1).pv.position.rotation;
            }
            else
            {
                cf = mechanismItem.getAssemblyItem(mechanismItem.numAssemblies()-1).pv.position;
            }
            rootPart->setLastCFrame(cf);
        }

        //bitStream >> numNode;
        VarInt<>::decode<RakNet::BitStream>(bitStream, &numNode);

        if (numNode > 0)
        {
			nodeStack.clear(); // Moved to header in order to avoid reallocation overhead

            Vector3 nodeTrans = cf.translation;
            AssemblyItem& assemblyItem = mechanismItem.getAssemblyItem(mechanismItem.numAssemblies()-1);
            timeToEnd = 0.f;
            for (uint8_t i=0; i<(uint8_t)numNode; i++)
            {
                bitStream >> precisionLevel;
                bitStream >> x;
                bitStream >> y;
                bitStream >> z;
                bitStream >> timeInterval2Ms;

                // reconstruct the time
                timeInterval = MovementHistory::getSecFrom2Ms(timeInterval2Ms);
                // reconstruct the cframe
                Vector3 delta;
                delta.x = MovementHistory::decompress(x, precisionLevel);
                delta.y = MovementHistory::decompress(y, precisionLevel);
                delta.z = MovementHistory::decompress(z, precisionLevel);
                nodeTrans = nodeTrans - delta;
                CoordinateFrame nodeCFrame(cf.rotation, nodeTrans);

                bool isBaselineNode = false;
                if (crossPacketCompression)
                {
                    if (i==0)
                    {
                        // this node is the current CFrame
                        assemblyItem.pv.position.translation = nodeCFrame.translation;
                        if (rootPart)
                        {
                            RBXASSERT(numNode > 1); // we should have at least two nodes: a baseline node and a path node
                            rootPart->setLastCFrame(nodeCFrame);
                            isBaselineNode = true;
                            //addVectorAdorn(cf.translation, nodeCFrame.translation, Color3::purple());
                        }
                    }
                    else if (i==1)
                    {
                        // Calculate the linear velocity based on last delta
                        assemblyItem.pv.velocity.linear = delta / timeInterval;
                        if (NetworkSettings::singleton().getTotalNumMovementWayPoint()%7 == 0) // magic number to print velocity
                        {
                            addWayPointAdorn(nodeCFrame.translation + assemblyItem.pv.velocity.linear, PathBasedMovementDebug::VelocityNode);
                        }
                    }
                }

                if (!isBaselineNode)
                {
                    TimedCF timedCf;
                    timedCf.cf = nodeCFrame;
                    timeToEnd += timeInterval;
                    timedCf.timeToEnd = timeToEnd;
                    nodeStack.push_back(timedCf);
                }
                else
                {
                    addWayPointAdorn(nodeCFrame.translation, PathBasedMovementDebug::CompressedBaselineCFrame);
                }
            }
            if (rootPart)
            {
                //StandardOut::singleton()->printf(MESSAGE_INFO, "-------------- %d", crossPacketCompression);
                int numNodesAhead = nodeStack.size();
                numNodesInHistory = numNodesAhead;

                while (nodeStack.size()>0)
                {
                    TimedCF timedCf = nodeStack.back();
                    RemoteTime nodeTime = remoteSendTime - Time::Interval(timedCf.timeToEnd);
                    if (replicator->remoteRaknetTimeToLocalRbxTime(nodeTime) >= rootPart->getLastUpdateTime()) // only process the new nodes
                    {
						if(DFFlag::SimpleHermiteSplineInterpolate)
							rootPart->addInterpolationSample(timedCf.cf,  mechanismItem.getAssemblyItem(mechanismItem.numAssemblies()-1).pv.velocity , nodeTime, replicator->remoteRaknetTimeToLocalRbxTime(remoteSendTime), timedCf.timeToEnd, numNodesAhead);
						else
							rootPart->addInterpolationSample(timedCf.cf,  Velocity() , nodeTime, replicator->remoteRaknetTimeToLocalRbxTime(remoteSendTime), timedCf.timeToEnd, numNodesAhead);

						addWayPointAdorn(timedCf.cf.translation, crossPacketCompression ? PathBasedMovementDebug::XPacketCompressedPathNode : PathBasedMovementDebug::PathNode);
                    }
                    else
                    {
                        if (replicator->settings().printPhysicsErrors) {
                            RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Discard old node *****");

							addWayPointAdorn(timedCf.cf.translation, PathBasedMovementDebug::OldNode);
                        }
                    }

                    nodeStack.pop_back();
                }
            }
        }
    }
    else
    {
        AssemblyItem& assemblyItem = mechanismItem.appendAssembly();
        assemblyItem.rootPart = shared_from<PartInstance>(rootPart);
        if (rootPart)
        {
            assemblyItem.pv.position = rootPart->getCoordinateFrame();
            assemblyItem.pv.velocity = rootPart->getVelocity();
        }
        readMotorAngles(bitStream, assemblyItem);
    }
    NETPROFILE_END("readMovementHistory", &bitStream);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicsReceiver::readMechanismAttributes(RakNet::BitStream& bitStream, MechanismItem& historyItem)
{
	NETPROFILE_START("readMechanismAttributes", &bitStream);
	historyItem.hasVelocity = replicator->settings().distributedPhysicsEnabled;

	bool hasState;
	bitStream >> hasState;
	if (hasState) {
		bitStream >> historyItem.networkHumanoidState;
	}
	else {
		historyItem.networkHumanoidState = 0;
	}
	NETPROFILE_END("readMechanismAttributes", &bitStream);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void PhysicsReceiver::readAssembly(RakNet::BitStream& bitstream, PartInstance* rootPart, MechanismItem& mechanismItem, bool crossPacketCompression)
{
	NETPROFILE_START("readAssembly", &bitstream);
	AssemblyItem& assemblyItem = mechanismItem.appendAssembly();
	assemblyItem.rootPart = shared_from<PartInstance>(rootPart);

    readPV(bitstream, assemblyItem, crossPacketCompression); 
    if (crossPacketCompression && rootPart)
    {
        // reuse the last translation (for now)
        assemblyItem.pv.position.translation = rootPart->getCoordinateFrame().translation;
    }

	readMotorAngles(bitstream, assemblyItem);

    if (rootPart)
    {
        addWayPointAdorn(assemblyItem.pv.position.translation, PathBasedMovementDebug::FirstNode);
    }

	if (rootPart)
	{
		RBXASSERT(rootPart->getPartPrimitive()->getAssembly());

		Primitive* primitive = rootPart->getPartPrimitive();
		Joint* toParent = rbx_static_cast<Joint*>(primitive->getEdgeToParent());

		// This joint could be null if the primitive still exists, but is no longer a child joint to parent

		if (toParent) {
			toParent->setPhysics();		// read on joint angles, etc.
		}
	}
	NETPROFILE_END("readAssembly", &bitstream);
}

void PhysicsReceiver::deserializeTouches(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from, std::vector<TouchPair>& touchPairs)
{
	while (true)
	{
		TouchPair tp;
		if (!deserializeTouch(bitstream, from, tp))
			return;

		if (tp.p1 && tp.p2)
			touchPairs.push_back(tp);
	}
}

bool PhysicsReceiver::deserializeTouch(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from, TouchPair& touchPair)
{
	shared_ptr<PartInstance> part1;
	if (!receivePart(part1, bitstream))
		return false;	// this marks the end of the packet

	shared_ptr<PartInstance> part2;
	bool ok = receivePart(part2, bitstream);
	RBXASSERT(ok);

	bool touched;
	bitstream >> touched;

	if (!part1)
		return true;
	if (!part2)
		return true;

	if (replicator->settings().printTouches) {
		if (touched) {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"Replication: Touch:%s->%s << %s", 
				part1->getName().c_str(), 
				part2->getName().c_str(), 
				RakNetAddressToString(replicator->remotePlayerId).c_str() 
				);
		} else {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"Replication: Untouch:%s->%s << %s", 
				part1->getName().c_str(), 
				part2->getName().c_str(), 
				RakNetAddressToString(replicator->remotePlayerId).c_str() 
				);
		}
	}

	touchPair = TouchPair(part1, part2, touched ? TouchPair::Touch : TouchPair::Untouch, RakNetToRbxAddress(from));

	return true;
}

void PhysicsReceiver::readTouches(RakNet::BitStream& bitstream, const RakNet::SystemAddress& from)
{
	while (true)
	{
		TouchPair tp;
		if (!deserializeTouch(bitstream, from, tp))
			return;

		if (tp.p1 && tp.p2)
			processTouchPair(tp);
	}
}

void PhysicsReceiver::processTouchPair(const TouchPair& tp)
{
	if (tp.type == TouchPair::Touch)
		tp.p1->reportTouch(tp.p2);
	else
		tp.p1->reportUntouch(tp.p2);

	// Send pair to other Replicators
	if (!physicsService)
		physicsService = shared_from(ServiceProvider::find<PhysicsService>(replicator));

	physicsService->onTouchStep(tp);
}

void PhysicsReceiver::readPV(RakNet::BitStream& bitStream, AssemblyItem& item, bool crossPacketCompression)
{
    NETPROFILE_START("readPV", &bitStream);
    if (crossPacketCompression)
    {
        // readRotation only
        Compressor::readRotation(bitStream, item.pv.position.rotation);
    }
    else
    {
        readCoordinateFrame(bitStream, item.pv.position);
        readVelocity(bitStream, item.pv.velocity);

        if (NetworkSettings::singleton().getTotalNumMovementWayPoint()%7 == 0) // magic number to print velocity
        {
            addWayPointAdorn(item.pv.position.translation + item.pv.velocity.linear, PathBasedMovementDebug::VelocityNode);
        }
    }
    NETPROFILE_END("readPV", &bitStream);
}

void PhysicsReceiver::readCoordinateFrame(RakNet::BitStream& bitStream, CoordinateFrame& cFrame)
{
    NETPROFILE_START("readCoordinateFrame", &bitStream);

	if(stats == NULL)
	{
		Compressor::readTranslation(bitStream, cFrame.translation);
		Compressor::readRotation(bitStream, cFrame.rotation);
		NETPROFILE_END("readCoordinateFrame", &bitStream);
	}
	else
	{
		NETPROFILE_START("readTranslation", &bitStream);
		int bitStart = bitStream.GetReadOffset();
		Compressor::readTranslation(bitStream, cFrame.translation);
		if (stats)
		{
			stats->details.translation.increment();
			stats->details.translationSize.sample((bitStream.GetReadOffset() - bitStart)/8);
		}
		NETPROFILE_END("readTranslation", &bitStream);
		NETPROFILE_START("readRotation", &bitStream);
		bitStart = bitStream.GetReadOffset();
		Compressor::readRotation(bitStream, cFrame.rotation);
		if (stats)
		{
			stats->details.rotation.increment();
			stats->details.rotationSize.sample((bitStream.GetReadOffset() - bitStart)/8);
		}
		NETPROFILE_END("readRotation", &bitStream);
		NETPROFILE_END("readCoordinateFrame", &bitStream);
	}
}

void PhysicsReceiver::readVelocity(RakNet::BitStream& bitStream, Velocity& velocity)
{
    NETPROFILE_START("readVelocity", &bitStream);
    int bitStart = bitStream.GetReadOffset();
	if (replicator->settings().distributedPhysicsEnabled)
	{
		readVectorFast( bitStream, velocity.linear.x, velocity.linear.y, velocity.linear.z );
		readVectorFast( bitStream, velocity.rotational.x, velocity.rotational.y, velocity.rotational.z );
	}
	else
	{
		velocity = Velocity::zero();
	}
    if (stats)
    {
        stats->details.velocity.increment();
        stats->details.velocitySize.sample((bitStream.GetReadOffset() - bitStart)/8);
    }
    NETPROFILE_END("readVelocity", &bitStream);
}

void PhysicsReceiver::readMotorAngles(RakNet::BitStream& bitStream, AssemblyItem& item)
{
    NETPROFILE_START("readMotorAngles", &bitStream);
	unsigned char compactNum;
	bitStream >> compactNum;

	if (compactNum>50 && replicator->settings().printPhysicsErrors)
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Physics-in has %d motors", compactNum);

	const int numMotors = compactNum;
	item.motorAngles.resize(numMotors);		// i.e. - fast clear, no allocation

	for (int i = 0; i < numMotors; ++i) 
	{
		readCompactCFrame(bitStream, item.motorAngles[i]);
	}
    NETPROFILE_END("readMotorAngles", &bitStream);
}

void PhysicsReceiver::readCompactCFrame(RakNet::BitStream& bitStream, CompactCFrame& cFrame)
{
    NETPROFILE_START("readCompactCFrame", &bitStream);
	bool isSimpleZAngle = bitStream.ReadBit();

	if(isSimpleZAngle)
	{
		unsigned char byteAngle;
		bitStream >> byteAngle;
		cFrame = CompactCFrame(Vector3::zero(), Vector3::unitZ(), Math::rotationFromByte(byteAngle));
		RBXASSERT(!Math::isNanInfVector3(cFrame.getAxis()));
		RBXASSERT(!Math::isNanInf(cFrame.getAngle()));
		RBXASSERT(!Math::isNanInfVector3(cFrame.translation));
	}
	else
	{
		bool hasTranslation = bitStream.ReadBit();
		bool hasRotation = bitStream.ReadBit();
		if(hasTranslation)
		{
			Compressor::readTranslation(bitStream, cFrame.translation);
		}
		else
		{
			cFrame.translation = Vector3::zero();
		}

		if(hasRotation)
		{
			Vector3 axis;
			readVectorFast( bitStream, axis.x, axis.y, axis.z );
			unsigned char byteAngle;
			bitStream >> byteAngle;
			float angle = Math::rotationFromByte(byteAngle);
			cFrame.setAxisAngle(axis, angle);
		}
		else
		{
			cFrame.setAxisAngle(RBX::Vector3::unitX(), 0);
		}

		RBXASSERT(!Math::isNanInfVector3(cFrame.getAxis()));
		RBXASSERT(!Math::isNanInf(cFrame.getAngle()));
		RBXASSERT(!Math::isNanInfVector3(cFrame.translation));
	}
    NETPROFILE_END("readCompactCFrame", &bitStream);
}

void PhysicsReceiver::setPhysics(const MechanismItem& item, const RBX::RemoteTime& remoteSendTime, const RakNet::TimeMS lagInMs, int numNodesInHistory)
{
	bool hasVelocity = item.hasVelocity;
	Time::Interval lag(static_cast<double>(lagInMs) / 1000.0f);

	for (int i = 0; i < item.numAssemblies(); ++i)
	{
		AssemblyItem& assemblyItem = item.getAssemblyItem(i);

		// assemblyItem.primitive will be NULL if the Part was destroyed while the physics packet was outstanding

		if (PartInstance* part = assemblyItem.rootPart.get())
		{
			if (this->replicator->filterPhysics(part) == Reject)
			{
				if (replicator->settings().printPhysicsFilters)
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "filterPhysics %s", part->getName().c_str());
				continue;
			}

			Primitive* primitive = part->getPartPrimitive();

			// Show david this assert and then destroy - validating the case where we received this history item in the past, and the primitive is no longer in world
			RBXASSERT(primitive->getWorld());

			if (!Assembly::isAssemblyRootPrimitive(primitive))
			{
				if (replicator->settings().printPhysicsFilters)
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "!isAssemblyRootPrimitive %s", part->getName().c_str());
				continue;
			}

			Assembly* a = primitive->getAssembly();
			if (a->computeIsGrounded())
			{
				if (replicator->settings().printPhysicsFilters)
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "computeIsGrounded %s", part->getName().c_str());
				continue;
			}
			
			a->setPhysics(assemblyItem.motorAngles, assemblyItem.pv);				// motor angles
			a->setNetworkHumanoidState((i == 0) ? item.networkHumanoidState : 0);

			if (hasVelocity)
			{
				if (iAmServer)
				{
					part->addInterpolationSample(assemblyItem.pv.position, assemblyItem.pv.velocity, remoteSendTime, now, 0.f, numNodesInHistory);
					part->setPhysics(assemblyItem.pv);

                    Time localTime = replicator->remoteRaknetTimeToLocalRbxTime(remoteSendTime);
                    part->addMovementNode(assemblyItem.pv.position, assemblyItem.pv.velocity, localTime); // force update the movement node
				} 
				else
				{
					bool isLagCompenstated = false;

                    if (isLagCompenstated)
                    {
						// Extrapolate with velocity
						if (assemblyItem.pv.velocity.rotational.squaredLength() >= 0.01)
						{
							Quaternion qOrientation(assemblyItem.pv.position.rotation);
							qOrientation.normalize();
							Quaternion qDot(Quaternion(assemblyItem.pv.velocity.rotational) * qOrientation * 0.5f);
							qOrientation += qDot * lag.seconds();
							qOrientation.normalize();
							qOrientation.toRotationMatrix(assemblyItem.pv.position.rotation);
						}
						if (assemblyItem.pv.velocity.linear.squaredLength() >= 1)
							assemblyItem.pv.position.translation += assemblyItem.pv.velocity.linear  * lag.seconds();

                        part->addInterpolationSample(assemblyItem.pv.position, assemblyItem.pv.velocity, remoteSendTime + lag, now, 0.f, numNodesInHistory);
                    } 
					else 
					{
                        part->addInterpolationSample(assemblyItem.pv.position, assemblyItem.pv.velocity, remoteSendTime, now, 0.f, numNodesInHistory);
					}

					CoordinateFrame oldPosition;
					RBX::Velocity previousLinearVelocity;
					RBX::Time lastUpdateTime;
					
					if (DFFlag::HumanoidFloorPVUpdateSignal)
					{
						oldPosition = part->getCoordinateFrame();
						previousLinearVelocity = part->getVelocity();
						lastUpdateTime = part->getLastUpdateTime();

					}
					part->setPhysics(assemblyItem.pv);

					if (DFFlag::HumanoidFloorPVUpdateSignal)
					{
						float deltaTime =  (replicator->remoteRaknetTimeToLocalRbxTime(remoteSendTime) - lastUpdateTime).seconds();
						//shared_ptr<PartInstance> partPointer = shared_from(part);
                        if (part->onDemandRead())
    						part->onDemandWrite()->onPositionUpdatedByNetworkSignal(boost::ref(part), boost::ref(oldPosition), boost::ref(previousLinearVelocity), boost::ref(deltaTime));
					}
				}
			}
			else
			{
    			part->addInterpolationSample(assemblyItem.pv.position, assemblyItem.pv.velocity, remoteSendTime, now, 0.f, numNodesInHistory);
				CoordinateFrame oldPosition;
				RBX::Velocity previousLinearVelocity;
				RBX::Time lastUpdateTime;
				if (DFFlag::HumanoidFloorPVUpdateSignal)
				{
					oldPosition = part->getCoordinateFrame();
					previousLinearVelocity = part->getVelocity();
					lastUpdateTime = part->getLastUpdateTime();
				}
				part->setPhysics(assemblyItem.pv.position);

				if (DFFlag::HumanoidFloorPVUpdateSignal)
				{
					float deltaTime =  (replicator->remoteRaknetTimeToLocalRbxTime(remoteSendTime) - lastUpdateTime).seconds();
					//shared_ptr<PartInstance> partPointer = shared_from(part);
                    if (part->onDemandRead())
    					part->onDemandWrite()->onPositionUpdatedByNetworkSignal(boost::ref(part), boost::ref(oldPosition), boost::ref(previousLinearVelocity), boost::ref(deltaTime));
				}
			}
		}
	}
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void PhysicsReceiver::addWayPointAdorn(const Vector3& p, const PathBasedMovementDebug::NodeDebugInfo& info, const std::string& debugText)
{
	if (Workspace::showPartMovementPath && info.show)
	{
		if (movementWaypointList.capacity() != NetworkSettings::singleton().getTotalNumMovementWayPoint())
		{
			movementWaypointList.rset_capacity(NetworkSettings::singleton().getTotalNumMovementWayPoint());
		}
		if (movementWaypointList.capacity() > 0)
		{
			movementWaypointList.push_back(MovementWaypointAdorn(p, info.color, info.size, debugText));
		}
	}
}

void PhysicsReceiver::addVectorAdorn(const Vector3& start, const Vector3& end, const RBX::Color4& c)
{
    if (Workspace::showPartMovementPath)
    {
        if (movementVectorList.capacity() != NetworkSettings::singleton().getTotalNumMovementWayPoint())
        {
            movementVectorList.rset_capacity(NetworkSettings::singleton().getTotalNumMovementWayPoint());
        }
        if (movementVectorList.capacity() > 0)
        {
            movementVectorList.push_back(MovementVectorAdorn(start, end, c));
        }
    }
}


bool PhysicsReceiver::okDistributedReceivePart(const shared_ptr<PartInstance>& part)
{
	return (	!replicator->settings().distributedPhysicsEnabled
			||	replicator->checkDistributedReceive(part.get())	);
}


bool PhysicsReceiver::receiveRootPart(shared_ptr<PartInstance>& part, RakNet::BitStream& inBitstream)
{
    NETPROFILE_START("receiveRootPart", &inBitstream);
	bool answer = receivePart(part, inBitstream);

	if (part)
	{
		if (part->getPartPrimitive()->getAssembly()->computeIsGrounded())
		{
			part.reset();
		}
		else if (!okDistributedReceivePart(part))
		{
			part.reset();
		}
	}
    NETPROFILE_END("receiveRootPart", &inBitstream);
	return answer;
}


bool PhysicsReceiver::receivePart(shared_ptr<PartInstance>& part, RakNet::BitStream& inBitstream)
{
	shared_ptr<Instance> instance;
	RBX::Guid::Data id;
	if (replicator->deserializeInstanceRef(inBitstream, instance, id))
	{
		if (instance==NULL) {
			return false;			// packet end tag
		}
		part = Instance::fastSharedDynamicCast<PartInstance>(instance);
	}
	else
	{
		if (replicator->settings().printPhysicsErrors) {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,
				"Physics-in of unidentified %s", id.readableString().c_str());
		}
	}

	if (part)
	{
		if (!PartInstance::nonNullInWorkspace(part))
		{
			if (replicator->settings().printPhysicsErrors) {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,
					"Physics-in of part not in workspace %s", id.readableString().c_str());
			}
			part.reset();
		}
	}
	return true;
}


void PhysicsReceiver::renderPartMovementPath(Adorn* adorn)
{

    for (boost::circular_buffer<MovementWaypointAdorn>::iterator iter = movementWaypointList.begin(); iter != movementWaypointList.end(); iter++)
    {
        MovementWaypointAdorn wp = *iter;
        DrawAdorn::star(adorn, wp.position, wp.size, wp.color, wp.color, wp.color);

		if (wp.text.length() > 0)
		{
			Vector3 aboveStar = wp.position;
			aboveStar.y += 5.0f;

			const Camera& camera = *adorn->getCamera();
			Vector3 screenLoc = camera.project(aboveStar);
			if(screenLoc.z == std::numeric_limits<float>::infinity())
				continue;

			adorn->drawFont2D(
				wp.text,
				screenLoc.xy(),
				10.0f,
				false,
				Color4(Color3::white(), 1.0f),
				Color4(Color3::black(), 1.0f),
				Text::FONT_ARIALBOLD,
				Text::XALIGN_CENTER,
				Text::YALIGN_BOTTOM );
		}
    }

    for (boost::circular_buffer<MovementVectorAdorn>::iterator iter = movementVectorList.begin(); iter != movementVectorList.end(); iter++)
    {
        MovementVectorAdorn wp = *iter;
        adorn->line3d(wp.startPos, wp.endPos, wp.color);
    }
}

