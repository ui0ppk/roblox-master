#pragma once

#include "util/Vector3int32.h"
#include <vector>

namespace RBX { namespace Voxel2 {

    class Cell
    {
    public:
        enum Material
		{
            Material_Air = 0,
            Material_Water = 1,

            Material_Bits = 6,
            Material_Max = (1 << Material_Bits) - 1
		};

        enum Occupancy
		{
            Occupancy_Bits = 8,
			Occupancy_Max = (1 << Occupancy_Bits) - 1
		};

        Cell()
        {
			// we rely on Air being 0 since we use memset elsewhere
			BOOST_STATIC_ASSERT(Material_Air == 0);

            this->material = 0;
            this->occupancy = 0;
        }
        
        Cell(unsigned char material, unsigned char occupancy)
        {
			RBXASSERT_VERY_FAST(material <= Material_Max && occupancy <= Occupancy_Max);

            // make sure occupancy is always 0 for Air material (0)
            this->material = material;
            this->occupancy = occupancy & (-static_cast<int>(material) >> 31);
        }
        
        unsigned char getMaterial() const { return material; }
        unsigned char getOccupancy() const { return occupancy; }

		bool operator==(const Cell& other) const { return material == other.material && occupancy == other.occupancy; }
		bool operator!=(const Cell& other) const { return !(*this == other); }
        
    private:
        unsigned char material;
        unsigned char occupancy;
    };
    
    class Region
    {
    public:
		Region(const Vector3int32& begin, const Vector3int32& end)
        : begin_(begin)
        , end_(end)
        {
            RBXASSERT_VERY_FAST(begin.x <= end.x && begin.y <= end.y && begin.z <= end.z);
        }
        
        Region(const Vector3int32& begin, unsigned int size)
        : begin_(begin)
        , end_(begin + Vector3int32(size, size, size))
        {
        }

		static Region fromChunk(const Vector3int32& id, unsigned int chunkSizeLog2)
		{
			return Region(id << chunkSizeLog2, 1 << chunkSizeLog2);
		}

		static Region fromExtents(const Vector3& min, const Vector3& max);
        
        const Vector3int32& begin() const { return begin_; }
        const Vector3int32& end() const { return end_; }
        
        Vector3int32 size() const { return end_ - begin_; }
 
        bool empty() const { return begin_.x == end_.x || begin_.y == end_.y || begin_.z == end_.z; }

        bool operator==(const Region& other) const { return begin_ == other.begin_ && end_ == other.end_; }
        bool operator!=(const Region& other) const { return begin_ != other.begin_ || end_ != other.end_; }
        
        bool aligned(unsigned int size) const;
		bool inside(const Region& other) const;

        Region intersect(const Region& other) const;
        Region expand(unsigned int size) const;
        Region expandToGrid(unsigned int size) const;
		Region offset(const Vector3int32& offset) const;
        Region downsample(unsigned int lod) const;

        std::vector<Vector3int32> getChunkIds(unsigned int chunkSizeLog2) const;
        unsigned long long getChunkCount(unsigned int chunkSizeLog2) const;

    private:
        Vector3int32 begin_;
        Vector3int32 end_;
    };
    
    class Box
    {
    public:
		Box();
        Box(int sizeX, int sizeY, int sizeZ);
        
        const Cell& get(int x, int y, int z) const
        {
			RBXASSERT_VERY_FAST(static_cast<unsigned>(x) < static_cast<unsigned>(sizeX) && static_cast<unsigned>(y) < static_cast<unsigned>(sizeY) && static_cast<unsigned>(z) < static_cast<unsigned>(sizeZ));
            return data.get() ? data[x + sizeX * z + sliceXZ * y] : emptyCell;
        }

        void set(int x, int y, int z, const Cell& cell)
        {
			RBXASSERT_VERY_FAST(static_cast<unsigned>(x) < static_cast<unsigned>(sizeX) && static_cast<unsigned>(y) < static_cast<unsigned>(sizeY) && static_cast<unsigned>(z) < static_cast<unsigned>(sizeZ));
            if (!data) allocate();
            data[x + sizeX * z + sliceXZ * y] = cell;
        }

        const Cell* readRow(int x, int y, int z) const
        {
			RBXASSERT_VERY_FAST(static_cast<unsigned>(x) < static_cast<unsigned>(sizeX) && static_cast<unsigned>(y) < static_cast<unsigned>(sizeY) && static_cast<unsigned>(z) < static_cast<unsigned>(sizeZ));
			RBXASSERT_VERY_FAST(data.get());
            return &data[x + sizeX * z + sliceXZ * y];
        }

        Cell* writeRow(int x, int y, int z)
        {
			RBXASSERT_VERY_FAST(static_cast<unsigned>(x) < static_cast<unsigned>(sizeX) && static_cast<unsigned>(y) < static_cast<unsigned>(sizeY) && static_cast<unsigned>(z) < static_cast<unsigned>(sizeZ));
            if (!data) allocate();
            return &data[x + sizeX * z + sliceXZ * y];
        }

        int getSizeX() const { return sizeX; }
        int getSizeY() const { return sizeY; }
        int getSizeZ() const { return sizeZ; }

		Vector3int32 getSize() const { return Vector3int32(sizeX, sizeY, sizeZ); }

        bool isEmpty() const { return !data; }

        Box clone() const;
        
    private:
        int sizeX;
        int sizeY;
        int sizeZ;
        int sliceXZ;
        
        boost::shared_ptr<Cell[]> data;

        static const Cell emptyCell;

        void allocate();
    };

    class GridListener;

    class Grid
    {
    public:
        Grid();

		void connectListener(GridListener* listener);
		void disconnectListener(GridListener* listener);
        
        Box read(const Region& region, int lod = 0) const;
        void write(const Region& region, const Box& box);

        Cell getCell(int x, int y, int z) const;

        std::vector<Region> getNonEmptyRegions() const;
        std::vector<Region> getNonEmptyRegionsInside(const Region& region) const;

        unsigned int getNonEmptyCellCountApprox() const;

		bool isAllocated() const { return !chunks.empty(); }

        void serialize(std::string& result) const;
        void deserialize(const std::string& result);
    
    private:
		enum { kChunkMips = 4 };

        struct Chunk
        {
			Box data[kChunkMips];
            unsigned int volume;
            
            Chunk();

			bool isEmpty() const;
        };
        
		boost::unordered_map<Vector3int32, Chunk> chunks;
        unsigned int chunksVolume;

        std::vector<GridListener*> listeners;
    };

} }
