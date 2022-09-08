#pragma once

#include <boost/static_assert.hpp>
#include <ostream>

///////////////////////////////////////////////////////////////////////////////
// Defines voxels at the cellular and sub-cellular level

namespace RBX { namespace Voxel {

enum CellMaterial
{
	CELL_MATERIAL_Deprecated_Empty = 0,
	CELL_MATERIAL_Grass = 1,
	CELL_MATERIAL_Sand = 2,
	CELL_MATERIAL_Brick = 3,
	CELL_MATERIAL_Granite = 4,
	CELL_MATERIAL_Asphalt = 5,
	CELL_MATERIAL_Iron = 6,
	CELL_MATERIAL_Aluminum = 7,
	CELL_MATERIAL_Gold = 8,
	CELL_MATERIAL_Wood_Plank = 9,
	CELL_MATERIAL_Wood_Log = 10,
	CELL_MATERIAL_Gravel = 11,
	CELL_MATERIAL_Cinder_Block = 12,
	CELL_MATERIAL_Stone_Block = 13,
	CELL_MATERIAL_Cement = 14,
	CELL_MATERIAL_Red_Plastic = 15,
	CELL_MATERIAL_Blue_Plastic = 16,
	CELL_MATERIAL_Water = 17,
	CELL_MATERIAL_Unspecified = 255,
	MAX_CELL_MATERIALS = 18,
};

enum CellBlock
{
	CELL_BLOCK_Solid = 0,
	CELL_BLOCK_VerticalWedge = 1,
	CELL_BLOCK_CornerWedge = 2,
	CELL_BLOCK_InverseCornerWedge = 3,
	CELL_BLOCK_HorizontalWedge = 4,

	// Enum values below this line are intentionally not reflected!
	// Talk with dignatoff@ before exposing these enums!
	CELL_BLOCK_Empty = 5,

	//InverseVerticalWedge = 4,
	//TopCornerWedge = 5,
	MAX_CELL_BLOCKS = 8,
};

// Viewed from a downwards vertical perspective,
//  orientation defines the corner that the block starts in in a clockwise fashion
enum CellOrientation
{
	CELL_ORIENTATION_NegZ = 0, // upper left
	CELL_ORIENTATION_X = 1, // upper right
	CELL_ORIENTATION_Z = 2, // lower right
	CELL_ORIENTATION_NegX = 3, // lower left
	MAX_CELL_ORIENTATIONS = 4
};

enum WaterCellForce
{
	WATER_CELL_FORCE_None = 0,
	WATER_CELL_FORCE_Small = 1,
	WATER_CELL_FORCE_Medium = 2,
	WATER_CELL_FORCE_Strong = 3,
	WATER_CELL_FORCE_MaxForce = 4,
	MAX_WATER_CELL_FORCES = 5
};

enum WaterCellDirection
{
	WATER_CELL_DIRECTION_NegX = 0,
	WATER_CELL_DIRECTION_X = 1,
	WATER_CELL_DIRECTION_NegY = 2,
	WATER_CELL_DIRECTION_Y = 3,
	WATER_CELL_DIRECTION_NegZ = 4,
	WATER_CELL_DIRECTION_Z = 5,
	MAX_WATER_CELL_DIRECTIONS = 6
};

class SolidTerrainCell {
    // Material used to be stored in solid terrain voxel; it has
    // subsequently been moved to a completely separate storage mechanism.
    // The field is here to preserve memory layout with the earlier version.
	unsigned char DEPRECATED_material : 3;
	unsigned char block : 3;
	unsigned char orientation : 2;
	
public:
	CellBlock getBlock() const { return (CellBlock) block; }
	CellOrientation getOrientation() const {
		return (CellOrientation) orientation;
	}
	
	void setBlock(CellBlock block) { this->block = block; }
	void setOrientation(CellOrientation orientation) {
		this->orientation = orientation;
	}
};

class WaterCell {
    // Water voxels are implemented to be bit-compatible with solid voxels,
    // so this bit layout matches the bit layout of SolidTerrainCell.
	unsigned char dataPart2 : 3;
	unsigned char blockMustBeEmpty : 3;
	unsigned char dataPart1 : 2;
    
    unsigned int getWaterData() const {
        return ((dataPart1 << 3) | dataPart2) - 1;
    }
public:
	WaterCellForce getForce() const {
		return (WaterCellForce) ((getWaterData()  / MAX_WATER_CELL_DIRECTIONS) % MAX_WATER_CELL_FORCES);
	}
	WaterCellDirection getDirection() const {
		return (WaterCellDirection) (getWaterData() % MAX_WATER_CELL_DIRECTIONS);
	}
	
	void setForceAndDirection(WaterCellForce force, WaterCellDirection direction) {
		unsigned int rawData = (force * MAX_WATER_CELL_DIRECTIONS + direction) + 1;
		dataPart2 = rawData & 0x7;
		dataPart1 = (rawData >> 3) & 0x3;
	}
};

// Data structure that represents one voxel cell. The cell can either be water or solid terrain,
// so this class is a union of WaterCell and SolidTerrainCell.
union Cell {
	SolidTerrainCell solid;
	WaterCell water;

	Cell() {
        // There isn't a simple way to make sure all of the members are
        // zero by going through setter methods. SolidTerrainCell, for example,
        // has no way to control the bits in DEPRECATED_material.
        // Performance testing has shown this to not be a perf hit compared to
        // casting this to unsigned char* and setting that to zero.
		memset(this, 0, sizeof(Cell));
	}

	// True if the voxel is completely empty (no solid terrain and no water)
	inline bool isEmpty() const;
    
	// Indicates if this cell has been set to water explicitly by the user.
	// Note that water can also exist in wedge cells. Use Region and/or
	// Region::iterator methods for a way to detect all kinds of water
	// simultaneously.
	inline bool isExplicitWaterCell() const { return !isEmpty() && solid.getBlock() == CELL_BLOCK_Empty; }

	inline bool operator==(const Cell& other) const {
		return ((const unsigned char*)this)[0] == ((const unsigned char*)&other)[0];
	}
	inline bool operator!=(const Cell& other) const {
		return !((*this) == other);
	}

	// Convert to/from unsigned char, for networking and saving to file
	static inline unsigned char serializeAsUnsignedChar(const Cell v) {
		return ((unsigned char*)&v)[0];
	}
	static inline Cell deserializeFromUnsignedChar(unsigned char cell) {
		return ((Cell*)&cell)[0];
	}
	static inline unsigned char convertToUnsignedCharForFile(const Cell v) {
		return ((unsigned char*)&v)[0];
	}
	static inline Cell readUnsignedCharFromFile(unsigned char cell) {
		return ((Cell*)&cell)[0];
	}
	// Old style voxel access. Avoid using these methods where possible.
	static inline unsigned char asUnsignedCharForDeprecatedUses(const Cell v) {
		return ((unsigned char*)&v)[0];
	}
	static inline Cell readUnsignedCharFromDeprecatedUse(unsigned char cell) {
		return ((Cell*)&cell)[0];
	}
};

BOOST_STATIC_ASSERT(sizeof(Cell) == 1);

namespace Constants {
    // There is exactly one way to represent an empty cell. This constant stores that representation.
	extern const Cell kUniqueEmptyCellRepresentation;
    // When there is water in a cell that has a solid wedge, the water state is always the same. This
    // constant stores the water state for water-on-wedge cells.
	extern const Cell kWaterOnWedgeCell;
}

bool Cell::isEmpty() const {
	return (*this) == Constants::kUniqueEmptyCellRepresentation;
}

std::ostream& operator<<(std::ostream& os, const RBX::Voxel::Cell& v);

const int kXZ_CHUNK_SIZE = 32;
const int kY_CHUNK_SIZE = 16;

const int kCELL_SIZE = 4;
const int kHALF_CELL = kCELL_SIZE / 2;
const int kCELL_SIZE_AS_BIT_SHIFT = 2;

} }

