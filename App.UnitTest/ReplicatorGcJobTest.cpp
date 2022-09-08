#include "boost/test/unit_test.hpp"

#include "NetworkSettings.h"
#include "Network/Player.h"
#include "rbx/test/DataModelFixture.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "Util/ScopedAssign.h"
#include "Util/SoundService.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/SpecialMesh.h"
#include "Voxel/Cell.h"
#include "Voxel/Grid.h"
#include "util/MemoryStats.h"
#include "Client.h"
#include "ClientReplicator.h"

using namespace boost;
using namespace RBX;
using namespace Network;

#ifdef RBX_PLATFORM_IOS
#define SLEEPTIME 5
#else
#define SLEEPTIME 2
#endif

BOOST_AUTO_TEST_SUITE(ReplicatorGcJobTest)

bool SetMemoryLevel(DataModelFixture& dm)
{
	RBX::Network::Client* client = RBX::ServiceProvider::find<RBX::Network::Client>(&dm);

	if (!client)
	{
		BOOST_MESSAGE("Warning: Failed to set memory level, client == null");
		return false;
	}

	RBX::Network::ClientReplicator* replicator = client->findFirstChildOfType<RBX::Network::ClientReplicator>();

	if (!replicator)
	{
		BOOST_MESSAGE("Warning: Failed to set memory level, replicator == null");
		return false;
	}

	if (replicator->getMemoryLevel() != RBX::MemoryStats::MEMORYLEVEL_ALL_CRITICAL_LOW)
	{
		if (NetworkSettings::singleton().getExtraMemoryUsedInMB() < MemoryStats::totalMemoryBytes()/1024/1024)
			NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);

		BOOST_MESSAGE("force setting memory level");
		replicator->updateMemoryStats();
	}

	BOOST_WARN(replicator->getMemoryLevel() <= RBX::MemoryStats::MEMORYLEVEL_ONLY_PHYSICAL_CRITICAL_LOW);
	
	return true;
}

// Tests to add:
// * Add basic test with a grid of parts (one per stream region), teleport
//   character around under memory pressure, and check that the expected regions
//   are streamed in / streamed out
// * Add a test where a gc instance list message has a ships-in-the-night with
//   a server position update (one position update moves part out of gc'ed region,
//   another motion puts another part into a gc'ed region).

BOOST_AUTO_TEST_CASE(MovingPartOutsideOfStreamedAreaCausesItToBeGced)
{
    ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

    DataModelFixture serverDm;

    {
	    DataModel::LegacyLock l(&serverDm, DataModelJob::Write);
		
	    Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);
	    serverDm->getWorkspace()->createTerrain();

	    shared_ptr<BasicPartInstance> partToStreamOut = Creatable<Instance>::create<BasicPartInstance>();
	    partToStreamOut->setTranslationUi(Vector3(10, 10, 0));
	    partToStreamOut->setAnchored(true);
	    partToStreamOut->setName("ToStreamOut");
	    partToStreamOut->setParent(serverDm->getWorkspace());
    }

    NetworkFixture::startServer(serverDm);

    DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// create base plate on client so player is guaranteed to land on it
		shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
		basePlate->setTranslationUi(Vector3::zero());
		basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
		basePlate->setAnchored(true);
		basePlate->setName("BasePlate");
		basePlate->setParent(clientDm->getWorkspace());
	}
    NetworkFixture::startClient(clientDm, true);

    // spawning player triggers the initial gc checker in gc job step so we need give it some time before shrinking the memory
    G3D::System::sleep(SLEEPTIME);

    NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);


    {
	    DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

	    BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("BasePlate"));
	    BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("ToStreamOut"));
    }
	

    {
	    DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

	    BasicPartInstance* partToStreamOut = Instance::fastDynamicCast<BasicPartInstance>(
		    serverDm->getWorkspace()->findFirstChildByName("ToStreamOut"));
	    BOOST_REQUIRE(partToStreamOut);

	    partToStreamOut->setTranslationUi(Vector3(1000000, 0, 0));
        BOOST_MESSAGE("move part away");
    }

    // replication
    G3D::System::sleep(SLEEPTIME);

    {
	    DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

	    // move character to trigger GC
	    RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
	    RBXASSERT(players);
	    RBX::Network::Player* player = players->getLocalPlayer();
	    RBXASSERT(player);
	    ModelInstance* character = player->getCharacter();
	    PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
		
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
		BOOST_MESSAGE("move player to trigger GC");
	    torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3(100, 5, 0)));

    }

    // wait for client to catch up
    G3D::System::sleep(SLEEPTIME);

    {
	    DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

	    BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("BasePlate"));
	    BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("ToStreamOut") == NULL);
    }

    NetworkSettings::singleton().setExtraMemoryUsedInMB(0);
}

BOOST_AUTO_TEST_CASE(MovingPartAndItsParentOutsideOfStreamedAreaShouldNotRemoveThePartTwice)
{
    ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

    DataModelFixture serverDm;
    shared_ptr<BasicPartInstance> parentPart;
    shared_ptr<BasicPartInstance> childPart;
    {
        DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

        Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);
        serverDm->getWorkspace()->createTerrain();

        parentPart = Creatable<Instance>::create<BasicPartInstance>();
        parentPart->setTranslationUi(Vector3(10, 10, 0));
        parentPart->setAnchored(true);
        parentPart->setName("ParentPart");
        parentPart->setParent(serverDm->getWorkspace());

        shared_ptr<SpecialShape> specialMesh;
        specialMesh = Creatable<Instance>::create<SpecialShape>();
        specialMesh->setName("Mesh");
        specialMesh->setParent(parentPart.get());

        childPart = Creatable<Instance>::create<BasicPartInstance>();
        childPart->setTranslationUi(Vector3(10, 10, 2));
        childPart->setName("ChildPart");
        childPart->setParent(parentPart.get());
    }

    NetworkFixture::startServer(serverDm);

    DataModelFixture clientDm;
    {
        DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

        // create base plate on client so player is guaranteed to land on it
        shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
        basePlate->setTranslationUi(Vector3::zero());
        basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
        basePlate->setAnchored(true);
        basePlate->setName("BasePlate");
        basePlate->setParent(clientDm->getWorkspace());
    }
    NetworkFixture::startClient(clientDm, true);

    // spawning player triggers the initial gc checker in gc job step so we need give it some time before shrinking the memory
    G3D::System::sleep(SLEEPTIME);

    NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);

    {
        DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

        SetMemoryLevel(clientDm);

        Instance* parent = clientDm->getWorkspace()->findFirstChildByName("ParentPart");
        BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("BasePlate"));
        BOOST_REQUIRE(parent);
        BOOST_REQUIRE(parent->findFirstChildByName("Mesh"));
        BOOST_REQUIRE(parent->findFirstChildByName("ChildPart"));
    }

    {
        DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

        BasicPartInstance* parentPart = Instance::fastDynamicCast<BasicPartInstance>(
            serverDm->getWorkspace()->findFirstChildByName("ParentPart"));
        BOOST_REQUIRE(parentPart);

        parentPart->setTranslationUi(Vector3(1000000, 0, 0));
        childPart->setTranslationUi(Vector3(1000000, 2, 0));
        BOOST_MESSAGE("move both parent and child parts away");
    }

    // replication
    G3D::System::sleep(SLEEPTIME);

    {
        DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

        SetMemoryLevel(clientDm);

        // move character to trigger GC
        RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
        RBXASSERT(players);
        RBX::Network::Player* player = players->getLocalPlayer();
        RBXASSERT(player);
        ModelInstance* character = player->getCharacter();
        PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));

        RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
        BOOST_MESSAGE("move player to trigger GC");
        torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3(100, 5, 0)));

    }

    // wait for client to catch up
    G3D::System::sleep(SLEEPTIME);

    {
        DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

        BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("BasePlate"));
        BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("ParentPart") == NULL);
    }

    NetworkSettings::singleton().setExtraMemoryUsedInMB(0);
}

BOOST_AUTO_TEST_CASE(ReparentingToGcedPartCausesInstanceToLeave)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(0, -2.4f, 0);

	DataModelFixture serverDm;
	shared_ptr<PartInstance> partInsideStreamedArea;
	shared_ptr<PartInstance> partToBeOutsideStreamedArea;
	shared_ptr<SpecialShape> specialMesh;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);
		serverDm->getWorkspace()->createTerrain();

		partInsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partInsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(9, 9, 0));
		partInsideStreamedArea->setAnchored(true);
		partInsideStreamedArea->setName("Inside");
		partInsideStreamedArea->setParent(serverDm->getWorkspace());

		partToBeOutsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partToBeOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(-9, 9, 0));
		partToBeOutsideStreamedArea->setAnchored(true);
		partToBeOutsideStreamedArea->setName("Outside");
		partToBeOutsideStreamedArea->setParent(serverDm->getWorkspace());

		// put special mesh under part inside streamed area
		specialMesh = Creatable<Instance>::create<SpecialShape>();
		specialMesh->setName("Mesh");
		specialMesh->setParent(partInsideStreamedArea.get());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// create base plate on client so player is guaranteed to land on it
		shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
		basePlate->setTranslationUi(baseplateLocation);
		basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
		basePlate->setAnchored(true);
		basePlate->setName("BasePlate");
		basePlate->setParent(clientDm->getWorkspace());
	}
	NetworkFixture::startClient(clientDm, true);

    // spawning player triggers the initial gc checker in gc job step so we need give it some time before shrinking the memory
    G3D::System::sleep(SLEEPTIME);

	NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

		// Sanity check that inside part is present and outside part is not
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside"));
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		partToBeOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(1000000, 0, 0));
	}

    // replication
    G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

		// move character to trigger GC
		RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
		RBXASSERT(players);
		RBX::Network::Player* player = players->getLocalPlayer();
		RBXASSERT(player);
		ModelInstance* character = player->getCharacter();
		PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
		
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
		BOOST_MESSAGE("move player to trigger GC");
		torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3(100, 5, 0)));
	}

	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// sanity check part is streamed out
		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		specialMesh->setParent(partToBeOutsideStreamedArea.get());
	}

	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		Instance* inside = clientDm->getWorkspace()->findFirstChildByName("Inside");
		BOOST_REQUIRE(inside);

		// check that the reparented mesh is no longer under "Inside"
		BOOST_CHECK(inside->findFirstChildByName("Mesh") == NULL);
	}

	NetworkSettings::singleton().setExtraMemoryUsedInMB(0);
}

BOOST_AUTO_TEST_CASE(ReparentingFromGcedPartToStreamedPartCausesInstanceToEnter)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(0, -2.4f, 0);

	DataModelFixture serverDm;
	shared_ptr<PartInstance> partInsideStreamedArea;
	shared_ptr<PartInstance> partToBeOutsideStreamedArea;
	shared_ptr<SpecialShape> specialMesh;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);
		serverDm->getWorkspace()->createTerrain();

		partInsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partInsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(9, 9, 0));
		partInsideStreamedArea->setAnchored(true);
		partInsideStreamedArea->setName("Inside");
		partInsideStreamedArea->setParent(serverDm->getWorkspace());

		partToBeOutsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partToBeOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(-9, 9, 0));
		partToBeOutsideStreamedArea->setAnchored(true);
		partToBeOutsideStreamedArea->setName("Outside");
		partToBeOutsideStreamedArea->setParent(serverDm->getWorkspace());

		// put special mesh under part inside streamed area
		specialMesh = Creatable<Instance>::create<SpecialShape>();
		specialMesh->setName("Mesh");
		specialMesh->setParent(partToBeOutsideStreamedArea.get());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// create base plate on client so player is guaranteed to land on it
		shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
		basePlate->setTranslationUi(baseplateLocation);
		basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
		basePlate->setAnchored(true);
		basePlate->setName("BasePlate");
		basePlate->setParent(clientDm->getWorkspace());
	}
	NetworkFixture::startClient(clientDm, true);

    // spawning player triggers the initial gc checker in gc job step so we need give it some time before shrinking the memory
    G3D::System::sleep(SLEEPTIME);

	NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

		// Sanity check that inside part is present and outside part is not
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside"));
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		partToBeOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(1000000, 0, 0));
	}

    // replication
    G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

		// move character to trigger GC
		RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
		RBXASSERT(players);
		RBX::Network::Player* player = players->getLocalPlayer();
		RBXASSERT(player);
		ModelInstance* character = player->getCharacter();
		PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
		
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
		BOOST_MESSAGE("move player to trigger GC");
		torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3(100, 5, 0)));
	}

	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// sanity check part is streamed out
		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);
	}

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		specialMesh->setParent(partInsideStreamedArea.get());
	}

	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		Instance* inside = clientDm->getWorkspace()->findFirstChildByName("Inside");
		BOOST_REQUIRE(inside);

		// check that the reparented mesh is now present under "Inside"
		BOOST_CHECK(inside->findFirstChildByName("Mesh"));
	}

	NetworkSettings::singleton().setExtraMemoryUsedInMB(0);
}

BOOST_AUTO_TEST_CASE(ShipsInTheNightReparentingAndGc)
{
	ScopedAssign<bool> disableSound(RBX::Soundscape::SoundService::soundDisabled, true);

	const Vector3 baseplateLocation(0, -2.4f, 0);

	DataModelFixture serverDm;
	shared_ptr<PartInstance> partInsideStreamedArea;
	shared_ptr<PartInstance> partToBeOutsideStreamedArea;
	shared_ptr<SpecialShape> specialMeshOutsideToInside;
	shared_ptr<SpecialShape> specialMeshInsideToOutside;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

		Workspace::prop_StreamingEnabled.set(serverDm->getWorkspace(), true);
		serverDm->getWorkspace()->createTerrain();

		partInsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partInsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(9, 9, 0));
		partInsideStreamedArea->setAnchored(true);
		partInsideStreamedArea->setName("Inside");
		partInsideStreamedArea->setParent(serverDm->getWorkspace());

		partToBeOutsideStreamedArea = Creatable<Instance>::create<BasicPartInstance>();
		partToBeOutsideStreamedArea->setTranslationUi(baseplateLocation + Vector3(-9, 9, 0));
		partToBeOutsideStreamedArea->setAnchored(true);
		partToBeOutsideStreamedArea->setName("Outside");
		partToBeOutsideStreamedArea->setParent(serverDm->getWorkspace());

		specialMeshOutsideToInside = Creatable<Instance>::create<SpecialShape>();
		specialMeshOutsideToInside->setName("MeshO2I");
		specialMeshOutsideToInside->setParent(partToBeOutsideStreamedArea.get());

		specialMeshInsideToOutside = Creatable<Instance>::create<SpecialShape>();
		specialMeshInsideToOutside->setName("MeshI2O");
		specialMeshInsideToOutside->setParent(partInsideStreamedArea.get());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// create base plate on client so player is guaranteed to land on it
		shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
		basePlate->setTranslationUi(baseplateLocation);
		basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
		basePlate->setAnchored(true);
		basePlate->setName("BasePlate");
		basePlate->setParent(clientDm->getWorkspace());
	}
	NetworkFixture::startClient(clientDm, true);

    // spawning player triggers the initial gc checker in gc job step so we need give it some time before shrinking the memory
    G3D::System::sleep(SLEEPTIME);

    NetworkSettings::singleton().setExtraMemoryUsedInMB(MemoryStats::totalMemoryBytes()/1024/1024);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		SetMemoryLevel(clientDm);

		// Sanity check that both parts are present in client
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside"));
		BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Outside"));
	}
	{
		// set network delay to ensure that packets are queued up
		ScopedAssign<double> assignReplicationLag(NetworkSettings::singleton().incommingReplicationLag, 10000);
		{
			DataModel::LegacyLock l(&serverDm, DataModelJob::Write);

            BOOST_MESSAGE("reparent MeshO2I to partInside");
			specialMeshOutsideToInside->setParent(partInsideStreamedArea.get());
            BOOST_MESSAGE("reparent MeshI2O to partToBeOutside");
			specialMeshInsideToOutside->setParent(partToBeOutsideStreamedArea.get());
		}
		{
			DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

			SetMemoryLevel(clientDm);

			PartInstance* partToBeMoved = Instance::fastDynamicCast<PartInstance>(
				clientDm->getWorkspace()->findFirstChildByName("Outside"));

            BOOST_CHECK(partToBeMoved->findFirstChildByName("MeshO2I"));
            BOOST_WARN(partToBeMoved->findFirstChildByName("MeshI2O") == NULL);

            partToBeMoved->setTranslationUi(baseplateLocation + Vector3(100000, 100000, 0));

			// move character to trigger GC
			RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
			RBXASSERT(players);
			RBX::Network::Player* player = players->getLocalPlayer();
			RBXASSERT(player);
			ModelInstance* character = player->getCharacter();
			PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
			
			RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
			BOOST_MESSAGE("move player to trigger GC");
			torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3(100, 5, 0)));
		}

        BOOST_MESSAGE("wait for gc");
		G3D::System::sleep(SLEEPTIME);

		{
			DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

			// sanity check that the right part has been gc'ed
			BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);
            BOOST_REQUIRE(clientDm->getWorkspace()->findFirstChildByName("Inside"));
		}
	} // clear network delay by leaving scope

    BOOST_MESSAGE("wait for replications");
	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		// Check that the mesh swap was interpreted correctly on the client
		BOOST_WARN(clientDm->getWorkspace()->findFirstChildByName("Outside") == NULL);
		Instance* remainingPart = clientDm->getWorkspace()->findFirstChildByName("Inside");
		BOOST_REQUIRE(remainingPart);
		BOOST_CHECK(remainingPart->findFirstChildByName("MeshO2I"));
		BOOST_CHECK(remainingPart->findFirstChildByName("MeshI2O") == NULL);
	}

	NetworkSettings::singleton().setExtraMemoryUsedInMB(0);
}


BOOST_AUTO_TEST_SUITE_END()
