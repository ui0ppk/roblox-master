/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "TopNErrorsPhysicsSender.h"
#include "Replicator.h"
#include "Compressor.h"

#include "Network/Player.h"

#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "MechanismItem.h"

#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PhysicsService.h"

#include "GetTime.h"
#include "RakPeerInterface.h"
#include "NetworkSettings.h"
#include "ConcurrentRakPeer.h"
#include "NetworkPacketCache.h"

#include "Streaming.h"
#include "BitStream.h"
#include "Util.h"
#include "util/MovementHistory.h"
#include "util/VarInt.h"
#include "util/Http.h"
#include "v8world/DistributedPhysics.h"
#include "V8World/Mechanism.h"

using namespace RBX;
using namespace RBX::Network;

DYNAMIC_FASTINT(PhysicsCompressionSizeFilter)

DYNAMIC_FASTINTVARIABLE(PhysicsSenderBufferHealthThreasholdPercent, 40)
DYNAMIC_FASTINTVARIABLE(PhysicsSenderRotationThresholdThousandth, 20)

DYNAMIC_FASTFLAGVARIABLE(PhysicsSenderSleepingUpdate, false)
LOGVARIABLE(PhysicsSenderSleepingLog, 0);

DYNAMIC_FASTFLAGVARIABLE(PhysicsSenderUseOwnerTimestamp, false)
DYNAMIC_FASTFLAGVARIABLE(PhysicsSenderCheckPartInServiceBeforeSend, false)
DYNAMIC_FASTFLAGVARIABLE(DebugPhysicsSenderLogCacheMissToGA, false)

SYNCHRONIZED_FASTFLAG(PhysicsPacketSendWorldStepTimestamp)

#define CROSS_PACKET_COMPRESSION_DISTANCE_SQ_THRESHOLD 2500.0f
#define CROSS_PACKET_COMPRESSION_VELOCITY_THRESHOLD 15.0f

TopNErrorsPhysicsSender::TopNErrorsPhysicsSender(Replicator& replicator)
: PhysicsSender(replicator)
, stepId(0)
, sendId(0)
, sortId(0)
{
    if (replicator.isProtocolCompatible())
	    physicsPacketCache = shared_from(ServiceProvider::find<PhysicsPacketCache>(&replicator));
}

TopNErrorsPhysicsSender::~TopNErrorsPhysicsSender()
{
}

void TopNErrorsPhysicsSender::step()
{
	/*
		1. Update error on all objects. TODO: pick object with highest potential delta error and only update them.
		2. Find top N items based on error and place them at front of list to be send next.
	*/
    currentStepTimestamp = Time::now<Time::Fast>();

	if (!physicsService)
	{
		physicsService = shared_from(ServiceProvider::find<PhysicsService>(&replicator));

		RBXASSERT(physicsService);		// datamodel should always create this

		// Get all current assemblies
		std::for_each(physicsService->begin(), physicsService->end(), boost::bind(&TopNErrorsPhysicsSender::addNugget, this, _1));

		// Listen for assembly changes
		addingAssemblyConnection = physicsService->assemblyAddingSignal.connect(boost::bind(&TopNErrorsPhysicsSender::onAddingAssembly, this, _1));
		//removedAssemblyConnection = physicsService->assemblyRemovedSignal.connect(boost::bind(&ErrorCompPhysicsSender::onRemovedAssembly, this, _1));
	}

	std::for_each(newMovingAssemblies.begin(), newMovingAssemblies.end(), boost::bind(&TopNErrorsPhysicsSender::addNugget2, this, _1));
	newMovingAssemblies.clear();

	// Prepare the data needed to compute errors
	const ModelInstance* characterModel = NULL;
	CoordinateFrame focus;
	if (const Player* player = replicator.findTargetPlayer()) {
		if (player->hasCharacterHead(focus)) {
			characterModel = player->getConstCharacter();
		}
	}

	stepId++;		// can go for 100's of days at 30 fps until maxing out

	// no need to re-sort list if we haven't sent from previously sorted list
	if ((stepId - sortId < 2) && (sortId > sendId))
		return;

	sortId = stepId;

	int numAsleep = 0;

	NuggetList::iterator iter = nuggetList.begin();
	while (iter != nuggetList.end())
	{
		Nugget* nugget = *iter;
		if( !DFFlag::PhysicsSenderSleepingUpdate )
		{
			if (!nugget->part->PhysicsServiceHook::is_linked())
			{
				// remove part from sender if it's no longer in physics service, it's not moving
				NuggetMap::iterator mapIter = nuggetMap.find(nugget->part);	// need to do find() here because boost unordered_map invalidates iterators during insert
				RBXASSERT(mapIter != nuggetMap.end());

				nuggetMap.erase(mapIter);
				iter = nuggetList.erase(iter);
				continue;
			}
		}
		else
		{
			if (!nugget->part->PhysicsServiceHook::is_linked())
			{
				if( isSleepingRootPrimitive(nugget->part.get()) )
				{
					nugget->notInService = true;
					++numAsleep;
				}
				else
				{
					NuggetMap::iterator mapIter = nuggetMap.find(nugget->part);	// need to do find() here because boost unordered_map invalidates iterators during insert
					RBXASSERT(mapIter != nuggetMap.end());

					nuggetMap.erase(mapIter);
					iter = nuggetList.erase(iter);
					continue;
				}
			}
			else
			{
				nugget->notInService = false;
			}
		}

		if (replicator.checkDistributedSendFast(nugget->part.get()))
			nugget->computeError(focus, characterModel, stepId, this);
		else
        {
            nugget->lastSendTime = currentStepTimestamp;
            nugget->lastSent = nugget->part->getCoordinateFrame();
            //nugget->accumulatedError = MH_TOLERABLE_COMPRESSION_ERROR+1.f; // force to send full CFrame for the first packet after ownership change
			nugget->error = 0.0f;
        }
		iter++;
	}

    std::nth_element(nuggetList.begin(), nuggetList.begin() + static_cast<int>(replicator.physicsSenderStats().physicsItemsPerPacket.value()), nuggetList.end(), Nugget::compError);

	FASTLOG2(FLog::PhysicsSenderSleepingLog, "Asleep: %d Total: %d", numAsleep, (int)nuggetList.size() );
}

int TopNErrorsPhysicsSender::sendPacket(int maxPackets, PacketPriority packetPriority,
		ReplicatorStats::PhysicsSenderStats* stats)
{

	double percent = DFInt::PhysicsSenderBufferHealthThreasholdPercent / 100.0;
	if ((replicator.replicatorStats.bufferHealth < percent) && (replicator.getBufferCountAvailable(maxPackets, packetPriority) <= 0))
		return 0;

	const unsigned int maxStreamSize(replicator.getPhysicsMtuSize());

	boost::shared_ptr<RakNet::BitStream> bitStream;

	int packetCount = 0;
	int sentItemCount = 0;

    // Prepare the data needed to compute errors
    const PartInstance* playerHead = NULL;
    CoordinateFrame focus;
    if (const Player* player = replicator.findTargetPlayer()) {
        playerHead = player->hasCharacterHead(focus);
    }

	int numAsleepSent = 0;

    NuggetList::iterator iter = nuggetList.begin();
	while (iter != nuggetList.end())
	{
		Nugget* nugget = *iter;
		if (!bitStream) {
			if (packetCount>=maxPackets)
				break;

			bitStream.reset(new RakNet::BitStream(maxStreamSize));

			++packetCount;

			*bitStream << (unsigned char) ID_TIMESTAMP;
			RakNet::Time timestamp = RakNet::GetTime();
			RakNet::Time interpolationTimeStamp;

			if (!SFFlag::getPhysicsPacketSendWorldStepTimestamp() && DFFlag::PhysicsSenderUseOwnerTimestamp && nugget->part->computeNetworkOwnerIsSomeoneElse())
			{
				// retrieve the original packet send time of the part
				RBXASSERT(timestamp >= nugget->part->raknetTime);
				if (nugget->part->raknetTime)
					timestamp = nugget->part->raknetTime;
			}

			*bitStream << timestamp;
			*bitStream << (unsigned char) ID_PHYSICS;
			
			
			if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
			{
				if (nugget->part->computeNetworkOwnerIsSomeoneElse())
				{
					if (nugget->part->raknetTime)
						interpolationTimeStamp = nugget->part->raknetTime;
					else
						interpolationTimeStamp = timestamp;
				}
				else
				{
					float timeOffsetDueToPhysics = 0.0f;
					Workspace* w = ServiceProvider::find<RBX::Workspace>(&replicator);
					if (w)
					{
						timeOffsetDueToPhysics = w->getWorld()->getUpdateExpectedStepDelta();		
					}

					// Generates timestamp for this frame, minus a small offset based on
					// missed or extra world steps taken.
					interpolationTimeStamp = (RakNet::Time)((TaskScheduler::singleton().lastCyclcTimestamp +  Time::Interval(replicator.getRakOffsetTime() - timeOffsetDueToPhysics)).timestampSeconds()* 1000.0);
				}

				*bitStream << interpolationTimeStamp;
			}
		}

		if (DFFlag::PhysicsSenderCheckPartInServiceBeforeSend && !nugget->part->PhysicsServiceHook::is_linked())
		{
			// remove part from sender if it's no longer in physics service, it's not moving
			NuggetMap::iterator mapIter = nuggetMap.find(nugget->part);	// need to do find() here because boost unordered_map invalidates iterators during insert
			RBXASSERT(mapIter != nuggetMap.end());

			nuggetMap.erase(mapIter);
			iter = nuggetList.erase(iter);
			continue;
		}

        CoordinateFrame sentCFrame = nugget->lastSent;
        float accumulatedError = nugget->accumulatedError;
		if (!sendPhysicsData(*bitStream, *nugget, sentCFrame, accumulatedError, playerHead)) 
		{
			++iter;
			continue;
		}
		else
		{
			RBXASSERT(nugget->part->getConstPartPrimitive()->getConstAssembly());

			if( !DFFlag::PhysicsSenderSleepingUpdate )
			{
				nugget->onSent(currentStepTimestamp, sentCFrame, accumulatedError);
				RBXASSERT((*iter)->lastSent == sentCFrame);

				++iter;
			}
			else
			{
				if(nugget->notInService)
				{
					NuggetMap::iterator mapIter = nuggetMap.find(nugget->part);	// need to do find() here because boost unordered_map invalidates iterators during insert
					RBXASSERT(mapIter != nuggetMap.end());
					
					nuggetMap.erase(mapIter);
					iter = nuggetList.erase(iter);

					++numAsleepSent;
				}
				else
				{
					nugget->onSent(currentStepTimestamp, sentCFrame, accumulatedError);
					RBXASSERT((*iter)->lastSent == sentCFrame);

					++iter;
				}
			}

			sentItemCount++;
		}


		if (bitStream->GetNumberOfBytesUsed() > static_cast<int>(maxStreamSize)) 
		{
			// Packet "end" tag
			if (replicator.isStreamingEnabled())
				*bitStream << true; // done
			else
				replicator.serializeId(*bitStream, NULL);

			// Send ID_PHYSICS
			replicator.rakPeer->Send(bitStream, packetPriority, UNRELIABLE, PHYSICS_CHANNEL, replicator.remotePlayerId, false);
			stats->physicsPacketsSent.sample();
			stats->physicsPacketsSentSmooth.sample();
			stats->physicsPacketsSentSize.sample(bitStream->GetNumberOfBytesUsed());
			stats->physicsItemsPerPacket.sample(sentItemCount);

			bitStream.reset();
		}
	}

	if (bitStream)
	{
		// Packet "end" tag
		if (replicator.isStreamingEnabled())
			*bitStream << true; // done
		else
			replicator.serializeId(*bitStream, NULL);

		// Send ID_PHYSICS
		replicator.rakPeer->Send(bitStream, packetPriority, UNRELIABLE, PHYSICS_CHANNEL, replicator.remotePlayerId, false);
		stats->physicsPacketsSent.sample();
		stats->physicsPacketsSentSmooth.sample();
		stats->physicsPacketsSentSize.sample(bitStream->GetNumberOfBytesUsed());
		stats->physicsItemsPerPacket.sample(sentItemCount);
	}

	sendId = stepId;

	FASTLOG1(FLog::PhysicsSenderSleepingLog, "Asleep sent: %d", numAsleepSent );

	return packetCount;
};


void TopNErrorsPhysicsSender::onAddingAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	newMovingAssemblies.push_back(part);
}

void TopNErrorsPhysicsSender::addNugget(PartInstance& part)
{
	addNugget2(shared_from(&part));
}

void TopNErrorsPhysicsSender::addNugget2(shared_ptr<PartInstance> part)
{
	// try add item to map
	std::pair<NuggetMap::iterator, bool> result = nuggetMap.insert(std::make_pair(part, Nugget(part)));

	// add succeeded
	if (result.second)
	{
		nuggetList.push_back(&(result.first->second));
	}
	else if( DFFlag::PhysicsSenderSleepingUpdate )
	{
		result.first->second.notInService = false;
	}
}

void TopNErrorsPhysicsSender::removeNugget(shared_ptr<const PartInstance> part)
{
	NuggetMap::iterator iter = nuggetMap.find(part);
	RBXASSERT(iter != nuggetMap.end());

	for (NuggetList::iterator i = nuggetList.begin(); i != nuggetList.end(); i++)
	{
		if (*i == &(iter->second))
		{
			nuggetList.erase(i);
			break;
		}
	}

	nuggetMap.erase(iter);
}

void TopNErrorsPhysicsSender::onRemovedAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<const PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	removeNugget(part);
}


bool TopNErrorsPhysicsSender::sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget, CoordinateFrame& outLastSendCFrame, float& accumulatedError, const PartInstance* playerHead)
{
	sendDetailed = nugget.sendDetailed;
	return PhysicsSender::sendPhysicsData(bitStream, nugget.part.get(), sendDetailed, nugget.lastSendTime, outLastSendCFrame, accumulatedError, playerHead);
}

void TopNErrorsPhysicsSender::writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
	if (physicsPacketCache)
	{
		unsigned char index = 0;
		if (sendDetailed)
			index |= 1;
		if (crossPacketCompression)
			index |= 2;

		// look for this packet in the cache
		if (!physicsPacketCache->fetchIfUpToDate(assembly, index, bitStream))
		{
			// cache miss, recreate bit stream and update cache
			unsigned int startBit = bitStream.GetWriteOffset();

			PhysicsSender::writeAssembly(bitStream, assembly, compressionType, crossPacketCompression);

			bitStream.SetReadOffset(startBit);

			if (!physicsPacketCache->update(assembly, index, bitStream, startBit, bitStream.GetNumberOfBitsUsed() - startBit))
			{
				if( !DFFlag::PhysicsSenderSleepingUpdate )
				{
					// this item should be in the cache
					//StandardOut::singleton()->print(MESSAGE_INFO, "cache update failed");

					if (DFFlag::DebugPhysicsSenderLogCacheMissToGA)
					{
						static boost::once_flag flag = BOOST_ONCE_INIT;
						boost::call_once(flag, boost::bind(&Analytics::GoogleAnalytics::trackEventWithoutThrottling, "Error", "PhysicsSenderCacheMiss", 
							RBX::Http::placeID.c_str(), 0, false));
					}

					RBXASSERT(0);
				}
			}
		}
	}
	else
		PhysicsSender::writeAssembly(bitStream, assembly, compressionType, crossPacketCompression);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float TopNErrorsPhysicsSender::Nugget::minDistance() {return 9.0f;}
float TopNErrorsPhysicsSender::Nugget::maxDistance() {return 1000000.0f;}
float TopNErrorsPhysicsSender::Nugget::minSize()		{return 2.0f;}
float TopNErrorsPhysicsSender::Nugget::maxSize()		{return 50.0f;}

TopNErrorsPhysicsSender::Nugget::Nugget(shared_ptr<PartInstance> part)
	: part(part)
	, error(0.0f)
	, lastSent(CoordinateFrame())
	, biggestSize(minSize())
	, sendDetailed(true)
	, notInService(false)
	, radius(0.0f)
    , lastSendTime(Time())
    , accumulatedError(0.f)
{
}

void TopNErrorsPhysicsSender::Nugget::onSent(const Time& t, const CoordinateFrame& cf, float err)			
{
	// part - no change
	// error - will be computed
	lastSent = cf;
    lastSendTime = t;
	accumulatedError = err;
}


void TopNErrorsPhysicsSender::Nugget::computeError(const CoordinateFrame& focus, const ModelInstance* focusModel, int stepId, const TopNErrorsPhysicsSender* sender)
{
	// Opt: getting assembly is expensive, only do this periodically 
	if ((stepId % 20) == 0)
	{
		const Assembly* assembly = part->getConstPartPrimitive()->getConstAssembly();
		RBXASSERT(assembly);

		radius = assembly->getLastComputedRadius();
		
		// 1. Biggest Size
		biggestSize = G3D::clamp(2*radius, minSize(), maxSize());
	}

	const CoordinateFrame& cf = part->getCoordinateFrame();
		
	// current
	float currentDistance = minDistance();			// There is no center of interest.  Location doesn't affect error
	if (focusModel)	{
		currentDistance = (cf.translation - focus.translation).squaredMagnitude() - radius*radius;
		currentDistance = G3D::clamp(currentDistance, minDistance(), maxDistance());
	}

	// 2. Error
	// 3. Send Detailed
	if (part->isDescendantOf(focusModel)) {				// the character!
		error = std::numeric_limits<float>::max();
		sendDetailed = true;
	}
	else {
		const float positionalError = Math::taxiCabMagnitude(cf.translation - lastSent.translation);
		const float rotationError = Math::sumDeltaAxis(cf.rotation, lastSent.rotation) * radius * 0.2f;	// factor down - we are summing the axis
		error = biggestSize * biggestSize * (rotationError + positionalError) / currentDistance;
		sendDetailed = currentDistance < 400.0f; /*20.0f * 20.0f */ // using square distance
		if (!sendDetailed)
		{
			// if it is a huge object, we send details otherwise it could be jittering due to heavy compression
            if (biggestSize > (float)(DFInt::PhysicsCompressionSizeFilter))
			{
				sendDetailed = true;
			}
		}
	}
}

void TopNErrorsPhysicsSender::writePV(RakNet::BitStream& bitStream, const Assembly* a, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
        const CoordinateFrame* cf;
        const Velocity* v;
        const PartInstance* part = PartInstance::fromConstAssembly(a);
        if (part)
        {
            cf = &part->getMovementHistory().getBaselineCFrame();
            v = &part->getMovementHistory().getBaselineVelocity();
        }
        else
        {
            cf = &a->getConstAssemblyPrimitive()->getPV().position;
            v = &a->getConstAssemblyPrimitive()->getPV().velocity;
        }
        // writeTranslation
        if (!crossPacketCompression)
        {
            Compressor::writeTranslation(bitStream, cf->translation, compressionType);
        }
        // writeRotation
        Compressor::writeRotation(bitStream, cf->rotation, compressionType);

        // writeVelocity
        if (!crossPacketCompression)
        {
            PhysicsSender::writeVelocity(bitStream, *v);
        }
    }

bool TopNErrorsPhysicsSender::writeMovementHistory(RakNet::BitStream& bitStream, const MovementHistory& history, const Assembly* assembly, const Time& lastSendTime, CoordinateFrame& lastSendCFrame, bool& hasMovement, Compressor::CompressionType compressionType, float& accumulatedError, const PartInstance* playerHead)
{
        CoordinateFrame currentCFrame = history.getBaselineCFrame();
        if (history.hasHistory(accumulatedError) || Math::sumDeltaAxis(currentCFrame.rotation, lastSendCFrame.rotation) > DFInt::PhysicsSenderRotationThresholdThousandth / 1000.0f)
        {
            hasMovement = true;
            bitStream << true; // has movement
            
            bool crossPacketCompression = (!replicator.isStreamingEnabled()) && history.getNumNodes()>2 && compressionType != Compressor::UNCOMPRESSED && accumulatedError <= MH_TOLERABLE_COMPRESSION_ERROR && (!lastSendTime.isZero()) && (!lastSendCFrame.translation.isZero());
            if (crossPacketCompression)
            {
                const Primitive* p = assembly->getConstAssemblyPrimitive();
                Vector3 size = p->getSize();
                if (size.x > 6 || size.y > 6 || size.z > 6) // we don't do cross-packet compression for large objects
                {
                    crossPacketCompression = false;
                }
            }

            CoordinateFrame calculatedCFrame;
            Vector3 calculatedLinearVelocity;
            std::deque<MovementHistory::MovementNode> movementNodeList;
            history.getMovementNodeList(lastSendTime, currentStepTimestamp, movementNodeList, crossPacketCompression, lastSendCFrame, calculatedCFrame, calculatedLinearVelocity);

            if (movementNodeList.size() == 0)
            {
                crossPacketCompression = false;
            }

            bool isPartInSimulationRange = false;
			
            if (crossPacketCompression)
            {
				bool isPvOverThreshold = false;
				Vector3 baselineTranslation = history.getBaselineCFrame().translation;
				Vector3 baselineVelocity = history.getBaselineVelocity().linear;

				// Don't use cross packet compression for nearby parts, baseline nodes could be lost and create error
				if (playerHead)
				{
					float distanceToPart = (baselineTranslation - playerHead->getCoordinateFrame().translation).squaredMagnitude();
					if (distanceToPart <= CROSS_PACKET_COMPRESSION_DISTANCE_SQ_THRESHOLD)
					{
						// slow moving objects are ok
						if (Math::taxiCabMagnitude(baselineVelocity) > CROSS_PACKET_COMPRESSION_VELOCITY_THRESHOLD)
							isPvOverThreshold = true;
					}
				}

                // Make sure the errors are not escalating..
                float positionalError = Math::taxiCabMagnitude(calculatedCFrame.translation - baselineTranslation);
                bool isEstimatedVelocaityInaccurate = false;

                // if the part is within the client's simulation radius, check the velocity to make sure it's accurate enough for the client to take over the ownership
				Vector2 partLoc = baselineTranslation.xz();
                Region2::WeightedPoint weightedPoint;
				replicator.readPlayerSimulationRegion(playerHead, weightedPoint);
				isPartInSimulationRange = Region2::pointInRange(partLoc, weightedPoint, DistributedPhysics::CLIENT_SLOP());
				if (isPartInSimulationRange)
				{
					float velocityDiff = Math::taxiCabMagnitude(baselineVelocity - calculatedLinearVelocity);
					float velocityError = velocityDiff / Math::taxiCabMagnitude(baselineVelocity);
					isEstimatedVelocaityInaccurate = (velocityError > 0.1f);
				}

                if (positionalError > MH_TOLERABLE_COMPRESSION_ERROR || isEstimatedVelocaityInaccurate || isPvOverThreshold)
				{	
#ifdef NETWORK_DEBUG
                    if (positionalError > MH_TOLERABLE_COMPRESSION_ERROR)
                    {
                        StandardOut::singleton()->printf(MESSAGE_INFO, "Calculated positional error (%f) too large, send full CFrame instead ", positionalError);
                    }
                    if (isEstimatedVelocaityInaccurate)
                    {
						StandardOut::singleton()->printf(MESSAGE_INFO, "Calculated velocity error too large send full CFrame instead");
                    }
					if (isPvOverThreshold)
					{
						StandardOut::singleton()->printf(MESSAGE_INFO, "PV is over threshold, send full CFrame instead");
					}
#endif
                    // disable cross packet compression for this round
                    crossPacketCompression = false;
                    // and remove the calculated baseline CFrame node
                    movementNodeList.pop_front();
                }
            }

            bitStream << crossPacketCompression;
            writeAssembly(bitStream, assembly, compressionType, crossPacketCompression);
            if (crossPacketCompression)
            {
                lastSendCFrame = calculatedCFrame;
                accumulatedError += 0.1f;
            }
            else
            {
                accumulatedError = 0.f;
                lastSendCFrame = currentCFrame;
            }

            VarInt<>::encode(bitStream, (unsigned int)(movementNodeList.size()));

            if (movementNodeList.size() > 0)
            {
                for (std::deque<MovementHistory::MovementNode>::iterator iter = movementNodeList.begin(); iter != movementNodeList.end(); iter++)
                {
                    MovementHistory::MovementNode node = *iter;
                    bitStream << node.translation.precisionLevel; // uint8_t
                    bitStream << node.translation.dX; // int8_t
                    bitStream << node.translation.dY; // int8_t
                    bitStream << node.translation.dZ; // int8_t
                    bitStream << node.delta2Ms; // uint8_t
                }
            }
        }
        else
        {
            hasMovement = false;
            bitStream << false; // no movement since last send

            writeMotorAngles(bitStream, assembly, compressionType);
        }
        return true;
    }


bool TopNErrorsPhysicsSender::isSleepingRootPrimitive(const PartInstance* part)
{
	const Primitive* prim = part->getConstPartPrimitive();
	if(prim->getWorld() == NULL || !Assembly::isAssemblyRootPrimitive(prim))
	{
		return false;
	}

	// Modification of the objects radius after removal from service means that 
	// its maxRadius will be dirty causing its error calculation to stop being
	// thread safe.
	const Assembly* assembly = prim->getConstAssembly();
	if( assembly->isComputedRadiusDirty() )
	{
		return false;
	}

	// The ServerReplicator applies additional restrictions.
	// Search for:  RBXASSERT(getConstMechanismRootMovingPart(part) == part)

	const Primitive* p = Mechanism::getConstRootMovingPrimitive(part->getConstPartPrimitive());
	return PartInstance::fromConstPrimitive(p) == part;
}
