#include <boost/test/unit_test.hpp>

#include "Client.h"
#include "rbx/test/DataModelFixture.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Explosion.h"
#include "V8DataModel/MegaCluster.h"

using namespace RBX;
using namespace RBX::Voxel;

struct AutowedgeTestFixture {
	static const Vector3int16 kStartingBlockMin, kStartingBlockMax;
	static const Region3int16 kStartingBlockRegion;

	DataModelFixture dm;
	Workspace* workspace;
	MegaClusterInstance* cluster;
	AutowedgeTestFixture() {
		DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);
		workspace = dm->getWorkspace();
		workspace->createTerrain();
		cluster = Instance::fastDynamicCast<MegaClusterInstance>(
			workspace->getTerrain());
		Cell solidVoxel;
		solidVoxel.solid.setBlock(CELL_BLOCK_Solid);
		solidVoxel.solid.setOrientation(CELL_ORIENTATION_NegZ);
		
		// set one cell so that the cluster is allocated
		cluster->getVoxelGrid()->setCell(Vector3int16(0, 0, 0),
			solidVoxel, CELL_MATERIAL_Grass);
		cluster->getVoxelGrid()->setCell(Vector3int16(0, 0, 0),
			Constants::kUniqueEmptyCellRepresentation, CELL_MATERIAL_Water);
	}
};

const Vector3int16 AutowedgeTestFixture::kStartingBlockMin(-2,16,-2);
const Vector3int16 AutowedgeTestFixture::kStartingBlockMax(2,17,2);
const Region3int16 AutowedgeTestFixture::kStartingBlockRegion(AutowedgeTestFixture::kStartingBlockMin, AutowedgeTestFixture::kStartingBlockMax);

static inline Cell createCell(CellBlock cellBlock, CellOrientation cellOrientation) {
	Cell result;
	result.solid.setBlock(cellBlock);
	result.solid.setOrientation(cellOrientation);
	return result;
}

static void createStartingBlock(AutowedgeTestFixture& fixture) {
	fixture.cluster->setCellsScript(
		AutowedgeTestFixture::kStartingBlockRegion,
		CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
}

static void autowedgeStartingBlock(AutowedgeTestFixture& fixture) {
	fixture.cluster->autoWedgeCellsScript(
		AutowedgeTestFixture::kStartingBlockRegion);
}

typedef std::vector<std::vector<std::vector<Cell> > > ExpectedResult;

static void checkExpectedResult(const char* message,
	const ExpectedResult& expected,
	const AutowedgeTestFixture& actualSource) {
    const int kOffset = 0;

	const Vector3int16& min = AutowedgeTestFixture::kStartingBlockMin;
	Vector3int16 delta = AutowedgeTestFixture::kStartingBlockMax - min;

	int xTravel = 1 + delta.x;
	int yTravel = 1 + delta.y;
	int zTravel = 1 + delta.z;

	for (int x = 0; x < xTravel; ++x) {
		for (int y = 0; y < yTravel; ++y) {
			for (int z = 0; z < zTravel; ++z) {
				Cell actual = actualSource.cluster->getVoxelGrid()->getCell(min + Vector3int16(x + kOffset, y, z + kOffset));
				char messageBuffer[256];
				snprintf(messageBuffer, 256,
					"%s @ (%d,%d,%d): actual != expected %d != %d",
					message, min.x + x, min.y + y, min.z + z,
					Cell::asUnsignedCharForDeprecatedUses(actual),
					Cell::asUnsignedCharForDeprecatedUses(expected[x][y][z]));
				BOOST_CHECK_MESSAGE(actual == expected[x][y][z], messageBuffer);
			}
		}
	}
}

static void initializeExpectedResult(ExpectedResult& out) {
	out.clear();

	Vector3int16 delta = AutowedgeTestFixture::kStartingBlockMax -
		AutowedgeTestFixture::kStartingBlockMin;

	int xTravel = 1 + delta.x;
	int yTravel = 1 + delta.y;
	int zTravel = 1 + delta.z;

	out.resize(xTravel);
	for (int x = 0; x < xTravel; ++x) {
		out[x].resize(yTravel);
		for (int y = 0; y < yTravel; ++y) {
			out[x][y].resize(zTravel);
			for (int z = 0; z < zTravel; ++z) {
				out[x][y][z] = createCell(CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
			}
		}
	}
}

static void applyStandardExteriorWedging(ExpectedResult& out) {
	out[0][0][0] = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_NegX);
	out[0][1][0] = createCell(CELL_BLOCK_CornerWedge,     CELL_ORIENTATION_NegX);

	out[0][0][4] = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_NegZ);
	out[0][1][4] = createCell(CELL_BLOCK_CornerWedge,     CELL_ORIENTATION_NegZ);

	out[4][0][0] = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_Z);
	out[4][1][0] = createCell(CELL_BLOCK_CornerWedge,     CELL_ORIENTATION_Z);

	out[4][0][4] = createCell(CELL_BLOCK_HorizontalWedge, CELL_ORIENTATION_X);
	out[4][1][4] = createCell(CELL_BLOCK_CornerWedge,     CELL_ORIENTATION_X);

	for (int x = 1; x <= 3; ++x) {
		out[x][1][4] = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_NegZ);
		out[x][1][0] = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_Z);
		out[0][1][x] = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_NegX);
		out[4][1][x] = createCell(CELL_BLOCK_VerticalWedge, CELL_ORIENTATION_X);
	}
}

static void fourCellSquareTest(const Vector3int16& minCorner) {
    const int kOffset = 0;

	const int minX = minCorner.x;
	const int minY = minCorner.y;
	const int minZ = minCorner.z;

	AutowedgeTestFixture fixture;

	fixture.cluster->setCellScript(minX + 1, minY, minZ + 1,
		CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	fixture.cluster->setCellScript(minX + 1, minY, minZ,
		CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	fixture.cluster->setCellScript(minX,     minY, minZ + 1,
		CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	fixture.cluster->setCellScript(minX,     minY, minZ,
		CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);

	const Vector3int16 minExt(minCorner - Vector3int16(10,10,10));
	const Vector3int16 maxExt(minCorner + Vector3int16(10,10,10));
	fixture.cluster->autoWedgeCellsScript(Region3int16(minExt, maxExt));
	Cell expectedCells[4] = {
		createCell(CELL_BLOCK_CornerWedge, CELL_ORIENTATION_NegX),
		createCell(CELL_BLOCK_CornerWedge, CELL_ORIENTATION_Z),
		createCell(CELL_BLOCK_CornerWedge, CELL_ORIENTATION_NegZ),
		createCell(CELL_BLOCK_CornerWedge, CELL_ORIENTATION_X)
	};

	for (int y = minExt.y; y <= maxExt.y; ++y) {
		for (int z = minExt.z; z <= maxExt.z; ++z) {
			for (int x = minExt.x; x <= maxExt.x; ++x) {
				if (x < minX || x > minX + 1 || z < minZ || z > minZ + 1 || y < minY || y > minY) {
					BOOST_CHECK_EQUAL(CELL_MATERIAL_Deprecated_Empty,
						fixture.cluster->getCellScript(x, y, z)->at(0).cast<CellMaterial>());
				} else {
					int index = (x - minX) + 2 * (z - minZ);
					BOOST_REQUIRE_LE(index, 3);
					BOOST_REQUIRE_GE(index, 0);
					Cell expected = expectedCells[index];
					Cell actual = fixture.cluster->getVoxelGrid()->getCell(Vector3int16(x + kOffset, y, z + kOffset));
					char messageBuffer[256];
					snprintf(messageBuffer, 256, "(%d,%d,%d) %d != %d", x, y, z,
						Cell::asUnsignedCharForDeprecatedUses(expected),
						Cell::asUnsignedCharForDeprecatedUses(actual));
					BOOST_CHECK_MESSAGE(expected == actual, messageBuffer);
				}
			}
		}
	}
}

BOOST_AUTO_TEST_SUITE( AutowedgeTest )

BOOST_AUTO_TEST_CASE( AutowedgeStartingBlock )
{
	AutowedgeTestFixture fixture;
	createStartingBlock(fixture);
	
	ExpectedResult result;
	initializeExpectedResult(result);
	checkExpectedResult("unwedged", result, fixture);

	autowedgeStartingBlock(fixture);
	initializeExpectedResult(result);
	applyStandardExteriorWedging(result);
	checkExpectedResult("wedged", result, fixture);
}

BOOST_AUTO_TEST_CASE( RemoveTopCenter ) {

	AutowedgeTestFixture fixture;
	createStartingBlock(fixture);
	fixture.cluster->setCellScript(0, 17, 0, CELL_MATERIAL_Deprecated_Empty, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	
	autowedgeStartingBlock(fixture);

	ExpectedResult result;
	initializeExpectedResult(result);
	applyStandardExteriorWedging(result);
	result[2][1][2] = Constants::kUniqueEmptyCellRepresentation;
	
	// get the vertical wedges from the outside for the sides of the hole
	result[1][1][2] = result[4][1][2];
	result[3][1][2] = result[0][1][2];
	result[2][1][1] = result[2][1][4];
	result[2][1][3] = result[2][1][0];

	// get the corners of the interior hole
	result[1][1][1] = createCell(CELL_BLOCK_InverseCornerWedge, CELL_ORIENTATION_X);
	result[1][1][3] = createCell(CELL_BLOCK_InverseCornerWedge, CELL_ORIENTATION_Z);
	result[3][1][3] = createCell(CELL_BLOCK_InverseCornerWedge, CELL_ORIENTATION_NegX);
	result[3][1][1] = createCell(CELL_BLOCK_InverseCornerWedge, CELL_ORIENTATION_NegZ);

	checkExpectedResult("wedged", result, fixture);
}

BOOST_AUTO_TEST_CASE( CornerOfTerrainWorks ) {
	// sanity check: autowedge span entirely inside terrain
	fourCellSquareTest(Vector3int16(20, 20, 20));

	// max terrain extent
	fourCellSquareTest(Vector3int16(253, 62, 253));
	
	// min terrain extent
	fourCellSquareTest(Vector3int16(-256, 0, -256));
}

BOOST_AUTO_TEST_CASE( BelowTerrainWorks ) {
	bool check = true;
	AutowedgeTestFixture fixture;
	// ensure that this call doesn't crash
	fixture.cluster->autoWedgeCellsScript(Region3int16(Vector3int16(-5,-20,-5), Vector3int16(5,-10,5)));
	BOOST_REQUIRE(check);
}

BOOST_AUTO_TEST_CASE( ExplosionBelowTerrainWorks ) {
    const int kOffset = 0;

	bool check = true;
	AutowedgeTestFixture fixture;
	DataModel::LegacyLock lock(&fixture.dm, RBX::DataModelJob::Write);

	shared_ptr<Explosion> exp = Creatable<Instance>::create<Explosion>();
	exp->setBlastRadius(10.f);
	Explosion::propPosition.setValue(exp.get(), Vector3(0,2,0));
	fixture.cluster->setCellScript(0, 0, 0, CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	BOOST_REQUIRE_EQUAL(fixture.cluster->getVoxelGrid()->getCellMaterial(Vector3int16(kOffset, 0, kOffset)), CELL_MATERIAL_Grass);

	Stepped stepped(100, 101, false);
	exp->setParent(fixture.workspace);
	// DE4979: explosions that extend below the surface used to cause a crash.
	// This line triggers an explosion that extends beneath the bottom surface
	// of the terrain. This function shouldn't crash if the bug is fixed.
	ServiceProvider::create<RunService>(fixture.workspace)->steppedSignal(stepped);
	BOOST_REQUIRE(check);
	BOOST_REQUIRE_EQUAL(fixture.cluster->getVoxelGrid()->getCellMaterial(Vector3int16(kOffset, 0, kOffset)), CELL_MATERIAL_Water);
}

BOOST_AUTO_TEST_CASE( ExplosionAboveTerrainWorks ) {
    const int kOffset = 0;

	bool check = true;
	AutowedgeTestFixture fixture;
	DataModel::LegacyLock lock(&fixture.dm, RBX::DataModelJob::Write);

	shared_ptr<Explosion> exp = Creatable<Instance>::create<Explosion>();
	exp->setBlastRadius(20.f);
	Explosion::propPosition.setValue(exp.get(), Vector3(0,240,0));
	fixture.cluster->setCellScript(0, 62, 0, CELL_MATERIAL_Grass, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	BOOST_REQUIRE_EQUAL(fixture.cluster->getVoxelGrid()->getCellMaterial(Vector3int16(kOffset, 62, kOffset)), CELL_MATERIAL_Grass);

	Stepped stepped(100, 101, false);
	exp->setParent(fixture.workspace);
	// DE4979: explosions that extend above the surface used to cause a crash.
	// This line triggers an explosion that extends above the top surface
	// of the terrain. This function shouldn't crash if the bug is fixed.
	ServiceProvider::create<RunService>(fixture.workspace)->steppedSignal(stepped);
	BOOST_REQUIRE(check);
	BOOST_REQUIRE_EQUAL(fixture.cluster->getVoxelGrid()->getCellMaterial(Vector3int16(kOffset, 62, kOffset)), CELL_MATERIAL_Water);
}


BOOST_AUTO_TEST_SUITE_END()
