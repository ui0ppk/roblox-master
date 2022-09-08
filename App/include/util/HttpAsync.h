#pragma once

#include <string>
#include <istream>

#include <boost/thread/future.hpp>
#include <boost/unordered_map.hpp>
#include "util/HttpAux.h"

namespace RBX
{
    typedef boost::shared_future<std::string> HttpFuture;

    class HttpOptions
	{
        friend class HttpAsync;

	public:
        HttpOptions()
			: external(false)
			, doNotUseCachedResponse(false)
		{
		}

        void addHeader(const std::string& key, const std::string& value);
        void setExternal(bool value);
		void setDoNotUseCachedResponse();

	private:
        HttpAux::AdditionalHeaders headers;
        bool external;
		bool doNotUseCachedResponse;
	};

    class HttpPostData
	{
        friend class HttpAsync;

	public:
        HttpPostData(const std::string& contents, const std::string& contentType, bool compress);
        HttpPostData(const boost::shared_ptr<std::istream>& contents, const std::string& contentType, bool compress);

	private:
        boost::shared_ptr<std::istream> data;
        std::string contentType;
        bool compress;
	};

	class HttpAsync
	{
	public:
        static HttpFuture get(const std::string& url, const HttpOptions& options = HttpOptions());
        static HttpFuture getWithRetries(const std::string& url, int retryCount, const HttpOptions& options = HttpOptions());

        static HttpFuture post(const std::string& url, const HttpPostData& postData, const HttpOptions& options = HttpOptions());
    };
}
