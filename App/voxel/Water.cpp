#include "stdafx.h"

#include "Voxel/Water.h"

namespace RBX { namespace Voxel { namespace Water {

const RelevantNeighbors kRelevantNeighbors[MAX_CELL_ORIENTATIONS] = {
	RelevantNeighbors(CELL_ORIENTATION_NegZ),
	RelevantNeighbors(CELL_ORIENTATION_X),
	RelevantNeighbors(CELL_ORIENTATION_Z),
	RelevantNeighbors(CELL_ORIENTATION_NegX),
};

RelevantNeighbors::RelevantNeighbors(CellOrientation orientation) :
	aboveNeighbor(kAboveNeighborCellOffset),
	primaryNeighbor(kPrimaryNeighborCellOffset[orientation]),
	secondaryNeighbor(kSecondaryNeighborCellOffset[orientation]),
	diagonalNeighbor(primaryNeighbor + secondaryNeighbor),
	diagonalUpNeighbor(primaryNeighbor + secondaryNeighbor + aboveNeighbor) {}

} } }
