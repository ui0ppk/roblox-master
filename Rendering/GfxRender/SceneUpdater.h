#pragma once

#include "util/Profiling.h"

#include "GfxBase/GfxPart.h"
#include "GfxBase/RenderStats.h"
#include "GfxBase/RenderSettings.h"

#include "voxel/CellChangeListener.h"
#include "voxel2/GridListener.h"

#include "RbxG3D/Frustum.h"

#include <boost/pool/pool_alloc.hpp>

namespace RBX
{
	class Humanoid;
    class MegaClusterInstance;

    namespace Voxel { struct DataModelPartCache; }
}

namespace RBX
{
namespace Graphics
{
    using RBX::Voxel::DataModelPartCache;
    class VisualEngine;
	class FastCluster;
	class SuperCluster;
	template <class Cluster> class SpatialGrid;
	struct SpatialGridIndex;
    struct LightGridChunk;

	class SceneUpdater:
		public Voxel::CellChangeListener,
		public Voxel2::GridListener
	{
	private:
		RBX::mutex queue_mutex;
		CRenderSettings* mSettings;
	
		// debugging.
		std::vector<rbx::signals::connection> connections;

	protected:
		boost::shared_ptr<RBX::DataModel> dataModel;
		RenderStats* mRenderStats; // for debugging.
		const RenderCaps* mRenderCaps;

		unsigned long currentFrameNum;
		RBX::Frustum updateFrustum;

	public:
        void unbind();

		// some of these should be made private?
		// call this in response to sleep events.
		void notifyAwake(RBX::GfxPart* part);
		void notifySleeping(RBX::GfxPart* part); 
		
		void notifyDestroyed(RBX::GfxPart* part);

		// call this when a part or a cluster's properties changed, and must be refreshed.
		void queueInvalidatePart(RBX::GfxPart* part);

		void queueInvalidateFastCluster(RBX::GfxPart* cluster);
		void queuePriorityInvalidateFastCluster(RBX::GfxPart* cluster);
		void queueFullInvalidateMegaCluster(RBX::GfxPart* part);
		void queueChunkInvalidateMegaCluster(RBX::GfxPart* part, const SpatialRegion::Id& pos, bool isWaterChunk);

        void invalidateAllFastClusters();

		void updatePrepare(unsigned long currentFrameNum, const RBX::Frustum& updateFrustum);
        void updatePerform();

		void notifyWaitingForAssets(RBX::GfxPart* part, const std::vector<RBX::ContentId>& ids);

		void queuePartToCreate(const boost::shared_ptr<RBX::PartInstance>& part);
		void queueAttachementToCreate(const boost::shared_ptr<RBX::Instance>& instance);

		void queueFastClusterCheck(RBX::GfxPart* cluster, bool isFW);

		bool arePartsWaitingForAssets();

		size_t getUpdateQueueSize() const;

        void setComputeLightingEnabled(bool value);

		// struct and typedef visible for testing
		struct MegaClusterChunk
		{
			MegaClusterChunk() : chunkPos(Vector3int16::zero()) {};
			MegaClusterChunk(RBX::GfxPart* part, const SpatialRegion::Id& pos, bool isWaterChunk)
				: cluster(part), chunkPos(pos), isWaterChunk(isWaterChunk) {};
			RBX::GfxPart* cluster;
			SpatialRegion::Id chunkPos;
			bool isWaterChunk;
		};
		typedef std::vector<MegaClusterChunk> MegaClusterChunkList;

		static bool isPartStatic(RBX::PartInstance* part);

		const boost::shared_ptr<RBX::DataModel>& getDataModel() const { return dataModel; }

	protected:
		rbx::signals::scoped_connection workspaceDescendantAddedConnection;
        rbx::signals::scoped_connection propertyChangedSignal;

		void addMegaCluster(const shared_ptr<RBX::PartInstance>& part);
		void addFastPart(const shared_ptr<RBX::PartInstance>& part, bool isFW, bool isPriorityPart);
		void addAttachment(const shared_ptr<RBX::Instance>& instance);

		typedef boost::unordered_set<RBX::GfxPart*, boost::hash<RBX::GfxPart*>, std::equal_to<RBX::GfxPart*>, boost::fast_pool_allocator<RBX::GfxPart*> > GfxPartSet;	
		GfxPartSet mFastClustersToCheck;
		GfxPartSet mFastClustersToCheckFW;
	private:
		void onWorkspaceDescendantAdded(boost::shared_ptr<RBX::Instance> descendant);

		void updateDynamicParts();
		void processPendingMegaClusters();
		void processPendingParts(bool priorityParts);
		void processPendingAttachments();
		void updateAllInvalidParts(bool bulkExecution);
		void updateWaitingParts(bool bulkExecution);
		void updateInvalidatedFastClusters(bool bulkExecution);
		void checkFastClusters();
        void computeLightingPrepare();
		void computeLightingPerform();

        void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* descriptor);

		void updateMegaClusters(bool bulkExecution);

		typedef boost::unordered_map<RBX::PartInstance*, boost::weak_ptr<RBX::PartInstance> > PartInstanceSet;		

		GfxPartSet mDynamicNodes;
		GfxPartSet mInvalidatedParts;

		GfxPartSet mInvalidatedFastClusters;
		GfxPartSet mPriorityInvalidateFastClusters;

		PartInstanceSet mAddedParts;
		PartInstanceSet mAddedMegaClusters;

		GfxPartSet mMegaClusters;

		GfxPartSet mFullInvalidatedClusters;
		MegaClusterChunkList mCloseChunkInvalidates;
		MegaClusterChunkList mMiddleChunkInvalidates;
		MegaClusterChunkList mFarChunkInvalidates;
		
		typedef boost::unordered_map<RBX::Instance*, boost::weak_ptr<RBX::Instance> > InstanceSet;		
		InstanceSet mAddedAttachementInstances;

		typedef std::multimap<RBX::GfxPart*, RBX::ContentId> AssetPartMap;
		AssetPartMap mWaitingParts; // parts waiting for content.

	public:
		typedef SpatialGrid<FastCluster>  FastGrid;
		typedef SpatialGrid<SuperCluster> FastGridSC;

		SceneUpdater(shared_ptr<RBX::DataModel> dataModel, VisualEngine* ve);
		~SceneUpdater();
	
		void destroyAttachment(GfxPart* object);
		void destroyFastCluster(FastCluster* cluster);
		void destroySuperCluster(SuperCluster* cluster);
		
		static RBX::Humanoid* getHumanoid(RBX::PartInstance* part);
		bool checkAddSeenFastClusters(const SpatialGridIndex& index);

		void lightingInvalidateOccupancy(const RBX::Extents& extents, const RBX::Vector3& highPriorityPoint, bool isFixed);
		void lightingInvalidateLocal(const RBX::Extents& extents);

		RBX::WindowAverage<double,double>::Stats getLightingTimeStats();
		unsigned getLastOccupancyUpdates() { return mLastOccupancyUpdates; }
		unsigned getLastLightingUpdates() { return mLastLightingUpdates; }
		unsigned getLightOldestAge();
		bool isLightingActive() { return mLightingActive; }
        
        void setPointOfInterest(const Vector3& poi) { pointOfInterest = poi; }

		/*override*/ virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);
		/*override*/ virtual void onTerrainRegionChanged(const Voxel2::Region& region);
	private:
		VisualEngine* mVisualEngine;

	private:
		typedef std::map<void*, FastCluster*> HumanoidClusterMap;
		HumanoidClusterMap mHumanoidClusters;
		scoped_ptr<FastGridSC> mFastGridSC;

		GfxPartSet mAttachments;

		std::vector<SpatialGridIndex> mSeenFastClusters;

		bool seenIndexBefore(const SpatialGridIndex& index);
		void checkAndActivateLighting();

		RBX::WindowAverage<double, double> mLightingComputeAverage;
		unsigned mLastOccupancyUpdates;
		unsigned mLastLightingUpdates;

		unsigned mAgeDirtyProportion;
		bool mLightingActive;

        bool computeLightingEnabled;

        std::vector< DataModelPartCache > mOccupancyPartCache;
        std::vector< std::pair<LightGridChunk*, unsigned> > mLgridchunksToUpdate; // .first = chunk, .second = cached chunk's dirty flags
        Vector3 mFocusPoint;
        bool    mLightgridMoved;
        unsigned getChunkBudget();
        
        Vector3 pointOfInterest;
	};
}
}
