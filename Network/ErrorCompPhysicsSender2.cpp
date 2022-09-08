/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */
#if 1 // removing deprecated senders, set to 0 when FastFlag RemoveUnusedPhysicsSenders is set to true and removed (delete this file)

#include "ErrorCompPhysicsSender2.h"
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

#define NUM_GROUPS 4

ErrorCompPhysicsSender2::ErrorCompPhysicsSender2(Replicator& replicator)
: PhysicsSender(replicator)
, bucketIndex(0)
, stepId(0)
{

    if (replicator.isProtocolCompatible())
	    physicsPacketCache = shared_from(ServiceProvider::find<PhysicsPacketCache>(&replicator));

	for (int i = 0; i < NUM_GROUPS; i++)
	{
		nuggetSendGroups.push_back(Bucket());
	}
}

ErrorCompPhysicsSender2::~ErrorCompPhysicsSender2()
{
}



void ErrorCompPhysicsSender2::step()
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
		std::for_each(physicsService->begin(), physicsService->end(), boost::bind(&ErrorCompPhysicsSender2::addNugget, this, _1));

		// Listen for assembly changes
		// Optimization: onAddingAssemly simply adds new item to a STL list, more expensive operations are deferred til later
		addingAssemblyConnection = physicsService->assemblyAddingSignal.connect(boost::bind(&ErrorCompPhysicsSender2::onAddingAssembly, this, _1));
		//removedAssemblyConnection = physicsService->assemblyRemovedSignal.connect(boost::bind(&ErrorCompPhysicsSender2::onRemovedAssembly, this, _1));
	}

	// Optimization: adding and removing nugget is expensive, by deferring it to here we allow it to run concurrently  
	// process new moving assemblies
	std::for_each(newMovingAssemblies.begin(), newMovingAssemblies.end(), boost::bind(&ErrorCompPhysicsSender2::addNugget2, this, _1));
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

	// Compute errors
	{
		int numOldVisits = 0;

		NuggetUpdateList::iterator iter = nuggetUpdateList.begin();
		while (numOldVisits <= 0)
		{
			if (iter == nuggetUpdateList.end())
				break;

			Nugget* nugget = &(**iter);
			RBXASSERT(nugget->errorStepId <= stepId);

			// remove nugget if part is not in physics service sender list, it's not moving.
			if (!nugget->part->PhysicsServiceHook::is_linked())
			{
				iter++;
				removeNugget(nugget->part);
				continue;
			}

			int oldGroup = nugget->groupId;

			if (nugget->errorStepId == stepId) {	
				break;								// we are done - first in the list is caught up	
			}
			else {
				if (nugget->errorStepId > 0)	{		// don't include just-sent items in the count of old items
					++numOldVisits;
				}
				nugget->computeDeltaError(focus, characterModel, stepId);
			}

			// add to group
			NuggetIterator sendIter = nuggetSendGroups[nugget->groupId].splice(nuggetSendGroups[nugget->groupId].iter, &nuggetSendGroups[oldGroup], *iter);

			RBXASSERT(nuggetSendGroups[nugget->groupId].size() > 0 ? nuggetSendGroups[nugget->groupId].iter != nuggetSendGroups[nugget->groupId].nuggetList.end() : true);
			RBXASSERT(nuggetSendGroups[oldGroup].size() > 0 ? nuggetSendGroups[oldGroup].iter != nuggetSendGroups[oldGroup].nuggetList.end() : true);

			*nugget->mapIter->second = sendIter;
			
			iter++;
		}
	}
}

void ErrorCompPhysicsSender2::calculateSendCount()
{
	bool last = true;
	int prev = nuggetSendGroups.size() - 1;
	for (int i = nuggetSendGroups.size() - 1; i >= 0; i--)
	{
		if (nuggetSendGroups[i].size() == 0)
		{
			nuggetSendGroups[i].targetSentCount = 0;
			nuggetSendGroups[i].sendCount = 0;
			continue;
		}
		// last non empty bucket always send 1
		if (last)
		{
			nuggetSendGroups[i].targetSentCount = 1;
			nuggetSendGroups[i].sendCount = 0;
			prev = i;
			last = false;
			continue;
		}

		// how many to send from this bucket before sending 1 item in next bucket, multiply by actual send count of next bucket count to keep ratio at 2:1
		float count = nuggetSendGroups[i].size() * 2.0f / nuggetSendGroups[prev].size() * nuggetSendGroups[prev].targetSentCount;

		nuggetSendGroups[i].targetSentCount = G3D::iCeil(count);
		
		prev = i;
	}
}


int ErrorCompPhysicsSender2::sendPacket(int maxPackets, PacketPriority packetPriority,
		ReplicatorStats::PhysicsSenderStats* stats)
{
	const unsigned int maxStreamSize(replicator.getPhysicsMtuSize());

	boost::shared_ptr<RakNet::BitStream> bitStream;

	int packetCount = 0;

	bool done = false;

	calculateSendCount();

	// flag is a set of bits where each one representing a bucket, 1 == has stuff to send 
	int bucketFlag = (1 << nuggetSendGroups.size()) - 1;
	while (!done && bucketFlag)
	{
		// reset bucket iterator
		if (bucketIndex >= nuggetSendGroups.size())
			bucketIndex = 0;

		Bucket *curBucket = &nuggetSendGroups[bucketIndex];

		if (curBucket->nuggetList.size() == 0)
		{
			bucketFlag &= ~(1 << bucketIndex);
			bucketIndex++;
			continue;
		}
		else if (curBucket->iter->sendStepId == stepId)
		{
			bucketFlag &= ~(1 << bucketIndex);
			bucketIndex++;
			continue;
		}

		RBXASSERT(curBucket->iter != curBucket->nuggetList.end());

		while (curBucket->sendCount < curBucket->targetSentCount)
		{
			if (!bitStream) 
			{
				if (packetCount>=maxPackets)
				{
					done = true;
					break;
				}

				bitStream.reset(new RakNet::BitStream(maxStreamSize));

				++packetCount;

				*bitStream << (unsigned char) ID_TIMESTAMP;

				RakNet::Time timestamp = RakNet::GetTime();
				if (curBucket->iter->part->computeNetworkOwnerIsSomeoneElse())
				{
					// retrieve the original packet send time of the part
					RBXASSERT(timestamp >= curBucket->iter->part->raknetTime);
					if (curBucket->iter->part->raknetTime)
						timestamp = curBucket->iter->part->raknetTime;
				}

				*bitStream << timestamp;
				*bitStream << (unsigned char) ID_PHYSICS;
			}

			RBXASSERT(curBucket->iter != curBucket->nuggetList.end());

			// remove nugget if part is not in physics service sender list, it's not moving
			if (!curBucket->iter->part->PhysicsServiceHook::is_linked())
			{
				removeNugget(curBucket->iter->part);

				if (curBucket->size() == 0)
					break;
			}
			else
			{
				RBXASSERT(curBucket->iter->sendStepId <= stepId);
				writeNugget(*bitStream, *curBucket->iter);

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

			curBucket->sendCount++;
			curBucket->iter++;
			
			// reset iterator for next time in loop
			if (curBucket->iter == curBucket->nuggetList.end())
			{
				curBucket->iter = curBucket->nuggetList.begin();
				break;
			}

			RBXASSERT(curBucket->iter != curBucket->nuggetList.end());
		}

		if (curBucket->sendCount >= curBucket->targetSentCount)
			curBucket->sendCount = 0;

		bucketIndex++;
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

bool ErrorCompPhysicsSender2::writeNugget(RakNet::BitStream& bitStream, Nugget& nugget)
{
	if (!sendPhysicsData(bitStream, nugget)) {
		nugget.sendStepId = stepId;
		return false;
	}

	if (nugget.part->getConstPartPrimitive()->getConstAssembly()) 
	{
		nugget.onSent(stepId);
		RBXASSERT(nugget.biggestSize == Nugget::minSize());
		RBXASSERT(nugget.errorStepId == 0);

		// move just sent item to front of update list
		nuggetUpdateList.splice(nuggetUpdateList.begin(), nuggetUpdateList, nugget.mapIter->second);

		// update iterators in map
		nugget.mapIter->second = nuggetUpdateList.begin();

		return true;
	}
	else
		RBXASSERT(0);

	return false;
}

void ErrorCompPhysicsSender2::onAddingAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	newMovingAssemblies.push_back(part);
}

void ErrorCompPhysicsSender2::addNugget(PartInstance& part)
{
	addNugget2(shared_from(&part));
}

void ErrorCompPhysicsSender2::addNugget2(shared_ptr<PartInstance> part)
{
	// try add item to map
	std::pair<NuggetMap::iterator, bool> result = nuggetMap.insert(std::make_pair(part, nuggetUpdateList.end()));

	// add succeeded
	if (result.second)
	{
		int group = 0;
		NuggetIterator iter = nuggetSendGroups[group].push_back(part);
		iter->groupId = group;

		// add to send iterator to front of update list, so the nugget will be updated in step()
		nuggetUpdateList.push_front(iter);

		// set update iterator inside map
		result.first->second = nuggetUpdateList.begin();

		// cache map iterator inside nugget
		iter->mapIter = result.first;
	}
}

void ErrorCompPhysicsSender2::removeNugget(shared_ptr<const PartInstance> part)
{
	NuggetMap::iterator iter = nuggetMap.find(part);
	RBXASSERT(iter != nuggetMap.end());

	NuggetIterator nuggetIter = *iter->second;

	int groupID = nuggetIter->groupId;

	nuggetSendGroups[groupID].erase(nuggetIter);

	nuggetUpdateList.erase(iter->second);

	nuggetMap.erase(iter);
}

void ErrorCompPhysicsSender2::onRemovedAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<const PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	NuggetMap::iterator iter = nuggetMap.find(part);
	RBXASSERT(iter != nuggetMap.end());

	NuggetIterator nuggetIter = *iter->second;

	int groupID = nuggetIter->groupId;

	nuggetSendGroups[groupID].erase(nuggetIter);

	nuggetUpdateList.erase(iter->second);

	nuggetMap.erase(iter);
}

bool ErrorCompPhysicsSender2::sendPhysicsData(RakNet::BitStream& bitStream, const Nugget& nugget)
{
	sendDetailed = nugget.sendDetailed;
    CoordinateFrame cFrame = nugget.part->getCoordinateFrame();
    float accumulatedError;
	return PhysicsSender::sendPhysicsData(bitStream, nugget.part.get(), sendDetailed, currentStepTimestamp, cFrame, accumulatedError, NULL);
}

void ErrorCompPhysicsSender2::writeAssembly(RakNet::BitStream& bitStream, const Assembly* assembly, Compressor::CompressionType compressionType, bool crossPacketCompression)
{
	if (physicsPacketCache)
	{
		// look for this packet in the cache
		if (!physicsPacketCache->fetchIfUpToDate(assembly, (unsigned char)sendDetailed, bitStream))
		{
			// cache miss, recreate bit stream and update cache
			RakNet::BitStream stream;

			unsigned int startBit = bitStream.GetWriteOffset();

			PhysicsSender::writeAssembly(bitStream, assembly, compressionType);

			bitStream.SetReadOffset(startBit);

			if (!physicsPacketCache->update(assembly, (unsigned char)sendDetailed, bitStream, startBit, bitStream.GetNumberOfBitsUsed() - startBit))
			{
				// this item should be in the cache
				StandardOut::singleton()->print(MESSAGE_INFO, "cache update failed");
				RBXASSERT(0);
			}

			stream.SetReadOffset(0);
			bitStream.Write(stream);
		}
	}
	else
		PhysicsSender::writeAssembly(bitStream, assembly, compressionType);
}

ErrorCompPhysicsSender2::NuggetIterator ErrorCompPhysicsSender2::Bucket::push_back(shared_ptr<PartInstance> part)
{
	nuggetList.push_back(ErrorCompPhysicsSender2::Nugget(part));

	// set iterator at start of list if 1st item
	if (nuggetList.size() == 1)
		iter = nuggetList.begin();

	return --nuggetList.end();
}

ErrorCompPhysicsSender2::NuggetIterator ErrorCompPhysicsSender2::Bucket::splice(const NuggetIterator where, Bucket* from, const NuggetIterator first)
{
	if (this == from && where == first)
		return where;

	// splicing within self does not cause iterators to become invalid
	if (this != from && from->iter == first)
	{
		from->iter++;

		if (from->iter == from->nuggetList.end())
			from->iter = from->nuggetList.begin();
	}

	if (nuggetList.size() > 0)
	{
		nuggetList.splice(where, from->nuggetList, first);
		NuggetIterator temp = where;
		return --temp;
	}
	else
	{
		nuggetList.splice(nuggetList.begin(), from->nuggetList, first);
		iter = nuggetList.begin();
		return nuggetList.begin();
	}
}

void ErrorCompPhysicsSender2::Bucket::erase(const NuggetIterator where)
{
	if (iter == where)
	{
		iter = nuggetList.erase(where);
		if (iter == nuggetList.end())
			iter = nuggetList.begin();
	}
	else
		nuggetList.erase(where);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float ErrorCompPhysicsSender2::Nugget::minDistance()	{return 3.0f;}
float ErrorCompPhysicsSender2::Nugget::maxDistance()	{return 800.0f;}
float ErrorCompPhysicsSender2::Nugget::minSize()		{return 2.0f;}
float ErrorCompPhysicsSender2::Nugget::maxSize()		{return 25.0f;}
float ErrorCompPhysicsSender2::Nugget::midArea()		{return 312.5f;}

ErrorCompPhysicsSender2::Nugget::Nugget(shared_ptr<PartInstance> part)
	: part(part)			// indexes
	, errorStepId(-1)
	, sendStepId(-1)
	, biggestSize(minSize())
	, sendDetailed(true)
	, groupId(NUM_GROUPS-1)
{
}

ErrorCompPhysicsSender2::Nugget::~Nugget()
{
}

// static
void ErrorCompPhysicsSender2::Nugget::onSent(Nugget& nugget)			
{
	// part - no change
	nugget.errorStepId = 0;
	nugget.biggestSize = minSize();					// resets
}

void ErrorCompPhysicsSender2::Nugget::onSent(int stepId)
{
	// part - no change
	errorStepId = 0;
	sendStepId = stepId;
	biggestSize = minSize();					// resets
}

void ErrorCompPhysicsSender2::Nugget::computeDeltaError(const CoordinateFrame& focus, const ModelInstance* focusModel, int stepId)
{
	const Assembly* assembly = part->getConstPartPrimitive()->getConstAssembly();
	RBXASSERT(assembly);

	float radius = assembly->getLastComputedRadius();

	// 1. Biggest Size
	biggestSize = G3D::clamp(2*radius, biggestSize, maxSize());

	// current
	float currentDistance = minDistance();			// There is no center of interest.  Location doesn't affect error
	if (focusModel)	{
		currentDistance = Math::taxiCabMagnitude(part->getCoordinateFrame().translation - focus.translation) - radius;
		currentDistance = G3D::clamp(currentDistance, minDistance(), maxDistance());
	}

	float velocityError;
	float sizeError;

	// 2. Error
	// 3. Send Detailed
	if (part->isDescendantOf(focusModel)) {				// the character!
		groupId = 0;
		sendDetailed = true;
	}
	else 
	{
		// which group this object belongs to is initially set by the distance to player, then modified by the velocity and size
		float distance = currentDistance * NUM_GROUPS / maxDistance();
		
		float lVel = G3D::clamp(part->getLinearVelocity().squaredMagnitude(), 0.0f, 10000.0f);
		float rVel = G3D::clamp(part->getRotationalVelocity().squaredMagnitude(), 0.0f, 10000.0f);

		velocityError = -1 + ((lVel / 5000.0f * 0.80f) + (rVel/ 5000.0f * 0.25f));

		// -1 + (0 to 2), 0 == small object, 2 == big object
		sizeError = -1 + biggestSize * biggestSize / midArea();

		groupId = G3D::iRound(distance - sizeError - velocityError);
		groupId = G3D::clamp((double)groupId, 0, NUM_GROUPS - 1);

		sendDetailed = currentDistance < 20.0f;
	}

	// 4. errorStepId
	errorStepId = stepId;

	//const_cast<PartInstance*>(part.get())->setColor3(Color::colorFromError(groupId));

}
#endif  // removing deprecated senders