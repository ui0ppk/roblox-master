#pragma once

#include "Voxel/Water.h"

/////////////////////////////////////////////////////
// template implementation file for Region.h

namespace RBX { namespace Voxel {

template<class InternalStorageType>
const Region<InternalStorageType> Region<InternalStorageType>::kEndRegion(NULL, Vector3int16::one(), Vector3int16::zero());

template<class InternalStorageType>
const typename Region<InternalStorageType>::iterator Region<InternalStorageType>::kEndIterator(Region<InternalStorageType>::kEndRegion);

template<class InternalStorageType>
const typename Region<InternalStorageType>::xline_iterator Region<InternalStorageType>::kEndXLineIterator(Region<InternalStorageType>::kEndRegion);

template<class InternalStorageType>
Region<InternalStorageType>::Region() :
	internalStorage(NULL), minCoords(Vector3int16::zero()), maxCoords(Vector3int16::zero()) {}

template<class InternalStorageType>
Region<InternalStorageType>::Region(const InternalStorageType* internalStorage,
		const Vector3int16& minCoords, const Vector3int16& maxCoords) :
	internalStorage(internalStorage), minCoords(minCoords), maxCoords(maxCoords) {}

template<class InternalStorageType>
bool Region<InternalStorageType>::isGuaranteedAllEmpty() const {
	return internalStorage == NULL;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::contains(const Vector3int16& globalCoord) const {
	return globalCoord.isBetweenInclusive(minCoords, maxCoords);
}

template<class InternalStorageType>
const Cell& Region<InternalStorageType>::voxelAt(
		const Vector3int16& globalCoord) const {
	RBXASSERT_SLOW(contains(globalCoord));

	if (isGuaranteedAllEmpty()) {
		return Constants::kUniqueEmptyCellRepresentation;
	} else {
		return voxelAtSkipAllEmptyCheck(globalCoord);
	}	
}

template<class InternalStorageType>
CellMaterial Region<InternalStorageType>::materialAt(
		const Vector3int16& globalCoord) const {
	RBXASSERT_SLOW(contains(globalCoord));

	if (isGuaranteedAllEmpty()) {
		return CELL_MATERIAL_Water;
	} else {
		unsigned int index = internalStorage->voxelCoordToArrayIndex(globalCoord);
		return readMaterial(&internalStorage->getConstMaterial()[0],
			index, internalStorage->getConstData()[index]);
	}
}

template<class InternalStorageType>
bool Region<InternalStorageType>::hasWaterAt(
		const Vector3int16& globalCoord) const {
	RBXASSERT_SLOW(contains(globalCoord));

	if (isGuaranteedAllEmpty()) {
		return false;
	} else {
		return hasWaterAtSkipAllEmptyCheck(
			voxelAtSkipAllEmptyCheck(globalCoord), globalCoord);
	}
}

template<class InternalStorageType>
typename Region<InternalStorageType>::iterator
Region<InternalStorageType>::begin() const {
	return iterator(*this);
}

template<class InternalStorageType>
const typename Region<InternalStorageType>::iterator&
Region<InternalStorageType>::end() const {
	return kEndIterator;
}

template<class InternalStorageType>
typename Region<InternalStorageType>::xline_iterator
Region<InternalStorageType>::xLineBegin() const {
	return xline_iterator(*this);
}

template<class InternalStorageType>
const typename Region<InternalStorageType>::xline_iterator&
Region<InternalStorageType>::xLineEnd() const {
	return kEndXLineIterator;
}

template<class InternalStorageType>
Region<InternalStorageType>& Region<InternalStorageType>::operator=(
		const Region<InternalStorageType>& other) {
	internalStorage = other.internalStorage;
	minCoords = other.minCoords;
	maxCoords = other.maxCoords;
	return *this;
}

template<class InternalStorageType>
bool Region<InternalStorageType>::operator==(
		const Region<InternalStorageType>& other) const {
	return internalStorage == other.internalStorage &&
		minCoords == other.minCoords &&
		maxCoords == other.maxCoords;
}

template<class InternalStorageType>
const Cell& Region<InternalStorageType>::voxelAtSkipAllEmptyCheck(
		const Vector3int16& globalCoord) const {
	RBXASSERT_SLOW(!isGuaranteedAllEmpty());
	return internalStorage->getConstData()[
			internalStorage->voxelCoordToArrayIndex(globalCoord)];
}

template<class InternalStorageType>
bool Region<InternalStorageType>::hasWaterAtSkipAllEmptyCheck(
		const Cell& cell,
		const Vector3int16& globalCoord) const {
	RBXASSERT_SLOW(!isGuaranteedAllEmpty());
	return Water::cellHasWater(internalStorage, cell, globalCoord);
}

} }
