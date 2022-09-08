#include "stdafx.h"
#include "v8datamodel/CacheableContentProvider.h"
#include "V8DataModel/DataModel.h"

namespace RBX {

	const char* const sCacheableContentProvider = "CacheableContentProvider";
	CacheableContentProvider::CacheableContentProvider(CacheSizeEnforceMethod enforceMethod, unsigned long size)
		:pendingRequests(0)
		,immediateMode(false)
	{
		setName(sCacheableContentProvider);
		lruCache.reset(new ConcurrentControlledLRUCache<std::string, boost::shared_ptr<CachedItem> >(size, 300, enforceMethod));
	}

	CacheableContentProvider::~CacheableContentProvider()
	{
	}

	void CacheableContentProvider::onHeartbeat(const Heartbeat& event)
	{
		lruCache->onHeartbeat();
	}

	void CacheableContentProvider::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) 
	{
		if(contentJob){
			contentJob->abort();
			TaskScheduler::singleton().remove(contentJob);
			contentJob.reset();
		}

		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat

		if (newProvider)
		{
			contentJob = shared_ptr<ContentProviderJob>(new ContentProviderJob(shared_from(DataModel::get(this)), getName().c_str(),
				boost::bind(&CacheableContentProvider::ProcessTaskHelper, weak_from(this), _1, _2),
				boost::bind(&CacheableContentProvider::ErrorTaskHelper, weak_from(this), _1)));
			TaskScheduler::singleton().add(contentJob);
			if(immediateMode){
				contentJob ->setExecutionMode(ContentProviderJob::ImmediateMode);
			}
		}
	}

	void CacheableContentProvider::setCacheSize(int size)
	{
		lruCache->resize(size);
	}

	void CacheableContentProvider::setImmediateMode()
	{
		if(contentJob){
			contentJob->setExecutionMode(ContentProviderJob::ImmediateMode);
		}
		immediateMode = true;
	}

	bool CacheableContentProvider::isAssetContent(ContentId id)
	{
		if (ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this))
		{
			id.convertToLegacyContent(contentProvider->getBaseUrl());
			return id.isAsset();
		}
		else
			return false;

	}
    
    bool CacheableContentProvider::clearContent()
    {
        return lruCache->evictAll();
    }

	bool CacheableContentProvider::hasContent(const ContentId& id)
	{
		if(id.isNull()){
			return false;
		}
		if(isAssetContent(id)){  
			if(ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this)){
				return contentProvider->hasContent(id);
			}
			return false;
		}
		if(getContentStatus(id.toString()) == AsyncHttpQueue::Succeeded){
			return true;
		}
		return false;
	}

	bool CacheableContentProvider::isAssetFailed(const ContentId& id)
	{
		ContentId localId = id;
		if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
		{
			localId.convertAssetId(contentProvider->getBaseUrl(), DataModel::get(this)->getUniverseId());
			localId.convertToLegacyContent(contentProvider->getBaseUrl());
		}
		boost::mutex::scoped_lock lock(failedCacheMutex);
		return failedCache.find(localId.toString()) != failedCache.end();
	}

	boost::shared_ptr<void> CacheableContentProvider::fetchContent(const ContentId& id)
	{
		if(id.isNull())
			return shared_ptr<void>();

		//if(isAssetContent(id))
		//	return shared_ptr<void>();

		{
			boost::mutex::scoped_lock lock(failedCacheMutex);
			if(failedCache.find(id.toString()) != failedCache.end()){
				return shared_ptr<void>();
			}
		}

		shared_ptr<CachedItem> item;
		if (lruCache->fetch(id.toString(), &item, true)){
			if(item->requestResult == AsyncHttpQueue::Succeeded){
				return item->data;
			}
		}
		return shared_ptr<void>();
	}

	boost::shared_ptr<void> CacheableContentProvider::requestContent(const ContentId& id, float priority, bool markUsed, AsyncHttpQueue::RequestResult& result)
	{
		if(id.isNull()){
			result = AsyncHttpQueue::Failed;
			return shared_ptr<void>();
		}

		{
			boost::mutex::scoped_lock lock(failedCacheMutex);

			if(failedCache.find(id.toString()) != failedCache.end()){
				result = AsyncHttpQueue::Failed;
				return shared_ptr<void>();
			}
		}

		shared_ptr<CachedItem> cachedItem;
		if (lruCache->fetch(id.toString(), &cachedItem, false))
		{
			result = cachedItem->requestResult;
			if(result == AsyncHttpQueue::Succeeded){
				if(markUsed){
					lruCache->markEvictable(id.toString());
				}
			}
			return cachedItem->data;
		}

		// mark img as waiting
		if(ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this)){
			cachedItem.reset(new CachedItem());
			cachedItem->requestResult = AsyncHttpQueue::Waiting;
			lruCache->insert(id.toString(), cachedItem);
			{
				++pendingRequests;
				//Make a request to content provider
				contentProvider->getContent(id, priority, 
					boost::bind(&CacheableContentProvider::LoadContentCallbackHelper, weak_from(this), _1, _2, _3, id.toString()), AsyncHttpQueue::AsyncInline);
			}
		}
		result = AsyncHttpQueue::Waiting;
		return shared_ptr<void>();
	}

    boost::shared_ptr<void> CacheableContentProvider::blockingRequestContent(const ContentId& id, bool markUsed)
    {
        AsyncHttpQueue::RequestResult reqResult = AsyncHttpQueue::Waiting;

        while(reqResult == AsyncHttpQueue::Waiting)
        {
            boost::shared_ptr<void> asset = requestContent(id, ContentProvider::PRIORITY_MFC, true, reqResult);

            if (reqResult == AsyncHttpQueue::Succeeded)
            {
                return asset;
            }
            else if (reqResult == AsyncHttpQueue::Failed)
            {
                return shared_ptr<void>();
            }

            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
        }

   		return shared_ptr<void>();
    }


	void CacheableContentProvider::LoadContentCallbackHelper(boost::weak_ptr<CacheableContentProvider> cacheableContentProvider, AsyncHttpQueue::RequestResult result,
										std::istream* filestream, shared_ptr<const std::string> data, std::string id)
	{
		if(boost::shared_ptr<CacheableContentProvider> ccp = cacheableContentProvider.lock())
			ccp->LoadContentCallback(result, filestream, data, id);
	}

	void CacheableContentProvider::LoadContentCallback(AsyncHttpQueue::RequestResult result, std::istream* filestream, shared_ptr<const std::string> data, std::string id)
	{
		if(contentJob)
			contentJob->addTask(id, result, filestream, data);
	}

	TaskScheduler::StepResult CacheableContentProvider::ProcessTaskHelper(boost::weak_ptr<CacheableContentProvider> weakCcp, const std::string& id, shared_ptr<const std::string> data)
	{
		shared_ptr<CacheableContentProvider> ccp = weakCcp.lock();
		if(!ccp) {
			return TaskScheduler::Done;
		}

		if(ccp->getContentStatus(id) != AsyncHttpQueue::Waiting) {
			//another callback took care of our loading, stop the requests
			--ccp->pendingRequests;
			return TaskScheduler::Stepped;
		}
		
		return ccp->ProcessTask(id, data);
	}

	void CacheableContentProvider::ErrorTaskHelper(boost::weak_ptr<CacheableContentProvider> weakCcp, const std::string& id)
	{
		if(boost::shared_ptr<CacheableContentProvider> ccp = weakCcp.lock())
			ccp->ErrorTask(id);
	}

	void CacheableContentProvider::ErrorTask(const std::string& id)
	{
		markContentFailed(id);
	}

	void CacheableContentProvider::markContentFailed(const std::string& id)
	{
		{
			boost::mutex::scoped_lock lock(failedCacheMutex);
			failedCache.insert(id);
		}
		lruCache->remove(id);

		--pendingRequests;
	}

	void CacheableContentProvider::updateContent(const std::string& id, boost::shared_ptr<CachedItem> cachedItem)
	{
		lruCache->insert(id, cachedItem);
	}

	AsyncHttpQueue::RequestResult CacheableContentProvider::getContentStatus(const std::string& id)
	{
		shared_ptr<CachedItem> cachedItem;
		if (lruCache->fetch(id, &cachedItem, false))
		{
			return cachedItem->requestResult;
		}
		return AsyncHttpQueue::Waiting;
	}
}
