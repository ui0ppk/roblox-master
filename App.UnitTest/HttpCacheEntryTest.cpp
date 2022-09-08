#include <sstream>
#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>

#include "util/HttpPlatformImpl.h"

#include "rbx/test/ScopedFastFlagSetting.h"
#include "rbx/test/test_tools.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(HttpCacheEntryTest)

namespace
{
struct Fixture
{
    Fixture() :
        url("http://www.roblox.com/alphabetcity.html"),
        httpHeaders(
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: private\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Vary: Accept-Encoding\r\n"
        "Server: Microsoft-IIS/7.5\r\n"
        "Access-Control-Allow-Origin: http://converter.telerik.com\r\n"
        "Date: Fri, 10 Oct 2014 01:14:34 GMT\r\n"
        "Content-Length: 599\r\n"
        "Connection: close\r\n"
        "\r\n"
        ),
        httpBody(
        "2\n"
        "4\n"
        "9\n"
        "4\n"
        "2.4.9.4 [9/18/2014]\n"
        "Enable request streaming\n"
        "Expose more HTTPS & Security info\n"
        "Support Content-Encoding: Xpress on Win8+\n"
        "Enable SAZ drop from Outlook\n"
        "Enable AutoResponse of files over 2gb\n"
        "Various bugfixes\n"
        "\n"
        "2.4.9.3 [8/19/2014]\n"
        "Improve performance\n"
        "Improve troubleshooter\n"
        "Improve FiddlerHook\n"
        "\n"
        "2.4.9.2 [7/16/2014]\n"
        "Improve Transformer\n"
        "Improve ImageView, WebView, HexView\n"
        "Improve CURL interop\n"
        "New items in Options dialog\n"
        "\n"
        "@WELCOME@\n"
        "Think Selenium is free?\n"
        "See what it'll cost you in setup, creation and maintenance.\n"
        "http://debugtheweb.com/r/?is-selenium-free\n"
        )
    {}

    ~Fixture()
    {}

    boost::filesystem::path pathA;
    boost::filesystem::path pathB;

    std::string url;
    std::string httpHeaders;
    std::string httpBody;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CACHE_DATA_TOSTRING_TEST, Fixture)
{
    HttpPlatformImpl::Cache::Data headers(httpHeaders.data(), httpHeaders.size());
    BOOST_CHECK_EQUAL(httpHeaders, headers.toString());
}

BOOST_FIXTURE_TEST_CASE(CACHE_UPDATE_TEST, Fixture)
{
    {
        HttpPlatformImpl::Cache::Data headers(httpHeaders.data(), httpHeaders.size());
        HttpPlatformImpl::Cache::Data body(httpBody.data(), httpBody.size());

        HttpPlatformImpl::Cache::CacheResult result = HttpPlatformImpl::Cache::CacheResult::update(NULL, url.c_str(), 200, headers, body);

        BOOST_CHECK_EQUAL(200, result.getCacheHeader().responseCode);

        BOOST_CHECK_EQUAL(httpHeaders, result.getResponseHeader().toString());
        BOOST_CHECK_EQUAL(httpBody, result.getResponseBody().toString());

        BOOST_CHECK_EQUAL(httpHeaders.size(), result.getResponseHeader().size());
        BOOST_CHECK_EQUAL(httpBody.size(), result.getResponseBody().size());
    }

    {
        HttpPlatformImpl::Cache::CacheResult result = HttpPlatformImpl::Cache::CacheResult::open(NULL, url.c_str());

        BOOST_CHECK_EQUAL(200, result.getCacheHeader().responseCode);

        BOOST_CHECK_EQUAL(httpHeaders, result.getResponseHeader().toString());
        BOOST_CHECK_EQUAL(httpBody, result.getResponseBody().toString());

        BOOST_CHECK_EQUAL(httpHeaders.size(), result.getResponseHeader().size());
        BOOST_CHECK_EQUAL(httpBody.size(), result.getResponseBody().size());
    }


	/* We are not going to store invalid Non 200 responses in Cache any more
	We will kill it shortly
    {
        static const std::string newHttpeaders = "Test headers";
        static const std::string newHttpBody = "Test body";

        HttpPlatformImpl::Cache::Data headers(newHttpeaders.data(), newHttpeaders.size());
        HttpPlatformImpl::Cache::Data body(newHttpBody.data(), newHttpBody.size());

        HttpPlatformImpl::Cache::CacheResult result = HttpPlatformImpl::Cache::CacheResult::update(NULL, url.c_str(), 404, headers, body);

        BOOST_CHECK_EQUAL(404, result.getCacheHeader().responseCode);

        BOOST_CHECK_EQUAL(newHttpeaders, result.getResponseHeader().toString());
        BOOST_CHECK_EQUAL(newHttpBody, result.getResponseBody().toString());

        BOOST_CHECK_EQUAL(newHttpeaders.size(), result.getResponseHeader().size());
        BOOST_CHECK_EQUAL(newHttpBody.size(), result.getResponseBody().size());
    }

    {
        static const std::string newHttpeaders = "Test headers";
        static const std::string newHttpBody = "Test body";

        HttpPlatformImpl::Cache::CacheResult result = HttpPlatformImpl::Cache::CacheResult::open(NULL, url.c_str());

        BOOST_CHECK_EQUAL(404, result.getCacheHeader().responseCode);

        BOOST_CHECK_EQUAL(newHttpeaders, result.getResponseHeader().toString());
        BOOST_CHECK_EQUAL(newHttpBody, result.getResponseBody().toString());

        BOOST_CHECK_EQUAL(newHttpeaders.size(), result.getResponseHeader().size());
        BOOST_CHECK_EQUAL(newHttpBody.size(), result.getResponseBody().size());
    }
	*/
}

BOOST_AUTO_TEST_SUITE_END()
