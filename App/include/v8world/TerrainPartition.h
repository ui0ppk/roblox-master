#pragma once
/* Copyright 2003-2011 ROBLOX Corporation, All Rights Reserved */

#include <vector>
#include "Util/G3DCore.h"
#include "V8World/Primitive.h"
#include "Voxel/Util.h"
#include "Voxel/CellChangeListener.h"
#include "Voxel/ChunkMap.h"
#include "Voxel2/GridListener.h"

namespace RBX {
namespace Voxel {
	class Grid;
}

namespace Voxel2 {
	class Grid;
}

class TerrainPartitionMega:
    public Voxel::CellChangeListener
{
public:
    TerrainPartitionMega(Voxel::Grid* voxelGrid);
    ~TerrainPartitionMega();

    void findCellsTouchingExtents(const Extents& extents, std::vector<Vector3int16>* found) const;

private:
    struct ChunkData
    {
        // filled[y][z][x] represents a sub-chunk of 4x2x4 cells, where 1 cell = 1 bit
        unsigned int count;
        unsigned int filled[Voxel::kY_CHUNK_SIZE / 2][Voxel::kXZ_CHUNK_SIZE / 4][Voxel::kXZ_CHUNK_SIZE / 4];

        ChunkData(): count(0)
        {
            memset(filled, 0, sizeof(filled));
        }
    };

    Voxel::ChunkMap<ChunkData> chunks;
	Voxel::Grid* voxelGrid;

	/*override*/ virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);

    void findCellsInRegion(const SpatialRegion::Id& region, const ChunkData& chunk, const Vector3int16& minOffset, const Vector3int16& maxOffset, std::vector<Vector3int16>* found) const;
};

class TerrainPartitionSmooth
{
public:
    static const int kChunkSizeLog2 = 3;
	static const int kChunkSize = 1 << kChunkSizeLog2;

    TerrainPartitionSmooth(Voxel2::Grid* grid);
    ~TerrainPartitionSmooth();

    struct ChunkResult
	{
		Vector3int32 id;
        bool touchesSolid;
        bool touchesWater;
	};

	void findChunksTouchingExtents(const Extents& extents, std::vector<ChunkResult>* found) const;

    void updateChunk(const Vector3int32& id);

private:
    struct ChunkSlice
	{
        // 8x8 bits for each slice
        uint64_t solid;
        uint64_t water;
	};

    struct ChunkData
    {
		ChunkSlice slices[kChunkSize];

        ChunkData()
        {
            memset(slices, 0, sizeof(slices));
        }
    };

	typedef boost::unordered_map<Vector3int32, ChunkData> ChunkMap;
	
	ChunkMap chunks;
	Voxel2::Grid* grid;

	uint64_t masksHor[kChunkSize][kChunkSize];
	uint64_t masksVer[kChunkSize][kChunkSize];

	void fillChunkIfTouchingExtents(const Vector3int32& chunkId, const ChunkData& chunkData, const Vector3int32& minPos, const Vector3int32& maxPos, std::vector<ChunkResult>* found) const;
};

} // namespace

