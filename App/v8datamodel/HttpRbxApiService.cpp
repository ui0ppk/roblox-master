#include "stdafx.h"

#include "V8DataModel/HttpRbxApiService.h"
#include "V8DataModel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/HttpRbxApiJob.h"
#include "Network/Players.h"
#include "V8Xml/WebParser.h"
#include "Util/RobloxGoogleAnalytics.h"

#define HTTP_POST_COMPRESSION_LIMIT 256

DYNAMIC_FASTINTVARIABLE(PercentApiRequestsRecordGoogleAnalytics, 1)

DYNAMIC_FASTINTVARIABLE(HttpRbxApiClientPerMinuteRequestLimit, 300)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxBudgetMultiplier, 1)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiRequestsPerMinuteServerLimit, 300)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiRequestsPerMinutePerPlayerInServerLimit, 100)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxThrottledQueueSize, 50)

DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxRetryBudgetPerMinute, 500)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxRetryCount, 10)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxRetryQueueSize, 500)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiMaxSyncRetries, 3)
DYNAMIC_FASTINTVARIABLE(HttpRbxApiSyncRetryWaitTimeMSec, 500)

DYNAMIC_FASTFLAG(UseR15Character)

LOGVARIABLE(HttpRbxApiBudget, 0);

namespace {
	static inline void sendApiServiceDidThrottle()
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "GameHasBeenAPIThrottled");
	}
	static inline void sendApiServiceDidQueue()
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "GameHasBeenAPIQueued");
	}
}

namespace RBX {
	std::string HttpRbxApiService::StaticApiBaseUrl;
	const char* const sHttpRbxApiService = "HttpRbxApiService";

    REFLECTION_BEGIN();
	// CoreScript Exposed Functions
	static Reflection::BoundYieldFuncDesc<HttpRbxApiService, std::string(std::string, bool, ThrottlingPriority)> apiGetAsyncFunction(&HttpRbxApiService::getAsyncLua, "GetAsync", "apiUrlPath", "useHttps", true, "priority", PRIORITY_DEFAULT, Security::RobloxScript);
	static Reflection::BoundYieldFuncDesc<HttpRbxApiService, std::string(std::string, std::string, bool, ThrottlingPriority, HttpService::HttpContentType)> apiPostAsyncFunction(&HttpRbxApiService::postAsyncLua, "PostAsync", "apiUrlPath", "data", "useHttps", true, "priority", PRIORITY_DEFAULT, "content_type", HttpService::APPLICATION_JSON, Security::RobloxScript);
    REFLECTION_END();

	namespace Reflection {
		template<>
		EnumDesc<ThrottlingPriority>::EnumDesc()
			:EnumDescriptor("ThrottlingPriority")
		{
			addPair(PRIORITY_EXTREME,			"Extreme");
			addPair(PRIORITY_SERVER_ELEVATED,	"ElevatedOnServer");
			addPair(PRIORITY_DEFAULT,			"Default");
		}

		template<>
		ThrottlingPriority& Variant::convert<ThrottlingPriority>(void)
		{
			return genericConvert<ThrottlingPriority>();
		}

	} // namespace Reflection

	template<>
	bool StringConverter<ThrottlingPriority>::convertToValue(const std::string& text, ThrottlingPriority& value)
	{
		return Reflection::EnumDesc<ThrottlingPriority>::singleton().convertToValue(text.c_str(), value);
	}


	HttpRbxApiService::HttpRbxApiService() :
		apiBaseUrl(""),
		serverPresent(false),
		clientPresent(false),
		isPlaySolo(false),
		totalNumOfApiCalls(0)
	{
		recordInGoogleAnalytics = rand() % 100 < DFInt::PercentApiRequestsRecordGoogleAnalytics;

		setName(sHttpRbxApiService);
	}

	void HttpRbxApiService::setStaticApiBaseUrl(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor)
	{
		if (*pPropertyDescriptor == RBX::ContentProvider::desc_baseUrl)
		{		
			contentProviderPropertyChangedConnection.disconnect();

			apiBaseUrl = ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();

			if (!apiBaseUrl.empty())
			{
				const std::string httpsString = "https";
				apiBaseUrl.replace(apiBaseUrl.find(httpsString),httpsString.length(),"");

				HttpRbxApiService::StaticApiBaseUrl = apiBaseUrl;

				if (DFFlag::UseR15Character)
				{
					if(DataModel* dataModel = DataModel::get(this)) 
					{
						dataModel->setCanRequestUniverseInfo(true);
					}
				}

			}
		}

	}

	void HttpRbxApiService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if (oldProvider)
		{
			if (getRecordInGoogleAnalytics())
			{
				if (isPlaySolo)
				{
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "TotalHttpApiCallsInPlaySolo", totalNumOfApiCalls);
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "AvgHttpApiCallsPerSecInPlaySolo",((double)totalNumOfApiCalls)/instanceAliveTimer.delta().seconds());
				}
				else if(serverPresent)
				{
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "TotalHttpApiCallsInServer",totalNumOfApiCalls);
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "AvgHttpApiCallsPerSecInServer",((double)totalNumOfApiCalls)/instanceAliveTimer.delta().seconds());
				}
				else if(clientPresent)
				{
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "TotalHttpApiCallsInClient",totalNumOfApiCalls);
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "AvgHttpApiCallsPerSecInClient",((double)totalNumOfApiCalls)/instanceAliveTimer.delta().seconds());
				}
			}

			disconnectEventConnections();

			serverPresent = false;
			clientPresent = false;
			isPlaySolo = false;

			TaskScheduler::singleton().removeBlocking(httpRbxApiJob);
			httpRbxApiJob.reset();
		}

		Super::onServiceProvider(oldProvider,newProvider);

		if (newProvider)
		{
			apiBaseUrl = ServiceProvider::create<ContentProvider>(newProvider)->getApiBaseUrl();
			instanceAliveTimer.reset();

			if (!apiBaseUrl.empty())
			{
				const std::string httpsString = "https";
				apiBaseUrl.replace(apiBaseUrl.find(httpsString),httpsString.length(),"");

				HttpRbxApiService::StaticApiBaseUrl = apiBaseUrl;
			}
			else
			{
				contentProviderPropertyChangedConnection = ServiceProvider::find<ContentProvider>(newProvider)->propertyChangedSignal.connect(boost::bind(&HttpRbxApiService::setStaticApiBaseUrl, this, _1));
			}

			// check to see if we are a client, server, or in play solo.
			// this may not return a mode immediately so set up connections
			// for verification later
			checkForClientAndServer(newProvider);
			if (!serverPresent && !clientPresent)
			{
				if (Network::Players* players = ServiceProvider::find<Network::Players>(newProvider))
				{
					playersChangedConnection = players->propertyChangedSignal.connect( boost::bind(&HttpRbxApiService::playersPropertyChanged, this, _1) );
				}

				serviceAddedConnection = newProvider->serviceAddedSignal.connect( boost::bind(&HttpRbxApiService::newServiceAdded, this, _1) );
			}

			httpRbxApiJob.reset(new HttpRbxApiJob(this));
			TaskScheduler::singleton().add(httpRbxApiJob);
		}
	}

	void HttpRbxApiService::disconnectEventConnections()
	{
		serviceAddedConnection.disconnect();
		playersChangedConnection.disconnect();
	}

	void HttpRbxApiService::playersPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc)
	{
		if (!serverPresent && !clientPresent && desc == &Network::Players::propLocalPlayer)
		{
			// this should only be hit and is play solo because there is no client or server, but we have local player
			disconnectEventConnections();

			isPlaySolo = true;

			defaultServerThrottle.addBudget(DFInt::HttpRbxApiRequestsPerMinuteServerLimit, DFInt::HttpRbxApiRequestsPerMinuteServerLimit);
			elevatedServerThrottle.addBudget(DFInt::HttpRbxApiRequestsPerMinuteServerLimit, DFInt::HttpRbxApiRequestsPerMinuteServerLimit);
			clientThrottle.addBudget(DFInt::HttpRbxApiClientPerMinuteRequestLimit, DFInt::HttpRbxApiClientPerMinuteRequestLimit);
			retryBudget.addBudget(DFInt::HttpRbxApiMaxRetryBudgetPerMinute, DFInt::HttpRbxApiMaxRetryBudgetPerMinute);
		}
	}

	void HttpRbxApiService::newServiceAdded(shared_ptr<Instance> newService)
	{
		checkForClientAndServer(this);
	}

	void HttpRbxApiService::checkForClientAndServer(Instance* context)
	{
		if (!serverPresent && Workspace::serverIsPresent(context))
		{
			disconnectEventConnections();

			defaultServerThrottle.addBudget(DFInt::HttpRbxApiRequestsPerMinuteServerLimit, DFInt::HttpRbxApiRequestsPerMinuteServerLimit);
			elevatedServerThrottle.addBudget(DFInt::HttpRbxApiRequestsPerMinuteServerLimit, DFInt::HttpRbxApiRequestsPerMinuteServerLimit);
			retryBudget.addBudget(DFInt::HttpRbxApiMaxRetryBudgetPerMinute, DFInt::HttpRbxApiMaxRetryBudgetPerMinute);

			serverPresent = true;
		}

		if (!clientPresent && Workspace::clientIsPresent(context))
		{
			disconnectEventConnections();

			clientThrottle.addBudget(DFInt::HttpRbxApiClientPerMinuteRequestLimit, DFInt::HttpRbxApiClientPerMinuteRequestLimit);
			retryBudget.addBudget(DFInt::HttpRbxApiMaxRetryBudgetPerMinute, DFInt::HttpRbxApiMaxRetryBudgetPerMinute);

			clientPresent = true;
		}
	}

	void HttpRbxApiService::setErrorForAsync(const std::string& errorString, boost::function<void(std::string)> errorFunction)
	{
		if (errorFunction && !errorFunction.empty())
		{
			errorFunction(errorString);
		}
	}

	bool HttpRbxApiService::isAPIHttpRequest(const Http& httpRequest)
	{
		if (StaticApiBaseUrl.empty())
		{
			return false;
		}

		return (httpRequest.url.find(StaticApiBaseUrl) != std::string::npos);
	}

	std::string HttpRbxApiService::getApiUrlPath(const Http& httpRequest)
	{
		const std::string urlString = httpRequest.url;

		size_t pos = urlString.find(StaticApiBaseUrl);
		if (pos != std::string::npos)
		{
			return urlString.substr(pos + StaticApiBaseUrl.length(), urlString.length());
		}

		return "";
	}

	static bool shouldRetryFromStatusCode(const long statusCode, HttpRbxApiService::HttpApiRequest& request)
	{
		// retry on 503 error, this means the web service is throttling
		// web is switching to 429 for this, so also check 429
		if (statusCode == 503 ||
			statusCode == 429)
		{
			return true;
		}

		return false;
	}

	static bool shouldRetryRequest(std::exception* exception, HttpRbxApiService::HttpApiRequest& request, bool recordInGoogleAnalytics)
	{
		bool shouldRetry = false;
		if (exception)
		{		
			long statusCode = -1;
			if (RBX::http_status_error* httpError = dynamic_cast<RBX::http_status_error*>(exception))
			{
				statusCode = httpError->statusCode;
			}

			if (request.retryCount == 0 && recordInGoogleAnalytics)
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "RetryRequestStarted");
			}

			shouldRetry = shouldRetryFromStatusCode(statusCode, request) && (request.retryCount < DFInt::HttpRbxApiMaxRetryCount);
			request.retryCount++;
		}

		return shouldRetry;
	}

	bool HttpRbxApiService::retrySyncRequest(Http& http, std::string& syncResponse)
	{
		int retryCount = 0;
		while (retryCount < DFInt::HttpRbxApiMaxSyncRetries)
		{
			retryCount++;
			boost::this_thread::sleep(boost::posix_time::milliseconds(DFInt::HttpRbxApiSyncRetryWaitTimeMSec));

			try
			{
				http.get(syncResponse,false);
				return true;
			}
			catch (std::exception&)
			{
				// just continue retrying, don't throw
			}
		}

		return false;
	}

	void httpHelperRetryLockAcquired(
		shared_ptr<HttpRbxApiService> apiService, 
		HttpRbxApiService::HttpApiRequest request,  
		boost::function<void(std::string)> errorFunction)
	{
		if (!apiService->addToRetryQueue(request))
		{
			errorFunction("Request failed, but could not retry due to too many current retry requests.");
			return;
		}
	}

	void httpHelperExecuteLockAcquired(
		shared_ptr<HttpRbxApiService> apiService, 
		shared_ptr<std::string> response, 
		shared_ptr<std::exception> exception, 
		HttpRbxApiService::HttpApiRequest request, 
		boost::function<void(std::string)> resumeFunction, 
		boost::function<void(std::string)> errorFunction)
	{
		if(response)
		{
			if (apiService->getRecordInGoogleAnalytics() && request.retryCount > 0)
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "RetryRequestSucceeded");
			}

			resumeFunction(*response);
		}
		else
		{
			if (apiService->getRecordInGoogleAnalytics() && request.retryCount > 0)
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", "RetryRequestFailed");
			}

			errorFunction(exception->what());
		}
	}

	void HttpRbxApiService::httpHelper(weak_ptr<HttpRbxApiService> weakApiService, std::string* response, std::exception* exception, HttpApiRequest request, ThrottlingPriority throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{ 
		if (shared_ptr<HttpRbxApiService> apiService = weakApiService.lock())
		{
			if (shouldRetryRequest(exception, request, apiService->getRecordInGoogleAnalytics())) // we got an error that requires retry, try to do it again
			{
				DataModel::processHttpRequestResponseOnLock(
					RBX::DataModel::get(apiService.get()),
					response,
					exception,
					boost::bind(&httpHelperRetryLockAcquired,
						apiService,
						request,
						errorFunction));
			}
			else
			{
				DataModel::processHttpRequestResponseOnLock(
					RBX::DataModel::get(apiService.get()),
					response,
					exception,
					boost::bind(&httpHelperExecuteLockAcquired,
						apiService,
						_1, /* shared_ptr<std::string> response, */
						_2, /* shared_ptr<std::exception> exception, */
						request,
						resumeFunction,
						errorFunction));
			}
		}
	}

	///////////////////////////////////////////////////////////////
	// HTTP Post Functions
	///////////////////////////////////////////////////////////////
	void HttpRbxApiService::postAsync(Http& httpRequest, std::string& data, ThrottlingPriority throttlePriority, HttpService::HttpContentType content,
		boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{	
		if (!isAPIHttpRequest(httpRequest))
		{
			// don't pass non api calls here! Use regular http functions instead unless you want to get throttled
			RBXASSERT(false);
			if (getRecordInGoogleAnalytics())
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", ("postAsyncNotApiRequest for " + httpRequest.url).c_str());
			}
		}

		postAsyncInternal(httpRequest, data, content, data.size() > HTTP_POST_COMPRESSION_LIMIT, throttlePriority, resumeFunction, errorFunction);
	}

    void HttpRbxApiService::checkAndUpdatePostUrl(std::string& fullUrl, const std::string& urlPath) const
    {
		if (urlPath.find(StaticApiBaseUrl) != std::string::npos)
		{
			// don't pass calls here with api proxy domain already in them!
			// just pass the path, ex: adimpression/validate-request
			RBXASSERT(false);

			if (getRecordInGoogleAnalytics())
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", std::string("postAsyncUrlPathContainsDomain for " + urlPath).c_str());
			}

			fullUrl = urlPath;
		}
    }

	void HttpRbxApiService::postAsync(std::string urlPath, std::string data, bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, 
		boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;
        checkAndUpdatePostUrl(fullUrl, urlPath);

		Http http(fullUrl);
		postAsyncInternal(http, data, content, data.size() > HTTP_POST_COMPRESSION_LIMIT, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::postAsyncWithAdditionalHeaders(std::string urlPath, std::string data,bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, RBX::HttpAux::AdditionalHeaders additionalHeaders,
		boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;
        checkAndUpdatePostUrl(fullUrl, urlPath);

		Http http(fullUrl);
		if (additionalHeaders.size() > 0)
		{
			http.applyAdditionalHeaders(additionalHeaders);
		}
		postAsyncInternal(http, data, content, data.size() > HTTP_POST_COMPRESSION_LIMIT, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::postAsyncLua(std::string urlPath, std::string data, bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, 
		boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;
		checkAndUpdatePostUrl(fullUrl, urlPath);

		Http http(fullUrl);
		robloxScriptModifiedCheck(RBX::apiPostAsyncFunction.security);
		postAsyncInternal(http, data, content, data.size() > HTTP_POST_COMPRESSION_LIMIT, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::postAsyncInternal(Http& httpRequest, std::string& data, const HttpService::HttpContentType& contentType, const bool shouldCompress, const ThrottlingPriority& throttlePriority,
		boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string contentTypeName;

		switch(contentType)
		{
		case HttpService::APPLICATION_JSON: contentTypeName = Http::kContentTypeApplicationJson; break;
		case HttpService::APPLICATION_XML: contentTypeName = Http::kContentTypeApplicationXml; break;
		case HttpService::APPLICATION_URLENCODED: contentTypeName = Http::kContentTypeUrlEncoded; break;
		case HttpService::TEXT_PLAIN: contentTypeName = Http::kContentTypeTextPlain; break;
		case HttpService::TEXT_XML: contentTypeName = Http::kContentTypeTextXml; break;
		default:
			errorFunction("Unsupported content type");
			return; 
		}

		if (data.empty())
		{
			data = " ";
		}

		HttpApiRequest apiRequest;
		apiRequest.isPost = true;
		apiRequest.postData = data;
		apiRequest.httpContentType = contentTypeName;
		apiRequest.resumeFunction = resumeFunction;
		apiRequest.errorFunction = errorFunction;
		apiRequest.throttlingPriority = throttlePriority;
		apiRequest.setHttp(httpRequest);

		executeApiRequest(apiRequest, throttlePriority, errorFunction);
	}

	///////////////////////////////////////////////////////////////
	// HTTP Get Functions
	///////////////////////////////////////////////////////////////
	
	void HttpRbxApiService::get(const std::string& urlPath, bool useHttps, ThrottlingPriority throttlePriority, std::string& response)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;

		Http http(fullUrl);

		get(http, useHttps, throttlePriority, response);
	}

	void HttpRbxApiService::get(Http& httpRequest, bool useHttps, ThrottlingPriority throttlePriority, std::string& response)
	{
		HttpApiRequest apiRequest;
		apiRequest.async = false;
		apiRequest.throttlingPriority = throttlePriority;
		apiRequest.setHttp(httpRequest);

		executeApiRequest(apiRequest, throttlePriority, NULL);

		response = apiRequest.syncResponse;
	}

	void HttpRbxApiService::getAsync(Http& httpRequest, ThrottlingPriority throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!isAPIHttpRequest(httpRequest))
		{
			// don't pass non api calls here! Use regular http functions instead unless you want to get throttled
			RBXASSERT(false);

			if (getRecordInGoogleAnalytics())
			{
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "HttpRbxApiService", std::string("getAsyncNonApiCall for " + httpRequest.url).c_str());
			}
		}

		getAsyncInternal(httpRequest, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::getAsync(std::string urlPath, bool useHttps, ThrottlingPriority throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;

        Http http(fullUrl);
		getAsyncInternal(http, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::getAsyncLua(std::string urlPath, bool useHttps, ThrottlingPriority throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fullUrl = "https" + apiBaseUrl + urlPath;


		Http http(fullUrl);
		robloxScriptModifiedCheck(RBX::apiGetAsyncFunction.security);
		getAsyncInternal(http, throttlePriority, resumeFunction, errorFunction);
	}

	void HttpRbxApiService::getAsyncInternal(Http& httpRequest, const ThrottlingPriority& throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		HttpApiRequest apiRequest;
		apiRequest.throttlingPriority = throttlePriority;
		apiRequest.resumeFunction = resumeFunction;
		apiRequest.errorFunction = errorFunction;
		apiRequest.setHttp(httpRequest);

		executeApiRequest(apiRequest, throttlePriority, errorFunction);
	}


	/////////////////////////////////////////////////////////////////////////////////////////////
	// Throttling Logic
	//////////////////////////////////////////////////////////////

	int HttpRbxApiService::getPlayerNum()
	{
		const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(this);
		if (serviceProvider == NULL) 
		{
			RBXASSERT(false);
			return 0;
		}

		Network::Players* players = serviceProvider->find<Network::Players>();
		return players ? players->getNumPlayers() : 0;
	}

	void HttpRbxApiService::addThrottlingBudgets(float timeDeltaMinutes)
	{
		if (Network::Players::serverIsPresent(this) || isPlaySolo)
		{
			const int serverMaxBudget = (DFInt::HttpRbxApiRequestsPerMinutePerPlayerInServerLimit * getPlayerNum()) + DFInt::HttpRbxApiRequestsPerMinuteServerLimit;
			const float addedServerBudget = timeDeltaMinutes*serverMaxBudget;

			defaultServerThrottle.addBudget(addedServerBudget, serverMaxBudget);
			elevatedServerThrottle.addBudget(addedServerBudget, serverMaxBudget);

			FASTLOG3F(FLog::HttpRbxApiBudget, "Adding budget %f, default budget is at: %f, elevated budget is at: %f", 
				addedServerBudget, defaultServerThrottle.getBudget(), elevatedServerThrottle.getBudget());
		}

		if (Network::Players::clientIsPresent(this) || isPlaySolo)
		{
			const float addedClientBudget = timeDeltaMinutes * DFInt::HttpRbxApiClientPerMinuteRequestLimit;
			clientThrottle.addBudget(addedClientBudget, DFInt::HttpRbxApiMaxBudgetMultiplier *  DFInt::HttpRbxApiClientPerMinuteRequestLimit);

			FASTLOG2F(FLog::HttpRbxApiBudget, "Adding budget %f, client budget is at: %f", 
				addedClientBudget, clientThrottle.getBudget());
		}

		const int maxRetryBudget = DFInt::HttpRbxApiMaxRetryBudgetPerMinute;
		const float addedRetryBudget =  DFInt::HttpRbxApiMaxRetryBudgetPerMinute * timeDeltaMinutes;
		retryBudget.addBudget(addedRetryBudget, maxRetryBudget);
	}

	void HttpRbxApiService::HttpApiRequest::execute(HttpRbxApiService* apiService)
	{
		if (apiService->getRecordInGoogleAnalytics())
		{
			apiService->addToApiCallCount();
		}

		if (isPost)
		{
			http.post(postData, httpContentType,  (postData.size() > HTTP_POST_COMPRESSION_LIMIT), 
				boost::bind(&HttpRbxApiService::httpHelper, weak_from(apiService), _1, _2, *this, throttlingPriority, resumeFunction, errorFunction), false);
		}
		else // we are a get call
		{
			if (!async) // synchronous call to http (better be for good reason!)
			{
				try
				{
					http.get(syncResponse,false);
				}
				catch (std::exception& e)
				{
					// if synchronous call fails, we have to just retry here
					// still throw if we never get a proper response
					if (shouldRetryRequest(&e, *this, apiService->getRecordInGoogleAnalytics()))
					{
						if (!retrySyncRequest(http, syncResponse))
						{
                            throw std::runtime_error( format("Could not get a valid response for %s after retrying synchronous get.", http.url.c_str()).c_str() );
						}
					}
					else
					{
						throw e;
					}
				}
			}
			else
			{
				http.get(boost::bind(&HttpRbxApiService::httpHelper, weak_from(apiService), _1, _2, *this, throttlingPriority,  resumeFunction, errorFunction), false);
			}
		}
	}

	bool HttpRbxApiService::addToRetryQueue(HttpApiRequest apiRequest)
	{
		if (retryQueue.size() < (unsigned) DFInt::HttpRbxApiMaxRetryQueueSize)
		{
			retryQueue.push_back(apiRequest);
			return true;
		}

		return false;
	}

	void HttpRbxApiService::executeRetryRequests()
	{
		if (retryQueue.size() > 0)
		{
			FASTLOG2F(FLog::HttpRbxApiBudget, "Executing retry requests, size: %f, budget: %f", (float)retryQueue.size(), retryBudget.getBudget());
		}

		while(retryQueue.size() > 0 && retryBudget.checkAndReduceBudget())
		{
			HttpApiRequest request;
			retryQueue.pop_front(&request);

			executeApiRequest(request, request.throttlingPriority, request.errorFunction);
		}
	}

	void HttpRbxApiService::executeThrottledRequests(DoubleEndedVector<HttpApiRequest>& queue, BudgetedThrottlingHelper& helper)
	{
		if (queue.size() > 0)
		{
			FASTLOG2F(FLog::HttpRbxApiBudget, "Executing throttled requests, size: %f, budget: %f", (float)queue.size(), helper.getBudget());
		}

		while(queue.size() > 0 && helper.checkAndReduceBudget())
		{
			HttpApiRequest request;
			queue.pop_front(&request);
			request.execute(this);
		}
	}

	void HttpRbxApiService::executeThrottledRequests()
	{
		if (Network::Players::serverIsPresent(this) || isPlaySolo)
		{
			executeThrottledRequests(throttledDefaultServerRequests, defaultServerThrottle);
			executeThrottledRequests(throttledElevatedServerRequests, elevatedServerThrottle);
		}

		if (Network::Players::clientIsPresent(this) || isPlaySolo)
		{
			executeThrottledRequests(throttledClientRequests, clientThrottle);
		}
	}

	bool HttpRbxApiService::tryThrottleRequest(const HttpApiRequest& apiRequest, BudgetedThrottlingHelper& budgetThrottler, DoubleEndedVector<HttpApiRequest>& throttledRequestQueue,
		boost::function<void(std::string)> errorFunction)
	{
		if (!budgetThrottler.checkAndReduceBudget())
		{
			FASTLOG1F(FLog::HttpRbxApiBudget, "Throttling, budget: %f", budgetThrottler.getBudget());

			if (getRecordInGoogleAnalytics())
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(&sendApiServiceDidQueue, flag);
			}

			if (throttledRequestQueue.size() < (unsigned)DFInt::HttpRbxApiMaxThrottledQueueSize)
			{
				throttledRequestQueue.push_back(apiRequest);
			}
			else
			{
				if (getRecordInGoogleAnalytics())
				{
					static boost::once_flag flag = BOOST_ONCE_INIT;
					boost::call_once(&sendApiServiceDidThrottle, flag);
				}

				std::string throttleError = RBX::format("number of API requests/minute exceeded limit for HTTP API throttle. Please don't issue more than %i API requests/minute with server scripts and no more than %i API requests/minute with local scripts.",
					DFInt::HttpRbxApiRequestsPerMinuteServerLimit + (DFInt::HttpRbxApiRequestsPerMinutePerPlayerInServerLimit * getPlayerNum()), DFInt::HttpRbxApiClientPerMinuteRequestLimit);
				setErrorForAsync(throttleError, errorFunction);
			}

			return true;
		}

		return false;
	}

	bool HttpRbxApiService::executeApiRequest(HttpApiRequest& apiRequest, const ThrottlingPriority& throttlePriority, boost::function<void(std::string)> errorFunction)
	{
		if (apiRequest.getHttp().url.empty())
		{
			setErrorForAsync("Empty URL", errorFunction);
			return false;
		}

		if (apiRequest.getHttp().url.find(apiBaseUrl) == std::string::npos)
		{
			setErrorForAsync("Non-API Proxy BaseURL used. HttpRbxApiService is only for API Proxy calls.", errorFunction);
			return false;
		}

		switch (throttlePriority)
		{
		case PRIORITY_DEFAULT:
			{
				if (isPlaySolo &&
					tryThrottleRequest(apiRequest, defaultServerThrottle, throttledDefaultServerRequests, errorFunction))
				{
					return false;
				}
				else if ( (Network::Players::serverIsPresent(this) ) &&
					tryThrottleRequest(apiRequest, defaultServerThrottle, throttledDefaultServerRequests, errorFunction))
				{
					return false;
				}
				else if ( (Network::Players::clientIsPresent(this) ) &&
					tryThrottleRequest(apiRequest, clientThrottle, throttledClientRequests, errorFunction))
				{
					return false;
				}

				break;
			}
		case PRIORITY_SERVER_ELEVATED:
			{
				if (isPlaySolo &&
					tryThrottleRequest(apiRequest, elevatedServerThrottle, throttledElevatedServerRequests, errorFunction))
				{
					return false;
				}
				else if ( Network::Players::serverIsPresent(this) &&
					tryThrottleRequest(apiRequest, elevatedServerThrottle, throttledElevatedServerRequests, errorFunction))
				{
					return false;
				}
				else if ( Network::Players::clientIsPresent(this) &&
					tryThrottleRequest(apiRequest, clientThrottle, throttledClientRequests, errorFunction))
				{
					return false;
				}

				break;
			}
		case PRIORITY_EXTREME:
			{
				// don't do anything here, extreme priority request should always go thru
				break;
			}
		default:
			{
				setErrorForAsync("No priority set for API proxy request, please set a priority.", errorFunction);
				return false;
				break;
			}
		}

		apiRequest.execute(this);

		return true;
	}

} //namespace RBX
