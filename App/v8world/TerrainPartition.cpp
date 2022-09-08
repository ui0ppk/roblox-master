/* Copyright 2003-2011 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/TerrainPartition.h"

#include "V8DataModel/MegaCluster.h"
#include "V8World/Primitive.h"
#include "Voxel/Grid.h"

#include "voxel2/Grid.h"

LOGGROUP(TerrainCellListener)

namespace RBX {

TerrainPartitionMega::TerrainPartitionMega(Voxel::Grid* voxelGrid)
    : voxelGrid(voxelGrid)
{
    voxelGrid->connectListener(this);
}

TerrainPartitionMega::~TerrainPartitionMega()
{
    voxelGrid->disconnectListener(this);
}

void TerrainPartitionMega::findCellsTouchingExtents(const Extents& extents, std::vector<Vector3int16>* found) const
{
    Extents clampedExtents = extents;

    if (!clampedExtents.clampToOverlap(Voxel::getTerrainExtents()))
    {
        return;
    }

    Vector3int16 minPos = Voxel::worldToCell_floor(clampedExtents.min());
    Vector3int16 maxPos = Voxel::worldToCell_floor(clampedExtents.max());

    SpatialRegion::Id minRegion = SpatialRegion::regionContainingVoxel(minPos);
    SpatialRegion::Id maxRegion = SpatialRegion::regionContainingVoxel(maxPos);

    for (int y = minRegion.value().y; y <= maxRegion.value().y; ++y)
    {
        for (int z = minRegion.value().z; z <= maxRegion.value().z; ++z)
        {
            for (int x = minRegion.value().x; x <= maxRegion.value().x; ++x)
            {
                SpatialRegion::Id region(x, y, z);

                if (const ChunkData* chunk = chunks.find(region))
                {
                    Region3int16 regionExtents = SpatialRegion::inclusiveVoxelExtentsOfRegion(region);

                    Vector3int16 minOffset = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(regionExtents.getMinPos().max(minPos));
                    Vector3int16 maxOffset = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(regionExtents.getMaxPos().min(maxPos));

                    findCellsInRegion(region, *chunk, minOffset, maxOffset, found);
                }
            }
        }
    }
}

inline unsigned int lsb(unsigned int value)
{
#ifdef _WIN32
	unsigned long bitPosition;
	_BitScanForward(&bitPosition, value);
	return bitPosition;
#else
	return ffs(value) - 1;
#endif
}

void TerrainPartitionMega::findCellsInRegion(const SpatialRegion::Id& region, const ChunkData& chunk, const Vector3int16& minOffset, const Vector3int16& maxOffset, std::vector<Vector3int16>* found) const
{
    Vector3int16 minSubOffset = minOffset >> Vector3int16(2, 1, 2);
    Vector3int16 maxSubOffset = maxOffset >> Vector3int16(2, 1, 2);

    Region3int16 offsetExtents(minOffset, maxOffset);

    for (int y = minSubOffset.y; y <= maxSubOffset.y; ++y)
    {
        for (int z = minSubOffset.z; z <= maxSubOffset.z; ++z)
        {
            for (int x = minSubOffset.x; x <= maxSubOffset.x; ++x)
            {
                unsigned int group = chunk.filled[y][z][x];

                // this group has certain bits set; let's just iterate through all of them
                while (group)
                {
                    unsigned int bit = lsb(group);

                    Vector3int16 pos((x << 2) | (bit & 3), (y << 1) | (bit >> 4), (z << 2) | ((bit >> 2) & 3));

                    if (offsetExtents.contains(pos))
                    {
                        found->push_back(SpatialRegion::globalVoxelCoordinateFromRegionAndRelativeCoordinate(region, pos));
                    }

                    group &= ~(1 << bit);
                }
            }
        }
    }
}

void TerrainPartitionMega::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	bool beforeNotSolid = info.beforeCell.solid.getBlock() == Voxel::CELL_BLOCK_Empty;
	bool afterNotSolid = info.afterCell.solid.getBlock() == Voxel::CELL_BLOCK_Empty;

	if (beforeNotSolid != afterNotSolid)
    {
        SpatialRegion::Id region = SpatialRegion::regionContainingVoxel(info.position);
        ChunkData& chunk = chunks.insert(region);

        Vector3int16 localPos = SpatialRegion::voxelCoordinateRelativeToEnclosingRegion(info.position);

        unsigned int bitPos = (localPos.x & 3) | ((localPos.z & 3) << 2) | ((localPos.y & 1) << 4);
        unsigned int bitMask = 1 << bitPos;

		if (beforeNotSolid)
        {
            RBXASSERT((chunk.filled[localPos.y >> 1][localPos.z >> 2][localPos.x >> 2] & bitMask) == 0);

            chunk.filled[localPos.y >> 1][localPos.z >> 2][localPos.x >> 2] |= bitMask;
            chunk.count++;
        }
		else
        {
            RBXASSERT((chunk.filled[localPos.y >> 1][localPos.z >> 2][localPos.x >> 2] & bitMask) != 0);

            chunk.filled[localPos.y >> 1][localPos.z >> 2][localPos.x >> 2] &= ~bitMask;
            chunk.count--;

            if (chunk.count == 0)
            {
                // last cell in the chunk, we don't need it any more
                chunks.erase(region);
            }
        }
	}
}

TerrainPartitionSmooth::TerrainPartitionSmooth(Voxel2::Grid* grid)
    : grid(grid)
{
	for (int min = 0; min < kChunkSize; ++min)
        for (int max = 0; max < kChunkSize; ++max)
		{
            uint64_t hor = 0;

            for (int z = min; z <= max; ++z)
				for (int x = 0; x < kChunkSize; ++x)
					hor |= 1ull << (z * kChunkSize + x);

            uint64_t ver = 0;

            for (int z = 0; z < kChunkSize; ++z)
                for (int x = min; x <= max; ++x)
					ver |= 1ull << (z * kChunkSize + x);

			masksHor[min][max] = hor;
			masksVer[min][max] = ver;
		}
}

TerrainPartitionSmooth::~TerrainPartitionSmooth()
{
}

void TerrainPartitionSmooth::findChunksTouchingExtents(const Extents& extents, std::vector<ChunkResult>* found) const
{
	Voxel2::Region region = Voxel2::Region::fromExtents(extents.min(), extents.max());

	if (region.empty())
		return;

    Vector3int32 margin = Vector3int32::one();

    Vector3int32 minPos = region.begin();
    Vector3int32 maxPos = region.end() - Vector3int32::one();

	Vector3int32 minChunk = (minPos - margin) >> kChunkSizeLog2;
	Vector3int32 maxChunk = (maxPos + margin) >> kChunkSizeLog2;

	for (int cy = minChunk.y; cy <= maxChunk.y; ++cy)
		for (int cz = minChunk.z; cz <= maxChunk.z; ++cz)
			for (int cx = minChunk.x; cx <= maxChunk.x; ++cx)
			{
				ChunkMap::const_iterator it = chunks.find(Vector3int32(cx, cy, cz));

                if (it != chunks.end())
					fillChunkIfTouchingExtents(it->first, it->second, minPos, maxPos, found);
			}
}

template <int Size> static void dilate(uint32_t (&data)[Size][Size])
{
	// Dilate along X
	for (int y = 0; y < Size; ++y)
		for (int z = 0; z < Size; ++z)
			data[y][z] = (data[y][z] << 1) | data[y][z] | (data[y][z] >> 1);

	// Dilate along Z
	uint32_t temp[Size][Size];

	for (int y = 0; y < Size; ++y)
		for (int z = 1; z < Size - 1; ++z)
			temp[y][z] = data[y][z - 1] | data[y][z] | data[y][z + 1];

	// Dilate along Y
	for (int y = 1; y < Size - 1; ++y)
		for (int z = 0; z < Size; ++z)
			data[y][z] = temp[y - 1][z] | temp[y][z] | temp[y + 1][z];
}

void TerrainPartitionSmooth::updateChunk(const Vector3int32& id)
{
    Voxel2::Region region = Voxel2::Region::fromChunk(id, kChunkSizeLog2).expand(1);
	Voxel2::Box box = grid->read(region);

	if (box.isEmpty())
	{
		chunks.erase(id);
        return;
	}

	ChunkData& chunk = chunks[id];

	static const int size = (1 << kChunkSizeLog2) + 2;

	// we pack two bitmasks in 1 32-bit register
	BOOST_STATIC_ASSERT(size < 16);

	RBXASSERT(box.getSizeX() == size && box.getSizeY() == size && box.getSizeZ() == size);

	// Low 16 bits are solid, high 16 bits are water; we only use 10/16 bits
	uint32_t bits[size][size];

	// Fill bitmasks
	for (int y = 0; y < size; ++y)
	{
		for (int z = 0; z < size; ++z)
		{
			uint32_t solidRow = 0;
			uint32_t waterRow = 0;

			const Voxel2::Cell* row = box.readRow(0, y, z);

			for (int x = 0; x < size; ++x)
			{
				unsigned char material = row[x].getMaterial();

				solidRow |= (material > Voxel2::Cell::Material_Water) << x;
				waterRow |= (material == Voxel2::Cell::Material_Water) << x;
			}

			bits[y][z] = solidRow | (waterRow << 16);
		}
	}

	// Dilate along X/Y/Z; output border bits are not defined but we won't use them
	dilate(bits);

	// Fill slices
    for (int y = 0; y < kChunkSize; ++y)
	{
        uint64_t solid = 0;
        uint64_t water = 0;

		for (int z = 0; z < kChunkSize; ++z)
		{
            unsigned char solidRow = bits[y + 1][z + 1] >> 1;
            unsigned char waterRow = bits[y + 1][z + 1] >> 17;

            solid |= uint64_t(solidRow) << (z * kChunkSize);
			water |= uint64_t(waterRow) << (z * kChunkSize);
		}

        chunk.slices[y].solid = solid;
        chunk.slices[y].water = water;
	}
}

void TerrainPartitionSmooth::fillChunkIfTouchingExtents(const Vector3int32& chunkId, const ChunkData& chunkData, const Vector3int32& minPos, const Vector3int32& maxPos, std::vector<ChunkResult>* found) const
{
	Vector3int32 chunkOffset = chunkId << kChunkSizeLog2;

	Vector3int32 regionMin = (minPos - chunkOffset).max(Vector3int32(0, 0, 0)).min(Vector3int32(kChunkSize - 1, kChunkSize - 1, kChunkSize - 1));
	Vector3int32 regionMax = (maxPos - chunkOffset).max(Vector3int32(0, 0, 0)).min(Vector3int32(kChunkSize - 1, kChunkSize - 1, kChunkSize - 1));

    RBXASSERT(regionMin.x <= regionMax.x && regionMin.y <= regionMax.y && regionMin.z <= regionMax.z);

	uint64_t mask = masksVer[regionMin.x][regionMax.x] & masksHor[regionMin.z][regionMax.z];

    bool touchesSolid = false;
    bool touchesWater = false;

    for (int y = regionMin.y; y <= regionMax.y; ++y)
	{
		if (chunkData.slices[y].solid & mask)
			touchesSolid = true;

		if (chunkData.slices[y].water & mask)
			touchesWater = true;

		if (touchesSolid & touchesWater)
            break;
	}

	if (touchesSolid | touchesWater)
	{
		ChunkResult result = { chunkId, touchesSolid, touchesWater };
		found->push_back(result);
	}
}

} // namespace
