#pragma once
// suffix header file for Grid.h

#include "Util/SpatialRegion.h"
#include "Voxel/Water.h"

namespace RBX { namespace Voxel {

// Private storage structure supporting Grid.
// NOT TO BE USED ANYWHERE EXCEPT Grid.cpp AND Grid.h.
// Stores a contiguous 3-D box of terrain contents. Namespace contains
// constants and helper methods for accessing the data.
class Grid::Chunk {

private:
	
	bool initialized;
	unsigned int countOfNonEmptyCells;
	std::vector<Cell> data;
	std::vector<unsigned char> material;
	const Grid* owner;

	static const int kXOffsetMultiplier = 1;
	static const int kZOffsetMultiplier =
		SpatialRegion::Constants::kRegionXDimensionInVoxels;
	static const int kYOffsetMultiplier =
		SpatialRegion::Constants::kRegionXDimensionInVoxels *
		SpatialRegion::Constants::kRegionZDimensionInVoxels;

public:

	static const int kFaceDirectionToPointerOffset[7];

	static inline int voxelCoordOffsetToIndexOffset(const Vector3int16& offset) {
		return (offset * Vector3int16(kXOffsetMultiplier, kYOffsetMultiplier, kZOffsetMultiplier)).sum();
	}

	static inline unsigned int voxelCoordToArrayIndex(const Vector3int16& coord) {
		return voxelCoordOffsetToIndexOffset(
				SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(coord));
	}

	Chunk();
    ~Chunk();

	// Initialization method. Safe to call multiple times. This object owns
	// a significant amount of memory, so a separate init method was made to
	// allow explicit control over when that memory is allocated.
	void init(const Grid* owner);

	std::vector<Cell>& getData() { return data; }
	const std::vector<Cell>& getConstData() const { return data; }
	std::vector<unsigned char>& getMaterial() { return material; }
	const std::vector<unsigned char>& getConstMaterial() const { return material; }

	void updateCountOfNonEmptyCells(int delta) {
		countOfNonEmptyCells += delta;
		RBXASSERT((int)(countOfNonEmptyCells) >= 0);
	}
	bool hasNoUsefulData() const {
		return countOfNonEmptyCells == 0;
	}

	// for water
	void fillLocalAreaInfo(const Vector3int16& centerCoord,
			const Water::RelevantNeighbors& relevantNeighbors,
			Water::LocalAreaInfo* out) const {
		return owner->fillLocalAreaInfo(centerCoord, relevantNeighbors, out);
	}
};

} }

