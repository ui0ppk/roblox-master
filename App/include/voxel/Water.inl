#pragma once

#include "Voxel/Util.h"

namespace RBX { namespace Voxel {

namespace Water {

extern const RelevantNeighbors kRelevantNeighbors[MAX_CELL_ORIENTATIONS];

namespace {

const FaceDirection kOppositeFaceDirection[Invalid] = {
	MinusX,
	MinusZ,
	PlusX,
	PlusZ,
	MinusY,
	PlusY,
};

const FaceDirection kPrimaryNeighborByOrientation[MAX_CELL_ORIENTATIONS] = {
	PlusZ,
	PlusX,
	MinusZ,
	MinusX
};

const FaceDirection kSecondaryNeighborByOrientation[MAX_CELL_ORIENTATIONS] = {
	MinusX,
	PlusZ,
	PlusX,
	MinusZ
};

const Vector3int16 kAboveNeighborCellOffset(0,1,0);
const Vector3int16 kPrimaryNeighborCellOffset[MAX_CELL_ORIENTATIONS] = {
	Vector3int16(0,0,1),
	Vector3int16(1,0,0),
	Vector3int16(0,0,-1),
	Vector3int16(-1,0,0),
};
const Vector3int16 kSecondaryNeighborCellOffset[MAX_CELL_ORIENTATIONS] = {
	Vector3int16(-1,0,0),
	Vector3int16(0,0,1),
	Vector3int16(1,0,0),
	Vector3int16(0,0,-1),
};

bool isWaterOnWedge(const Cell& center, const LocalAreaInfo& info) {
	if (center.solid.getBlock() != CELL_BLOCK_Empty && center.solid.getBlock() != CELL_BLOCK_Solid) {

		if (info.aboveNeighbor.isExplicitWaterCell()) {
			return true;
		}

		CellOrientation cellOrientation = center.solid.getOrientation();
		FaceDirection primaryDirection = kPrimaryNeighborByOrientation[cellOrientation];
		const Cell& primaryNeighbor = info.primaryNeighbor;

		if (center.solid.getBlock() == CELL_BLOCK_VerticalWedge) {
			return primaryNeighbor.isExplicitWaterCell();
		} else {
			bool isPrimaryNeighborWater = primaryNeighbor.isExplicitWaterCell();
			bool isPrimarysSharedFaceNotSolidAndNotEmpty = !primaryNeighbor.isEmpty() &&
				isWedgeSideNotFull(primaryNeighbor, kOppositeFaceDirection[primaryDirection]);

			FaceDirection secondaryDirection = kSecondaryNeighborByOrientation[cellOrientation];
			const Cell& secondaryNeighbor = info.secondaryNeighbor;
			bool isSecondaryNeighborWater = secondaryNeighbor.isExplicitWaterCell();
			bool isSecondarysSharedFaceNotSolidAndNotEmpty = !secondaryNeighbor.isEmpty() &&
				isWedgeSideNotFull(secondaryNeighbor, kOppositeFaceDirection[secondaryDirection]);

			const Cell& diagonalNeighbor = info.diagonalNeighbor;
			bool isDiagonalWater = diagonalNeighbor.isExplicitWaterCell();

			// add a special case for inv corner water wedges:
			// * can check the x, z, and +y offsets
			// * the block is an InverseCornerWedge
			// * x, z, and x+z offsets are all seperately not empty
			// * x + z + y offsets taken together contains explicit water
			bool inverseCornerWedgeVerticalDiagonalWaterCase =
				center.solid.getBlock() == CELL_BLOCK_InverseCornerWedge &&
				!primaryNeighbor.isEmpty() &&
				!secondaryNeighbor.isEmpty() &&
				!diagonalNeighbor.isEmpty() &&
				info.diagonalUpNeighbor.isExplicitWaterCell();

			bool bothOrthoNeighborsAreExplicitWater = isPrimaryNeighborWater && isSecondaryNeighborWater;

			bool bothOrthoNeighborsSupportDiagonalWater = 
				(isPrimaryNeighborWater || isPrimarysSharedFaceNotSolidAndNotEmpty) &&
				(isSecondaryNeighborWater || isSecondarysSharedFaceNotSolidAndNotEmpty);

			return
				inverseCornerWedgeVerticalDiagonalWaterCase ||
				(isDiagonalWater && bothOrthoNeighborsSupportDiagonalWater) ||
				bothOrthoNeighborsAreExplicitWater;
		}
	}
	return false;
}

template<class BoxType>
bool isWaterOnWedge(const BoxType* reader, const Cell& cell, const Vector3int16& globalCoord) {
	LocalAreaInfo info;
	reader->fillLocalAreaInfo(globalCoord, kRelevantNeighbors[cell.solid.getOrientation()], &info);
	return isWaterOnWedge(cell, info);
}

}

template<class BoxType>
bool cellHasWater(const BoxType* reader, const Cell& center,
		const Vector3int16& globalCoord) {
	return !center.isEmpty() &&
		center.solid.getBlock() != CELL_BLOCK_Solid &&
		(center.solid.getBlock() == CELL_BLOCK_Empty || isWaterOnWedge(reader, center, globalCoord));
}

template<class BoxType>
Cell interpretAsWaterCell(const BoxType* reader, const Cell& cell,
		const Vector3int16& globalCoord) {
	if (cellHasWater(reader, cell, globalCoord)) {
		if (cell.solid.getBlock() == CELL_BLOCK_Empty) {
			return cell;
		} else {
			return Constants::kWaterOnWedgeCell;
		}
	} else {
		return Constants::kUniqueEmptyCellRepresentation;
	}
}

} // Water
} }

