#include "stdafx.h"
#include "SceneUpdater.h"

#include "MegaCluster.h"
#include "SmoothCluster.h"
#include "FastCluster.h"
#include "SuperCluster.h"
#include "SpatialHashedScene.h"
#include "LightGrid.h"
#include "LightObject.h"
#include "ParticleEmitter.h"
#include "ExplosionEmitter.h"

#include "SpatialGrid.h"

#include "GfxBase/FrameRateManager.h"

#include "humanoid/Humanoid.h"
#include "v8DataModel/Accoutrement.h"
#include "v8world/ContactManager.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Light.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/ForceField.h"
#include "v8datamodel/Explosion.h"
#include "v8datamodel/PartCookie.h"
#include "v8datamodel/CustomParticleEmitter.h"

#include "Voxel/Util.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "GlobalShaderData.h"

#include "v8datamodel/CustomParticleEmitter.h"
#include "CustomEmitter.h"

#include "voxel2/Grid.h"

#include "rbx/Profiler.h"

LOGGROUP(GfxClusters)
LOGGROUP(GfxClustersFull)
LOGGROUP(RenderLightGrid)
LOGGROUP(RenderLightGridAgeProportion)

FASTFLAG(RenderNewExplosionEnable)
FASTFLAG(NoRandomColorsWithoutOutlines)

LOGGROUP(ViewRbxInit)
LOGGROUP(TerrainCellListener)

FASTFLAGVARIABLE(FixCameraTargetStudio, false)
FASTFLAGVARIABLE(CustomEmitterRenderEnabled, false)
FASTFLAG(SmoothTerrainRenderLOD)

DYNAMIC_FASTFLAG(HumanoidCookieRecursive)
DYNAMIC_FASTFLAG(UseR15Character)

FASTINTVARIABLE(FastClusterUpdateWaitingBudgetMs, 4)

namespace RBX
{
namespace Graphics
{

const double CLUSTER_INVALIDATE_FRAME_BUDGET_MS = 4.0;

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
const int FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET = 2;
const size_t MAX_INVALIDATIONS_PER_FRAME = 16;
#else
const int FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET = 4;
const size_t MAX_INVALIDATIONS_PER_FRAME = 64;
#endif

SceneUpdater::SceneUpdater(shared_ptr<RBX::DataModel> dataModel, VisualEngine* ve)
    : dataModel(dataModel)
    , mSettings(ve->getSettings())
    , mRenderCaps(ve->getRenderCaps())
    , mRenderStats(ve->getRenderStats())
    , currentFrameNum(0)
	, mVisualEngine(ve)
	, mLastLightingUpdates(0)
	, mLastOccupancyUpdates(0)
	, mLightingComputeAverage(15)
	, mAgeDirtyProportion(0)
	, mLightingActive(false)
	, computeLightingEnabled(true)
{
	mSeenFastClusters.reserve(FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET);

	FASTLOG(FLog::ViewRbxInit, "SceneUpdater bind - start");
	// Find stuff to render:
	dataModel->getWorkspace()->visitDescendants(boost::bind(&SceneUpdater::onWorkspaceDescendantAdded, this, _1));
	workspaceDescendantAddedConnection = dataModel->getWorkspace()->onDemandWrite()->descendantAddedSignal.connect(boost::bind(&SceneUpdater::onWorkspaceDescendantAdded, this, _1));

    FASTLOG1(FLog::GfxClusters, "After initial bind, added parts: %u", mAddedParts.size());
	
	RBX::Vector3 cellExtents = RBX::Vector3(32 * 4, 16 * 4, 32 * 4);
	float largeCoeff = 1.5f;
	
	mFastGridSC.reset(new FastGridSC(cellExtents, largeCoeff));
	
	FASTLOG(FLog::ViewRbxInit, "SceneUpdater bind - end");

    if (FFlag::NoRandomColorsWithoutOutlines)
    {
        RBX::Lighting* lighting = ServiceProvider::find<Lighting>(dataModel.get());
        if (lighting)
            propertyChangedSignal = lighting->propertyChangedSignal.connect(boost::bind(&SceneUpdater::onPropertyChanged, this, _1));
    }
}

SceneUpdater::~SceneUpdater()
{
}

void SceneUpdater::unbind()
{
	FASTLOG(FLog::ViewRbxInit, "SceneUpdater unbind - start");

	for(size_t i = 0; i < connections.size(); ++i)
	{
		connections[i].disconnect();
	}
	connections.clear();
	FASTLOG(FLog::ViewRbxInit, "SceneUpdater unbind - end");

    for (GfxPartSet::iterator it = mAttachments.begin(); it != mAttachments.end(); ++it)
        delete *it;

	for (HumanoidClusterMap::const_iterator it = mHumanoidClusters.begin(); it != mHumanoidClusters.end(); ++it)
		delete it->second;

    GfxPartSet megaClusters = mMegaClusters;

	for (GfxPartSet::const_iterator it = megaClusters.begin(); it != megaClusters.end(); ++it)
		delete *it;

    RBXASSERT(mMegaClusters.empty());

    mAttachments.clear();
	mHumanoidClusters.clear();
	mMegaClusters.clear();
    mFastGridSC.reset();
}

void SceneUpdater::onWorkspaceDescendantAdded(shared_ptr<RBX::Instance> descendant)
{
	// See if the new instance is a PartInstance
	
	RBX::PartInstance* pi = RBX::Instance::fastDynamicCast<RBX::PartInstance>(descendant.get());
	if (pi!=NULL)
	{
		queuePartToCreate(shared_from(pi));
		return;
	}

	if ( dynamic_cast<Effect*>(descendant.get()))
	{
		queueAttachementToCreate(descendant);
	}
}

bool SceneUpdater::isPartStatic(RBX::PartInstance* part)
{
	return part->getSleeping();
}

void SceneUpdater::queuePartToCreate(const boost::shared_ptr<RBX::PartInstance>& part)
{
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	
	if (part->getPartType() == MEGACLUSTER_PART)
	{
		FASTLOG2(FLog::GfxClustersFull, "Frame %u: Adding Megacluster to queue - %p", currentFrameNum, part.get());
		mAddedMegaClusters[part.get()] = part;
 	} else {
		FASTLOG2(FLog::GfxClustersFull, "Frame %u: Adding part to queue - %p", currentFrameNum, part.get());
		mAddedParts[part.get()] = part;
	}
}

void SceneUpdater::queueAttachementToCreate(const boost::shared_ptr<RBX::Instance>& instance)
{
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	mAddedAttachementInstances[instance.get()] = instance;
}

void SceneUpdater::processPendingAttachments()
{
	RBXPROFILER_SCOPE("Render", "processPendingAttachments");

	InstanceSet tmp;
	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		std::swap(tmp, mAddedAttachementInstances);
	}
	{
		// in dynamic clumping, we just create clumps of size 1.
		for(InstanceSet::iterator it = tmp.begin(); it != tmp.end(); ++it)
		{
			shared_ptr<Instance> p = it->second.lock();

			if (p && GfxBinding::isInWorkspace(p.get())) // part could have dissapeared before render ever happened.
			{
				addAttachment(p);
			}
		}
	}
}

void SceneUpdater::queueChunkInvalidateMegaCluster(RBX::GfxPart* part, const SpatialRegion::Id& pos, bool isWaterChunk)
{
	RBXASSERT(part->getPartInstance()->getPartType() == RBX::MEGACLUSTER_PART);
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	FASTLOG3(FLog::MegaClusterDirty, "Chunk queued for update: %u x %u x %u",
		pos.value().x, pos.value().y, pos.value().z);

	const Camera* camera = dataModel->getWorkspace()->getConstCamera();
	RBX::Vector3 testLocation(SpatialRegion::centerOfRegionInGlobalCoordStuds(pos).toVector3());
    
	float distance = (camera->getCameraCoordinateFrame().translation - testLocation).squaredMagnitude();

	if (distance < 256 * 256) { // radius = ~ 2 chunks
		mCloseChunkInvalidates.insert(mCloseChunkInvalidates.begin(),
			MegaClusterChunk(part, pos, isWaterChunk));
	} else if (distance < 2048 * 2048) { // radius = ~ 16 chunks
		mMiddleChunkInvalidates.insert(mMiddleChunkInvalidates.begin(),
			MegaClusterChunk(part, pos, isWaterChunk));
	} else {
		mFarChunkInvalidates.insert(mFarChunkInvalidates.begin(),
			MegaClusterChunk(part, pos, isWaterChunk));
	}
}

void SceneUpdater::queueFullInvalidateMegaCluster(RBX::GfxPart* part)
{
	RBXASSERT(part->getPartInstance()->getPartType() == RBX::MEGACLUSTER_PART);
	FASTLOG1(FLog::MegaClusterInit, "Full cluster queued for update - %p", part);
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	mFullInvalidatedClusters.insert(part);
}

void SceneUpdater::queueInvalidatePart(RBX::GfxPart* part)
{
	FASTLOG2(FLog::GfxClustersFull, "Frame %u: Queue invalidate part: %p", currentFrameNum, part);
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	mInvalidatedParts.insert(part);
}

void SceneUpdater::queueInvalidateFastCluster(RBX::GfxPart* cluster)
{
	FASTLOG2(FLog::GfxClustersFull, "Frame %u: Queue invalidate fast cluster: %p", currentFrameNum, cluster);

	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	mInvalidatedFastClusters.insert(cluster);
}

void SceneUpdater::queuePriorityInvalidateFastCluster(RBX::GfxPart* cluster)
{
	FASTLOG2(FLog::GfxClustersFull, "Frame %u: Queue priority invalidate fast cluster: %p", currentFrameNum, cluster);
	mPriorityInvalidateFastClusters.insert(cluster);
}

void SceneUpdater::notifyWaitingForAssets(RBX::GfxPart* part, const std::vector<RBX::ContentId>& ids)
{	
    std::vector<RBX::ContentId> uids = ids;

    std::sort(uids.begin(), uids.end());
    uids.erase(std::unique(uids.begin(), uids.end()), uids.end());

    FASTLOG3(FLog::GfxClustersFull, "Cluster %p: notify waiting for %d assets (%d unique)", part, ids.size(), uids.size());
		
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);

    mWaitingParts.erase(part);

    for (size_t i = 0; i < uids.size(); ++i)
        mWaitingParts.insert(std::make_pair(part, uids[i]));
}

void SceneUpdater::updateAllInvalidParts(bool bulkExecution)
{
	RBXPROFILER_SCOPE("Render", "updateInvalidParts");

	GfxPartSet tmp;
	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);

		if(mInvalidatedParts.size() > 0)
			FASTLOG2(FLog::GfxClusters, "Total invalidated parts: %u, bulkExecution: %u", mInvalidatedParts.size(), bulkExecution);

		if(mInvalidatedParts.size() < MAX_INVALIDATIONS_PER_FRAME || bulkExecution)
		{
			std::swap(tmp, mInvalidatedParts);
		}
		else
		{
			GfxPartSet::iterator it = mInvalidatedParts.begin();
			while(tmp.size() < MAX_INVALIDATIONS_PER_FRAME)
			{
				tmp.insert(*it);
				mInvalidatedParts.erase(it++);
			}
		}
	}

    if(!tmp.empty())
		FASTLOG1(FLog::GfxClusters, "Invalidating entities: %u", tmp.size());

	for(GfxPartSet::iterator it = tmp.begin(); it != tmp.end(); ++it)
	{
		// remove this part from the waiting list if it managed to find it's way in the invalidation queue (it could get deleted!)
        mWaitingParts.erase(*it);

		FASTLOG1(FLog::GfxClustersFull, "Updating entity on part cluster: %p", *it);

		(*it)->updateEntity();
	
	}
}

struct WaitingPart
{
    GfxPart* part;
    std::multimap<GfxPart*, ContentId>::iterator assets;

    float distance;
    unsigned int assetCount;
    
    bool operator<(const WaitingPart& other) const
    {
        return distance < other.distance;
    }

    bool isNewContentAvailable(ContentProvider* contentProvider) const
    {
        auto it = assets;
        
        for (unsigned int i = 0; i < assetCount; ++i)
        {
            const ContentId& id = it->second;

            if (contentProvider->hasContent(id) || contentProvider->isUrlBad(id))
                return true;

            ++it;
        }
        
        return false;
    }
};

void SceneUpdater::updateWaitingParts(bool bulkExecution)
{
	RBXPROFILER_SCOPE("Render", "updateWaitingParts");

	if(!mWaitingParts.empty())
		FASTLOG1(FLog::GfxClusters, "Waiting parts, pass begin: %u", mWaitingParts.size());

	ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(dataModel.get());

	// Gather all clusters that are waiting for assets
	std::vector<WaitingPart> waitingParts;

	for (AssetPartMap::iterator it = mWaitingParts.begin(); it != mWaitingParts.end(); )
	{
		float distance = (it->first->getCenter() - pointOfInterest).squaredLength();
		
		WaitingPart part = { it->first, it, distance, 0 };

		// Scan through assets with the same part (map is sorted by part pointer)
		while (it != mWaitingParts.end() && it->first == part.part)
		{
			++it;
			part.assetCount++;
		}
			
		waitingParts.push_back(part);
	}
	
	// Sort all clusters so that we process front-to-back
	std::sort(waitingParts.begin(), waitingParts.end());
	
	// Process all clusters with a timeout - closest clusters get processed first
	Timer<Time::Precise> timer;
	
	for (auto& part: waitingParts)
	{
		// part.assets iterator must still point to a valid element since map iterators are only invalidated by erases of their keys
		RBXASSERT(part.assets->first == part.part);

		if (part.isNewContentAvailable(contentProvider))
		{
			// Remove all entries for this part from the list; they will reappear if the part still needs them and they are not available
			mWaitingParts.erase(part.part);

			part.part->updateEntity(/* assetsUpdated= */ true);
			
			if (!bulkExecution && timer.delta().msec() > FInt::FastClusterUpdateWaitingBudgetMs)
			{
				// come again next time!
				break;
			}
		}
	}
}

static void limitCopy(unsigned int maxSize, SceneUpdater::MegaClusterChunkList& source, SceneUpdater::MegaClusterChunkList& dest)
{
	while (dest.size() < maxSize && !source.empty())
	{
		dest.push_back(source.back());
		source.pop_back();
	}
}

void SceneUpdater::updateMegaClusters(bool bulkExecution)
{
	RBXPROFILER_SCOPE("Render", "updateClusters");

	GfxPartSet tmpFull;

	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		std::swap(tmpFull, mFullInvalidatedClusters);
	}

	if(tmpFull.size() > 0)
		FASTLOG1(FLog::GfxClusters, "Updating %u Full clusters", tmpFull.size());

	for(GfxPartSet::iterator it = tmpFull.begin(); it != tmpFull.end(); ++it)
	{
		(*it)->updateEntity();
	}

	MegaClusterChunkList tmpChunk;

	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		
		if (bulkExecution)
		{
			limitCopy(UINT_MAX, mCloseChunkInvalidates, tmpChunk);
			limitCopy(UINT_MAX, mMiddleChunkInvalidates, tmpChunk);
			limitCopy(UINT_MAX, mFarChunkInvalidates, tmpChunk);
		}
		else
		{
			limitCopy(5, mCloseChunkInvalidates, tmpChunk);
			limitCopy(7, mMiddleChunkInvalidates, tmpChunk);
			limitCopy(8, mFarChunkInvalidates, tmpChunk);
			limitCopy(8, mCloseChunkInvalidates, tmpChunk);
			limitCopy(8, mMiddleChunkInvalidates, tmpChunk);
			limitCopy(8, mFarChunkInvalidates, tmpChunk);
		}
	}
	
	if(tmpChunk.size() > 0)
		FASTLOG1(FLog::GfxClusters, "Updating %u cluster chunks", tmpChunk.size());

	mRenderStats->lastFrameMegaClusterChunks = 0;

	for(MegaClusterChunkList::iterator itChunk = tmpChunk.begin(); itChunk != tmpChunk.end(); ++itChunk)
	{
		itChunk->cluster->updateChunk(itChunk->chunkPos, itChunk->isWaterChunk);
		mRenderStats->lastFrameMegaClusterChunks++;
	}
}

void SceneUpdater::updateInvalidatedFastClusters(bool bulkExecution /* = false */)
{
	RBXPROFILER_SCOPE("Render", "updateInvalidatedFastClusters");

	mRenderStats->lastFrameFast.clusters = 0;
	mRenderStats->lastFrameFast.parts = 0;

	RBX::Timer<RBX::Time::Precise> timer;

	if(FLog::GfxClusters)
	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		if(!mPriorityInvalidateFastClusters.empty() || !mInvalidatedFastClusters.empty())
		{
			FASTLOG2(FLog::GfxClusters, "Invalidating Fast Clusters, PriorityInvalidatedClusters: %u, InvalidatedClusters: %u", mPriorityInvalidateFastClusters.size(), mInvalidatedFastClusters.size());
		}
	}
	
	do
	{
		GfxPart* cluster = NULL;
		
		if(mPriorityInvalidateFastClusters.empty())
		{
			RBX::mutex::scoped_lock scoped_lock(queue_mutex);

			if (mInvalidatedFastClusters.empty())
				break;
				
			cluster = *mInvalidatedFastClusters.begin();
			mInvalidatedFastClusters.erase(mInvalidatedFastClusters.begin());		
			FASTLOG3(FLog::GfxClusters, "Updating invalidated cluster %p (remaining: %u, bulkExecution: %u)", cluster, mPriorityInvalidateFastClusters.size(), bulkExecution);
		}
		else
		{
			cluster = *mPriorityInvalidateFastClusters.begin();
			mPriorityInvalidateFastClusters.erase(mPriorityInvalidateFastClusters.begin());
		}
		
		RBXASSERT(cluster);

		mRenderStats->lastFrameFast.clusters++;
		mRenderStats->lastFrameFast.parts += cluster->getPartCount();
	
		// update cluster
		cluster->updateEntity();
	}
	while (bulkExecution || !mPriorityInvalidateFastClusters.empty() || timer.delta().msec() <= CLUSTER_INVALIDATE_FRAME_BUDGET_MS);
}

bool SceneUpdater::arePartsWaitingForAssets()
{
	return !mWaitingParts.empty();
}

size_t SceneUpdater::getUpdateQueueSize() const
{
	return mInvalidatedFastClusters.size() + mPriorityInvalidateFastClusters.size();
}

void SceneUpdater::notifyAwake(RBX::GfxPart* part)
{
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);

	FASTLOG1(FLog::GfxClustersFull, "notifyAwake, adding cluster %p to dynamic nodes", part);

	mDynamicNodes.insert(part);
}


void SceneUpdater::notifySleeping(RBX::GfxPart* part)
{
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	GfxPartSet::iterator it = mDynamicNodes.find(part);
	if(it != mDynamicNodes.end())
	{
		FASTLOG1(FLog::GfxClustersFull, "NotifySleeping for cluster %p, erasing from dynamic nodes", part);
		mDynamicNodes.erase(it);
	}
}

struct IsChunkFromCluster
{
	GfxPart* part;

	IsChunkFromCluster(GfxPart* part): part(part)
	{
	}

	bool operator()(const SceneUpdater::MegaClusterChunk& chunk) const
	{
		return chunk.cluster == part;
	}
};

void SceneUpdater::notifyDestroyed(RBX::GfxPart* part)
{
	// clear deleted parts from invalid/waiting lists.
	RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	
	mInvalidatedParts.erase(part);
	mWaitingParts.erase(part);
	mDynamicNodes.erase(part);
	mFastClustersToCheck.erase(part);
	mFastClustersToCheckFW.erase(part);
	mPriorityInvalidateFastClusters.erase(part);
	mInvalidatedFastClusters.erase(part);

	if (mMegaClusters.count(part))
	{
		mMegaClusters.erase(part);

		mFullInvalidatedClusters.erase(part);
		mCloseChunkInvalidates.erase(std::remove_if(mCloseChunkInvalidates.begin(), mCloseChunkInvalidates.end(), IsChunkFromCluster(part)), mCloseChunkInvalidates.end());
		mMiddleChunkInvalidates.erase(std::remove_if(mMiddleChunkInvalidates.begin(), mMiddleChunkInvalidates.end(), IsChunkFromCluster(part)), mMiddleChunkInvalidates.end());
		mFarChunkInvalidates.erase(std::remove_if(mFarChunkInvalidates.begin(), mFarChunkInvalidates.end(), IsChunkFromCluster(part)), mFarChunkInvalidates.end());

		if (MegaClusterInstance* terrain = Instance::fastDynamicCast<MegaClusterInstance>(part->getPartInstance()))
		{
			if (terrain->isSmooth() == (dynamic_cast<SmoothClusterBase*>(part) != NULL))
			{
				if (terrain->isSmooth())
					terrain->getSmoothGrid()->disconnectListener(this);
				else
					terrain->getVoxelGrid()->disconnectListener(this);
			}
			else
			{
				// terrain is smooth but the cluster is not smooth
				// this can happen when we do convert-to-smooth
				// however, in this case we don't really need to disconnect from old grid since it's dead at this point
			}
		}
	}
}

void SceneUpdater::queueFastClusterCheck(RBX::GfxPart* cluster, bool isFW)
{
	// FIXME: Do we need to take queue_mutex here? Right now it seems to be protected by physics DM Write access
	FASTLOG2(FLog::GfxClustersFull, "Fast Cluster queued to check - %p, isFW - %u", cluster, isFW);
	if(isFW)
		mFastClustersToCheckFW.insert(cluster);
	else
		mFastClustersToCheck.insert(cluster);
}

void SceneUpdater::updateDynamicParts()
{
	RBXPROFILER_SCOPE("Render", "updateDynamicParts");

	RBX::Profiling::Mark mark(*mRenderStats->updateDynamicParts, true, true);

	std::vector<GfxPart*> tmp;
	tmp.reserve(mDynamicNodes.size());
	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		std::copy(mDynamicNodes.begin(), mDynamicNodes.end(), std::back_inserter(tmp));
	}
	if(!mDynamicNodes.empty())
		FASTLOG1(FLog::GfxClusters, "Dynamic parts to update: %u", mDynamicNodes.size());

	//update the cframes of all the dynamic objects

	for(std::vector<GfxPart*>::iterator childit = tmp.begin(); childit != tmp.end(); ++childit )
	{
		GfxPart* aggNode = *childit;
		
        aggNode->updateCoordinateFrame();
	}

}

void SceneUpdater::processPendingMegaClusters()
{
	RBXPROFILER_SCOPE("Render", "processPendingClusters");

	if(mAddedMegaClusters.size() > 0)
		FASTLOG1(FLog::GfxClusters, "Processing %u new MegaClusters", mAddedMegaClusters.size());

	PartInstanceSet localCopy;

	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
		std::swap(localCopy, mAddedMegaClusters);
	}

	for(PartInstanceSet::iterator it = localCopy.begin(); it != localCopy.end(); ++it)
	{
		shared_ptr<RBX::PartInstance> part = it->second.lock();
		if(part)
			addMegaCluster(part);
	}
}

void SceneUpdater::processPendingParts(bool priorityParts)
{
	RBXPROFILER_SCOPE("Render", "processPendingParts");

	PartInstanceSet localCopy;
	
	{
		RBX::mutex::scoped_lock scoped_lock(queue_mutex);
	
		localCopy.swap(mAddedParts);
	}
	
	if(!localCopy.empty())
		FASTLOG2(FLog::GfxClusters, "Added parts to process: %u, priority: %u", localCopy.size(), priorityParts);

	for(PartInstanceSet::iterator it = localCopy.begin(); it != localCopy.end(); ++it)
	{
		shared_ptr<PartInstance> p = it->second.lock();

		if(p && GfxBinding::isInWorkspace(p.get()))
		{
			p->setCookie(PartCookie::compute(p.get()));

			addFastPart(p, /* isFW= */ isPartStatic(p.get()), priorityParts);
		}
		else
		{
			FASTLOG2(FLog::GfxClustersFull, "Part %p died or removed from workspace before we could process it (died: %u)", it->first, p.get() == NULL);
		}
	}
}

void SceneUpdater::updatePrepare(unsigned long currentFrameNum, const RBX::Frustum& updateFrustum)
{
	RBXPROFILER_SCOPE("Render", "UpdatePrepare");

	this->currentFrameNum = currentFrameNum;
	this->updateFrustum = updateFrustum;

    FASTLOG2(FLog::GfxClusters, "Scene updater %p update cycle, current frame num: %u", this, currentFrameNum);

	RBX::Profiling::Mark mark(*mRenderStats->updateSceneGraph, true, true);

    processPendingParts(false);
    processPendingMegaClusters();
    processPendingAttachments();

    updateMegaClusters(mSettings->getEagerBulkExecution());
    updateAllInvalidParts(mSettings->getEagerBulkExecution());
    updateWaitingParts(mSettings->getEagerBulkExecution());

    checkFastClusters();
    processPendingParts(true);
    updateInvalidatedFastClusters(mSettings->getEagerBulkExecution());

    updateDynamicParts();

    computeLightingPrepare();

	FASTLOG(FLog::GfxClusters, "Scene updater finish");
}

void SceneUpdater::updatePerform()
{
    computeLightingPerform();
}

void SceneUpdater::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	// info.position is in terrain voxel coordinates; convert to studs before using
	G3D::Vector3 terrainCoordInStuds = Voxel::cellToWorld_center(info.position);

	if (LightGrid* lgrid = mVisualEngine->getLightGrid())
	{
		if (mLightingActive)
		{
			lgrid->invalidatePoint(terrainCoordInStuds, LightGridChunk::Dirty_OccupancyAndDependents | LightGridChunk::Dirty_HighPriority);
		}
	}
}

void SceneUpdater::onTerrainRegionChanged(const Voxel2::Region& region)
{
	if (LightGrid* lgrid = mVisualEngine->getLightGrid())
	{
		if (mLightingActive)
		{
			Vector3 begin = Voxel::cellSpaceToWorldSpace(region.begin().toVector3());
			Vector3 end = Voxel::cellSpaceToWorldSpace(region.end().toVector3());

			lgrid->invalidateExtents(Extents(begin, end), LightGridChunk::Dirty_OccupancyAndDependents);
			lgrid->invalidatePoint((begin + end) / 2, LightGridChunk::Dirty_HighPriority);
		}
	}
}

void SceneUpdater::lightingInvalidateOccupancy(const RBX::Extents& extents, const RBX::Vector3& highPriorityPoint, bool isFixed)
{
	if (LightGrid* lgrid = mVisualEngine->getLightGrid())
	{
		if (mLightingActive && (isFixed || lgrid->getNonFixedPartsEnabled()))
		{
			lgrid->invalidateExtents(extents, LightGridChunk::Dirty_OccupancyAndDependents);
			lgrid->invalidatePoint(highPriorityPoint, LightGridChunk::Dirty_HighPriority);
		}
	}
}

void SceneUpdater::lightingInvalidateLocal(const RBX::Extents& extents)
{
	if (LightGrid* lgrid = mVisualEngine->getLightGrid())
	{
		checkAndActivateLighting();
		
		lgrid->invalidateExtents(extents, LightGridChunk::Dirty_LightingLocal);
	}
}

void SceneUpdater::checkAndActivateLighting()
{
	if (!mLightingActive)
	{
		mLightingActive = true;
		
		// inactive lighting muted all occupancy updates; re-issue them
		if (LightGrid* lgrid = mVisualEngine->getLightGrid())
		{
			lgrid->invalidateAll(LightGridChunk::Dirty_Occupancy);
		}
	}
}

void SceneUpdater::setComputeLightingEnabled(bool value)
{
	computeLightingEnabled = value;
}

void SceneUpdater::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor)
{
    if (*descriptor==Lighting::desc_Outlines)
    {
        invalidateAllFastClusters();
    } 
}

void SceneUpdater::computeLightingPrepare()
{
	RBXPROFILER_SCOPE("Render", "computeLightingPrepare");

    bool bulkExecution = mSettings->getEagerBulkExecution();

    GlobalShaderData& globalShaderData = mVisualEngine->getSceneManager()->writeGlobalShaderData();

    // Dummy setup so that light grid sampling still works without the grid
    globalShaderData.LightConfig0 = Vector4();
    globalShaderData.LightConfig1 = Vector4();
    globalShaderData.LightConfig2 = Vector4();
    globalShaderData.LightConfig3 = Vector4(-1, -1, -1, 0);
    globalShaderData.LightBorder = Vector4(0, 0, 0, 1);

    if (!computeLightingEnabled)
        return;

    mLastOccupancyUpdates = 0;

    if (!computeLightingEnabled)
        return;

    if (LightGrid* lgrid = mVisualEngine->getLightGrid()) 
    {
        ContactManager* contactManager = dataModel->getWorkspace()->getWorld()->getContactManager();
        MegaClusterInstance* terrain = boost::polymorphic_downcast<MegaClusterInstance*>(dataModel->getWorkspace()->getTerrain());
        RBX::Camera* camera = dataModel->getWorkspace()->getCamera();
        RBX::Lighting* lighting = ServiceProvider::find<Lighting>(dataModel.get());

        // Make sure lighting is active if global shadows are on
        if (lighting->getGlobalShadows())
        {
            checkAndActivateLighting();
        }

        // Compute focus point; note that the computation is different in play mode and studio mode
    	if (camera->getCameraSubject())
        {
			mFocusPoint = camera->getCameraFocus().translation;
        }
        else 
        {
			RBX::Vector3 hit = RBX::Vector3::zero();

			if (FFlag::FixCameraTargetStudio)
			{
				RBX::Vector3 origin = mVisualEngine->getSceneManager()->getMinumumSqDistanceCenter();
				RBX::Vector3 direction = camera->getCameraCoordinateFrame().vectorToWorldSpace(RBX::Vector3(0.0f,0.0f,-1.0f));

				hit = origin + direction;
			}
			else
			{
				// TODO: getMinumumSqPartDistance will get data from previous frame, fix this when we move compute lighting to perform
				float minSqDistance = mVisualEngine->getSceneManager()->getMinumumSqPartDistance();
				float distance = (minSqDistance == FLT_MAX ? 0 : sqrt(minSqDistance));

				// Has to be consistent with minSqDistance, so take it from SceneManager, not from camera
				RBX::Vector3 origin = mVisualEngine->getSceneManager()->getMinumumSqDistanceCenter(); 
				// WARN: still taking direction from camera, might mean one frame delay
				RBX::Vector3 direction = camera->getCameraCoordinateFrame().vectorToWorldSpace(RBX::Vector3(0.0f,0.0f,-1.0f));
				hit = origin + direction*distance;

				direction.y = 0;
				hit += direction * 16*4.0f;
			}

            mFocusPoint = hit;
        }

        unsigned chunkBudget = getChunkBudget();

        // Relocate the grid if focus point moved far enough
        // Note: we can skip updating chunk contents (filling with dummy color and uploading) if lighting is not active
        mLightgridMoved = lgrid->updateGridCenter(mFocusPoint, /* skipChunkUpdates= */ !mLightingActive);

        mOccupancyPartCache.clear();
        mLgridchunksToUpdate.clear();

        if ((!mLightgridMoved || bulkExecution) && mLightingActive)
        {
            lgrid->updateAgePriorityForChunks(mFocusPoint);

            // Update global light attributes for the light grid
            if (1)
            {
                lgrid->setLightShadows(lighting->getGlobalShadows());

                if (lighting->getGlobalShadows())
                {
                    lgrid->setSkyAmbient(Color3uint8(lighting->getSkyAmbient()));
                }
                else
                {
                    lgrid->setSkyAmbient(Color3uint8());
                }
            }

            if (lighting->getGlobalShadows())
            {
                lgrid->setLightDirection(-lighting->getSkyParameters().lightDirection.unit());
            }

            for (unsigned i = 0; i < chunkBudget; ++i)
            {
                LightGridChunk* chunk = NULL;
                if (bulkExecution)
                    chunk = lgrid->findFirstDirtyChunk();
                else if (mAgeDirtyProportion == FLog::RenderLightGridAgeProportion)
                {
                    chunk = lgrid->findOldestChunk();
                    mAgeDirtyProportion = 0;
                } 
                else
                {
                    chunk = lgrid->findDirtyChunk();
                    mAgeDirtyProportion++;
                }

                if (!chunk)
                    break;

                RBXASSERT(chunk->dirty);

                if (chunk->dirty & LightGridChunk::Dirty_Occupancy)
                {
                    lgrid->occupancyUpdateChunkPrepare(*chunk, terrain, contactManager, mOccupancyPartCache);
                    mLastOccupancyUpdates++;
                }

                mLgridchunksToUpdate.push_back( std::make_pair( chunk, (unsigned)chunk->dirty ) );

                // reset chunk dirty flag so that findDirtyChunk does not pick it up again
                chunk->dirty = 0;
                chunk->age   = 0;
            }

			// restore chunk dirty flags so that we get correct state in perform
			for (auto& chunk: mLgridchunksToUpdate)
				chunk.first->dirty = chunk.second;
        }

    }
}

void SceneUpdater::computeLightingPerform()
{
	RBXPROFILER_SCOPE("Render", "computeLightingPerform");

    bool bulkExecution = mSettings->getEagerBulkExecution();
    GlobalShaderData& globalShaderData = mVisualEngine->getSceneManager()->writeGlobalShaderData();

	if (LightGrid* lgrid = mVisualEngine->getLightGrid()) 
	{
		RBX::Timer<RBX::Time::Benchmark> timer;

		mLastLightingUpdates = 0;

		SpatialHashedScene* spatialHashedScene = mVisualEngine->getSceneManager()->getSpatialHashedScene();

		// Don't update lighting in the same frame where we move the grid to reduce relocation stall
		// Don't update lighting unless it is active
		// Do update lighting if bulkExecution is true so that one pass is enough
		if ((!mLightgridMoved || bulkExecution) && mLightingActive)
		{
			RBX::Timer<RBX::Time::Precise> timer;
		
			lgrid->updateBorderColor(mFocusPoint, updateFrustum);

            lgrid->occupancyUpdateChunkPerform(mOccupancyPartCache);

            unsigned chunkBudget = getChunkBudget();
 
            if ((!mLightgridMoved || bulkExecution) && mLightingActive)
            {
                for (unsigned i = 0; i < chunkBudget; ++i)
                {
                    // now the tricky part...
                    LightGridChunk* chunk = NULL;
                    
                    if( i < mLgridchunksToUpdate.size() ) // first, try the chunk cache from prepare
                    {
                        chunk = mLgridchunksToUpdate[i].first;
                    }
                    else // otherwise, we have some budget left, try other dirty chunks
                    {
                        if (bulkExecution)
                            chunk = lgrid->findFirstDirtyChunk();
                        else
                            chunk = lgrid->findDirtyChunk();
                    }

                    if(!chunk)
                        break; // nothing to update

				    if (chunk->dirty & (LightGridChunk::Dirty_LightingLocal | LightGridChunk::Dirty_LightingLocalShadowed))
					    lgrid->lightingUpdateChunkLocal(*chunk, spatialHashedScene);

				    if (chunk->dirty & LightGridChunk::Dirty_LightingGlobal)
					    lgrid->lightingUpdateChunkGlobal(*chunk);
				
				    if (chunk->dirty & LightGridChunk::Dirty_LightingSkylight)
					    lgrid->lightingUpdateChunkSkylight(*chunk);
				
				    if (chunk->dirty & (LightGridChunk::Dirty_LightingGlobal | LightGridChunk::Dirty_LightingSkylight))
					    lgrid->lightingUpdateChunkAverage(*chunk);

				    mLastLightingUpdates++;
				
				    if (!bulkExecution)
					    lgrid->lightingUploadChunk(*chunk);

				    chunk->dirty = 0;
				    chunk->age = 0;
			    }

                if (bulkExecution && mLastLightingUpdates != 0)
                    lgrid->lightingUploadAll();

				lgrid->lightingUploadCommit();

                if (mLastOccupancyUpdates > 0 || mLastLightingUpdates > 0)
                {
                    FASTLOG3(FLog::RenderLightGrid, "LightGrid: Updated %d chunks in %d usec (occupancy: %d chunks)", mLastLightingUpdates, (int)(timer.delta().msec() * 1000), mLastOccupancyUpdates);
                }

            }

		}
		
		// Pass grid offset to shader via global data
        RBX::Vector3 gridOffset = lgrid->getGridCornerOffset();
        RBX::Vector3 gridSize = lgrid->getGridSize();
        RBX::Color4uint8 borderColor = lgrid->getBorderColor();
        
        float frmRadius = mVisualEngine->getFrameRateManager()->getLightGridRadius();
        
        RBX::Vector3 gridCenter = gridOffset + gridSize / 2.f;
        RBX::Vector3 gridRadius = RBX::Vector3(frmRadius, gridSize.y / 2.f, frmRadius).min(gridSize / 2.f);

        // world space -> texture space: v * scale + offset
        RBX::Vector3 gridTextureScale = RBX::Vector3(1.f / gridSize.x, 1.f / gridSize.y, 1.f / gridSize.z);

        // note: technically we can make the offset zero - the texture is wrapped and shifted so that
        // the world space to texture space mapping is constant.
        // however, to improve precision we'd like the transformed coordinates to be small, so we need to offset.
        RBX::Vector3 gridTextureOffset = lgrid->getWrapSafeOffset() * gridTextureScale;
        
        // for 2D texture sampling we have to offset the texture space by half of the texel in Y so that nearest sampling on LUT works
        if (lgrid->hasTexture() && lgrid->getTexture()->getType() == Texture::Type_2D)
        {
            float gridDepth = lgrid->getChunkCount().y * kLightGridChunkSizeY;

            gridTextureOffset.y -= 0.5f / gridDepth;

            // iPad4 has a precision issue with perspective interpolation: a constant output from VS is
            // interpolated with precision issues. For voxel-aligned surfaces the sampling coordinate is transformed
            // to an integer in texture space - which is a boundary condition for frac() and nearest filtering.
            // Add a small offset so that at least voxel-aligned surfaces are perfect.
            gridTextureOffset.y += 0.01f / gridDepth;
        }

        // half-size of the grid in Y direction is 32 pixels, so need at least 0.5/32=0.015625 wide border
        // if an extra half-texel offset is applied, the border shifts by the full texel => 1/32=0.03125
        RBX::Vector3 gridRadiusEffective = gridRadius * 0.95f;
        
        RBX::Vector3 gridTextureSpaceCenter = gridCenter * gridTextureScale + gridTextureOffset;
        RBX::Vector3 gridTextureSpaceRadius = gridRadiusEffective * gridTextureScale;
        
        // Note: the parameters have swizzle .yxz pre-applied so that we can avoid doing it in PS (helps ps_2_0 and GLSLES)
        globalShaderData.LightConfig0 = Vector4(gridTextureScale.y, gridTextureScale.x, gridTextureScale.z, 0);
        globalShaderData.LightConfig1 = Vector4(gridTextureOffset.y, gridTextureOffset.x, gridTextureOffset.z, 0);
        globalShaderData.LightConfig2 = Vector4(gridTextureSpaceCenter.y, gridTextureSpaceCenter.x, gridTextureSpaceCenter.z, 0);
        globalShaderData.LightConfig3 = Vector4(gridTextureSpaceRadius.y, gridTextureSpaceRadius.x, gridTextureSpaceRadius.z, 0);
        globalShaderData.LightBorder = Vector4(borderColor.r / 255.f, borderColor.g / 255.f, borderColor.b / 255.f, borderColor.a / 255.f);

		// Update grid settings from FRM
		// Make sure it's at the end so that FastCluster updates and lighting updates see the same value during one frame
		lgrid->setNonFixedPartsEnabled(mVisualEngine->getFrameRateManager()->getLightingNonFixedEnabled());
		
		mLightingComputeAverage.sample(timer.delta().msec());
	}
}

void SceneUpdater::checkFastClusters()
{
	RBXPROFILER_SCOPE("Render", "checkFastClusters");

	mSeenFastClusters.resize(0);

	if(!mFastClustersToCheck.empty() || !mFastClustersToCheckFW.empty())
		FASTLOG2(FLog::GfxClusters, "Fast clusters to check for break: %u, fw: %u", mFastClustersToCheck.size(), mFastClustersToCheckFW.size());

	while (!mFastClustersToCheck.empty() || !mFastClustersToCheckFW.empty())
	{
		FastCluster* gfxcluster = NULL;
		if(!mFastClustersToCheckFW.empty())
		{
			gfxcluster = boost::polymorphic_downcast<FastCluster*>(*mFastClustersToCheckFW.begin());
			RBXASSERT(gfxcluster->isFW());
		}
		else
		{
			RBXASSERT(!mFastClustersToCheck.empty());
			gfxcluster = boost::polymorphic_downcast<FastCluster*>(*mFastClustersToCheck.begin());
		}
		 
		if(mSeenFastClusters.size() >= FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET)
			break;

		// Doesn't make sense to continue if just one slot left in the budget and current chunk is not there yet
		if(mSeenFastClusters.size() >= FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET-1 && !seenIndexBefore(gfxcluster->getSpatialIndex()))
			break;

		// erase has to happen before checkCluster because it can queue itself if over budget
		if(gfxcluster->isFW())
		{
			RBXASSERT(!mFastClustersToCheckFW.empty());
			mFastClustersToCheckFW.erase(mFastClustersToCheckFW.begin());
		}
		else
		{
			RBXASSERT(!mFastClustersToCheck.empty());
			mFastClustersToCheck.erase(mFastClustersToCheck.begin());
		}
		gfxcluster->checkCluster();
	} 

	RBXASSERT(mSeenFastClusters.size() <= FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET);
}

bool SceneUpdater::seenIndexBefore(const SpatialGridIndex& index)
{
	return std::find(mSeenFastClusters.begin(), mSeenFastClusters.end(), index) != mSeenFastClusters.end();;
}

bool SceneUpdater::checkAddSeenFastClusters(const SpatialGridIndex& index)
{
	if(!seenIndexBefore(index))
	{
		if(mSeenFastClusters.size() < FAST_CLUSTER_PRIORITY_INVALIDATE_BUDGET)
			mSeenFastClusters.push_back(index);
		else
			return false;
	}

	return true;
}

void SceneUpdater::addMegaCluster(const shared_ptr<RBX::PartInstance>& part)
{
	shared_ptr<MegaClusterInstance> terrain = shared_polymorphic_downcast<MegaClusterInstance>(part);

    GfxPart* cluster;

	if (terrain->isSmooth())
	{
		terrain->getSmoothGrid()->connectListener(this);

		if (FFlag::SmoothTerrainRenderLOD)
			cluster = new SmoothClusterLOD(mVisualEngine, part);
		else
			cluster = new SmoothClusterChunked(mVisualEngine, part);
	}
	else
	{
		terrain->getVoxelGrid()->connectListener(this);

        cluster = new MegaCluster(mVisualEngine, part);
	}

	mMegaClusters.insert(cluster);

    queueFullInvalidateMegaCluster(cluster);
}

RBX::Humanoid* SceneUpdater::getHumanoid(RBX::PartInstance* part)
{
    if (DFFlag::HumanoidCookieRecursive)
    {
        if (part->getCookie() & PartCookie::IS_HUMANOID_PART)
            return part->findAncestorModelWithHumanoid();
        else
            return NULL;
    }
    else
    {
    	RBX::Instance* parent = part->getParent();

    	// Regular humanoid part
    	if (part->getCookie() & PartCookie::IS_HUMANOID_PART)
    		return RBX::Humanoid::modelIsCharacter(parent);

    	// For the purposes of flex clustering, we treat accoutrements as humanoid parts since we can composit their textures
    	if (RBX::Instance::isA<RBX::Accoutrement>(parent) || RBX::Instance::isA<RBX::Tool>(parent))
    		return RBX::Humanoid::modelIsCharacter(parent->getParent());

    	return NULL;
    }
}

RBX::WindowAverage<double,double>::Stats SceneUpdater::getLightingTimeStats()
{
	return mLightingComputeAverage.getStats();
}

unsigned SceneUpdater::getLightOldestAge()
{
	if (LightGrid* lgrid = mVisualEngine->getLightGrid()) 
	{
		LightGridChunk* chunk = lgrid->findOldestChunk();
		if(chunk)
			return chunk->age;
	}

	return 0;
}

void SceneUpdater::addFastPart(const shared_ptr<RBX::PartInstance>& part, bool isFW, bool priorityPart)
{
    if (RBX::Humanoid* humanoid = getHumanoid(part.get()))
    {
        FastCluster*& cluster = mHumanoidClusters[humanoid];

        if (!cluster)
            cluster = new FastCluster(mVisualEngine, humanoid, NULL, false);

        cluster->addPart(part);

        cluster->invalidateEntity();
    }
    else
    {
        SpatialGridIndex index = mFastGridSC->getIndexUnsafe(part.get(), isFW ? SpatialGridIndex::fFW : 0);
        FastGridSC::Cell* cell = mFastGridSC->requestCell(index);
        SuperCluster*& cluster = cell->cluster;

        if (!cluster)
            cluster = new SuperCluster(mVisualEngine, mFastGridSC.get(), index, isFW);

        FastCluster* fc = cluster->addPart(part);

        if(priorityPart)
            fc->priorityInvalidateEntity();
        else
            fc->invalidateEntity();
    }
}

void SceneUpdater::invalidateAllFastClusters()
{
    std::vector<SuperCluster*> clusters = mFastGridSC->getClusters();

    for (unsigned i = 0; i < clusters.size(); ++i)
        clusters[i]->invalidateAllFastClusters();
}

void SceneUpdater::destroyAttachment(GfxPart* object)
{
    size_t count = mAttachments.erase(object);
    RBXASSERT(count == 1);

    delete object;
}

void SceneUpdater::destroyFastCluster(FastCluster* cluster)
{
    if (void* humanoid = cluster->getHumanoidKey())
    {
        size_t count = mHumanoidClusters.erase(humanoid);
        RBXASSERT(count == 1);
        delete cluster;
    }
    else
    {
        RBXASSERT( !"Owned FastClusters must be deleted via their owners (see SuperCluster.h)" );
    }
}

void SceneUpdater::destroySuperCluster( SuperCluster* cluster )
{
    size_t count = mFastGridSC->removeCell(cluster->getSpatialIndex());
    RBXASSERT( count == 1 );
    delete cluster;
}

void SceneUpdater::addAttachment(const shared_ptr<RBX::Instance>& instance)
{
	PartInstance* part = 0;
	if(RBX::Instance::fastDynamicCast<RBX::ForceField>(instance.get())) // Special case for forcefield - only put on character's Torso
		if (DFFlag::UseR15Character)
		{
			Humanoid *humanoid = RBX::Instance::fastDynamicCast<Humanoid>(instance->getParent()->findFirstChildByName2("Humanoid", false).get());;
			if (humanoid)
				part = humanoid->getVisibleTorsoSlow();
			else
				part = RBX::Instance::fastDynamicCast<PartInstance>(instance->getParent()->findFirstChildByName2("Torso", false).get());
		} 
		else 
		{
			part = RBX::Instance::fastDynamicCast<PartInstance>(instance->getParent()->findFirstChildByName2("Torso", false).get());
		}
	else
		part = RBX::Instance::fastDynamicCast<PartInstance>(instance->getParent());

	if (dynamic_cast<Effect*>(instance.get()))
	{
		if (shared_ptr<RBX::Light> light = Instance::fastSharedDynamicCast<RBX::Light>(instance))
		{
			LightObject* lightObject = new LightObject(mVisualEngine);
		
			lightObject->bind(shared_from(part), light);

            mAttachments.insert(lightObject);
		}
        else if (FFlag::RenderNewExplosionEnable && instance->isA<RBX::Explosion>())
        {
            ExplosionEmitter* emitter = new ExplosionEmitter(mVisualEngine);
            emitter->bind(shared_from(part), instance);
            mAttachments.insert(emitter);
        }
        else if (FFlag::CustomEmitterRenderEnabled && instance->isA<RBX::CustomParticleEmitter>())
        {
            CustomEmitter* emitter = new CustomEmitter(mVisualEngine);
            emitter->bind(shared_from(part), instance);
            mAttachments.insert(emitter);
        }
        else
		{
			ParticleEmitter* emitter = new ParticleEmitter(mVisualEngine);

            emitter->bind(shared_from(part), instance);

            mAttachments.insert(emitter);
		}
	}
}

unsigned SceneUpdater::getChunkBudget()
{
    unsigned chunkBudget = mVisualEngine->getFrameRateManager()->getLightingChunkBudget();
    if (mSettings->getEagerBulkExecution())
    {
        RBX::Vector3int32 chunkCount = mVisualEngine->getLightGrid()->getChunkCount();
        chunkBudget = chunkCount.x * chunkCount.y * chunkCount.z * kLightGridChunkSizeY;
    }
    return chunkBudget;
}

}
}
