#include "stdafx.h"

#include "Util/SpatialRegion.h"

#include "Voxel/Cell.h"

namespace RBX {

namespace SpatialRegion {

std::size_t hash_value(const Id& key)
{
	Id::boost_compatible_hash_value hash;
	return hash(key);
}

namespace _PrivateConstants {
	const Vector3int16 kRegionDimensionInStudsAsBitShifts(
		getRegionDimensionInVoxelsAsBitShifts() +
		Vector3int16(Voxel::kCELL_SIZE_AS_BIT_SHIFT,
			Voxel::kCELL_SIZE_AS_BIT_SHIFT,
			Voxel::kCELL_SIZE_AS_BIT_SHIFT));

}

}

}
