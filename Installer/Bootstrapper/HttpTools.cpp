#include "stdafx.h"
#include "HttpTools.h"
#include "SharedHelpers.h"
#include "atlutil.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "wininet.h"
#include <strstream>
#pragma comment (lib, "Wininet.lib")

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>

static const std::string sContentLength = "content-length: ";
static const std::string sEtag = "etag: ";

namespace HttpTools
{
	class WININETHINTERNET : boost::noncopyable
	{
		HINTERNET handle;
	public:
		WININETHINTERNET(HINTERNET handle):handle(handle) {}
		WININETHINTERNET():handle(0) {}
		WININETHINTERNET& operator = (HINTERNET handle)
		{
			::InternetCloseHandle(this->handle);
			this->handle = handle;
			return *this;
		}
		operator bool() { return handle!=0; }
		operator HINTERNET() { return handle; }
		~WININETHINTERNET()
		{
			::InternetCloseHandle(handle);
		}
	};

	int httpWinInet(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress);
	int httpboost(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress);
	int http(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log = true);

	static void dummyProgress(int, int) {}
	const std::string getPrimaryCdnHost(IInstallerSite *site)
	{
		static std::string cdnHost;
		static bool cdnHostLoaded = false;
		static bool validCdnHost = false;

		if(!cdnHostLoaded)
		{
			if (site->ReplaceCdnTxt()) {
				std::string host = site->InstallHost();
				std::string prod ("setup.roblox.com");
				if (host.compare(prod) == 0) {
					cdnHost = "setup.rbxcdn.com";
					validCdnHost = true;
					LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
				} else {
					cdnHost = host;
					validCdnHost = true;
					LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
				}
			} else {
				try
				{
					std::ostrstream result;
					int status_code;
					switch(status_code = httpGet(site, site->InstallHost(), "/cdn.txt", std::string(), result, false, boost::bind(&dummyProgress, _1, _2)))
					{
						case 200:
						case 304:
							result << (char) 0;
							cdnHost = result.str();
							LLOG_ENTRY1(site->Logger(), "primaryCdn: %s", cdnHost.c_str());
							validCdnHost = true;
							break;
						default:
							validCdnHost = false;
							LLOG_ENTRY1(site->Logger(), "primaryCdn failure code=%d, falling back to secondary installHost", status_code);
							break;
					}
				}
				catch(std::exception&)
				{
					//Quash exceptions, set validCdnHost to false
					validCdnHost = false;
					LLOG_ENTRY(site->Logger(), "primaryCdn exception, falling back to secondary installHost");
				}
			}
			
			//Only try to load the CDN once, then give up
			cdnHostLoaded = true;
		}

		return cdnHost;
	}

	const std::string getCdnHost(IInstallerSite *site)
	{
		static std::string cdnHost;
		static bool cdnHostLoaded = false;
		static bool validCdnHost = false;

		if(!cdnHostLoaded)
		{
			try
			{
				std::ostrstream result;
				int status_code;
				switch(status_code = httpGet(site, site->BaseHost(), "/install/GetInstallerCdns.ashx", std::string(), result, false, boost::bind(&dummyProgress, _1, _2)))
				{
				case 200:
				case 304:
				{
					result << (char) 0;
					LLOG_ENTRY1(site->Logger(), "primaryCdns: %s", result.str());

					std::stringstream stream(result.str());
					boost::property_tree::ptree ptree;
					boost::property_tree::json_parser::read_json(stream, ptree);

					std::vector<std::pair<std::string, int>> cdns;
					int totalValue = 0;
					boost_FOREACH(const boost::property_tree::ptree::value_type& child, ptree.get_child(""))
					{
						int value = boost::lexical_cast<int>(child.second.data());
						if (cdns.size() > 0)
							cdns.push_back(std::make_pair(child.first.data(), cdns.back().second + value));
						else
							cdns.push_back(std::make_pair(child.first.data(), boost::lexical_cast<int>(child.second.data())));

						totalValue += value;
					}

					if (cdns.size() && (totalValue > 0))
					{
						// randomly pick a cdn
						int r = rand() % totalValue;
						for (unsigned int i = 0; i < cdns.size(); i++)
						{
							if (r < cdns[i].second)
							{
								cdnHost = cdns[i].first;
								validCdnHost = true;
								break;
							}
						}
					}

					break;
				}
				default:
					validCdnHost = false;
					LLOG_ENTRY1(site->Logger(), "primaryCdn failure code=%d, falling back to secondary installHost", status_code);
					break;
				}
			}
			catch(std::exception&)
			{
				LLOG_ENTRY(site->Logger(), "primaryCdn exception, falling back to secondary installHost");
			}
			//Only try to load the CDN once, then give up
			cdnHostLoaded = true;
		}

		return cdnHost;
	}

	using boost::asio::ip::tcp;

	class CInternet : boost::noncopyable
	{
		HINTERNET handle;
	public:
		CInternet(HINTERNET handle):handle(handle) {}
		CInternet():handle(0) {}
		CInternet& operator = (HINTERNET handle)
		{
			::InternetCloseHandle(handle);
			this->handle = handle;
			return *this;
		}
		operator bool() { return handle!=0; }
		operator HINTERNET() { return handle; }
		~CInternet()
		{
			::InternetCloseHandle(handle);
		}
	};

	class Buffer : boost::noncopyable
	{
		void* const data;
	public:
		Buffer(size_t size):data(malloc(size)) {}
		~Buffer() { free(data); }
		operator const void*() const { return data; }
		operator void*() { return data; }
		operator char*() { return (char*)data; }
	};

	int httpGet(IInstallerSite *site, std::string host, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		try
		{
			std::string tmp = etag;
			std::strstream input;
			int i = http(site, "GET", host, path, input, NULL, tmp, result, ignoreCancel, progress, log);
			etag = tmp;
			return i;
		}
		catch (silent_exception&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			LLOG_ENTRY2(site->Logger(), "WARNING: First HTTP GET error for %s: %s", path.c_str(), exceptionToString(e).c_str());
			std::strstream input;
			result.seekp(0);
			result.clear();
			// try again
			return http(site, "GET", host, path, input, NULL, etag, result, ignoreCancel, progress, log);
		}
	}

	std::string httpGetString(const std::string& url)
	{
		CUrl u;
		u.CrackUrl(convert_s2w(url).c_str());

		const bool isSecure = u.GetScheme() == ATL_URL_SCHEME_HTTPS;

		// Initialize the User Agent
		WININETHINTERNET session = InternetOpen(L"Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
		if (!session) 
		{
			throw std::runtime_error("httpGetString - InternetOpen ERROR");
		}

		WININETHINTERNET connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 0); 
		if (!connection) 
		{
			throw std::runtime_error("httpGetString - InternetConnect ERROR");
		}

		CString s = u.GetUrlPath();
		s += u.GetExtraInfo();
		WININETHINTERNET request = ::HttpOpenRequest(connection, _T("GET"), s, HTTP_VERSION, _T(""), NULL, isSecure ? INTERNET_FLAG_SECURE : 0, 0); 
		if (!request) 
		{
			throw std::runtime_error("httpGetString - HttpOpenRequest ERROR");
		}

		DWORD httpSendResult = ::HttpSendRequest(request, NULL, 0, 0, 0);
		if (!httpSendResult) 
		{
			throw std::runtime_error("httpGetString - HttpSendRequest ERROR");
		}

		std::ostringstream data;
		while (true)
		{
			DWORD numBytes;
			if (!::InternetQueryDataAvailable(request, &numBytes, 0, 0))
			{
				DWORD err = GetLastError();
				if(err == ERROR_IO_PENDING)
				{
					Sleep(100); // we block on data.
					continue;
				}
				else
				{
					throw std::runtime_error("httpGetString - InternetQueryDataAvailable ERROR");
				}
			}

			if (numBytes==0)
				break; // EOF

			if (numBytes == -1)
			{
				throw std::runtime_error("httpGetString - No Settings ERROR");
			}

			char* buffer = (char*)malloc(numBytes + 1);
			DWORD bytesRead;
			throwLastError(::InternetReadFile(request, (LPVOID) buffer, numBytes, &bytesRead), "InternetReadFile failed");
			data.write(buffer, bytesRead);
			free(buffer);
		}

		return data.str();
	}

	int httpWinInet(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress)
	{
		CUrl u;
		BOOL urlCracked;
		#ifdef UNICODE
			urlCracked = u.CrackUrl(convert_s2w(host).c_str());
		#else
			urlCracked = u.CrackUrl(host.c_str());
		#endif

		// Initialize the User Agent
		CInternet session = InternetOpen(_T("Roblox/WinInet"), PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
		if (!session)
			throw std::runtime_error(format_string("InternetOpen failed for %s http://%s%s, Error Code: %d", method, host.c_str(), path.c_str(), GetLastError()).c_str());

		CInternet connection;
		if (urlCracked)
			connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		else
			connection = ::InternetConnect(session, convert_s2w(host).c_str(), 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);

		if (!connection)
			throw std::runtime_error(format_string("InternetConnect failed for %s http://%s%s, Error Code: %d, Port Number: %d", method, host.c_str(), path.c_str(), GetLastError() , urlCracked ? u.GetPortNumber() : 80).c_str());

		//   1. Open HTTP Request (pass method type [get/post/..] and URL path (except server name))
		CInternet request = ::HttpOpenRequest(
			connection, convert_s2w(method).c_str(), convert_s2w(path).c_str(), NULL, NULL, NULL, 
			INTERNET_FLAG_KEEP_CONNECTION |
			INTERNET_FLAG_EXISTING_CONNECT |
			INTERNET_FLAG_NEED_FILE, // ensure that it gets cached
			1); 
		if (!request)
			throw std::runtime_error(format_string("HttpOpenRequest failed for %s http://%s%s, Error Code: %d", method, host.c_str(), path.c_str(), GetLastError()).c_str());

		if (contentType)
		{
			std::string header = format_string("Content-Type: %s\r\n", contentType);
			throwLastError(::HttpAddRequestHeaders(request, convert_s2w(header).c_str(), header.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
		}

		size_t uploadSize;
		{
			size_t x = input.tellg();
			input.seekg (0, std::ios::end);
			size_t y = input.tellg();
			uploadSize = y - x;
			input.seekg (0, std::ios::beg);
		}

		if (uploadSize==0)
		{
			throwLastError(::HttpSendRequest(request, NULL, 0, 0, 0), "HttpSendRequest failed");
		}
		else
		{
			Buffer uploadBuffer(uploadSize);

			input.read((char*)uploadBuffer, uploadSize);

			// Send the request
			{
				INTERNET_BUFFERS buffer;
				memset(&buffer, 0, sizeof(buffer));
				buffer.dwStructSize = sizeof(buffer);
				buffer.dwBufferTotal = uploadSize;
				if (!HttpSendRequestEx(request, &buffer, NULL, 0, 0))
					throw std::runtime_error("HttpSendRequestEx failed");

				try
				{
					DWORD bytesWritten;
					throwLastError(::InternetWriteFile(request, uploadBuffer, uploadSize, &bytesWritten), "InternetWriteFile failed");

					if (bytesWritten!=uploadSize)
						throw std::runtime_error("Failed to upload content");
				}
				catch (std::exception&)
				{
					::HttpEndRequest(request, NULL, 0, 0);
					throw;
				}

				throwLastError(::HttpEndRequest(request, NULL, 0, 0), "HttpEndRequest failed");
			}
		}

		// Check the return HTTP Status Code
		DWORD statusCode;
		{
			DWORD dwLen = sizeof(DWORD);
			throwLastError(::HttpQueryInfo(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &dwLen, NULL), "HttpQueryInfo HTTP_QUERY_STATUS_CODE failed");
		}
		DWORD contentLength = 0;
		{
			DWORD dwLen = sizeof(DWORD);
			::HttpQueryInfo(request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwLen, NULL);
		}
		{
			CHAR buffer[256];
			DWORD dwLen = sizeof(buffer);
			if (::HttpQueryInfo(request, HTTP_QUERY_ETAG, &buffer, &dwLen, NULL))
			{
				etag = buffer;
				etag = etag.substr(1, etag.size()-2);	// remove the quotes
			}
			else
				etag.clear();
		}

		DWORD readSoFar = 0;
		while (true)
		{
			DWORD numBytes;
			if (!::InternetQueryDataAvailable(request, &numBytes, 0, 0))
				numBytes = 0;
			if (numBytes==0)
				break; // EOF

			char buffer[1024];
			DWORD bytesRead;
			throwLastError(::InternetReadFile(request, (LPVOID) buffer, 1024, &bytesRead), "InternetReadFile failed");
			result.write(buffer, bytesRead);
			readSoFar += bytesRead;
			progress(bytesRead, contentLength);
		}

		if (statusCode!=HTTP_STATUS_OK)
		{
			TCHAR buffer[512];
			DWORD length = 512;
			if (::HttpQueryInfo(request, HTTP_QUERY_STATUS_TEXT, (LPVOID) buffer, &length, 0))
				throw std::runtime_error(convert_w2s(buffer).c_str());
			else
				throw std::runtime_error(format_string("statusCode = %d", statusCode));
		}

		return statusCode;
	}

	int httpboost(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		if (log)
		{
			if (!etag.empty())
			{
				LLOG_ENTRY4(site->Logger(), "%s http://%s%s If-None-Match: \"%s\"", method, host.c_str(), path.c_str(), etag.c_str());
			}
			else
			{
				LLOG_ENTRY3(site->Logger(), "%s http://%s%s", method, host.c_str(), path.c_str());
			}
		}

		CUrl u;
		BOOL urlCracked;
#ifdef UNICODE
		urlCracked = u.CrackUrl(convert_s2w(host).c_str());
#else
		urlCracked = u.CrackUrl(host.c_str());
#endif

		boost::asio::io_service io_service;

		tcp::socket socket(io_service);

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);

		std::string port = urlCracked ? boost::lexical_cast<std::string>(u.GetPortNumber()) : "http";
		std::string hostName = urlCracked ? convert_w2s(u.GetHostName()) : host;
		tcp::resolver::query query(hostName, port);
		
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;

		if (httpboostPostTimeout > 0 && !std::strcmp(method, "POST"))
		{
			DWORD timeout = httpboostPostTimeout * 1000;
			setsockopt(socket.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
			setsockopt(socket.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		}

		// Try each endpoint until we successfully establish a connection.
		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end)
		{
		  socket.close();
		  socket.connect(*endpoint_iterator++, error);
		}

		if (error)
		{
			if (log)
			{
				LLOG_ENTRY1(site->Logger(), "Failed to find an endpoint: %s", error.message().c_str());
				LLOG_ENTRY3(site->Logger(), "Trying WinInet for %s http://%s%s", method, host.c_str(), path.c_str());
			}
			return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
		}

		size_t inputSize;
		{
			size_t x = input.tellg();
			input.seekg (0, std::ios::end);
			size_t y = input.tellg();
			inputSize = y - x;
			input.seekg (0, std::ios::beg);
		}

		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << method << " " << path << " HTTP/1.0\r\n";
		request_stream << "Host: " << host << "\r\n";
		request_stream << "Accept: */*\r\n";
		if (inputSize || !std::strcmp(method, "POST"))
			request_stream << "Content-Length: " << inputSize << "\r\n";
		if (contentType)
			request_stream << "Content-Type: " << contentType << "\r\n";
		request_stream << "Connection: close\r\n";
		if (!etag.empty())
			request_stream << "If-None-Match: \"" << etag << "\"\r\n";
		// TODO: Accept gzip encoding!
		request_stream << "\r\n";
		if (inputSize)
			std::copy(
				std::istreambuf_iterator<char>(input),
				std::istreambuf_iterator<char>(),
				std::ostreambuf_iterator<char>(request_stream)
			);	

		// Send the request.
		boost::asio::write(socket, request);

		// Read the response status line.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
			throw std::runtime_error("Invalid response");

		if (log)
			LLOG_ENTRY4(site->Logger(), "%s http://%s%s status_code:%d", method, host.c_str(), path.c_str(), status_code);

		switch (status_code)
		{
		case 200:
		case 304:
			break;
		default:
			{
				if (log)
				{
					LLOG_ENTRY2(site->Logger(), "Response returned with bad status code %d: %s", status_code, status_message.c_str());
					LLOG_ENTRY3(site->Logger(), "Trying WinInet for %s http://%s%s", method, host.c_str(), path.c_str());
				}
				return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
			}
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");

		// Process the response headers.
		size_t contentLength = 0;
		etag.clear();
		std::string header;
		while (std::getline(response_stream, header) && header != "\r")
		{
			// convert header to lower case
			std::transform(header.begin(), header.end(), header.begin(), tolower);

			if (header.find(sContentLength)==0)
				contentLength = atoi(header.substr(sContentLength.size()).c_str());
			else if (header.find(sEtag)==0)
				// Strip the double-quotes
				etag = header.substr(sEtag.size()+1, header.size()-sEtag.size()-3);
			if (!ignoreCancel)
				site->CheckCancel();
		}

		if (status_code!=304)
		{
			// Write whatever content we already have to output.
			if (response.size() > 0)
			  result << &response;

			progress(result.tellp(), contentLength);

			// Read until EOF, writing data to output as we go.
			while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
			{
				result << &response;
				progress(result.tellp(), contentLength);
				if (!ignoreCancel)
					site->CheckCancel();
			}
			if (error != boost::asio::error::eof)
			  throw boost::system::system_error(error, "Failed to read response");
		}
		else
			progress(1, 1);

		return status_code;
	}

	int http(IInstallerSite *site, const char* method, const std::string& host, const std::string& path, std::istream& input, const char* contentType, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		try
		{
			return httpboost(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress, log);
		}
		catch (std::exception&)
		{
			LLOG_ENTRY(site->Logger(), "httpboost failed, falling back to winInet");
			return httpWinInet(site, method, host, path, input, contentType, etag, result, ignoreCancel, progress);
		}
	}

	int httpPost(IInstallerSite *site, std::string host, std::string path, std::istream& input, const char* contentType, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress, bool log)
	{
		return http(site, "POST", host, path, input, contentType, std::string(), result, ignoreCancel, progress, log);
	}

	int httpGetCdn(IInstallerSite *site, std::string secondaryHost, std::string path, std::string& etag, std::ostream& result, bool ignoreCancel, boost::function<void(int, int)> progress)
	{
		std::string cdnHost;
		if (site->UseNewCdn())
			cdnHost = getCdnHost(site);
		
		if (cdnHost.empty())
			cdnHost = getPrimaryCdnHost(site);
		
		if(!cdnHost.empty()){
			try
			{
				std::string tmp = etag;
				int status_code = httpGet(site, cdnHost, path, tmp, result, ignoreCancel, progress);
				switch(status_code){
					case 200:
					case 304:
						//We succeeded so save the etag and return success
						etag = tmp;
						return status_code;
					default:
						LLOG_ENTRY3(site->Logger(), "Failure getting '%s' from cdnHost='%s', falling back to secondaryHost='%s'", path.c_str(), cdnHost.c_str(), secondaryHost.c_str());
						//Failure of some kind, fall back to secondaryHost below
						break;
				}
			}
			catch(std::exception&)
			{ 
				//Trap first exception and try again with secondaryHost
			}
		}

		//Reset our result vector
		result.seekp(0);
		result.clear();

		return httpGet(site, secondaryHost,path,etag,result,ignoreCancel,progress);
	}

}
