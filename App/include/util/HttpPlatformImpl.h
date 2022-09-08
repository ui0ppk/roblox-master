#pragma once
#define _CRT_SECURE_NO_WARNINGS 1

#include "util/Http.h"
#include "util/HttpAux.h"
#include "util/FileSystem.h"
#include "util/NamedMutex.h"
#include "rbx/Debug.h"

#include <boost/filesystem.hpp>

namespace RBX
{
namespace HttpPlatformImpl
{
namespace Cache
{
// Given a url, provide hashed file location on disk.
boost::filesystem::path cacheFilePath(const char* url);

struct Header
{
#define RBX_CACHE_FILE_MAGIC            0x52425848 // RBXH
#define RBX_CACHE_FILE_VERSION          0x1
#define RBX_CACHE_URL_MAX_LENGTH        1024U

    const uint32_t magic;
    const uint32_t version;
    const uint32_t urlBytes;
    const uint8_t  url[RBX_CACHE_URL_MAX_LENGTH]; // not null-terminated
    const uint32_t responseCode;
    const uint32_t responseHeadersSize;
    const uint32_t responseHeadersHash;
    const uint32_t responseBodySize;
    const uint32_t responseBodyHash;
    const uint32_t reserved;
};

class Data
{
    const uint8_t* underlying;
    const size_t bytes;

public:
    Data(const uint8_t* data, const size_t bytes) :
        underlying(data), bytes(bytes)
    {}

    Data(const char* data, const size_t bytes) :
        underlying(reinterpret_cast<const uint8_t*>(data)), bytes(bytes)
    {
        RBXASSERT(sizeof(char) == sizeof(uint8_t));
    }

    uint8_t operator[](size_t index) const
    {
        return underlying[index];
    }

    const uint8_t* data() const
    {
        return underlying;
    }

    std::string toString() const
    {
        std::string result;
        result.assign(underlying, underlying+bytes);
        return result;
    }

    size_t size() const
    {
        return bytes;
    }
};

// The data in this structure is read-only, and if you
// want to update the underlying data, you must do it via
// the update() method.
struct CacheEntry // Word-aligned data structure
{
    Header cacheHeader;
    uint8_t data[1];

    const Data getResponseHeader() const;
    const Data getResponseBody() const;

    CacheEntry(const Header& cacheHeader, const Data& responseHeaders, const Data& responseBody);
};

class CacheResult
{
    boost::shared_ptr<CacheEntry> cacheEntry;
    size_t                        cacheSize;
    const std::string             invalidReason;

public:
    explicit CacheResult(const std::string& invalidReason) : invalidReason(invalidReason)
    {}

    explicit CacheResult( shared_ptr<CacheEntry> entry, size_t size) 
        :  cacheEntry(entry), cacheSize(size)
    {
        RBXASSERT(size);
    }

    bool isValid() const
    {
        return NULL != cacheEntry;
    }

    const std::string& getInvalidReason() const
    {
        return invalidReason;
    }

    const Header& getCacheHeader() const
    {
        return cacheEntry->cacheHeader;
    }

    const Data getResponseHeader() const
    {
        return cacheEntry->getResponseHeader();
    }
    const Data getResponseBody() const
    {
        return cacheEntry->getResponseBody();
    }

    const size_t size() const
    {
        return cacheSize;
    }


    // Converts the URL to a known location on disk and tries to open and return that file.
    // Returns NULL if file could not be opened.
    static CacheResult open(const char* assetUrl, const char* cdnUrl);

    // Atomically update the file with new data and returned a new CacheEntry to it.
    static CacheResult update(const char* assetUrl, const char* cdnUrl, const uint32_t responseCode, const Data& headers, const Data& body);
};

struct CacheCleanOptions
{
    size_t numFilesRequiredBeforeCleaning;
    size_t numFilesToKeep;
	size_t numGigaBytesAvailableTrigger;
	bool flagCleanUpBasedOnMemory;
};

void cleanCache(const CacheCleanOptions& options);
} // namespace Cache

struct HttpOptions
{
    // Basic data
    const std::string& url;
    bool externalRequest;
    HttpCache::Policy cachePolicy;

    // Connection handling information
    long connectTimeoutMillis;
    long performTimeoutMillis;

    // Post data
    std::istream* postData;
    bool compressedPostData;

    // Header data
    std::string const* hdrContentType;
    HttpAux::AdditionalHeaders const* addlHeaders;

    HttpOptions(const std::string& url, bool externalRequest, HttpCache::Policy cachePolicy, long connectTimeoutMillis, long performTimeoutMillis)
        :url(url)
        ,externalRequest(externalRequest)
        ,cachePolicy(cachePolicy)
        ,connectTimeoutMillis(connectTimeoutMillis)
        ,performTimeoutMillis(performTimeoutMillis)
        ,postData(NULL)
        ,hdrContentType(NULL)
        ,addlHeaders(NULL)
    {}

    void setPostData(std::istream* dataStream, bool compressed)
    {
        postData = dataStream;
        compressedPostData = compressed;
    }

    void setHeaders(const std::string* contentType, const HttpAux::AdditionalHeaders* headers)
    {
        hdrContentType = contentType;
        addlHeaders = headers;
    }
}; // struct HttpOptions

void init(Http::CookieSharingPolicy cookieSharingPolicy); // NOTE: This call is not thread-safe.
void setCookiesForDomain(const std::string& domain, const std::string& cookies);
void getCookiesForDomain(const std::string& domain, std::string& cookies);
boost::filesystem::path getRobloxCookieJarPath();
void setProxy(const std::string& host, long port = 0);
void perform(HttpOptions& options, std::string& response);
} // namespace HttpPlatformImpl
} // namespace RBX
