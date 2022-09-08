#pragma once
#include "V8Tree/Service.h"
#include "Util/AsyncHttpCache.h"

namespace RBX {
    extern const char* const sPages;
    
	class Pages :
    public DescribedNonCreatable<Pages, Instance, sPages, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
    protected:
        bool finished;
		shared_ptr<const Reflection::ValueArray> currentPage;
        
	public:
		Pages();
        
		virtual void fetchNextChunk(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction) {};
        
		shared_ptr<const Reflection::ValueArray> getCurrentPage();
		bool isFinished() const;
		void advanceToNextPageAsync(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
	};

    extern const char* const sStandardPages;
    
    class StandardPages :
        public DescribedNonCreatable<StandardPages, Pages, sStandardPages, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
        weak_ptr<DataModel> weakDM;
        std::string fieldName;
        std::string requestUrl;
        int pageNumber;
        
		void processFetchSuccess(std::string response, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
		void processFetchError(std::string error, boost::function<void(std::string)> errorFunction);
        void processFetch(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

    public:
        StandardPages(weak_ptr<DataModel> weakDM, const std::string& requestUrl, const std::string& fieldName);
        virtual void fetchNextChunk(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
    };

	extern const char* const sFriendPages;

	class FriendPages :
        public DescribedNonCreatable<FriendPages, Pages, sFriendPages, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
        weak_ptr<DataModel> weakDM;
        std::string fieldName;
        std::string requestUrl;
		shared_ptr<const Reflection::ValueArray> nextPage;
        int pageNumber;
		bool firstTime;
        
		void processFetchSuccess(std::string response, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
		void processFetchError(std::string error, boost::function<void(std::string)> errorFunction);
        void processFetch(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

    public:
        FriendPages(weak_ptr<DataModel> weakDM, const std::string& requestUrl);
        virtual void fetchNextChunk(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
    };

	extern const char *const sLuaWebService;

	#define LUA_WEB_SERVICE_STANDARD_PRIORITY 50

	// This service is used to make web queries. It is frequently used by
	// function calls originating in Lua, but is not restricted to use
	// by Lua
	class LuaWebService 
		: public DescribedNonCreatable<LuaWebService, Instance, sLuaWebService>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<LuaWebService, Instance, sLuaWebService> Super;

		struct CachedLuaWebServiceInfo
		{
			Reflection::Variant value;
			CachedLuaWebServiceInfo() {}
			CachedLuaWebServiceInfo(shared_ptr<const std::string> data, shared_ptr<const std::string> filename);
		};

		struct CachedRawLuaWebServiceInfo
		{
			std::string value;
			CachedRawLuaWebServiceInfo() {}
			CachedRawLuaWebServiceInfo(shared_ptr<const std::string> data, shared_ptr<const std::string> filename);
		};

		boost::shared_ptr<AsyncHttpCache<CachedLuaWebServiceInfo, true> > webCache;
		boost::shared_ptr<AsyncHttpCache<CachedRawLuaWebServiceInfo, true> > webRawCache;
		bool checkApiAccess;
		Time timeToRecheckApiAccess;
		boost::optional<bool > apiAccess;

		template<typename Result>
		bool checkCache(const std::string& url, boost::function<void(Result)> resumeFunction, boost::function<void(std::string)> errorFunction);

		template<typename Result>
		static bool TryDispatchRequest(AsyncHttpCache<LuaWebService::CachedLuaWebServiceInfo, true>* webCache, const std::string& url, 
									   boost::function<void(Result)> resumeFunction, boost::function<void(std::string)> errorFunction);

		template<typename Result>
		static bool TryRawDispatchRequest(AsyncHttpCache<LuaWebService::CachedRawLuaWebServiceInfo, true>* webCache, const std::string& url,
			boost::function<void(Result)> resumeFunction, boost::function<void(std::string)> errorFunction);

		template<typename Result>
		static void Callback(boost::weak_ptr<LuaWebService> weakLuaWebService, AsyncHttpQueue::RequestResult requestResult, std::string url,
						  boost::function<void(Result)> resumeFunction, boost::function<void(std::string)> errorFunction);

		static void RawCallback(boost::weak_ptr<LuaWebService> weakLuaWebService, AsyncHttpQueue::RequestResult requestResult, std::string url,
								boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
	public:
		LuaWebService();
		void asyncRequest(const std::string& url, float priority,
						  boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void asyncRequest(const std::string& url, float priority,
						  boost::function<void(shared_ptr<const Reflection::ValueMap>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void asyncRequest(const std::string& url, float priority,
						  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void asyncRequest(const std::string& url, float priority,
						  boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void asyncRequest(const std::string& url, float priority,
						  boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

		//Skips the caches
		void asyncRequestNoCache(const std::string& url, float priority, boost::function<void(shared_ptr<const Reflection::ValueMap>)> callback, AsyncHttpQueue::ResultJob resultJob);

		// will block until api access request has returned
		bool isApiAccessEnabled();
		void setCheckApiAccessBecauseInStudio();

		static bool parseWebJSONResponseHelper(std::string* response, std::exception* exception, 
			shared_ptr<const Reflection::ValueTable>& result, std::string& status);
	};
}

