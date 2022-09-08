#pragma once

#include <stdio.h>
#include <string>
#include <istream>
#include <memory>
#include <vector>

#include "util/name.h"
#include "rbx/boost.hpp"
#include "rbx/rbxTime.h"
#include "Util/contentid.h"
#include "Util/HeartbeatInstance.h"
#include "Util/LRUCache.h"
#include "Util/ThreadPool.h"
#include "Util/Http.h"
#include "V8Tree/Service.h"

LOGGROUP(HttpQueue)

namespace RBX {
	class DataModel;
	class HttpQueueStatsItem;

class AsyncHttpQueue
	 : public boost::enable_shared_from_this<AsyncHttpQueue>
	 , public boost::noncopyable
{
	shared_ptr<HttpQueueStatsItem> statsItem;

public:
	typedef enum { Waiting, Succeeded, Failed } RequestResult;
	typedef enum { AsyncInline, AsyncNone, AsyncRead, AsyncWrite} ResultJob;

	typedef boost::function<void(RequestResult, std::istream*, shared_ptr<const std::string> response, shared_ptr<std::exception> exception)> RequestCallback;

	struct CallbackWrapper
	{
		RequestCallback callback;
		ResultJob jobType;
		CallbackWrapper(RequestCallback callback, ResultJob jobType)
			:callback(callback)
			,jobType(jobType)
		{}
	};

	void setThreadPool(int count);
    void setCachePolicy(const HttpCache::Policy policy) { cachePolicy = policy; }
	
	bool isRequestQueueEmpty();
	bool isUrlBad(const std::string& id);

	void asyncRequest(const std::string& id, float priority, RequestCallback* callback, ResultJob jobType, bool ignoreBadRequests=false, const std::string& expectedType = "");
	bool syncRequest(const std::string& id, const std::string& expectedType = "");


	static void dispatchGenericCallback(boost::function<void(DataModel*)> theCallback, Instance* instance, ResultJob jobType);
	static void dispatchCallback(RequestCallback theCallback, Instance* instance, 
								 RequestResult result, boost::shared_ptr<const std::string> data, ResultJob jobType, shared_ptr<std::exception> exception);

	AsyncHttpQueue(Instance* owner,boost::function<bool(const std::string& url, std::string* result)> getLocalFile, int threadCount);
	virtual ~AsyncHttpQueue();
	void onHeartbeat(const Heartbeat& heartbeatEvent);

	int getRequestQueueSize() const;
	shared_ptr<const Reflection::ValueArray> getFailedUrls();
	shared_ptr<const Reflection::ValueArray> getRequestQueueUrls();

	void resetStatsItem(ServiceProvider* provider);
	double getAvgTimeInQueue() { return avgTimeInQueue.value(); }
	double getAvgRequestCompleteTime() { return avgRequestCompleteTime.value(); }
	int getNumSlowRequests() {return numSlowRequests;}

protected:
	virtual void registerContent(const std::string& url, shared_ptr<const std::string> response, shared_ptr<const std::string> filename)
	{}
	struct Request
	{
		std::string url;
		std::vector<CallbackWrapper> callbacks;
		float priority;
		std::string expectedType;	// used in header
		boost::shared_ptr<Http> http; // keep a handle so we can cancel.
		RBX::Time startTime; // the time when this request was issued
		bool operator==(const std::string& url) const { return this->url==url; }
	};

	RunningAverage<double> avgTimeInQueue;				// in msec
	RunningAverage<double> avgRequestCompleteTime;		// in msec
	int numSlowRequests;

	struct FailedUrl
  	{
		std::string url;
		RBX::Time expiration;
		FailedUrl(const char* url);
		bool expired() const;
	};

	typedef std::list< Request > RequestList;
	typedef RequestList::iterator RequestHandle;

	struct AsyncRetryTask
	{
		double retryTime;
		RequestHandle request;
		AsyncRetryTask(RequestHandle request , double retryTime)
			:request(request), retryTime(retryTime)
		{}
	};

	mutable boost::recursive_mutex requestSync;		// synchronizes the requestQueue and failedUrls queue, and threadPool
			  									    // also used to protect modification of the http shared pointer.
	
	
	RequestList requestQueue;				// queue of requested URLs
	std::list< FailedUrl > failedUrls;		// List of bad URLs

	boost::scoped_ptr<PriorityThreadPool> threadPool;

	boost::recursive_mutex asyncRetrySync;
	std::queue<AsyncRetryTask> asyncRetryTasks;

	double currentWallTime;

	static void processRequests(boost::weak_ptr<AsyncHttpQueue> httpQueue, RequestHandle request, boost::shared_ptr<rbx::spin_mutex> lock);
	void addAsyncRetryTask(RequestHandle request);

	Instance* owner;
	boost::function<bool(const std::string& url, std::string* result)> getLocalFile;

    HttpCache::Policy cachePolicy;
};
}