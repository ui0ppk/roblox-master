#pragma once
#include "FastLog.h"
#include "Util/AsyncHttpQueue.h"
#include "rbx/make_shared.h"

namespace RBX
{
template<typename CachedContent, bool Log=false>
class AsyncHttpCache
	:public AsyncHttpQueue
{
public:
	AsyncHttpCache(Instance* owner, boost::function<bool(const std::string&, std::string*)> getLocalFile, int threadCount, int cacheSize)
		:AsyncHttpQueue(owner, getLocalFile, threadCount)
		,contentCache(cacheSize)
	{}
	shared_ptr<const Reflection::ValueArray> getRequestedUrls()
	{
		shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
		{
			{
				boost::mutex::scoped_lock lock(contentCacheMutex);
				for (typename ContentCache::List_Iter iter = contentCache.begin(); iter != contentCache.end(); ++iter)
				{
					ContentId id(iter->first);
					if (id.isHttp())
						result->push_back(id.toString());
				}
			}

			{
				boost::recursive_mutex::scoped_lock lock(requestSync);
				std::list< FailedUrl >::const_iterator end = failedUrls.end();
				for (std::list< FailedUrl >::const_iterator iter = failedUrls.begin(); iter!=end; ++iter)
				{
					result->push_back(iter->url);
				}
			}
		}
		return result;
	}

	void setCacheSize(int count)
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		contentCache.resize(count);
	}

	bool findCacheItem(const std::string& id, CachedContent* result)
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		return contentCache.fetch(id, result);
	}
    void removeCacheItem(const std::string& id)
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		contentCache.remove(id);
	}
	void invalidateCacheItemOrFailure(const std::string& id)
	{
		{
			boost::mutex::scoped_lock lock(contentCacheMutex);
			contentCache.remove(id);
		}
		{
			boost::recursive_mutex::scoped_lock lock(requestSync);
			std::list<FailedUrl>::iterator found = failedUrls.end();
			for (std::list<FailedUrl>::iterator itr = failedUrls.begin();
				itr != failedUrls.end() && found == failedUrls.end(); ++itr)
			{
				if (itr->url == id)
				{
					found = itr;
				}
			}
			if (found != failedUrls.end())
				failedUrls.erase(found);
		}
	}
	void insertCacheItem(const std::string& id, const CachedContent& result)
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		contentCache.insert(id, result);
	}
	void renameCacheItem(const std::string& id, const std::string& newId)
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		CachedContent content;
		if (contentCache.fetch(id, &content))
		{
			//"rename" the entry.
			contentCache.remove(id);
			contentCache.insert(newId, content);
		}
	}
    
    void clearCache()
    {
		{
			boost::mutex::scoped_lock lock(contentCacheMutex);
			contentCache.clear();
		}
		{
			boost::recursive_mutex::scoped_lock lock(requestSync);
			failedUrls.clear();
		}
    }

	void printContentNames()
	{
		boost::mutex::scoped_lock lock(contentCacheMutex);
		contentCache.printContentNames();
	}

protected:
	/*override*/ void registerContent(const std::string& url, shared_ptr<const std::string> response, shared_ptr<const std::string> filename)
	{
		if(Log)	FASTLOGS(FLog::HttpQueue, "URL(%s)", url.c_str());
		boost::mutex::scoped_lock lock(contentCacheMutex);
		contentCache.insert(url, CachedContent(response, filename));
	}
	typedef SizeEnforcedLRUCache<std::string, CachedContent> ContentCache;

	boost::mutex contentCacheMutex;			//synchronizes the contentCache
	ContentCache contentCache;
};
}