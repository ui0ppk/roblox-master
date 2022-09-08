#pragma once

#include "Util/G3DCore.h"
#include "Voxel/Util.h"

namespace RBX { namespace Voxel {

namespace Water {
	// Generate relative cell coords relevant to the water on wedge state of a
	// cell. Some locations will be initialized to the center location if they
	// are irelevant to the water on wedge state.
	struct RelevantNeighbors {
		const Vector3int16 aboveNeighbor;
		const Vector3int16 primaryNeighbor;
		const Vector3int16 secondaryNeighbor;
		const Vector3int16 diagonalNeighbor;
		const Vector3int16 diagonalUpNeighbor;

		RelevantNeighbors(CellOrientation orientation);
	};

	struct LocalAreaInfo {
		Cell aboveNeighbor;
		Cell primaryNeighbor;
		Cell secondaryNeighbor;
		Cell diagonalNeighbor;
		Cell diagonalUpNeighbor;
	};

	template<class BoxType>
	inline bool cellHasWater(const BoxType* reader, const Cell& cell,
		const Vector3int16& globalCoord);
	template<class BoxType>
	Cell interpretAsWaterCell(const BoxType* reader, const Cell& cell,
		const Vector3int16& globalCoord);
}

} }

#include "Voxel/Water.inl"
