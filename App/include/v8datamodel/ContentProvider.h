#pragma once

#include "stdio.h"
#include "util/name.h"
#include <string>
#include <istream>
#include <memory>
#include <vector>
#include "rbx/boost.hpp"
#include "rbx/rbxTime.h"
#include "Util/contentid.h"
#include "Util/HeartbeatInstance.h"
#include "Util/ThreadPool.h"
#include "Util/AsyncHttpCache.h"
#include "Util/LRUCache.h"
#include "V8Tree/Service.h"
#include "rbx/Log.h"
#include "util/ProtectedString.h"
#include "boost/filesystem.hpp"
#include "boost/optional.hpp"

namespace RBX {

	class AssetFetchMediator;
	class Instance;
	class Http;

	extern const char* const sContentProvider;
	class ContentProvider 
		: public DescribedNonCreatable<ContentProvider, Instance, sContentProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL>
		, public Service
		, public HeartbeatInstance
	{
	private:
		typedef DescribedNonCreatable<ContentProvider, Instance, sContentProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;
	public:
		static Log *appLog;
		static RBX::mutex *appLogLock;

		static float PRIORITY_DEFAULT;

		static float PRIORITY_MFC;
		static float PRIORITY_SCRIPT;
        static float PRIORITY_MESH;
        static float PRIORITY_SOLIDMODEL;
		static float PRIORITY_INSERT;
		static float PRIORITY_CHARACTER;
		static float PRIORITY_ANIMATION;
		static float PRIORITY_TEXTURE;
		static float PRIORITY_DECAL;
		static float PRIORITY_SOUND;
		static float PRIORITY_GUI;
		static float PRIORITY_SKY;
	private:
		boost::shared_ptr<boost::thread> legacyContentCleanupThread;
		boost::shared_ptr<boost::thread> contentCleanupThread;

        static boost::filesystem::path assetFolderPath;
        static boost::filesystem::path platformAssetFolderPath;
        static std::string assetFolderString;
        static std::string platformAssetFolderString;
        static bool assetFolderAlreadyInit;

		struct CachedContent
		{
			shared_ptr<const std::string> data;	
			shared_ptr<const std::string> filename;
			CachedContent()
			{}
			CachedContent(shared_ptr<const std::string> filename)
				:filename(filename)
			{}
			CachedContent(shared_ptr<const std::string> data, shared_ptr<const std::string> filename)
				:data(data)
				,filename(filename)
			{}
		};
		boost::shared_ptr<AsyncHttpCache<CachedContent> > contentCache;

		class PreloadAsyncRequest 
		{
		public:
			int outstanding;
			int failed;

			PreloadAsyncRequest()
				:outstanding(0)
				,failed(0)
			{}

			PreloadAsyncRequest(int requestCount)
				:outstanding(requestCount)
				,failed(0)
			{}
		};
	public:
		ContentProvider();
		~ContentProvider();

		static ContentId registerContent(std::istream& stream);

		bool isUrlBad(RBX::ContentId id);

		void blockingLoadInstances(ContentId id, std::vector<shared_ptr<Instance> >& instances);

		bool isRequestQueueEmpty();
		
		const std::string& getBaseUrl() const;
		const std::string getApiBaseUrl() const;
		const std::string getUnsecureApiBaseUrl() const;
		static std::string getApiBaseUrl(const std::string& baseUrl);
		static std::string getUnsecureApiBaseUrl(const std::string& baseUrl);

		void setBaseUrl(std::string url);
		void setThreadPool(int count);
		void setCacheSize(int count);
		void preloadContentWithCallback(RBX::ContentId id, float priority, boost::function<void (AsyncHttpQueue::RequestResult)> callback, AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline, const std::string& expectedType = "");
		void preloadContent(RBX::ContentId id);

		void preloadContentBlockingList(shared_ptr<const Reflection::ValueArray> idList, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
		static boost::mutex preloadContentBlockingMutex;
		void preloadContentBlockingListHelper(AsyncHttpQueue::RequestResult results, boost::function<void()> resumeFunction, 
													boost::function<void(std::string)> errorFunction, PreloadAsyncRequest *pRequest, ContentId id);
		void preloadContentResultCallback(AsyncHttpQueue::RequestResult results, ContentId id);

        void clearContent();
		void invalidateCache(ContentId contentId);

		// returns true if the content is available.
		bool hasContent(const ContentId& id);
		
		// returns non-NULL if the content is available. If not available, the provider does an asynchronous request of the content for later
		shared_ptr<const std::string> requestContentString(const ContentId& id, float priority);

		// Async request
		void getContent(const RBX::ContentId& id, float priority, AsyncHttpQueue::RequestCallback callback, AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline, const std::string& expectedType = "");
		void loadContent(const RBX::ContentId& id, float priority, boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback, AsyncHttpQueue::ResultJob jobType=AsyncHttpQueue::AsyncInline);
		void loadContentString(const RBX::ContentId& id, float priority, boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback, AsyncHttpQueue::ResultJob jobType=AsyncHttpQueue::AsyncInline);

		// The following functions throw exceptions upon failure or return NULL
		shared_ptr<const std::string> getContentString(ContentId id);
		std::auto_ptr<std::istream> getContent(const ContentId& contentId, const std::string& expectedType = "");
		std::string getFile(ContentId contentId);
		static std::string getAssetFile(const char* filePath);

		//throws an exception
		static void verifyRequestedScriptSignature(const ProtectedString& source, const std::string& assetId, bool required);
		static void verifyScriptSignature(const ProtectedString& source, bool required);

		int getRequestQueueSize() const;
		shared_ptr<const Reflection::ValueArray> getFailedUrls();
		shared_ptr<const Reflection::ValueArray> getRequestQueueUrls();
		shared_ptr<const Reflection::ValueArray> getRequestedUrls();

		static void setAssetFolder(const char* path);
		static std::string assetFolder();
		static std::string platformAssetFolder();

		static bool isUrl(const std::string& s);
		static bool isHttpUrl(const std::string& s);

		static std::string findAsset(RBX::ContentId contentId); 

		static Reflection::PropDescriptor<ContentProvider, std::string> desc_baseUrl;

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			contentCache->resetStatsItem(newProvider);
			Super::onServiceProvider(oldProvider, newProvider);
			onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat
		}
		/*implement*/ virtual void onHeartbeat(const Heartbeat& event);

		void printContentNames() { contentCache->printContentNames(); }

		void setAssetFetchMediator(AssetFetchMediator* afm);


	private:
		// FullAsyncRequest: all requests (including disk requests) can be asynchronous. disk requests are returned as memory streams, exactly like http responses.
		typedef enum { NoHttpRequest, AsyncHttpRequest, SyncHttpRequest, FullAsyncRequest } RequestType;

		bool clearFinishFlag;
		AssetFetchMediator* afm;

		std::string baseUrl;

		bool isContentLoaded(ContentId id);
		bool blockingLoadContent(ContentId id, CachedContent* result, const std::string& expectedType = "");
		AsyncHttpQueue::RequestResult privateLoadContent(ContentId& id, RequestType httpRequestType, float priority, CachedContent* result, AsyncHttpQueue::RequestCallback* callback, AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline, const std::string& expectedType = "");

		static std::string findHashFile(ContentId contentId);
		static bool findLocalFile(const std::string& url, std::string* filename);

		bool registerFile(const ContentId& id, CachedContent* item);

        static bool isInSandbox(const boost::filesystem::path& path, const boost::filesystem::path& sandbox);
	};

	class AssetFetchMediator
	{
	public:
		virtual boost::optional<std::string> findCachedAssetOrEmpty(const ContentId& contentId, int universeId) = 0;
	};
}
