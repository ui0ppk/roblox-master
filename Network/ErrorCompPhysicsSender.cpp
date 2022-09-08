/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */
#if 1 // removing deprecated senders, set to 0 when FastFlag RemoveUnusedPhysicsSenders is set to true and removed (delete this file)
#include "ErrorCompPhysicsSender.h"
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

using namespace RBX;
using namespace RBX::Network;



ErrorCompPhysicsSender::ErrorCompPhysicsSender(Replicator& replicator)
: PhysicsSender(replicator)
, stepId(0)
{
    if (replicator.isProtocolCompatible())
	    physicsPacketCache = shared_from(ServiceProvider::find<PhysicsPacketCache>(&replicator));
}

ErrorCompPhysicsSender::~ErrorCompPhysicsSender()
{
}



void ErrorCompPhysicsSender::step()
{
	/*

		First, update the error on all items that had been sent 
		after the last step.  These will have a stepId of 0.

		Also, visit some of the oldest items in the list as well.

	    TODO: Rather than picking a constant "countdown", pick a number that
		      is a fraction of the items sent last frame (This will
			  aid in auto-throttling how much we send).

	*/

	if (!physicsService)
	{
		physicsService = shared_from(ServiceProvider::find<PhysicsService>(&replicator));

		RBXASSERT(physicsService);		// datamodel should always create this

		// Get all current assemblies
		std::for_each(physicsService->begin(), physicsService->end(), boost::bind(&ErrorCompPhysicsSender::addNugget, this, _1));

		// Listen for assembly changes
		addingAssemblyConnection = physicsService->assemblyAddingSignal.connect(boost::bind(&ErrorCompPhysicsSender::onAddingAssembly, this, _1));
		//removedAssemblyConnection = physicsService->assemblyRemovedSignal.connect(boost::bind(&ErrorCompPhysicsSender::onRemovedAssembly, this, _1));
	}

	std::for_each(newMovingAssemblies.begin(), newMovingAssemblies.end(), boost::bind(&ErrorCompPhysicsSender::addNugget2, this, _1));
	newMovingAssemblies.clear();

	// Prepare the data needed to compute errors

	// TODO: const
	const ModelInstance* characterModel = NULL;
	CoordinateFrame focus;
	if (const Player* player = replicator.findTargetPlayer()) {
		if (player->hasCharacterHead(focus)) {
			characterModel = player->getConstCharacter();
		}
	}

	stepId++;		// can go for 100's of days at 30 fps until maxing out

	{
		int numOldVisits = 0;

		while (numOldVisits < 100)
		{
			NuggetList::iterator iter = nuggetUpdateList.begin();
			if (iter == nuggetUpdateList.end())
				break;

			Nugget* nugget = &(**iter);

			RBXASSERT(nugget->errorStepId <= stepId);

			// remove nugget if part is not in physics service sender list, it's not moving.
			if (!nugget->part->PhysicsServiceHook::is_linked())
			{
				removeNugget(nugget->part);
				continue;
			}

			if (nugget->errorStepId == stepId) {	
				break;								// we are done - first in the list is caught up	
			}
			else {
				if (nugget->errorStepId > 0)	{		// don't include just-sent items in the count of old items
					++numOldVisits;
				}
				nugget->computeError(focus, characterModel, stepId);
			}

			// resort nugget into send list
			nuggetSendList.erase(*iter);
			SortedNuggetList::iterator newPos = nuggetSendList.insert(*nugget);

			// update update iterator
			*iter = newPos;

			// move just updated item to end of update list
			nuggetUpdateList.splice(nuggetUpdateList.end(), nuggetUpdateList, iter);

			// update nugget iterator inside update list
			nugget->updateListIter = --nuggetUpdateList.end();

		}
	}
}

int ErrorCompPhysicsSender::sendPacket(int maxPackets, PacketPriority packetPriority,
		ReplicatorStats::PhysicsSenderStats* stats)
{
	const unsigned int maxStreamSize(replicator.getPhysicsMtuSize());

	boost::shared_ptr<RakNet::BitStream> bitStream;

	int packetCount = 0;

	SortedNuggetList::iterator iter = nuggetSendList.begin();

	// Iterate by error until 
	while (iter != nuggetSendList.end())
	{
		if (!bitStream) {
			if (packetCount>=maxPackets)
				break;

			bitStream.reset(new RakNet::BitStream(maxStreamSize));

			++packetCount;

			*bitStream << (unsigned char) ID_TIMESTAMP;
			RakNet::Time now = RakNet::GetTime();
			*bitStream << now;
			*bitStream << (unsigned char) ID_PHYSICS;
		}

		Nugget& nugget = *iter;

		// remove nugget if part is not in physics service sender list, it's not moving
		if (!nugget.part->PhysicsServiceHook::is_linked())
		{
			++iter;
			// remove will increment curBucket->iter
			removeNugget(nugget.part);
		}
		else
		{
			if (!sendPhysicsData(*bitStream, nugget)) {
				++iter;
				continue;
			}

			if (nugget.part->getConstPartPrimitive()->getConstAssembly()) 
			{
				Nugget::onSent(nugget);

				// move just sent item to front of update list
				nuggetUpdateList.splice(nuggetUpdateList.begin(), nuggetUpdateList, nugget.updateListIter);
				nugget.updateListIter = nuggetUpdateList.begin();

				RBXASSERT((*iter).lastSent == nugget.part->getCoordinateFrame());
				RBXASSERT((*iter).biggestSize == Nugget::minSize());
				RBXASSERT((*iter).errorStepId == 0);
				++iter;


			}
			else {	
				RBXASSERT(0);
			}
		}

		if (bitStream->GetNumberOfBytesUsed() > static_cast<int>(maxStreamSize)) {
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
	}

	return packetCount;
};


void ErrorCompPhysicsSender::onAddingAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	newMovingAssemblies.push_back(part);
}

void ErrorCompPhysicsSender::addNugget(PartInstance& part)
{
	addNugget2(shared_from(&part));
}

void ErrorCompPhysicsSender::addNugget2(shared_ptr<PartInstance> part)
{
	// try add item to map
	std::pair<NuggetMap::iterator, bool> result = nuggetMap.insert(std::make_pair(part, Nugget(part)));

	// add succeeded
	if (result.second)
	{
		// put new nugget to end of send list, it'll get updated next step()
		SortedNuggetList::iterator iter = nuggetSendList.insert(nuggetSendList.end(), result.first->second);
		nuggetUpdateList.push_front(iter);

		iter->updateListIter = nuggetUpdateList.begin();
		iter->radius = part->getPartPrimitive()->getAssembly()->getLastComputedRadius();

	}
}

void ErrorCompPhysicsSender::removeNugget(shared_ptr<const PartInstance> part)
{
	NuggetMap::iterator iter = nuggetMap.find(part);
	RBXASSERT(iter != nuggetMap.end());

	nuggetSendList.erase(*iter->second.updateListIter);
	nuggetUpdateList.erase(iter->second.updateListIter);
	nuggetMap.erase(iter);
}

void ErrorCompPhysicsSender::onRemovedAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<const PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	removeNugget(part);
}

bool ErrorCompPhysicsSender::sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget)
{
	sendDetailed = nugget.sendDetailed;
    CoordinateFrame cFrame = nugget.lastSent;
    float accumulatedError;
	return PhysicsSender::sendPhysicsData(bitStream, nugget.part.get(), sendDetailed, currentStepTimestamp, cFrame, accumulatedError, NULL);
}

void ErrorCompPhysicsSender::writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
	if (physicsPacketCache)
	{
		// look for this packet in the cache
		if (!physicsPacketCache->fetchIfUpToDate(assembly, (unsigned char)sendDetailed, bitStream))
		{
			// cache miss, recreate bit stream and update cache
			unsigned int startBit = bitStream.GetWriteOffset();

			PhysicsSender::writeAssembly(bitStream, assembly, compressionType);

			bitStream.SetReadOffset(startBit);

			if (!physicsPacketCache->update(assembly, (unsigned char)sendDetailed, bitStream, startBit, bitStream.GetNumberOfBitsUsed() - startBit))
			{
				// this item should be in the cache
				StandardOut::singleton()->print(MESSAGE_INFO, "cache update failed");
				RBXASSERT(0);
			}
		}
	}
	else
		PhysicsSender::writeAssembly(bitStream, assembly, compressionType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float ErrorCompPhysicsSender::Nugget::minDistance() {return 3.0f;}
float ErrorCompPhysicsSender::Nugget::maxDistance() {return 1000.0f;}
float ErrorCompPhysicsSender::Nugget::minSize()		{return 2.0f;}
float ErrorCompPhysicsSender::Nugget::maxSize()		{return 50.0f;}

ErrorCompPhysicsSender::Nugget::Nugget(shared_ptr<PartInstance> part)
	: part(part)			// indexes
	, error(0.0f)
	, errorStepId(-1)
	, lastSent(part->getCoordinateFrame())
	, biggestSize(minSize())
	, sendDetailed(true)
	, radius(0.0f)
{
}

// static
void ErrorCompPhysicsSender::Nugget::onSent(Nugget& nugget)			
{
	// part - no change
	// error - will be computed
	nugget.errorStepId = 0;
	nugget.lastSent = nugget.part->getCoordinateFrame();
	nugget.biggestSize = minSize();					// resets
}


void ErrorCompPhysicsSender::Nugget::computeError(const CoordinateFrame& focus, const ModelInstance* focusModel, int stepId)
{
	RBXASSERT(part->getConstPartPrimitive()->getConstAssembly());

	if ((stepId % 20) == 0)
	{
		const Assembly* assembly = part->getConstPartPrimitive()->getConstAssembly();
		RBXASSERT(assembly);

		radius = assembly->getLastComputedRadius();
	}

	const CoordinateFrame& cf = part->getCoordinateFrame();

	// 1. Biggest Size
	biggestSize = G3D::clamp(2*radius, biggestSize, maxSize());
		
	// current
	float currentDistance = minDistance();			// There is no center of interest.  Location doesn't affect error
	if (focusModel)	{
		currentDistance = Math::taxiCabMagnitude(cf.translation - focus.translation) - radius;
		currentDistance = G3D::clamp(currentDistance, minDistance(), maxDistance());
	}

	// 2. Error
	// 3. Send Detailed
	if (part->isDescendantOf(focusModel)) {				// the character!
		error = std::numeric_limits<double>::max();
		sendDetailed = true;
	}
	else {
		const float positionalError = Math::taxiCabMagnitude(cf.translation - lastSent.translation);
		const float rotationError = Math::sumDeltaAxis(cf.rotation, lastSent.rotation) * radius * 0.2f;	// factor down - we are summing the axis
		error = biggestSize * biggestSize * (rotationError + positionalError) / currentDistance;
		sendDetailed = currentDistance < 20.0f;
	}

	// 4. errorStepId
	errorStepId = stepId;
}
#endif  // removing deprecated senders