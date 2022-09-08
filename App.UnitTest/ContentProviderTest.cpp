#include <boost/test/unit_test.hpp>

#include "v8datamodel/ContentProvider.h"
#include "util/Statistics.h"

DYNAMIC_FASTFLAG(UrlReconstructToAssetGame)

using namespace RBX;

class TestProvider
{
public:
	static void testContentIdAssetUrlReconstruction(std::string raw_url, const std::string& expected_sanitized_url, bool expected_result)
	{
		ContentId id = ContentId::fromUrl(raw_url);
		
		BOOST_CHECK_MESSAGE(id.reconstructAssetUrl(GetBaseURL()) == expected_result, raw_url);

		if (expected_result)
			BOOST_CHECK_EQUAL(expected_sanitized_url, id.toString());
	}
};

#if RBX_PLATFORM_IOS
virtual void onHeartbeat(const Heartbeat& event)
{
}
#endif

BOOST_AUTO_TEST_SUITE( ContentProvider )

BOOST_AUTO_TEST_CASE( UrlReconstruction)
{
	::SetBaseURL("http://www.roblox.com");

	std::string host = "http://www.roblox.com";
	if (DFFlag::UrlReconstructToAssetGame)
		host = "https://assetgame.roblox.com";

	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/thumbs/asset.ashx?id=123", host + "/thumbs/asset.ashx?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/thumbs/avatar.ashx?id=123", host + "/thumbs/avatar.ashx?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction(
		"http://www.roblox.com/Game/Tools/ThumbnailAsset.ashx?fmt=png&wd=75&ht=75&aid=56450668",
		host + "/Game/Tools/ThumbnailAsset.ashx?fmt=png&wd=75&ht=75&aid=56450668", true);

	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/?id=123&version=1", host + "/asset/?id=123&version=1", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/ /asset/   ?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/?id=123   ", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset /?id=1 23", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset////?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com///asset/?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com////asset////?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com//asset/?id=123", host + "/asset/?id=123", true);
	
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/blah/../?id=123  ", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/../asset/?id=1234", host + "/asset/?id=1234", true);

	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/../?id=123  ", "", false);	
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/blah/?id=123&version=1", "", false);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/asset/id=123&version=1", "", false);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com/?id=123", "", false);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.roblox.com", "", false);
	TestProvider::testContentIdAssetUrlReconstruction(
		"http://www.roblox.com/asset/../Bloxxer-Cap-item?id=51353039"
		"&__EVENTTARGET=&__EVENTARGUMENT=&ctl00%24cphRoblox"
		"%24ProceedWithTicketsPurchaseButton=Buy%20now!",
		"", false);

	TestProvider::testContentIdAssetUrlReconstruction("http://www.notroblox.com/asset/?id=123", host + "/asset/?id=123", true);

	::SetBaseURL("http://www.gametest1.robloxlabs.com");
	host = "http://www.gametest1.robloxlabs.com";
	if (DFFlag::UrlReconstructToAssetGame)
		host = "https://assetgame.gametest1.robloxlabs.com";

	TestProvider::testContentIdAssetUrlReconstruction("http://www.gametest1.robloxlabs.com/asset/?id=123", host + "/asset/?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.gametest1.robloxlabs.com/thumbs/asset.ashx?id=123", host + "/thumbs/asset.ashx?id=123", true);
	TestProvider::testContentIdAssetUrlReconstruction("http://www.gametest1.robloxlabs.com/thumbs/avatar.ashx?id=123", host + "/thumbs/avatar.ashx?id=123", true);
    
    // Non-http ids
    TestProvider::testContentIdAssetUrlReconstruction("rbxasset://fonts/character3.rbxm", "rbxasset://fonts/character3.rbxm", true);
    TestProvider::testContentIdAssetUrlReconstruction("rbxgameasset://fonts/character3.rbxm", "rbxgameasset://fonts/character3.rbxm", true);
    //TestProvider::testContentIdAssetUrlReconstruction("/asset/?id=11911558", "", false);
    //TestProvider::testContentIdAssetUrlReconstruction("/Asset/?id=180435571&serverplaceid=0&clientinsert=1", "", false);
}

BOOST_AUTO_TEST_SUITE_END()
