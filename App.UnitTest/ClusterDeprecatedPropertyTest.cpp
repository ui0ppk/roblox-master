#include <boost/test/unit_test.hpp>
#include <exception>

#include "rbx/test/DataModelFixture.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"

using namespace RBX;

void dataModelSetup(DataModelFixture& dataModel) {
	DataModel::LegacyLock lock(dataModel.dataModel.get(), DataModelJob::Write);
	dataModel->getWorkspace()->createTerrain();
}

BOOST_AUTO_TEST_SUITE( ClusterDeprecatedProperty )

BOOST_AUTO_TEST_CASE( AnchoredCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.Anchored");
	BOOST_REQUIRE_EQUAL(true, resultTuple->at(0).get<bool>());
	
	dataModel.execute("Workspace.Terrain.Anchored = false");
	
	resultTuple = dataModel.execute("return Workspace.Terrain.Anchored");
	BOOST_REQUIRE_EQUAL(true, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_CASE( CFrameAndPositionCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	Vector3 zeroVector = Vector3();
	CoordinateFrame zeroFrame(zeroVector);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.CFrame");
	BOOST_CHECK(zeroFrame == resultTuple->at(0).get<CoordinateFrame>());
	resultTuple = dataModel.execute("return Workspace.Terrain.Position");
	BOOST_REQUIRE_EQUAL(zeroVector, resultTuple->at(0).get<Vector3>());
	
	dataModel.execute("Workspace.Terrain.CFrame = CFrame.new(11,12,13,1,2,3,4,5,6,7,8,9)");
	
	resultTuple = dataModel.execute("return Workspace.Terrain.CFrame");
	BOOST_REQUIRE(zeroFrame == resultTuple->at(0).get<CoordinateFrame>());
	resultTuple = dataModel.execute("return Workspace.Terrain.Position");
	BOOST_REQUIRE_EQUAL(zeroVector, resultTuple->at(0).get<Vector3>());
}

BOOST_AUTO_TEST_CASE( NameCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.Name");
	BOOST_REQUIRE_EQUAL("Terrain", resultTuple->at(0).get<std::string>());
	
	dataModel.execute("Workspace.Terrain.Name = 'MajelBarrett'");
	
	resultTuple = dataModel.execute("return Workspace.Terrain.Name");
	BOOST_REQUIRE_EQUAL("Terrain", resultTuple->at(0).get<std::string>());
}

BOOST_AUTO_TEST_CASE( ParentCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.Parent");
	BOOST_REQUIRE_EQUAL(dataModel->getWorkspace(), resultTuple->at(0).get<shared_ptr<Instance> >().get());

	dataModel.execute("Instance.new('Part', Workspace)");
	bool checked = false;
	try {
		dataModel.execute("Workspace.Terrain.Parent = Workspace.Part");
	} catch (std::runtime_error& e) {
		BOOST_CHECK_EQUAL("Cannot change Parent of type Terrain", e.what());
		checked = true;
	}
	BOOST_CHECK(checked);

	resultTuple = dataModel.execute("return Workspace.Terrain.Parent");
	BOOST_REQUIRE_EQUAL(dataModel->getWorkspace(), resultTuple->at(0).get<shared_ptr<Instance> >().get());
}

BOOST_AUTO_TEST_CASE( SizeCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	Vector3 sizeVector = Vector3(2044, 252, 2044);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.Size");
	BOOST_REQUIRE_EQUAL(sizeVector, resultTuple->at(0).get<Vector3>());

	dataModel.execute("Workspace.Terrain.Size = Vector3.new(10,20,30)");

	resultTuple = dataModel.execute("return Workspace.Terrain.Size");
	BOOST_REQUIRE_EQUAL(sizeVector, resultTuple->at(0).get<Vector3>());

	bool checked = false;
	try {
		dataModel.execute("Workspace.Terrain:Resize(0, 10)");
	} catch (std::runtime_error& e) {
		BOOST_CHECK_EQUAL("Cannot Resize() Terrain", e.what());
		checked = true;
	}
	BOOST_CHECK(checked);

	resultTuple = dataModel.execute("return Workspace.Terrain.Size");
	BOOST_REQUIRE_EQUAL(sizeVector, resultTuple->at(0).get<Vector3>());
}

BOOST_AUTO_TEST_CASE( LockedCannotBeSet )
{
	DataModelFixture dataModel;
	dataModelSetup(dataModel);

	std::auto_ptr<Reflection::Tuple> resultTuple;
	resultTuple = dataModel.execute("return Workspace.Terrain.Locked");
	BOOST_REQUIRE_EQUAL(true, resultTuple->at(0).get<bool>());

	dataModel.execute("Workspace.Terrain.Locked = false");

	resultTuple = dataModel.execute("return Workspace.Terrain.Locked");
	BOOST_REQUIRE_EQUAL(true, resultTuple->at(0).get<bool>());
}


BOOST_AUTO_TEST_SUITE_END()
