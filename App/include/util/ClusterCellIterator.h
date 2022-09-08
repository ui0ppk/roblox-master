#pragma once

#include "G3DCore.h"
#include "rbx/Debug.h"
#include "Voxel/Cell.h"
#include "Voxel/Util.h"
#include "Util/StreamRegion.h"
#include "Util/SpatialRegion.h"

namespace RBX {

class ClusterChunksIterator
{
public:
	ClusterChunksIterator()
		: indexOfNextCellToIssue(0)
        , internalSize(0)
    {
    }

	explicit ClusterChunksIterator(const std::vector<SpatialRegion::Id>& chunks)
        : chunks(chunks)
		, indexOfNextCellToIssue(0)
        , internalSize(chunks.size() * kChunkSize)
    {
    }

	// single chunk
	explicit ClusterChunksIterator(const SpatialRegion::Id& chunk)
		: indexOfNextCellToIssue(0)
		, internalSize(kChunkSize)
	{
		chunks.push_back(chunk);
	} 

    static inline void nextCellInIterationOrder(const Vector3int16& cellpos, Vector3int16* out)
    {
        unsigned int index = (cellpos.x & 0x1f) | ((cellpos.z & 0x1f) << 5) | ((cellpos.y & 0x0f) << 10);

        if (index == kChunkSize - 1)
        {
            // last cell in a chunk, it does not matter what we return except that it has to be a different chunk
            *out = cellpos + Vector3int16(1, 0, 0);
        }
        else
        {
            unsigned int next = index + 1;
            Vector3int16 local = Vector3int16((next & 0x1f), ((next >> 10) & 0xf), ((next >> 5) & 0x1f));

            *out = SpatialRegion::globalVoxelCoordinateFromRegionAndRelativeCoordinate(SpatialRegion::regionContainingVoxel(cellpos), local);
        }
    }

	inline void pop(Vector3int16* out)
    {
        RBXASSERT(internalSize > 0);

        Vector3int16 local = Vector3int16((indexOfNextCellToIssue & 0x1f), ((indexOfNextCellToIssue >> 10) & 0xf), ((indexOfNextCellToIssue >> 5) & 0x1f));

        *out = SpatialRegion::globalVoxelCoordinateFromRegionAndRelativeCoordinate(chunks[indexOfNextCellToIssue / kChunkSize], local);

		indexOfNextCellToIssue++;
		internalSize--;
	}

	inline bool chk(const Vector3int16& pos) const
    {
		return internalSize > 0;
	}

	size_t size() const
    {
		return internalSize;
	}

private:
	std::vector<SpatialRegion::Id> chunks;
	size_t indexOfNextCellToIssue;
    size_t internalSize;

    enum { kChunkSize = Voxel::kXZ_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE };
};

// Cell iterator for 1/4 of a chunk.
struct OneQuarterClusterChunkCellIterator
{
	Vector3int16 cellOffset;
	unsigned short internalCell;
	unsigned short internalSize;
    StreamRegion::Id regionId;

	OneQuarterClusterChunkCellIterator()
	{
		setToStartOfStreamRegion(StreamRegion::Id(0,0,0));
	}

	void setToStartOfStreamRegion(const StreamRegion::Id &_regionId)
	{
        regionId = _regionId;
		internalSize = StreamRegion::getTotalVoxelVolumeOfARegion();
		internalCell = 0;
		cellOffset = StreamRegion::getMinVoxelCoordinateInsideRegion(regionId);
	}

	static inline void cellFromIndex(const Vector3int16 &cellOffset, unsigned int index, Vector3int16* out) {
		(*out) = cellOffset + Vector3int16(
			(index & 0xf),
			((index >> 8) & 0xf),
			((index >> 4) & 0xf));
	}
    static inline void nextCellInIterationOrder(const Vector3int16& cellpos, Vector3int16* out)
    {
        Vector3int16 offset = StreamRegion::getMinVoxelCoordinateInsideRegion(StreamRegion::regionContainingVoxel(cellpos));
        Vector3int16 delta = cellpos - offset;
        int index = delta.x | (delta.y << 8) | (delta.z << 4);
        //RBXASSERT(index+1 < (int)StreamRegion::getTotalVoxelVolumeOfARegion());
        cellFromIndex(offset, index+1, out);
    }

	inline void pop(Vector3int16* out) {
		RBXASSERT(internalSize);
		cellFromIndex(cellOffset, internalCell, out);
		++internalCell;
		--internalSize;
	}

	inline bool chk(const Vector3int16 &cellPos) const
    {
        return (internalSize > 0) && (StreamRegion::regionContainingVoxel(cellPos) == regionId);
	}
	inline size_t size() const {
		return internalSize;
	}
};

}
