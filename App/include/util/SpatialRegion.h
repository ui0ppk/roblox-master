#pragma once

#include "Util/G3DCore.h"
#include "Util/Region3int16.h"
#include "Util/Vector3int32.h"

namespace RBX {

// Utilities for working with sub-sections of global coordinate space called
// SpacialRegions. The sub-sections are of a fixed, globally defined size and
// position. The sub-sections are integer aligned and origin aligned (the
// origin is at the meeting point of 8 regions).
namespace SpatialRegion {

namespace Constants {
	// these are const int to allow proper constant folding, inlining, and
	// to allow other, derived static const int values to also have these
	// benefits.
	const int kRegionXDimensionInVoxelsAsBitShift = 5;
	const int kRegionYDimensionInVoxelsAsBitShift = 4;
	const int kRegionZDimensionInVoxelsAsBitShift = 5;

	const int kRegionInVoxelsAsBitShift = kRegionXDimensionInVoxelsAsBitShift + kRegionYDimensionInVoxelsAsBitShift + kRegionZDimensionInVoxelsAsBitShift;

	const int kRegionXDimensionInVoxels = 1 << Constants::kRegionXDimensionInVoxelsAsBitShift;
	const int kRegionYDimensionInVoxels = 1 << Constants::kRegionYDimensionInVoxelsAsBitShift;
	const int kRegionZDimensionInVoxels = 1 << Constants::kRegionZDimensionInVoxelsAsBitShift;

	const int kMaxXVoxelOffsetInsideRegion = kRegionXDimensionInVoxels - 1;
	const int kMaxYVoxelOffsetInsideRegion = kRegionYDimensionInVoxels - 1;
	const int kMaxZVoxelOffsetInsideRegion = kRegionZDimensionInVoxels - 1;
}

namespace _PrivateConstants {
	extern const Vector3int16 kRegionDimensionInStudsAsBitShifts;
}

// Identifier object for regions of space. Has a Vector3int16 representation
// also. The Vector3int16 representation has constraints:
// * If two Ids represent the same area of space, then the Vector3int16's are ==
// * If one Vector3int16's values (x, y, and/or z) is > another, then the
//   spacial coordinates on those axis(es) are also >.
// * If two regions are adjacent, then the Vector3int16 values will be
//   sequential on the axis(es) they are adjacent in.
// * The Vector3int16 representation is consitent between runs, and between
//   client/server.
class Id {
	Vector3int16 internalValue;
public:
	explicit Id(const Vector3int16& internalValue) : internalValue(internalValue) {}
	Id(int x, int y, int z) : internalValue(Vector3int16(x,y,z)) {}

	const Vector3int16& value() const { return internalValue; }

	Id operator+(const Vector3int16& other) const { return Id(internalValue + other); }
	bool operator==(const Id& other) const { return internalValue == other.internalValue; }
	bool operator!=(const Id& other) const { return internalValue != other.internalValue; }

	struct boost_compatible_hash_value {
		size_t operator()(const Id& key) const {
			return Vector3int16::boost_compatible_hash_value()(key.internalValue);
		}
	};
};

std::size_t hash_value(const Id& key);

inline Vector3int16 getRegionDimensionsInVoxels() {
	using namespace Constants;
	return Vector3int16(
		kRegionXDimensionInVoxels,
		kRegionYDimensionInVoxels,
		kRegionZDimensionInVoxels);
}

inline Vector3int16 getRegionDimensionInVoxelsAsBitShifts() {
	using namespace Constants;
	return Vector3int16(
		kRegionXDimensionInVoxelsAsBitShift,
		kRegionYDimensionInVoxelsAsBitShift,
		kRegionZDimensionInVoxelsAsBitShift);
}

inline Vector3int16 getMaxVoxelOffsetInsideRegion() {
	using namespace Constants;
	return Vector3int16(
		kMaxXVoxelOffsetInsideRegion,
		kMaxYVoxelOffsetInsideRegion,
		kMaxZVoxelOffsetInsideRegion);
}

inline Id regionContainingVoxel(const Vector3int16& globalVoxelCoordinate) {
	return Id(globalVoxelCoordinate >> getRegionDimensionInVoxelsAsBitShifts());
}

inline Vector3int16 voxelCoordinateRelativeToEnclosingRegion(
		const Vector3int16& globalVoxelCoordinate) {
	return globalVoxelCoordinate & getMaxVoxelOffsetInsideRegion();
}

inline Vector3int16 globalVoxelCoordinateFromRegionAndRelativeCoordinate(
		const Id& id, const Vector3int16& relativeCoordinate) {
	return (id.value() << getRegionDimensionInVoxelsAsBitShifts()) + relativeCoordinate;
}

inline Region3int16 inclusiveVoxelExtentsOfRegion(const Id& id) {
	Vector3int16 min(id.value() << getRegionDimensionInVoxelsAsBitShifts());
	return Region3int16(min, min + getMaxVoxelOffsetInsideRegion());
}

inline Vector3int32 smallestCornerOfRegionInGlobalCoordStuds(const Id& id) {
    return Vector3int32(id.value()) << _PrivateConstants::kRegionDimensionInStudsAsBitShifts;
}

inline Vector3int32 centerOfRegionInGlobalCoordStuds(const Id& id) {
	static const Vector3int32 kHalfDimensionInStuds(Vector3int32::one() <<
		(_PrivateConstants::kRegionDimensionInStudsAsBitShifts - Vector3int16::one()));
	return smallestCornerOfRegionInGlobalCoordStuds(id) +
		kHalfDimensionInStuds;
}

inline Vector3int32 largestCornerOfRegionInGlobalCoordStuds(const Id& id) {
	return smallestCornerOfRegionInGlobalCoordStuds(Id(id.value() + Vector3int16::one()));
}

} // SpacialRegion

} // RBX
