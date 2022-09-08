#pragma once 
#include "Reflection/Reflection.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "rbx/RunningAverage.h"
#include "v8datamodel/HttpService.h"
#include "Util/Http.h"
#include "Util/DoubleEndedVector.h"

DYNAMIC_FASTINT(PercentApiRequestsRecordGoogleAnalytics)

namespace RBX {

	class HttpRbxApiJob;

	typedef enum 
	{
		PRIORITY_EXTREME			= 2, // Request will NOT be throttled ever
		PRIORITY_SERVER_ELEVATED	= 1, // Request may be throttled, should only be higher priority items (only works on server calls)
		PRIORITY_DEFAULT			= 0, // Request may be throttled
	} ThrottlingPriority;

	extern const char* const sHttpRbxApiService;
	class HttpRbxApiService
		:public DescribedCreatable<HttpRbxApiService, Instance, sHttpRbxApiService, Reflection::ClassDescriptor::INTERNAL>
		,public Service
	{
	public:

		// this struct is used to store/execute API requests
		struct HttpApiRequest
		{
		private:
			Http http;

		public:
			bool isPost;
			std::string postData;
			std::string httpContentType;

			bool async;
			std::string syncResponse;

			ThrottlingPriority throttlingPriority;

			int retryCount;

			boost::function<void(std::string)> resumeFunction;
			boost::function<void(std::string)> errorFunction;

			void execute(HttpRbxApiService* apiService);

			void setHttp(Http& newHttp)
			{
				http = newHttp;
				http.shouldRetry = false;
			}

			Http getHttp() const { return http; }

			HttpApiRequest()
			{
				retryCount = 0;
				postData = "";
				httpContentType = "";
				async = true;
				isPost = false;
				syncResponse = "";
				throttlingPriority = PRIORITY_DEFAULT;

				http.shouldRetry = false;
			}
		};

	private:
		typedef DescribedCreatable<HttpRbxApiService, Instance, sHttpRbxApiService, Reflection::ClassDescriptor::INTERNAL> Super;

		//////////////////////////////////////////////////////////////////////
		// MEMBERS
		//////////////////////////////////////////////////////////////////////

		// These track the number of API requests we execute by storing a budget
		// which is decremented every request, and is added to as time passes
		BudgetedThrottlingHelper defaultServerThrottle, elevatedServerThrottle, clientThrottle, retryBudget;

		// used to store requests that go over the throttle budgets
		// each queue can only store HttpRbxApiMaxThrottledQueueSize items
		// if HttpRbxApiMaxThrottledQueueSize is reached then the request returns a throttle error
		DoubleEndedVector<HttpApiRequest> throttledDefaultServerRequests, throttledElevatedServerRequests, throttledClientRequests, retryQueue;

		// a DataModel job responsible for allowing time for the
		// execution of HttpApiRequest objects
		shared_ptr<HttpRbxApiJob> httpRbxApiJob;

		std::string apiBaseUrl;
		static std::string StaticApiBaseUrl;

		bool serverPresent;
		bool clientPresent;
		bool isPlaySolo;
		rbx::signals::scoped_connection serviceAddedConnection;
		rbx::signals::scoped_connection playersChangedConnection;
		rbx::signals::scoped_connection contentProviderPropertyChangedConnection;

		unsigned int totalNumOfApiCalls;
		RBX::Timer<RBX::Time::Fast> instanceAliveTimer;

		bool recordInGoogleAnalytics;

		//////////////////////////////////////////////////////////////////////
		// METHODS
		//////////////////////////////////////////////////////////////////////

		// initialization helpers
		void checkForClientAndServer(Instance* context);
		void newServiceAdded(shared_ptr<Instance> newService);
		void playersPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc);
		void disconnectEventConnections();

		// general function that allows either async or sync functions to have their errors set appropriately
		void setErrorForAsync(const std::string& errorString, boost::function<void(std::string)> errorFunction);

		// Throttling Helpers
		bool tryThrottleRequest(const HttpApiRequest& apiRequest, BudgetedThrottlingHelper& budgetThrottler, DoubleEndedVector<HttpApiRequest>& throttledRequestQueue,
			 boost::function<void(std::string)> errorFunction);
		bool executeApiRequest(HttpApiRequest& apiRequest, const ThrottlingPriority& throttlePriority,
			boost::function<void(std::string)> errorFunction);
		void executeThrottledRequests(DoubleEndedVector<HttpApiRequest>& queue, BudgetedThrottlingHelper& helper);
		int getPlayerNum();

		// Post/Get Helpers
		void getAsyncInternal(Http& httpRequest, const ThrottlingPriority& throttlePriority,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void postAsyncInternal(Http& httpRequest, std::string& data, const HttpService::HttpContentType& contentType, 
			const bool shouldCompress, const ThrottlingPriority& throttlePriority, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void checkAndUpdatePostUrl(std::string& fullUrl, const std::string& urlPath) const;

		static void httpHelper(weak_ptr<HttpRbxApiService> weakApiService, std::string* response, std::exception* exception, HttpApiRequest request, ThrottlingPriority throttlePriority,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		static bool retrySyncRequest(Http& http, std::string& syncResponse);

		void setStaticApiBaseUrl(const RBX::Reflection::PropertyDescriptor* pPropertyDescriptor);

		//////////////////////////////////////////////////////////////
		// Instance
		///////////////////////
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	public:
		//////////////////////////////////////////////////////////////////////
		// FUNCTIONS
		//////////////////////////////////////////////////////////////////////

		HttpRbxApiService();

		// Google analytics helpers
		void addToApiCallCount() { totalNumOfApiCalls++; }
		bool getRecordInGoogleAnalytics() const { return recordInGoogleAnalytics; }

		// Throttling Functions
		void addThrottlingBudgets(float timeDeltaMinutes);
		void executeThrottledRequests();
		
		void executeRetryRequests();
		bool addToRetryQueue(HttpApiRequest apiRequest);

		// Synchronous Http calls (only use if we need to block the thread)
		void get(Http& httpRequest, bool useHttps, ThrottlingPriority throttlePriority, std::string& response);
		void get(const std::string& urlPath, bool useHttps, ThrottlingPriority throttlePriority, std::string& response);

		// Asynchronous Http calls
		void getAsync(Http& httpRequest, ThrottlingPriority throttlePriority,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getAsync(std::string urlPath, bool useHttps, ThrottlingPriority throttlePriority,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getAsyncLua(std::string urlPath, bool useHttps, ThrottlingPriority throttlePriority,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		void postAsync(Http& httpRequest, std::string& data, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, 
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void postAsync(std::string urlPath, std::string data,bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, 
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void postAsyncWithAdditionalHeaders(std::string urlPath, std::string data,bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, RBX::HttpAux::AdditionalHeaders additionalHeaders,
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void postAsyncLua(std::string urlPath, std::string data,bool useHttps, ThrottlingPriority throttlePriority, HttpService::HttpContentType content, 
			boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		// API URL Helpers
		static bool isAPIHttpRequest(const Http& httpRequest);
		static std::string getApiUrlPath(const Http& httpRequest);
	};
} //namespace RBX
