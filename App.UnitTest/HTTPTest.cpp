/*
 *  HTTP.cpp
 *  MacClient
 *
 *  Created by Rick Kimball on 12/7/10.
 *  Copyright 2010 Roblox Inc. All rights reserved.
 *
 */
#include "FastLog.h"

#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>
#include "Util/Http.h"
#include "Util/Guid.h"
#include <sstream>
#include "rbx/test/ScopedFastFlagSetting.h"
#include "rbx/test/test_tools.h"
#include "rbx/test/DataModelFixture.h"

#include "Server.h"
#include "V8Xml/WebParser.h"
#include "v8datamodel/HttpRbxApiService.h"

BOOST_AUTO_TEST_SUITE(HTTPSuite)

static void testGet(RBX::Http& http, bool externalRequest = false)
{
	std::string response1a;
	http.get(response1a, externalRequest);

    if (externalRequest)
    {
        BOOST_CHECK_EQUAL(std::string::npos != response1a.find("\"url\": \"http://httpbin.org/get\""), true);
    }
    else
    {
        BOOST_CHECK_EQUAL(response1a.substr(0, 6),"<html>");
    }
}

static void testGetFailure(RBX::Http& http, bool externalRequest)
{
	std::string response1a;
	BOOST_CHECK_THROW(http.get(response1a, externalRequest), std::exception);
}

static void testPost(RBX::Http& http)
{
    std::stringstream formdata;
    formdata << "test=fame";

    std::string response;
    http.post(formdata, RBX::Http::kContentTypeUrlEncoded, false, response, true);
    BOOST_CHECK_EQUAL(std::string::npos != response.find("\"test\": \"fame\""), true);
}

std::string guidify(std::string url)
{
	if (url.find('?') == std::string::npos)
		return url + "?" + RBX::Guid().readableString(12);
	else
		return url + "&" + RBX::Guid().readableString(12);
}

static void PostAsyncSuccess(std::string response)
{
	BOOST_CHECK_EQUAL(response, "\"RGS OK\"");
}
static void PostAsyncError(std::string error)
{
	BOOST_CHECK_EQUAL(true, false);
}

static void GetAsyncSuccess(std::string response)
{
	shared_ptr<const RBX::Reflection::ValueTable> table(rbx::make_shared<const RBX::Reflection::ValueTable>());
	RBX::WebParser::parseJSONTable(response, table);

	RBX::Reflection::Variant assetVariant = table->at("AssetId");
	if (assetVariant.isType<int>())
	{
		int asset = assetVariant.cast<int>();
		BOOST_CHECK_EQUAL(asset, 28277486);
	}

	RBX::Reflection::Variant productVariant = table->at("ProductId");
	if (productVariant.isType<int>())
	{
		int product = productVariant.cast<int>();
		BOOST_CHECK_EQUAL(product, 4678951);
	}
}
static void GetAsyncError(std::string error)
{
	BOOST_CHECK_EQUAL(true, false);
}

BOOST_AUTO_TEST_CASE(HTTP_SYNC_GET_TESTS)
{
	{
		// Make a synchronous get request expecting data
		{
			RBX::Http a(guidify("http://www.roblox.com/Info/EULA.htm"));
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(a), false), RBX::Time::Interval(10));
			RBX::Http b(guidify("http://www.roblox.com/Info/EULA.htm"), RBX::Http::WinHttp);
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(b), false), RBX::Time::Interval(10));
			RBX::Http c(guidify("http://www.roblox.com/Info/EULA.htm"), RBX::Http::WinInet);
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(c), false), RBX::Time::Interval(10));
		}
	
		{
			RBX::Http a(guidify("http://blahblah.roblox.com/this_should_fail.php"));
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(a), false), RBX::Time::Interval(10));
			RBX::Http b(guidify("http://blahblah.roblox.com/this_should_fail.php"), RBX::Http::WinHttp);
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(b), false), RBX::Time::Interval(10));
			RBX::Http c(guidify("http://blahblah.roblox.com/this_should_fail.php"), RBX::Http::WinInet);
			RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(c), false), RBX::Time::Interval(10));
		}
		
		// Need test for iostream read
		
	}
}

BOOST_AUTO_TEST_CASE(HTTP_SYNC_GET_EXTERNAL_TESTS)
{
    {
        RBX::Http a("http://httpbin.org/get");
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(a), true), RBX::Time::Interval(10));
        RBX::Http b("http://httpbin.org/get", RBX::Http::WinHttp);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(b), true), RBX::Time::Interval(10));
        RBX::Http c("http://httpbin.org/get", RBX::Http::WinInet);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGet, boost::ref(c), true), RBX::Time::Interval(10));
    }

    {
        RBX::Http a("http://httpbin.org/status/404");
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(a), true), RBX::Time::Interval(10));
        RBX::Http b("http://httpbin.org/status/404", RBX::Http::WinHttp);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(b), true), RBX::Time::Interval(10));
        RBX::Http c("http://httpbin.org/status/404", RBX::Http::WinInet);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testGetFailure, boost::ref(c), true), RBX::Time::Interval(10));
    }
}

BOOST_AUTO_TEST_CASE(HTTP_SYNC_POST_EXTERNAL_TESTS)
{
    {
        std::string response;
        RBX::Http a("http://httpbin.org/post");
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testPost, boost::ref(a)), RBX::Time::Interval(10));
        RBX::Http b("http://httpbin.org/post", RBX::Http::WinHttp);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testPost, boost::ref(b)), RBX::Time::Interval(10));
        RBX::Http c("http://httpbin.org/post", RBX::Http::WinInet);
        RBX_TEST_WITH_TIMEOUT(boost::bind(&testPost, boost::ref(c)), RBX::Time::Interval(10));
    }
}


static void doHTTP_SYNC_POST_TESTS()
{
	std::stringstream formdata;

	formdata << "";

	std::string url = "https://games.api.sitetest1.robloxlabs.com/";
	RBX::Http postrequest1a(url);
	std::string response1a;
	postrequest1a.post(formdata, RBX::Http::kContentTypeApplicationJson, false, response1a);
	BOOST_CHECK_EQUAL(response1a, "\"RGS OK\"");

	// Need test for iostream write/read
}

BOOST_AUTO_TEST_CASE(HTTP_SYNC_POST_TESTS)
{
	RBX_TEST_WITH_TIMEOUT(doHTTP_SYNC_POST_TESTS, RBX::Time::Interval(10));
}

BOOST_AUTO_TEST_CASE(HTTP_URL_ENCODE_TEST)
{
	BOOST_CHECK_EQUAL(RBX::Http::urlEncode("this is a&#test"), "this%20is%20a%26%23test");
}

BOOST_AUTO_TEST_CASE(HTTP_URL_DECODE_TEST)
{
	std::string allChars;
	for (int i = 1; i < 256; ++i)
	{
		allChars += (char)i;
	}
	BOOST_REQUIRE_EQUAL(allChars.size(), 255);
	BOOST_CHECK_EQUAL(allChars, RBX::Http::urlDecode(RBX::Http::urlEncode(allChars)));
}

BOOST_AUTO_TEST_CASE(API_PROXY_HTTP_SYNC_GET_TESTS)
{
	DataModelFixture dm;

	RBX::DataModel::LegacyLock l(&dm, RBX::DataModelJob::Write);

	RBX::Reflection::Tuple args;
	dm.execute("game:GetService('ContentProvider'):SetBaseUrl('http://www.roblox.com/')", args);

	NetworkFixture::startServer(dm);
	RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::create<RBX::HttpRbxApiService>(&dm);

	std::string responseString;
	apiService->get("marketplace/productinfo?assetId=28277486", true, RBX::PRIORITY_EXTREME, responseString);

	shared_ptr<const RBX::Reflection::ValueTable> table(rbx::make_shared<const RBX::Reflection::ValueTable>());
	RBX::WebParser::parseJSONTable(responseString, table);

	RBX::Reflection::Variant assetVariant = table->at("AssetId");
	if (assetVariant.isType<int>())
	{
		int asset = assetVariant.cast<int>();
		BOOST_CHECK_EQUAL(asset, 28277486);
	}

	RBX::Reflection::Variant productVariant = table->at("ProductId");
	if (productVariant.isType<int>())
	{
		int product = productVariant.cast<int>();
		BOOST_CHECK_EQUAL(product, 4678951);
	}
}

BOOST_AUTO_TEST_CASE(API_PROXY_HTTP_ASYNC_GET_TESTS)
{
	DataModelFixture dm;

	RBX::DataModel::LegacyLock l(&dm, RBX::DataModelJob::Write);

	RBX::Reflection::Tuple args;
	dm.execute("game:GetService('ContentProvider'):SetBaseUrl('http://www.roblox.com/')", args);

	NetworkFixture::startServer(dm);
	RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::create<RBX::HttpRbxApiService>(&dm);

	apiService->getAsync("marketplace/productinfo?assetId=28277486", true, RBX::PRIORITY_EXTREME,
		boost::bind(&GetAsyncSuccess, _1),
		boost::bind(&GetAsyncError, _1) );

	G3D::System::sleep(3.0);
}

BOOST_AUTO_TEST_CASE(API_PROXY_HTTP_ASYNC_POST_TESTS)
{
	DataModelFixture dm;

	RBX::DataModel::LegacyLock l(&dm, RBX::DataModelJob::Write);

	RBX::Reflection::Tuple args;
	dm.execute("game:GetService('ContentProvider'):SetBaseUrl('http://games.www.sitetest1.robloxlabs.com/')", args);

	RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::create<RBX::HttpRbxApiService>(&dm);
	RBX::ContentProvider* cp = RBX::ServiceProvider::create<RBX::ContentProvider>(&dm);

	NetworkFixture::startServer(dm);

	std::string apiBaseUrl = cp->getApiBaseUrl();
    RBX::Http http(apiBaseUrl);
    std::string postString(" ");
	apiService->postAsync(http, postString, RBX::PRIORITY_EXTREME, RBX::HttpService::APPLICATION_JSON,
		boost::bind(&PostAsyncSuccess, _1),
		boost::bind(&PostAsyncError, _1));

	G3D::System::sleep(3.0);
}


BOOST_AUTO_TEST_SUITE_END()

