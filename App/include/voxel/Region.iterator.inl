#pragma once

//
// Implementation file for Region.iterator

namespace RBX { namespace Voxel {

namespace VoxelIteratorConstants {
	const Vector3int16 kFaceDirectionToLocationOffset[6] =
	{
		Vector3int16( 1, 0, 0),
		Vector3int16( 0, 0, 1),
		Vector3int16(-1, 0, 0),
		Vector3int16( 0, 0,-1),
		Vector3int16( 0, 1, 0),
		Vector3int16( 0,-1, 0),
	};
}

template<class InternalStorageType>
Region<InternalStorageType>::iterator::iterator(
		const Region<InternalStorageType>& owningRegion) :
	owningRegion(owningRegion),
	rangeSize((owningRegion.maxCoords - owningRegion.minCoords) + Vector3int16::one()),
	xCounter(0), zCounter(0), reachedEnd(false) {

	// read min and max coord from owning region to simplify constructor logic
	Vector3int16 minCoord = owningRegion.minCoords;
	Vector3int16 maxCoord = owningRegion.maxCoords;

	if (!owningRegion.isGuaranteedAllEmpty()) {
		// For speed, this implementation keeps a pointer to the current voxel.
		// In order to implement operator++, we want to keep a "carriage return"
		// pointer offset, for when the pointer needs to go from the end of
		// an x line to the beginning of the x line in the next z line, and
		// another offset for when the pointer needs to go from the end of
		// an x-z plane to the beginning of the plane in the next y level.

		// The "carriage return" offset for the end of an x line should be
		// zero in the degenerate case where the z dimension is 1. 

		// skip at end of x line: (minX,minY,minZ+1) - (maxX,minY,minZ)
		pointerSkipAtEndOfXLine = 0;
		if (rangeSize.z > 1) {
			pointerSkipAtEndOfXLine = 
				owningRegion.internalStorage->voxelCoordToArrayIndex(
					Vector3int16(minCoord.x, minCoord.y, minCoord.z + 1)) -
				owningRegion.internalStorage->voxelCoordToArrayIndex(
					Vector3int16(maxCoord.x, minCoord.y, minCoord.z));
		}

		// skip at end of x-z plane: (minX,minY+1,minZ) - (maxX,minY,maxZ)
		pointerSkipAtEndOfZLine = 0;
		if (rangeSize.y > 1) {
			pointerSkipAtEndOfZLine = 
				owningRegion.internalStorage->voxelCoordToArrayIndex(
					Vector3int16(minCoord.x, minCoord.y + 1, minCoord.z)) -
				owningRegion.internalStorage->voxelCoordToArrayIndex(
					Vector3int16(maxCoord.x, minCoord.y, maxCoord.z));
		}

		currentLocation = minCoord;
		currentIndex = owningRegion.internalStorage->voxelCoordToArrayIndex(currentLocation);
		currentCell = &owningRegion.internalStorage->getConstData()[currentIndex];
		reachedEnd = currentLocation.y > maxCoord.y;
	} else {
		reachedEnd = true;
	}
}

template<class InternalStorageType>
const Vector3int16& Region<InternalStorageType>::iterator::getCurrentLocation() const {
	return currentLocation;
}

template<class InternalStorageType>
const Cell& Region<InternalStorageType>::iterator::getCellAtCurrentLocation() const {
	return *currentCell;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::iterator::hasWaterAtCurrentLocation() const {
	return owningRegion.hasWaterAtSkipAllEmptyCheck(*currentCell, currentLocation);
}

template<class InternalStorageType>
CellMaterial Region<InternalStorageType>::iterator::getMaterialAtCurrentLocation() const {
	return (CellMaterial)readMaterial(
		&owningRegion.internalStorage->getConstMaterial()[0], currentIndex, *currentCell);
}

template<class InternalStorageType>
const Cell& Region<InternalStorageType>::iterator::getNeighborCell(
		FaceDirection direction) const {
	RBXASSERT_SLOW(owningRegion.contains(currentLocation + 
		kFaceDirectionToLocationOffset[direction]));
	return currentCell[
		InternalStorageType::kFaceDirectionToPointerOffset[direction]];
}

template<class InternalStorageType>
const Cell& Region<InternalStorageType>::iterator::getNeighborCell(
	FaceDirection direction1, FaceDirection direction2) const {
		RBXASSERT_SLOW(owningRegion.contains(currentLocation + 
			kFaceDirectionToLocationOffset[direction1] + kFaceDirectionToLocationOffset[direction2]));
		return currentCell[
			InternalStorageType::kFaceDirectionToPointerOffset[direction1] + InternalStorageType::kFaceDirectionToPointerOffset[direction2]];
}

template<class InternalStorageType>
CellMaterial Region<InternalStorageType>::iterator::getNeighborMaterial(
		FaceDirection direction) const {
	RBXASSERT_SLOW(owningRegion.contains(currentLocation + 
		kFaceDirectionToLocationOffset[direction]));
	const int offset(InternalStorageType::kFaceDirectionToPointerOffset[direction]);
	return (CellMaterial)readMaterial(&owningRegion.internalStorage->getConstMaterial()[0],
		currentIndex + offset, currentCell[offset]);
}

template<class InternalStorageType>
CellMaterial Region<InternalStorageType>::iterator::getNeighborMaterial(
	FaceDirection direction1, FaceDirection direction2) const {
		RBXASSERT_SLOW(owningRegion.contains(currentLocation + 
			kFaceDirectionToLocationOffset[direction1] + kFaceDirectionToLocationOffset[direction2));
		const int offset(InternalStorageType::kFaceDirectionToPointerOffset[direction1] + InternalStorageType::kFaceDirectionToPointerOffset[direction2]);
		return (CellMaterial)readMaterial(&owningRegion.internalStorage->getConstMaterial()[0],
			currentIndex + offset, currentCell[offset]);
}


template<class InternalStorageType>
const Cell& Region<InternalStorageType>::iterator::getArbitraryNeighborCell(
		const Vector3int16& neighborOffsets) const {
	RBXASSERT_SLOW(owningRegion.contains(currentLocation + neighborOffsets));
	return currentCell[
		InternalStorageType::voxelCoordOffsetToIndexOffset(neighborOffsets)];
}

template<class InternalStorageType>
bool Region<InternalStorageType>::iterator::hasWaterAtNeighbor(
		const FaceDirection& direction) const {
	RBXASSERT_SLOW(owningRegion.contains(currentLocation + 
		kFaceDirectionToLocationOffset[direction]));
	return owningRegion.hasWaterAtSkipAllEmptyCheck(
		currentCell[InternalStorageType::kFaceDirectionToPointerOffset[direction]],
		currentLocation +
			VoxelIteratorConstants::kFaceDirectionToLocationOffset[direction]);
}

template<class InternalStorageType>
typename Region<InternalStorageType>::iterator&
Region<InternalStorageType>::iterator::operator++() {
	++xCounter;
	if (xCounter == rangeSize.x) {
		xCounter = 0;
		++zCounter;
		if (zCounter == rangeSize.z) {
			zCounter = 0;
			currentLocation.x = owningRegion.minCoords.x;
			++currentLocation.y;
			currentLocation.z = owningRegion.minCoords.z;
			currentIndex += pointerSkipAtEndOfZLine;
			currentCell += pointerSkipAtEndOfZLine;
		} else {
			currentLocation.x = owningRegion.minCoords.x;
			++currentLocation.z;
			currentIndex += pointerSkipAtEndOfXLine;
			currentCell += pointerSkipAtEndOfXLine;
		}
	} else {
		++currentIndex;
		++currentCell;
		++currentLocation.x;
	}

	reachedEnd = currentLocation.y > owningRegion.maxCoords.y;
	return *this;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::iterator::operator==(const iterator& other) {
	if (reachedEnd || other.reachedEnd) {
		return reachedEnd == other.reachedEnd;
	}
	return owningRegion == other.owningRegion &&
		currentLocation == other.currentLocation;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::iterator::operator!=(const iterator& other) {
	return !(this->operator==(other));
}

} }
