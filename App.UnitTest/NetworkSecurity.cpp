#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "v8dataModel/BasicPartInstance.h"
#include "v8dataModel/Workspace.h"
#include "v8dataModel/Hopper.h"
#include "script/script.h"
#include "network/Players.h"

using namespace RBX;

BOOST_GLOBAL_FIXTURE(NetworkFixture)

BOOST_AUTO_TEST_SUITE(NetworkSecurity)

BOOST_AUTO_TEST_CASE(PropertyFilter)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName("TestPart");
		serverPart->setParent(serverDm->getWorkspace());
	}

	// Wait for it to replicate
	G3D::System::sleep(2);

	// Confirm the client got it
	PartInstance* clientPart;
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart = boost::polymorphic_downcast<PartInstance*>(clientDm->getWorkspace()->findFirstChildByName("TestPart"));
		BOOST_REQUIRE(clientPart);
	}

	// Hook up a dummy filter that rejects everything
	serverDm.execute("rep = game:GetService('NetworkServer'):GetChildren()[1] rep.PropertyFilter = function(_, member) if member == 'Reflectance' then return Enum.FilterResult.Accepted else return Enum.FilterResult.Rejected end end");

	// Try to replicate the property
	clientDm.execute("Workspace.TestPart.Transparency = 0.5 Workspace.TestPart.Reflectance = 0.5");

	// Wait for it to replicate
	G3D::System::sleep(2);

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getTransparencyUi(), 0);
		BOOST_CHECK_EQUAL(serverPart->getReflectance(), 0.5);
	}
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		// Property bounce-back isn't part of security filtering
		BOOST_WARN_EQUAL(clientPart->getTransparencyUi(), 0);
	}
}


BOOST_AUTO_TEST_CASE(NewFilter)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Wait for connection to be made
	G3D::System::sleep(2);

	// Hook up a filter
	serverDm.execute("rep = game:GetService('NetworkServer'):GetChildren()[1] rep.NewFilter = function(item) if item.Name == 'Accepted' then return Enum.FilterResult.Accepted else return Enum.FilterResult.Rejected end end");

	// Try to replicate an instance
	clientDm.execute("p = Instance.new('Part') p.Name = 'Accepted' p.Parent = Workspace");
	clientDm.execute("p = Instance.new('Part') p.Name = 'Rejected' p.Parent = Workspace");

	// Wait for it to replicate
	G3D::System::sleep(2);

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK(serverDm->getWorkspace()->findFirstChildByName("Accepted") != NULL);
		BOOST_CHECK(serverDm->getWorkspace()->findFirstChildByName("Rejected") == NULL);
	}
}


BOOST_AUTO_TEST_CASE(DeleteFilter)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create instances
	serverDm.execute("p = Instance.new('Part') p.Name = 'Accepted' p.Parent = Workspace");
	serverDm.execute("p = Instance.new('Part') p.Name = 'Rejected' p.Parent = Workspace");

	// Wait for it to replicate
	G3D::System::sleep(2);

	// Hook up a filter
	serverDm.execute("rep = game:GetService('NetworkServer'):GetChildren()[1] rep.DeleteFilter = function(item) if item.Name == 'Accepted' then return Enum.FilterResult.Accepted else return Enum.FilterResult.Rejected end end");

	clientDm.execute("Workspace.Accepted:remove()");
	clientDm.execute("Workspace.Rejected:remove()");

	// Wait for it to replicate
	G3D::System::sleep(2);

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK(serverDm->getWorkspace()->findFirstChildByName("Accepted") == NULL);
		BOOST_CHECK(serverDm->getWorkspace()->findFirstChildByName("Rejected") != NULL);
	}
}


BOOST_AUTO_TEST_CASE(ServerScriptProtection)
{
	// Server scripts should not be replicated in a readable fashion to the client
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	const char* code = "print('hello world')";

	// Create scripts on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<HopperBin> hopperBin = Creatable<Instance>::create<HopperBin>(); 
		hopperBin->setParent(ServiceProvider::create<StarterPackService>(&serverDm));
		shared_ptr<Script> serverScript = Creatable<Instance>::create<Script>();
		serverScript->setName("HopperBinScript");
		serverScript->setEmbeddedCode(ProtectedString::fromTestSource(code));
		serverScript->setParent(hopperBin.get());

		serverScript = Creatable<Instance>::create<Script>();
		serverScript->setName("TestScript");
		serverScript->setEmbeddedCode(ProtectedString::fromTestSource(code));
		serverScript->setParent(serverDm->getWorkspace());


	}

	// Wait for it to replicate
	G3D::System::sleep(2);

	// Confirm the client got them
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		Script* clientScript = dynamic_cast<Script*>(clientDm->getWorkspace()->findFirstChildByName("TestScript"));
		BOOST_REQUIRE(clientScript);
		// The client should NOT have the source code
        BOOST_WARN_NE(clientScript->getEmbeddedCodeSafe().getSource(), code);
	}

	// Clone the script client-side
	clientDm.execute("s = workspace.TestScript:Clone() s.Name = 'TestScriptCopy' s.Parent = workspace");

	// Wait for it to replicate
	G3D::System::sleep(2);

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		Script* copiedScript = dynamic_cast<Script*>(serverDm->getWorkspace()->findFirstChildByName("TestScriptCopy"));
		BOOST_REQUIRE(copiedScript);
		// The server should have the source code
		BOOST_CHECK_EQUAL(copiedScript->getEmbeddedCodeSafe().getSource(), code);
	}
}

BOOST_AUTO_TEST_CASE(SecurePlayerReplication)
{
	// Player properties should not be changed from client
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm, true);

	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(&clientDm);
		BOOST_REQUIRE(player);

		RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
		player->setName("hacked");
		player->setUserId(100);

	}

	// wait for replication
	G3D::System::sleep(2);

	{
		RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&serverDm);
		BOOST_REQUIRE(players);

		RBX::Network::Player* player = Instance::fastDynamicCast<RBX::Network::Player>(players->getChild(0));

		BOOST_CHECK_EQUAL(player->getName(), "Player1");
		BOOST_CHECK_EQUAL(player->getUserID(), -1);
	}
}


BOOST_AUTO_TEST_SUITE_END()

