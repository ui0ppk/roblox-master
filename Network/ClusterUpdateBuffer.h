#pragma once

#include <vector>

#include "util/G3DCore.h"
#include "util/UintSet.h"
#include "boost/cstdint.hpp"
#include "util/ClusterCellIterator.h"
#include "boost/unordered_map.hpp"

namespace RBX { 

// these constants visible for testing
const int kXZ_CHUNK_SIZE_AS_BITSHIFT = 5;
const int kXZ_CHUNK_SIZE_AS_BITMASK = 0x1f;
const int kY_CHUNK_SIZE_AS_BITSHIFT = 4;
const int kY_CHUNK_SIZE_AS_BITMASK = 0x0f;

namespace Network {

typedef Vector3int16 ClusterCellUpdate;
typedef unsigned int ChunkIndex;


struct ClusterUpdateBuffer {

private:
    typedef boost::unordered_map<SpatialRegion::Id, UintSet, SpatialRegion::Id::boost_compatible_hash_value> BitSetUpdateMap;
    BitSetUpdateMap::iterator lastFound;
    BitSetUpdateMap bitSetUpdates;

	size_t internalSize;
public:
	static void computeUintRepresentingLocationInChunk(
		const ClusterCellUpdate& update, unsigned int* out);
	static void computeGlobalLocationFromUintRepresentation(
		const unsigned int& info, const Vector3int16& baseCellOffset,
		ClusterCellUpdate* out);

	ClusterUpdateBuffer();

	size_t size() const;

	void push(const ClusterCellUpdate& inputData);

	bool chk(const ClusterCellUpdate& test);

	void pop(ClusterCellUpdate* out);

    static inline void nextCellInIterationOrder(const Vector3int16& cellpos, Vector3int16* out)
    {
        ClusterChunksIterator::nextCellInIterationOrder(cellpos, out);
    }
};

}

}
