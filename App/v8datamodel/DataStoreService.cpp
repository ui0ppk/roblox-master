#include "stdafx.h"

#include "V8DataModel/DataStoreService.h"
#include "V8DataModel/DataModel.h"
#include "v8datamodel/DataStore.h"
#include "Network/Players.h"
#include "Util/RobloxGoogleAnalytics.h"

#include "util/Analytics.h"
#include "Util/RobloxGoogleAnalytics.h"

LOGGROUP(DataStore);

DYNAMIC_FASTINTVARIABLE(DataStoreJobFrequencyInSeconds, 1);
DYNAMIC_FASTINTVARIABLE(DataStoreFetchFrequenceInSeconds, 30);

DYNAMIC_FASTINTVARIABLE(DataStoreFixedRequestLimit, 60);
DYNAMIC_FASTINTVARIABLE(DataStorePerPlayerRequestLimit, 10);
DYNAMIC_FASTINTVARIABLE(DataStoreInitialBudget, 100);

DYNAMIC_FASTINTVARIABLE(DataStoreOrderedSetFixedRequestLimit, 30);
DYNAMIC_FASTINTVARIABLE(DataStoreOrderedSetPerPlayerRequestLimit, 5);
DYNAMIC_FASTINTVARIABLE(DataStoreOrderedSetInitialBudget, 50);

DYNAMIC_FASTINTVARIABLE(DataStoreRefetchFixedRequestLimit, 30);
DYNAMIC_FASTINTVARIABLE(DataStoreRefetchPerPlayerRequestLimit, 5);

DYNAMIC_FASTINTVARIABLE(DataStoreSortedFixedRequestLimit, 5);
DYNAMIC_FASTINTVARIABLE(DataStoreSortedPerPlayerRequestLimit, 2);
DYNAMIC_FASTINTVARIABLE(DataStoreSortedInitialBudget, 10);

DYNAMIC_FASTINTVARIABLE(DataStoreMaxBudgetMultiplier, 100);

DYNAMIC_FASTINTVARIABLE(DataStoreMaxThrottledQueue, 30);

DYNAMIC_FASTINT(DataStoreKeyLengthLimit);

DYNAMIC_FASTFLAGVARIABLE(GetGlobalDataStorePcallFix, false);

DYNAMIC_FASTFLAGVARIABLE(UseNewDataStoreLogging, true)

DYNAMIC_FASTINTVARIABLE(DataStoreAnalyticsReportEveryNSeconds, 60);

DYNAMIC_FASTFLAGVARIABLE(UseNewDataStoreRequestSetTimestampBehaviour, true)

LOGVARIABLE(DataStoreBudget, 0);



namespace {
	static inline void sendDataStoreStats()
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DataStorePersistence");
	}

	static inline void sendDataStoreOldNamingSchemeStats()
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DataStoreOldNamingScheme");
	}
}
	
namespace RBX {
	const char* const sDataStoreService = "DataStoreService";

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<DataStoreService, shared_ptr<Instance>(void)> getGlobalStoreFunction(&DataStoreService::getGlobalDataStore, "GetGlobalDataStore", Security::None);
	static Reflection::BoundFuncDesc<DataStoreService, shared_ptr<Instance>(std::string,std::string)> getDataStoreFunction(&DataStoreService::getDataStore, "GetDataStore", "name", "scope", "global", Security::None);
	static Reflection::BoundFuncDesc<DataStoreService, shared_ptr<Instance>(std::string,std::string)> getOrderedDataStore(&DataStoreService::getOrderedDataStore, "GetOrderedDataStore", "name", "scope", "global", Security::None);

	static Reflection::PropDescriptor<DataStoreService, bool> prop_disableUrlEncoding
		("LegacyNamingScheme", category_Behavior, &DataStoreService::isUrlEncodingDisabled, &DataStoreService::setUrlEncodingDisabled, Reflection::PropertyDescriptor::STANDARD, Security::LocalUser);
    REFLECTION_END();

	
	class DataStoreJob : public DataModelJob
	{
		shared_ptr<DataStoreService> dataStoreService;

		int lastJobFrequency;
		double desiredHz;

		int secondsSinceLastFetch;

	public:
		DataStoreJob(DataStoreService* owner)
			: DataModelJob("DataStoreJob", DataModelJob::Write, false, shared_from(DataModel::get(owner)), Time::Interval(0.01))
			,dataStoreService(shared_from(owner))
		{
			updateHz();
			secondsSinceLastFetch = 0;

			cyclicExecutive = true;
		}

		void updateHz()
		{
			desiredHz = 1.0f / DFInt::DataStoreJobFrequencyInSeconds;
			lastJobFrequency = DFInt::DataStoreJobFrequencyInSeconds;
		}

		/*override*/ Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, desiredHz);
		}
		/*override*/ Job::Error error(const Stats& stats)
		{
			return computeStandardErrorCyclicExecutiveSleeping(stats, desiredHz);
		}
		/*override*/ TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
		{
			if (DFInt::DataStoreJobFrequencyInSeconds != lastJobFrequency)
				updateHz();

			dataStoreService->addThrottlingBudgets(DFInt::DataStoreJobFrequencyInSeconds / 60.0f);
			dataStoreService->executeThrottledRequests();

			secondsSinceLastFetch += DFInt::DataStoreJobFrequencyInSeconds;
			if (secondsSinceLastFetch >= DFInt::DataStoreFetchFrequenceInSeconds)
			{
				dataStoreService->refetchCachedKeys();
				secondsSinceLastFetch -= DFInt::DataStoreFetchFrequenceInSeconds;
			}

			if (DFFlag::UseNewDataStoreLogging)
			{
				dataStoreService->reportAnalytics();
			}
			return TaskScheduler::Stepped;
		}
	};

	DataStoreService::DataStoreService() : 
		disableUrlEncoding(false),
		msReadSuccessAverageRequestTime(), 
		msErrorAverageRequestTime(), 
		msWriteAverageRequestTime(),
		msUpdateAverageRequestTime(), 
		msBatchAverageRequestTime(),
		readSuccessCachedCount(0)
	{ 
		setName(sDataStoreService);
		throttleCounterGets.addBudget(DFInt::DataStoreInitialBudget, DFInt::DataStoreInitialBudget);
		throttleCounterGetSorteds.addBudget(DFInt::DataStoreInitialBudget, DFInt::DataStoreInitialBudget);
		throttleCounterSets.addBudget(DFInt::DataStoreSortedInitialBudget, DFInt::DataStoreSortedInitialBudget);
		throttleCounterOrderedSets.addBudget(DFInt::DataStoreOrderedSetInitialBudget, DFInt::DataStoreOrderedSetInitialBudget);

		boost::mutex::scoped_lock lock(analyticsReportMutex);
		lastAnalyticsReportTime = boost::posix_time::microsec_clock::local_time();
	}

	void DataStoreService::onRequestFinishReport(DataStoreService::HttpRequest* request, bool isError, std::string errorMessage)
	{
		boost::mutex::scoped_lock lock(analyticsReportMutex);
		unsigned int requestDurationMilliseconds = (unsigned int)((boost::posix_time::microsec_clock::local_time() - request->requestStartTime).total_microseconds() / 1000);
		
		if (isError)
		{
			msErrorAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DataStoreError", errorMessage.c_str());
		}
		else
		{
			switch (request->requestType)
			{
				case DataStoreService::HttpRequest::RequestType::GET_ASYNC: 
				{
					msReadSuccessAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
				} break;
				case DataStoreService::HttpRequest::RequestType::UPDATE_ASYNC: 
				{
					msUpdateAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
				} break;
				case DataStoreService::HttpRequest::RequestType::SET_ASYNC: 
				{
					msWriteAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
				} break;
				case DataStoreService::HttpRequest::RequestType::INCREMENT_ASYNC: 
				{
					msWriteAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
				} break;
				case DataStoreService::HttpRequest::RequestType::GET_SORTED_ASYNC_PAGE: 
				{
					msBatchAverageRequestTime.incrementValueAverage(requestDurationMilliseconds);
				} break;
				default: break;
			}
		}
	}

	void reportDataStoreStat(unsigned int &count, unsigned int &average, std::string countReportName, std::string averageReportName)
	{
		if (count > 0) 
		{
			Analytics::EphemeralCounter::reportCounter(countReportName, count);
			Analytics::EphemeralCounter::reportCounter(averageReportName, average);
			count = 0; 
			average = 0; 
		}
	}

	void DataStoreService::reportAnalytics()
	{
		boost::mutex::scoped_lock lock(analyticsReportMutex);

		boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time();
		if ((currentTime - lastAnalyticsReportTime).total_milliseconds() / 1000 < DFInt::DataStoreAnalyticsReportEveryNSeconds)
		{
			return;
		}
		lastAnalyticsReportTime = currentTime;

		reportDataStoreStat(msReadSuccessAverageRequestTime.count, msReadSuccessAverageRequestTime.average, "DataStoreReadRequestCount", "DataStoreReadRequestAverageTime");
		reportDataStoreStat(msErrorAverageRequestTime.count, msErrorAverageRequestTime.average, "DataStoreErrorRequestCount", "DataStoreErrorRequestAverageTime");
		reportDataStoreStat(msWriteAverageRequestTime.count, msWriteAverageRequestTime.average, "DataStoreWriteRequestCount", "DataStoreWriteRequestAverageTime");
		reportDataStoreStat(msUpdateAverageRequestTime.count, msUpdateAverageRequestTime.average, "DataStoreUpdateRequestCount", "DataStoreUpdateRequestAverageTime");
		reportDataStoreStat(msBatchAverageRequestTime.count, msBatchAverageRequestTime.average, "DataStoreBatchRequestCount", "DataStoreBatchRequestAverageTime");
	
		if (readSuccessCachedCount > 0)
		{
			Analytics::EphemeralCounter::reportCounter("DataStoreReadCachedCount", readSuccessCachedCount);
			readSuccessCachedCount = 0;
		}
	}

	void DataStoreService::reportCachedRequestGet()
	{
		boost::mutex::scoped_lock lock(analyticsReportMutex);
		++readSuccessCachedCount;
	}

	shared_ptr<Instance> DataStoreService::getGlobalDataStore()
	{
		return getDataStoreInternal("", "u", true, false);
	}

	void DataStoreService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if (oldProvider)
		{
			FASTLOG1(FLog::DataStore, "Closing down DataStoreService. Backend processing: ", backendProcessing);
  
			visitChildren(boost::bind(&Instance::unlockParent, _1));
			if (backendProcessing) 
			{
				TaskScheduler::singleton().removeBlocking(dataStoreJob);
				dataStoreJob.reset();
			}

			backendProcessing = false;
		}

		Super::onServiceProvider(oldProvider, newProvider);

		if(newProvider)
		{
			backendProcessing = Network::Players::backendProcessing(this);
			FASTLOG1(FLog::DataStore, "Backend processing: %u", backendProcessing);

			if (backendProcessing) {
				dataStoreJob.reset(new DataStoreJob(this));
				TaskScheduler::singleton().add(dataStoreJob);
			}
		}
	}

	shared_ptr<Instance> DataStoreService::getDataStoreInternal(std::string name, std::string scope, bool legacy, bool ordered)
	{
		DataModel* dm = DataModel::get(this);

		if (dm->getPlaceID() == 0) {
			if (DFFlag::GetGlobalDataStorePcallFix)
			{
				throw std::runtime_error("Place has to be opened with Edit button to access DataStores");
			}
			StandardOut::singleton()->print(MESSAGE_ERROR, "Place has to be opened with Edit button to access DataStores");
			return shared_ptr<Instance>();
		}

		if (!backendProcessing)
			throw std::runtime_error("DataStore can't be accessed from client");

		RBXASSERT(!(legacy && ordered));

		if(legacy)
		{
			if(!legacyDataStore) {
				FASTLOG(FLog::DataStore, "Creating legacy data store");
				legacyDataStore = Creatable<Instance>::create<DataStore>(name, scope, true);
				legacyDataStore->setParent(this);
				legacyDataStore->lockParent();
			}
			return legacyDataStore;
		}
		else if(ordered)
		{
			StringPair key(name, scope);
			DataStores::iterator it = orderedDataStores.find(key);
			if(it == orderedDataStores.end())
			{
				FASTLOGS(FLog::DataStore, "Creating data store, name: %s", name);
				shared_ptr<DataStore> ds = Creatable<Instance>::create<OrderedDataStore>(name, scope);
				ds->setName(name);
				ds->setParent(this);
				ds->lockParent();
				orderedDataStores[key] = ds;
				return ds;
			}
			return it->second;
		}
		else
		{
			StringPair key(name, scope);
			DataStores::iterator it = dataStores.find(key);
			if(it == dataStores.end())
			{
				FASTLOGS(FLog::DataStore, "Creating data store, name: %s", name);
				shared_ptr<DataStore> ds = Creatable<Instance>::create<DataStore>(name, scope, false);
				ds->setName(name);
				ds->setParent(this);
				ds->lockParent();
				dataStores[key] = ds;
				return ds;
			}
			return it->second;
		}
	}

	void checkNameAndScope(const std::string& name, const std::string& scope)
	{
		if(scope.length() == 0)
			throw std::runtime_error("DataStore scope can't be empty string");

		if(scope.length() > (unsigned)DFInt::DataStoreKeyLengthLimit)
			throw std::runtime_error("DataStore scope is too long");

		if(name.length() == 0)
			throw std::runtime_error("DataStore name can't be empty string");

		if(name.length() > (unsigned)DFInt::DataStoreKeyLengthLimit)
			throw std::runtime_error("DataStore name is too long");
	}


	shared_ptr<Instance> DataStoreService::getDataStore(std::string name, std::string scope)
	{
		checkNameAndScope(name, scope);
		return getDataStoreInternal(name, scope, false, false);
	}

	shared_ptr<Instance> DataStoreService::getOrderedDataStore(std::string name, std::string scope)
	{
		checkNameAndScope(name, scope);
		return getDataStoreInternal(name, scope, false, true);
	}

	int DataStoreService::getPlayerNum()
	{
		const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(this);
		if (serviceProvider == NULL) {
			RBXASSERT(false);
			return 0;
		}

		Network::Players* players = serviceProvider->find<Network::Players>();
		return players ? players->getNumPlayers() : 0;
	}

	void DataStoreService::addThrottlingBudgets(float timeDeltaMinutes)
	{
		int numPlayers = getPlayerNum();

		{
			int sortedPerPlayerPerMinuteBudget = DFInt::DataStoreSortedPerPlayerRequestLimit * numPlayers + DFInt::DataStoreSortedFixedRequestLimit;
			float sortedAddedBudget = timeDeltaMinutes*sortedPerPlayerPerMinuteBudget;
			throttleCounterGetSorteds.addBudget(sortedAddedBudget, DFInt::DataStoreMaxBudgetMultiplier * sortedPerPlayerPerMinuteBudget);
		}

		{
			int orderedSetsPerPlayerPerMinuteBudget = DFInt::DataStoreOrderedSetPerPlayerRequestLimit * numPlayers + DFInt::DataStoreOrderedSetFixedRequestLimit;
			float orderedSetsAddedBudget = timeDeltaMinutes*orderedSetsPerPlayerPerMinuteBudget;
			throttleCounterOrderedSets.addBudget(orderedSetsAddedBudget, DFInt::DataStoreMaxBudgetMultiplier * orderedSetsPerPlayerPerMinuteBudget);
		}

		{
			int perPlayerPerMinuteBudget = DFInt::DataStorePerPlayerRequestLimit * numPlayers + DFInt::DataStoreFixedRequestLimit;
			float addedBudget = timeDeltaMinutes*perPlayerPerMinuteBudget;

			throttleCounterGets.addBudget(addedBudget, DFInt::DataStoreMaxBudgetMultiplier * perPlayerPerMinuteBudget);
			throttleCounterSets.addBudget(addedBudget, DFInt::DataStoreMaxBudgetMultiplier * perPlayerPerMinuteBudget);
			FASTLOG4F(FLog::DataStoreBudget, "Adding budget %f, gets budget: %f, sets budget: %f, sorted budget: %f", 
				addedBudget, throttleCounterGets.getBudget(), throttleCounterSets.getBudget(), throttleCounterGetSorteds.getBudget());
		}
	}

	static void dataStorePostSuccess(std::string response, boost::function<void(std::string*, std::exception*)> handler)
	{
		handler(&response, 0);
	}

	static void dataStorePostError(std::string error, boost::function<void(std::string*, std::exception*)> handler)
	{
        std::runtime_error e(error.c_str());
		handler(0, &e);
	}

	void handlerExecuteWithAnalytics(DataStoreService::HttpRequest request, std::string* response, std::exception* error)
	{
		DataStoreService *dataStoreService = request.owner->getParentDataStoreService();
		if (dataStoreService != NULL)
		{
			dataStoreService->onRequestFinishReport(&request, error != NULL, error == NULL ? "" : error->what());
		}
		request.handler(response, error);
	}

	void DataStoreService::HttpRequest::execute(DataStoreService* dataStoreService)
	{
		if (DFFlag::UseNewDataStoreRequestSetTimestampBehaviour)
		{
			if (requestType == RequestType::SET_ASYNC || requestType == RequestType::INCREMENT_ASYNC || requestType == RequestType::UPDATE_ASYNC)
			{
				owner->setKeySetTimestamp(key, Time::nowFast());
			}
		}
		else
		{
			if(owner)
				owner->setKeySetTimestamp(key, Time::nowFast());
		}

		RBX::Http http(url);
		http.additionalHeaders["Cache-Control"] = "no-cache";
		http.doNotUseCachedResponse = true;


		// todo: use HttpRbxApiService to throttle datastores instead of doing in this class
		if (DFFlag::UseNewDataStoreLogging)
		{
			http.post(postData.size() == 0 ? " " : postData, Http::kContentTypeUrlEncoded, false, 
				boost::bind(&handlerExecuteWithAnalytics, *this, _1, _2)
			);
		}
		else
		{
			http.post(postData.size() == 0 ? " " : postData, Http::kContentTypeUrlEncoded, false, handler);
		}
	}

	bool DataStoreService::HttpRequest::isKeyThrottled(Time timestamp)
	{
		if (owner == NULL)
			return false;

		return owner->isKeyThrottled(key, timestamp);
	}

	bool DataStoreService::queueOrExecuteRequest(HttpRequest& request, std::list<HttpRequest>& queue, BudgetedThrottlingHelper& helper)
	{
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendDataStoreStats, flag);
		}

		FASTLOG1(FLog::DataStore, "Queue suze: %u", queue.size());
		if (request.isKeyThrottled(Time::nowFast()) || !helper.checkAndReduceBudget())
		{
			FASTLOG1F(FLog::DataStore, "Throttling, budget: %f", helper.getBudget());
			if (queue.size() < (unsigned)DFInt::DataStoreMaxThrottledQueue)
			{
				queue.push_back(request);
				return true;
			}
			else
				return false;
		}

		FASTLOG1F(FLog::DataStore, "Not throttling, budget after: %f, queue size: %f", helper.getBudget());
		request.execute(this);
		return true;
	}

	bool DataStoreService::queueOrExecuteGet(DataStore* source, HttpRequest& request)
	{
		DataStoreService* service = fastDynamicCast<DataStoreService>(source->getParent());
		RBXASSERT(service);
		if (!service)
			return false;

		return service->queueOrExecuteRequest(request, service->throttledGets, service->throttleCounterGets);
	}

	bool DataStoreService::queueOrExecuteGetSorted(DataStore* source, HttpRequest& request)
	{
		DataStoreService* service = fastDynamicCast<DataStoreService>(source->getParent());
		RBXASSERT(service);
		if (!service)
			return false;

		return service->queueOrExecuteRequest(request, service->throttledGetSorteds, service->throttleCounterGetSorteds);
	}

	bool DataStoreService::queueOrExecuteSet(DataStore* source, HttpRequest& request)
	{
		DataStoreService* service = fastDynamicCast<DataStoreService>(source->getParent());
		RBXASSERT(service);
		if (!service)
			return false;

		return service->queueOrExecuteRequest(request, service->throttledSets, service->throttleCounterSets);
	}

	bool DataStoreService::queueOrExecuteOrderedSet(DataStore* source, HttpRequest& request)
	{
		DataStoreService* service = fastDynamicCast<DataStoreService>(source->getParent());
		RBXASSERT(service);
		if (!service)
			return false;

		return service->queueOrExecuteRequest(request, service->throttledOrderedSets, service->throttleCounterOrderedSets);
	}

	void DataStoreService::executeThrottledRequests(std::list<HttpRequest>& queue, BudgetedThrottlingHelper& helper)
	{
		if (queue.size() > 0)
			FASTLOG2F(FLog::DataStore, "Executing throttled requests, size: %f, budget: %f", (float)queue.size(), helper.getBudget());

		Time now = Time::nowFast();

		for(std::list<HttpRequest>::iterator it = queue.begin(); it != queue.end();)
		{
			if(it->isKeyThrottled(now))
			{
				++it;
				continue;
			}

			if(!helper.checkAndReduceBudget())
				break;

			it->execute(this);
			it = queue.erase(it);
		}
	}

	void DataStoreService::executeThrottledRequests()
	{
		executeThrottledRequests(throttledGets, throttleCounterGets);
		executeThrottledRequests(throttledGetSorteds, throttleCounterGetSorteds);
		executeThrottledRequests(throttledSets, throttleCounterSets);
		executeThrottledRequests(throttledOrderedSets, throttleCounterOrderedSets);
	}

	void DataStoreService::setUrlEncodingDisabled(bool disabled)
	{
		if(disabled != disableUrlEncoding)
		{
			disableUrlEncoding = disabled;
			if(disableUrlEncoding)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(&sendDataStoreOldNamingSchemeStats, flag);
			}
		}
	}

	void DataStoreService::refetchCachedKeys()
	{
		int budget = (DFInt::DataStoreRefetchFixedRequestLimit +  DFInt::DataStoreRefetchPerPlayerRequestLimit * getPlayerNum()) * DFInt::DataStoreFetchFrequenceInSeconds / 60; 

		FASTLOG1(FLog::DataStore, "Initial budget for refetch: %i", budget);

		if(legacyDataStore)
			legacyDataStore->refetchCachedKeys(&budget);

		for (DataStores::iterator it = dataStores.begin(); it != dataStores.end(); ++it) 
		{
			if(budget < 0)
				break;

			it->second->refetchCachedKeys(&budget);
		}

		for (DataStores::iterator it = orderedDataStores.begin(); it != orderedDataStores.end(); ++it) 
		{
			if(budget < 0)
				break;

			it->second->refetchCachedKeys(&budget);
		}

		if(budget >= 0)
		{
			FASTLOG1(FLog::DataStore, "Refetch budget is %i, so reset refetch on all DataStores", budget);

			if(legacyDataStore)
				legacyDataStore->resetRefetch();

			for (DataStores::iterator it = dataStores.begin(); it != dataStores.end(); ++it) 
			{
				it->second->resetRefetch();
			}

			for (DataStores::iterator it = orderedDataStores.begin(); it != orderedDataStores.end(); ++it) 
			{
				it->second->resetRefetch();
			}
		}
	}
}
