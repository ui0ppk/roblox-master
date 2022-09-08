#include "boost/test/unit_test.hpp"

#include "Network/Player.h"
#include "rbx/test/DataModelFixture.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "Util/ScopedAssign.h"
#include "Util/SoundService.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/SpecialMesh.h"
#include "Voxel/Cell.h"
#include "Voxel/Grid.h"
#include "util/StreamRegion.h"

using namespace boost;
using namespace RBX;
using namespace Network;

BOOST_AUTO_TEST_SUITE(ReplicatorStreamJobTest)

static void SpawnBasePlate(DataModelFixture& dm, const Vector3& baseplateLocation)
{
    shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
    basePlate->setTranslationUi(baseplateLocation);
    basePlate->setPartSizeUi(Vector3(50, 2.4f, 50));
    basePlate->setAnchored(true);
    basePlate->setName("BasePlate");
    basePlate->setParent(dm->getWorkspace());
}

static void InsertSpawnLocation(DataModelFixture& dm, const Vector3& location)
{
	shared_ptr<SpawnLocation> spawnLocation = Creatable<Instance>::create<SpawnLocation>();
	spawnLocation->setAnchored(true);
	spawnLocation->setTranslationUi(location);
	spawnLocation->setParent(dm->getWorkspace());
}

// NOTE: As the constants used in streaming change over time, this test will
// need to be updated. This test is trying to show that streaming is able
// to only send a subset of the workspace to a client that joins, and
// that on join, that subset includes parts that are close to the spawn
// location.
void _PartsCloseToSpawnLocationAreStreamedOnJoin()
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 boxSize(3, 3, 3);
	const int studsBetweenBoxCenters = 20;
	const int gridDim = 7;
	const char* nameFormat = "|%d|%d|%d|";
	size_t serverWorkspaceSizeBeforeSpawn = 0;

	DataModelFixture serverDm;
	shared_ptr<SpawnLocation> spawnLocation;

	// Build an array of boxes and a spawn location
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);

		for (int z = -7; z <= 7; ++z) {
			for (int y = -7; y <= 7; ++y) {
				for (int x = -7; x <= 7; ++x) {
					shared_ptr<BasicPartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
					part->setTranslationUi(studsBetweenBoxCenters * Vector3(x, y, z));
					part->setPartSizeUi(boxSize);
					part->setAnchored(true);
					part->setName(format(nameFormat, x, y, z));
					part->setParent(serverDm->getWorkspace());
				}
			}
		}

		spawnLocation = Creatable<Instance>::create<SpawnLocation>();
		spawnLocation->setAnchored(true);
		spawnLocation->setParent(serverDm->getWorkspace());
		serverWorkspaceSizeBeforeSpawn = serverDm->getWorkspace()->numChildren();
	}

	const Vector3 minBoxGridCoordinate = -gridDim * studsBetweenBoxCenters * Vector3::one();
	const Vector3 maxBoxGridCoordinate = gridDim * studsBetweenBoxCenters * Vector3::one();

	NetworkFixture::startServer(serverDm);

	// try spawning character around max coordinate
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		spawnLocation->setTranslationUi(maxBoxGridCoordinate + Vector3(10, 10, 10));
	}

	{
		DataModelFixture clientDm;
		{
			DataModel::LegacyLock l(&clientDm, DataModelJob::Write);
			const Vector3 baseplateLocation(0, 9.5f, 0);
			SpawnBasePlate(clientDm, baseplateLocation);
		}
		NetworkFixture::startClient(clientDm, true);
		{
			DataModel::LegacyLock l(&clientDm, DataModelJob::Read);

			Player* player = Players::findLocalPlayer(&clientDm);
			// make sure we spawned in the right place
			BOOST_WARN_EQUAL(
				player->getCharacter()->getLocation().translation,
				maxBoxGridCoordinate + Vector3(10, 10, 10) + Vector3(0, 10.5f, -0.5f));

			// Check that the client has significantly fewer parts than server.
			// This is flaky! There is not explicit throttle stopping a fast
			// server from sending more than this by this point.
			//BOOST_CHECK(clientDm->getWorkspace()->numChildren() < 
			//	(serverWorkspaceSizeBeforeSpawn / 2));
			//BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName(
			//	format(nameFormat, -gridDim, -gridDim, -gridDim)) == NULL);

			// Assert that the near corner of the block cube is present
			BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName(
				format(nameFormat, gridDim, gridDim, gridDim)));
		}
	}

	// try spawning character around min coordinate
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		spawnLocation->setTranslationUi(minBoxGridCoordinate - Vector3(10, 10, 10));
	}

	{
		DataModelFixture clientDm;
		NetworkFixture::startClient(clientDm, true);

		{
			DataModel::LegacyLock l(&clientDm, DataModelJob::Read);

			Player* player = Players::findLocalPlayer(&clientDm);
			// make sure we spawned in the right place
			BOOST_WARN_EQUAL(
				player->getCharacter()->getLocation().translation,
				minBoxGridCoordinate - Vector3(10, 10, 10) + Vector3(0, 10.5f, -0.5f));

			// Check that the client has significantly fewer parts than server.
			// This is flaky! There is not explicit throttle stopping a fast
			// server from sending more than this by this point.
			//BOOST_CHECK(clientDm->getWorkspace()->numChildren() < 
			//	(serverWorkspaceSizeBeforeSpawn / 2));
			//BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName(
			//	format(nameFormat, gridDim, gridDim, gridDim)) == NULL);

			// Assert that the near corner of the block cube is present
			BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName(
				format(nameFormat, -gridDim, -gridDim, -gridDim)));

		}
	}
}

BOOST_AUTO_TEST_CASE(PartsCloseToSpawnLocationAreStreamedOnJoin)
{
	// disabling this test for now, we don't currently have a throttle to stop a fast server to make this test reliable
	//_PartsCloseToSpawnLocationAreStreamedOnJoin();
}

BOOST_AUTO_TEST_CASE(PartEnteringStreamedZoneFromUnstreamedZoneIsStreamedIn)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(1024, 0, 0);

	DataModelFixture serverDm;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);

		shared_ptr<SpawnLocation> spawnLocation = Creatable<Instance>::create<SpawnLocation>();
		spawnLocation->setTranslationUi(baseplateLocation + Vector3(0, 4, 0));
		spawnLocation->setAnchored(true);
		spawnLocation->setParent(serverDm->getWorkspace());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);
		SpawnBasePlate(clientDm, baseplateLocation);
	}
	NetworkFixture::startClient(clientDm, true);

	// Assert world streamed correctly
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("BasePlate"));
	}

	// Create a part on the server, outside of the streamed zone. 
	shared_ptr<BasicPartInstance> movedPart = Creatable<Instance>::create<BasicPartInstance>();
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		movedPart->setTranslationUi(baseplateLocation + Vector3(10000, 10000, 0));
		movedPart->setPartSizeUi(Vector3(4, 4, 4));
		movedPart->setAnchored(true);
		movedPart->setName("MovedPart");
		movedPart->setParent(serverDm->getWorkspace());
	}

	// Assert that, even after yielding for a while, the part isn't streamed yet.
	G3D::System::sleep(.25f);
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("MovedPart") == NULL);
	}

	CEvent descendantAdded(true);
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		clientDm->getWorkspace()->getOrCreateDescendantAddedSignal()->connect(
			boost::bind(&CEvent::Set, &descendantAdded));
	}

	// Move the part on the server into a known streamed region, and check that
	// the client gets it (after giving the server some time to stream it).
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		movedPart->setTranslationUi(baseplateLocation + Vector3(4, 10, 0));
	}
	
	BOOST_CHECK(descendantAdded.Wait(2500));
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("MovedPart"));
	}
}

class PartAndTerrainSentAtSameTimeHelper
{
	Workspace* workspace;
	Vector3int16 firstRegionVoxelCoord;
	Vector3int16 secondRegionVoxelCoord;
	Vector3int16 thirdRegionVoxelCoord;
	CEvent* doneEvent;
public:
	PartAndTerrainSentAtSameTimeHelper(
		Workspace* workspace, const Vector3int16& firstCoord,
		const Vector3int16& secondCoord, const Vector3int16& thirdCoord,
		CEvent* doneEvent)
		: workspace(workspace)
		, firstRegionVoxelCoord(firstCoord)
		, secondRegionVoxelCoord(secondCoord)
		, thirdRegionVoxelCoord(thirdCoord)
		, doneEvent(doneEvent)
	{
	}

	void notify(shared_ptr<Instance> newDescendant) {
		int partIndex = -1;
		if (newDescendant->getName() == "FirstPart")
		{
			partIndex = 0;
		}
		else if (newDescendant->getName() == "SecondPart")
		{
			partIndex = 1;
		}
		else if (newDescendant->getName() == "ThirdPart")
		{
			partIndex = 2;
		}

		if (partIndex == -1)
		{
			return;
		}

		Instance* firstPart = workspace->findFirstChildByName("FirstPart");
		Instance* secondPart = workspace->findFirstChildByName("SecondPart");
		Instance* thirdPart = workspace->findFirstChildByName("ThirdPart");
	
		BOOST_CHECK(partIndex >= 0 ? firstPart != NULL : firstPart == NULL);
		BOOST_CHECK(partIndex >= 1 ? secondPart != NULL : secondPart == NULL);
		BOOST_CHECK(partIndex >= 2 ? thirdPart != NULL : thirdPart == NULL);

		Voxel::Grid* grid = Instance::fastDynamicCast<MegaClusterInstance>(
			workspace->getTerrain())->getVoxelGrid();
		Voxel::Grid::Region firstRegion = grid->getRegion(
			firstRegionVoxelCoord, firstRegionVoxelCoord);
		Voxel::Cell firstCell = firstRegion.voxelAt(firstRegionVoxelCoord);
		BOOST_CHECK(partIndex >= 0 ? !firstCell.isEmpty() : true);

		Voxel::Grid::Region secondRegion = grid->getRegion(
			secondRegionVoxelCoord, secondRegionVoxelCoord);
		Voxel::Cell secondCell = secondRegion.voxelAt(secondRegionVoxelCoord);
		BOOST_CHECK(partIndex >= 1 ? !secondCell.isEmpty() : true);

		Voxel::Grid::Region thirdRegion = grid->getRegion(
			thirdRegionVoxelCoord, thirdRegionVoxelCoord);
		Voxel::Cell thirdCell = thirdRegion.voxelAt(thirdRegionVoxelCoord);
		BOOST_CHECK(partIndex >= 2 ? !thirdCell.isEmpty() : true);

		if (partIndex == 2) {
			doneEvent->Set();
		}
	}
};

// The constants in this test are highly dependent on constants
// used internally in streaming. This test will likely need to
// be changed if stream region size changes.
BOOST_AUTO_TEST_CASE(PartsAndTerrainSentAtSameTime)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	// some constants
	StreamRegion::Id spawnRegion(0, 0, 0);

	StreamRegion::Id firstRegion(1, 0, 0);
	Vector3int16 firstRegionVoxelCoord = StreamRegion::getMinVoxelCoordinateInsideRegion(firstRegion);
	Voxel::Cell firstRegionCell;
	firstRegionCell.solid.setBlock(Voxel::CELL_BLOCK_Solid);
	Voxel::CellMaterial firstRegionMaterial = Voxel::CELL_MATERIAL_Grass;
	
	StreamRegion::Id secondRegion(2, 0, 0);
	Vector3int16 secondRegionVoxelCoord = StreamRegion::getMinVoxelCoordinateInsideRegion(secondRegion);
	Voxel::Cell secondRegionCell;
	secondRegionCell.solid.setBlock(Voxel::CELL_BLOCK_HorizontalWedge);
	Voxel::CellMaterial secondRegionMaterial = Voxel::CELL_MATERIAL_Sand;

	StreamRegion::Id thirdRegion(3, 0, 0);
	Vector3int16 thirdRegionVoxelCoord = StreamRegion::getMinVoxelCoordinateInsideRegion(thirdRegion);
	Voxel::Cell thirdRegionCell;
	thirdRegionCell.solid.setBlock(Voxel::CELL_BLOCK_VerticalWedge);
	Voxel::CellMaterial thirdRegionMaterial = Voxel::CELL_MATERIAL_Aluminum;

	// setup server
	DataModelFixture serverDm;

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);

		// ensure terrain is set up
		serverDm->getWorkspace()->createTerrain();
		Voxel::Grid* voxelGrid = Instance::fastDynamicCast<MegaClusterInstance>(
			serverDm->getWorkspace()->getTerrain())->getVoxelGrid();

		// add spawn location
		Extents startRegionExtents = StreamRegion::extentsFromRegionId(spawnRegion);
		shared_ptr<SpawnLocation> spawnLocation = Creatable<Instance>::create<SpawnLocation>();
		spawnLocation->setTranslationUi(startRegionExtents.center());
		spawnLocation->setAnchored(true);
		spawnLocation->setParent(serverDm->getWorkspace());

		// put some terrain and a part in first region
		{
			Extents firstRegionExtents = StreamRegion::extentsFromRegionId(firstRegion);
			shared_ptr<BasicPartInstance> firstPart = Creatable<Instance>::create<BasicPartInstance>();
			firstPart->setTranslationUi(firstRegionExtents.center());
			firstPart->setAnchored(true);
			firstPart->setName("FirstPart");
			firstPart->setParent(serverDm->getWorkspace());
			voxelGrid->setCell(firstRegionVoxelCoord, firstRegionCell, firstRegionMaterial);
		}

		// put some terrain and a part in second region
		{
			Extents secondRegionExtents = StreamRegion::extentsFromRegionId(secondRegion);
			shared_ptr<BasicPartInstance> secondPart = Creatable<Instance>::create<BasicPartInstance>();
			secondPart->setTranslationUi(secondRegionExtents.center());
			secondPart->setAnchored(true);
			secondPart->setName("SecondPart");
			secondPart->setParent(serverDm->getWorkspace());
			voxelGrid->setCell(secondRegionVoxelCoord, secondRegionCell, secondRegionMaterial);
		}

		// put some terrain and a part in third region
		{
			Extents thirdRegionExtents = StreamRegion::extentsFromRegionId(thirdRegion);
			shared_ptr<BasicPartInstance> thirdPart = Creatable<Instance>::create<BasicPartInstance>();
			thirdPart->setTranslationUi(thirdRegionExtents.center());
			thirdPart->setAnchored(true);
			thirdPart->setName("ThirdPart");
			thirdPart->setParent(serverDm->getWorkspace());
			voxelGrid->setCell(thirdRegionVoxelCoord, thirdRegionCell, thirdRegionMaterial);
		}
	}

	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	CEvent doneEvent(false /*manual reset*/);
	PartAndTerrainSentAtSameTimeHelper helper(clientDm->getWorkspace(),
		firstRegionVoxelCoord, secondRegionVoxelCoord, thirdRegionVoxelCoord,
		&doneEvent);
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		clientDm->getWorkspace()->getOrCreateDescendantAddedSignal()->connect(
			boost::bind(&PartAndTerrainSentAtSameTimeHelper::notify, &helper, _1));
	}
	NetworkFixture::startClient(clientDm, true);

	BOOST_CHECK(doneEvent.Wait(2000));
}

BOOST_AUTO_TEST_CASE(ReparentIntoStreamedAreaFromOutside)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(0, -2.4f, 0);

	DataModelFixture serverDm;
	shared_ptr<PartInstance> partInsideStreamedArea;
	shared_ptr<PartInstance> partOutsideStreamedArea;
	shared_ptr<SpecialShape> specialMesh;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);

		InsertSpawnLocation(serverDm, baseplateLocation + Vector3(0, 5, 0));

		partInsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partInsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(9, 9, 0));
		partInsideStreamedArea->setAnchored(true);
		partInsideStreamedArea->setName("Inside");
		partInsideStreamedArea->setParent(serverDm->getWorkspace());

		partOutsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(1000000, 0, 0));
		partOutsideStreamedArea->setAnchored(true);
		partOutsideStreamedArea->setName("Outside");
		partOutsideStreamedArea->setParent(serverDm->getWorkspace());

		// put special mesh under part inside streamed area
		specialMesh = Creatable<Instance>::create<SpecialShape>();
		specialMesh->setName("Mesh");
		specialMesh->setParent(partOutsideStreamedArea.get());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);
		SpawnBasePlate(clientDm, baseplateLocation);
	}
	NetworkFixture::startClient(clientDm, true);
	G3D::System::sleep(1.0f);

	CEvent descendantAdded(false /*manual reset*/);
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// Sanity check that inside part is present and outside part is not
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);

		clientDm->getWorkspace()->getOrCreateDescendantAddedSignal()->connect(
			boost::bind(&CEvent::Set, &descendantAdded));
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		specialMesh->setParent(partInsideStreamedArea.get());
	}

	int remainingRetries = 10;
	bool meshWasSent = false;
	while (remainingRetries > 0) {
		BOOST_CHECK(descendantAdded.Wait(1000));

		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// Sanity check that inside part is present and outside part is not
		Instance* insidePart = clientDm->getWorkspace()->findFirstChildByName("Inside");
		BOOST_REQUIRE(insidePart);
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);

		if (insidePart->findFirstChildByName("Mesh"))
		{
			meshWasSent = true;
			break;
		}
		else
		{
			remainingRetries--;
		}
	}
	BOOST_CHECK(meshWasSent);
}

BOOST_AUTO_TEST_CASE( ManualJointManipulation )
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(0, -2.4f, 0);

	DataModelFixture serverDm;
	shared_ptr<PartInstance> part1InsideStreamedArea;
	shared_ptr<PartInstance> part2InsideStreamedArea;
	shared_ptr<PartInstance> partOutsideStreamedArea;
	shared_ptr<ManualWeld> firstManualWeld;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

        Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);

		InsertSpawnLocation(serverDm, baseplateLocation + Vector3(0, 5, 0));

		part1InsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		part1InsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(9, 9, 0));
		part1InsideStreamedArea->setAnchored(true);
		part1InsideStreamedArea->setName("Inside1");
		part1InsideStreamedArea->setParent(serverDm->getWorkspace());

		part2InsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		part2InsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(-9, 9, 0));
		part2InsideStreamedArea->setAnchored(true);
		part2InsideStreamedArea->setName("Inside2");
		part2InsideStreamedArea->setParent(serverDm->getWorkspace());

		partOutsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(1000000, 0, 0));
		partOutsideStreamedArea->setAnchored(true);
		partOutsideStreamedArea->setName("Outside");
		partOutsideStreamedArea->setParent(serverDm->getWorkspace());

		firstManualWeld = Creatable<Instance>::create<ManualWeld>();
		firstManualWeld->setName("FirstWeld");
		firstManualWeld->setPart0(part1InsideStreamedArea.get());
		firstManualWeld->setPart1(part2InsideStreamedArea.get());
		firstManualWeld->setParent(serverDm->getWorkspace());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);
		SpawnBasePlate(clientDm, baseplateLocation);
	}
	NetworkFixture::startClient(clientDm, true);
	G3D::System::sleep(1.0f);
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// Sanity check that inside part is present and outside part is not
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside1"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside2"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("FirstWeld"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		firstManualWeld->setPart1(partOutsideStreamedArea.get());
		shared_ptr<ManualWeld> secondManualWeld = Creatable<Instance>::create<ManualWeld>();
		secondManualWeld->setName("SecondWeld");
		secondManualWeld->setPart0(part1InsideStreamedArea.get());
		secondManualWeld->setPart1(part2InsideStreamedArea.get());
		secondManualWeld->setParent(serverDm->getWorkspace());

		// check that the first weld hasn't been kicked
		BOOST_CHECK(firstManualWeld->getParent());
	}

	G3D::System::sleep(.5f);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// Check the weld state
		ManualWeld* firstWeld = Instance::fastDynamicCast<ManualWeld>(
			clientDm->getWorkspace()->findFirstChildByName("FirstWeld"));
		ManualWeld* secondWeld = Instance::fastDynamicCast<ManualWeld>(
			clientDm->getWorkspace()->findFirstChildByName("SecondWeld"));

		BOOST_CHECK(firstWeld);
		BOOST_REQUIRE(secondWeld);
		BOOST_CHECK(secondWeld->getPart0() == clientDm->getWorkspace()->findFirstChildByName("Inside1"));
		BOOST_CHECK(secondWeld->getPart1() == clientDm->getWorkspace()->findFirstChildByName("Inside2"));
	}

}

BOOST_AUTO_TEST_SUITE_END()
