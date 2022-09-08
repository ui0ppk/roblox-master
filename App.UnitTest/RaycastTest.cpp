#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/Workspace.h"

using namespace RBX;
using namespace RBX::Voxel;

struct RaycastTestFixture {
	DataModelFixture dm;
	Workspace* workspace;
	MegaClusterInstance* cluster;
	RaycastTestFixture() {
		RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);
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

// This needs to be a macro to make usable errors
static void checkRaycastResult(const char* message,
	const Instance* hitInstance, const Vector3& hitPoint,
	shared_ptr<const Reflection::Tuple>& actual) {

	Instance* actualHitInstance =
		actual->values[0].cast<shared_ptr<Instance> >().get();
	char tmp[150];
	snprintf(tmp, 150, "%s: Instance %p != %p", message, hitInstance,
		actualHitInstance);

	BOOST_REQUIRE(actual->values[0].isType<shared_ptr<Instance> >());
	BOOST_CHECK_MESSAGE(hitInstance == actualHitInstance, tmp);

	Vector3 actualHitPoint = actual->values[1].cast<Vector3>();
	snprintf(tmp, 150, "%s: hit pt [%5.2f,%5.2f,%5.2f] != [%5.2f,%5.2f,%5.2f]",
		message, hitPoint.x, hitPoint.y, hitPoint.z,
		actualHitPoint.x, actualHitPoint.y, actualHitPoint.z);
	BOOST_REQUIRE(actual->values[1].isType<Vector3>());
	BOOST_CHECK_MESSAGE(hitPoint == actualHitPoint, tmp);
}

BOOST_AUTO_TEST_SUITE( RaycastTest )

BOOST_AUTO_TEST_CASE( CheckInvalidFloatValue ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	volatile float zero = 0;
	Vector3 vectorPermutations[4] = {
		Vector3(0,1,0), // valid
		Vector3(1/zero,0,0), // invalid
		Vector3(0,-1/zero,0), // invalid
		Vector3(0,0,zero/zero), // invalid
	};
	
	// make sure none of these cause the system to horf.
	for (unsigned int i = 0; i < 4; ++i) {
		for (unsigned int j = 0; j < 4; ++j) {
			dm.workspace->getRayHit<Instance>(
				RbxRay(vectorPermutations[i], vectorPermutations[j]),
				shared_ptr<Instance>());
		}
	}
}

BOOST_AUTO_TEST_CASE( OriginCloseToEntryPoint ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3 rayStart(0,-1e-7f,0);
	BOOST_REQUIRE(Math::fuzzyEq(rayStart, Vector3(0,0,0)));

	// check that this doesn't crash
	dm.workspace->getRayHit<Instance>(RbxRay(rayStart, Vector3(0,3,0)), shared_ptr<Instance>());
}

BOOST_AUTO_TEST_CASE( CheckClusterBoundaryEmptyCluster ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 positionOutsideCluster(-1, -1, -1);
	Vector3int16 positionInsideCluster(4, 4, 4);

	Cell cell = dm.cluster->getVoxelGrid()->getCell(positionOutsideCluster);
	cell = dm.cluster->getVoxelGrid()->getCell(positionInsideCluster);
	
	Vector3 locationOutsideCluster =
		dm.cluster->cellToWorldExtents(positionOutsideCluster).min();
	Vector3 locationInsideCluster =
		dm.cluster->cellToWorldExtents(positionInsideCluster).max();

	// outside => inside
	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(locationOutsideCluster, locationInsideCluster - locationOutsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("outside => inside", NULL, locationInsideCluster, rayHit);

	// inside => outside
	rayHit = dm.workspace->getRayHit<Instance>(
			RbxRay(locationInsideCluster, locationOutsideCluster - locationInsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("inside => outside", NULL, locationOutsideCluster, rayHit);
}

BOOST_AUTO_TEST_CASE( CheckClusterBoundaryWithWater ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 positionOutsideCluster(-1, -1, -1);
	Vector3int16 positionInsideCluster(4, 4, 4);
	Vector3int16 waterPosition(0, 0, 0);

	dm.cluster->getVoxelGrid()->setCell(waterPosition, Constants::kWaterOnWedgeCell,
		CELL_MATERIAL_Water);

	Cell cell = dm.cluster->getVoxelGrid()->getCell(positionOutsideCluster);
	cell = dm.cluster->getVoxelGrid()->getCell(positionInsideCluster);
	
	Vector3 locationOutsideCluster =
		dm.cluster->cellToWorldExtents(positionOutsideCluster).min();
	Vector3 locationInsideCluster =
		dm.cluster->cellToWorldExtents(positionInsideCluster).max();
	Extents waterCellExtents =
		dm.cluster->cellToWorldExtents(waterPosition);

	// outside => inside
	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(locationOutsideCluster, locationInsideCluster - locationOutsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("outside => inside", dm.cluster, waterCellExtents.min(), rayHit);

	// inside => outside
	rayHit = dm.workspace->getRayHit<Instance>(
			RbxRay(locationInsideCluster, locationOutsideCluster - locationInsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("inside => outside", dm.cluster, waterCellExtents.max(), rayHit);
}

BOOST_AUTO_TEST_CASE( CheckClusterBoundaryWithWaterThroughToOrigin ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 positionOutsideCluster(-1, -1, -1);
	Vector3int16 positionInsideCluster(4, 4, 4);

	Vector3int16 waterPosition;
	for (waterPosition.y = 0; waterPosition.y <= 5; ++waterPosition.y) {
		for (waterPosition.z = 0; waterPosition.z <= 5; ++waterPosition.z) {
			for (waterPosition.x = 0; waterPosition.x <= 5; ++waterPosition.x) {
				dm.cluster->getVoxelGrid()->setCell(waterPosition, 
					Constants::kWaterOnWedgeCell,
					CELL_MATERIAL_Water);
			}
		}
	}

	Cell cell = dm.cluster->getVoxelGrid()->getCell(positionOutsideCluster);
	cell = dm.cluster->getVoxelGrid()->getCell(positionInsideCluster);
	
	Vector3 locationOutsideCluster =
		dm.cluster->cellToWorldExtents(positionOutsideCluster).min();
	Vector3 locationInsideCluster =
		dm.cluster->cellToWorldExtents(positionInsideCluster).max();
	Vector3 waterClusterBoundary =
		dm.cluster->cellToWorldExtents(Vector3int16(0, 0, 0)).min();

	// outside => inside
	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(locationOutsideCluster, locationInsideCluster - locationOutsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("outside => inside", dm.cluster, waterClusterBoundary, rayHit);

	// inside => outside
	rayHit = dm.workspace->getRayHit<Instance>(
			RbxRay(locationInsideCluster, locationOutsideCluster - locationInsideCluster),
			shared_ptr<Instance>());
	checkRaycastResult("inside => outside", dm.cluster, waterClusterBoundary, rayHit);
}

BOOST_AUTO_TEST_CASE( StartOutsideWaterHitsWaterFace ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 startPosition(4, 4, 4);
	Vector3int16 waterPosition(6, 4, 4);
	Vector3int16 endSearchPosition(8, 4, 4);

	dm.cluster->getVoxelGrid()->setCell(waterPosition, Constants::kWaterOnWedgeCell,
		CELL_MATERIAL_Water);

	Vector3 start = dm.cluster->cellToWorldExtents(startPosition).center();
	Extents waterExt = dm.cluster->cellToWorldExtents(waterPosition);
	Vector3 end = dm.cluster->cellToWorldExtents(endSearchPosition).center();

	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("+X ray", dm.cluster,
		Vector3(waterExt.min().x, waterExt.center().y, waterExt.center().z),
		rayHit);
}

BOOST_AUTO_TEST_CASE( StartInsideWaterHitsWaterAirInterface ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 startPosition(4, 4, 4);
	Vector3int16 waterPosition(6, 4, 4);
	Vector3int16 endSearchPosition(8, 4, 4);

	for (int x = 4; x < 7; ++x) {
		dm.cluster->getVoxelGrid()->setCell(Vector3int16(x, 4, 4),
			Constants::kWaterOnWedgeCell, CELL_MATERIAL_Water);
	}
	
	Vector3 start = dm.cluster->cellToWorldExtents(startPosition).center();
	Extents waterExt = dm.cluster->cellToWorldExtents(waterPosition);
	Vector3 end = dm.cluster->cellToWorldExtents(endSearchPosition).center();

	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("+X ray", dm.cluster,
		Vector3(waterExt.max().x, waterExt.center().y, waterExt.center().z),
		rayHit);
}

BOOST_AUTO_TEST_CASE( StartInsideWaterHitsWaterBlockInterface ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Vector3int16 startPosition(4, 4, 4);
	Vector3int16 waterPosition(6, 4, 4);
	Vector3int16 endSearchPosition(8, 4, 4);

	for (int x = 4; x < 6; ++x) {
		dm.cluster->getVoxelGrid()->setCell(Vector3int16(x, 4, 4),
			Constants::kWaterOnWedgeCell, CELL_MATERIAL_Water);
	}

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);
	dm.cluster->getVoxelGrid()->setCell(waterPosition, cell, CELL_MATERIAL_Grass);
	
	Vector3 start = dm.cluster->cellToWorldExtents(startPosition).center();
	Extents waterExt = dm.cluster->cellToWorldExtents(waterPosition);
	Vector3 end = dm.cluster->cellToWorldExtents(endSearchPosition).center();

	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("+X ray", dm.cluster,
		Vector3(waterExt.min().x, waterExt.center().y, waterExt.center().z),
		rayHit);
}

BOOST_AUTO_TEST_CASE( HitsWedgeInWaterWedgeAsAppropriate ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	// diagram (left is -X, right is +X):
	// [Water] [Water-Wedge] [Water] [Air]
	// start in leftmost cell, cast two rays: one that hits the wedge in
	// the water-wedge cell, and one that misses and hits the water-air
	// interface

	Vector3int16 startPosition(4, 4, 4);
	Vector3int16 endSearchPosition(8, 4, 4);
	Vector3int16 wedgePosition(6, 4, 4);
	Vector3int16 endWaterPosition(7, 4, 4);

	for (int x = 4; x <= endWaterPosition.x; ++x) {
		dm.cluster->getVoxelGrid()->setCell(Vector3int16(x, 4, 4),
			Constants::kWaterOnWedgeCell, CELL_MATERIAL_Water);
	}
	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_VerticalWedge);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);
	dm.cluster->getVoxelGrid()->setCell(wedgePosition, cell,
		CELL_MATERIAL_Grass);
	// add water cell in front of wedge to force wedge to be a 
	// waster wedge
	dm.cluster->getVoxelGrid()->setCell(wedgePosition + Vector3int16(0,0,1),
		Constants::kWaterOnWedgeCell, CELL_MATERIAL_Water);

	Extents startExt = dm.cluster->cellToWorldExtents(startPosition);
	Vector3 endCenter = dm.cluster->cellToWorldExtents(endSearchPosition).center();
	Extents wedgeCellExt = dm.cluster->cellToWorldExtents(wedgePosition);
	Extents endWaterExt = dm.cluster->cellToWorldExtents(endWaterPosition);

	float yHeight = startExt.max().y - startExt.min().y;

	Vector3 hitWedgeStart(startExt.center().x,
		startExt.min().y + .25f * yHeight, startExt.center().z);
	Vector3 missWedgeStart(startExt.center().x,
		startExt.min().y + .75f * yHeight, startExt.center().z);
	Vector3 direction = endCenter - startExt.center();

	// hit wedge
	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(hitWedgeStart, direction),
			shared_ptr<Instance>());
	checkRaycastResult("hit wedge", dm.cluster,
		Vector3(wedgeCellExt.min().x, hitWedgeStart.y, wedgeCellExt.center().z),
		rayHit);

	// miss wedge
	rayHit = dm.workspace->getRayHit<Instance>(
			RbxRay(missWedgeStart, direction),
			shared_ptr<Instance>());
	checkRaycastResult("miss wedge", dm.cluster,
		Vector3(endWaterExt.max().x, missWedgeStart.y, wedgeCellExt.center().z),
		rayHit);
}

BOOST_AUTO_TEST_CASE( GetSurfaceReturnsCellSurface ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);

	Vector3int16 filledCell(150, 45, 220);
	dm.cluster->getVoxelGrid()->setCell(filledCell, cell, CELL_MATERIAL_Grass);

	Vector3 worldCellCenter = cellToWorld_smallestCorner(filledCell) + (Vector3(kCELL_SIZE, kCELL_SIZE, kCELL_SIZE) / 2);
	float rayDelta = 2*kCELL_SIZE;

	// -X
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter - Vector3(rayDelta, 0, 0), Vector3(2 * rayDelta, 0, 0)), tmp);;
		BOOST_CHECK_EQUAL(tmp, NORM_X_NEG);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_X_NEG);
	}

	// +X
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter + Vector3(rayDelta, 0, 0), Vector3(-2 * rayDelta, 0, 0)), tmp);;
		BOOST_CHECK_EQUAL(tmp, NORM_X);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_X);
	}

	// -Y
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter - Vector3(0, rayDelta, 0), Vector3(0, 2 * rayDelta, 0)), tmp);
		BOOST_CHECK_EQUAL(tmp, NORM_Y_NEG);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_Y_NEG);
	}

	// +Y
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter + Vector3(0, rayDelta, 0), Vector3(0, -2 * rayDelta, 0)), tmp);
		BOOST_CHECK_EQUAL(tmp, NORM_Y);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_Y);
	}

	// -Z
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter - Vector3(0, 0, rayDelta), Vector3(0, 0, 2 * rayDelta)), tmp);
		BOOST_CHECK_EQUAL(tmp, NORM_Z_NEG);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_Z_NEG);
	}

	// +Z
	{
		int tmp = 0;
		RBX::Surface surface = dm.cluster->getSurface(RbxRay(worldCellCenter + Vector3(0, 0, rayDelta), Vector3(0, 0, -2 * rayDelta)), tmp);
		BOOST_CHECK_EQUAL(tmp, NORM_Z);
		BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
		BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_Z);
	}
}

BOOST_AUTO_TEST_CASE( GetSurfaceDoesNotHitTerrainEdge )
{
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);

	Vector3int16 filledCell(0, 20, 0);
	dm.cluster->getVoxelGrid()->setCell(filledCell, cell, CELL_MATERIAL_Grass);

	Vector3 worldCellCorner = cellToWorld_smallestCorner(filledCell);

	Vector3 rayStart(
		worldCellCorner.x + kCELL_SIZE / 2, // center in x
		worldCellCorner.y - kCELL_SIZE, // below by one cell
		worldCellCorner.z - (kCELL_SIZE / 2) // outside of cluster by half a cell
		);

	Vector3int16 sanityCheck = worldToCell_floor(rayStart);
	BOOST_REQUIRE_EQUAL(sanityCheck, Vector3int16(0, 19, -1));

	int tmp = 0;
	// pass through -Z plane of cluster, but hit -Y plane of cell
	RBX::Surface surface = dm.cluster->getSurface(
		RbxRay(rayStart, Vector3(0, 10 * kCELL_SIZE, 10 * kCELL_SIZE)), tmp);

	BOOST_CHECK_EQUAL(tmp, NORM_Y_NEG);
	BOOST_CHECK_EQUAL(surface.getPartInstance(), dm.cluster);
	BOOST_CHECK_EQUAL(surface.getNormalId(), NORM_Y_NEG);
}

BOOST_AUTO_TEST_CASE( BoundingBoxIncludesTerrainButRayMissesTerrain ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);

	Vector3int16 filledCell(0, 0, 0);
	dm.cluster->getVoxelGrid()->setCell(filledCell, cell, CELL_MATERIAL_Grass);

	const Vector3 kHalfCell(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);
	Vector3 start = cellToWorld_smallestCorner(Vector3int16(-2,0,0)) + kHalfCell;
	Vector3 end = cellToWorld_smallestCorner(Vector3int16(0,-2,0)) + kHalfCell;

	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("Outside Cluster", NULL, end, rayHit);
}

BOOST_AUTO_TEST_CASE( RayInvovlesThreeChunks ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);

	Vector3int16 filledCell(0, 0, 0);
	dm.cluster->getVoxelGrid()->setCell(filledCell, cell, CELL_MATERIAL_Grass);

	Vector3 start(0, 15, 15);
	Vector3 end(
		3.5f * 
		32 * // cells/chunk
		4, // studs/cell
		15, 15);

	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("Outside Cluster", NULL, end, rayHit);
}

BOOST_AUTO_TEST_CASE( TerrainLoopRepro ) {
	RaycastTestFixture dm;
	RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

	Cell cell;
	cell.solid.setBlock(CELL_BLOCK_Solid);
	cell.solid.setOrientation(CELL_ORIENTATION_NegZ);

	Vector3int16 filledCell(0, 0, 0);
	dm.cluster->getVoxelGrid()->setCell(filledCell, cell, CELL_MATERIAL_Grass);

	// these numbers were pulled from a crash
	Vector3 start(-5.8e-22f, 253.4f, -.5f);
	Vector3 end(-5.6e-22f, 258.343f, -9.48f);

	// DE5001 - if worldToCell_floor rounds incorrectly, this call
	// will go into an infinite loop adding elements to a vector.
	shared_ptr<const Reflection::Tuple> rayHit =
		dm.workspace->getRayHit<Instance>(
			RbxRay(start, end - start),
			shared_ptr<Instance>());
	checkRaycastResult("Outside Cluster", NULL, end, rayHit);
}

BOOST_AUTO_TEST_CASE( IsRegion3Empty ) {
    const int kOffset = 0;

	static const Region3 kTestRegionInsideCluster(Vector3(10,10,10), Vector3(30,30,30));
	static const Region3 kTestRegionEnvelopesCluster(Vector3(-2000, -10, -2000), Vector3(2000, 1000, 2000));
	static const Vector3int16 kCellInsideBothTestRegions(kOffset + 5, 5, kOffset + 5);
	static const Vector3int16 kCellInsideBiggerRegion(0,0,0);
	// empty terrain
	{
		DataModelFixture dm;
		
		RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);
		RBX::Workspace* workspace = dm->getWorkspace();
		workspace->createTerrain();
		RBX::MegaClusterInstance* cluster = Instance::fastDynamicCast<MegaClusterInstance>(
			workspace->getTerrain());

		BOOST_REQUIRE(!cluster->isAllocated());

		BOOST_REQUIRE(workspace->isRegion3Empty(kTestRegionInsideCluster, shared_ptr<Instance>()));
		BOOST_REQUIRE(workspace->isRegion3Empty(kTestRegionEnvelopesCluster, shared_ptr<Instance>()));
	}

	// terrain with cell in both test regions
	{
		RaycastTestFixture dm;
		RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

		Cell solidVoxel;
		solidVoxel.solid.setBlock(CELL_BLOCK_Solid);
		dm.cluster->getVoxelGrid()->setCell(kCellInsideBothTestRegions, solidVoxel, CELL_MATERIAL_Grass);

		BOOST_REQUIRE(!dm.workspace->isRegion3Empty(kTestRegionInsideCluster, shared_ptr<Instance>()));
		BOOST_REQUIRE(!dm.workspace->isRegion3Empty(kTestRegionEnvelopesCluster, shared_ptr<Instance>()));
	}

	// terrain with water cell in both test regions (water == empty)
	{
		RaycastTestFixture dm;
		RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

		Cell waterVoxel = Constants::kWaterOnWedgeCell;
		dm.cluster->getVoxelGrid()->setCell(kCellInsideBothTestRegions, waterVoxel, CELL_MATERIAL_Water);

		BOOST_REQUIRE(dm.workspace->isRegion3Empty(kTestRegionInsideCluster, shared_ptr<Instance>()));
		BOOST_REQUIRE(dm.workspace->isRegion3Empty(kTestRegionEnvelopesCluster, shared_ptr<Instance>()));
	}

	// terrain with cell in outer regions
	{
		RaycastTestFixture dm;
		RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

		Cell solidVoxel;
		solidVoxel.solid.setBlock(CELL_BLOCK_Solid);
		dm.cluster->getVoxelGrid()->setCell(kCellInsideBiggerRegion, solidVoxel, CELL_MATERIAL_Grass);

		BOOST_REQUIRE(dm.workspace->isRegion3Empty(kTestRegionInsideCluster, shared_ptr<Instance>()));
		BOOST_REQUIRE(!dm.workspace->isRegion3Empty(kTestRegionEnvelopesCluster, shared_ptr<Instance>()));
	}

	// terrain with water cell in outer regions (water == empty)
	{
		RaycastTestFixture dm;
		RBX::DataModel::LegacyLock lock(&dm.dm, RBX::DataModelJob::Write);

		Cell waterVoxel = Constants::kWaterOnWedgeCell;
		dm.cluster->getVoxelGrid()->setCell(kCellInsideBothTestRegions, waterVoxel, CELL_MATERIAL_Water);

		BOOST_REQUIRE(dm.workspace->isRegion3Empty(kTestRegionInsideCluster, shared_ptr<Instance>()));
		BOOST_REQUIRE(dm.workspace->isRegion3Empty(kTestRegionEnvelopesCluster, shared_ptr<Instance>()));
	}
}

BOOST_AUTO_TEST_SUITE_END()
