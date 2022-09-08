#include "stdafx.h"

// HACK! This avoids "nil" macro compatibility issues between MacOS SDK and Boost
#ifdef nil
#undef nil
#endif

#include <algorithm>

#include "g3d/format.h"

#include "util/Http.h"
#include "util/HttpPlatformImpl.h"
#undef HAVE_MEMCPY
#undef HAVE_CTYPE_H
#include "util/HTW3C.h"
#include "util/URL.h"
#include "util/RobloxGoogleAnalytics.h"
#include "util/standardout.h"
#include "util/Statistics.h"
#include "util/ThreadPool.h"
#include "util/Analytics.h"
#include "Util/RobloxGoogleAnalytics.h"

#include "rbx/TaskScheduler.h"

#include "v8datamodel/DataStore.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/MarketplaceService.h"
#include "v8datamodel/Stats.h"
#include "RobloxServicesTools.h"



#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include <atlutil.h>
#endif // defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)

LOGGROUP(Http)
DYNAMIC_FASTINT(ExternalHttpRequestSizeLimitKB)
DYNAMIC_FASTINTVARIABLE(HttpResponseDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpSendDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpConnectDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpDataSendDefaultTimeoutMillis, 60000)

DYNAMIC_LOGVARIABLE(HttpTrace, 0)
DYNAMIC_FASTINTVARIABLE(HttpSendStatsEveryXSeconds, 60)
DYNAMIC_FASTINTVARIABLE(HttpGAFailureReportPercent, 1)
DYNAMIC_FASTINTVARIABLE(HttpRBXEventFailureReportHundredthsPercent, 0)
DYNAMIC_FASTFLAGVARIABLE(DebugHttpAsyncCallsForStatsReporting, true)
DYNAMIC_FASTINT(HttpInfluxHundredthsPercentage)
DYNAMIC_FASTFLAG(UseNewAnalyticsApi)
DYNAMIC_FASTSTRING(HttpInfluxURL)

DYNAMIC_FASTFLAGVARIABLE(UseAssetTypeHeader, false)

using namespace RBX;

#ifdef __APPLE__
extern "C" {
    int rbx_trustCheckBrowser(const char* url)
    {
        return RBX::Http::trustCheckBrowser(url) ? 1 : 0;
    }
}

// NOTICE: Remove these calls after transition for Apple is complete.
int rbx_isRobloxSite(const char* url);
int rbx_isMoneySite(const char* url);
#endif // ifdef __APPLE__

namespace
{
#if defined(RBX_PLATFORM_DURANGO)
static bool useCurlHttpImpl = false;
#else
static bool useCurlHttpImpl = true;
#endif

static const int kNumberThreadPoolThreads = 16;
static ThreadPool* threadPool;

static bool inline sendHttpFailureToEvents()
{
    static const int r = rand() % 10000;
    return r < DFInt::HttpRBXEventFailureReportHundredthsPercent;
}

static bool inline sendHttpFailureToGA()
{
    static const int r = rand() % 100;
    return r < DFInt::HttpGAFailureReportPercent;
}

static bool inline sendHttpInfluxEvents()
{
    static const int r = rand() % 10000;
    return r < DFInt::HttpInfluxHundredthsPercentage;
}
    
bool isValidScheme(const char* scheme)
{
    return 0 == strncmp(scheme, "http", 4) || 0 == strncmp(scheme, "https", 5);
}

bool hasEnding(const std::string& fullString, const std::string& ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

class HTTPStatistics
{
    static HTTPStatistics httpStatistics;

    boost::mutex mutex;

    double successDelays;
    double failureDelays;
    unsigned numSuccesses;
    unsigned numFailures;

    unsigned numDataStoreSuccesses;
    unsigned numDataStoreFailures;

    unsigned numMarketPlaceSuccesses;
    unsigned numMarketPlaceFailures;

    size_t successBytes;

    enum ServiceCategory
    {
        ServiceCategoryDataStore,
        ServiceCategoryMarketPlace,
        ServiceCategoryOther
    };

	struct PendingStat
	{
		std::string url;
		std::string reason;
		double delay;
		size_t size;
		int statusCode;

		PendingStat(const char* url, const char* reason, size_t bytes, double delay, int httpStatusCode)
			: url(url)
			, reason(reason)
			, delay(delay)
			, size(bytes)
			, statusCode(httpStatusCode)
		{}
	};

	std::list<PendingStat> pendingStats;

    HTTPStatistics()
        :successDelays(0.0)
        ,failureDelays(0.0)
        ,numSuccesses(0)
        ,numFailures(0)
        ,numDataStoreSuccesses(0)
        ,numDataStoreFailures(0)
        ,numMarketPlaceSuccesses(0)
        ,numMarketPlaceFailures(0)
        ,successBytes(0)
    {}

    void addSuccess(double delay, const char* url, size_t bytes)
    {
        const boost::mutex::scoped_lock lock(mutex);
        ++numSuccesses;
        successDelays += delay;
        successBytes += bytes;

        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            ++numDataStoreSuccesses;
            break;
        case ServiceCategoryMarketPlace:
            ++numMarketPlaceSuccesses;
            break;
        default:
            break;
        }
    }

    void addFailure(double delay, const char* url, const char* reason)
    {
        const boost::mutex::scoped_lock lock(mutex);
        ++numFailures;
        failureDelays += delay;

        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            ++numDataStoreFailures;
            break;
        case ServiceCategoryMarketPlace:
            ++numMarketPlaceFailures;
            break;
        default:
            break;
        }

        // Report immediately to GA, but we are assuming failure rates are
        // low so that we don't end up with an infinite recursion when
        // reporting failure data.
        if (sendHttpFailureToGA())
        {
            RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HTTPFailure", reason, delay);
        }

        if (sendHttpFailureToEvents())
        {
            int placeID = atoi(Http::placeID.c_str());
            RobloxGoogleAnalytics::sendEventRoblox(GA_CATEGORY_GAME, "HTTPFailure", reason, placeID);
        }
    }

	void addPendingStat(double delay, const char* url, size_t bytes, const char* reason, int httpStatusCode = 0)
	{
		const boost::mutex::scoped_lock lock(mutex);
		pendingStats.push_back(PendingStat(url, reason, bytes, delay, httpStatusCode));
	}

    void report()
    {
        const boost::mutex::scoped_lock lock(mutex);

		// No need for this to be blocking any more as per CS 61109 on CI
		// If we keep this blocking & suppose the influx goes down or is taking too much time
		// Then we will have to wait 60 sec for each of this request to time out and we will have too many jobs waiting for the mutex to be released
		// Keeping it behind a Flag if we need to flip it other way. Do not remove DebugHttpAsyncCallsForStatsReporting
        if (numSuccesses)
        {
            Analytics::EphemeralCounter::reportStats("HTTPDelaySuccess-"+DebugSettings::singleton().osPlatform(), successDelays / numSuccesses, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPSuccess-"+DebugSettings::singleton().osPlatform(), numSuccesses, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPSuccessDataStore-"+DebugSettings::singleton().osPlatform(), numDataStoreSuccesses, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPSuccessMarketPlace-"+DebugSettings::singleton().osPlatform(), numMarketPlaceSuccesses, !DFFlag::DebugHttpAsyncCallsForStatsReporting);

            Analytics::EphemeralCounter::reportStats("HTTPAccumulatedBytes-"+DebugSettings::singleton().osPlatform(), successBytes, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
        }

        if (numFailures)
        {
            Analytics::EphemeralCounter::reportStats("HTTPDelayFailure-"+DebugSettings::singleton().osPlatform(), failureDelays / numFailures, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPFailure-"+DebugSettings::singleton().osPlatform(), numFailures, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPFailureDataStore-"+DebugSettings::singleton().osPlatform(), numDataStoreFailures, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
            Analytics::EphemeralCounter::reportCounter("HTTPFailureMarketPlace-"+DebugSettings::singleton().osPlatform(), numMarketPlaceFailures, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
        }

        successDelays = failureDelays = 0.0;
        numSuccesses = numFailures = 0;
        numDataStoreSuccesses = numDataStoreFailures = 0;
        numMarketPlaceSuccesses = numMarketPlaceFailures = 0;
        successBytes = 0;

		if (!pendingStats.empty())
		{
			for (std::list<PendingStat>::iterator i = pendingStats.begin(); i != pendingStats.end(); i++)
			{
				sendInfluxEvent(i->delay, i->url.c_str(), i->size, i->reason.c_str(), i->statusCode, !DFFlag::DebugHttpAsyncCallsForStatsReporting);
			}
		}

		pendingStats.clear();
    }

    static ServiceCategory getServiceCategory(const char* url)
    {
        if (DFFlag::UseNewUrlClass)
        {
            RBX::Url parsed = RBX::Url::fromString(url);
            if (parsed.pathEquals(DataStore::urlApiPath()))
            {
                return ServiceCategoryDataStore;
            }

            if (parsed.pathEquals(MarketplaceService::urlApiPath()))
            {
                return ServiceCategoryMarketPlace;
            }
            return ServiceCategoryOther;
        }

        boost::scoped_ptr<char> chost(HTParse(url, NULL, PARSE_PATH));
        if (chost.get() == strstr(chost.get(), DataStore::urlApiPath()))
        {
            return ServiceCategoryDataStore;
        }

        if (chost.get() == strstr(chost.get(), MarketplaceService::urlApiPath()))
        {
            return ServiceCategoryMarketPlace;
        }

        return ServiceCategoryOther;
    }

    static const char* serviceToString(const char* url)
    {
        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            return "DataStore";
        case ServiceCategoryMarketPlace:
            return "MarketPlace";
        default:
            return "Unknown";
        }
    }

    static void sendInfluxEvent(double delay, const char* url, size_t bytes, const char* reason, int httpStatusCode = 0, bool blocking = false)
    {
        if (!DFFlag::UseNewAnalyticsApi && !sendHttpInfluxEvents())
        {
            return;
        }

		// throttle chat filter requests
		std::string s(url);
		if (s.find("/moderation/filtertext") != std::string::npos)
		{
			const int r = rand() % 10000;
			if (r > DFInt::HttpInfluxHundredthsPercentage)
				return;
		}

        RBX::Analytics::InfluxDb::Points points;
        points.addPoint("DelayMillis", delay);
        points.addPoint("IsSuccess", true);
        points.addPoint("ServiceType", serviceToString(url));
        points.addPoint("Bytes", static_cast<unsigned>(bytes));
        points.addPoint("FailureReason", reason);
        points.addPoint("URL", url);
        points.addPoint("HttpStatusCode", httpStatusCode);

		points.report("http", DFInt::HttpInfluxHundredthsPercentage, blocking);
    }

public:
    static void reportingThreadHandler()
    {
        while (true)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(DFInt::HttpSendStatsEveryXSeconds));
            httpStatistics.report();
        }
    }

    static void success(double delay, const char* url, size_t bytes)
    {
        httpStatistics.addSuccess(delay, url, bytes);
		httpStatistics.addPendingStat(delay, url, bytes, "No error");
    }

    static void failure(double delay, const char* url, size_t bytes, const char* reason, int httpStatusCode=0)
    {
		httpStatistics.addFailure(delay, url, reason);
		httpStatistics.addPendingStat(delay, url, bytes, reason, httpStatusCode);
    }
}; // struct CacheStatistics
HTTPStatistics HTTPStatistics::httpStatistics;
} // namespace

namespace RBX
{
static const int kWindowSize = 256;
std::string Http::accessKey;
std::string Http::gameSessionID;
std::string Http::gameID;
std::string Http::placeID;
std::string Http::requester = "Client";
#if defined (__APPLE__) && !defined(RBX_PLATFORM_IOS)
std::string Http::rbxUserAgent = "Roblox/Darwin";
#elif defined(RBX_PLATFORM_DURANGO)
std::string Http::rbxUserAgent = "Roblox/XboxOne";
#elif defined (_WIN32)
std::string Http::rbxUserAgent = "Roblox/WinInet";
#else
std::string Http::rbxUserAgent;
#endif
    
int Http::playerCount = 0;
bool Http::useDefaultTimeouts = true;
Http::CookieSharingPolicy Http::cookieSharingPolicy;

#ifdef _WIN32
Http::API Http::defaultApi = Http::WinHttp;
#else
Http::API Http::defaultApi = Http::Uninitialized;
#endif

rbx::atomic<int> Http::cdnSuccessCount = 0;
rbx::atomic<int> Http::cdnFailureCount = 0;
rbx::atomic<int> Http::alternateCdnSuccessCount = 0;
rbx::atomic<int> Http::alternateCdnFailureCount = 0;
double Http::lastCdnFailureTimeSpan = 0;
rbx::atomic<int> Http::robloxSuccessCount = 0;
rbx::atomic<int> Http::robloxFailureCount = 0;
WindowAverage<double, double> Http::robloxResponce(kWindowSize);
WindowAverage<double, double> Http::cdnResponce(kWindowSize);

RBX::mutex *Http::robloxResponceLock = NULL;
RBX::mutex *Http::cdnResponceLock = NULL;

Http::MutexGuard Http::lockGuard;

#ifdef __APPLE__
// NOTICE: Remove these calls after transition for Apple is complete.
namespace Cocoa
{
extern void httpGetPostCocoa(const std::string &url, const std::string &authDomainUrl, bool isPost, std::istream& data, const std::string& contentType, bool compressData, HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response, bool useDefaultTimeout, int requestTimeoutMillis);
}

void Http::httpGetPostImpl(bool isPost, std::istream& data, const std::string& contentType, bool compressData, const HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response)
{
    if(externalRequest)
        ThrowIfFailure(trustCheck(url.c_str(), true), "Trust check failed");
    
    HttpAux::AdditionalHeaders headers = additionalHeaders;

    if (!externalRequest)
    {
        if (Http::gameSessionID.length() > 0 && headers.end() == headers.find(kGameSessionHeaderKey))
        {
            headers[kGameSessionHeaderKey] = Http::gameSessionID;
        }

        if (Http::gameID.length() > 0 && headers.end() == headers.find(kGameIdHeaderKey))
        {
            headers[kGameIdHeaderKey] = Http::gameID;
        }

        if (Http::placeID.length() > 0 && headers.end() == headers.find(kPlaceIdHeaderKey))
        {
            headers[kPlaceIdHeaderKey] = Http::placeID;
        }

        if (Http::requester.length() > 0 && headers.end() == headers.find(kRequesterHeaderKey))
        {
            headers[kRequesterHeaderKey] = Http::requester;
        }

        if (headers.end() == headers.find(kPlayerCountHeaderKey))
    	{
            headers[kPlayerCountHeaderKey] = RBX::format("%d", Http::playerCount);
    	}
    }

    // call Mac specific http implementation
    RBX::Cocoa::httpGetPostCocoa(url, authDomainUrl, isPost, data, contentType, compressData, headers, externalRequest, response, useDefaultTimeouts, responseTimeoutMillis);
}
#endif // ifdef __APPLE__
} // RBX

Http::MutexGuard::MutexGuard()
{
    robloxResponceLock = new RBX::mutex();
    cdnResponceLock = new RBX::mutex();
}

Http::MutexGuard::~MutexGuard()
{
    delete robloxResponceLock;
    robloxResponceLock = NULL;

    delete cdnResponceLock;
    cdnResponceLock = NULL;
}

RBX::mutex *Http::getRobloxResponceLock()
{
    return robloxResponceLock;
}

RBX::mutex *Http::getCdnResponceLock()
{
    return cdnResponceLock;
}

http_status_error::http_status_error(int statusCode)
    : std::runtime_error(RBX::format("HTTP %d", statusCode))
    , statusCode(statusCode)
{
}

http_status_error::http_status_error(int statusCode, const std::string& message)
    : std::runtime_error(RBX::format("HTTP %d (%s)", statusCode, message.c_str()))
    , statusCode(statusCode)
{
}

void Http::init()
{
    recordStatistics = true;
	shouldRetry = true;
    cachePolicy = HttpCache::PolicyDefault;
    doNotUseCachedResponse = false;
    connectTimeoutMillis = DFInt::HttpConnectDefaultTimeoutMillis;
    responseTimeoutMillis = DFInt::HttpResponseDefaultTimeoutMillis;
    sendTimeoutMillis = DFInt::HttpSendDefaultTimeoutMillis;
    dataSendTimeoutMillis = DFInt::HttpDataSendDefaultTimeoutMillis;
    sendHttpFailureToGA(); //initializes the static random number in function
    sendHttpFailureToEvents(); //initializes the static random number in function
    sendHttpInfluxEvents(); //initializes the static random number in function
}

void Http::init(API api, CookieSharingPolicy sharingPolicy)
{
    static boost::scoped_ptr<ThreadPool> tp(new ThreadPool(kNumberThreadPoolThreads));
    threadPool = tp.get();
    Http::defaultApi = api;
    FASTLOG2(FLog::Http, "Http initialization M1 = 0x%x M2=0x%x", &Http::robloxResponceLock, &Http::cdnResponceLock);
    cookieSharingPolicy = sharingPolicy;
	Http::SetUseCurl(useCurlHttpImpl);
}

void Http::SetUseStatistics(bool value)
{
    if (value)
    {
        boost::function0<void> f = boost::bind(&HTTPStatistics::reportingThreadHandler);
        boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, MESSAGE_ERROR, false);
        boost::thread(thread_wrapper(g, "rbx_http_stats_report"));
    }
}

void Http::SetUseCurl(bool value)
{
	FASTLOGS(DFLog::HttpTrace, "Use the new http api: %s", value ? "yes" : "no");
    useCurlHttpImpl = value;

    if (useCurlHttpImpl)
    {
        static boost::mutex mutex;
        static bool initialized = false;

        boost::mutex::scoped_lock lock(mutex);

        if (!initialized)
        {
            HttpPlatformImpl::init(cookieSharingPolicy);
            initialized = true;
        }
    }
}

void Http::setCookiesForDomain(const std::string& domain, const std::string& cookies)
{
    HttpPlatformImpl::setCookiesForDomain(domain, cookies);
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	if (!useCurlHttpImpl && Http::defaultApi == WinInet)
	{
		setCookiesForDomainWinInet(domain, cookies);
	}
#endif
}

void Http::getCookiesForDomain(const std::string& domain, std::string& cookies){
  HttpPlatformImpl::getCookiesForDomain(domain, cookies);
}

void Http::ThrowIfFailure(bool success, const char* message)
{
    ThrowIfFailure(success, url.c_str(), message);
}

void Http::ThrowIfFailure(bool success, const char* url, const char* message)
{
    if (!success)
    {
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
        ThrowLastError(GetLastError(), url, message);
#else
        throw RBX::runtime_error("%s: %s", url, message);
#endif
    }
}

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
void Http::ThrowLastError(int err, const char* url, const char* message)
{
    TCHAR buffer[256];
    if (::FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            buffer, 256, NULL) == 0
        )
        throw RBX::runtime_error("%s: %s, err=0x%X", url, message, err);
    else
        throw RBX::runtime_error("%s: %s, %s", url, message, buffer);
}
#endif // ifdef _WIN32


#if defined(RBX_PLATFORM_DURANGO)
void dprintf( const char* fmt, ... );
#endif

void Http::httpGetPost(bool isPost, std::istream& dataStream,
					   const std::string& contentType, bool compressData,
					   const HttpAux::AdditionalHeaders& additionalHeaders,
					   bool externalRequest, std::string& response,
					   bool forceNativeHttp)
{
    RBX::Timer<RBX::Time::Fast> httpTimer;
#ifdef __APPLE__
	if (!useCurlHttpImpl || forceNativeHttp)
    {
        RBXASSERT(isPost == !contentType.empty());
        try
        {
            // instanceApi is irrelevant on a Mac, there is only one style NSUrl
            httpGetPostImpl(isPost, dataStream, contentType, compressData, additionalHeaders, externalRequest, response);
            if (recordStatistics)
            {
                HTTPStatistics::success(httpTimer.delta().msec(), url.c_str(), response.size());
            }
        }
        catch (const RBX::http_status_error& e)
        {
            if (recordStatistics)
            {
                HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what(), e.statusCode);
            }
            throw;
        }
        catch (const std::exception& e)
        {
            if (recordStatistics)
            {
                HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(),  e.what());
            }
            throw;
        }
        return;
    }
#endif // ifdef __APPLE__
    
    RBXASSERT(isPost == !contentType.empty());

	ThrowIfFailure(trustCheck(url.c_str(), externalRequest), "Trust check failed");

    if (externalRequest)
    {
        if (isPost)
        {
            RBXASSERT(0 == dataStream.tellg());
            dataStream.seekg(0, std::ios::end);
            size_t length = dataStream.tellg();
            dataStream.seekg(0, std::ios::beg);
            if ((length / 1024) >= static_cast<size_t>(DFInt::ExternalHttpRequestSizeLimitKB))
            {
                throw RBX::runtime_error("Post data too large. Limit: %d KB. Post size: %d KB",
                                         DFInt::ExternalHttpRequestSizeLimitKB, static_cast<int>(length / 1024));
            }
        }
    }

    HttpAux::AdditionalHeaders headers = additionalHeaders;

    if (!externalRequest)
    {
        if (Http::gameSessionID.length() > 0 && headers.end() == headers.find(kGameSessionHeaderKey))
        {
            headers[kGameSessionHeaderKey] = Http::gameSessionID;
        }

        if (Http::gameID.length() > 0 && headers.end() == headers.find(kGameIdHeaderKey))
        {
            headers[kGameIdHeaderKey] = Http::gameID;
        }

        if (Http::placeID.length() > 0 && headers.end() == headers.find(kPlaceIdHeaderKey))
        {
            headers[kPlaceIdHeaderKey] = Http::placeID;
        }

        if (Http::requester.length() > 0 && headers.end() == headers.find(kRequesterHeaderKey))
        {
            headers[kRequesterHeaderKey] = Http::requester;
        }

		if (useCurlHttpImpl && !forceNativeHttp)
        {
            if (externalRequest && headers.end() == headers.find(kAccessHeaderKey))
            {
                headers[kAccessHeaderKey] = "UserRequest";
            }
            else if (!Http::accessKey.empty() && headers.end() == headers.find(kAccessHeaderKey))
            {
                headers[kAccessHeaderKey] = Http::accessKey;
            }
        }
    }

#if defined(RBX_PLATFORM_DURANGO)
	if (!useCurlHttpImpl || forceNativeHttp)
	{
        if (url.find("http://") == 0) // starts with http?
        {
            dprintf( "WARN HTTP: %s\n", url.c_str() );
        }

		try
		{
			httpGetPostXbox(isPost, dataStream, contentType, compressData, headers, externalRequest, cachePolicy, response);
			if (recordStatistics)
			{
				HTTPStatistics::success(httpTimer.delta().msec(), url.c_str(), response.size());
			}
		}
		catch (const RBX::http_status_error& e)
		{
			if (recordStatistics)
			{
				HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what(), e.statusCode);
			}
			throw;
		}
		catch (const std::exception& e)
		{
			if (recordStatistics)
			{
				HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what());
			}
			throw;
		}
		return;
	}
#elif defined(_WIN32)
	if (!useCurlHttpImpl || forceNativeHttp)
    {
        try
        {
            switch (instanceApi)
            {
            case WinInet:
                httpGetPostWinInet(isPost, dataStream, contentType, compressData, headers, externalRequest, response);
                break;
            case WinHttp:
                httpGetPostWinHttp(isPost, dataStream, contentType, compressData, headers, externalRequest, response);
                break;
            default:
                RBXASSERT(false);
            }
            if (recordStatistics)
            {
                HTTPStatistics::success(httpTimer.delta().msec(), url.c_str(), response.size());
            }
        }
        catch (const RBX::http_status_error& e)
        {
            if (recordStatistics)
            {
                HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what(), e.statusCode);
            }
            throw;
        }
        catch (const std::exception& e)
        {
            if (recordStatistics)
            {
                HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what());
            }
            throw;
        }
        return;
    }
#endif // ifdef _WIN32

    HttpPlatformImpl::HttpOptions httpOpts(url, externalRequest, cachePolicy, connectTimeoutMillis, responseTimeoutMillis);
    if (isPost)
    {
        httpOpts.setPostData(&dataStream, compressData);
    }
    httpOpts.setHeaders(&contentType, &headers);

    try
    {
        HttpPlatformImpl::perform(httpOpts, response);
        if (recordStatistics)
        {
            HTTPStatistics::success(httpTimer.delta().msec(), url.c_str(), response.size());
        }
    }
    catch (const RBX::http_status_error& e)
    {
		bool didMakeNativeRequest = doHttpGetPostWithNativeFallbackForReporting(isPost, dataStream, contentType, compressData, additionalHeaders,
				externalRequest, response);
		
        if (recordStatistics)
        {
            HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what(), e.statusCode);
        }
		if(!didMakeNativeRequest) throw;
    }
    catch (const std::exception& e)
    {
        if (recordStatistics)
        {
            HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what());
        }
        throw;
    }
}

void Http::post(std::istream& input, const std::string& contentType,
                bool compress, std::string& response, bool externalRequest)
{
    httpGetPost(true, input, contentType, compress, additionalHeaders, externalRequest, response);
}

static void doGet(Http http, bool externalRequest,
                  boost::function<void(std::string*, std::exception*)> handler)
{
    std::string response;
    try
    {
        http.get(response, externalRequest);
    }
    catch (RBX::base_exception& ex)
    {
        handler(0, &ex);
        return;
    }
    handler(&response, 0);
}

void Http::get(
    boost::function<void(std::string*, std::exception*)> handler,
    bool externalRequest)
{
    threadPool->schedule(boost::bind(&doGet, *this, externalRequest, handler));
}

#if defined(_WIN32)
void Http::onWinHttpRedirect(unsigned long dwInternetStatus, std::string redirectUrl)
{
    // This function determines if we are redirecting to a CDN.
    // If so, then record an alternate URL in case the CDN fails.
    // The alternate URL is the corresponding Amazon S3 URL.
    // bitgravity:
    int pos = redirectUrl.find("bg.roblox");
    if (pos != std::string::npos)
    {
        alternateUrl = redirectUrl.substr(0, pos);
        alternateUrl += redirectUrl.substr(pos + 2);
        return;
    }

    // cloudfront:
    pos = redirectUrl.find("-cf.roblox");
    if (pos != std::string::npos)
    {
        alternateUrl = redirectUrl.substr(0, pos);
        alternateUrl += redirectUrl.substr(pos + 3);
        return;
    }
}
#endif // ifdef _WIN32

static void doPostStream(std::string url, boost::shared_ptr<std::istream> input,
						 const std::string& contentType, bool compress, bool externalRequest, bool recordStatistics,
						 boost::function<void(std::string *, std::exception*)> handler)
{
	std::string response;
	try
	{
		Http http(url);
        http.recordStatistics = recordStatistics;
		http.post(*input, contentType, compress, response, externalRequest);
	}
	catch (RBX::base_exception &ex)
	{
		handler(0, &ex);
		return;
	}
	handler(&response, 0);
}

static void doPost(std::string url, std::string input,
                   const std::string& contentType, bool compress, bool externalRequest, bool recordStatistics,
                   boost::function<void(std::string*, std::exception*)> handler)
{
    shared_ptr<std::istream> inputStream(new std::istringstream(input));
    doPostStream(url, inputStream, contentType, compress,
        externalRequest, recordStatistics, handler);
}

void Http::post(const std::string& input, const std::string& contentType,
                bool compress,
                boost::function<void(std::string*, std::exception*)> handler,
                bool externalRequest)
{
    threadPool->schedule(
        boost::bind(&doPost, url, input, contentType, compress,
                    externalRequest, recordStatistics, handler));
}

void Http::post(boost::shared_ptr<std::istream> input,
                const std::string& contentType, bool compress,
                boost::function<void(std::string *, std::exception*)> handler,
                bool externalRequest)
{
    threadPool->schedule(
        boost::bind(&doPostStream, url, input, contentType, compress,
                    externalRequest, recordStatistics, handler));
}

void Http::get(std::string& response, bool externalRequest)
{
    Time startTime = Time::now<Time::Fast>();

    std::istringstream dummy;
    try
    {
        httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
        if (!alternateUrl.empty())
            ++cdnSuccessCount;
    }
	catch (RBX::base_exception& e)
    {
        if (shouldRetry)
        {
			shouldRetry = false;

            response.clear();
            const double elapsed = (Time::now<Time::Fast>() - startTime).seconds();
            if (!alternateUrl.empty())
            {
                ++cdnFailureCount;
                lastCdnFailureTimeSpan = elapsed;
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,
                                                      "httpGet %s failed. Trying alternate %s. Error: %s.  Elapsed time: %g",
                                                      url.c_str(), alternateUrl.c_str(), e.what(), elapsed);
                try
                {
                    httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
                    ++alternateCdnSuccessCount;
                }
				catch (RBX::base_exception&)
                {
                    ++alternateCdnFailureCount;
                    throw;
                }
            }
            else
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,
                                                      "httpGet %s failed. Trying again. Error: %s.  Elapsed time: %g",
                                                      url.c_str(), e.what(), elapsed);

                httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
            }
        }
        else
        {
            // rethrow the existing exception
            throw;
        }
    }
}

bool Http::isScript(const char* url)
{
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (!useCurlHttpImpl)
    {
        CUrl crack;
        crack.CrackUrl(url);
        switch (crack.GetScheme())
        {
        case ATL_URL_SCHEME_HTTP:
        case ATL_URL_SCHEME_HTTPS:
        {
            return false;
        }
        default:
        {
            CString sUrl = url;
            if (sUrl.Find("javascript:")==0)
                return true;
            if (sUrl.Find("jscript:")==0)
                return true;

            return false;
        }
        }
    }
#endif // ifdef _WIN32

    static const char* javascript = "javascript:";
    static const char* jscript = "jscript:";
    return
        0 == strncmp(url, javascript, strlen(javascript)) ||
        0 == strncmp(url, jscript, strlen(jscript));
}

bool Http::isMoneySite(const char* url)
{
#ifdef __APPLE__
    if (!useCurlHttpImpl)
    {
        return rbx_isMoneySite(url) != 0;
    }
#endif // ifdef __APPLE__

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (!useCurlHttpImpl)
    {
        CUrl crack;
        crack.CrackUrl(url);
        switch (crack.GetScheme())
        {
        case ATL_URL_SCHEME_HTTP:
        case ATL_URL_SCHEME_HTTPS:
        case ATL_URL_SCHEME_FTP:
        {
            // only trust urls from roblox.com
            CString hostName = crack.GetHostName();
            hostName.MakeLower();
            if (hostName.Right(10)=="paypal.com")
                return true;
            if (hostName.Right(9)=="rixty.com")
                return true;
            return false;
        }
        default:
        {
            return false;
        }
        }
    }
#endif // ifdef _WIN32

    if (DFFlag::UseNewUrlClass)
    {
        RBX::Url parsed = RBX::Url::fromString(url);

        return parsed.hasHttpScheme() &&
            (parsed.isSubdomainOf("paypal.com") || parsed.isSubdomainOf("rixty.com"));
    }
    
    std::string host;
	boost::scoped_ptr<char> cscheme(HTParse(url, NULL, PARSE_ACCESS));
	if (!isValidScheme(cscheme.get()))
	{
		return false;
	}

	boost::scoped_ptr<char> chost(HTParse(url, NULL, PARSE_HOST));
	host = chost.get();

    std::transform(host.begin(), host.end(), host.begin(), ::tolower);

    // Only trust urls from roblox.com.
    if ("paypal.com" == host || hasEnding(host, ".paypal.com"))
    {
        return true;
    }
    if ("rixty.com" == host || hasEnding(host, ".rixty.com"))
    {
        return true;
    }

    return false;
}

// This allows just roblox domains, not roblox trusted domains like facebook.
bool Http::isStrictlyRobloxSite(const char* url)
{
    if (DFFlag::UseNewUrlClass)
    {
        RBX::Url parsed = RBX::Url::fromString(url);
    
        return parsed.isSubdomainOf("roblox.com") || parsed.isSubdomainOf("robloxlabs.com");
    }

    std::string host(HTParse(url, NULL, PARSE_HOST));
    if ("roblox.com" != host && !hasEnding(host, ".roblox.com") 
        && "robloxlabs.com" != host && !hasEnding(host, ".robloxlabs.com"))
    {
        return false;
    }
    return true;
}

bool Http::isRobloxSite(const char* url)
{
#ifdef __APPLE__
    if (!useCurlHttpImpl)
    {
        return rbx_isRobloxSite(url) != 0;
    }
#endif // ifdef __APPLE__

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (!useCurlHttpImpl)
    {
        CUrl crack;
        crack.CrackUrl(url);
        switch (crack.GetScheme())
        {
        case ATL_URL_SCHEME_HTTP:
        case ATL_URL_SCHEME_HTTPS:
        case ATL_URL_SCHEME_FTP:
        {
            CString hostName = crack.GetHostName();
            hostName.MakeLower();
            CString urlPath = crack.GetUrlPath();
            urlPath.MakeLower();

            // trust urls from roblox.com
			if (hostName.Right(10)=="roblox.com" || hostName.Right(14)=="robloxlabs.com")
				return true;

            // trust facebook login
            if ((hostName == "login.facebook.com" && urlPath == "/login.php")
                || (hostName == "ssl.facebook.com" && urlPath == "/connect/uiserver.php")
                || (hostName == "www.facebook.com" && (urlPath == "/connect/uiserver.php" || urlPath == "/logout.php")))
                return true;

            // trust google/youtube upload/auth
            if (hostName == "www.youtube.com" && (urlPath == "/auth_sub_request" || urlPath == "/signin" || urlPath == "/issue_auth_sub_token"))
                return true;
            if (hostName == "uploads.gdata.youtube.com")
                return true;
            if (hostName == "www.google.com" && urlPath == "/accounts/serviceloginauth")
                return true;
            if (hostName == "accounts.google.com" && urlPath == "/serviceloginauth") {
                return true;
            }

            return false;
        }
        default:
        {
            return false;
        }
        }
    }
#endif // ifdef _WIN32
    
    if (DFFlag::UseNewUrlClass)
    {
        RBX::Url parsed = RBX::Url::fromString(url);
        if (!parsed.hasHttpScheme())
        {
            return false;
        }
    
        const bool isRoblox =
            parsed.isSubdomainOf("roblox.com") ||
            parsed.isSubdomainOf("robloxlabs.com");

        const bool isFacebook =
            ("login.facebook.com" == parsed.host()
                && parsed.pathEqualsCaseInsensitive("/login.php")) ||
            ("ssl.facebook.com" == parsed.host()
                && parsed.pathEqualsCaseInsensitive("/connect/uiserver.php")) ||
            ("www.facebook.com" == parsed.host()
                && (parsed.pathEqualsCaseInsensitive("/connect/uiserver.php")
                    || parsed.pathEqualsCaseInsensitive("/logout.php")));
        
        const bool isYoutube =
            ("www.youtube.com" == parsed.host()
                && (parsed.pathEqualsCaseInsensitive("/auth_sub_request")
                || parsed.pathEqualsCaseInsensitive("/signin")
                || parsed.pathEqualsCaseInsensitive("/issue_auth_sub_token"))) ||
            ("uploads.gdata.youtube.com" == parsed.host());
        
        const bool isGoogle =
            ("www.google.com" == parsed.host()
                && parsed.pathEqualsCaseInsensitive("/accounts/serviceloginauth")) ||
            ("accounts.google.com" == parsed.host()
                && parsed.pathEqualsCaseInsensitive("/serviceloginauth"));

        return isRoblox || isFacebook || isYoutube || isGoogle;
    } // if (DFFlag::UseNewUrlClass)

    std::string host;
    std::string path;

    boost::scoped_ptr<char> cscheme(HTParse(url, NULL, PARSE_ACCESS));
    if (!isValidScheme(cscheme.get()))
    {
        return false;
    }

    boost::scoped_ptr<char> chost(HTParse(url, NULL, PARSE_HOST));
    host = chost.get();

    boost::scoped_ptr<char> cpath(HTParse(url, NULL, PARSE_PATH));
    path = cpath.get();

    std::transform(host.begin(), host.end(), host.begin(), ::tolower);
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    return
        "roblox.com" == host || hasEnding(host, ".roblox.com") ||
        "robloxlabs.com" == host || hasEnding(host, ".robloxlabs.com") ||
        // trust facebook login
        ("login.facebook.com" == host && "/login.php") ||
        ("ssl.facebook.com" == host && "/connect/uiserver.php" == path) ||
        ("www.facebook.com" == host && ("/connect/uiserver.php" == path || "/logout.php" == path)) ||
        // trust google/youtube upload/auth
        ("www.youtube.com" == host && ("/auth_sub_request" == path || "/signin" == path || "/issue_auth_sub_token" == path)) ||
        ("uploads.gdata.youtube.com" == host) ||
        ("www.google.com" == host && "/accounts/serviceloginauth" == path) ||
        ("accounts.google.com" == host && "/serviceloginauth" == path);
}

bool Http::trustCheckBrowser(const char* url)
{
    return trustCheck(url) || isMoneySite(url);
}

bool Http::isExternalRequest(const char* url)
{
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (!useCurlHttpImpl)
    {
        std::string urlLower = url;
        std::transform(urlLower.begin(), urlLower.end(), urlLower.begin(), tolower);

        CUrl urlParsed;
        urlParsed.CrackUrl(urlLower.c_str());

        ATL_URL_SCHEME scheme = urlParsed.GetScheme();
        if(scheme != ATL_URL_SCHEME_HTTP && scheme != ATL_URL_SCHEME_HTTPS)
            return false;

        std::string hostname = urlParsed.GetHostName();

        if(hostname.find("roblox.com") != std::string::npos || hostname.find("robloxlabs.com") != std::string::npos)
            return false;

        return true;
    }
#endif // ifdef _WIn32

    if (DFFlag::UseNewUrlClass)
    {
        RBX::Url parsed = RBX::Url::fromString(url);
        if (!parsed.hasHttpScheme())
        {
            return false;
        }

        return !parsed.isSubdomainOf("roblox.com")
            && !parsed.isSubdomainOf("robloxlabs.com");
    }

    std::string host;

    boost::scoped_ptr<char> cscheme(HTParse(url, NULL, PARSE_ACCESS));
    if (!isValidScheme(cscheme.get()))
    {
        return false;
    }

    boost::scoped_ptr<char> chost(HTParse(url, NULL, PARSE_HOST));
    host = chost.get();

    std::transform(host.begin(), host.end(), host.begin(), ::tolower);

    return
        "roblox.com" != host && !hasEnding(host, ".roblox.com") &&
        "robloxlabs.com" != host && !hasEnding(host, ".robloxlabs.com");
}

void Http::setProxy(const std::string& host, long port)
{
    HttpPlatformImpl::setProxy(host, port);
}

bool Http::trustCheck(const char* url, bool externalRequest)
{
    static const bool kSkipTrustCheck = true;
    if (kSkipTrustCheck)
    {
        return true;
    }

    if (externalRequest)
    {
        return isExternalRequest(url);
    }

    if (std::string("about:blank") == url ||
        isRobloxSite(url) ||
        isScript(url))
    {
        return true;
    }

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    if (!useCurlHttpImpl)
    {
        CUrl crack;
        crack.CrackUrl(url);
        switch (crack.GetScheme())
        {
        case ATL_URL_SCHEME_HTTP:
        case ATL_URL_SCHEME_HTTPS:
        {
            return false;
        }
        default:
        {
            // also trust some embedded resources
            CString sUrl = url;
            // Patch for IE6:
            sUrl.Replace("%20", " ");
            if (sUrl.Find("res://")==0)
            {
                if (sUrl.Find("res://mshtml.dll")==0)
                    return true;
                if (sUrl.Find("res://ieframe.dll")==0)
                    return true;
                if (sUrl.Find("res://shdoclc.dll")==0)
                    return true;

                // Allow resources embedded inside this app
                TCHAR module[_MAX_PATH];
                if (GetModuleFileName(NULL, module, _MAX_PATH))
                {
                    CString strResourceURL;
                    strResourceURL.Format(_T("res://%s"), module);
                    if (sUrl.Find(strResourceURL)==0)
                        return true;
                }
            }

            return false;
        }
        }
    }
    else
    {
        if (DFFlag::UseNewUrlClass)
        {
		    RBX::Url parsed = RBX::Url::fromString(url);
		
		    if (parsed.hasHttpScheme())
		    {
			    return false;
		    }

		    if ("mshtml.dll" == parsed.host() ||
			    "ieframe.dll" == parsed.host() ||
			    "shdoclc.dll" == parsed.host())
		    {
			    return true;
		    }

		    // Allow resources embedded inside this app
		    TCHAR module[_MAX_PATH];
		    if (GetModuleFileName(NULL, module, _MAX_PATH))
		    {
			    if ("res" == parsed.scheme() && 0 == parsed.host().find(module))
			    {
			        return true;
			    }
		    }

		    return false;
        } // if (DFFlag::UseNewUrlClass)

        std::string host;
        boost::scoped_ptr<char> cscheme(HTParse(url, NULL, PARSE_ACCESS));
        if (isValidScheme(cscheme.get()))
        {
            return false;
        }

        boost::scoped_ptr<char> chost(HTParse(url, NULL, PARSE_HOST));
        host = chost.get();

        if ("mshtml.dll" == host ||
            "ieframe.dll" == host ||
            "shdoclc.dll" == host)
        {
            return true;
        }

        // Allow resources embedded inside this app
        TCHAR module[_MAX_PATH];
        if (GetModuleFileName(NULL, module, _MAX_PATH))
        {
            std::stringstream ss;
            ss << "res://" << module;
            if (0 == host.find(ss.str()))
            {
                return true;
            }
        }

        return false;
    }
#else // not ifdef _WIN32
    return false;
#endif // not ifdef _WIN32
}

bool Http::doHttpGetPostWithNativeFallbackForReporting(bool isPost, std::istream& dataStream, const std::string& contentType, bool compressData, const HttpAux::AdditionalHeaders& additionalHeaders, bool allowExternal, std::string& response)
{
#ifdef __ANDROID__
	return false;
#endif

	std::string googleAnalyticsBaseURL = RobloxGoogleAnalytics::kGoogleAnalyticsBaseURL;
	std::string counterMultiUrl = GetCountersMultiIncrementUrl(::GetBaseURL(), RBX::Stats::countersApiKey);
	std::string counterSingleUrl = GetCountersUrl(::GetBaseURL(), RBX::Stats::countersApiKey);
	std::string statsURL = RBX::format("%sgame/report-stats", ::GetBaseURL().c_str());
	std::string inFluxBaseURL = DFString::HttpInfluxURL;
	

	if((url.compare(0, googleAnalyticsBaseURL.length(), googleAnalyticsBaseURL) == 0 || 
		url.compare(0, counterMultiUrl.length(), counterMultiUrl) == 0 ||
		url.compare(0, counterSingleUrl.length(), counterSingleUrl) == 0 ||
		url.compare(0, statsURL.length(), statsURL) == 0 ||
		url.compare(0, inFluxBaseURL.length(), inFluxBaseURL) == 0 ))
	{
		response.clear();
		httpGetPost(isPost, dataStream, contentType, compressData, additionalHeaders, allowExternal, response, true);
		return true;
	}
	return false;
}

void Http::applyAdditionalHeaders(RBX::HttpAux::AdditionalHeaders& outHeaders)
{
	for (RBX::HttpAux::AdditionalHeaders::const_iterator itr = outHeaders.begin(); itr != outHeaders.end(); ++itr)
	{
		this->additionalHeaders[itr->first] = itr->second;
	}
}

