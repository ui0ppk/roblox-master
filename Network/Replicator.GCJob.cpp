#include "Replicator.GCJob.h"

#include "ClientReplicator.h"
#include "Compressor.h"
#include "Player.h"
#include "Util.h"

#include "AppDraw/DrawAdorn.h"
#include "rbx/DenseHash.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/datamodel.h"
#include "V8DataModel/PartInstance.h"
#include "v8world/ContactManager.h"
#include "v8world/ContactManagerSpatialHash.h"
#include "util/ScopedAssign.h"
#include "util/StreamRegion.h"
#include "GfxBase/Adorn.h"
#include "v8datamodel/JointsService.h"
#include "NetworkProfiler.h"
#include "Humanoid/Humanoid.h"

//#include "v8world/Contact.h"
//#include "v8kernel/Body.h"
//#include "v8kernel/SimBody.h"

DYNAMIC_LOGGROUP(PartStreamingRequests)
DYNAMIC_FASTINTVARIABLE(PartStreamingGCMinRegionLength, 2);
// Default value: each id takes about 5 bytes (1 byte for scope, 4 bytes for id)
// and expected mtu size ~1500, so a little less than 300 ids fit in one mtu.
FASTINTVARIABLE(StreamOutCompressionIdListLengthThreshold, 250);
DYNAMIC_FASTINTVARIABLE(JoinDataCompressionLevel, 1)
DYNAMIC_FASTINTVARIABLE(JoinDataBonus, 0)

using namespace RBX;
using namespace RBX::Network;

namespace {
    bool compRegionDistance(const RegionsMap::iterator a, const RegionsMap::iterator b)
    {
        // far to near, in axis
        return a->second.streamDistance > b->second.streamDistance;
    }
}

class ClientReplicator::GCJob::RegionRemovalItem : public Item
{
	StreamRegion::Id region;
	std::vector<Guid::Data> collectedInstanceGuids;

public:
	RegionRemovalItem(Replicator* replicator, StreamRegion::Id id) : Item(*replicator), region(id) {}
	bool addInstance(shared_ptr<Instance> partInstance)
	{
        Guid::Data data;
        partInstance->getGuid().extract(data);
        if(std::find(collectedInstanceGuids.begin(), collectedInstanceGuids.end(), data)==collectedInstanceGuids.end())
        {
		    collectedInstanceGuids.push_back(data);
            return true;
        }
        return false;
	}
	bool write(RakNet::BitStream& bitStream) 
	{
		writeItemType(bitStream, ItemTypeRegionRemoval); 
		bitStream << region;
		boost::uint32_t numberOfCollectedGuids = collectedInstanceGuids.size();
		bitStream << numberOfCollectedGuids;

		bool useCompression = FInt::StreamOutCompressionIdListLengthThreshold >= 0 &&
			numberOfCollectedGuids > ((unsigned int)FInt::StreamOutCompressionIdListLengthThreshold);

		bitStream << useCompression;

		if (useCompression)
		{
			RakNet::BitStream uncompressedBuffer;

			for (boost::uint32_t i = 0; i < collectedInstanceGuids.size(); ++i)
			{
				replicator.serializeId(uncompressedBuffer, collectedInstanceGuids[i]);
			}

            Replicator::compressBitStream(uncompressedBuffer, bitStream, DFInt::JoinDataCompressionLevel);
		}
		else
		{
			for (boost::uint32_t i = 0; i < collectedInstanceGuids.size(); ++i)
			{
				replicator.serializeId(bitStream, collectedInstanceGuids[i]);
			}
		}

		return true;
	}
};

class ClientReplicator::GCJob::InstanceRemovalItem : public Item
{
public:
	InstanceRemovalItem(Replicator* replicator, Guid::Data id) : Item(*replicator), id(id) {}
	bool write(RakNet::BitStream& bitStream)
	{
		writeItemType(bitStream, ItemTypeInstanceRemoval);
		replicator.serializeId(bitStream, id);
		return true;
	}
private:
	Guid::Data id;
};

ClientReplicator::GCJob::GCJob(Replicator& replicator) 
: ReplicatorJob("Replicator GC Job", replicator, DataModelJob::Write)
, numSorted(0)
, numRegionToGC(0)
, renderingDistance(NULL)
, renderingRegionDistance(0x7FFF)
, gcRegionDistance(0x7FFF)
, maxRegionDistance(0x7FFF)
, lastPlayerPosition(Vector3::zero())
, centerRegion(Vector3int32::maxInt())
{
	cyclicExecutive = true;

	if (Workspace* workspace = Workspace::findWorkspace(&replicator)) {
		if (World* world = workspace->getWorld()) {
			spatialHash = world->getContactManager()->getSpatialHash();
		}
        renderingDistance = &workspace->renderingDistance;
	}

	RBXASSERT(spatialHash);
	if (spatialHash) {
		spatialHash->registerCoarseMovementCallback(this);
	}
}

ClientReplicator::GCJob::~GCJob() {
    unregisterCoarseMovementCallback();
}

void ClientReplicator::GCJob::unregisterCoarseMovementCallback()
{
    if (Workspace* workspace = Workspace::findWorkspace(replicator.get())) {
        if (World* world = workspace->getWorld()) {
            spatialHash = world->getContactManager()->getSpatialHash();
            if (spatialHash) {
                StandardOut::singleton()->printf(MESSAGE_INFO, "unregisterCoarseMovementCallback() from ~GCJob()");
                spatialHash->unregisterCoarseMovementCallback(this);
            }
        }
    }
}

TaskScheduler::Job::Error ClientReplicator::GCJob::error( const Stats& stats)
{
	// TODO: compute error based on need to gc
	return computeStandardError(stats, replicator->settings().getDataGCRate());
}

void ClientReplicator::GCJob::evaluateNumRegionToGC()
{
    if (numRegionToGC <= 0)
    {
        numRegionToGC = streamedRegionList.size() / 2;
        int minGcSize = StreamRegion::Constants::kMinNumPlayableRegion;
        if (numRegionToGC < minGcSize)
        {
            numRegionToGC = streamedRegionList.size()-minGcSize;
            if (numRegionToGC < 0)
            {
                numRegionToGC = 0;
            }
        }
    }
}

TaskScheduler::StepResult ClientReplicator::GCJob::stepDataModelJob(const Stats& stats) 
{
    bool tryGC = false;
    bool forceGC = false;

	if(!replicator)
		return TaskScheduler::Done;

	// get the player CFrame if they are alive
	CoordinateFrame cf;
	bool playerIsDead = true;
	if (const Player* player = replicator->findTargetPlayer()) 
		if (const ModelInstance* characterModel = player->getConstCharacter())
			if (const Humanoid* h = characterModel->findConstFirstModifierOfType<Humanoid>())
				if (!h->getDead())
				{
					playerIsDead = false;
					if (const PartInstance* head = Humanoid::getConstHeadFromCharacter(characterModel))
						cf = head->getCoordinateFrame();
				}

    ClientReplicator* clientRep = replicator->fastDynamicCast<ClientReplicator>();
    RBXASSERT(clientRep);
    clientRep->updateMemoryStats();
    if (!playerIsDead && (clientRep->getMemoryLevel() <= RBX::MemoryStats::MEMORYLEVEL_ALL_CRITICAL_LOW))
    {
        // Memory level too low, force GC
        tryGC = true;
        forceGC = true;
    }

    // sample the rendering region distance
    RBXASSERT(renderingDistance);
    if (renderingDistance)
    {
        RBXASSERT((*renderingDistance) / StreamRegion::Id::streamGridCellSizeInStuds() < 32767.f);
        float oldRenderingRegionDistance = renderingRegionDistance;
        short newRenderingRegionDistance = (short)((*renderingDistance) / StreamRegion::Id::streamGridCellSizeInStuds()) + 1;
        // smoothing the variance by weight averaging the final value
        renderingRegionDistance = (renderingRegionDistance*2.0f + newRenderingRegionDistance)/3.0f;
        if (renderingRegionDistance != oldRenderingRegionDistance)
        {
            FASTLOG1F(DFLog::PartStreamingRequests, "Rendering region distance: %f", renderingRegionDistance);
        }
    }

    // check current player location, reset stream center if needed
	if (!playerIsDead)
	{
		StreamRegion::Id playerRegionId = StreamRegion::regionContainingWorldPosition(cf.translation);
		if (centerRegion != playerRegionId)
		{
			int regionSizeSquared = StreamRegion::Id::streamGridCellSizeInStuds() * StreamRegion::Id::streamGridCellSizeInStuds();
			if (forceGC || (cf.translation - lastPlayerPosition).squaredMagnitude() >= regionSizeSquared)
			{
				FASTLOG1F(DFLog::PartStreamingRequests, "Last player pos translation squared: %.0f", (cf.translation - lastPlayerPosition).squaredMagnitude());

				// recompute distance
				for (RegionList::iterator iter = streamedRegionList.begin(); iter != streamedRegionList.end(); iter++)
				{
					(*iter)->second.computeRegionDistance(playerRegionId);
				}

				// TODO: nth_element sort?
				streamedRegionList.sort(compRegionDistance);
				numSorted = streamedRegionList.size();
				lastPlayerPosition = cf.translation;
				tryGC = true;

				centerRegion = playerRegionId;
			}
		}
	}

    if (pendingGC())
    {
        tryGC = true;
    }

	DataModel::scoped_write_request request(replicator.get());

	if (tryGC)
	{
        if (clientRep->needGC())
        {
            evaluateNumRegionToGC();
            if (numRegionToGC > 0)
            {
                pendingRemovalPartInstances.clear();
                // if GC is needed, check to see if new regions arrived since last sort, GC should always use a sorted list
                if ((numSorted < (int)streamedRegionList.size()))
                {
                    // recompute distance for newly arrived regions using cached player position
                    StreamRegion::Id lastPlayerRegionId = StreamRegion::regionContainingWorldPosition(lastPlayerPosition);
                    int numNewRegions = streamedRegions.size() - numSorted;
                    RegionList::iterator iter = streamedRegionList.end();
                    std::advance(iter, -numNewRegions);
                    for (; iter != streamedRegionList.end(); iter++)
                    {
                        (*iter)->second.computeRegionDistance(lastPlayerRegionId);
                    }

                    streamedRegionList.sort(compRegionDistance);
                    numSorted = streamedRegionList.size();
                }
                FASTLOG1(DFLog::PartStreamingRequests, "Not enough memory: %u.  Garbage collecting some items.", MemoryStats::freeMemoryBytes());

                // local delete
		        Time t = Time::now<Time::Fast>() + Time::Interval(0.2 / replicator->settings().getDataGCRate());
		        while (numSorted > 0 && numRegionToGC > 0)
		        {
			        StreamRegion::Id regionId = streamedRegionList.front()->second.id;
			        StreamRegion::Id characterRegionId = StreamRegion::regionContainingWorldPosition(cf.translation);
                    int regionDistance = StreamRegion::Id::getRegionLongestAxisDistance(regionId, characterRegionId);
        			
                    if (!forceGC && (!clientRep->needGC() || regionDistance <= DFInt::PartStreamingGCMinRegionLength))
			        {
                        numRegionToGC = 0;
				        tryGC = false;
				        FASTLOG1(DFLog::PartStreamingRequests, "GC Job completed.  Memory free: %u", MemoryStats::freeMemoryBytes());
				        break;
			        }

                    if (regionDistance < gcRegionDistance)
                    {
                        gcRegionDistance = regionDistance - 1;
                        RBXASSERT(gcRegionDistance > 0);
                    }

			        // remove region from containers
			        streamedRegionList.pop_front();
			        streamedRegions.erase(regionId);
			        numSorted--;
                    numRegionToGC--;

			        // TODO: batch regions into 1 item
			        RegionRemovalItem* regionRemoveItem = new RegionRemovalItem(replicator.get(), regionId);

			        gcRegion(regionId, regionRemoveItem);

			        clientRep->pendingItems.push_back(regionRemoveItem);

                    // break if over time budget
                    if (Time::now<Time::Fast>() > t)
                    {
                        if (forceGC)
                        {
                            // if we are in forced GC mode, we only break if memory is above critical level
                            clientRep->updateMemoryStats();
                            if (clientRep->getMemoryLevel() >= RBX::MemoryStats::MEMORYLEVEL_LIMITED)
                            {
                                break;
                            }
                        }
                        else
                        {
				            break;
                        }
                    }
		        }

                if (pendingRemovalPartInstances.size() > 0)
                {
                    CPUPROFILER_START(NetworkProfiler::PROFILER_jointRemoval);
                    // gc invalid joints
                    ClientReplicator* cRep = static_cast<ClientReplicator*>(replicator.get());
                    if (JointsService* jointsService = ServiceProvider::find<JointsService>(cRep)) {
                        jointsService->visitDescendants(
                            boost::bind(&ClientReplicator::streamOutAutoJointHelper, cRep, pendingRemovalPartInstances, _1));
                    }
                    CPUPROFILER_STEP(NetworkProfiler::PROFILER_jointRemoval);
                    for (std::vector<shared_ptr<PartInstance> >::iterator iter = pendingRemovalPartInstances.begin(); iter != pendingRemovalPartInstances.end(); iter++)
                    {
                        shared_ptr<PartInstance> partInstance = *iter;
                        {
                            if (partInstance)
                            {
                                RBX::ScopedAssign<Instance*> assign(clientRep->removingInstance, partInstance.get());
                                partInstance->setParent(NULL);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // if gc is not needed, gradually increase the gc radius
            updateGcRegionDistance();
        }
	}

	setMaxSimulationRadius((float)(gcRegionDistance - 2.5f) * StreamRegion::Id::streamGridCellSizeInStuds());

	return TaskScheduler::Stepped;
}

void ClientReplicator::GCJob::insertRegion(const StreamRegion::Id& id)
{
	std::pair<RegionsMap::iterator, bool> result = streamedRegions.insert(std::make_pair(id, RegionInfo(id)));
	if (result.second)
	{
		// new region
		streamedRegionList.push_back(result.first);
	}
}

void ClientReplicator::GCJob::setMaxSimulationRadius(float radius)
{
	if (Player* player = replicator->findTargetPlayer())
	{
		if (player->getMaxSimulationRadius() > radius)
			player->setMaxSimulationRadius(radius);
	}
}

void ClientReplicator::GCJob::coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info) {

	StreamRegion::IdExtents oldRegionExtents, newRegionExtents;
	if (!StreamRegion::coarseMovementCausesStreamRegionChange(
			info, &oldRegionExtents, &newRegionExtents)) {
		return;
	}

	shared_ptr<PartInstance> instance = shared_from(PartInstance::fromPrimitive(p));
	if (!newRegionExtents.intersectsContainer(streamedRegions))
	{
		// Mark the region as streamed so it can be GC'ed when needed
		insertRegion(StreamRegion::regionContainingWorldPosition(instance->getCoordinateFrame().translation));
	}
}

void ClientReplicator::GCJob::gcRegion(const StreamRegion::Id& regionId, RegionRemovalItem* removeItem)
{
	DenseHashSet<Primitive*> found(NULL);
	spatialHash->getPrimitivesOverlapping(StreamRegion::extentsFromRegionId(regionId), found);

	for (DenseHashSet<Primitive*>::const_iterator iter = found.begin(); iter != found.end(); ++iter)
	{
		// This assumes that the streamed regions set has already been updated
		StreamRegion::IdExtents regionExtents = StreamRegion::regionExtentsFromContactManagerLevelAndExtents(
			(*iter)->getSpatialNodeLevel(), (*iter)->getOldSpatialExtents());

		PartInstance* part = PartInstance::fromPrimitive(*iter);
		if (!regionExtents.intersectsContainer(streamedRegions) &&
			!replicator->isInstanceAChildOfClientsCharacterModel(part))
		{
			gcPartInstance(part, removeItem);
		}
	}

	OneQuarterClusterChunkCellIterator cellIterator;
	cellIterator.setToStartOfStreamRegion(regionId);
	while (cellIterator.size()) {
		Vector3int16 cellPos;
		cellIterator.pop(&cellPos);
		replicator->fastDynamicCast<ClientReplicator>()->streamOutTerrain(cellPos);
	}
}

void ClientReplicator::GCJob::gcPartInstance(PartInstance* part, RegionRemovalItem* removeItem) {
	RBXASSERT_SLOW(!replicator->isInstanceAChildOfClientsCharacterModel(part));

    pendingRemovalPartInstances.push_back(shared_from(part));

    if (removeItem->addInstance(shared_from(part)))
    {
        part->visitDescendants(boost::bind(&RegionRemovalItem::addInstance, removeItem, _1));
        replicator->fastDynamicCast<ClientReplicator>()->streamOutInstance(part, false);
    }
}

void ClientReplicator::GCJob::render3dAdorn(Adorn* adorn)
{
	for (RegionList::reverse_iterator iter = streamedRegionList.rbegin(); iter != streamedRegionList.rend(); iter++)
	{
		Extents ext = StreamRegion::extentsFromRegionId((*iter)->second.id);
		DrawAdorn::outlineBox(adorn, AABox(ext.min(), ext.max()), Color4(Color3::red(), 0.5f));
	}
}

bool ClientReplicator::GCJob::updateMaxRegionDistance()
{
    if (renderingDistanceUpdateTimer.delta().seconds() > kRenderingDistanceUpdateInterval)
    {
        renderingDistanceUpdateTimer.reset();
		short newMaxRegionDistance = (renderingRegionDistance < gcRegionDistance ? renderingRegionDistance : gcRegionDistance);
        if (maxRegionDistance != newMaxRegionDistance)
        {
            maxRegionDistance = newMaxRegionDistance;
            return true;
        }
        else
            return false;
    }
    else
    {
        return false;
    }
}

void ClientReplicator::GCJob::updateGcRegionDistance()
{
    if (maxRegionExpansionTimer.delta().seconds() > kMaxRegionExpansionTimerLimit)
    {
        maxRegionExpansionTimer.reset();
        if (gcRegionDistance < 0x7FFF)
            gcRegionDistance++;
    }
}

void ClientReplicator::GCJob::notifyServerGcingInstanceAndDescendants(shared_ptr<Instance> instance)
{
	Guid::Data data;
	instance->getGuid().extract(data);
	InstanceRemovalItem* instanceRemovalItem = new InstanceRemovalItem(replicator.get(), data);
	ClientReplicator* clientRep = replicator->fastDynamicCast<ClientReplicator>();
	clientRep->pendingItems.push_back(instanceRemovalItem);

	// important: use visit children (_not_ visitDescendants) because this function
	// is recurring down one step at a time. If visitDescendants is called we can queue
	// multiple removal items for grandchildren and great-grandchildren.
	instance->visitChildren(boost::bind(&ClientReplicator::GCJob::notifyServerGcingInstanceAndDescendants,
		this, _1));
}
