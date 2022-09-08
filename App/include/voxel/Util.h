#pragma once

#include "Util/G3DCore.h"
#include "Voxel/Cell.h"
#include "rbx/Debug.h"
#include "Util/Extents.h"
#include "Util/Region3int16.h"

////////////////////////////////////////////////////////////////////////////////
// This file has methods for reading and writing individual voxel cells

namespace RBX { namespace Voxel {

inline CellMaterial getCellMaterial_Deprecated( unsigned char cell ) { return (CellMaterial)(cell & 0x07); }
inline void setCellMaterial_Deprecated( unsigned char& cell, CellMaterial material ) { cell = (cell & 0xf8) | ((int)material & 0x07); }

inline CellMaterial readMaterial(const unsigned char* materials, const unsigned int cellIndex, const Cell cell) {
    return (CellMaterial)(
		cell.solid.getBlock() == CELL_BLOCK_Empty ?
		CELL_MATERIAL_Water :
		((materials[cellIndex >> 1] >> (4 * (cellIndex & 0x1))) & 0x0f) + 1);
}
inline void writeMaterial(unsigned char* materials, unsigned int cellIndex, const CellMaterial newMaterial) {
	RBXASSERT(newMaterial > 0);
    unsigned char& wholeByte = materials[cellIndex >> 1];
    unsigned int shift = (4 * (cellIndex & 0x1));
    unsigned char mask = 0x0f << shift;
    wholeByte &= (~mask);
    wholeByte |= (((newMaterial-1) << shift) & mask);
}

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

inline const BlockAxisFace& GetOrientedFace(Cell cell, FaceDirection f)
{
	return OrientedFaceMap[ Cell::asUnsignedCharForDeprecatedUses(cell)*6 + f ];
}

inline bool isWedgeSideNotFull(Cell voxel, FaceDirection f) {
	return GetOrientedFace(voxel, f).skippedCorner != BlockAxisFace::FullNoneSkipped;
}

inline Vector3int16 worldToCell_floor(const Vector3& worldPos) {
	const int kXZOffset = 0;
	return Vector3int16(
		(int)(floorf(worldPos.x / kCELL_SIZE)) + kXZOffset,
		(int)(floorf(worldPos.y / kCELL_SIZE)),
		(int)(floorf(worldPos.z / kCELL_SIZE)) + kXZOffset);
}

inline Vector3 worldSpaceToCellSpace(const Vector3& worldPos) {
	return Vector3(
		(worldPos.x * (1.0f / kCELL_SIZE)),
		(worldPos.y * (1.0f / kCELL_SIZE)),
		(worldPos.z * (1.0f / kCELL_SIZE)));
}

inline Vector3 cellSpaceToWorldSpace(const Vector3& cellPos)
{
	return Vector3(
		(cellPos.x * kCELL_SIZE),
		(cellPos.y * kCELL_SIZE),
		(cellPos.z * kCELL_SIZE));
}


inline Vector3 cellToWorld_smallestCorner(const Vector3int16& cellPos) {
	const int kXZOffset = 0;
	return Vector3(
		(cellPos.x - kXZOffset) * kCELL_SIZE,
		cellPos.y * kCELL_SIZE,
		(cellPos.z - kXZOffset) * kCELL_SIZE);

}

inline Vector3 cellToWorld_center(const Vector3int16& cellPos) {
	Vector3 pos = cellToWorld_smallestCorner(cellPos);
	return pos + Vector3(kHALF_CELL, kHALF_CELL, kHALF_CELL);
}

inline Vector3 cellToWorld_largestCorner(const Vector3int16& cellPos) {
	return cellToWorld_smallestCorner(cellPos + Vector3int16(1, 1, 1));
}

inline Region3int16 getTerrainExtentsInCells()
{
    const int kRadius = 32000;

    return Region3int16(Vector3int16(-kRadius, -kRadius, -kRadius), Vector3int16(kRadius, kRadius, kRadius));
}

inline Extents getTerrainExtents()
{
    Region3int16 extents = getTerrainExtentsInCells();

    return Extents(cellToWorld_smallestCorner(extents.getMinPos()), cellToWorld_largestCorner(extents.getMaxPos()));
}

} }
