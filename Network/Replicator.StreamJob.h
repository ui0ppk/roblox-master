#ifndef REPLICATOR_STREAMJOB_H_
#define REPLICATOR_STREAMJOB_H_

#include "Replicator.h"
#include "Replicator.JoinDataItem.h"
#include "ServerReplicator.h"
#include "Util.h"

#include "V8DataModel/PartInstance.h"
#include "V8World/DistributedPhysics.h"
#include "V8World/ContactManagerSpatialHash.h"
#include "util/StreamRegion.h"
#include "util/ObscureValue.h"

#include <boost/unordered_set.hpp>
#include "V8DataModel/Workspace.h"

namespace RBX {

namespace Network {

	static const int kStreamCenterResetThreshold = 32*32;

class Replicator::StreamJob : public ReplicatorJob,
	public ContactManagerSpatialHash::CoarseMovementCallback
{
public:
	class StreamDataItem;

    enum RegionIteratorSuccessor
    {
        ITER_NONE = 0, //00
        ITER_INCY = 1, //01
        ITER_INCZ = 2, //10
        ITER_INCX = 3, //11
    };

private:
	class StreamRegionIterator 
	{
		StreamRegion::Id worldMinRegion;
		StreamRegion::Id worldMaxRegion;
		
		StreamRegion::Id currentRegion;
		Vector3 centerPosition;
		StreamRegion::Id centerRegion;
		int regionRadius;
        int clientMaxRegionRadius;
        int serverMaxRegionRadius;

        Workspace* workspace;

	public:
		StreamRegionIterator(Replicator& replicator);
		
		bool resetCenter(const Vector3 &position, bool force);

		Extents possibleExtentsStreamed() const;
		const Vector3& getCenterPosition() const { return centerPosition; }
		const StreamRegion::Id& getCenterRegion() const { return centerRegion; }
		const StreamRegion::Id& getCurrentRegion() const { return currentRegion; }
		int getRadius() const { return regionRadius; }
        void setClientMaxRegionRadius(int val) { clientMaxRegionRadius = val; }
        int getMaxRegionRadius() const { return clientMaxRegionRadius < serverMaxRegionRadius ? clientMaxRegionRadius : serverMaxRegionRadius; }

		// Gets the next region in iteration order.  Returns false if we've reached end of iteration.
		bool getNextRegion(StreamRegion::Id &regionId);
		void updateWorldExtents();

		bool isRegionInTerrainBoundaries(const StreamRegion::Id &regionId) const;
	};

    typedef boost::unordered_set<StreamRegion::Id> RegionSet;
    RegionSet collectedRegions;

	const ObscureValue<float> dataSendRate;
	const PacketPriority packetPriority;

    int numClientInstanceQuota;
	int numCollectedInstances;
	StreamRegionIterator regionIterator;
    bool readyToWork;
    int numPacketsPerStep;
    RBX::Timer<RBX::Time::Fast> networkHealthCheckTimer;

	typedef std::deque<StreamDataItem*> PendingItemsList;
	PendingItemsList pendingStreamItems;

	typedef std::multimap<float, StreamDataItem*> SortedPendingItemsList;
	SortedPendingItemsList pendingHighPriorityStreamItems;

    ContactManagerSpatialHash* spatialHash;

	rbx::signals::scoped_connection playerSpawningConnection;
	rbx::signals::scoped_connection playerCharacterAddedConnection;
	rbx::signals::scoped_connection playerTorsoPropChangedConnection;

	// stats
	friend class ServerReplicator::ServerStatsItem;
	RunningAverageTimeInterval<> packetsSent;
	RunningAverage<int> avgInstancePerStep;
	RunningAverage<int> avgPacketSize;

	template<class ContainerIterator>
	StreamDataItem* iteratorToItem(ContainerIterator& iter);

public:

    StreamJob(Replicator& replicator);
	virtual ~StreamJob();

	void updateClientQuota(int diff, short maxRegionRadius);
	int getClientInstanceQuota() { return numClientInstanceQuota; }

	bool isRegionCollected(const StreamRegion::Id& regionId) const;
    bool isRegionInPendingStreamItemQueue(const StreamRegion::Id& regionId) const;
	bool isInStreamedRegions(const Extents& ext) const;
	bool isAreaInStreamedRadius(const Vector3& center, float radius) const;
	void setupListeners(Player* player);
	void adjustSimulationOwnershipRange(Region2::WeightedPoint* updatee);
    void setReady(bool isReady) {readyToWork = isReady;}
    bool getReady() {return readyToWork;}

	void readRegionRemoval(RakNet::BitStream& bitStream);
	void readInstanceRemoval(RakNet::BitStream& bitStream);

	void addInstanceToRegularQueue(shared_ptr<Instance> instance);
	void addInstanceAndDescendantsToRegularQueue(shared_ptr<Instance> instance);

    void unregisterCoarsePrimitiveCallback();

	// implement CoarseMovementCallback
	virtual void coarsePrimitiveMovement(Primitive* p, const UpdateInfo& info);
    void sendPackets(int maxPackets);

	template <class Container>
	int sendItems(Container& itemList, int maxInstances, Time syncTime);

    StreamRegion::Id lastSentRegionId;

private:
    float getPriorityAmplifier();
	virtual Time::Interval sleepTime(const Stats& stats);
	virtual Error error(const Stats& stats);
	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);

	bool streamTerrain(StreamDataItem &item);

	void addInstanceToStreamQueue(shared_ptr<Instance> instance, StreamDataItem *item);
	size_t addInstanceAndDescendantsToStreamQueue(shared_ptr<Instance> instance, StreamDataItem* item);
	void clearPendingItems();
	
	bool getNextUncollectedRegion(StreamRegion::Id &regionId);
	bool collectPartsForMinPlayerArea();
	bool collectPartsFromNextRegion(bool highPriority);

    void onPlayerCharacterAdd(shared_ptr<Instance> character);
    void onPlayerTorsoChanged(const Reflection::PropertyDescriptor* desc);
	bool setStreamCenterAndSentMinRegions(const Vector3& worldPos, const bool forceSet);
	void receiveInstanceGcMessage(const Guid::Data& id);
};

class DeserializedStreamDataItem : public DeserializedItem
{
public:
	StreamRegion::Id id;
	shared_ptr<DeserializedJoinDataItem> deserializedJoinDataItem;
	int successorBitMask;

	DeserializedStreamDataItem();
	~DeserializedStreamDataItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::StreamJob::StreamDataItem : public JoinDataItem
{
	StreamRegion::Id regionId;
	StreamRegion::Id& lastSentRegionId;
	int instancesSentLastWrite;
	bool streamedTerrain;

public:
	StreamDataItem(Replicator* replicator, const StreamRegion::Id &id, StreamRegion::Id &lastId) : JoinDataItem(replicator),
		regionId(id), lastSentRegionId(lastId), instancesSentLastWrite(0), streamedTerrain(false) 
	{
		timestamp = Time::nowFast();
	}

	virtual ~StreamDataItem() {}

	/* override */ bool write(RakNet::BitStream& bitStream);

	int getInstancesSentLastWrite() const {
		return instancesSentLastWrite;
	}

	const StreamRegion::Id getRegionId() const {
		return regionId;
	}

	bool didStreamTerrain() const {
		return streamedTerrain;
	}

	void setStreamedTerrain() {
		streamedTerrain = true;
	}

	static shared_ptr<DeserializedStreamDataItem> read(Replicator& replicator, RakNet::BitStream& inBitstream);
};

}}

#endif

