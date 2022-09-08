#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( LuaVector3Test )

BOOST_AUTO_TEST_CASE( IsCloseNumArgs ) {
	DataModelFixture dataModel;
	RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
	std::auto_ptr<Reflection::Tuple> resultTuple;

	BOOST_CHECK_THROW(dataModel.execute("return Vector3.new(0,0,0):isClose()"), std::runtime_error);

	BOOST_CHECK_THROW(dataModel.execute("return Vector3.new(.00002,0,0):isClose()"), std::runtime_error);

	resultTuple = dataModel.execute("return Vector3.new(1,2,3):isClose(Vector3.new(1,2,3))");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return Vector3.new(1.00001,2,3):isClose(Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_CASE( IsClosePrecision ) {
	DataModelFixture dataModel;
	RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
	std::auto_ptr<Reflection::Tuple> resultTuple;

	// default precision
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00001,2,3):isClose("
		"Vector3.new(1,2,3))");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	// zero epsilon
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00001,2,3):isClose("
		"Vector3.new(1,2,3), 0)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00002,2,3):isClose("
		"Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00002,2,3):isClose("
		"Vector3.new(1,2,3), .00002)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(1,2.00003,3):isClose("
		"Vector3.new(1,2,3), .00002)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(1,2,3):isClose("
		"Vector3.new(1,2.00003,3), .00002)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	// big epsilon
	resultTuple = dataModel.execute("return "
		"Vector3.new(1,2,3):isClose("
		"Vector3.new(2,3,4), 1)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_CASE( NegativeValues ) {
	DataModelFixture dataModel;
	RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
	std::auto_ptr<Reflection::Tuple> resultTuple;

	resultTuple = dataModel.execute("return "
		"Vector3.new(-1,-2,-3):isClose("
		"Vector3.new(-1,-2,-3), .00001)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(-1.00001,-2,-3):isClose("
		"Vector3.new(-1,-2,-3), .00001)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(-1,-2.00005,-3):isClose("
		"Vector3.new(-1,-2,-3), .00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_CASE( MultipleAxisDiff ) {
	DataModelFixture dataModel;
	RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
	std::auto_ptr<Reflection::Tuple> resultTuple;

	// x in y in
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00001,2.00002,3):isClose("
		"Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	// x out y in
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00005,2.00002,3):isClose("
		"Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());

	// x in y out
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00001,2.00005,3):isClose("
		"Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());

	// x out y out
	resultTuple = dataModel.execute("return "
		"Vector3.new(1.00005,2.00005,3):isClose("
		"Vector3.new(1,2,3), .00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_CASE( NegativeEpsilon ) {
		DataModelFixture dataModel;
	RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
	std::auto_ptr<Reflection::Tuple> resultTuple;

	resultTuple = dataModel.execute("return "
		"Vector3.new(1,2,3):isClose("
		"Vector3.new(1,2,3), -.00001)");
	BOOST_CHECK_EQUAL(true, resultTuple->at(0).get<bool>());

	resultTuple = dataModel.execute("return "
		"Vector3.new(1,2.00005,3):isClose("
		"Vector3.new(1,2,3), -.00001)");
	BOOST_CHECK_EQUAL(false, resultTuple->at(0).get<bool>());
}

BOOST_AUTO_TEST_SUITE_END()
