/**
 * For all those wishing to enter this code, it implements HTTP using libcurl.
 * In any operations you implement, you must remember that when you're finished
 * setting options (curl_easy_setopt(3)), you must call curl_easy_perform(3)
 * before you will see the effect of these things.
 **/

#include "util/HttpPlatformImpl.h"
#include "util/Http.h"
#include "FastLog.h"
#include "rbx/Debug.h"
#include "util/StreamHelpers.h"
#include "util/Guid.h"
#include "util/standardout.h"
#include "util/xxhash.h"
#include "util/Statistics.h"
#include "util/ThreadPool.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/DebugSettings.h"
#include "util/RobloxGoogleAnalytics.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <string.h>
#endif

#include <cmath>

#include <curl/curl.h>
#include <sstream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/algorithm/string/replace.hpp>

#pragma warning(push)
// disable signed/unsigned mismatch with our use of std::streamsize (signed) vs size_t (unsigned) used by boost
// should be okay as long as our data is not over 2gb in size
#pragma warning(disable: 4244)
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#pragma warning(pop)

LOGGROUP(Http)
DYNAMIC_LOGGROUP(HttpTrace)
DYNAMIC_FASTFLAG(DebugHttpAsyncCallsForStatsReporting)
DYNAMIC_FASTINTVARIABLE(HttpMaxRedirects, 10)
DYNAMIC_FASTSTRINGVARIABLE(HttpCurlProxyHostAndPort, "")
DYNAMIC_FASTINTVARIABLE(HttpCacheCleanMinFilesRequired, 3000)
DYNAMIC_FASTINTVARIABLE(HttpCacheCleanMaxFilesToKeep, 1500)
DYNAMIC_FASTINTVARIABLE(HttpCacheSendStatsEveryXSeconds, 60)
DYNAMIC_FASTINTVARIABLE(HttpCacheCleanIfGBLessThan, 5)
DYNAMIC_FASTFLAGVARIABLE(HttpCacheCleanBasedOnMemory, false)
DYNAMIC_FASTFLAGVARIABLE(HttpCurlDomainTrimmingWithBaseURL, false)
DYNAMIC_FASTINTVARIABLE(HttpCurlDeepErrorReportingCount, 0)
DYNAMIC_FASTFLAGVARIABLE(HttpZeroLatencyCaching, false) // Never kill this flag, ZeroLatencyCahing will be ON only on Clients. We will still use CDN Caching on RCC & Studio
DYNAMIC_FASTFLAGVARIABLE(CleanMutexHttp, true)
DYNAMIC_FASTFLAGVARIABLE(SSLErrorLogAll, false)

using namespace RBX;
using namespace RBX::HttpPlatformImpl;

namespace
{
static std::string robloxCookieOverrideDomain;
static std::string robloxCookieOverride;

static std::string proxyHost = "";
static long proxyPort = 0;

static Http::CookieSharingPolicy cookieSharingPolicy = Http::CookieSharingUndefined;

static bool initialized = false;

static boost::shared_ptr<CURLSH> gCurlsh;

// For sharing cookie data within the same process in multiple threads.
SAFE_HEAP_STATIC(boost::mutex, curlshCookieMutex);

// For sharing ssl session information.
SAFE_HEAP_STATIC(boost::mutex, curlshSSLMutex);

// For sharing dns information.
SAFE_HEAP_STATIC(boost::mutex, curlshDNSMutex);

static void
print_cookies(const char* tag, CURL *curl)
{
  CURLcode res;
  struct curl_slist *cookies;
  struct curl_slist *nc;
  int i;

  StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "print_cookies() from:%s Curl cookies:\n", tag);
  res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
  if (res != CURLE_OK) {
    StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "print_cookies() Curl curl_easy_getinfo failed\n");
    return;
  }
  nc = cookies, i = 1;
  while (nc) {
    StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "print_cookies() [%d]: %s\n", i, nc->data);
    nc = nc->next;
    i++;
  }
  if (i == 1) {
    StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "print_cookies() (none)\n");
  }
  curl_slist_free_all(cookies);
}

static const std::string kXCSRFTokenHeaderKey = "X-CSRF-TOKEN: ";

template <typename Code>
bool logCurlError(void* caller, const char* curlOperation, Code code, bool doThrow = true);
void debugCallback(CURL* curl, curl_infotype infotype, char* dataNonTerminated, size_t dataBytes, void* userdata);

size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);

struct CurlDeleter
{
    void operator()(CURL* curl)
    {
        curl_easy_cleanup(curl);
    }
}; // struct CurlDeleter

struct CurlshDeleter
{
    void operator()(CURLSH* curlsh)
    {
        logCurlError(this, "curl_share_cleanup", curl_share_cleanup(curlsh), false);
    }
}; // struct CurlshDeleter

void cacheCleanupThreadHandler()
{
	Cache::CacheCleanOptions options;

	options.numFilesRequiredBeforeCleaning = DFInt::HttpCacheCleanMinFilesRequired;
	options.numFilesToKeep = DFInt::HttpCacheCleanMaxFilesToKeep;
	options.numGigaBytesAvailableTrigger = DFInt::HttpCacheCleanIfGBLessThan;
	options.flagCleanUpBasedOnMemory = DFFlag::HttpCacheCleanBasedOnMemory;

	Cache::cleanCache(options);
}

class CacheStatistics
{

	friend class CurlHandle;
    static CacheStatistics cacheStatistics;
	SAFE_HEAP_STATIC(boost::mutex, mutex);


    unsigned long cacheBytesInserted;
    unsigned long cacheHits;
    unsigned long cacheRequests;
	const char* appName;
#ifdef _WIN32
    //TODO -- fix me to use c++ strings
    char appPath[ MAX_PATH ];
#endif

    CacheStatistics()
        :cacheBytesInserted(0)
        ,cacheHits(0)
        ,cacheRequests(0)
		,appName("")
	{
#ifdef _WIN32
		DWORD size = GetModuleFileNameA( NULL, appPath, MAX_PATH );
		appName = strrchr(appPath, '\\');
		appName = appName ? appName + 1 : appPath;
#endif
	}

    void addHit()
    {
        const boost::mutex::scoped_lock lock(mutex());
        ++cacheHits;
        ++cacheRequests;
    }

    void addMiss(size_t bytesInserted)
    {
        const boost::mutex::scoped_lock lock(mutex());
        ++cacheRequests;
        cacheBytesInserted += bytesInserted;
    }

    void report()
    {
        const boost::mutex::scoped_lock lock(mutex());
        if (cacheRequests)
        {
			// These stats reports need to use blocking calls because the calling thread may
			// continue to run as the whole application shuts down (and continue running after
			// the stats report thread pool has been destroyed).
			Analytics::EphemeralCounter::reportCounter("HTTPCacheHit-"+DebugSettings::singleton().osPlatform() + appName, cacheHits,  !DFFlag::DebugHttpAsyncCallsForStatsReporting);
			Analytics::EphemeralCounter::reportCounter("HTTPCacheMiss-"+DebugSettings::singleton().osPlatform() + appName, cacheRequests - cacheHits,  !DFFlag::DebugHttpAsyncCallsForStatsReporting);
			Analytics::EphemeralCounter::reportCounter("HTTPCacheTotal-"+DebugSettings::singleton().osPlatform() + appName, cacheRequests,  !DFFlag::DebugHttpAsyncCallsForStatsReporting);
			Analytics::EphemeralCounter::reportStats("HTTPCacheBytes-"+DebugSettings::singleton().osPlatform() + appName, cacheBytesInserted,  !DFFlag::DebugHttpAsyncCallsForStatsReporting);

            cacheHits = cacheRequests = 0;
            cacheBytesInserted = 0;
        }
    }

	static void hit()
	{
		cacheStatistics.addHit();
	}

	static void miss(size_t bytesInserted)
	{
		cacheStatistics.addMiss(bytesInserted);
	}

public:
	static void initializeCachingThreadHandler()
	{
		// Call cacheCleanup Thread after 10 seconds after we are done reading ClientSettings
		boost::this_thread::sleep(boost::posix_time::seconds(10));
		boost::function0<void> f = boost::bind(&cacheCleanupThreadHandler);
		boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, MESSAGE_ERROR, false);
		boost::thread(thread_wrapper(g, "rbx_http_cache_clean"));

		while (true)
		{
			boost::this_thread::sleep(boost::posix_time::seconds(DFInt::HttpCacheSendStatsEveryXSeconds));
			cacheStatistics.report();
		}
	}

}; // struct CacheStatistics
CacheStatistics CacheStatistics::cacheStatistics;

void getMutexName(const char* url, std::string& hash)
{
    void* hashState = XXH32_init(123890);
    XXH32_feed(hashState, url, strlen(url));
    std::stringstream ss;
    ss << XXH32_result(hashState);
    hash = ss.str();
}

std::string sanitizeUrl(const std::string& url)
{
    std::string newUrl;
    newUrl.reserve(url.size() * 3); // make enough space to encode all characters.
    for (std::string::const_iterator it = url.begin(); url.end() != it; ++it)
    {
        if (' ' == *it)
        {
            newUrl += "%20";
        }
        else
        {
            newUrl += *it;
        }
    }
    return newUrl;
}

std::string parseHeadersForKey(const char* key, const std::string& kHeaders)
{
    size_t pos = kHeaders.find(key);
    if (std::string::npos == pos)
    {
        return std::string();
    }

    std::string headers = kHeaders.substr(kHeaders.find(key));
    return headers.substr(strlen(key), headers.find_first_of("\r\n")-strlen(key));
}

void addHeader(curl_slist*& headers, const char* header)
{
    FASTLOGS(DFLog::HttpTrace, "Adding CURL header: %s", header);
    headers = curl_slist_append(headers, header);
    if (NULL == headers)
    {
        throw RBX::runtime_error("Error adding header %s", header);
    }
}

const boost::filesystem::path& getCookieJarPath()
{
    static const boost::filesystem::path path = FileSystem::getCacheDirectory(true, "http") / "cookies.txt";
    return path;
}

void curlshLock(CURL* handle, curl_lock_data data, curl_lock_access access, void* userdata)
{
    FASTLOG2(DFLog::HttpTrace, "Locking mutex for data(%d) on CURL handle %p", data, handle);
    if (handle)
    {
        switch (data)
        {
        case CURL_LOCK_DATA_SSL_SESSION:
            curlshSSLMutex().lock();
            break;
        case CURL_LOCK_DATA_DNS:
            curlshDNSMutex().lock();
            break;
        case CURL_LOCK_DATA_COOKIE:
            curlshCookieMutex().lock();
            break;
        default:
            break;
        }
    }
}

void curlshUnlock(CURL* handle, curl_lock_data data, void* userdata)
{
    FASTLOG2(DFLog::HttpTrace, "Unlocking mutex for data(%d) on CURL handle %p", data, handle);
    if (handle)
    {
        switch (data)
        {
        case CURL_LOCK_DATA_SSL_SESSION:
            curlshSSLMutex().unlock();
            break;
        case CURL_LOCK_DATA_DNS:
            curlshDNSMutex().unlock();
            break;
        case CURL_LOCK_DATA_COOKIE:
            curlshCookieMutex().unlock();
            break;
        default:
            break;
        }
    }
}

const char* getCurlStrerror(CURLcode code)
{
    return curl_easy_strerror(code);
}

const char* getCurlStrerror(CURLSHcode code)
{
    return curl_share_strerror(code);
}

bool curlCodeOkay(CURLcode code)
{
    return CURLE_OK == code;
}

bool curlCodeOkay(CURLSHcode code)
{
    return CURLSHE_OK == code;
}

template <typename Code>
bool logCurlError(void* caller, const char* curlOperation, Code curlCode, bool doThrow)
{
    std::stringstream ss;
    if (DFLog::HttpTrace)
    {
        ss << curlOperation << "(" << caller << "): " << getCurlStrerror(curlCode);
        FASTLOGS(DFLog::HttpTrace, "%s", ss.str().c_str());
    }

    if (!curlCodeOkay(curlCode))
    {
        if (doThrow)
        {
            throw runtime_error("CURL error (%s, %p): %s", curlOperation, caller, getCurlStrerror(curlCode));
        }
        else
        {
            FASTLOGS(FLog::Http, "CURL error: %s", ss.str().c_str());
        }
    }

    return !curlCodeOkay(curlCode);
}

struct Data
{
    std::stringstream ss;
    size_t bytes;

    Data() : bytes(0) {}

    void reset()
    {
        bytes = 0;
        ss.str("");
        ss.clear();
    }
};

struct StringSink  : public boost::iostreams::sink {
    std::string& ss;
    StringSink(std::string& ss_) : ss(ss_) {}
    std::streamsize write(const char* s, std::streamsize n)
    {
        ss.append(s, static_cast<size_t>(n));
        return n;
    }
};

size_t writeCurlData(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t bytes = size * nmemb;
    Data& writeData = *(Data *)userdata;
    writeData.ss.write(ptr, bytes);
    writeData.bytes += bytes;
        
    return bytes; // number of bytes written
}

class CurlHandle
{
    const std::string url;
    const bool externalRequest;
    const HttpCache::Policy cachePolicy;
    const long connectTimeoutMillis;
    const long performTimeoutMillis;
    const boost::filesystem::path cookieJarTempPath;

    CURL* curl;
    boost::shared_ptr<CURLSH> curlsh;

    std::stringstream logBuffer;

    curl_slist* curlHeaders;

    bool isPost;
    std::string postData; // POST data is not copied into CURL and must therefore be preserved for the lifetime of the handle.

	std::string lastUsedCsrfToken;
    bool csrfTokenChanged;

    std::stringstream responseHeaders;
    std::string responseCodeReason;

    bool logCurlError(const char* curlOperation, CURLcode code)
    {
        return ::logCurlError(this, curlOperation, code, true);
    }

    bool logCurlErrorNoThrow(const char* curlOperation, CURLcode code)
    {
        return ::logCurlError(this, curlOperation, code, false);
    }

    void setupDebugger()
    {
        if (DFLog::HttpTrace || DFInt::HttpCurlDeepErrorReportingCount)
        {
            logCurlError("CURLOPT_DEBUGFUNCTION", curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, &debugCallback));
        }

        if (DFLog::HttpTrace)
        {
            logCurlError("CURLOPT_DEBUGDATA", curl_easy_setopt(curl, CURLOPT_DEBUGDATA, this));
            logCurlError("CURLOPT_VERBOSE", curl_easy_setopt(curl, CURLOPT_VERBOSE, 1));
        }
    }

    void setupResponseWriter(Data* responseStream)
    {
        logCurlError("CURLOPT_WRITEDATA", curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseStream));
        logCurlError("CURLOPT_WRITEFUNCTION", curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCurlData));
    }

    void logCacheResultStatus(const char* assetUrl, const char* cdnUrl, const char* openFailureReason, const std::string& kHeaders, size_t bytesDecompressed, std::string& binaryData)
    {
        const bool kSucceeded = '\0' == openFailureReason[0];
        if (DFLog::HttpTrace)
        {
            std::stringstream ss;
            if (kSucceeded)
            {
                ss << reinterpret_cast<void*>(this) << ", " << cdnUrl << ")";
                FASTLOGS(DFLog::HttpTrace, "Succeeded CacheResult::open(%s", ss.str().c_str());
            }
            else
            {
                ss << reinterpret_cast<void*>(this) << ", " << openFailureReason << ", " << cdnUrl << ")";
                FASTLOGS(DFLog::HttpTrace, "Failed CacheResult::open(%s", ss.str().c_str());
            }
        }

        static const char* kContentLength = "Content-Length: ";
        size_t contentLength = atoi(parseHeadersForKey(kContentLength, kHeaders).c_str());

        if (kSucceeded)
        {
            CacheStatistics::hit();
        }
        else
        {
            CacheStatistics::miss(contentLength);
            FASTLOGS(FLog::Http, "Http cache failed: %s", openFailureReason);
        }

#if defined(_DEBUG) || defined(_NOOPT)
        // Write some http download statistics to a file that can be used for detailed analyses.
        {
            static boost::mutex mutex;
            boost::mutex::scoped_lock lock(mutex);

            static boost::filesystem::path kPath = FileSystem::getCacheDirectory(true, NULL) / "httpstats.csv";
            static bool firstLogRun = true;
            static std::ofstream fout(kPath.string().c_str());

            static size_t totalBytesHit = 0;
            static size_t totalBytesMissed = 0;

            if (kSucceeded)
            {
                totalBytesHit += contentLength;
            }
            else
            {
                totalBytesMissed += contentLength;
            }

            if (firstLogRun)
            {
                firstLogRun = false;
                fout << "AssetURL,CDN_URL,FilePath,ContentEncoding,ContentType,MagicNum,ContentLength,BytesDecompressed,HitOrMiss,BytesMissedSoFar,BytesHitSoFar" << std::endl;
            }

            static const char* kContentType = "Content-Type: ";
            std::string contentType = parseHeadersForKey(kContentType, kHeaders);

            static const char* kContentEncoding = "Content-Encoding: ";
            std::string contentEncoding = parseHeadersForKey(kContentEncoding, kHeaders);

            char magic[5];
            magic[0] = ' ';
            magic[1] = '\0';
            if (binaryData.size() >= 4)
            {
                memcpy(magic, binaryData.data(), 4);
            }
            magic[4] = '\0';

            fout << assetUrl << ',' << cdnUrl << ',' << Cache::cacheFilePath(cdnUrl).string() << ',' << contentEncoding
                << ',' << contentType << ',' << magic << ','
                << contentLength << ',' << bytesDecompressed << ',' << kSucceeded
                << ',' << totalBytesMissed << ',' << totalBytesHit << std::endl;
        }
#endif
    }

public:
    CurlHandle(
        const std::string&          url,
        bool                        externalRequest,
        HttpCache::Policy           cachePolicy,
        long                        connectTimeoutMillis,
        long                        performTimeoutMillis)
        :url(url)
        ,isPost(false)
        ,externalRequest(externalRequest)
        ,cachePolicy(cachePolicy)
        ,connectTimeoutMillis(connectTimeoutMillis)
        ,performTimeoutMillis(performTimeoutMillis)
        ,cookieJarTempPath(FileSystem::getTempFilePath())
        ,curlHeaders(NULL)
        ,csrfTokenChanged(false)
        ,curlsh(gCurlsh)
    {
        if (DFLog::HttpTrace)
        {
            std::stringstream ss;
            ss << reinterpret_cast<void*>(this) << ", " << url << ", " << externalRequest << ", " << cachePolicy;
            FASTLOGS(DFLog::HttpTrace, "CurlHandle(%s)", ss.str().c_str());
        }

        curl = curl_easy_init();
        RBXASSERT(curl);
        if (NULL == curl)
        {
            throw runtime_error("Error initializing CURL handle.");
        }
        FASTLOG2(DFLog::HttpTrace, "CurlHandle(%p) using curl easy handle %p", this, curl);

        setupDebugger();
        
        
        logCurlError("CURLOPT_SSLVERSION", curl_easy_setopt(curl,  CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1));

        // Setup our handle to share data.
        logCurlError("CURLOPT_SHARE", curl_easy_setopt(curl,  CURLOPT_SHARE, curlsh.get()));

        // Setup URL to access.
        logCurlError("CURLOPT_URL", curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));

        // Setup timeout.
        logCurlError("CURLOPT_CONNECTTIMEOUT", curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connectTimeoutMillis));
        logCurlError("CURLOPT_TIMEOUT_MS", curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, performTimeoutMillis));

        // Setup proxy information, if available.
        if (!DFString::HttpCurlProxyHostAndPort.empty())
        {
            logCurlError("CURLOPT_PROXY", curl_easy_setopt(curl, CURLOPT_PROXY, DFString::HttpCurlProxyHostAndPort.c_str()));
        }
        else
        {
            if (!proxyHost.empty())
            {
                logCurlError("CURLOPT_PROXY", curl_easy_setopt(curl, CURLOPT_PROXY, proxyHost.c_str()));
            }

            if (proxyPort)
            {
                logCurlError("CURLOPT_PROXYPORT",
                             curl_easy_setopt(curl, CURLOPT_PROXYPORT, proxyPort));
            }
        }

        // Setup User-Agent.
        logCurlError("CURLOPT_USERAGENT", curl_easy_setopt(curl, CURLOPT_USERAGENT, Http::rbxUserAgent.c_str()));
                                              
        // Capture header data explicitly.
        logCurlError("CURLOPT_HEADERFUNCTION", curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &headerCallback));
        logCurlError("CURLOPT_HEADERDATA", curl_easy_setopt(curl, CURLOPT_HEADERDATA, this));

        // Send referrer field during redirects.
        logCurlError("CURLOPT_AUTOREFERER", curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1));

        if (HttpCache::PolicyDefault == cachePolicy)
        {
            // Follow redirects.
            logCurlError("CURLOPT_FOLLOWLOCATION", curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1));

            // Set max redirects: -1 means infinite, >= 0 means >= 0 redirects allowed.
            logCurlError("CURLOPT_MAXREDIRS", curl_easy_setopt(curl, CURLOPT_MAXREDIRS, DFInt::HttpMaxRedirects));
        }

        // Set Accept-Encoding header to all supported encoding schemes: identity, deflate, and gzip.
        logCurlError("CURLOPT_ACCEPT_ENCODING", curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""));

        // Don't verify SSL peers.  Less secure, but means we don't have to install certificates.
        logCurlError("CURLOPT_SSL_VERIFYPEER", curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0));

        // Don't use signals, which will prevent problems with resolver timeouts in multi-threaded environments.
        // XXX Might need to setup SSL's mutex callback mechanism, per this URL:
        // http://curl.haxx.se/mail/lib-2010-12/0345.html
        logCurlError("CURLOPT_NOSIGNAL", curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1));

        RBXASSERT(initialized ? Http::CookieSharingUndefined != cookieSharingPolicy : Http::CookieSharingUndefined == cookieSharingPolicy);
        if (cookieSharingPolicy & Http::CookieSharingMultipleProcessesRead)
        {
            // Load cookies from the cookie jar.
            logCurlError("CURLOPT_COOKIEFILE", curl_easy_setopt(curl, CURLOPT_COOKIEFILE, getCookieJarPath().string().c_str()));
            boost::system::error_code ec;
            boost::filesystem::last_write_time( getCookieJarPath(), time(0), ec ); // update access time so cache doesn't remove it as easily
        }
        if (cookieSharingPolicy & Http::CookieSharingMultipleProcessesWrite)
        {
            // Write cookies to a cookie jar.
            logCurlError("CURLOPT_COOKIEJAR", curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieJarTempPath.string().c_str()));
        }

        // Don't load session cookies from the previous session.
        logCurlError("CURLOPT_COOKIESESSION", curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1));
    }

    ~CurlHandle()
    {
        FASTLOG1(DFLog::HttpTrace, "~CurlHandle(%p)", this);
        curl_easy_cleanup(curl);
        curl_slist_free_all(curlHeaders);

        // Attempt to move the cookies to the right location if we are a cookie jar writer.
        if (Http::CookieSharingMultipleProcessesWrite == cookieSharingPolicy)
        {
            try
            {
                boost::filesystem::rename(cookieJarTempPath, getCookieJarPath());
            }
            catch (...)
            {
                boost::system::error_code ec;
                boost::filesystem::remove(cookieJarTempPath, ec);
            }
        }
    }

    bool getCsrfTokenChanged() const
    {
        return csrfTokenChanged;
    }

    const std::string& getResponseCodeReason() const
    {
        return responseCodeReason;
    }

    void addLogLine(const char* line)
    {
        logBuffer << line;
    }

    // If we are doing caching, make sure we capture the data.
    void addResponseHeader(char* buffer, size_t size, size_t nitems)
    {
        if (HttpCache::PolicyDefault != cachePolicy)
        {
            responseHeaders.write(buffer, size*nitems);
        }

        // Replace CRLF with a null-terminator to make handling easier.
        buffer[size*nitems-2] = '\0';

        if (DFLog::HttpTrace)
        {
            std::stringstream ss;
            ss << this << "): " << buffer;
            FASTLOGS(DFLog::HttpTrace, "headerCallback(%s", ss.str().c_str());
        }

        if (responseCodeReason.empty())
        {
            responseCodeReason = buffer;
        }

        static const size_t tokenHeaderKeyLength = kXCSRFTokenHeaderKey.size();
        if (size*nitems > tokenHeaderKeyLength && 0 == strncmp(kXCSRFTokenHeaderKey.c_str(), buffer, tokenHeaderKeyLength))
        {
            FASTLOGS(DFLog::HttpTrace, "Found CSRF token: %s", buffer);

            buffer += tokenHeaderKeyLength;
            if (lastUsedCsrfToken != buffer)
                updateCsrfToken(buffer);
        }
    }

    void setupPostData(
        std::istream&    dataStream,
        bool             compressData
        )
    {
        // if compressing data, set the Content-Encoding header to gzip and deflate our data
        if (compressData)
        {
            namespace io = boost::iostreams;
            try
            {
                std::stringstream postStream;
                const io::gzip_params gzipParams = io::gzip::default_compression;
                io::filtering_ostream out;
                if (compressData)
                {
                    out.push(io::gzip_compressor(gzipParams));
                }
                out.push(StringSink(postData));
                io::copy(dataStream, out);
            }
            catch (io::gzip_error& e)
            {
                int e1 = e.error();
                int e2 = e.zlib_error_code();
                throw RBX::runtime_error("Upload GZip error %d, ZLib error %d, \"%s\"", e1, e2, url.c_str());
            }

            addHeader(curlHeaders, "Content-Encoding: gzip");
        }
        else
        {
            readStreamIntoString(dataStream, postData);
        }

        isPost = true;
        if (!externalRequest)
        {
            std::string token = Http::getLastCsrfToken();
            lastUsedCsrfToken = token;
            
            if (!token.empty())
            {
                const std::string header = kXCSRFTokenHeaderKey + token;
                addHeader(curlHeaders, header.c_str());
            }
        }

        // Set POST data.  Do not send it in chunked format.
        // Review http://curl.haxx.se/libcurl/c/curl_easy_setopt#CURLOPTPOST if chunked
        // post requests are necessary.
        logCurlError("CURLOPT_POSTFIELDS", curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &postData[0]));
        logCurlError("CURLOPT_POSTFIELDSIZE_LARGE",
                     curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)postData.size()));
    }

    void setupHeaders(
        const std::string&                  contentType,
        const HttpAux::AdditionalHeaders&   headers
        )
    {
        FASTLOG(DFLog::HttpTrace, "Setting up CURL headers.");
        RBXASSERT(!curlHeaders);

        if (contentType.size())
        {
            std::string header = "Content-Type: " + contentType;
            addHeader(curlHeaders, header.c_str());
        }

        for (HttpAux::AdditionalHeaders::const_iterator it = headers.begin();
             headers.end() != it; ++it)
        {
            std::string header = it->first + ": " + it->second;
            addHeader(curlHeaders, header.c_str());
        }

        if (DFLog::HttpTrace && curlHeaders)
        {
            for (curl_slist *list = curlHeaders; list; list = list->next)
            {
                std::stringstream ss;
                ss << reinterpret_cast<void*>(this) << "): " << list->data;
                FASTLOGS(DFLog::HttpTrace, "CURL header(%s", ss.str().c_str());
            }
        }

        if (curlHeaders)
        {
            logCurlError("CURLOPT_HTTPHEADER", curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders));
        }
    }

    void updateCsrfToken(const char* token)
    {
        RBXASSERT(curlHeaders);

        Http::setLastCsrfToken(token); // set the global CSRF token
        lastUsedCsrfToken = token;

        csrfTokenChanged = true;
        std::string header = kXCSRFTokenHeaderKey + token;

        if (isPost && !externalRequest)
        {
            bool foundCsrfHeader = false;
            curl_slist* list = curlHeaders;
            for (; list && !foundCsrfHeader; list = list->next)
            {
                foundCsrfHeader = 0 == strncmp(
                    list->data,
                    kXCSRFTokenHeaderKey.c_str(),
                    std::min(strlen(list->data), kXCSRFTokenHeaderKey.size()));
                
                if (foundCsrfHeader) break;
            }

            if (foundCsrfHeader)
            {
                // This follows curl_slist_append(3) closely re: memory management.
                char* dupdata = strdup(header.c_str());

                if (!dupdata)
                {
                    char* errmsg = strerror(errno);
                    throw RBX::runtime_error("CurlHandle(%p), strdup failure: %s", this, errmsg);
                }

                if (DFLog::HttpTrace)
                {
                    const std::string change = std::string(list->data) + " ==> " + dupdata;
                    FASTLOGS(DFLog::HttpTrace, "Updating header: ", change.c_str());
                }

                free(list->data);
                list->data = dupdata;
            }
            else
            {
                if (!lastUsedCsrfToken.empty())
                    addHeader(curlHeaders, header.c_str());
            }
        }
    }

	bool getOldCachedData(std::string& response)
	{
		if (!isPost && HttpCache::PolicyFinalRedirect == cachePolicy)
		{
			Data responseData;
			setupResponseWriter(&responseData);
		
			responseHeaders.str("");
			responseHeaders.clear();
			responseData.reset();

			// Check the URL against our cache of known good 200s before doing any further perform()'s.
#ifdef _WIN32
            if (DFFlag::CleanMutexHttp)
            {
			std::string name;
			getMutexName(url.c_str(), name);
			ScopedNamedMutex lock(name.c_str());
            }
#endif
			Cache::CacheResult cacheResult = Cache::CacheResult::open(url.c_str(), NULL);

			if (cacheResult.isValid())
			{
				responseHeaders.write(reinterpret_cast<const char*>(cacheResult.getResponseHeader().data()), cacheResult.getCacheHeader().responseHeadersSize);
				responseData.bytes = cacheResult.getCacheHeader().responseBodySize;
				responseData.ss.write(reinterpret_cast<const char*>(cacheResult.getResponseBody().data()), responseData.bytes);
				std::vector<char> buffer(responseData.bytes);
				responseData.ss.read(buffer.data(), responseData.bytes);
				response.assign(buffer.begin(), buffer.end());
				return true;
			}
		}

		return false;
	}


    long perform(std::string& response, bool zeroLatencyCaching)
    {
		responseCodeReason.clear();
        csrfTokenChanged = false;
		
        Data responseData;
        setupResponseWriter(&responseData);

        {
            static boost::mutex mutex;

            // See note at the top of this file regarding curl_easy_perform.
            CURLcode retcode = curl_easy_perform(curl);

            switch (retcode)
            {
            case CURLE_SSL_CONNECT_ERROR:
                {
                    if (DFFlag::SSLErrorLogAll)
                    {
                    static volatile int count = 0;

                    if (count < DFInt::HttpCurlDeepErrorReportingCount)
                    {
                        ++count;

                        boost::mutex::scoped_lock lock(mutex);
                        for (std::string line; std::getline(logBuffer, line); )
                        {
                            RobloxGoogleAnalytics::sendEventRoblox("CURLE_SSL_CONNECT_ERROR", "custom", Http::urlEncode(line).c_str());
                        }
                    }
                    }
                }
                break;

            case CURLE_OK:
                break;

            default: // all other errors
                break;
            }

            logCurlError("curl_easy_perform", retcode);
        }

        long responseCode;
        logCurlError("CURLINFO_RESPONSE_CODE", curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode));

        // Cache the URL after redirects are finished.
        bool cacheFailed = HttpCache::PolicyDefault != cachePolicy;
        std::string redirectUrl;
        std::string cacheInvalidReason;
        if (!isPost && HttpCache::PolicyFinalRedirect == cachePolicy)
        {
            for (int redirects = 0;
                (302 == responseCode || 301 == responseCode) && (redirects < DFInt::HttpMaxRedirects || -1 == DFInt::HttpMaxRedirects);
                ++redirects)
            {
                cacheFailed = false;
                responseHeaders.str("");
                responseHeaders.clear();
                responseCodeReason.clear();
                responseData.reset();

                char* tmpRedirectUrl = NULL;
                logCurlError("CURLINFO_REDIRECT_URL", curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &tmpRedirectUrl));
                if (tmpRedirectUrl)
                {
                    redirectUrl = tmpRedirectUrl;
                }

                // Check the URL against our cache of known good 200s before doing any further perform()'s.

                
#ifdef _WIN32
                if (DFFlag::CleanMutexHttp)
                {
				std::string name;
				getMutexName(zeroLatencyCaching ? url.c_str() : tmpRedirectUrl, name);
				ScopedNamedMutex lock(name.c_str());
                }
#endif
				Cache::CacheResult cacheResult = Cache::CacheResult::open(zeroLatencyCaching ? url.c_str() : NULL, tmpRedirectUrl);

				if ((cacheFailed = !cacheResult.isValid()))
				{
					cacheInvalidReason = cacheResult.getInvalidReason();

					logCurlError("CURLOPT_URL", curl_easy_setopt(curl, CURLOPT_URL, redirectUrl.c_str()));
					logCurlError("curl_easy_perform", curl_easy_perform(curl));
					logCurlError("CURLINFO_RESPONSE_CODE", curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode));
				}
				else
				{
					responseHeaders.write(reinterpret_cast<const char*>(cacheResult.getResponseHeader().data()), cacheResult.getCacheHeader().responseHeadersSize);
					responseData.bytes = cacheResult.getCacheHeader().responseBodySize;
					responseData.ss.write(reinterpret_cast<const char*>(cacheResult.getResponseBody().data()), responseData.bytes);
					responseCode = cacheResult.getCacheHeader().responseCode;
				}
            }
        }

        std::vector<char> buffer(responseData.bytes);
        responseData.ss.read(buffer.data(), responseData.bytes);
        response.assign(buffer.begin(), buffer.end());

        switch (cachePolicy)
        {
        case HttpCache::PolicyFinalRedirect:
            if (!isPost)
            {
                // Use final URL as key to update cache.
                const std::string strHeaders = responseHeaders.str();
                if (cacheFailed)
                {
                    Cache::Data headers(strHeaders.data(), strHeaders.size());
                    Cache::Data body(response.data(), response.size());

					
#ifdef _WIN32
                    if (DFFlag::CleanMutexHttp)
                    {
					std::string name;
					getMutexName(zeroLatencyCaching ? url.c_str() : redirectUrl.c_str(), name);
					ScopedNamedMutex lock(name.c_str());
                    }
#endif

					try
					{
						Cache::CacheResult::update(zeroLatencyCaching ? url.c_str() : NULL, redirectUrl.c_str(), responseCode, headers, body);
					}
					catch (const std::runtime_error& e)
					{
						FASTLOGS(DFLog::HttpTrace, "Cache update failed: %s", e.what());
						cacheInvalidReason = std::string("Update failed: ") + e.what();
					}
                }
                logCacheResultStatus(url.c_str(), redirectUrl.c_str(), cacheInvalidReason.c_str(), strHeaders, responseData.bytes, response);
            }
            break;
        case HttpCache::PolicyDefault:
            // Do no caching in this case.
            break;
        }

        // Print our cookies to fastlog.
        if (DFLog::HttpTrace)
        {
            struct curl_slist *cookies;
            if (!logCurlErrorNoThrow("CURLINFO_COOKIELIST", curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies)))
            {
                if (cookies)
                {
                    for (struct curl_slist *curr = cookies; curr; curr = curr->next)
                    {
                        std::stringstream ss;
                        ss << reinterpret_cast<void*>(this) << "): " << curr->data;
                        FASTLOGS(DFLog::HttpTrace, "CURL cookie(%s", ss.str().c_str());
                    }
                    curl_slist_free_all(cookies);
                }
                else
                {
                    FASTLOG1(DFLog::HttpTrace, "No cookies(%p)", this);
                }
            }
        }

        return responseCode;
    }
}; // class CurlHandle

size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    CurlHandle* curl = reinterpret_cast<CurlHandle*>(userdata);
    FASTLOG3(DFLog::HttpTrace, "headerCallback(size=%d, nitems=%d, userdata=%p)", size, nitems, userdata);

    curl->addResponseHeader(buffer, size, nitems);

    return size*nitems;
}

void debugCallback(CURL* curl, curl_infotype infotype, char* dataNonTerminated, size_t dataBytes, void* userdata)
{
    const static char *infotype_map[CURLINFO_END] = {
        "CURLINFO_TEXT",
        "CURLINFO_HEADER_IN",
        "CURLINFO_HEADER_OUT",
        "CURLINFO_DATA_IN",
        "CURLINFO_DATA_OUT",
        "CURLINFO_SSL_DATA_IN",
        "CURLINFO_SSL_DATA_OUT",
    };

    boost::scoped_array<char> data(new char[dataBytes+1]);
    memcpy(data.get(), dataNonTerminated, dataBytes);
    data[dataBytes] = '\0';

    std::stringstream ss;
    ss << infotype_map[infotype] << "(" << userdata << "): ";
    ss << data.get();

    CurlHandle* handle = reinterpret_cast<CurlHandle*>(userdata);
    handle->addLogLine(data.get());

    FASTLOGS(DFLog::HttpTrace, "Curl debug callback: %s", ss.str().c_str());
}
} // namespace

namespace RBX
{
namespace HttpPlatformImpl
{

	static const int kNumberThreadPoolThreads = 4;
	static ThreadPool* threadPool;

void init(Http::CookieSharingPolicy cookieSharingPolicy)
{
    RBXASSERT(!initialized);
    if (!initialized)
    {
		static boost::scoped_ptr<ThreadPool> tp(new ThreadPool(kNumberThreadPoolThreads));
		threadPool = tp.get();

        ::cookieSharingPolicy = cookieSharingPolicy;

#ifndef __ANDROID__ // Android initializes curl in JNIMain.cpp
        logCurlError(NULL, "curl_global_init", curl_global_init(CURL_GLOBAL_DEFAULT | CURL_GLOBAL_ACK_EINTR));
#endif

        gCurlsh.reset(curl_share_init(), ::CurlshDeleter());

        logCurlError(NULL, "CURLSHOPT_LOCKFUNC", curl_share_setopt(gCurlsh.get(), CURLSHOPT_LOCKFUNC, &curlshLock));
        logCurlError(NULL, "CURLSHOPT_UNLOCKFUNC", curl_share_setopt(gCurlsh.get(), CURLSHOPT_UNLOCKFUNC, &curlshUnlock));
        logCurlError(NULL, "CURLSHOPT_SHARE", curl_share_setopt(gCurlsh.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE));
        logCurlError(NULL, "CURLSHOPT_SHARE", curl_share_setopt(gCurlsh.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS));
        logCurlError(NULL, "CURLSHOPT_SHARE", curl_share_setopt(gCurlsh.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION));
        
        setCookiesForDomain(robloxCookieOverrideDomain, robloxCookieOverride);

        {
            boost::function0<void> f = boost::bind(&CacheStatistics::initializeCachingThreadHandler);
            boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, MESSAGE_ERROR, false);
            boost::thread(thread_wrapper(g, "rbx_http_cache_stats_report"));
        }

        initialized = true;
    }
}

void setProxy(const std::string& host, long port)
{
    proxyHost = host;
    proxyPort = port;
}
    
void setCookiesForDomain(const std::string& domain, const std::string& cookies)
{
    if (gCurlsh)
    {
        if (domain.empty() || cookies.empty())
            return;
        
        // Do Domain trimming only for BaseURL's, only trim www. or m.,
        // This allows to propogate cookies accross www, m, web, api for all roblox baseURL sub domains
        std::string trimmedDomain = domain;
        if (DFFlag::HttpCurlDomainTrimmingWithBaseURL)
        {
            std::string coreBaseURL = ::GetBaseURL();
            boost::replace_all(coreBaseURL, "http://", "");
            boost::replace_all(coreBaseURL, "/", "");
            
            boost::replace_all(trimmedDomain, "http://", "");
            boost::replace_all(trimmedDomain, "https://", "");
            
            if (trimmedDomain.find_first_of(coreBaseURL) == 0)
            {
                if (trimmedDomain.find_first_of("www.") == 0)
                    boost::replace_all(trimmedDomain, "www.", "");
                else if(trimmedDomain.find_first_of("m.") == 0)
                    boost::replace_all(trimmedDomain, "m.", "");
                else
                    return; // do not propogate cookies for anything except www.BaseURL or m.BaseURL to BaseURL
            }
            else
                return; // Do no store cookies if it is not our baseURL
        }
        else
        {
#ifndef RBX_STUDIO_BUILD
            std::string::size_type pos = trimmedDomain.find_first_of(".");
            if (pos != std::string::npos)
            {
                trimmedDomain = trimmedDomain.substr(pos);
            }
#endif
        }
        
        FASTLOGS(DFLog::HttpTrace, "Setting cookies for domain: %s", trimmedDomain.c_str());
        
        boost::shared_ptr<CURL> curl(curl_easy_init(), CurlDeleter());
        RBXASSERT(curl);
        if (NULL == curl)
        {
            throw runtime_error("Error initializing CURL handle.");
        }
        
        logCurlError(NULL, "CURLOPT_SHARE", curl_easy_setopt(curl.get(), CURLOPT_SHARE, gCurlsh.get()));
        
        size_t semicolon = 0;
        size_t lastSemicolon = 0;
        do
        {
            semicolon = cookies.find_first_of(";", lastSemicolon);
            size_t equals = cookies.find_first_of("=", lastSemicolon);

            std::string key = cookies.substr(lastSemicolon, equals-lastSemicolon);
            std::string value = cookies.substr(equals+1, semicolon-equals-1);

            std::stringstream cookie;
            cookie << "#HttpOnly_" << trimmedDomain << "\tTRUE\t/\tFALSE\t0\t" << key << "\t" << value;
            FASTLOGS(DFLog::HttpTrace, "Setting domain cookie with: %s", cookie.str().c_str());
            logCurlError(NULL, "CURLOPT_COOKIELIST", curl_easy_setopt(curl.get(), CURLOPT_COOKIELIST, cookie.str().c_str()));
            
            lastSemicolon = semicolon + 2; // pass "; "
        } while (std::string::npos != semicolon);
    }
    else
    {
        robloxCookieOverrideDomain = domain;
        robloxCookieOverride = cookies;
    }
}

// note: domain currently ignored? Returns all cookies
void getCookiesForDomain(const std::string& domain, std::string& cookies)
{
  boost::shared_ptr<CURL> curl(curl_easy_init(), CurlDeleter());
  RBXASSERT(curl);
  if (NULL == curl)
  {
      throw runtime_error("Error initializing CURL handle.");
  }

  logCurlError(NULL, "CURLOPT_SHARE", curl_easy_setopt(curl.get(), CURLOPT_SHARE, gCurlsh.get()));

  CURLcode res;
  struct curl_slist *cookiesList;
  struct curl_slist *nc;
  int i;

  std::stringstream cookieStr;

  res = curl_easy_getinfo(curl.get(), CURLINFO_COOKIELIST, &cookiesList);
  if (res != CURLE_OK) {
    return;
  }
  nc = cookiesList, i = 1;
  if(nc != NULL){
    cookieStr << nc->data;
    nc = nc->next;
    i++;
  }
  while (nc) {
    cookieStr << "; " << nc->data;
    nc = nc->next;
    i++;
  }
  
  // return combined cookie string
  cookies = cookieStr.str();

  curl_slist_free_all(cookiesList);
}

void updateCachedData(shared_ptr<CurlHandle> curlHandle)
{
	std::string response;
	try
	{
		curlHandle->perform(response, true);
	}
	catch (RBX::base_exception& e)
	{
		// Basically we do a retry of a Get request at the HTTP level if the exception are thrown
		// But this is different and is not important, as no one is waiting for this data
		// This perform is only meant to do a update of the cached CDN data if it ever changed for the next session
		// I do not see a reason here to do a retry as it is OK if we do not update the cache
		// If someone feel strongly to retry then we can do similar to what we do in 
		// UGLY http.cpp -   void Http::get(std::string& response, bool externalRequest) 
		FASTLOGS(DFLog::HttpTrace, "Failed to update Cache: %s", e.what());
	}
}

void perform(HttpOptions& options, std::string& response)
{
    const std::string url = sanitizeUrl(options.url);
	shared_ptr<CurlHandle> curlHandle(new CurlHandle(url, options.externalRequest, options.cachePolicy, options.connectTimeoutMillis, options.performTimeoutMillis));
    curlHandle->setupHeaders(*options.hdrContentType, *options.addlHeaders);

    if (options.postData)
    {
        curlHandle->setupPostData(*options.postData, options.compressedPostData);
    }
#ifndef RBX_STUDIO_BUILD
	else
	{
		if (DFFlag::HttpZeroLatencyCaching && HttpCache::PolicyFinalRedirect == options.cachePolicy && curlHandle->getOldCachedData(response))
		{
			// We got the old cached data, create a boost thread to update the cache data & then return the old response
			threadPool->schedule(boost::bind(&RBX::HttpPlatformImpl::updateCachedData, curlHandle));
			return;
		}
	}
#endif

    long statusCode = 0;
    do
    {
#ifdef RBX_STUDIO_BUILD
		statusCode = curlHandle->perform(response, false);
#else
		statusCode = curlHandle->perform(response, DFFlag::HttpZeroLatencyCaching);
#endif
    } while (options.postData && 403 == statusCode && curlHandle->getCsrfTokenChanged());

	// 202 is not treated as success, with 202 we call again(Look in in AsynHttpQueue.cpp) after some time and hopefully we will get a 200 OK once server is done processing
	if ( statusCode < 200 || statusCode > 299 || statusCode == 202 )
    {
        FASTLOG2(DFLog::HttpTrace, "CurlHandle(%p) error status: %d", curlHandle.get(), statusCode);
        throw RBX::http_status_error(statusCode, curlHandle->getResponseCodeReason());
    }
}
} // namespace HttpPlatformImpl
} // namespace RBX
