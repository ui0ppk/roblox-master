#include "stdafx.h"
#include "Util/HttpAsync.h"

#include "Util/Http.h"

namespace RBX
{
    static const int kInitialRetryDelayMs = 200;

    typedef boost::promise<std::string> HttpPromise;

    static void responseHandler(std::string* result, std::exception* error, shared_ptr<HttpPromise> promise)
	{
        RBXASSERT(result || error);

        if (result)
			promise->set_value(*result);
		else
			promise->set_exception(boost::copy_exception(*error));
	}

    static void responseHandlerRetry(std::string* result, std::exception* error, shared_ptr<HttpPromise> promise, int retryCount, int retrySleep, bool externalRequest, Http http)
	{
        RBXASSERT(result || error);

        if (result)
		{
            promise->set_value(*result);
		}
		else
		{
            if (retryCount > 0)
			{
                // This takes out a thread in the thread pool :( use sparingly.
				boost::this_thread::sleep(boost::posix_time::milliseconds(retrySleep));

				http.get(boost::bind(responseHandlerRetry, _1, _2, promise, retryCount - 1, retrySleep * 2, externalRequest, http), externalRequest);
			}
			else
			{
                promise->set_exception(boost::copy_exception(*error));
			}
		}
	}

    void HttpOptions::addHeader(const std::string& key, const std::string& value)
	{
        headers[key] = value;
	}

    void HttpOptions::setExternal(bool value)
	{
        external = value;
	}

	void HttpOptions::setDoNotUseCachedResponse()
	{
		doNotUseCachedResponse = true;
	}

    HttpPostData::HttpPostData(const std::string& contents, const std::string& contentType, bool compress)
		: data(new std::istringstream(contents))
		, contentType(contentType)
        , compress(compress)
	{
	}

    HttpPostData::HttpPostData(const boost::shared_ptr<std::istream>& contents, const std::string& contentType, bool compress)
		: data(contents)
		, contentType(contentType)
        , compress(compress)
	{
	}

	HttpFuture HttpAsync::get(const std::string& url, const HttpOptions& options)
	{
		shared_ptr<HttpPromise> promise(new HttpPromise());

        Http http(url);
		http.additionalHeaders = options.headers;
		http.doNotUseCachedResponse = options.doNotUseCachedResponse;

		http.get(boost::bind(responseHandler, _1, _2, promise), options.external);

		return HttpFuture(promise->get_future());
	}

    HttpFuture HttpAsync::getWithRetries(const std::string& url, int retryCount, const HttpOptions& options)
    {
		shared_ptr<HttpPromise> promise(new HttpPromise());

        Http http(url);
		http.additionalHeaders = options.headers;
		http.doNotUseCachedResponse = options.doNotUseCachedResponse;

		http.get(boost::bind(responseHandlerRetry, _1, _2, promise, retryCount, kInitialRetryDelayMs, options.external, http), options.external);

		return HttpFuture(promise->get_future());
    }

	HttpFuture HttpAsync::post(const std::string& url, const HttpPostData& postData, const HttpOptions& options)
	{
		shared_ptr<HttpPromise> promise(new HttpPromise());

        Http http(url);
		http.additionalHeaders = options.headers;
		http.doNotUseCachedResponse = options.doNotUseCachedResponse;

		http.post(postData.data, postData.contentType, postData.compress, boost::bind(responseHandler, _1, _2, promise), options.external);

		return HttpFuture(promise->get_future());
	}
}
