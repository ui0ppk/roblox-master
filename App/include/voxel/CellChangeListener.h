#pragma once

#include "Voxel/Cell.h"

namespace RBX {

namespace Voxel {

struct CellChangeInfo {
	const Vector3int16 position;

	Cell beforeCell;
	Cell afterCell;
	bool hadWaterBefore;
	bool hasWaterAfter;
	CellMaterial afterMaterial;

	CellChangeInfo(const Vector3int16& position,
		Cell beforeCell, Cell afterCell,
		bool hadWaterBefore, bool hasWaterAfter,
		CellMaterial afterMaterial)
		: position(position)
		, beforeCell(beforeCell)
		, afterCell(afterCell)
		, hadWaterBefore(hadWaterBefore)
		, hasWaterAfter(hasWaterAfter)
		, afterMaterial(afterMaterial)
	{ }
};

// Callback interface for components that want to be notified when terrain
// cells change
class CellChangeListener {
public:
	virtual void terrainCellChanged(const CellChangeInfo& info) = 0;
};

} }
