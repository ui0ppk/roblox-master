#include <boost/test/unit_test.hpp>
#include "v8datamodel/FastLogSettings.h"
#include "util/FileSystem.h"
#include "util/Statistics.h"
#include "FastLog.h"
#include "rbx/test/DataModelFixture.h"

#include <iostream>


LOGVARIABLE(TestLogGroup,0)
FASTFLAGVARIABLE(TestFastFlag,false)


BOOST_AUTO_TEST_SUITE(FastLogSettings)

class TestJSON : public RBX::FastLogJSON
{
};

BOOST_AUTO_TEST_CASE(Serializer)
{
	BOOST_CHECK_EQUAL(FLog::TestLogGroup, 0);
	BOOST_CHECK_EQUAL(FFlag::TestFastFlag, false);

	boost::filesystem::path jsonFolder = RBX::FileSystem::getUserDirectory(true, RBX::DirExe, "ClientSettings") / "Test.json";
	
	std::ofstream localJSON;
	localJSON.open(jsonFolder.native().c_str());	
	localJSON << "{\n\"FLogTestLogGroup\":1,\n\"FFlagTestFastFlag\":true}";
	localJSON.close();

	TestJSON destJSON;

	FetchClientSettingsData("Test", "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", &destJSON);

	BOOST_CHECK_EQUAL(FLog::TestLogGroup, 1);
	BOOST_CHECK_EQUAL(FFlag::TestFastFlag, true);
}

FASTFLAGVARIABLE(TestTrueValue, true)
FASTFLAGVARIABLE(TestFalseValue, false)

BOOST_FIXTURE_TEST_CASE(Lua, DataModelFixture)
{
	BOOST_CHECK_THROW(execute("return settings():GetFastVariable('ThisFlagDoesNotExist')"), std::exception);
	std::auto_ptr<RBX::Reflection::Tuple> result = execute("return settings():GetFVariable('TestTrueValue'), settings():GetFVariable('TestFalseValue'), settings():GetFFlag('TestTrueValue'), settings():GetFFlag('TestFalseValue')");
	BOOST_CHECK_EQUAL(result->values.at(0).convert<std::string>(), "true");
	BOOST_CHECK_EQUAL(result->values.at(1).convert<std::string>(), "false");
	BOOST_CHECK_EQUAL(result->values.at(2).convert<bool>(), true);
	BOOST_CHECK_EQUAL(result->values.at(3).convert<bool>(), false);
}

BOOST_AUTO_TEST_SUITE_END()
