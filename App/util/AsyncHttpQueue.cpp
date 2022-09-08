#include "stdafx.h"

#include "Util/AsyncHttpQueue.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Stats.h"
#include "v8datamodel/ContentProvider.h"
#include "Network/Api.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "winhttp.h" //Used for StatusCode Defines
#endif

#include "Util/SafeToLower.h"
#include "util/standardout.h"
#include "rbx/make_shared.h"

#include "StringConv.h"

LOGVARIABLE(SlowHttpRequest, 0);

namespace RBX
{

class HttpQueueStatsItem : public Stats::Item
{
	AsyncHttpQueue* httpQueue;
	Stats::Item* avgTimeInQueue;
	Stats::Item* avgRequestCompleteTime;
	Stats::Item* queueSize;
	Stats::Item* numSlowRequests;

public:
	HttpQueueStatsItem(AsyncHttpQueue* _httpQueue, Instance* httpQueueOwner) : httpQueue(_httpQueue)
	{
		setName("HttpQueue_" + httpQueueOwner->getName());
	}

	static shared_ptr<HttpQueueStatsItem> create(AsyncHttpQueue* httpQueue, Instance* httpQueueOwner)
	{
		boost::shared_ptr<HttpQueueStatsItem> result = Creatable<Instance>::create<HttpQueueStatsItem>(httpQueue, httpQueueOwner);
		result->init();
		return result;
	}

	void init()
	{
		avgTimeInQueue = createChildItem("Average time in queue");
		avgRequestCompleteTime = createChildItem("Average process time");
		queueSize = createChildItem("Queue size");
		numSlowRequests = createChildItem("Num slow requests");
	}

	virtual void update()
	{
		avgTimeInQueue->formatValue(httpQueue->getAvgTimeInQueue(), "%.4f msec", httpQueue->getAvgTimeInQueue());
		avgRequestCompleteTime->formatValue(httpQueue->getAvgRequestCompleteTime(), "%.4f msec", httpQueue->getAvgRequestCompleteTime());
		queueSize->formatValue(httpQueue->getRequestQueueSize());
		numSlowRequests->formatValue(httpQueue->getNumSlowRequests());
	}

};

AsyncHttpQueue::AsyncHttpQueue(Instance* owner, boost::function<bool(const std::string& url, std::string* result)> getLocalFile, int threadCount)
	:owner(owner)
	,getLocalFile(getLocalFile)
	,avgRequestCompleteTime(0.1)
	,avgTimeInQueue(0.1)
	,currentWallTime(0)
	,numSlowRequests(0)
    ,cachePolicy(HttpCache::PolicyDefault)
{
	setThreadPool(threadCount);
}

void AsyncHttpQueue::resetStatsItem(ServiceProvider* provider)
{
	if (statsItem)
	{
		statsItem->setParent(NULL);
		statsItem.reset();
	}

	// create the stats item
	Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(provider);
	if (stats)
	{
		statsItem = HttpQueueStatsItem::create(this, owner);
		statsItem->setParent(stats);
	}
}

shared_ptr<const Reflection::ValueArray> AsyncHttpQueue::getFailedUrls()
{
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
	{
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
int AsyncHttpQueue::getRequestQueueSize() const
{
	boost::recursive_mutex::scoped_lock lock(requestSync);
	return requestQueue.size();
}
shared_ptr<const Reflection::ValueArray> AsyncHttpQueue::getRequestQueueUrls()
{
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
	{
		{
			boost::recursive_mutex::scoped_lock lock(requestSync);
			RequestList::const_iterator end = requestQueue.end();
			for (RequestList::const_iterator iter = requestQueue.begin(); iter!=end; ++iter)
			{
				result->push_back(iter->url);
			}
		}
	}
	return result;
}

void AsyncHttpQueue::setThreadPool(int count)
{
	if(!threadPool || (threadPool->getThreadCount() != count)){
		threadPool.reset(new PriorityThreadPool(count, ThreadPool::NoAction));
	}
}

AsyncHttpQueue::~AsyncHttpQueue()
{
	{
		boost::recursive_mutex::scoped_lock lock(requestSync);

		requestQueue.clear();
	}

	//  We must join the thread since the work_function is a member function of this
	setThreadPool(0);
}
void AsyncHttpQueue::addAsyncRetryTask(RequestHandle request)
{
	boost::recursive_mutex::scoped_lock lock(asyncRetrySync);
	asyncRetryTasks.push(AsyncRetryTask(request, currentWallTime + 5));
}


void AsyncHttpQueue::onHeartbeat(const Heartbeat& heartbeatEvent)
{
	std::list<AsyncRetryTask> objectsToRequeue;
	{
		boost::recursive_mutex::scoped_lock lock(asyncRetrySync);
		currentWallTime += heartbeatEvent.wallStep;
		while(!asyncRetryTasks.empty() && asyncRetryTasks.front().retryTime < currentWallTime){
			objectsToRequeue.push_back(asyncRetryTasks.front());
			asyncRetryTasks.pop();
		}
	}

	if(!objectsToRequeue.empty()){
		boost::recursive_mutex::scoped_lock lock(requestSync);
		while(!objectsToRequeue.empty()){
			AsyncRetryTask& front = objectsToRequeue.front();
			threadPool->schedule(boost::bind(&AsyncHttpQueue::processRequests, weak_from(this), front.request, _1), front.request->priority);
			objectsToRequeue.pop_front();
		}
	}
}

static void InvokeAsyncCallback(AsyncHttpQueue::RequestCallback func, AsyncHttpQueue::RequestResult result, shared_ptr<const std::string> response, shared_ptr<std::exception> exception)
{
	if(response){
		std::istringstream ss(*response);
		func(result, &ss, response, exception);
	}
	else{
		func(result, NULL, response, exception);
	}
}

void AsyncHttpQueue::dispatchGenericCallback(boost::function<void(DataModel*)> theCallback, Instance* instance, ResultJob jobType)
{
	switch(jobType)
	{
	case AsyncHttpQueue::AsyncInline:
		theCallback(NULL);
		return;
	case AsyncHttpQueue::AsyncNone:
		if(instance){
			if(DataModel* dataModel = DataModel::get(instance)){
				dataModel->submitTask(theCallback, DataModelJob::None);
				return;
			}
		}
		break;;
	case AsyncHttpQueue::AsyncRead:
		if(instance){
			if(DataModel* dataModel = DataModel::get(instance)){
				dataModel->submitTask(theCallback, DataModelJob::Read);
				return;
			}
		}
		break;
	case AsyncHttpQueue::AsyncWrite:
		if(instance){
			if(DataModel* dataModel = DataModel::get(instance)){
				dataModel->submitTask(theCallback, DataModelJob::Write);
				return;
			}
		}
		break;
	}
	throw std::runtime_error("Unable to dispatch task");
}


void AsyncHttpQueue::dispatchCallback(AsyncHttpQueue::RequestCallback theCallback, Instance* instance,
									  RequestResult result, boost::shared_ptr<const std::string> data, ResultJob jobType, shared_ptr<std::exception> exception)
{
	dispatchGenericCallback(boost::bind(&InvokeAsyncCallback, theCallback, result, data, exception), instance, jobType);
}
static void callback(AsyncHttpQueue::CallbackWrapper f, Instance* owner, AsyncHttpQueue::RequestResult result, shared_ptr<std::string> response, shared_ptr<std::exception> exception)
{
	try
	{
		AsyncHttpQueue::dispatchCallback(f.callback, owner, result, response, f.jobType, exception);
	}
	catch (RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "ContentProvider callback exception: %s", e.what());
	}
}
bool AsyncHttpQueue::isRequestQueueEmpty()
{
	boost::recursive_mutex::scoped_lock lock(requestSync);
	return requestQueue.empty();
}
void AsyncHttpQueue::processRequests(boost::weak_ptr<AsyncHttpQueue> weakHttpQueue, RequestHandle request, boost::shared_ptr<rbx::spin_mutex> criticalSection)
{
	shared_ptr<std::string> response(new std::string());
	RequestResult result;
	std::string url;
	std::string filename;
	shared_ptr<std::exception> exception;
	

	if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock())
	{
		boost::recursive_mutex::scoped_lock lock(httpQueue->requestSync);
		url = request->url;
	}
	else{
		return;
	}
	//Process the item
	try
	{
		try
		{
			try
			{

				if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock())
				{
					
					bool localResult = httpQueue->getLocalFile(url, &filename);
					if(!localResult){
						result = Failed;
						return;
					}

					// sample average time spent in queue before actually fetching this request
					Time::Interval deltaTime = Time::now<Time::Fast>() - request->startTime;
					httpQueue->avgTimeInQueue.sample(deltaTime.msec());
				}

				if (!filename.empty()) // is a file on disk. do file load
				{
					rbx::spin_mutex::scoped_lock lock(*criticalSection);
					std::ifstream filestream(utf8_decode(filename).c_str(), std::ios_base::in | std::ios_base::binary);
					std::ostringstream strbuffer;
					strbuffer << filestream.rdbuf();
					*response = strbuffer.str();
					result = Succeeded;
				}
				else
				{	
					//RBXASSERT(id.isHttp());
					// must be a net file.

					boost::shared_ptr<Http> httpRequest;
						httpRequest.reset(new RBX::Http(url));

					httpRequest->setExpectedAssetType(request->expectedType);

					if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock())					
					{
						// protect the usage of the shared pointer by resusing the requestSync lock.
						boost::recursive_mutex::scoped_lock lock(httpQueue->requestSync);

                        request->http = httpRequest; // the sole purpose of saving the http object is to be able to cancel it.
                        httpRequest->setCachePolicy(httpQueue->cachePolicy);
					}

					httpRequest->get(*response);
					result = Succeeded;
				}
			}
			catch (RBX::http_status_error& e)
			{
				// This means the server has accepted  the request, but is taking longer to process, call again after some time and hopefully we will get a 200 OK once server is done processing
				if(e.statusCode == 202)
				{
					if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock()) 
					{
						boost::recursive_mutex::scoped_lock lock(httpQueue->requestSync);
						//It accepted the request and we should try this request again later
						request->http.reset();
						httpQueue->addAsyncRetryTask(request);
					}
					return;
				}
				throw;
			}
		}
		catch (RBX::base_exception&)
		{
			result = Failed;
			if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock()) {
				boost::recursive_mutex::scoped_lock lock(httpQueue->requestSync);
				FASTLOGS(FLog::HttpQueue, "Failed url on async fetch: %s", url);
				httpQueue->failedUrls.push_front(url.c_str());
			}
			
			throw;
		}
		
		if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock()) {
			try
			{
				shared_ptr<std::string> filenamePtr;
				if(!filename.empty())
					filenamePtr.reset(new std::string(filename));
				httpQueue->registerContent(url, response, filenamePtr);
			}
			catch(RBX::base_exception&)
			{
				result = Failed;
				throw;
			}
		}
	}
	catch (std::logic_error& e)
	{
		if(url.find("http") != std::string::npos){
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "Content failed for %s because %s", url.c_str(), e.what());
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Content failed because %s", e.what());
		}
		// the id has probably been put into failedUrls now
        exception = shared_ptr<std::exception>(new std::runtime_error(e.what()));
	}
	catch (RBX::base_exception& e)
	{
		if(url.find("http") != std::string::npos){
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "Content failed for %s because %s", url.c_str(), e.what());
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Content failed because %s", e.what());
		}
		// the id has probably been put into failedUrls now
		exception = shared_ptr<std::exception>(new std::runtime_error(e.what()));
	}

	// keep a copy of the callbacks before we destroy them with the erase.
	
	if(boost::shared_ptr<AsyncHttpQueue> httpQueue = weakHttpQueue.lock())
	{
		Time::Interval deltaT = Time::now<Time::Fast>() - request->startTime;

		// increment slow request counter if we are over threshold
		if (deltaT.seconds() >= 5.0)
		{
			httpQueue->numSlowRequests++;
			FASTLOGS(FLog::SlowHttpRequest, "Slow Http request: %s", request->url.c_str());
			FASTLOG2F(FLog::SlowHttpRequest, "request time %f, queue size %f", deltaT.seconds(), httpQueue->getRequestQueueSize());
		}

		// sample average total process time between when request added to queue til callback
		httpQueue->avgRequestCompleteTime.sample(deltaT.msec());

		std::vector<CallbackWrapper> callbacks;
		{
			//Copy the callbacks and erase the request
			boost::recursive_mutex::scoped_lock lock(httpQueue->requestSync);
			callbacks = request->callbacks;
			httpQueue->requestQueue.erase(request);
		}

		{
			//Issue the callbacks
			rbx::spin_mutex::scoped_lock lock(*criticalSection);
			std::for_each(callbacks.begin(), callbacks.end(), boost::bind(&callback, _1, httpQueue->owner, result, response, exception));
		}
	}
}


	AsyncHttpQueue::FailedUrl::FailedUrl(const char* url)
		:url(url)
#ifdef _DEBUG
		,expiration(RBX::Time::now<Time::Fast>() + Time::Interval(10 * 60))
#else
		,expiration(RBX::Time::now<Time::Fast>() + Time::Interval(5 * 60))
#endif
	{
	}

	bool AsyncHttpQueue::FailedUrl::expired() const
	{
		return RBX::Time::now<Time::Fast>() >= expiration;
	}

	
	bool AsyncHttpQueue::isUrlBad(const std::string& id)
	{
		if(id.empty())
			return true;

		boost::recursive_mutex::scoped_lock lock(requestSync);
		std::list< FailedUrl >::iterator end = failedUrls.end();
		for (std::list< FailedUrl >::iterator iter = failedUrls.begin(); iter!=end; ++iter)
		{
			if (iter->expired())	
			{
				FASTLOGS(FLog::HttpQueue, "Removing url from failed list, expired: %s", iter->url);
				failedUrls.erase(iter, end);
				return false;
			}
			if (iter->url==id)
				return true;
		}
		return false;
	}

	void AsyncHttpQueue::asyncRequest(const std::string& id, float priority, RequestCallback* callback, ResultJob jobType, bool ignoreBadRequests, const std::string& expectedType)
	{
		if (ignoreBadRequests || !isUrlBad(id)) {
			boost::recursive_mutex::scoped_lock lock(requestSync);

			// See if it is already in the queue
			std::list<Request>::iterator iter = std::find(requestQueue.begin(), requestQueue.end(), id);						
			if (iter==requestQueue.end())
			{
				requestQueue.push_back(Request());
				RequestHandle r = requestQueue.end();
				r--; // get last element;
				r->url = id;
				r->priority = priority;
				r->startTime = RBX::Time::nowFast();
				r->expectedType = expectedType;
				if (callback)
					r->callbacks.push_back(CallbackWrapper(*callback, jobType));
				
				threadPool->schedule(boost::bind(&AsyncHttpQueue::processRequests, weak_from(this), r, _1), priority);
			}
			else {
				//It already exists
				if (callback)
				{
					iter->callbacks.push_back(CallbackWrapper(*callback, jobType));
				}
			}
		}
		else {
			if(callback)
				dispatchCallback(*callback, owner, Failed, shared_ptr<std::string>(), jobType, shared_ptr<std::exception>(new std::runtime_error("Bad request")));
		}
	}
	bool AsyncHttpQueue::syncRequest(const std::string& id, const std::string& expectedType)
	{
		if (!isUrlBad(id)) {
			shared_ptr<std::string> response(new std::string());
			try
			{
				// TODO: Should we check for isBadUrl() here or only in the request queue?
					RBX::Http http(id);
                    http.setCachePolicy(cachePolicy);

					if (!expectedType.empty())
						http.setExpectedAssetType(expectedType);

                    http.get(*response);
                }
			catch(RBX::base_exception&)
			{
				boost::recursive_mutex::scoped_lock lock(requestSync);
				FASTLOGS(FLog::HttpQueue, "Failed url on sync fetch: %s", id);
				failedUrls.push_front(id.c_str());
				throw;
			}
			try
			{
				registerContent(id, response, shared_ptr<const std::string>());
			}
			catch(RBX::base_exception&)
			{
				return false;
			}
			return true;
		}
		return false;
	}
}
