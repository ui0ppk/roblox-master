#include "ClusterUpdateBuffer.h"

#include <vector>

#include "rbx/Debug.h"
#include "util/G3DCore.h"
#include "V8DataModel/MegaCluster.h"
#include "boost/cstdint.hpp"

namespace RBX { 

namespace Network {

void ClusterUpdateBuffer::computeUintRepresentingLocationInChunk(
		const ClusterCellUpdate& update,
		unsigned int* out) {

	(*out) =
		(update.x & kXZ_CHUNK_SIZE_AS_BITMASK) +
		((update.z & kXZ_CHUNK_SIZE_AS_BITMASK) << kXZ_CHUNK_SIZE_AS_BITSHIFT) +
		((update.y & kY_CHUNK_SIZE_AS_BITMASK) << (2 * kXZ_CHUNK_SIZE_AS_BITSHIFT));
}

void ClusterUpdateBuffer::computeGlobalLocationFromUintRepresentation(
		const unsigned int& index,
		const Vector3int16& baseCellOffset,
		ClusterCellUpdate* out) {

	(*out) = baseCellOffset;
	out->x += index & kXZ_CHUNK_SIZE_AS_BITMASK;
	out->z += (index >> kXZ_CHUNK_SIZE_AS_BITSHIFT) & kXZ_CHUNK_SIZE_AS_BITMASK;
	out->y += (index >> (2 * kXZ_CHUNK_SIZE_AS_BITSHIFT)) & kY_CHUNK_SIZE_AS_BITMASK;
}

ClusterUpdateBuffer::ClusterUpdateBuffer()
    : internalSize(0)
{
    lastFound = bitSetUpdates.begin();
}

size_t ClusterUpdateBuffer::size() const
{
	return internalSize;
}

void ClusterUpdateBuffer::push(const ClusterCellUpdate& inputData)
{
	unsigned int info;
	computeUintRepresentingLocationInChunk(inputData, &info);

    internalSize += bitSetUpdates[SpatialRegion::regionContainingVoxel(inputData)].insert(info);

    // insert potentially invalidates iterators; also this preserves the invariant that no updates can be before lastFound in the map
    lastFound = bitSetUpdates.begin();
}

bool ClusterUpdateBuffer::chk(const ClusterCellUpdate& test) {
	unsigned int info;
	computeUintRepresentingLocationInChunk(test, &info);

    BitSetUpdateMap::iterator it = bitSetUpdates.find(SpatialRegion::regionContainingVoxel(test));

    return it != bitSetUpdates.end() && it->second.contains(info);
}

void ClusterUpdateBuffer::pop(ClusterCellUpdate* out)
{
	RBXASSERT(internalSize > 0);

    for (BitSetUpdateMap::iterator it = lastFound; it != bitSetUpdates.end(); ++it)
    {
        UintSet& updateVector = it->second;

        if (updateVector.size() > 0) {
            lastFound = it;

            unsigned int intVal;
            updateVector.pop_smallest(&intVal);
            computeGlobalLocationFromUintRepresentation(intVal, SpatialRegion::inclusiveVoxelExtentsOfRegion(it->first).getMinPos(), out);
            internalSize--;
            return;
        }
    }

	RBXASSERT(false);
}

} // namespace Network
} // namespace RBX
