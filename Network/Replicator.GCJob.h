#pragma once

#include "ClientReplicator.h"
#include "Util.h"
#include "V8World/ContactManagerSpatialHash.h"
#include "Util/StreamRegion.h"
#include "GfxBase/IAdornable.h"
#include <vector>


namespace RBX {

namespace Network {
	
    static const float kMaxRegionExpansionTimerLimit = 10.f;
    static const float kRenderingDistanceUpdateInterval = 1.f;
	struct RegionInfo
	{
		float distance;
        int streamDistance;
		StreamRegion::Id id;

		explicit RegionInfo(StreamRegion::Id id) : id(id) {}

		bool operator==(const RegionInfo& other) const
		{
			return id == other.id;
		}

        void computeRegionDistance(const StreamRegion::Id& focus)
        {
            streamDistance = StreamRegion::Id::getRegionLongestAxisDistance(id, focus);
        }
	};

	typedef boost::unordered_map<StreamRegion::Id, RegionInfo> RegionsMap;
	typedef std::list<RegionsMap::iterator> RegionList;

	class ClientReplicator::GCJob : public ReplicatorJob,
		public ContactManagerSpatialHash::CoarseMovementCallback
	{	
		ContactManagerSpatialHash* spatialHash;
		RegionsMap streamedRegions;
		RegionList streamedRegionList;

		Vector3 lastPlayerPosition;
		int numSorted;
        int numRegionToGC;
        float* renderingDistance;
        float renderingRegionDistance;
        short gcRegionDistance;
        short maxRegionDistance;
        StreamRegion::Id centerRegion;

        RBX::Timer<RBX::Time::Fast> maxRegionExpansionTimer;
        RBX::Timer<RBX::Time::Fast> renderingDistanceUpdateTimer;

        std::vector<shared_ptr<PartInstance> > pendingRemovalPartInstances;

		class RegionRemovalItem;
		class InstanceRemovalItem;
	
	public:	
		GCJob(Replicator& replicator);
		virtual ~GCJob();
		void insertRegion(const StreamRegion::Id& id);

		// implement CoarseMovementCallback
		virtual void coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info);

		void render3dAdorn(Adorn* adorn);
        bool pendingGC()
        {
            return (numRegionToGC > 0);
        }
        void evaluateNumRegionToGC();

        short getMaxRegionDistance() { return maxRegionDistance; }
        bool updateMaxRegionDistance();
        void updateGcRegionDistance();
		void notifyServerGcingInstanceAndDescendants(shared_ptr<Instance> instance);

        void unregisterCoarseMovementCallback();

		// debug
		int getNumRegionsToGC() { return numRegionToGC; }
		short getGCDistance() { return gcRegionDistance; }
		int getNumRegions() { return streamedRegionList.size(); }
	
	private:
		virtual Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, replicator->settings().getDataGCRate());
		}
		virtual Error error(const Stats& stats);
		virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);

		void gcRegion(const StreamRegion::Id& regionId, RegionRemovalItem* removeItem);
		void gcPartInstance(PartInstance* partInstance, RegionRemovalItem* removeItem);

		void setMaxSimulationRadius(float radius);
	};


}} // name spaces
