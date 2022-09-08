#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/MegaCluster.h"
#include "V8World/World.h"

using namespace RBX;
using namespace RBX::Voxel;

static CellMaterial generateMaterial(int x, int y, int z, int fudger) {
	int tmp = ((fudger * x) ^ y ^ z);
	return (CellMaterial)((tmp % 17) + 1);
}

static Cell generateCell(int x, int y, int z, int fudger, unsigned char material) {
	int tmp = ((fudger * x) ^ y ^ z) / 17;

	Cell result;
	if (material == CELL_MATERIAL_Water) {
		if ((tmp & 0xff) == 40) {
			return Constants::kUniqueEmptyCellRepresentation;
		}
		WaterCellForce force = WaterCellForce(tmp % MAX_WATER_CELL_FORCES);
		WaterCellDirection direction = WaterCellDirection((tmp / MAX_WATER_CELL_FORCES) % MAX_WATER_CELL_DIRECTIONS);
		result.solid.setBlock(CELL_BLOCK_Empty);
		result.water.setForceAndDirection(force, direction);
	} else {
		CellBlock block = CellBlock(tmp % CELL_BLOCK_Empty); // allow any cell block < CELL_BLOCK_Empty (5)
		CellOrientation orientation = CellOrientation((tmp / CELL_BLOCK_Empty) % MAX_CELL_ORIENTATIONS);
		result.solid.setBlock(block);
		result.solid.setOrientation(orientation);
	}
	return result;
}

void fillChunk(Vector3int16 base, int pass, MegaClusterInstance* instance) {
	const int maxXZ = 2 * kXZ_CHUNK_SIZE;
	const int maxY = 2 * kY_CHUNK_SIZE;
	
	for (int y = 0; y < maxY; ++y) {
	for (int z = 0; z < maxXZ; ++z) {
	for (int x = 0; x < maxXZ; ++x) {
		CellMaterial material = generateMaterial(x,y,z,pass);
		Cell cell = generateCell(x,y,z,pass,material);
		instance->getVoxelGrid()->setCell(base + Vector3int16(x,y,z), cell, material);
	}
	}
	}
}

void validateChunk(Vector3int16 base, int pass, MegaClusterInstance* instance) {
	const int maxXZ = 2 * kXZ_CHUNK_SIZE;
	const int maxY = 2 * kY_CHUNK_SIZE;

	int invalidX, invalidY, invalidZ;
	invalidX = invalidY = invalidZ = -1;

	for (int y = 0; y < maxY && invalidX == -1; ++y) {
	for (int z = 0; z < maxXZ && invalidX == -1; ++z) {
	for (int x = 0; x < maxXZ && invalidX == -1; ++x) {
		const CellMaterial expectedMaterial = generateMaterial(x,y,z,pass);
		Vector3int16 pos = base + Vector3int16(x,y,z);
		CellMaterial material = instance->getVoxelGrid()->getCellMaterial(pos);
		if (material != expectedMaterial) {
			invalidX = x;
			invalidY = y;
			invalidZ = z;
		}
	}
	}
	}

	BOOST_CHECK_EQUAL(Vector3int16(-1,-1,-1), Vector3int16(invalidX, invalidY, invalidZ));
}

struct ChangeHistoryFixture {
	DataModelFixture dm;
	MegaClusterInstance* cluster;
	ChangeHistoryService* changeHistory;
	ChangeHistoryFixture() {}
	void initWithEmptyCluster() {
		changeHistory = dm->create<ChangeHistoryService>();
		changeHistory->setEnabled(true);

		Workspace* workspace = dm->getWorkspace();
		workspace->createTerrain();
		cluster = Instance::fastDynamicCast<MegaClusterInstance>(
			workspace->getTerrain());
		
		// hack -- set one cell so that the cluster is allocated
		cluster->getVoxelGrid()->setCell(Vector3int16(0, 0, 0),
			Voxel::Constants::kUniqueEmptyCellRepresentation, CELL_MATERIAL_Water);
	}
};

BOOST_AUTO_TEST_SUITE(ClusterChangeHistoryTest)

BOOST_AUTO_TEST_CASE(FillChunkStressTest) {
	ChangeHistoryFixture chf;
	DataModel::LegacyLock lock(&chf.dm, RBX::DataModelJob::Write);
	chf.initWithEmptyCluster();

	const Vector3int16 base(16,8,16);
	
	for (int pass = 1; pass < 5; ++pass) {
		fillChunk(base, pass, chf.cluster);
		chf.changeHistory->requestWaypoint2(format("Pass %d", pass));
	}

	for (int i = 0; i < 10; ++i) {
		for (int currentPassState = 4; currentPassState > 0; --currentPassState) {
			validateChunk(base, currentPassState, chf.cluster);
			chf.changeHistory->unplay();
		}

		BOOST_CHECK_EQUAL(0, chf.cluster->getNonEmptyCellCount());

		for (int currentPassState = 1; currentPassState < 5; ++currentPassState) {
			chf.changeHistory->play();
			validateChunk(base, currentPassState, chf.cluster);
		}
	}
}

BOOST_AUTO_TEST_CASE(PartiallyOverlappingChunkTest) {
	ChangeHistoryFixture chf;
	DataModel::LegacyLock lock(&chf.dm, RBX::DataModelJob::Write);
	chf.initWithEmptyCluster();

	Vector3int16 offset(kXZ_CHUNK_SIZE / 2, kY_CHUNK_SIZE / 2, kXZ_CHUNK_SIZE / 2);
	Vector3int16 base1(12,12,12);
	Vector3int16 base2 = base1 + offset;
	Vector3int16 base3 = base2 + offset;
	
	// Load up data
	fillChunk(base1, 1, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 1");

	fillChunk(base2, 2, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 2");

	fillChunk(base3, 3, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 3");

	fillChunk(base1, 4, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 4");

	fillChunk(base2, 5, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 5");

	fillChunk(base3, 6, chf.cluster);
	chf.changeHistory->requestWaypoint("pass 6");

	// unwind all changes
	validateChunk(base3, 6, chf.cluster);
	
	chf.changeHistory->unplay();
	validateChunk(base2, 5, chf.cluster);

	chf.changeHistory->unplay();
	validateChunk(base1, 4, chf.cluster);

	chf.changeHistory->unplay();
	validateChunk(base3, 3, chf.cluster);

	chf.changeHistory->unplay();
	validateChunk(base2, 2, chf.cluster);

	chf.changeHistory->unplay();
	validateChunk(base1, 1, chf.cluster);

	chf.changeHistory->unplay();
	BOOST_CHECK_EQUAL(0, chf.cluster->getNonEmptyCellCount());

	// replay all changes
	chf.changeHistory->play();
	validateChunk(base1, 1, chf.cluster);

	chf.changeHistory->play();
	validateChunk(base2, 2, chf.cluster);

	chf.changeHistory->play();
	validateChunk(base3, 3, chf.cluster);

	chf.changeHistory->play();
	validateChunk(base1, 4, chf.cluster);

	chf.changeHistory->play();
	validateChunk(base2, 5, chf.cluster);

	chf.changeHistory->play();
	validateChunk(base3, 6, chf.cluster);
}

BOOST_AUTO_TEST_SUITE_END()
