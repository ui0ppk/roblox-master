#pragma once

#include "Util/G3DCore.h"
#include "Util/SpatialRegion.h"
#include "Voxel/CellChangeListener.h"
#include "Voxel/ChunkMap.h"
#include "Voxel/Region.h"
#include "Voxel/Water.h"

#include <boost/unordered_map.hpp>
#include <vector>

namespace RBX { namespace Voxel {

// Storage class for terrain Voxels. Has methods for reading and writing
// voxels. The voxels for different areas of the terrain may be stored 
// internally in separate sub-containers. Frequently allocates and re-
// allocates memory, and does not take any data model locks, so do not store
// VoxelRegions for later use (e.g. storing for a later job run).
class Grid {
	class Chunk;
	
	typedef ChunkMap<Chunk> ChunkMapType;
	ChunkMapType chunkMap;
	unsigned int countOfNonEmptyCells;

	// Whenever a cell is changed, the cellChangedSignal will be notified.
	// Note that this doesn't necessarily happen every time setCell is called:
	// if setCell would set a cell to be the same value it currently it has,
	// then the cellChangedSignal won't fire for that setCell call.
	std::vector<CellChangeListener*> cellChangeListeners;

	const Cell& getVoxelLikelyThisChunk(const SpatialRegion::Id& id,
		const Chunk& chunk, const Vector3int16& coord) const;

	void fillLocalAreaInfo(const Vector3int16& globalCoord,
		const Water::RelevantNeighbors& neighbors,
		Water::LocalAreaInfo* out) const;

public:
	typedef RBX::Voxel::Region<Chunk> Region;

	Grid();

	// returns the number of cells in the terrain that are not empty
	inline unsigned int getNonEmptyCellCount() const { return countOfNonEmptyCells; }

	// subscribe and unsubscribe from cell change events
	void connectListener(CellChangeListener* listener);
	void disconnectListener(CellChangeListener* listener);

	// Updates one cell. Will notify listeners of the cellChanged signal if the
	// targeted cell is actually altered (new values != old values) after the
	// new value is put in place.
	void setCell(const Vector3int16& location, Cell newCell,
		CellMaterial newMaterial);

	// Get a Cell region covering the extents specified. Does not support
	// extents that span SpatialRegion boundaries.
	Region getRegion(const Vector3int16& extent1, const Vector3int16& extent2) const;

    // Gets information about one cell
	Cell getCell(const Vector3int16& pos) const;
	CellMaterial getCellMaterial(const Vector3int16& pos) const;
	Cell getWaterCell(const Vector3int16& pos) const;

    // Gets live chunk ids
    std::vector<SpatialRegion::Id> getNonEmptyChunks() const;
    std::vector<SpatialRegion::Id> getNonEmptyChunksInRegion(const Region3int16& extents) const;

	bool isAllocated() const;
};

} }

#include "Voxel/Grid.Chunk.h"
