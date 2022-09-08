#pragma once

#include <vector>

#include "V8Tree/Service.h"
#include "v8datamodel/ContentProvider.h"
#include "util/ContentProviderJob.h"
#include "util/ControlledLRUCache.h"
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

typedef boost::unordered_set<std::string> Set;

namespace RBX {

	extern const char* const sCacheableContentProvider;
	class CacheableContentProvider
		: public DescribedNonCreatable<CacheableContentProvider, Instance, sCacheableContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>
		, public Service
		, public HeartbeatInstance
	{
		typedef DescribedNonCreatable<CacheableContentProvider, Instance, sCacheableContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

	protected:
		class CachedItem
		{
		public:
			AsyncHttpQueue::RequestResult requestResult;
			shared_ptr<void> data;
		public:
			CachedItem(shared_ptr<void> data = shared_ptr<void>(), AsyncHttpQueue::RequestResult result = AsyncHttpQueue::Waiting)
				: data(data), requestResult(result) 
			{}
			~CachedItem()
			{
				data.reset();
			}
			friend class CacheableContentProvider;
		};

		shared_ptr<ContentProviderJob> contentJob;
        rbx::atomic<int> pendingRequests;
		bool immediateMode;

		boost::scoped_ptr< ConcurrentControlledLRUCache<std::string, boost::shared_ptr<CachedItem> > > lruCache;

		boost::mutex failedCacheMutex;
		Set failedCache;

	public:

		CacheableContentProvider(CacheSizeEnforceMethod enforceMethod, unsigned long size);
		virtual ~CacheableContentProvider();

		void setCacheSize(int size);
		void setImmediateMode();
		bool isRequestQueueEmpty() { return pendingRequests == 0; }

        bool clearContent();
        
		bool hasContent(const ContentId& id);
		boost::shared_ptr<void> requestContent(const ContentId& id, float priority, bool markUsed, AsyncHttpQueue::RequestResult& result);
        boost::shared_ptr<void> blockingRequestContent(const ContentId& id, bool markUsed);
		boost::shared_ptr<void> fetchContent(const ContentId& id);

		bool isAssetFailed(const ContentId& id);

		// HeartbeatInstance
		/*override*/ void onHeartbeat(const Heartbeat& event);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	protected:
		static void LoadContentCallbackHelper(boost::weak_ptr<CacheableContentProvider> cacheableContentProvider, AsyncHttpQueue::RequestResult result, std::istream* filestream, shared_ptr<const std::string> data, std::string id);
		void LoadContentCallback(AsyncHttpQueue::RequestResult result, std::istream* filestream, shared_ptr<const std::string> data, std::string id);

		static TaskScheduler::StepResult ProcessTaskHelper(boost::weak_ptr<CacheableContentProvider> weakCcp, const std::string& id, shared_ptr<const std::string> data);
		virtual TaskScheduler::StepResult ProcessTask(const std::string& id, shared_ptr<const std::string> data) = 0;

		static void ErrorTaskHelper(boost::weak_ptr<CacheableContentProvider> weakCcp, const std::string& id);
		virtual void ErrorTask(const std::string& id);

		bool isAssetContent(ContentId id);
		void markContentFailed(const std::string& id);
		AsyncHttpQueue::RequestResult getContentStatus(const std::string& id);

		virtual void updateContent(const std::string& id, boost::shared_ptr<CachedItem> cachedItem);
	};

}