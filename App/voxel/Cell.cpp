#include "stdafx.h"

#include "Voxel/Cell.h"

namespace RBX { namespace Voxel {

static Cell constructEmptyRepresentation() {
	Cell v;
	v.solid.setOrientation(CELL_ORIENTATION_NegZ);
	v.solid.setBlock(CELL_BLOCK_Empty);
	return v;
}
static Cell constructWaterOnWedge() {
	Cell v;
	v.solid.setBlock(CELL_BLOCK_Empty);
	v.water.setForceAndDirection(WATER_CELL_FORCE_None, WATER_CELL_DIRECTION_NegY);
	return v;
}	

const Cell Constants::kUniqueEmptyCellRepresentation = constructEmptyRepresentation();
const Cell Constants::kWaterOnWedgeCell = constructWaterOnWedge();

std::ostream& operator<<(std::ostream& os, const RBX::Voxel::Cell& v) {
	return os << (Cell::asUnsignedCharForDeprecatedUses(v));
}

} }
