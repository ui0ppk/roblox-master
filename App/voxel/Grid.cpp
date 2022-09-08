#include "stdafx.h"

#include "Voxel/Grid.h"

#include "rbx/Debug.h"
#include "Util/G3DCore.h"
#include "Voxel/Cell.h"
#include "Voxel/Util.h"

#include <boost/unordered_map.hpp>

namespace RBX { namespace Voxel {

Grid::Grid() : chunkMap(), countOfNonEmptyCells(0) {}

const Cell& Grid::getVoxelLikelyThisChunk(const SpatialRegion::Id& id,
		const Chunk& chunk, const Vector3int16& coord) const {
	const Chunk* chunkPtr;
	SpatialRegion::Id coordChunkId = SpatialRegion::regionContainingVoxel(coord);
	if (coordChunkId == id) {
		chunkPtr = &chunk;
	} else {
		chunkPtr = chunkMap.find(coordChunkId);
		if (chunkPtr == NULL) {
			return Constants::kUniqueEmptyCellRepresentation;
		}
	}
	return chunkPtr->getConstData()[Chunk::voxelCoordToArrayIndex(coord)];
}

void Grid::fillLocalAreaInfo(const Vector3int16& globalCoord,
		const Water::RelevantNeighbors& relevantNeighbors,
		Water::LocalAreaInfo* info) const {

		
	SpatialRegion::Id mainChunkId = SpatialRegion::regionContainingVoxel(globalCoord);
	const Chunk* mainChunkPtr = chunkMap.find(mainChunkId);
	if (mainChunkPtr == NULL) {
		RBXASSERT(false);
		return;
	}
	
	const Chunk& mainChunk = *mainChunkPtr;
	const Vector3int16 relativeCoord = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(globalCoord);

	if (relativeCoord.isBetweenInclusive(Vector3int16::one(),
			SpatialRegion::getMaxVoxelOffsetInsideRegion() - Vector3int16::one())) {
		unsigned int index = Chunk::voxelCoordToArrayIndex(globalCoord);
		info->aboveNeighbor = 
			mainChunk.getConstData()[index +
			Chunk::voxelCoordOffsetToIndexOffset(relevantNeighbors.aboveNeighbor)];
		info->primaryNeighbor = 
			mainChunk.getConstData()[index +
			Chunk::voxelCoordOffsetToIndexOffset(relevantNeighbors.primaryNeighbor)];
		info->secondaryNeighbor = 
			mainChunk.getConstData()[index +
			Chunk::voxelCoordOffsetToIndexOffset(relevantNeighbors.secondaryNeighbor)];
		info->diagonalNeighbor = 
			mainChunk.getConstData()[index +
			Chunk::voxelCoordOffsetToIndexOffset(relevantNeighbors.diagonalNeighbor)];
		info->diagonalUpNeighbor = 
			mainChunk.getConstData()[index +
			Chunk::voxelCoordOffsetToIndexOffset(relevantNeighbors.diagonalUpNeighbor)];
	} else {

		info->aboveNeighbor = getVoxelLikelyThisChunk(mainChunkId, mainChunk,
			globalCoord + relevantNeighbors.aboveNeighbor);
		info->primaryNeighbor = getVoxelLikelyThisChunk(mainChunkId, mainChunk,
			globalCoord + relevantNeighbors.primaryNeighbor);
		info->secondaryNeighbor = getVoxelLikelyThisChunk(mainChunkId, mainChunk,
			globalCoord + relevantNeighbors.secondaryNeighbor);
		info->diagonalNeighbor = getVoxelLikelyThisChunk(mainChunkId, mainChunk,
			globalCoord + relevantNeighbors.diagonalNeighbor);
		info->diagonalUpNeighbor = getVoxelLikelyThisChunk(mainChunkId, mainChunk,
			globalCoord + relevantNeighbors.diagonalUpNeighbor);
	}
}

void Grid::setCell(const Vector3int16& location, Cell newCell, CellMaterial inputMaterial)
{
    if (!Voxel::getTerrainExtentsInCells().contains(location))
        return;

	const SpatialRegion::Id chunkId = SpatialRegion::regionContainingVoxel(location);
	const unsigned int arrayIndex = Chunk::voxelCoordToArrayIndex(location);

	Cell prevCell = Constants::kUniqueEmptyCellRepresentation;
	CellMaterial prevMaterial = CELL_MATERIAL_Water;

	Chunk* existingChunk = chunkMap.find(chunkId);
    
    if (existingChunk) {
		prevCell = existingChunk->getConstData()[arrayIndex];
		prevMaterial = readMaterial(&(existingChunk->getConstMaterial()[0]),
			arrayIndex, prevCell);
	}

	CellMaterial newMaterial = inputMaterial == CELL_MATERIAL_Unspecified ?
		prevMaterial : inputMaterial;

	bool changed = prevCell != newCell || prevMaterial != newMaterial;

	if (changed) {
		// find or create Chunk
		Chunk* updatingChunk = existingChunk;
        
        if (!updatingChunk)
        {
            updatingChunk = &chunkMap.insert(chunkId);
            updatingChunk->init(this);
        }

		bool hadWaterBefore = Water::cellHasWater(updatingChunk, prevCell, location);
		
		updatingChunk->getData()[arrayIndex] = newCell;
		writeMaterial(&(updatingChunk->getMaterial()[0]), arrayIndex, newMaterial);

		bool hasWaterAfter = Water::cellHasWater(updatingChunk, newCell, location);

		int nonEmptyDelta = prevCell.isEmpty() - newCell.isEmpty();

		updatingChunk->updateCountOfNonEmptyCells(nonEmptyDelta);
		countOfNonEmptyCells += nonEmptyDelta;

		if (updatingChunk->hasNoUsefulData()) {
			chunkMap.erase(chunkId);
		}

		CellChangeInfo info(location, prevCell, newCell, hadWaterBefore, hasWaterAfter, newMaterial);

        for (unsigned int i = 0; i < cellChangeListeners.size(); ++i) {
            cellChangeListeners[i]->terrainCellChanged(info);
        }
	}
}

Grid::Region Grid::getRegion(
		const Vector3int16& minCoord, const Vector3int16& maxCoord) const {
	SpatialRegion::Id minId = SpatialRegion::regionContainingVoxel(minCoord);
	RBXASSERT(minId == SpatialRegion::regionContainingVoxel(maxCoord));

	return Region(chunkMap.find(minId), minCoord, maxCoord);
}

Cell Grid::getCell(const Vector3int16& pos) const {
	SpatialRegion::Id chunkId = SpatialRegion::regionContainingVoxel(pos);

	if (const Chunk* chunk = chunkMap.find(chunkId)) {
		return chunk->getConstData()[Chunk::voxelCoordToArrayIndex(pos)];
	}

	return Constants::kUniqueEmptyCellRepresentation;
}

CellMaterial Grid::getCellMaterial(const Vector3int16& pos) const {
	SpatialRegion::Id chunkId = SpatialRegion::regionContainingVoxel(pos);
	
	if (const Chunk* chunk = chunkMap.find(chunkId)) {
		const unsigned int index = Chunk::voxelCoordToArrayIndex(pos);
		return readMaterial(&chunk->getConstMaterial()[0], index,
			chunk->getConstData()[index]);
	}

	return CELL_MATERIAL_Water;
}

Cell Grid::getWaterCell(const Vector3int16& pos) const {
	SpatialRegion::Id chunkId(SpatialRegion::regionContainingVoxel(pos));

	if (const Chunk* chunk = chunkMap.find(chunkId)) {
		unsigned int arrayIndex = Chunk::voxelCoordToArrayIndex(pos);
		return Water::interpretAsWaterCell(chunk, chunk->getConstData()[arrayIndex], pos);
	}

	return Constants::kUniqueEmptyCellRepresentation;
}

void Grid::connectListener(CellChangeListener* listener) {
    if (std::find(cellChangeListeners.begin(), cellChangeListeners.end(), listener) == cellChangeListeners.end()) {
        cellChangeListeners.push_back(listener);
    } else {
        RBXASSERT(false);
    }
}

void Grid::disconnectListener(CellChangeListener* listener) {
    std::vector<Voxel::CellChangeListener*>::iterator itr =
        std::find(cellChangeListeners.begin(), cellChangeListeners.end(), listener);
    if (itr != cellChangeListeners.end()) {
        cellChangeListeners.erase(itr);	
    } else {
        RBXASSERT(false);
    }
}

std::vector<SpatialRegion::Id> Grid::getNonEmptyChunks() const
{
    return chunkMap.getChunks();
}

bool Grid::isAllocated() const {
	return countOfNonEmptyCells > 0;
}

std::vector<SpatialRegion::Id> Grid::getNonEmptyChunksInRegion(const Region3int16& extents) const
{
    if (extents.empty())
        return std::vector<SpatialRegion::Id>();

    SpatialRegion::Id minRegion(SpatialRegion::regionContainingVoxel(extents.getMinPos()));
    SpatialRegion::Id maxRegion(SpatialRegion::regionContainingVoxel(extents.getMaxPos()));

    unsigned int totalRegionCount =
        (maxRegion.value().x - minRegion.value().x + 1) * 
        (maxRegion.value().y - minRegion.value().y + 1) * 
        (maxRegion.value().z - minRegion.value().z + 1);

    // chunkMap.find() is more expensive than isBetweenInclusive
    if (totalRegionCount < chunkMap.size() * 2)
    {
        // We're querying a relatively small area, let's just iterate through all regions
        std::vector<SpatialRegion::Id> result;

        for (int ry = minRegion.value().y; ry <= maxRegion.value().y; ++ry)
            for (int rz = minRegion.value().z; rz <= maxRegion.value().z; ++rz)
                for (int rx = minRegion.value().x; rx <= maxRegion.value().x; ++rx)
                {
                    SpatialRegion::Id id(rx, ry, rz);

                    if (chunkMap.find(id))
                        result.push_back(id);
                }

        return result;
    }
    else
    {
        // We're querying a relatively large area, let's scan through filled regions inside the grid
        std::vector<SpatialRegion::Id> chunks = chunkMap.getChunks();

        std::vector<SpatialRegion::Id> result;

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            SpatialRegion::Id id = chunks[i];

            if (id.value().isBetweenInclusive(minRegion.value(), maxRegion.value()))
                result.push_back(id);
        }

        return result;
    }
}

} }
