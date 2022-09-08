#include "Replicator.StreamJob.h"
#include "Replicator.JoinDataItem.h"
#include "Replicator.ItemSender.h"
#include "ClientReplicator.h"

#include "rbx/DenseHash.h"
#include "V8DataModel/ModelInstance.h"
#include "v8world/ContactManager.h"
#include "v8world/ContactManagerSpatialHash.h"
#include "util/StreamRegion.h"
#include "Players.h"
#include "Humanoid/Humanoid.h"

using namespace RBX;
using namespace RBX::Network;

LOGVARIABLE(NetworkStreaming, 0)
DYNAMIC_LOGGROUP(PartStreamingRequests)
DYNAMIC_LOGGROUP(MaxJoinDataSizeKB)
DYNAMIC_FASTINTVARIABLE(MaxStreamPacketsPerStep, 16)
DYNAMIC_FASTINTVARIABLE(MaxServerStreamRegionRadius, 16)
DYNAMIC_FASTINTVARIABLE(StreamJobPriorityAmplifierRadius, 0)
DYNAMIC_FASTINTVARIABLE(MaxConsecutiveStreamJobWorkLoad, 1)

DYNAMIC_FASTFLAG(CyclicExecutiveForServerTweaks)

DeserializedStreamDataItem::DeserializedStreamDataItem()
{
	type = Item::ItemTypeStreamData;
}

void DeserializedStreamDataItem::process(Replicator& replicator)
{
	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
	rep->readStreamDataItem(this);
}

Replicator::StreamJob::StreamRegionIterator::StreamRegionIterator(Replicator& replicator) 
: worldMinRegion(0,0,0)
, worldMaxRegion(0,0,0)
, currentRegion(0,0,0)
, centerRegion(Vector3int32::maxInt())
, regionRadius(0)
, clientMaxRegionRadius(0x7FFF)
, serverMaxRegionRadius(DFInt::MaxServerStreamRegionRadius)
, centerPosition(0, -10000, 0) // this should be a safe number to trigger a reset on first use, because anything below -500 is automatically removed by physics engine
{
    workspace = Workspace::findWorkspace(&replicator);
}

Extents Replicator::StreamJob::StreamRegionIterator::possibleExtentsStreamed() const {
	Vector3int32 min = (centerRegion + Vector3int32(-regionRadius, -regionRadius, -regionRadius)).value();
	Vector3int32 max = (centerRegion + Vector3int32(regionRadius, regionRadius, regionRadius)).value();
	return Extents(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

bool Replicator::StreamJob::StreamRegionIterator::resetCenter(const Vector3 &worldPos, bool force) 
{
	StreamRegion::Id newCenterRegion = StreamRegion::regionContainingWorldPosition(worldPos);

	if (centerRegion == newCenterRegion && !force)
		return false;

	Vector3 newCenterPos = (newCenterRegion.value() * StreamRegion::gridCellDimension() + StreamRegion::gridCellHalfDimension()).toVector3();

	float distToCurCenterPos = (worldPos - centerPosition).squaredMagnitude();
	float distToNewCenterPos = (worldPos - newCenterPos).squaredMagnitude();

	if (force || (fabsf(distToNewCenterPos - distToCurCenterPos) > kStreamCenterResetThreshold))
	{
		centerPosition = newCenterPos;
		currentRegion = centerRegion = newCenterRegion;

		regionRadius = 0;
		updateWorldExtents();

		FASTLOG3F(DFLog::PartStreamingRequests, "centerPosition: (%f,%f,%f)", centerPosition.x, centerPosition.y, centerPosition.z);
		FASTLOG3(DFLog::PartStreamingRequests, "worldMinRegion: (%d,%d,%d)", worldMinRegion.value().x, worldMinRegion.value().y, worldMinRegion.value().z);
		FASTLOG3(DFLog::PartStreamingRequests, "worldMaxRegion: (%d,%d,%d)", worldMaxRegion.value().x, worldMaxRegion.value().y, worldMaxRegion.value().z);
		FASTLOG3(DFLog::PartStreamingRequests, "centerRegion: (%d,%d,%d)", centerRegion.value().x, centerRegion.value().y, centerRegion.value().z);

		return true;
	}

	return false;
}

void Replicator::StreamJob::StreamRegionIterator::updateWorldExtents()
{
	if (workspace) 
	{
		static Extents maxExtents = Extents(Vector3(-2e6, -2e6, -2e6), Vector3(2e6, 2e6, 2e6));

		Extents worldExtents = workspace->computeExtentsWorldFast();
		worldExtents = worldExtents.clampInsideOf(maxExtents);

		int gridSize = StreamRegion::Id::streamGridCellSizeInStuds();
		StreamRegion::Id newWorldMinRegion = StreamRegion::regionContainingWorldPosition(worldExtents.min() - Vector3(gridSize, gridSize, gridSize));
		StreamRegion::Id newWorldMaxRegion = StreamRegion::regionContainingWorldPosition(worldExtents.max() + Vector3(gridSize, gridSize, gridSize));

		if (worldMaxRegion != newWorldMaxRegion)
		{
			worldMaxRegion = newWorldMaxRegion;
			regionRadius = 0;
		}

		if (worldMinRegion != newWorldMinRegion)
		{
			worldMinRegion = newWorldMinRegion;
			regionRadius = 0;
		}

		RBXASSERT(worldMinRegion != worldMaxRegion);
	}
	else
		RBXASSERT(false);
}

bool Replicator::StreamJob::StreamRegionIterator::getNextRegion(StreamRegion::Id &regionId)
{
	RBXASSERT(worldMinRegion != worldMaxRegion);

	Vector3int32 current = currentRegion.value();

	if (centerRegion.value().x > worldMaxRegion.value().x || centerRegion.value().y > worldMaxRegion.value().y || centerRegion.value().z > worldMaxRegion.value().z ||
		centerRegion.value().x < worldMinRegion.value().x || centerRegion.value().y < worldMinRegion.value().y || centerRegion.value().z < worldMinRegion.value().z)
		return false;

	while (regionRadius <= getMaxRegionRadius()) {
		Vector3int32 minRegion = (centerRegion + Vector3int32(-regionRadius, -regionRadius, -regionRadius)).value();
		Vector3int32 maxRegion = (centerRegion + Vector3int32(regionRadius, regionRadius, regionRadius)).value();

		// we've iterated over everything
		if (minRegion.x < worldMinRegion.value().x && minRegion.y < worldMinRegion.value().y && minRegion.z < worldMinRegion.value().z && 
			maxRegion.x > worldMaxRegion.value().x && maxRegion.y > worldMaxRegion.value().y && maxRegion.z > worldMaxRegion.value().z)
			return false;

		// clamp outer box to world max extent
		Vector3int32 adjustedMinRegion = minRegion.max(worldMinRegion.value());
		Vector3int32 adjustedMaxRegion = maxRegion.min(worldMaxRegion.value());

		if (current.y > adjustedMaxRegion.y) {
			current.y = adjustedMinRegion.y;

			++current.z;
			if (current.z > adjustedMaxRegion.z) {
				current.z = adjustedMinRegion.z;

				++current.x;
				if (current.x > adjustedMaxRegion.x) {
					++regionRadius;
					current = centerRegion.value() - Vector3int32(regionRadius, regionRadius, regionRadius);
					current = current.max(worldMinRegion.value());
					currentRegion = StreamRegion::Id(current);
				}
			}
		} else {
			if ( current.x > minRegion.x && current.x < maxRegion.x &&
				current.y > minRegion.y && current.y < maxRegion.y &&
				current.z > minRegion.z && current.z < maxRegion.z)
            {
                ++current.y;
                currentRegion = StreamRegion::Id(current);
                continue;
            }
			regionId = StreamRegion::Id(current);
			++current.y;
			currentRegion = StreamRegion::Id(current);
			return true;
		}
	}

	return false;
}

bool Replicator::StreamJob::StreamDataItem::write(RakNet::BitStream& bitStream) 
{
	writeItemType(bitStream, ItemTypeStreamData);

	Replicator::StreamJob::RegionIteratorSuccessor successorBitMask = Replicator::StreamJob::ITER_NONE;
	if (regionId.value().y == lastSentRegionId.value().y + 1)
	{
		if (regionId.value().x == lastSentRegionId.value().x && regionId.value().z == lastSentRegionId.value().z)
		{
			successorBitMask = Replicator::StreamJob::ITER_INCY;
		}
	}
	else if (regionId.value().z == lastSentRegionId.value().z + 1)
	{
		if (regionId.value().x == lastSentRegionId.value().x && regionId.value().y == lastSentRegionId.value().y)
		{
			successorBitMask = Replicator::StreamJob::ITER_INCZ;
		}
	}
	else if (regionId.value().x == lastSentRegionId.value().x + 1)
	{
		if (regionId.value().y == lastSentRegionId.value().y && regionId.value().z == lastSentRegionId.value().z)
		{
			successorBitMask = Replicator::StreamJob::ITER_INCX;
		}
	}
	bitStream << (bool)((successorBitMask & 1) > 0);
	bitStream << (bool)((successorBitMask & 2) > 0);
	if (successorBitMask == Replicator::StreamJob::ITER_NONE)
	{
		bitStream << regionId;
	}

	lastSentRegionId = regionId;

	instancesSentLastWrite = writeInstances(bitStream);

	return empty();
}

shared_ptr<DeserializedStreamDataItem> Replicator::StreamJob::StreamDataItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedStreamDataItem> deserializedData(new DeserializedStreamDataItem());

	bool lowBit, highBit;
	inBitstream >> lowBit;
	inBitstream >> highBit;
	deserializedData->successorBitMask = (Replicator::StreamJob::RegionIteratorSuccessor)((int)lowBit + (((int)highBit)<<1));
	if ((Replicator::StreamJob::RegionIteratorSuccessor)(deserializedData->successorBitMask) == Replicator::StreamJob::ITER_NONE)
	{
		inBitstream >> deserializedData->id;
	}

	deserializedData->deserializedJoinDataItem = shared_polymorphic_downcast<DeserializedJoinDataItem>(JoinDataItem::read(replicator, inBitstream));

	return deserializedData;
}


Replicator::StreamJob::StreamJob(Replicator& replicator)
:ReplicatorJob("Replicator StreamData", replicator, DataModelJob::DataOut)
,regionIterator(replicator)
,dataSendRate(replicator.settings().getDataSendRate())
,packetPriority(replicator.settings().getDataSendPriority())
,numClientInstanceQuota(0)
,numCollectedInstances(0)
,readyToWork(false)
,lastSentRegionId(0x1FFFFFFF, 0x1FFFFFFF, 0x1FFFFFFF)
,numPacketsPerStep(DFInt::MaxStreamPacketsPerStep)
{
	cyclicExecutive = true;
	cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessIncoming;

	if (Workspace* workspace = Workspace::findWorkspace(&replicator)) {
		if (World* world = workspace->getWorld()) {
			spatialHash = world->getContactManager()->getSpatialHash();
		}

		regionIterator.updateWorldExtents();
	}

	RBXASSERT(spatialHash);
	if (spatialHash) {
		spatialHash->registerCoarseMovementCallback(this);
	}


}

Replicator::StreamJob::~StreamJob() {
    unregisterCoarsePrimitiveCallback();
	clearPendingItems();
}

void Replicator::StreamJob::unregisterCoarsePrimitiveCallback()
{
    if (Workspace* workspace = Workspace::findWorkspace(replicator.get())) {
        if (World* world = workspace->getWorld()) {
            spatialHash = world->getContactManager()->getSpatialHash();
            if (spatialHash)
            {
                StandardOut::singleton()->printf(MESSAGE_INFO, "unregisterCoarseMovementCallback() from ~StreamJob()");
                spatialHash->unregisterCoarseMovementCallback(this);
            }
        }
    }
}

void Replicator::StreamJob::updateClientQuota(int diff, short maxRegionRadius) {
	numClientInstanceQuota += diff;
    RBXASSERT(numClientInstanceQuota>=0);
    if (numClientInstanceQuota == 0)
    {
        clearPendingItems();
    }
    regionIterator.setClientMaxRegionRadius(maxRegionRadius);

    FASTLOG2(DFLog::PartStreamingRequests, "Received new client instance quota: %d, max region radius: %d", numClientInstanceQuota, maxRegionRadius);
    if (replicator->settings().printStreamInstanceQuota)
    {
        StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Received new client instance quota: %d, max region radius: %d", numClientInstanceQuota, maxRegionRadius);
    }
}

bool Replicator::StreamJob::isRegionCollected(const StreamRegion::Id& regionId) const
{
    return collectedRegions.end() != collectedRegions.find(regionId);
}

bool Replicator::StreamJob::isRegionInPendingStreamItemQueue(const StreamRegion::Id& regionId) const
{
    for (std::deque<Replicator::StreamJob::StreamDataItem*>::const_iterator iter = pendingStreamItems.begin(); iter != pendingStreamItems.end(); iter++)
    {
        if ((*iter)->getRegionId() == regionId)
        {
            return true;
        }
    }
    return false;
}

bool Replicator::StreamJob::isInStreamedRegions(const Extents& ext) const
{
	StreamRegion::IdExtents regionExtents;
	regionExtents.low = StreamRegion::regionContainingWorldPosition(ext.min());
	regionExtents.high = StreamRegion::regionContainingWorldPosition(ext.max());

	StreamRegion::Id found;
	if (regionExtents.intersectsContainer(collectedRegions, &found))
	{
		return !isRegionInPendingStreamItemQueue(found);
	}

	return false;
}

PartInstance* findRootPartInstanceAncestor(Instance* instance, PartInstance* bestSoFar)
{
	if (instance == NULL)
	{
		return bestSoFar;
	}

	PartInstance* instanceAsPart = Instance::fastDynamicCast<PartInstance>(instance);

	return findRootPartInstanceAncestor(instance->getParent(),
		instanceAsPart ? instanceAsPart : bestSoFar);
}

void Replicator::StreamJob::readRegionRemoval(RakNet::BitStream& bitStream)
{
	StreamRegion::Id regionId;
	bitStream >> regionId;

	collectedRegions.erase(regionId);

	// If we remove something inside our streamed radius, reset the streaming location
	// and streamed radius so we can re-stream it when possible.
	const Vector3int32 &vv = regionId.value();
	Vector3 regionMin(vv.x, vv.y, vv.z);
	if (regionIterator.possibleExtentsStreamed().contains(regionMin)) {
		regionIterator.resetCenter(regionIterator.getCenterPosition(), true);
	}

	boost::uint32_t numGuids;
	bitStream >> numGuids;

	bool compressed;
	bitStream >> compressed;

	RakNet::BitStream decompressedIds;
	RakNet::BitStream* idList;

	if (compressed)
	{
        Replicator::decompressBitStream(bitStream, decompressedIds);

		idList = &decompressedIds;
	}
	else
	{
		idList = &bitStream;
	}

	for (boost::uint32_t i = 0; i < numGuids; ++i)
	{
		Guid::Data tmp;
		replicator->deserializeId(*idList, tmp);
		receiveInstanceGcMessage(tmp);
	}
}

void Replicator::StreamJob::readInstanceRemoval(RakNet::BitStream& bitStream)
{
	Guid::Data id;
	replicator->deserializeId(bitStream, id);
	receiveInstanceGcMessage(id);
}

void Replicator::StreamJob::receiveInstanceGcMessage(const Guid::Data& id)
{
	shared_ptr<Instance> instance;
	replicator->guidRegistry->lookupByGuid(id, instance);

	RepConts::iterator iter = replicator->replicationContainers.find(instance.get());
	if (iter != replicator->replicationContainers.end())
	{
		// Figure out if the gc is legit, or if the server should re-send the
		// instance to the client because its state has changed since the client
		// GC'ed.
		PartInstance* rootAncestor = findRootPartInstanceAncestor(instance.get(), NULL);
		bool shouldReSend = rootAncestor == NULL;
		if (rootAncestor != NULL)
		{
			Primitive* rootAncestorPrim = rootAncestor->getPartPrimitive();

			StreamRegion::IdExtents idExtents = StreamRegion::regionExtentsFromContactManagerLevelAndExtents(
				rootAncestorPrim->getSpatialNodeLevel(),
				rootAncestorPrim->getOldSpatialExtents());

			shouldReSend = idExtents.intersectsContainer(collectedRegions);
		}

		if (shouldReSend)
		{
			replicator->addToPendingItemsList(instance);
		}
		else
		{
			// be sure not to do any recursive removal, the client will send an exact list
			// of items to remove
			replicator->closeReplicationItem(iter->second);
			replicator->replicationContainers.erase(iter);
		}
	}
}

bool Replicator::StreamJob::isAreaInStreamedRadius(const Vector3& center, float radius) const
{
	Extents ext = Extents::fromCenterRadius(center, radius);
	Extents streamedExt = Extents::fromCenterRadius(regionIterator.getCenterPosition(), regionIterator.getRadius() * StreamRegion::Id::streamGridCellSizeInStuds());

	return Extents::overlapsOrTouches(ext, streamedExt);
}

void Replicator::StreamJob::setupListeners(Player* player)
{
	RBXASSERT(player == replicator->findTargetPlayer());
	// listen to spawn signal (when player die and respawn), this is used to pre-compute spawn location and reset streaming starting point
	playerSpawningConnection = player->nextSpawnLocationChangedSignal.connect(boost::bind(&StreamJob::setStreamCenterAndSentMinRegions, this, _1, true));
    playerCharacterAddedConnection = player->characterAddedSignal.connect(boost::bind(&StreamJob::onPlayerCharacterAdd, this, _1));
}

void Replicator::StreamJob::adjustSimulationOwnershipRange(Region2::WeightedPoint* updatee) {
	// TODO: FIX THIS
	// This is an approximation. This does not ensure that an entire assembly
	// has been streamed before allowing it to be owned by the player.
	float completedSendRadius = (1 + regionIterator.getRadius()) * StreamRegion::Id::streamGridCellSizeInStuds() -
		(regionIterator.getCenterPosition().xz() - updatee->point).length();
	updatee->radius = std::min(updatee->radius, completedSendRadius);
}

float Replicator::StreamJob::getPriorityAmplifier()
{
    // Is the client running out of regions?
    float amplifier = (float)(DFInt::StreamJobPriorityAmplifierRadius - regionIterator.getRadius());
    // If the client hasn't streamed in moderate regions, we increase the stream job priority by increasing the error
    // This is to help speeding up streaming when server resource is constrained.
    if (amplifier > 1.f && numClientInstanceQuota > 1)
    {
        amplifier = sqrt(amplifier);
        return amplifier;
    }
    return 1.0f;
}

Time::Interval Replicator::StreamJob::sleepTime(const Stats& stats)
{
    return computeStandardSleepTime(stats, dataSendRate * getPriorityAmplifier());
}

TaskScheduler::Job::Error Replicator::StreamJob::error( const Stats& stats)
{
	Error result;

	if (replicator)
	{
		if (!canSendPacket(replicator, packetPriority))
			return result;

		if (DFFlag::CyclicExecutiveForServerTweaks)
		{
			result = computeStandardErrorCyclicExecutiveSleeping(stats, dataSendRate);
		}
		else
		{
		result = computeStandardError(stats, dataSendRate);
        result.error *= getPriorityAmplifier();
    }
    }

	return result;
}

bool Replicator::StreamJob::streamTerrain(StreamDataItem &item) {
	const StreamRegion::Id &regionId = item.getRegionId();
	if (!item.didStreamTerrain() && regionId.isRegionInTerrainBoundaries()) {
		FASTLOG3(DFLog::PartStreamingRequests, "Streaming terrain region (%d,%d,%d)",
			regionId.value().x, regionId.value().y, regionId.value().z);

		replicator->sendClusterChunk(regionId);
		item.setStreamedTerrain();
		return true;
	}
	return false;
}

TaskScheduler::StepResult Replicator::StreamJob::stepDataModelJob(const Stats& stats) 
{
    if (!readyToWork)
    {
        return TaskScheduler::Stepped;
    }

	if(replicator)
	{
		float streamJobConsecutiveRuns = 1.0f; 
		if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive && DFFlag::CyclicExecutiveForServerTweaks)
		{
			streamJobConsecutiveRuns = updateStepsRequiredForCyclicExecutive(stats.timespanSinceLastStep.seconds(), 
																			(float)dataSendRate, 
																			(float)DFInt::MaxConsecutiveStreamJobWorkLoad, 
																			(float)DFInt::MaxConsecutiveStreamJobWorkLoad);
		}
		// check current player location, reset stream center if needed
		CoordinateFrame focus;
		if (Player* player = replicator->findTargetPlayer()) 
		{
			if (ModelInstance* character = player->getCharacter())
			{
				if (Humanoid* h = character->findFirstModifierOfType<Humanoid>())
				{
					if (!h->getDead())
					{
						if (const PartInstance* head = Humanoid::getConstHeadFromCharacter(character))
							setStreamCenterAndSentMinRegions(head->getCoordinateFrame().translation, false);
					}
				}
			}
		}

		try
		{
            if (replicator->replicatorStats.dataPacketsReceived.value() < Time::Interval(1.f)) // if the client becomes idle / busy for more than a second, we stop collecting new instances
            {
                int numberToCollect = numClientInstanceQuota / (1.f / stats.timespanSinceLastStep.seconds()); // distribute the collection of one second content to each steps
                RBXASSERT(numberToCollect>=0);
			    if (numCollectedInstances < numberToCollect) 
			    {
				    RBXASSERT(Workspace::findWorkspace(replicator.get()));
                    regionIterator.updateWorldExtents();

					Time t = Time::now<Time::Fast>() + Time::Interval((0.25 * streamJobConsecutiveRuns) / dataSendRate);	
				    while ((numCollectedInstances < numberToCollect) && Time::now<Time::Fast>() <= t) {
					    if (!collectPartsFromNextRegion(false))
						    break;
				    }
#ifdef NETWORK_DEBUG
                    //StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "collect %d instances (%d), interrupted because %s", numCollectedInstances, numberToCollect, Time::now<Time::Fast>() > t ? "timeout":"all collected");
#endif
			    }
            }

            if (networkHealthCheckTimer.delta().seconds() > 1.0f)
            {
                float bufferHealth = replicator->rakPeer->GetBufferHealth();
                if (bufferHealth < 0.5)
                {
                    // buffer is growing, reduce packets per step
                    numPacketsPerStep = numPacketsPerStep >> 1;
                }
                else if (bufferHealth > 0.9)
                {
                    // healthy, push more packets
                    numPacketsPerStep++;
                    if (numPacketsPerStep > DFInt::MaxStreamPacketsPerStep)
                    {
                        numPacketsPerStep = DFInt::MaxStreamPacketsPerStep;
                    }
                }
                networkHealthCheckTimer.reset();
            }

            sendPackets(numPacketsPerStep * streamJobConsecutiveRuns);
		}
		catch (RBX::base_exception& e)
		{
			replicator->requestDisconnect(DisconnectReason_SendPacketError);
			const char* what = e.what();
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Network Data stream: %s", what ? what : "empty error string");
			return TaskScheduler::Done;
		}
		return TaskScheduler::Stepped;
	}
	return TaskScheduler::Done;
}

namespace RBX {
namespace Network {
    
template<>
Replicator::StreamJob::StreamDataItem* Replicator::StreamJob::iteratorToItem(SortedPendingItemsList::iterator& iter)
{
	return iter->second;
}

template<>
Replicator::StreamJob::StreamDataItem* Replicator::StreamJob::iteratorToItem(PendingItemsList::iterator& iter)
{
	return *iter;
}

} // namespace Network
} // namespace RBX


template <class Container>
int Replicator::StreamJob::sendItems(Container& itemList, int maxInstances, Time syncTime)
{
	int instancesToWrite = maxInstances;
	int totalInstancesSent = 0;

	ItemSender sender(*replicator.get(), replicator->rakPeer.get());
	bool bitStreamFull = false;
	while (!bitStreamFull && itemList.size()) 
	{
		typename Container::iterator iter = itemList.begin();
		StreamDataItem *item = iteratorToItem(iter);

		// do not send any items if we are ahead of replicator item queue
		if (!syncTime.isZero() && item->timestamp > syncTime)
			return 0;

		// Stream our terrain.
		if (streamTerrain(*item)) {
			--instancesToWrite;
			--numCollectedInstances;
		}

		// Stream our parts.
		const size_t initialItemSize = item->size();
		if (instancesToWrite > 0)
		{
			item->setMaxInstancesToWrite(instancesToWrite);
		}

		bitStreamFull = (ItemSender::SEND_BITSTREAM_FULL == sender.send(*item));

		// sender.send() may not write as many instances to the wire as we pop out of "item"'s
		// internal queue.  Therefore, numCollectedInstances may be necessarily reduced by
		// more than numPendingInstanceRequests in this step.
		numCollectedInstances -= initialItemSize - item->size();
		instancesToWrite -= item->getInstancesSentLastWrite();
		totalInstancesSent += item->getInstancesSentLastWrite();

		FASTLOG4(DFLog::PartStreamingRequests, "Sent %d items, %d pending requests remain, %d collected remain, bitstream full: %d",
			item->getInstancesSentLastWrite(),
			instancesToWrite,
			numCollectedInstances,
			bitStreamFull);

		if (!bitStreamFull)
		{
			// done sending this item, ok to delete
			itemList.erase(iter);
			delete item;
		}
	}

	if (totalInstancesSent)
		avgPacketSize.sample(sender.getNumberOfBytesUsed());

	return totalInstancesSent;
	
}

void Replicator::StreamJob::sendPackets(int maxPackets)
{
	Time syncTime = replicator->getSendQueueHeadTime();

	// always send all high priority items
	while (pendingHighPriorityStreamItems.size() > 0)
	{
		int numInstancesSent = sendItems(pendingHighPriorityStreamItems, -1, syncTime);
		if (numInstancesSent > 0)
		{
			packetsSent.sample();
			avgInstancePerStep.sample(numInstancesSent);
		}
		else
		{
			break;
		}
	}

    int numPacketSent = 0;
    int numPendingInstanceRequests = numClientInstanceQuota;
    while (pendingStreamItems.size() > 0)
    {
        if (maxPackets >= 0 && numPacketSent >= maxPackets)
            break; // we are done

		int numInstancesSent = sendItems(pendingStreamItems, numPendingInstanceRequests, syncTime);
        if (numInstancesSent > 0)
        {
            packetsSent.sample();
            avgInstancePerStep.sample(numInstancesSent);
        }
		else
		{
			break;
		}

        numPacketSent++;
    }
}

void Replicator::StreamJob::clearPendingItems()
{
	PendingItemsList::iterator iter = pendingStreamItems.begin();
	while (iter != pendingStreamItems.end())
	{
		StreamDataItem* item = *iter;
		collectedRegions.erase(item->getRegionId());
		numCollectedInstances -= item->size();
		delete item;
		iter = pendingStreamItems.erase(iter);
	}

	RBXASSERT(pendingStreamItems.size() == 0);
	numCollectedInstances = 0;
}

void Replicator::StreamJob::addInstanceToStreamQueue(shared_ptr<Instance> instance, StreamDataItem *item)
{
	replicator->addReplicationData(instance, true, true);
	item->addInstance(instance);
}

size_t Replicator::StreamJob::addInstanceAndDescendantsToStreamQueue(shared_ptr<Instance> instance, StreamDataItem* item) {
    size_t beforeSize;
    beforeSize = item->size();
    addInstanceToStreamQueue(instance, item);
    instance->visitDescendants(boost::bind(&Replicator::StreamJob::addInstanceToStreamQueue, this, _1, item));
    return item->size() - beforeSize;
}

bool Replicator::StreamJob::getNextUncollectedRegion(StreamRegion::Id &regionId)
{
	bool foundRegion = false;
	while (!foundRegion && regionIterator.getNextRegion(regionId))
	{
		foundRegion = collectedRegions.end() == collectedRegions.find(regionId);
	}
	return foundRegion;
}

bool Replicator::StreamJob::collectPartsForMinPlayerArea()
{
	bool collected = false;
	while (regionIterator.getRadius() < 2)
	{
		if (collectPartsFromNextRegion(true))
			collected = true;
		else
			break; // nothing left to collect
	}

	return collected;
}

bool Replicator::StreamJob::collectPartsFromNextRegion(bool highPriority)
{
	DenseHashSet<Primitive*> found(NULL);
	unsigned int collectedInstances = 0;

	StreamRegion::Id regionId;

	// TODO: check pending, if region is collected but in pending list, move it to high priority list if "highPrioirty" is set to true
	if (!getNextUncollectedRegion(regionId))
		return false; // all regions collected

	Extents extents = StreamRegion::extentsFromRegionId(regionId);

	spatialHash->getPrimitivesOverlapping(extents, found);

    // TODO: batch region ID into 1 item?
	StreamDataItem *item = new StreamDataItem(replicator.get(), regionId, lastSentRegionId);
    // If StreamJob is not activated yet, we are queueing up initial items, so we use a larger size
	item->setBytesPerStep(static_cast<int>(readyToWork ? replicator->getAdjustedMtuSize() : DFLog::MaxJoinDataSizeKB * 1000));
    if (highPriority)
    {
		pendingHighPriorityStreamItems.insert(std::make_pair(static_cast<float>(StreamRegion::Id::getRegionLongestAxisDistance(regionIterator.getCenterRegion(), regionId)), item));
    }
    else
    {
        pendingStreamItems.push_back(item);
    }

    if (regionId.isRegionInTerrainBoundaries()) {
        ++collectedInstances;
    }

    if (!found.empty())
	{
		for (DenseHashSet<Primitive*>::const_iterator iter = found.begin(); iter != found.end(); ++iter)
		{
			PartInstance* part = PartInstance::fromPrimitive(*iter);

			if (replicator->isInstanceAChildOfClientsCharacterModel(part)) {
				FASTLOGS(FLog::NetworkStreaming, "Skipping part %s because it is a child of character", part->getName().c_str());
				continue;
			}

			collectedInstances += addInstanceAndDescendantsToStreamQueue(shared_from(part), item);
		}
	}

	// mark region as collected
    collectedRegions.insert(regionId);

	if (collectedInstances) {
		FASTLOG1(DFLog::PartStreamingRequests, "Collected %u instances", collectedInstances);
	}

    numCollectedInstances += collectedInstances;
	return true;
}

void Replicator::StreamJob::onPlayerCharacterAdd(boost::shared_ptr<Instance> character)
{
    if (playerTorsoPropChangedConnection.connected())
        playerTorsoPropChangedConnection.disconnect();

    // find the torso of the player, used to monitor CFrame change
    PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
    playerTorsoPropChangedConnection = torso->propertyChangedSignal.connect(boost::bind(&StreamJob::onPlayerTorsoChanged, this, _1));

    // This is only need if we are not auto spawning the character. If auto spawning, we will be calling spawn function,
    // which will change CFrame of player and enter onPlayerTorsoChanged function
	if (!replicator->players->getShouldAutoSpawnCharacter())
		setStreamCenterAndSentMinRegions(torso->getCoordinateFrame().translation, false);
}

void Replicator::StreamJob::onPlayerTorsoChanged(const Reflection::PropertyDescriptor* desc)
{
    // listen to player torso CFrame prop change, this usually happens for "teleports"
    if (*desc == PartInstance::prop_CFrame)
    {
        Player* player = replicator->findTargetPlayer();
        RBXASSERT(player);
        shared_ptr<ModelInstance> character = player->getSharedCharacter();
        RBXASSERT(character);

        if (PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso")))
		{
			FASTLOG3(FLog::NetworkStreaming, "torso changed to (%d,%d,%d)",
				(int)torso->getCoordinateFrame().translation.x, (int)torso->getCoordinateFrame().translation.y, (int)torso->getCoordinateFrame().translation.z);

			setStreamCenterAndSentMinRegions(torso->getCoordinateFrame().translation, false);
		}
    }
}

bool Replicator::StreamJob::setStreamCenterAndSentMinRegions(const Vector3& worldPos, const bool forceSet)
{
	if (!readyToWork)
		return false;

	if (regionIterator.resetCenter(worldPos, forceSet))
	{
		RBXASSERT (numClientInstanceQuota >= 0);
		
		clearPendingItems();

		// parts collected here should be marked as high priority and will be sent right away
		if (collectPartsForMinPlayerArea())
		{
			if (readyToWork)
				sendPackets(1);
		}

		return true;
	}

	return false;
}

void Replicator::StreamJob::addInstanceToRegularQueue(shared_ptr<Instance> instance)
{
	replicator->addReplicationData(instance, true, true);
	replicator->addToPendingItemsList(instance);
}

void Replicator::StreamJob::addInstanceAndDescendantsToRegularQueue(shared_ptr<Instance> instance) {
	addInstanceToRegularQueue(instance);
	instance->visitDescendants(boost::bind(&Replicator::StreamJob::addInstanceToRegularQueue, this, _1));
}

void Replicator::StreamJob::coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info) 
{
	StreamRegion::IdExtents oldRegionExtents, newRegionExtents;
	if (!StreamRegion::coarseMovementCausesStreamRegionChange(
			info, &oldRegionExtents, &newRegionExtents)) {
		return;
	}

	shared_ptr<Instance> instance(shared_from(PartInstance::fromPrimitive(p)));
	// check to see if this indicates a piece of the character has moved
	if (replicator->isInstanceAChildOfClientsCharacterModel(instance.get())) {
		FASTLOGS(FLog::NetworkStreaming,
			"Ignoring region transition for part %s because it is a child of character",
			instance->getName().c_str());

		// TODO: Instead of listening to torso change events, consider detecting
		// teleports here.
		return;
	}
	
	StreamRegion::Id foundRegionId;
	if (newRegionExtents.intersectsContainer(collectedRegions, &foundRegionId))
    {
		if ((info.updateType == UpdateInfo::UPDATE_TYPE_Insert) && !readyToWork)
		{
			FASTLOGS(FLog::NetworkStreaming,
				"Sending %s in special packet because it entered a sent region",
				instance->getName().c_str());

			StreamDataItem *item = new StreamDataItem(replicator.get(), foundRegionId, lastSentRegionId);

			item->setStreamedTerrain();

			// If StreamJob is not activated yet, we are queueing up initial items, so we use a larger size
			item->setBytesPerStep(static_cast<int>(readyToWork ? replicator->getAdjustedMtuSize() : DFLog::MaxJoinDataSizeKB * 1000));

			numCollectedInstances += addInstanceAndDescendantsToStreamQueue(instance, item);
			pendingHighPriorityStreamItems.insert(std::make_pair(static_cast<float>(StreamRegion::Id::getRegionLongestAxisDistance(regionIterator.getCenterRegion(), foundRegionId)), item));
		}
		else if ((info.updateType == UpdateInfo::UPDATE_TYPE_Change) && !oldRegionExtents.intersectsContainer(collectedRegions))
		{
			// part moved from unsent region into sent region, add it to replicator send queue
			addInstanceAndDescendantsToRegularQueue(instance);
		}
    }
}
