#pragma once

namespace RBX { namespace Voxel {

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
bool AreaCopy<XDim, YDim, ZDim>::Chunk::contains(const Vector3int16& cellLocation) const {
	return cellLocation.isBetweenInclusive(firstCellLocation,
		firstCellLocation + Vector3int16(XDim, YDim, ZDim) - Vector3int16::one());
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
void AreaCopy<XDim, YDim, ZDim>::Chunk::fillEmpty(
		const Vector3int16& minLoc, const Vector3int16& maxLoc) {

	unsigned int xWidth = maxLoc.x - minLoc.x + 1;
	RBXASSERT((xWidth & 0x1) == 0);

	Vector3int16 counter;
	for (counter.y = minLoc.y; counter.y <= maxLoc.y; ++counter.y) {
	for (counter.z = minLoc.z; counter.z <= maxLoc.z; ++counter.z) {
		counter.x = minLoc.x;
		unsigned int index = voxelCoordToArrayIndex(counter);
		memset(&cells[index],
			Cell::convertToUnsignedCharForFile(Constants::kUniqueEmptyCellRepresentation),
			xWidth * sizeof(Cell));
		// technically material doesn't need to be set for empty cells
		memset(&materials[index / 2], 0xff, xWidth / 2);
	}
	}
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
template<class RegionType>
void AreaCopy<XDim, YDim, ZDim>::Chunk::fillFromRegion(const RegionType& region) {
	for (typename RegionType::xline_iterator itr = region.xLineBegin();
			itr != region.xLineEnd(); ++itr) {
		const size_t lineSize = itr.getLineSize();
		RBXASSERT(contains(itr.getCurrentLocation()));
		RBXASSERT(contains(itr.getCurrentLocation() + Vector3int16(lineSize - 1, 0, 0)));

		unsigned int index = voxelCoordToArrayIndex(itr.getCurrentLocation());
		if (lineSize == 32) {
			memcpy(&cells[index], itr.getLineCells(), 32 * sizeof(Cell));
			memcpy(&materials[index/2], itr.getLineMaterials(), 32 / 2);
		} else {
			const Cell* cellSrc = itr.getLineCells();
			const unsigned char* materialSrc = itr.getLineMaterials();
			for (size_t i = 0; i < lineSize; ++i) {
				cells[index + i] = cellSrc[i];
				materials[(index + i) / 2] = materialSrc[i / 2];
			}
		}
	}
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
int AreaCopy<XDim, YDim, ZDim>::Chunk::kFaceDirectionToPointerOffset[7] = {
	1,
	XDim,
	-1,
	-((int)XDim),
	XDim * ZDim,
	-(int)(XDim * ZDim),
	0
};

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
int AreaCopy<XDim, YDim, ZDim>::Chunk::voxelCoordOffsetToIndexOffset(
		const Vector3int16& localCoord) {
	return localCoord.x + (XDim * localCoord.z) + (XDim * ZDim * localCoord.y);
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
int AreaCopy<XDim, YDim, ZDim>::Chunk::voxelCoordToArrayIndex(
		const Vector3int16& globalCoord) const {
	return voxelCoordOffsetToIndexOffset(globalCoord - firstCellLocation);
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
const std::vector<Cell>& AreaCopy<XDim, YDim, ZDim>::Chunk::getConstData() const {
	return cells;
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
const std::vector<unsigned char>& AreaCopy<XDim, YDim, ZDim>::Chunk::getConstMaterial() const {
	return materials;
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
void AreaCopy<XDim, YDim, ZDim>::Chunk::fillLocalAreaInfo(
		const Vector3int16& globalCoord,
		const Water::RelevantNeighbors& relevantNeighbors,
		Water::LocalAreaInfo* out) const {

	RBXASSERT(contains(globalCoord));
	RBXASSERT(contains(globalCoord + relevantNeighbors.aboveNeighbor));
	RBXASSERT(contains(globalCoord + relevantNeighbors.primaryNeighbor));
	RBXASSERT(contains(globalCoord + relevantNeighbors.secondaryNeighbor));
	RBXASSERT(contains(globalCoord + relevantNeighbors.diagonalNeighbor));
	RBXASSERT(contains(globalCoord + relevantNeighbors.diagonalUpNeighbor));

	unsigned int centerIndex = voxelCoordToArrayIndex(globalCoord);

	out->aboveNeighbor =      cells[centerIndex +
		voxelCoordOffsetToIndexOffset(relevantNeighbors.aboveNeighbor)];
	out->primaryNeighbor =    cells[centerIndex +
		voxelCoordOffsetToIndexOffset(relevantNeighbors.primaryNeighbor)];
	out->secondaryNeighbor =  cells[centerIndex +
		voxelCoordOffsetToIndexOffset(relevantNeighbors.secondaryNeighbor)];
	out->diagonalNeighbor =   cells[centerIndex +
		voxelCoordOffsetToIndexOffset(relevantNeighbors.diagonalNeighbor)];
	out->diagonalUpNeighbor = cells[centerIndex +
		voxelCoordOffsetToIndexOffset(relevantNeighbors.diagonalUpNeighbor)];
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
template<class Source>
void AreaCopy<XDim, YDim, ZDim>::Chunk::loadData(const Source* source,
		const Vector3int16& rootCell) {

	if (cells.empty()) {
		std::vector<Cell> cellsSwap(kSize, Constants::kUniqueEmptyCellRepresentation);
		std::vector<unsigned char> materialsSwap((kSize + 1) / 2, 0xff);
		cells.swap(cellsSwap);
		materials.swap(materialsSwap);	
	}

	firstCellLocation = rootCell;

	const Vector3int16 maxCell = rootCell +
		Vector3int16(XDim, YDim, ZDim) - Vector3int16::one();

	const SpatialRegion::Id minRegion =
		SpatialRegion::regionContainingVoxel(rootCell);
	const SpatialRegion::Id maxRegion =
		SpatialRegion::regionContainingVoxel(maxCell);

	isAllEmpty = true;
	Vector3int16 regionIdCounter;
	for (regionIdCounter.y = minRegion.value().y; regionIdCounter.y <= maxRegion.value().y; ++regionIdCounter.y) {
	for (regionIdCounter.z = minRegion.value().z; regionIdCounter.z <= maxRegion.value().z; ++regionIdCounter.z) {
	for (regionIdCounter.x = minRegion.value().x; regionIdCounter.x <= maxRegion.value().x; ++regionIdCounter.x) {
		SpatialRegion::Id id(regionIdCounter);
		Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(id);
		const Vector3int16 queryMin = extents.getMinPos().max(rootCell);
		const Vector3int16 queryMax = extents.getMaxPos().min(maxCell);

		// for material alignment issues, all x segments must be even, and
		// start from an even location in the region
		RBXASSERT(((queryMax.x - queryMin.x + 1) & 0x1) == 0);
		RBXASSERT(((queryMin.x - firstCellLocation.x) & 0x1) == 0);

		typename Source::Region region = source->getRegion(queryMin, queryMax);
		if (region.isGuaranteedAllEmpty()) {
			fillEmpty(queryMin, queryMax);
		} else {
			isAllEmpty = false;
			fillFromRegion(region);
		}
	}
	}
	}
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
bool AreaCopy<XDim, YDim, ZDim>::Chunk::getIsAllEmpty() const {
	return isAllEmpty;
}


template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
typename AreaCopy<XDim, YDim, ZDim>::Region AreaCopy<XDim, YDim, ZDim>::getRegion(
		const Vector3int16& minCoords, const Vector3int16& maxCoords) const {
	return Region(storage.getIsAllEmpty() ? NULL : &storage, minCoords, maxCoords);
}

template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
template<class Source>
void AreaCopy<XDim, YDim, ZDim>::loadData(const Source* source,
		const Vector3int16& rootCell) {
	storage.loadData(source, rootCell);
}

} }
