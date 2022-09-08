#include "NetworkClusterPacketCache.h"

#include "v8datamodel/MegaCluster.h"

#include "voxel/Cell.h"
#include "voxel2/Grid.h"

// raknet
#include "BitStream.h"

using namespace RBX;
using namespace RBX::Network;

LOGGROUP(TerrainCellListener)

const char* const RBX::Network::sClusterPacketCacheBase = "ClusterPacketCacheBase";
const char* const RBX::Network::sClusterPacketCache = "ClusterPacketCache";
const char* const RBX::Network::sOneQuarterClusterPacketCache = "OneQuarterClusterPacketCacheBase";

template <class Key>
ClusterPacketCacheBase<Key>::ClusterPacketCacheBase()
{
}

template <class Key>
unsigned int ClusterPacketCacheBase<Key>::getCachedBitStreamBytesUsed(const Key &regionId)
{
	boost::shared_lock<boost::shared_mutex> lock(sharedMutex);

	CachedBitStream &cachedStream = streamCache[regionId];

	if (!cachedStream.dirty) {
		return cachedStream.bitStream->GetNumberOfBytesUsed();
	}

	return 0;
}

template <class Key>
bool ClusterPacketCacheBase<Key>::fetchIfUpToDate(const Key &regionId, RakNet::BitStream& outBitStream)
{
	boost::shared_lock<boost::shared_mutex> lock(sharedMutex);

	CachedBitStream &cachedStream = streamCache[regionId];

	if (!cachedStream.dirty)
	{
		outBitStream.WriteBits(cachedStream.bitStream->GetData(), cachedStream.bitStream->GetNumberOfBitsUsed(), false);
		return true;
	}

	return false;
}

// copy all data from bitStream to cache
template <class Key>
bool ClusterPacketCacheBase<Key>::update(const Key &regionId, RakNet::BitStream& bitStream, unsigned int numBits)
{
	boost::unique_lock<boost::shared_mutex> lock(sharedMutex);

	CachedBitStream &cachedStream = streamCache[regionId];

	if (!cachedStream.bitStream) {
		cachedStream.bitStream.reset(new RakNet::BitStream());
	}

	cachedStream.bitStream->Reset();
	cachedStream.bitStream->Write(bitStream, numBits);
	cachedStream.dirty = false;

	return true;
}

template <class Key>
void ClusterPacketCacheBase<Key>::setupListener(MegaClusterInstance* megaClusterInstance)
{
	if (!clusterInstance)
	{
		if (megaClusterInstance->isSmooth())
            megaClusterInstance->getSmoothGrid()->connectListener(this);
		else
            megaClusterInstance->getVoxelGrid()->connectListener(this);

		clusterInstance = shared_from(megaClusterInstance);
	}
}

template <class Key>
void ClusterPacketCacheBase<Key>::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		streamCache.clear();

		if (clusterInstance)
		{
			if (clusterInstance->isSmooth())
				clusterInstance->getSmoothGrid()->disconnectListener(this);
            else
                clusterInstance->getVoxelGrid()->disconnectListener(this);

			clusterInstance.reset();
		}
	}
}

// Explicit template instantiation
template class ClusterPacketCacheBase<StreamRegion::Id>;
template class ClusterPacketCacheBase<SpatialRegion::Id>;

void ClusterPacketCache::terrainCellChanged(const Voxel::CellChangeInfo& cell)
{
    streamCache[SpatialRegion::regionContainingVoxel(cell.position)].dirty = true;
}

void ClusterPacketCache::onTerrainRegionChanged(const Voxel2::Region& region)
{
    RBXASSERT(false);
}

void OneQuarterClusterPacketCache::terrainCellChanged(const Voxel::CellChangeInfo& cell)
{
    streamCache[StreamRegion::regionContainingVoxel(cell.position)].dirty = true;
}

void OneQuarterClusterPacketCache::onTerrainRegionChanged(const Voxel2::Region& region)
{
	std::vector<Vector3int32> ids = region.getChunkIds(StreamRegion::_PrivateConstants::kRegionSizeInVoxelsAsBitShift);

	for (auto i: ids)
		streamCache[StreamRegion::Id(i)].dirty = true;
}