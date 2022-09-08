#include "stdafx.h"
#include "voxel2/Grid.h"

#include "voxel2/GridListener.h"

#include "voxel/Util.h"

#include "rbx/Profiler.h"

namespace RBX { namespace Voxel2 {

    const unsigned int kChunkSizeLog2 = 5;
    const unsigned int kChunkSize = 1 << kChunkSizeLog2;

    static bool hasSolidCells(const Cell* row, int size)
    {
        for (int i = 0; i < size; ++i)
			if (row[i].getMaterial() != Cell::Material_Air)
                return true;

        return false;
    }

    static bool hasSolidCells(const Box& box)
    {
		if (box.isEmpty())
			return false;

		Vector3int32 size = box.getSize();

		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
				if (hasSolidCells(box.readRow(0, y, z), size.x))
					return true;

		return false;
    }
    
    static bool copyCells(Box& targetBox, const Region& targetRegion, const Box& sourceBox, const Region& sourceRegion)
    {
        Region region = sourceRegion.intersect(targetRegion);
        
        if (region.empty())
            return false;

        if (sourceBox.isEmpty() && targetBox.isEmpty())
            return false;
        
        Vector3int32 sourceOffset = region.begin() - sourceRegion.begin();
        Vector3int32 targetOffset = region.begin() - targetRegion.begin();
        
        Vector3int32 size = region.size();

        bool dirty = false;
        
		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
            {
                if (sourceBox.isEmpty())
                {
                    Cell* targetRow = targetBox.writeRow(targetOffset.x, targetOffset.y + y, targetOffset.z + z);

                    if (dirty || hasSolidCells(targetRow, size.x))
                    {
                        memset(targetRow, 0, size.x * sizeof(Cell));
                        dirty = true;
                    }
                }
                else
                {
                    const Cell* sourceRow = sourceBox.readRow(sourceOffset.x, sourceOffset.y + y, sourceOffset.z + z);

                    if (!targetBox.isEmpty() || hasSolidCells(sourceRow, size.x))
                    {
                        Cell* targetRow = targetBox.writeRow(targetOffset.x, targetOffset.y + y, targetOffset.z + z);

                        if (dirty || memcmp(targetRow, sourceRow, size.x * sizeof(Cell)) != 0)
                        {
    						memcpy(targetRow, sourceRow, size.x * sizeof(Cell));
                            dirty = true;
                        }
                    }
                }
            }

        return dirty;
    }

    static unsigned int countCells(const Box& box, unsigned int lod)
    {
        if (box.isEmpty())
            return 0;

        unsigned int result = 0;

		Vector3int32 size = box.getSize();

		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
            {
                const Cell* row = box.readRow(0, y, z);

                for (int x = 0; x < size.x; ++x)
                    if (row[x].getMaterial() != Cell::Material_Air)
                        result += (row[x].getOccupancy() + 1) << (lod * 3) >> Cell::Occupancy_Bits;
            }

        return result;
    }

	struct MergedCell
	{
		unsigned char material;
		unsigned int occupancy;

		MergedCell(const Cell& c)
		: material(c.getMaterial())
		, occupancy(c.getOccupancy())
		{
		}

		MergedCell(const MergedCell& c0, const MergedCell& c1)
		{
			if (c0.material == c1.material)
			{
				material = c0.material;
				occupancy = c0.occupancy + c1.occupancy;
			}
			else if (c0.occupancy != c1.occupancy)
			{
				// Cell with higher occupancy wins
				*this = (c0.occupancy > c1.occupancy) ? c0 : c1;
			}
			else
			{
				// Cell with higher material wins - this is important to make sure any solid cell wins over air
				*this = (c0.material > c1.material) ? c0 : c1;
			}
		}
	};

	static Cell downsampleCell(Cell c000, Cell c001, Cell c010, Cell c011, Cell c100, Cell c101, Cell c110, Cell c111)
	{
		unsigned char occupancy =
			(c000.getOccupancy() + c001.getOccupancy() + c010.getOccupancy() + c011.getOccupancy() +
			 c100.getOccupancy() + c101.getOccupancy() + c110.getOccupancy() + c111.getOccupancy() + 7) / 8;

		MergedCell m00 = MergedCell(c000, c001);
		MergedCell m01 = MergedCell(c010, c011);
		MergedCell m10 = MergedCell(c100, c101);
		MergedCell m11 = MergedCell(c110, c111);

		MergedCell m0 = MergedCell(m00, m01);
		MergedCell m1 = MergedCell(m10, m11);

		MergedCell m = MergedCell(m0, m1);

		return Cell(m.material, occupancy);
	}

	static void downsampleCells(Box& targetBox, const Region& targetRegion, const Box& sourceBox)
	{
        if (targetRegion.empty())
            return;

        if (sourceBox.isEmpty() && targetBox.isEmpty())
            return;

		Vector3int32 size = targetRegion.size();

        Vector3int32 sourceOffset = targetRegion.begin() * 2;
        Vector3int32 targetOffset = targetRegion.begin();

		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
			{
                if (sourceBox.isEmpty())
                {
                    Cell* targetRow = targetBox.writeRow(targetOffset.x, targetOffset.y + y, targetOffset.z + z);

                    memset(targetRow, 0, size.x * sizeof(Cell));
                }
                else
                {
                    const Cell* sourceRow00 = sourceBox.readRow(sourceOffset.x, sourceOffset.y + y * 2 + 0, sourceOffset.z + z * 2 + 0);
                    const Cell* sourceRow10 = sourceBox.readRow(sourceOffset.x, sourceOffset.y + y * 2 + 1, sourceOffset.z + z * 2 + 0);
                    const Cell* sourceRow01 = sourceBox.readRow(sourceOffset.x, sourceOffset.y + y * 2 + 0, sourceOffset.z + z * 2 + 1);
                    const Cell* sourceRow11 = sourceBox.readRow(sourceOffset.x, sourceOffset.y + y * 2 + 1, sourceOffset.z + z * 2 + 1);

					Cell* targetRow = targetBox.writeRow(targetOffset.x, targetOffset.y + y, targetOffset.z + z);

					for (int x = 0; x < size.x; ++x)
					{
						targetRow[x] = downsampleCell(
							sourceRow00[x * 2 + 0], sourceRow00[x * 2 + 1],
							sourceRow10[x * 2 + 0], sourceRow10[x * 2 + 1],
							sourceRow01[x * 2 + 0], sourceRow01[x * 2 + 1],
							sourceRow11[x * 2 + 0], sourceRow11[x * 2 + 1]);
                    }
                }
			}
	}
    
    static unsigned char readUInt8(const std::string& data, unsigned int& readOffset)
    {
        if (readOffset >= data.size())
            throw RBX::runtime_error("Error while decoding data: unexpected end at offset %u", readOffset);
        
        return data[readOffset++];
    }

	const int kEncodingCountBit = 7;
	const int kEncodingOccupancyBit = 6;
	const int kEncodingMaterialMask = (1 << kEncodingOccupancyBit) - 1;

    static void encodeCellRun(std::string& result, const Cell& cell, unsigned int count)
	{
		// material has two top bits free and we want byte-wise encoding that minimizes extra bytes
		BOOST_STATIC_ASSERT(Cell::Material_Max <= kEncodingMaterialMask);

		// 1xxx: single cell vs multiple cells (1 byte for count)
		// x1xx: trivial occupancy value (full for solid, empty for air) vs explicit occupancy
		// after material we have optional occupancy and optional count; count is 2+ but we'll store count-1 to have a max of 256 and reserve "0" for smth special just in case
		bool needCount = (count != 1);
		bool needOccupancy = (cell.getMaterial() != Cell::Material_Air && cell.getOccupancy() != Cell::Occupancy_Max);

		result += cell.getMaterial() | (needOccupancy << kEncodingOccupancyBit) | (needCount << kEncodingCountBit);

		if (needOccupancy)
			result += cell.getOccupancy();

		if (needCount)
			result += count - 1;
	}

    static void encodeChunk(std::string& result, const Box& data, std::vector<Cell>& cells)
    {
		Vector3int32 size = data.getSize();

		cells.resize(size.x * size.y * size.z);

        unsigned int offset = 0;

		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
			{
                memcpy(&cells[offset], data.readRow(0, y, z), size.x * sizeof(Cell));
                offset += size.x;
			}

        Cell lastCell;
        unsigned int lastCount = 0;
        
        for (size_t i = 0; i < cells.size(); ++i)
        {
            if (lastCount < 256 && (cells[i] == lastCell || lastCount == 0))
            {
                lastCell = cells[i];
                lastCount++;
            }
            else
            {
				encodeCellRun(result, lastCell, lastCount);
                
                lastCell = cells[i];
                lastCount = 1;
            }
        }
        
        if (lastCount)
        {
            encodeCellRun(result, lastCell, lastCount);
        }
    }

    static std::pair<Cell, unsigned int> decodeCellRun(const std::string& data, unsigned int& readOffset)
	{
		int meta = readUInt8(data, readOffset);
		int occupancy = (meta & (1 << kEncodingOccupancyBit)) ? readUInt8(data, readOffset) : Cell::Occupancy_Max;
		int count = (meta & (1 << kEncodingCountBit)) ? readUInt8(data, readOffset) + 1 : 1;

        return std::make_pair(Cell(meta & kEncodingMaterialMask, occupancy), count);
	}

    static void decodeChunk(const std::string& data, unsigned int& readOffset, Box& result, std::vector<Cell>& cells)
	{
		Vector3int32 size = result.getSize();

		cells.resize(size.x * size.y * size.z);

        unsigned int offset = 0;

        while (offset < cells.size())
		{
			std::pair<Cell, unsigned int> run = decodeCellRun(data, readOffset);

			if (offset + run.second > cells.size())
				throw RBX::runtime_error("Error while decoding data: chunk overflow at %u cells", offset + run.second);

            for (unsigned int i = 0; i < run.second; ++i)
                cells[offset + i] = run.first;

			offset += run.second;
		}

        unsigned int cellOffset = 0;

		for (int y = 0; y < size.y; ++y)
			for (int z = 0; z < size.z; ++z)
			{
				memcpy(result.writeRow(0, y, z), &cells[cellOffset], size.x * sizeof(Cell));
                cellOffset += size.x;
			}
	}
    
    struct DeallocateCells
    {
        DeallocateCells(size_t size)
        : size(size)
        {
        }
        
        void operator()(Cell* cells)
        {
            RBXPROFILER_COUNTER_SUB("memory/terrain/voxel", size);

    		::operator delete(cells);
        }
        
        size_t size;
    };

	Region Region::fromExtents(const Vector3& min, const Vector3& max)
	{
		Vector3 vmin = Voxel::worldSpaceToCellSpace(min);
		Vector3 vmax = Voxel::worldSpaceToCellSpace(max);

		Vector3int32 ibegin(floorf(vmin.x), floorf(vmin.y), floorf(vmin.z));
		Vector3int32 iend(ceilf(vmax.x), ceilf(vmax.y), ceilf(vmax.z));

		return Region(ibegin, ibegin.max(iend));
	}

	bool Region::aligned(unsigned int size) const
	{
        RBXASSERT(size != 0 && (size & (size - 1)) == 0);

        return ((begin_.x | begin_.y | begin_.z | end_.x | end_.y | end_.z) & (size - 1)) == 0;
	}

	bool Region::inside(const Region& other) const
	{
		Vector3int32 db = begin_ - other.begin_;
		Vector3int32 de = other.end_ - end_;

		return (db.x | db.y | db.z | de.x | de.y | de.z) >= 0;
	}

    Region Region::intersect(const Region& other) const
    {
        Vector3int32 ibegin = begin_.max(other.begin_);
        Vector3int32 iend = end_.min(other.end_);
        
        return Region(ibegin, ibegin.max(iend));
    }

    Region Region::expand(unsigned int size) const
    {
        Vector3int32 vsize(size, size, size);

        return Region(begin_ - vsize, end_ + vsize);
    }

	Region Region::expandToGrid(unsigned int size) const
	{
        RBXASSERT(size != 0 && (size & (size - 1)) == 0);

        int mask = size - 1;

        return Region(
			Vector3int32(begin_.x & ~mask, begin_.y & ~mask, begin_.z & ~mask),
			Vector3int32((end_.x + mask) & ~mask, (end_.y + mask) & ~mask, (end_.z + mask) & ~mask));
	}

	Region Region::offset(const Vector3int32& offset) const
	{
		return Region(begin_ + offset, end_ + offset);
	}

	Region Region::downsample(unsigned int lod) const
	{
		Region ar = expandToGrid(1 << lod);

		return Region(ar.begin_ >> lod, ar.end_ >> lod);
	}

    std::vector<Vector3int32> Region::getChunkIds(unsigned int chunkSizeLog2) const
    {
        if (empty())
			return std::vector<Vector3int32>();
        
        std::vector<Vector3int32> result;
        
        Vector3int32 min = begin() >> int(chunkSizeLog2);
        Vector3int32 max = (end() - Vector3int32(1, 1, 1)) >> int(chunkSizeLog2);
        
        for (int z = min.z; z <= max.z; ++z)
            for (int y = min.y; y <= max.y; ++y)
                for (int x = min.x; x <= max.x; ++x)
                    result.push_back(Vector3int32(x, y, z));
        
        return result;
    }

    unsigned long long Region::getChunkCount(unsigned int chunkSizeLog2) const
    {
		if (empty())
			return 0;

        Vector3int32 min = begin() >> int(chunkSizeLog2);
        Vector3int32 max = (end() - Vector3int32(1, 1, 1)) >> int(chunkSizeLog2);

		unsigned long long result = 1;
		result *= max.x - min.x + 1;
		result *= max.y - min.y + 1;
		result *= max.z - min.z + 1;

		return result;
    }

    const Cell Box::emptyCell;

    Box::Box()
	: sizeX(0)
	, sizeY(0)
	, sizeZ(0)
	, sliceXZ(0)
	{
	}

    Box::Box(int sizeX, int sizeY, int sizeZ)
    : sizeX(sizeX)
    , sizeY(sizeY)
    , sizeZ(sizeZ)
    , sliceXZ(sizeX * sizeZ)
    {
    }

    void Box::allocate()
    {
        size_t size = sizeX * sizeY * sizeZ * sizeof(Cell);

        // Cell can be zero-initialized for performance
        void* cells = ::operator new(size);
        memset(cells, 0, size);
        
        RBXPROFILER_COUNTER_ADD("memory/terrain/voxel", size);

		data.reset(static_cast<Cell*>(cells), DeallocateCells(size));
    }

    Box Box::clone() const
	{
        Box result(sizeX, sizeY, sizeZ);

        if (data)
		{
			size_t size = sizeX * sizeY * sizeZ * sizeof(Cell);

            void* cells = ::operator new(size);
            memcpy(cells, data.get(), size);

            RBXPROFILER_COUNTER_ADD("memory/terrain/voxel", size);

			result.data.reset(static_cast<Cell*>(cells), DeallocateCells(size));
		}

        return result;
	}

    Grid::Chunk::Chunk()
    : volume(0)
    {
		for (int mip = 0; mip < kChunkMips; ++mip)
		{
			int size = kChunkSize >> mip;

			data[mip] = Box(size, size, size);
		}
    }

	bool Grid::Chunk::isEmpty() const
	{
		return data[0].isEmpty();
	}
    
    Grid::Grid()
    : chunksVolume(0)
    {
		BOOST_STATIC_ASSERT(kChunkMips <= kChunkSizeLog2);
    }

	void Grid::connectListener(GridListener* listener)
	{
        RBXASSERT(std::find(listeners.begin(), listeners.end(), listener) == listeners.end());

		listeners.push_back(listener);
	}

	void Grid::disconnectListener(GridListener* listener)
	{
        std::vector<GridListener*>::iterator it = std::find(listeners.begin(), listeners.end(), listener);
		
		RBXASSERT(it != listeners.end());
		listeners.erase(it);
	}
    
    Box Grid::read(const Region& region, int lod) const
    {
        RBXPROFILER_SCOPE("Voxel", "read");

		RBXASSERT(region.aligned(1 << lod));

		if (lod < kChunkMips)
		{
			Region regionLod = region.downsample(lod);

			Box result(regionLod.size().x, regionLod.size().y, regionLod.size().z);
			
			std::vector<Vector3int32> chunkIds = region.getChunkIds(kChunkSizeLog2);
			
			for (auto cid: chunkIds)
			{
				auto cit = chunks.find(cid);
				
				if (cit != chunks.end())
				{
					const Chunk& chunk = cit->second;

					Region chunkRegionLod = Region::fromChunk(cid, kChunkSizeLog2 - lod);
					
					copyCells(result, regionLod, chunk.data[lod], chunkRegionLod);
				}
			}
			
			return result;
		}
		else
		{
			Box result = read(region, kChunkMips - 1);

			for (int i = kChunkMips - 1; i < lod; ++i)
			{
				Box next(result.getSizeX() / 2, result.getSizeY() / 2, result.getSizeZ() / 2);

				downsampleCells(next, Region(Vector3int32(), next.getSize()), result);

				result = next;
			}

			return result;
		}
    }
    
    void Grid::write(const Region& region, const Box& box)
    {
        RBXPROFILER_SCOPE("Voxel", "write");

		RBXASSERT(region.size() == box.getSize());
        
        std::vector<Vector3int32> chunkIds = region.getChunkIds(kChunkSizeLog2);
        std::vector<Region> dirtyRegions;
        
        for (auto cid: chunkIds)
        {
			auto cit = chunks.find(cid);

			// don't create new chunks during clears
			if (box.isEmpty() && cit == chunks.end())
				continue;

			Region chunkRegion = Region::fromChunk(cid, kChunkSizeLog2);

			// quick-erase chunks if we're clearing them with a single write 
			if (box.isEmpty() && chunkRegion.inside(region))
			{
                dirtyRegions.push_back(chunkRegion);

				chunksVolume -= cit->second.volume;

				chunks.erase(cit);
				continue;
			}

			// we have to create a chunk on demand
			if (cit == chunks.end())
				cit = chunks.insert(std::make_pair(cid, Chunk())).first;

			// copy data from box to chunk
			Chunk& chunk = cit->second;
		
			bool dirty = copyCells(chunk.data[0], chunkRegion, box, region);

            if (dirty)
            {
                Region updatedRegion = region.intersect(chunkRegion);

                dirtyRegions.push_back(updatedRegion);

    			// regenerate mipmap chain for the part of the chunk we updated
                Region updatedRegionChunk = updatedRegion.offset(-chunkRegion.begin());

    			for (int mip = 1; mip < kChunkMips; ++mip)
    			{
    				Region mipRegion = updatedRegionChunk.downsample(mip);

    				downsampleCells(chunk.data[mip], mipRegion, chunk.data[mip - 1]);
    			}

                // update approximate chunk volume based on last mip
				unsigned int cells = countCells(chunk.data[kChunkMips - 1], kChunkMips - 1);

				chunksVolume -= chunk.volume;
				chunksVolume += cells;

				chunk.volume = cells;
            }

			// if we did a lot of partial writes chunk may be empty; we can quickly check the low mip to make sure
			if (!hasSolidCells(chunk.data[kChunkMips - 1]))
            {
                RBXASSERT(chunk.volume == 0);

				chunks.erase(cit);
            }
        }

        // Update all listeners
        for (auto& l: listeners)
            for (auto& r: dirtyRegions)
    			l->onTerrainRegionChanged(r);
    }

	Cell Grid::getCell(int x, int y, int z) const
	{
		Vector3int32 chunkId = Vector3int32(x, y, z) >> int(kChunkSizeLog2);
		Vector3int32 chunkOffset = chunkId << int(kChunkSizeLog2);

        auto it = chunks.find(chunkId);
        if (it == chunks.end())
            return Cell();

		return it->second.data[0].get(x - chunkOffset.x, y - chunkOffset.y, z - chunkOffset.z);
	}

	std::vector<Region> Grid::getNonEmptyRegions() const
	{
        std::vector<Region> result;
		result.reserve(chunks.size());

        for (auto& c: chunks)
        {
            Region chunkRegion = Region::fromChunk(c.first, kChunkSizeLog2);

			result.push_back(chunkRegion);
		}

        return result;
	}

	std::vector<Region> Grid::getNonEmptyRegionsInside(const Region& region) const
	{
		std::vector<Region> result;

		// chunkMap.find() is more expensive than isBetweenInclusive
		if (region.getChunkCount(kChunkSizeLog2) < chunks.size() * 2)
		{
			// We're querying a relatively small area, let's just iterate through all regions
			std::vector<Vector3int32> chunkIds = region.getChunkIds(kChunkSizeLog2);

			for (auto cid: chunkIds)
			{
				if (chunks.find(cid) == chunks.end())
					continue;

				Region chunkRegion = Region::fromChunk(cid, kChunkSizeLog2);
				Region r = region.intersect(chunkRegion);

				result.push_back(r);
			}
		}
		else
		{
			// We're querying a relatively large area, let's scan through filled regions inside the grid
			for (auto& chunk: chunks)
			{
				Region chunkRegion = Region::fromChunk(chunk.first, kChunkSizeLog2);
				Region r = region.intersect(chunkRegion);

				if (!r.empty() && !chunk.second.isEmpty())
					result.push_back(r);
			}
		}

		return result;
	}

    unsigned int Grid::getNonEmptyCellCountApprox() const
	{
		return chunksVolume;
	}

    void Grid::serialize(std::string& result) const
	{
		int version = 1;

        result += version;
		result += kChunkSizeLog2;

        // get chunk ids sorted for stability and LZ efficiency
        std::vector<Vector3int32> ids;

        for (auto& c: chunks)
            if (!c.second.isEmpty())
                ids.push_back(c.first);

        std::sort(ids.begin(), ids.end());

        // encode chunks
        Vector3int32 lastIndex;
		std::vector<Cell> cells;

        for (size_t i = 0; i < ids.size(); ++i)
        {
            Vector3int32 id = ids[i];
            Vector3int32 diff = id - lastIndex;

            // encode chunk id (delta-encoding for LZ efficiency)
            for (int i = 3; i >= 0; --i)
            {
                result += diff.x >> (i * 8);
                result += diff.y >> (i * 8);
                result += diff.z >> (i * 8);
            }

            // encode chunk data
            auto cit = chunks.find(id);
            RBXASSERT(cit != chunks.end());

            encodeChunk(result, cit->second.data[0], cells);

            lastIndex = id;
        }
	}

    void Grid::deserialize(const std::string& data)
	{
        if (data.empty())
            return;

        unsigned int readOffset = 0;

		int version = static_cast<char>(readUInt8(data, readOffset));

        if (version != 1)
			throw RBX::runtime_error("Error while decoding data: unsupported version");

		int chunkSizeLog2 = readUInt8(data, readOffset);
		int chunkSize = 1 << chunkSizeLog2;

		if (chunkSizeLog2 > 8)
			throw RBX::runtime_error("Error while decoding data: malformed chunk size");

        Vector3int32 lastIndex;
		std::vector<Cell> cells;

		Box box(chunkSize, chunkSize, chunkSize);

        while (readOffset < data.size())
		{
            // decode chunk id
            for (int i = 3; i >= 0; --i)
            {
                lastIndex.x += static_cast<int>(readUInt8(data, readOffset) << (i * 8));
                lastIndex.y += static_cast<int>(readUInt8(data, readOffset) << (i * 8));
                lastIndex.z += static_cast<int>(readUInt8(data, readOffset) << (i * 8));
            }

            // decode chunk data
			decodeChunk(data, readOffset, box, cells);

			write(Region(lastIndex << chunkSizeLog2, chunkSize), box);
		}
	}

} }

