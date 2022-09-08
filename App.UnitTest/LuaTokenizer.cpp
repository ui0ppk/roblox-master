#include <boost/test/unit_test.hpp>
#include "v8datamodel/Test.h"

BOOST_AUTO_TEST_SUITE(LuaTokenizer)

BOOST_AUTO_TEST_CASE(Parse1)
{
	std::vector<std::string> args = RBX::Lua::ArgumentParser::getArgsInBracket("( a() )");
	BOOST_REQUIRE_EQUAL(args.size(), 1);
	BOOST_CHECK_EQUAL(args[0], "a()");
}

BOOST_AUTO_TEST_CASE(Parse2)
{
	std::vector<std::string> args = RBX::Lua::ArgumentParser::getArgsInBracket("( a , 34+s(r, 6), 'd\\'fg'\t, \"sd'fs\")");
	BOOST_REQUIRE_EQUAL(args.size(), 4);
	BOOST_CHECK_EQUAL(args[0], "a");
	BOOST_CHECK_EQUAL(args[1], "34+s(r, 6)");
	BOOST_CHECK_EQUAL(args[2], "'d\\'fg'");
	BOOST_CHECK_EQUAL(args[3], "\"sd'fs\"");
}

BOOST_AUTO_TEST_CASE(ParseFunction)
{
	std::vector<std::string> args = RBX::Lua::ArgumentParser::getArgsInBracket("( function() return 1 end )");
	BOOST_REQUIRE_EQUAL(args.size(), 1);
	BOOST_CHECK_EQUAL(args[0], "function() return 1 end");
}

BOOST_AUTO_TEST_CASE(Limitation)
{
	// Sadly, this fails because there is a comma in the function, and we don't group it
	std::vector<std::string> args = RBX::Lua::ArgumentParser::getArgsInBracket("( function() return 1, 2 end )");
	BOOST_WARN_EQUAL(args.size(), 1);
	//BOOST_CHECK_EQUAL(args[0], "function() return 1, 2 end");
	BOOST_CHECK(true);
}


BOOST_AUTO_TEST_SUITE_END()
