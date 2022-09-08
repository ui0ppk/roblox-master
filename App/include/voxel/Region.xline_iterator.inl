#pragma once

namespace RBX { namespace Voxel {

template<class InternalStorageType>
Region<InternalStorageType>::xline_iterator::xline_iterator(
		const Region<InternalStorageType>& owningRegion) :
	owningRegion(owningRegion),
	lineSize(owningRegion.maxCoords.x - owningRegion.minCoords.x + 1),
	minZ(owningRegion.minCoords.z),
	zDimSize(owningRegion.maxCoords.z - owningRegion.minCoords.z + 1),
	maxY(owningRegion.maxCoords.y) {

	zDimCounter = 0;

	if (!owningRegion.isGuaranteedAllEmpty()) {
		currentLocation = owningRegion.minCoords;

		pointerSkipAtEndOfXLine = InternalStorageType::voxelCoordOffsetToIndexOffset(
			Vector3int16(0, 0, 1));
		pointerSkipAtEndOfZLine = InternalStorageType::voxelCoordOffsetToIndexOffset(
			Vector3int16(0, 1, owningRegion.minCoords.z - owningRegion.maxCoords.z));

		currentIndex = owningRegion.internalStorage->voxelCoordToArrayIndex(currentLocation);
		currentCell = &owningRegion.internalStorage->getConstData()[currentIndex];
		reachedEnd = currentLocation.y > owningRegion.maxCoords.y;

		// index needs to be even for half byte material alignment reasons
		RBXASSERT((currentIndex & 0x1) == 0);
	} else {
		reachedEnd = true;
	}
}

template<class InternalStorageType>
const Vector3int16& Region<InternalStorageType>::xline_iterator::getCurrentLocation() const {
	return currentLocation;
}

template<class InternalStorageType>
unsigned int Region<InternalStorageType>::xline_iterator::getLineSize() const {
	return lineSize;
}

template<class InternalStorageType>
const Cell* Region<InternalStorageType>::xline_iterator::getLineCells() const {
	return currentCell;
}

template<class InternalStorageType>
const unsigned char* Region<InternalStorageType>::xline_iterator::getLineMaterials() const {
	return &owningRegion.internalStorage->getConstMaterial()[currentIndex / 2];
}

template<class InternalStorageType>
bool Region<InternalStorageType>::xline_iterator::operator==(
		const xline_iterator& other) const {
	if (reachedEnd || other.reachedEnd) {
		return reachedEnd == other.reachedEnd;
	}
	return owningRegion == other.owningRegion &&
		currentLocation == other.currentLocation;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::xline_iterator::operator!=(
		const xline_iterator& other) const {
	return !(*this == other);
}

template<class InternalStorageType>
typename Region<InternalStorageType>::xline_iterator&
Region<InternalStorageType>::xline_iterator::operator++() {

	++currentLocation.z;
	++zDimCounter;
	if (zDimCounter >= zDimSize) {
		currentLocation.z = minZ;
		zDimCounter = 0;
		++currentLocation.y;
		currentIndex += pointerSkipAtEndOfZLine;
		currentCell += pointerSkipAtEndOfZLine;
	} else {
		currentIndex += pointerSkipAtEndOfXLine;
		currentCell += pointerSkipAtEndOfXLine;
	}

	// index needs to be even for half byte material alignment reasons
	RBXASSERT((currentIndex & 0x1) == 0);

	reachedEnd = currentLocation.y > maxY;
	return *this;
}


} }
