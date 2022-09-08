#pragma once

#include "Util/G3DCore.h"
#include "Voxel/Cell.h"
#include "rbx/Debug.h"

////////////////////////////////////////////////////////////////////////////////
// This file has methods for reading and writing individual voxel cells

namespace RBX { 
using namespace Voxel;
namespace V1Deprecated {

const unsigned char UNIQUE_EMPTY_CELL_REPRESENTATION = ((unsigned char)CELL_BLOCK_Empty << 3);
const unsigned char WATER_ON_WEDGE_CELL = ((unsigned char) UNIQUE_EMPTY_CELL_REPRESENTATION + 3);


// Helper functions to access bit aligned properties of the cell grid
//  opted for this instead of a bit field struct in case of portability requirements
/*struct Terrain
{
	unused: 3;
	unsigned block : 3;
	unsigned orientation: 2;
};*/
/*struct Water
{
	water_state : 3;
	unsigned block : 3;
	water_state : 2;
};*/
// Water state has 5 bits total, so 32 possibilities. 30 of them used as (6 water cell directions x 5 water forces)
inline CellMaterial getCellMaterial_Deprecated( unsigned char cell ) { return (CellMaterial)(cell & 0x07); }
inline CellBlock getCellBlock( unsigned char cell )  { return (CellBlock)((cell & 0x38) >> 3); }
inline CellOrientation getCellOrientation( unsigned char cell )  { return (CellOrientation)((cell & 0xc0) >> 6); }
inline WaterCellForce getWaterCellForce( unsigned char cell ) {
	unsigned char nonBlockData = (((cell >> 3) & 0x18) | (cell & 0x7)) - 1;
	return (WaterCellForce) ((nonBlockData  / MAX_WATER_CELL_DIRECTIONS) % MAX_WATER_CELL_FORCES);
}
inline WaterCellDirection getWaterCellDirection( unsigned char cell ) {
	unsigned char nonBlockData = (((cell >> 3) & 0x18) | (cell & 0x7)) - 1;
	return (WaterCellDirection) (nonBlockData % MAX_WATER_CELL_DIRECTIONS);
}
    
// IMPORTANT! DUE TO THE ENCODING, THIS ONLY WORKS WHEN THE MATERIALS
// POINTER PASSED IN IS MEANT TO INDICATE cellIndex 0 and cellIndex 1
// (for example, if you created a cell box pointing to cell 5 as cell
// index 0, then this computation will have an off-by-one error).
// To account for this restriction, anywhere that material pointers are
// stored, be sure to only store pointers corresponding to even-indexed
// cells.
inline unsigned char readMaterial(const unsigned char* materials, const unsigned int cellIndex, const unsigned char cellValue) {
    return getCellBlock(cellValue) == CELL_BLOCK_Empty ?
		CELL_MATERIAL_Water :
		((materials[cellIndex >> 1] >> (4 * (cellIndex & 0x1))) & 0x0f) + 1;
}
inline void writeMaterial(unsigned char* materials, unsigned int cellIndex, const unsigned char newMaterial) {
	RBXASSERT(newMaterial > 0);
    unsigned char& wholeByte = materials[cellIndex >> 1];
    unsigned int shift = (4 * (cellIndex & 0x1));
    unsigned char mask = 0x0f << shift;
    wholeByte &= (~mask);
    wholeByte |= (((newMaterial-1) << shift) & mask);
}

inline void setCellMaterial_Deprecated( unsigned char& cell, CellMaterial material ) { cell = (cell & 0xf8) | ((int)material & 0x07); }
inline void setCellBlock( unsigned char& cell, CellBlock block ) { cell = (cell & 0xc7) | (((int)block & 0x07) << 3); }
inline void setCellOrientation( unsigned char& cell, CellOrientation orientation ) { cell = (cell & 0x3f) | (((int)orientation & 0x03) << 6); }
inline void setWaterForceAndDirection( unsigned char& cell, WaterCellForce force, WaterCellDirection direction) {
		unsigned char rawData = force * MAX_WATER_CELL_DIRECTIONS + direction + 1;
		cell = (cell & (0x38)) | (rawData & 0x7) | ((rawData & 0x18) << 3);
}

inline bool isCellEmpty( unsigned char cell ) { return cell == UNIQUE_EMPTY_CELL_REPRESENTATION; }

struct CellChangeInfo {
	const Vector3int16 position;

	unsigned char beforeCell;
	unsigned char afterCell;
	unsigned char beforeWaterCell;
	unsigned char afterWaterCell;
	unsigned char afterMaterial;

	CellChangeInfo(const Vector3int16& position,
		unsigned char beforeCell, unsigned char afterCell,
		unsigned char beforeWaterCell, unsigned char afterWaterCell,
		unsigned char afterMaterial)
		: position(position)
		, beforeCell(beforeCell)
		, afterCell(afterCell)
		, beforeWaterCell(beforeWaterCell)
		, afterWaterCell(afterWaterCell)
		, afterMaterial(afterMaterial)
	{ }
};

enum FaceDirection
{
	PlusX = 0,
	PlusZ = 1,
	MinusX = 2,
	MinusZ = 3,
	PlusY = 4,
	MinusY = 5,
	Invalid = 6
};

struct BlockAxisFace {
	enum SkippedCorner {
		TopRight = 0,
		TopLeft = 1,
		BottomLeft = 2,
		BottomRight = 3,
		EmptyAllSkipped = 4,
		FullNoneSkipped = 5
	};
	
	SkippedCorner skippedCorner;

	static inline SkippedCorner rotate(SkippedCorner corner, const CellOrientation orient) {
		return (SkippedCorner) (corner < 4 ? (corner + orient) % 4 : corner);
	}

	static inline bool divideTopLeftToBottomRight(SkippedCorner corner) {
		return corner == TopRight || corner == BottomLeft || corner == FullNoneSkipped;
	}

	static inline SkippedCorner XZAxisMirror(SkippedCorner corner) {
		static SkippedCorner MIRROR[6] =
			{ TopLeft, TopRight, BottomRight, BottomLeft, EmptyAllSkipped, FullNoneSkipped };
		return MIRROR[corner];
	}

	static inline SkippedCorner YAxisMirror(SkippedCorner corner) {
		static SkippedCorner MIRROR[6] = 
			{ BottomRight, BottomLeft, TopLeft, TopRight, EmptyAllSkipped, FullNoneSkipped };
		return MIRROR[corner];
	}

	static BlockAxisFace inverse(const BlockAxisFace other) {
		static const SkippedCorner OPPOSITE_CORNER[6] = {
			BottomLeft,
			BottomRight,
			TopRight,
			TopLeft,
			FullNoneSkipped,
			EmptyAllSkipped 
		};

		BlockAxisFace out;
		out.skippedCorner = OPPOSITE_CORNER[other.skippedCorner];
		return out;
	}
};

struct BlockFaceInfo {
	// indexed by FaceDirection
	BlockAxisFace faces[6];
};

extern const BlockFaceInfo UnOrientedBlockFaceInfos[6];
extern BlockAxisFace OrientedFaceMap[ 1536 ]; // 2^8 * 6

// ComputeOrientedFace is not declared because it is an implementation detail
void initBlockOrientationFaceMap();

inline const BlockAxisFace& GetOrientedFace(unsigned char cell, FaceDirection f)
{
	return OrientedFaceMap[ cell*6 + f ];
}

inline bool isWedgeSideNotFull(unsigned char cell, FaceDirection f) {
	return GetOrientedFace(cell, f).skippedCorner != BlockAxisFace::FullNoneSkipped;
}

// WARNING: Most of the time you want to use getWaterCellInternal().
// There are many edge cases when using either of these methods, don't use them
// unless you understand the edge cases. getWaterCellInternal encapsulates all
// of the edge cases for you.
inline bool isExplicitWaterCell(const unsigned char test) {
	return getCellBlock(test) == CELL_BLOCK_Empty && !isCellEmpty(test);
}


}

}
