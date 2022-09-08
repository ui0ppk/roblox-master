#pragma once 
#include "Reflection/Reflection.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "util/DoubleEndedVector.h"

namespace RBX {
	class DataStore;
	class DataStoreJob;

	struct UnsignedIntegerCountAverage
	{
	public:
		UnsignedIntegerCountAverage()
		{
			count = 0;
			average = 0;
		};

		unsigned int count;
		unsigned int average;

		void incrementValueAverage(unsigned int value) 
		{
			if (count == 0) 
			{ 
				count = 1; 
				average = value; 
			} 
			else 
			{ 
				average -= (average / count); 
				average += (value / count); 
				count = count + 1; 
			}
		}
	};

	extern const char* const sDataStoreService;
	class DataStoreService
		:public DescribedCreatable<DataStoreService, Instance, sDataStoreService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		,public Service
	{
	public:
		struct HttpRequest
		{
			std::string key;
			std::string url;
			std::string postData;
			boost::function<void(std::string*, std::exception*)> handler;
			shared_ptr<DataStore> owner;
			void execute(DataStoreService* dataStoreService);
			bool isKeyThrottled(Time timestamp);

			enum RequestType { GET_ASYNC = 5, UPDATE_ASYNC = 6, SET_ASYNC = 7, INCREMENT_ASYNC = 8, GET_SORTED_ASYNC_PAGE = 9 };
			RequestType requestType;
			boost::posix_time::ptime requestStartTime;
		};

	private:
		typedef DescribedCreatable<DataStoreService, Instance, sDataStoreService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
		typedef std::pair<std::string,std::string> StringPair;
		typedef std::map<StringPair, shared_ptr<DataStore> > DataStores;

		DataStores dataStores, orderedDataStores;
		shared_ptr<Instance> getDataStoreInternal(std::string name, std::string scope, bool legacy, bool ordered);

		shared_ptr<DataStore> legacyDataStore;
		bool disableUrlEncoding;

		shared_ptr<DataStoreJob> dataStoreJob;
		bool backendProcessing;
		BudgetedThrottlingHelper throttleCounterGets, throttleCounterGetSorteds, throttleCounterSets, throttleCounterOrderedSets;
		std::list<HttpRequest> throttledGets, throttledGetSorteds, throttledSets, throttledOrderedSets; // New requests go to back, executes from the front

		bool queueOrExecuteRequest(HttpRequest& request, std::list<HttpRequest>& queue, BudgetedThrottlingHelper& helper);
		void executeThrottledRequests(std::list<HttpRequest>& queue, BudgetedThrottlingHelper& helper);

		int getPlayerNum();

		boost::mutex analyticsReportMutex;
		UnsignedIntegerCountAverage msReadSuccessAverageRequestTime; 
		UnsignedIntegerCountAverage msErrorAverageRequestTime; 
		UnsignedIntegerCountAverage msWriteAverageRequestTime; 
		UnsignedIntegerCountAverage msUpdateAverageRequestTime; 
		UnsignedIntegerCountAverage msBatchAverageRequestTime;
		unsigned int readSuccessCachedCount;
		boost::posix_time::ptime lastAnalyticsReportTime;

	public:
		DataStoreService();
		shared_ptr<Instance> getGlobalDataStore();
		shared_ptr<Instance> getDataStore(std::string name, std::string scope);
		shared_ptr<Instance> getOrderedDataStore(std::string name, std::string scope);

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		void refetchCachedKeys();
		void addThrottlingBudgets(float timeDeltaMinutes);
		void executeThrottledRequests();

		static bool queueOrExecuteGet(DataStore* source, HttpRequest& request);
		static bool queueOrExecuteGetSorted(DataStore* source, HttpRequest& request);
		static bool queueOrExecuteSet(DataStore* source, HttpRequest& request);		
		static bool queueOrExecuteOrderedSet(DataStore* source, HttpRequest& request);		

		bool isUrlEncodingDisabled() const { return disableUrlEncoding; }
		void setUrlEncodingDisabled(bool disabled);


		void reportCachedRequestGet();
		void reportAnalytics();
		void onRequestFinishReport(DataStoreService::HttpRequest* request, bool isError, std::string errorMessage);
	};
}