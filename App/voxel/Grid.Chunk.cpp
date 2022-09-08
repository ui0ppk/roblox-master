#include "stdafx.h"

#include "Voxel/Grid.h"

#include "Voxel/Cell.h"

#include "rbx/Profiler.h"

namespace RBX { namespace Voxel {

const int Grid::Chunk::kFaceDirectionToPointerOffset[7] = {
    kXOffsetMultiplier,
	kZOffsetMultiplier,
	-kXOffsetMultiplier,
	-kZOffsetMultiplier,
	kYOffsetMultiplier,
	-kYOffsetMultiplier,
	0
};


Grid::Chunk::Chunk() :
	data(),
	material(),
	countOfNonEmptyCells(0),
	initialized(false)
{
}
 
Grid::Chunk::~Chunk()
{
    RBXPROFILER_COUNTER_SUB("memory/terrain/legacy", data.size() + material.size());
}

void Grid::Chunk::init(const Grid* owner) {
	using namespace SpatialRegion::Constants;

	if (!initialized) {
		this->owner = owner;

		static const int kTotalMemorySize = 
			kRegionXDimensionInVoxels *
			kRegionYDimensionInVoxels *
			kRegionZDimensionInVoxels;

		std::vector<Cell> tmp1(kTotalMemorySize, Constants::kUniqueEmptyCellRepresentation);
		data.swap(tmp1);
		std::vector<unsigned char> tmp2(kTotalMemorySize / 2, 0xff);
		material.swap(tmp2);
		initialized = true;

        RBXPROFILER_COUNTER_ADD("memory/terrain/legacy", data.size() + material.size());
	}
}

} }
