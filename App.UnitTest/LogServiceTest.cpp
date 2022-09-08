#include <boost/test/unit_test.hpp>

#include "V8DataModel/LogService.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(LogServiceTest)
struct logTestCase
{
    std::string input;
    std::string output;
    int mask;
    logTestCase(const char* inCstr, const char* outCstr, int mask) : input(inCstr), output(outCstr), mask(mask) {}
};

BOOST_AUTO_TEST_CASE(FilterLogService) {
    int result = 0;
    std::vector<logTestCase> testCases;
    // empty
    testCases.push_back(logTestCase("", 
                                    "", 0));
    // text comment
    testCases.push_back(logTestCase("There is Nothing Filtered and this STILL has CAPS!", 
                                    "There is Nothing Filtered and this STILL has CAPS!", 0));
    // one keyword
    testCases.push_back(logTestCase("http", 
                                    "http", 0));
    // two keywords
    testCases.push_back(logTestCase("http://rbxcdn", 
                                    "http://rbxcdn", 0));
    // hash case
    testCases.push_back(logTestCase("http://s3.rbxcdn.com/0123456789ABCDEF0123456789ABCDEF", 
                                    "http://s3.rbxcdn.com/", 4));
    // capitalized
    testCases.push_back(logTestCase("HTTP://S3.RBXCDN.COM/0123456789ABCDEF0123456789ABCDEF", 
                                    "HTTP://S3.RBXCDN.COM/", 4));
    // leading text
    testCases.push_back(logTestCase("failed to reach HTTP://S3.RBXCDN.COM/0123456789ABCDEF0123456789ABCDEF", 
                                    "failed to reach HTTP://S3.RBXCDN.COM/", 4));
    // trailing text
    testCases.push_back(logTestCase("HTTP://S3.RBXCDN.COM/0123456789ABCDEF0123456789ABCDEF request failed",
                                    "HTTP://S3.RBXCDN.COM/ request failed", 4));

    // multiple keys in one log
    testCases.push_back(logTestCase("http://www.roblox.com/h4x.ashx?apikey=1234-1234-1234-1234&evil=true&accesskey=5678-5678", 
                                    "http://www.roblox.com/h4x.ashx?apikey=&evil=true&accesskey=", 0x3));

    for (const auto& it : testCases)
    {
        std::string filterString = it.input;
        int result = LogService::filterSensitiveKeys(filterString);
	    BOOST_REQUIRE(filterString == it.output);
        BOOST_REQUIRE(result == it.mask);
    }

}

BOOST_AUTO_TEST_SUITE_END()
