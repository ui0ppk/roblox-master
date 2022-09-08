#pragma once 


#include "Reflection/Reflection.h"
#include "script/ThreadRef.h"
#include "V8Tree/Instance.h"
#include "rbx/RunningAverage.h"
#include "v8datamodel/DataStoreService.h"
#include "util/LuaWebService.h"
#include "signal.h"
#include <deque>

namespace RBX {

	extern const char* const sGlobalDataStore;

	class DataStore
		:public DescribedNonCreatable<DataStore, Instance, sGlobalDataStore, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		typedef DescribedNonCreatable<DataStore, Instance, sGlobalDataStore, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

		class CachedRecord
		{
		private:
			Reflection::Variant variant;
			std::string serialized;
			Time accessTimeStamp;
		public:
			CachedRecord(){};

			const Reflection::Variant& getVariant(bool touch = true);
			const std::string& getSerialized() { return serialized; }
			const Time& getTime() { return accessTimeStamp; }

			void update(const Reflection::Variant& variant, const std::string& serialized);
		};

		struct EventSlot
		{
			Lua::WeakFunctionRef callback;
			EventSlot(Lua::WeakFunctionRef);
			void fire(Reflection::Variant value);
		};

		bool isLegacy;

		typedef std::map<std::string, CachedRecord> CachedKeys;
		CachedKeys cachedKeys;
		typedef std::map<std::string, shared_ptr<rbx::signal<void(Reflection::Variant)> > > OnUpdateKeys;
		OnUpdateKeys onUpdateKeys;

		typedef std::map<std::string,Time> KeyTimestamps;
		KeyTimestamps lastSetByKey;

		enum RefetchState
		{
			RefetchOnUpdateKeys,
			RefetchCachedKeys,
			RefetchDone
		};

		RefetchState refetchState;
		std::string nextKeyToRefetch;

		bool backendProcessing;  

		std::string constructPostDataForKey(const std::string& key, unsigned index = 0);
		std::string constructGetUrl();
		std::string constructSetUrl(const std::string& key, unsigned valueLength);
		std::string constructSetIfUrl(const std::string& key, unsigned valueLength, unsigned expectedValueLength);
		std::string constructIncrementUrl(const std::string& key, int delta);
        
		void processSet(std::string key, std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

		void processFetchSingleKey(std::string* response, std::exception* exception, std::string key, bool expectSubKey, boost::function<void()> callback, boost::function<void(std::string)> errorFunction);
		void lockAcquiredProcessFetchSingleKey(shared_ptr<std::string> response, shared_ptr<std::exception> exception, std::string key, bool expectSubKey, boost::function<void()> callback, boost::function<void(std::string)> errorFunction);

		bool updateCachedKey(const std::string& key, const Reflection::Variant& value);
		
		void createFetchNewKeyRequest(const std::string& key, boost::function<void()> callback, boost::function<void(std::string)> errorFunction, DataStoreService::HttpRequest& request);

		// Has to be const reference to Lua::WeakFunctionRef so boost::bind won't copy it to pass by value
		// Lua::WeakFunctionRef needs access to Lua state to copy, so can't be done safely in http threadpool
		void processSetIf(std::string key, shared_ptr<Lua::WeakFunctionRef> transform, std::string* response, std::exception* exception, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void lockAcquiredProcessSetIf(std::string key, shared_ptr<Lua::WeakFunctionRef> transform, shared_ptr<std::string> response, shared_ptr<std::exception> exception,  boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		
		void runTransformFunction(std::string key, const shared_ptr<Lua::WeakFunctionRef> transform, 
			boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, 
			boost::function<void(std::string)> errorFunction);

		void processFetchCachedKeys(std::string* response, std::exception* exception);
		void lockAcquiredProcessFetchCachedKeys(shared_ptr<std::string> response, shared_ptr<std::exception> exception);

		bool checkAccess(const std::string& key, boost::function<void(std::string)>* errorFunction);
		bool checkStudioApiAccess(boost::function<void(std::string)> errorFunction);

		static std::string serializeVariant(const Reflection::Variant& variant, bool* hasNonJsonType);
		static bool deserializeVariant(const std::string& webValue, Reflection::Variant& result);

		void sendBatchGet(std::stringstream& keysList);
		void accumulateKeyToFetch(const std::string& key, std::stringstream& keysList, int& counter);

	protected:
		std::string serviceUrl;
		std::string name, scope;
		std::string scopeUrlEncodedIfNeeded, nameUrlEncodedIfNeeded;
		virtual bool checkValueIsAllowed(const Reflection::Variant&) { return true; };
		virtual const char* getDataStoreTypeString() { return "standard"; }
		virtual bool queueOrExecuteSet(DataStoreService::HttpRequest& request);

		std::string urlEncodeIfNeeded(const std::string& input);
	
	public:	
		DataStore(const std::string& name, const std::string& scope, bool legacy);

        static const char* urlApiPath() { return "persistence"; }

		void getAsync(std::string key, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void updateAsync(std::string key, Lua::WeakFunctionRef transformFunc, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void setAsync(std::string key, Reflection::Variant value, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
		void incrementAsync(std::string key, int delta, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction);

		rbx::signals::connection onUpdate(std::string key, Lua::WeakFunctionRef callback);
		
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		void refetchCachedKeys(int* budget);
		void resetRefetch();

		bool isKeyThrottled(const std::string& key, Time timestamp);
		void setKeySetTimestamp(const std::string& key, Time timestamp);
        DataStoreService* getParentDataStoreService();
    private:
        void runResumeFunction(std::string key, boost::function<void(Reflection::Variant)> resumeFunction);
	};

	extern const char* const sOrderedDataStore;
	class OrderedDataStore : 
		public DescribedNonCreatable<OrderedDataStore, DataStore, sOrderedDataStore, Reflection::ClassDescriptor::RUNTIME_LOCAL> 
	{
		typedef DescribedNonCreatable<DataStore, DataStore, sOrderedDataStore, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

		std::string constructGetSortedUrl(bool isAscending, int pagesize, const double* minValue, const double* maxValue);

	protected:
		virtual bool checkValueIsAllowed(const Reflection::Variant& v);
		virtual const char* getDataStoreTypeString() { return "sorted"; }
		virtual bool queueOrExecuteSet(DataStoreService::HttpRequest& request);

	public:
		OrderedDataStore(const std::string& name, const std::string& scope);

		void getSortedAsync(bool isAscending, int pagesize, Reflection::Variant minValue, Reflection::Variant maxValue,
			boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction);
	};

	extern const char* const sDataStorePages;

	class DataStorePages :
		public DescribedNonCreatable<DataStorePages, Pages, sDataStorePages, Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		weak_ptr<OrderedDataStore> ds;
		std::string requestUrl;
		std::string exclusiveStartKey;

		void processFetch(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
		void lockAcquiredProcessFetch(shared_ptr<std::string> response, shared_ptr<std::exception> exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

	public:
		DataStorePages(weak_ptr<OrderedDataStore> ds, const std::string& requestUrl);

		void fetchNextChunk(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
	};
}