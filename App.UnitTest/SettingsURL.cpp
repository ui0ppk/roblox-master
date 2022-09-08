#include <boost/test/unit_test.hpp>

#include "RobloxServicesTools.h"

BOOST_AUTO_TEST_SUITE(SettingsUrl)

BOOST_AUTO_TEST_CASE(SettingsUrlProd)
{
	std::string testUrl = "http://clientsettings.api.roblox.com/Setting/QuietGet/Test/?apiKey=TestKey";
	std::string url = GetSettingsUrl("www.roblox.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);

	url = GetSettingsUrl("http://www.roblox.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);
}

BOOST_AUTO_TEST_CASE(SettingsUrlNewGametest)
{
	std::string testUrl = "http://clientsettings.api.gametest1.robloxlabs.com/Setting/QuietGet/Test/?apiKey=TestKey";
	std::string url = GetSettingsUrl("www.gametest1.robloxlabs.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);

	url = GetSettingsUrl("http://www.gametest1.robloxlabs.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);
}

BOOST_AUTO_TEST_CASE(SettingsUrlGametest)
{
	std::string testUrl = "http://clientsettings.api.gametest1.roblox.com/Setting/QuietGet/Test/?apiKey=TestKey";
	std::string url = GetSettingsUrl("www.gametest1.roblox.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);

	url = GetSettingsUrl("http://www.gametest1.roblox.com", "Test", "TestKey");
	BOOST_CHECK_EQUAL(url, testUrl);
}

BOOST_AUTO_TEST_SUITE_END()
