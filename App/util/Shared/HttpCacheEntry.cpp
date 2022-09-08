#include "util/HttpPlatformImpl.h"
#include "util/MD5Hasher.h"
#include "util/xxhash.h"
#include "RbxFormat.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

DYNAMIC_LOGGROUP(HttpTrace)

using namespace RBX;
using namespace RBX::HttpPlatformImpl::Cache;
using namespace RBX::HttpCache;

using boost::shared_ptr;
static boost::mutex assetCacheMutex;

enum ListDirectoryAttribute
{
    ListAttributeNone = 0,
    ListAttributeModTime = 1 << 0,
};

struct DirectoryEntry
{
    boost::filesystem::path path;
    time_t lastWriteTime;
};
typedef std::vector<DirectoryEntry> DirectoryEntries;

static void listDirectory(const boost::filesystem::path& path, unsigned attributes, DirectoryEntries& paths)
{
    paths.reserve(1000); // reduce risk of resizing & copying

    DirectoryEntry dirent;
    boost::system::error_code ec;
    if (!path.empty() && boost::filesystem::exists(path, ec) && !ec)
    {
        boost::filesystem::directory_iterator end_iter;
        for (boost::filesystem::directory_iterator iter(path); end_iter != iter; ++iter)
        {
            dirent.path = *iter;
            dirent.lastWriteTime = ListAttributeModTime & attributes ? boost::filesystem::last_write_time(*iter) : 0;

            paths.push_back(dirent);
        }
    }
}


static const uint32_t& gMagic()
{
    static const uint32_t m = htonl(RBX_CACHE_FILE_MAGIC);
    return m;
}

static const boost::filesystem::path gCachePath()
{
    static const boost::filesystem::path path = FileSystem::getCacheDirectory(true, "http");
    return path;
}

static uint32_t calculateHash(const Data& data)
{
    const uint8_t* input = data.data();
    size_t size = data.size();

    if (!size)
    {
        return 0;
    }

    void* state = XXH32_init(12903780);

    uint8_t aligned[65536]; // 64 KB aligned memory for feeding the hash
    while (size)
    {
        size_t bytes = std::min(size, sizeof(aligned));
        memcpy(aligned, input, bytes);
        input += bytes;
        size -= bytes;
        
        XXH32_feed(state, aligned, bytes);
    }

    return XXH32_result(state);
}

struct SortDirEntByModTime
{
    bool operator()(const DirectoryEntries::value_type& lhs, const DirectoryEntries::value_type& rhs)
    {
        return lhs.lastWriteTime < rhs.lastWriteTime;
    }
};


namespace RBX
{
namespace HttpPlatformImpl
{
namespace Cache
{
boost::filesystem::path cacheFilePath(const char* url)
{
    shared_ptr<MD5Hasher> hasher(MD5Hasher::create());
    hasher->addData(url);
    boost::filesystem::path path = gCachePath() / hasher->toString();
    if (DFLog::HttpTrace)
    {
        std::stringstream ss;
        ss << url << "): " << path.string();
        FASTLOGS(DFLog::HttpTrace, "cacheFilePath(%s", ss.str().c_str());
    }
    return path;
}

void cleanCache(const CacheCleanOptions& options)
{
    DirectoryEntries dirents;
	boost::mutex::scoped_lock lock(assetCacheMutex);
    listDirectory(gCachePath(), ListAttributeModTime, dirents);

    boost::system::error_code ec;
    boost::filesystem::space_info s = boost::filesystem::space(gCachePath(), ec);
	boost::uintmax_t availableSpaceInGB = s.available/(1024*1024*1024);

    if (dirents.size() >= options.numFilesRequiredBeforeCleaning ||
       (options.flagCleanUpBasedOnMemory && availableSpaceInGB <= options.numGigaBytesAvailableTrigger))
    {
        std::sort(dirents.begin(), dirents.end(), SortDirEntByModTime());

        std::iterator_traits<DirectoryEntries::iterator>::difference_type numFilesToKeep = options.numFilesToKeep;
        for (DirectoryEntries::iterator it = dirents.begin();
            dirents.end() != it && std::distance(it, dirents.end()) > numFilesToKeep;
            ++it)
        {
            if (!boost::filesystem::is_directory(it->path))
            {
                FASTLOGS(DFLog::HttpTrace, "Unlinking %s", it->path.string().c_str());
                boost::system::error_code ec;
                boost::filesystem::remove(it->path, ec);
            }
        }
    }
}



} // namespace Cache
} // namespace HttpPlatformImpl
} // namespace RBX


static void* readWholeFile(const boost::filesystem::path& path, size_t* size)
{
    size_t sz = 0;
    FILE* vf = 0;
    char* data = 0;

    if (size) *size = 0;
    do
    {
#ifdef _WIN32
        vf = _wfopen(path.native().c_str(), L"rb");
#else
        vf = fopen(path.native().c_str(), "rb");
#endif
        if (!vf) break;

        if (0 != fseek(vf, 0, SEEK_END)) break;
        sz = (size_t)ftell(vf);
        if (0 != fseek(vf, 0, SEEK_SET)) break;

        data = new char[sz];
        if (!data) break;
        if (sz != fread(data, 1, sz, vf)) break;
        
        fclose(vf);
        if (size) *size = sz;
        return data;
    }
    while (0);
    
    if(data) delete[] data;
    if(vf) fclose(vf);
    return 0;
}

static bool writeWholeFile( const boost::filesystem::path& path, const void* buffer, size_t size )
{
    FILE* vf = 0;
    do
    {
#ifdef _WIN32
        vf = _wfopen(path.native().c_str(), L"wb");
#else
        vf = fopen(path.native().c_str(), "wb");
#endif
        if (!vf) break;
        if (size != fwrite(buffer, 1, size, vf)) break;
        fclose(vf);
        return true;
    }
    while (0);
    if(vf) fclose(vf);
    return false;
}


static void ceDeleter( RBX::HttpPlatformImpl::Cache::CacheEntry * e ) { delete[] ( (char*)e ); }

CacheResult CacheResult::open(const char* assetUrl, const char* cdnUrl)
{
    try
    {
		boost::mutex::scoped_lock lock(assetCacheMutex);
        boost::filesystem::path filepath = cacheFilePath(assetUrl ? assetUrl : cdnUrl);
        size_t filesize = 0;

        void* data = readWholeFile( filepath, &filesize ); // native returns a wide char string on windows and an utf-8 string on posix (hopefully?)
        if (!data)
        {
            return CacheResult("no file");
        }
        
        boost::shared_ptr<CacheEntry> ce((CacheEntry*)data, ceDeleter);

        const Header &hdr = ce->cacheHeader;
        if (gMagic() != hdr.magic)
        {
            return CacheResult("CacheEntry::open: magic failed");
        }

        if (RBX_CACHE_FILE_VERSION != hdr.version)
        {
            return CacheResult("CacheEntry::open version failed");
        }

		if (assetUrl)
		{
			if (cdnUrl && (strlen(cdnUrl) != hdr.urlBytes || 0 != memcmp(hdr.url, cdnUrl, hdr.urlBytes)))
			{
				return CacheResult("CacheEntry::open: url match failed");
			}
		}
		else
		{
			if (strlen(cdnUrl) != hdr.urlBytes || 0 != memcmp(hdr.url, cdnUrl, hdr.urlBytes)) // This is actually a CDN URL passed in
			{
				return CacheResult("CacheEntry::open: url match failed");
			}
		}

        if (calculateHash(ce->getResponseHeader()) != hdr.responseHeadersHash)
        {
            return CacheResult("CacheEntry::open: header hash failed");
        }

        if (calculateHash(ce->getResponseBody()) != hdr.responseBodyHash)
        {
            return CacheResult("CacheEntry::open: body hash failed");
        }

		// We should not do following check after may be a year, as we are no longer storing non 200 into the cache
		// This case will cover a scenario where we have a Non 200 cdn response cached (Example 404) & we will continue to fail as the 404 were cached earlier
		// Now when we see a Non 200 we do not cache it in update routine, 
		// but if it was cached earlier, we will not load it & it will become stale & cleaned up with decay for the old 404 cached data
		// It will fail at HTTP layer and we will again retry the CDN, if it is again 404 that is fine and will not be cached, but if it is a 200 then it will correctly get cached
		if (hdr.responseCode != 200)
		{
			return CacheResult("CacheEntry::open: Non 200 Responses are not opened");
		}

        boost::filesystem::last_write_time(filepath, time(0)); // update cache entry so it looks like it was accessed just now           ??? - Max
        return CacheResult(ce, filesize);
    }
    catch (const std::runtime_error& e)
    {
        return CacheResult(e.what());
    }
}

CacheResult CacheResult::update(const char* assetUrl, const char* cdnUrl, const uint32_t responseCode, const Data& headers, const Data& body)
{
	// We do not want to Cache Non 200 responses
	// Scenario: CDN is down for that asset
	// During update we will fail with a 404 but we will still cache it
	// When the CDN is up, we will have the entry in cache with a 404 response & we will return bad response
	// Just do not cache Bad Response, it is OK to retry and then fail at HTTP layer
	if (responseCode != 200)
		return CacheResult("CacheEntry::update: Non 200 Responses are not cached");

	boost::mutex::scoped_lock lock(assetCacheMutex);


	Header cacheHeader = {
		gMagic(),                                                               // magic
		RBX_CACHE_FILE_VERSION,                                                 // version
		std::min(strlen(cdnUrl), static_cast<size_t>(RBX_CACHE_URL_MAX_LENGTH)),// urlBytes
		{'\0'},                                                                 // url
		responseCode,                                                           // responseCode
		headers.size(),                                                         // responseHeadersSize
		calculateHash(headers),                                                 // responseHeadersHash
		body.size(),                                                            // responseBodySize
		calculateHash(body),                                                    // responseBodyHash
		0                                                                       // reserved
	};
	memcpy(const_cast<uint8_t*>(cacheHeader.url), cdnUrl, cacheHeader.urlBytes);

	boost::filesystem::path filepath = cacheFilePath(assetUrl ? assetUrl : cdnUrl);
    size_t filesize = sizeof(Header) + headers.size() + body.size();

    void* filedata = new char[filesize];
    CacheEntry* ce = new(filedata)CacheEntry(cacheHeader, headers, body);
    writeWholeFile(filepath, filedata, filesize);

    return CacheResult( boost::shared_ptr<CacheEntry>( ce, ceDeleter ), filesize );
}

CacheEntry::CacheEntry(const Header& cacheHeader, const Data& responseHeaders, const Data& responseBody) :
    cacheHeader(cacheHeader)
{
    memcpy(data, responseHeaders.data(), responseHeaders.size());
    memcpy(data + responseHeaders.size(), responseBody.data(), responseBody.size());
}

const Data CacheEntry::getResponseHeader() const
{
    Data result(data, cacheHeader.responseHeadersSize);
    return result;
}

const Data CacheEntry::getResponseBody() const
{
    Data result(data + cacheHeader.responseHeadersSize, cacheHeader.responseBodySize);
    return result;
}
