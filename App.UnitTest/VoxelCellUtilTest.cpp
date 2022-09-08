#include <boost/test/unit_test.hpp>

#include "Voxel/Util.h"
#include "VoxelCellUtilV1Deprecated.h"
#include "V8DataModel/MegaCluster.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( VoxelCellUtilTest )

BOOST_AUTO_TEST_CASE( WorldToCell ) {
    const int kOffset = 0;

	const float kEpsilon = 1e-3f;
	for (int y = -1; y < 2; y += 2) {
		for (int z = -1; z < 2; z += 2) {
			for (int x = -1; x < 2; x += 2) {
				BOOST_CHECK_EQUAL(Vector3int16(
						kOffset + (x - 1) / 2,
						(y - 1) / 2,
						kOffset + (z - 1) / 2),
					worldToCell_floor(kEpsilon * Vector3(x,y,z)));
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( CellToWorld ) {
	shared_ptr<MegaClusterInstance> mc = Creatable<Instance>::create<MegaClusterInstance>();
	mc->setCoordinateFrame(CoordinateFrame()); // dummy to make sure coordinate frame is initialzied
	for (int y = -5; y < 4; ++y) {
		for (int z = -5; z < 4; ++z) {
			for (int x = -5; x < 4; ++x) {
				const Vector3int16 cell(x,y,z);
				Extents mcExtents = mc->cellToWorldExtents(cell);
				Extents newExtents = Extents(cellToWorld_smallestCorner(cell),
					cellToWorld_smallestCorner(cell) + Vector3(kCELL_SIZE, kCELL_SIZE, kCELL_SIZE));
				BOOST_CHECK_EQUAL(mcExtents.min(), newExtents.min());
				BOOST_CHECK_EQUAL(mcExtents.max(), newExtents.max());
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( WriteOneFormatReadOther ) {
	BOOST_REQUIRE(CELL_MATERIAL_Deprecated_Empty == (CellMaterial)0);
	BOOST_REQUIRE(CELL_BLOCK_Solid == (CellBlock)0);
	BOOST_REQUIRE(CELL_ORIENTATION_NegZ == (CellOrientation)0);

	for (int m = CELL_MATERIAL_Deprecated_Empty; m < (CellMaterial)8; ++m) {
		for (int b = CELL_BLOCK_Solid; b < MAX_CELL_BLOCKS; ++b) {
			for (int o = CELL_ORIENTATION_NegZ; o < MAX_CELL_ORIENTATIONS; ++o) {
				// write old, read new
				{
					unsigned char oldCell;
					V1Deprecated::setCellMaterial_Deprecated(oldCell, (CellMaterial)m);
					V1Deprecated::setCellBlock(oldCell, (CellBlock)b);
					V1Deprecated::setCellOrientation(oldCell, (CellOrientation)o);

					Cell v = Cell::readUnsignedCharFromDeprecatedUse(oldCell);
					BOOST_REQUIRE(v.solid.getBlock() == b);
					BOOST_REQUIRE(v.solid.getOrientation() == o);
				}

				// write new, read old
				{
					Cell v;
					v.solid.setBlock((CellBlock)b);
					v.solid.setOrientation((CellOrientation)o);

					unsigned char oldCell = Cell::asUnsignedCharForDeprecatedUses(v);
					BOOST_REQUIRE(V1Deprecated::getCellMaterial_Deprecated(oldCell) == CELL_MATERIAL_Deprecated_Empty);
					BOOST_REQUIRE(V1Deprecated::getCellBlock(oldCell) == b);
					BOOST_REQUIRE(V1Deprecated::getCellOrientation(oldCell) == o);
				}
			}
		}
	}

	BOOST_REQUIRE(WATER_CELL_FORCE_None == (WaterCellForce)0);
	BOOST_REQUIRE(WATER_CELL_DIRECTION_NegX == (WaterCellDirection)0);
	for (int f = WATER_CELL_FORCE_None; f < MAX_WATER_CELL_FORCES; ++f) {
		for (int d = WATER_CELL_DIRECTION_NegX; d < MAX_WATER_CELL_DIRECTIONS; ++d) {
			{
				unsigned char oldCell;
				V1Deprecated::setCellBlock(oldCell, CELL_BLOCK_Empty);
				V1Deprecated::setWaterForceAndDirection(oldCell, (WaterCellForce)f, (WaterCellDirection)d);

				Cell v = Cell::readUnsignedCharFromDeprecatedUse(oldCell);
				BOOST_REQUIRE(v.isExplicitWaterCell());
				BOOST_REQUIRE(v.solid.getBlock() == CELL_BLOCK_Empty);
				BOOST_REQUIRE(v.water.getForce() == f);
				BOOST_REQUIRE(v.water.getDirection() == d);
			}

			{
				Cell v;
				v.solid.setBlock(CELL_BLOCK_Empty);
				v.water.setForceAndDirection((WaterCellForce)f, (WaterCellDirection)d);

				unsigned char oldCell = Cell::asUnsignedCharForDeprecatedUses(v);
				BOOST_REQUIRE(V1Deprecated::isExplicitWaterCell(oldCell));
				BOOST_REQUIRE(V1Deprecated::getCellBlock(oldCell) == CELL_BLOCK_Empty);
				BOOST_REQUIRE(V1Deprecated::getWaterCellForce(oldCell) == f);
				BOOST_REQUIRE(V1Deprecated::getWaterCellDirection(oldCell) == d);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(ConstantVoxelsMatchDeprecatedVersion) {
	BOOST_REQUIRE(Cell::asUnsignedCharForDeprecatedUses(Constants::kUniqueEmptyCellRepresentation) == V1Deprecated::UNIQUE_EMPTY_CELL_REPRESENTATION);
	BOOST_REQUIRE(Constants::kUniqueEmptyCellRepresentation == Cell::readUnsignedCharFromDeprecatedUse(V1Deprecated::UNIQUE_EMPTY_CELL_REPRESENTATION));

	BOOST_REQUIRE(Cell::asUnsignedCharForDeprecatedUses(Constants::kWaterOnWedgeCell) == V1Deprecated::WATER_ON_WEDGE_CELL);
	BOOST_REQUIRE(Constants::kWaterOnWedgeCell == Cell::readUnsignedCharFromDeprecatedUse(V1Deprecated::WATER_ON_WEDGE_CELL));
}

BOOST_AUTO_TEST_CASE(BlockFaceInfoIsEquivalent) {
	V1Deprecated::initBlockOrientationFaceMap();
	RBX::initBlockOrientationFaceMap();

	// check that struct sizes are same
	BOOST_REQUIRE(sizeof(RBX::BlockFaceInfo) == sizeof(V1Deprecated::BlockFaceInfo));
	BOOST_REQUIRE(sizeof(RBX::BlockAxisFace) == sizeof(V1Deprecated::BlockAxisFace));

	// check that pointers are different
	BOOST_REQUIRE((void*)V1Deprecated::UnOrientedBlockFaceInfos != (void*)RBX::UnOrientedBlockFaceInfos);
	BOOST_REQUIRE((void*)V1Deprecated::OrientedFaceMap != (void*)RBX::OrientedFaceMap);

	// check that pointer contents is the same
	BOOST_REQUIRE(memcmp(V1Deprecated::UnOrientedBlockFaceInfos,
		RBX::UnOrientedBlockFaceInfos, 6 * sizeof(BlockFaceInfo)) == 0);
	BOOST_REQUIRE(memcmp(V1Deprecated::OrientedFaceMap, RBX::OrientedFaceMap,
		1536 * sizeof(BlockAxisFace)) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
