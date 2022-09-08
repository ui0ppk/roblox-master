#pragma once

#include "v8datamodel/PartInstance.h"

#include "voxel/Grid.h"
#include "voxel2/Grid.h"

namespace RBX { namespace Voxel2 { namespace Conversion {

	static const int kOccupancySolid = Cell::Occupancy_Max;
	static const int kOccupancyWedge = Cell::Occupancy_Max / 2;
	static const int kOccupancyCorner = Cell::Occupancy_Max / 3;
	static const int kOccupancyInverseCorner = Cell::Occupancy_Max * 2 / 3;

	static const PartMaterial kMaterialTable[] =
	{
		AIR_MATERIAL,
		WATER_MATERIAL,
		GRASS_MATERIAL,
		SLATE_MATERIAL,
		CONCRETE_MATERIAL,
		BRICK_MATERIAL,
		SAND_MATERIAL,
		WOODPLANKS_MATERIAL,
		ROCK_MATERIAL,
		GLACIER_MATERIAL,
		SNOW_MATERIAL,
		SANDSTONE_MATERIAL,
		MUD_MATERIAL,
		BASALT_MATERIAL,
		GROUND_MATERIAL,
		CRACKED_LAVA_MATERIAL,
	};

    static const int kMaterialDefault = 2;

	inline unsigned char getOccupancyFromSolidBlock(Voxel::CellBlock block)
	{
		switch (block)
		{
		case Voxel::CELL_BLOCK_Solid:
			return kOccupancySolid;

		case Voxel::CELL_BLOCK_VerticalWedge:
		case Voxel::CELL_BLOCK_HorizontalWedge:
			return kOccupancyWedge;

		case Voxel::CELL_BLOCK_CornerWedge:
			return kOccupancyCorner;

		case Voxel::CELL_BLOCK_InverseCornerWedge:
			return kOccupancyInverseCorner;

		default:
			RBXASSERT(false);
			return 0;
		}
	}

	inline Voxel::CellBlock getCellBlockFromCell(const Cell& cell)
	{
		static const int kOccupancyRounder = Cell::Occupancy_Max / 6;

		if (cell.getMaterial() == Cell::Material_Air)
			return Voxel::CELL_BLOCK_Empty;
		else if (cell.getOccupancy() < kOccupancyCorner - kOccupancyRounder)
			return Voxel::CELL_BLOCK_Empty;
		else if (cell.getOccupancy() < kOccupancyWedge - kOccupancyRounder)
			return Voxel::CELL_BLOCK_CornerWedge;
		else if (cell.getOccupancy() < kOccupancyInverseCorner - kOccupancyRounder)
			return Voxel::CELL_BLOCK_VerticalWedge;
		else if (cell.getOccupancy() < kOccupancySolid - kOccupancyRounder)
			return Voxel::CELL_BLOCK_InverseCornerWedge;
		else
			return Voxel::CELL_BLOCK_Solid;
	}

	inline PartMaterial getMaterialFromVoxelMaterial(unsigned char material)
	{
		if (static_cast<size_t>(material) < sizeof(kMaterialTable) / sizeof(kMaterialTable[0]))
			return kMaterialTable[material];
		else
			return kMaterialTable[kMaterialDefault];
	}

	inline unsigned char getVoxelMaterialFromMaterial(PartMaterial material)
	{
		for (size_t i = 0; i < sizeof(kMaterialTable) / sizeof(kMaterialTable[0]); ++i)
			if (kMaterialTable[i] == material)
				return i;

		return kMaterialDefault;
	}

	inline PartMaterial getMaterialFromCellMaterial(Voxel::CellMaterial material)
	{
		switch (material)
		{
		case Voxel::CELL_MATERIAL_Deprecated_Empty:
			return AIR_MATERIAL;
		case Voxel::CELL_MATERIAL_Grass:
			return GRASS_MATERIAL;
		case Voxel::CELL_MATERIAL_Sand:
			return SAND_MATERIAL;
		case Voxel::CELL_MATERIAL_Brick:
			return BRICK_MATERIAL;
		case Voxel::CELL_MATERIAL_Granite:
			return SLATE_MATERIAL;
		case Voxel::CELL_MATERIAL_Asphalt:
			return CONCRETE_MATERIAL;
		case Voxel::CELL_MATERIAL_Wood_Plank:
		case Voxel::CELL_MATERIAL_Wood_Log:
			return WOODPLANKS_MATERIAL;
		case Voxel::CELL_MATERIAL_Gravel:
			return SLATE_MATERIAL;
		case Voxel::CELL_MATERIAL_Cinder_Block:
			return CONCRETE_MATERIAL;
		case Voxel::CELL_MATERIAL_Stone_Block:
			return SLATE_MATERIAL;
		case Voxel::CELL_MATERIAL_Cement:
			return CONCRETE_MATERIAL;
		case Voxel::CELL_MATERIAL_Water:
			return WATER_MATERIAL;
		default:
			return kMaterialTable[kMaterialDefault];
		}
	}

	inline Voxel::CellMaterial getCellMaterialFromMaterial(PartMaterial material)
	{
		switch (material)
		{
		case AIR_MATERIAL:
			return Voxel::CELL_MATERIAL_Deprecated_Empty;
		case WATER_MATERIAL:
			return Voxel::CELL_MATERIAL_Water;
		case GRASS_MATERIAL:
			return Voxel::CELL_MATERIAL_Grass;
		case SLATE_MATERIAL:
			return Voxel::CELL_MATERIAL_Stone_Block;
		case CONCRETE_MATERIAL:
			return Voxel::CELL_MATERIAL_Cement;
		case BRICK_MATERIAL:
			return Voxel::CELL_MATERIAL_Brick;
		case SAND_MATERIAL:
			return Voxel::CELL_MATERIAL_Sand;
		case WOODPLANKS_MATERIAL:
			return Voxel::CELL_MATERIAL_Wood_Plank;
		default:
			return Voxel::CELL_MATERIAL_Grass;
		}
	}

	inline void convertToSmooth(const Voxel::Grid& oldGrid, Voxel2::Grid& grid)
	{
		std::vector<SpatialRegion::Id> chunks = oldGrid.getNonEmptyChunks();

		for (size_t i = 0; i < chunks.size(); ++i)
		{
			Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(chunks[i]);
			Voxel::Grid::Region region = oldGrid.getRegion(extents.getMinPos(), extents.getMaxPos());

			Voxel2::Box box(Voxel::kXZ_CHUNK_SIZE, Voxel::kY_CHUNK_SIZE, Voxel::kXZ_CHUNK_SIZE);

			for (int y = 0; y < Voxel::kY_CHUNK_SIZE; ++y)
			for (int z = 0; z < Voxel::kXZ_CHUNK_SIZE; ++z)
			for (int x = 0; x < Voxel::kXZ_CHUNK_SIZE; ++x)
			{
				Vector3int16 cpos = extents.getMinPos() + Vector3int16(x, y, z);
				const Voxel::Cell& oldCell = region.voxelAt(cpos);
				const Voxel::CellMaterial& oldMaterial = region.materialAt(cpos);

				if (!oldCell.isEmpty())
				{
					using namespace Voxel2::Conversion;

                    Voxel2::Cell cell;

					if (oldCell.isExplicitWaterCell())
						cell = Voxel2::Cell(Voxel2::Cell::Material_Water, Voxel2::Cell::Occupancy_Max);
					else
						cell = Voxel2::Cell(getVoxelMaterialFromMaterial(getMaterialFromCellMaterial(oldMaterial)), getOccupancyFromSolidBlock(oldCell.solid.getBlock()));

                    box.set(x, y, z, cell);
				}
			}

			grid.write(Voxel2::Region(Vector3int32(extents.getMinPos()), Vector3int32(extents.getMaxPos() + Vector3int16(1, 1, 1))), box);
		}
	}

} } }
