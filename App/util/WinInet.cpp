#include "stdafx.h"

#ifdef _WIN32

#define NOMINMAX

// Grrrr! Need to include this before windows.h
#include "winsock2.h"
#include "windows.h"
#include "wininet.h"

#include "Util/Http.h"
#include "Util/SafeToLower.h"
#include "util/standardout.h"

#pragma comment (lib , "Wininet.lib")
#include "atlutil.h"
#include "g3d/format.h"
#include "Util/MD5Hasher.h"

#include "Strsafe.h"

#include <sstream>

#define BOOST_IOSTREAMS_NO_LIB
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace io = boost::iostreams;
#include <boost/algorithm/string.hpp>

#ifdef __RBX_FILEGZIP__
#include "zlib.h"
#endif

DYNAMIC_FASTINT(ExternalHttpResponseTimeoutMillis)
DYNAMIC_FASTINT(ExternalHttpResponseSizeLimitKB)

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

struct String_sink  : public io::sink {
	std::string& s;
	String_sink(std::string& s):s(s){}
    std::streamsize write(const char* s, std::streamsize n) 
    {
        this->s.append(s, n);
		return n;
    }
};

class WiniInetRequest_source : public io::source {
	HINTERNET request;
	DWORD numBytesAvailable;
public:
	WiniInetRequest_source(HINTERNET request)
		: request(request) 
		, numBytesAvailable(0)
	{}

	std::streamsize read(char* s, std::streamsize n) 
	{
		// problem? read this: http://www.ssuet.edu.pk/taimoor/books/1-56276-448-9/ch4.htm
		// seems this function can sometimes falsely return that no data is available. implement their hack?
		if(numBytesAvailable == 0)
		{
			// we have read all data given by last call to InternetQueryDataAvailable. 
			// let's see if there is more.
			// MSDN docs says we need to read _all_ numBytesAvailable data before this function recalculates available data.
			while (!::InternetQueryDataAvailable(request, &numBytesAvailable, 0, 0))
			{
				DWORD err = GetLastError();
				if(err == ERROR_IO_PENDING)
				{
					Sleep(100); // we block on data.
				}
				else
				{
					RBX::Http::ThrowLastError(err, "(unknown)", "InternetQueryDataAvailable");
				}
			}
		}

		if (numBytesAvailable==0)
			return -1; // EOF

		DWORD bytesRead;

		RBX::Http::ThrowIfFailure(TRUE == ::InternetReadFile(request, (LPVOID) s, std::min(numBytesAvailable, (DWORD)n), &bytesRead), "(unknown)", "InternetReadFile");
		numBytesAvailable -= bytesRead;
		return bytesRead;
	}
};



namespace RBX
{
	struct CallbackContext
	{
		Http* http;
	};

	static VOID CALLBACK callback(
	  __in  HINTERNET hInternet,
	  __in  DWORD_PTR dwContext,
	  __in  DWORD dwInternetStatus,
	  __in  LPVOID lpvStatusInformation,
	  __in  DWORD dwStatusInformationLength
	)
	{
		CallbackContext* context = reinterpret_cast<CallbackContext*>(dwContext);
		if(!context || !context->http)
			return;

		if (dwInternetStatus == INTERNET_STATUS_REDIRECT)
		{
			CString s = (LPCTSTR) lpvStatusInformation;
			context->http->onWinHttpRedirect(dwInternetStatus, (LPCTSTR) s);
		}
	}


	void Http::httpGetPostWinInet(bool isPost, std::istream& data, const std::string& contentType, bool compressData, const HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response)
	{
#ifdef _NOOPT
		InternetSetCookie(url.c_str(), "ThisCookieGetsYouInToSt3", "");
		InternetSetCookie(url.c_str(), "HTTPAccessByAdminCookie", "");
#endif
		const bool useCsrfHeaders = isPost && !externalRequest;
		if (useCsrfHeaders)
		{
			data.seekg(std::istream::beg);
		}

		std::string theUrl = alternateUrl.empty() ? url : alternateUrl;

		if (theUrl.size()==0)
			throw std::runtime_error("empty url");

 		if (!Http::trustCheck(theUrl.c_str(), externalRequest))
 			throw RBX::runtime_error("trust check failed for %s", theUrl.c_str());
		
		CUrl u;
		u.CrackUrl(theUrl.c_str());

		const bool isSecure = u.GetScheme() == ATL_URL_SCHEME_HTTPS;

		if (u.GetHostNameLength()==0)
			throw RBX::runtime_error("'%s' is missing a hostName", theUrl.c_str());

		// Initialize the User Agent
		WININETHINTERNET session = InternetOpen("Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
		if (!session)
			throw RBX::runtime_error("InternetOpen failed for %s", theUrl.c_str());

		WININETHINTERNET connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 0); 
		if (!connection)
			throw RBX::runtime_error("InternetConnect failed for %s", theUrl.c_str());

		//   1. Open HTTP Request (pass method type [get/post/..] and URL path (except server name))
		CString requestUrl = u.GetUrlPath();
		requestUrl += u.GetExtraInfo();

		DWORD flags;
		if (isSecure)
			flags = INTERNET_FLAG_SECURE; // | INTERNET_FLAG_DONT_CACHE;
		else
			flags = INTERNET_FLAG_KEEP_CONNECTION /*| INTERNET_FLAG_EXISTING_CONNECT*/ | INTERNET_FLAG_NEED_FILE; // ensure that it gets cached

		if (externalRequest)
		{
			flags |=  INTERNET_FLAG_NO_COOKIES;
		}

		if (doNotUseCachedResponse)
		{
			flags |= INTERNET_FLAG_RELOAD;
		}

		WININETHINTERNET request = ::HttpOpenRequest(
			connection, isPost ? "POST" : "GET", requestUrl, HTTP_VERSION, "", NULL, 
			flags, 
			0); 
		if (!request)
			throw RBX::runtime_error("HttpOpenRequest failed for %s", theUrl.c_str());

		std::string csrfTokenForRequest;
		{
			std::string headers;
			for (HttpAux::AdditionalHeaders::const_iterator iter = additionalHeaders.begin(); iter!=additionalHeaders.end(); ++iter)
			{
				RBXASSERT(!isPost || !boost::iequals("Content-Type", iter->first));
				headers += iter->first + ": " + iter->second + "\r\n";
			}
			if (isPost)
			{
				headers += "Content-Type: " + contentType + "\r\n";
				if (useCsrfHeaders)
				{
					csrfTokenForRequest = getLastCsrfToken();
					if (!csrfTokenForRequest.empty())
					{
						headers += "X-CSRF-TOKEN: " + csrfTokenForRequest + "\r\n";
					}
				}
			}

			if (headers.size()>0)
				ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, headers.c_str(), headers.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
		}

		{
			const char* acceptEncoding = "Accept-Encoding: gzip\r\n";
			ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, acceptEncoding, strlen(acceptEncoding), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
		}

		const io::gzip_params gzipParams = io::gzip::default_compression;

        if (!useDefaultTimeouts)
        {
			DWORD timeout = sendTimeoutMillis;
       	    ThrowIfFailure(TRUE==::InternetSetOption(request, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(DWORD)),
    		    "SetSendTimeoutHttpRequest");
			
            timeout = connectTimeoutMillis;
            ThrowIfFailure(TRUE==::InternetSetOption(request, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(DWORD)),
		        "SetConnectTimeoutHttpRequest");
			
            timeout = dataSendTimeoutMillis;
            ThrowIfFailure(TRUE==::InternetSetOption(request, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(DWORD)),
		        "SetDataSendTimeoutHttpRequest");
			
            timeout = responseTimeoutMillis;
            ThrowIfFailure(TRUE==::InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(DWORD)),
                "SetResponseTimeoutHttpRequest");
        }

		if (externalRequest)
		{
			DWORD responseTimeout = DFInt::ExternalHttpResponseTimeoutMillis;
			ThrowIfFailure(TRUE==::InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, &responseTimeout, sizeof(DWORD)),
				"SetTimeoutExternalHttpRequest");
		}

		if (!isPost)
		{
			CallbackContext context;
			try
			{
				context.http = this;

				void* buff = &context;
				ThrowIfFailure(FALSE != InternetSetOption(request, INTERNET_OPTION_CONTEXT_VALUE, &buff, sizeof(void*)), "InternetSetOption");
				ThrowIfFailure(INTERNET_INVALID_STATUS_CALLBACK != InternetSetStatusCallback(request, &callback), "InternetSetStatusCallback");

				DWORD httpSendResult = ::HttpSendRequest(request, NULL, 0, 0, 0);

				if(httpSendResult != TRUE)
				{
					DWORD err = GetLastError();
					Http::ThrowLastError(err, url.c_str(), "HttpSendRequest");
				}

				// make sure we unbind our callback.
				InternetSetStatusCallback(request, NULL);
				context.http = 0;
			}
			catch(...)
			{
				// make sure we unbind our callback.
				InternetSetStatusCallback(request, NULL);
				context.http = 0;
				throw;
			}
		}
		else
		{
			std::string uploadData;
			try
			{
				io::filtering_ostream out;
				if (compressData)
				{
					std::string contentEncodingHeader = "Content-Encoding: gzip\r\n";
					ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, contentEncodingHeader.c_str(), contentEncodingHeader.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
					out.push(io::gzip_compressor(gzipParams));
				}
				out.push(String_sink(uploadData));
				io::copy(data, out);
			}
			catch (io::gzip_error& e)
			{
				int e1 = e.error();
				int e2 = e.zlib_error_code();
				throw RBX::runtime_error("Upload GZip error %d, ZLib error %d, \"%s\"", e1, e2, theUrl.c_str());
			}
			const size_t uploadSize = uploadData.size();

			if (uploadSize>10000)
			{
				// TODO: These headers are added to debug the Gzip bug. Maybe we can eliminate them someday...
				std::string contentSizeHeader = RBX::format("Roblox-Content-Size: %d\r\n", uploadSize);
				ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, contentSizeHeader.c_str(), contentSizeHeader.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");

				std::string contentFirstHeader = RBX::format("Roblox-Content-First: %d\r\n", uploadData.c_str()[0]);
				ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, contentFirstHeader.c_str(), contentFirstHeader.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");

				std::string contentLastHeader = RBX::format("Roblox-Content-Last: %d\r\n", uploadData.c_str()[uploadSize-1]);
				ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, contentLastHeader.c_str(), contentLastHeader.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");

				boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
				hasher->addData(uploadData);
				std::string contentHashHeader = RBX::format("Roblox-Content-Hash: %s\r\n", hasher->c_str());
				ThrowIfFailure(TRUE==::HttpAddRequestHeaders(request, contentHashHeader.c_str(), contentHashHeader.size(), HTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
			}

			// Send the request
			{
				INTERNET_BUFFERS buffer;
				memset(&buffer, 0, sizeof(buffer));
				buffer.dwStructSize = sizeof(buffer);
				buffer.dwBufferTotal = uploadSize;
				if (!HttpSendRequestEx(request, &buffer, NULL, 0, 0))
                {
                    throw RBX::runtime_error("HttpSendRequestEx failed: %d", GetLastError());
                }
				
				if (uploadSize > 0)
				{
					try
					{
						DWORD bytesWritten;
						ThrowIfFailure(TRUE==::InternetWriteFile(request, (const void*)uploadData.c_str(), uploadSize, &bytesWritten), "InternetWriteFile failed");

						if (bytesWritten!=uploadSize)
							throw std::runtime_error("Failed to upload content");
					}
					catch (RBX::base_exception&)
					{
						::HttpEndRequest(request, NULL, 0, 0);
						throw;
					}
				}
				
				ThrowIfFailure(TRUE==::HttpEndRequest(request, NULL, 0, 0), "HttpEndRequest failed");
			}
		}

		// Check the return HTTP Status Code
		DWORD statusCode;
		{
			DWORD dwLen = sizeof(DWORD);
			ThrowIfFailure(TRUE==::HttpQueryInfo(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &dwLen, NULL), "HttpQueryInfo failed");
		}
		
		if (externalRequest)
		{
			// Check for non-identity transfer encoding
			TCHAR transferCoding[256];
			DWORD transferCodingLength = sizeof(transferCoding);
			if (::HttpQueryInfo(request, HTTP_QUERY_CONTENT_TRANSFER_ENCODING,
				(LPVOID)transferCoding, &transferCodingLength, 0))
			{
				if (CString(transferCoding) != "identity")
				{
					throw RBX::runtime_error("External http request uses unsupported transfer encoding.");
				}
			}

			DWORD contentLength = 0;
			DWORD contentLengthLength = sizeof(contentLength);
			if (::HttpQueryInfo(request, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
				&contentLength, &contentLengthLength, 0) &&
				(contentLength / 1024) >= static_cast<DWORD>(DFInt::ExternalHttpResponseSizeLimitKB))
			{
				throw RBX::runtime_error(
					"Response exceeded size limit. Current limit: %d KB. Response size: %d KB",
					DFInt::ExternalHttpResponseSizeLimitKB, (contentLength / 1024));
			}
		}

		{
			try
			{
				if (statusCode != 204) // 204: No Content
				{
					io::filtering_istream in;
					{
						TCHAR buffer[256];
						DWORD length = 256;
						if (::HttpQueryInfo(request, HTTP_QUERY_CONTENT_ENCODING, (LPVOID) buffer, &length, 0))
							if (std::string(buffer)=="gzip")
								in.push(io::gzip_decompressor());
					}
					in.push(WiniInetRequest_source(request));
					io::copy(in, String_sink(response));
				}
			}
			catch (io::gzip_error& e)
			{
				int e1 = e.error();
				int e2 = e.zlib_error_code();
				throw RBX::runtime_error("GZip error %d, ZLib error %d, \"%s\"", e1, e2, theUrl.c_str());
			}
			catch (io::zlib_error& e)
			{
				int e1 = e.error();
				throw RBX::runtime_error("ZLib error %d, \"%s\"", e1, theUrl.c_str());
			}
		}
		if(statusCode == HTTP_STATUS_CREATED)
		{
			// Youtube video uploaded successfully 
			return;
		}

		if (statusCode < 200 || statusCode >= 300)
		{
			if (useCsrfHeaders && statusCode == 403)
			{
				// check to see if there is a csrf token header in the response that is different from the
				// token in the original request, and if so update the csrf token and retry the request
				TCHAR headersOut[128];
				DWORD headersOutLength = 128;
				::StringCchPrintfA((LPSTR)headersOut, headersOutLength, "X-CSRF-TOKEN");

				// HttpQueryInfo will return false if it cannot find the requested header
				if (::HttpQueryInfo(request, HTTP_QUERY_CUSTOM, (LPVOID)headersOut, &headersOutLength, NULL))
				{
					std::string newToken(headersOut);
					if (newToken != csrfTokenForRequest)
					{
						setLastCsrfToken(newToken);
						response.clear();
						return httpGetPostWinInet(isPost, data, contentType, compressData, additionalHeaders, externalRequest, response);
					}
				}
			}

			TCHAR buffer[512];
			DWORD length = 512;
			if(response != "" && externalRequest)
				RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Error response: %s", response.c_str());

			if (::HttpQueryInfo(request, HTTP_QUERY_STATUS_TEXT, (LPVOID) buffer, &length, 0))
				throw RBX::http_status_error(statusCode,buffer);
			else
				throw RBX::http_status_error(statusCode);
		}


		if (isSecure)
		{
			INTERNET_CERTIFICATE_INFO certificateInfo; 
			DWORD certInfoLength = sizeof(INTERNET_CERTIFICATE_INFO); 
			if ( TRUE == InternetQueryOption( request,
															  INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
															  &certificateInfo,
															  &certInfoLength) )
			{ 
				// free up memory with LocalFree()

				if ( certificateInfo.lpszEncryptionAlgName )
					LocalFree( certificateInfo.lpszEncryptionAlgName);

				if ( certificateInfo.lpszIssuerInfo )
					LocalFree( certificateInfo.lpszIssuerInfo );

				if ( certificateInfo.lpszProtocolName )
					LocalFree( certificateInfo.lpszProtocolName );

				if ( certificateInfo.lpszSignatureAlgName )
					LocalFree( certificateInfo.lpszSignatureAlgName );

				if ( certificateInfo.lpszSubjectInfo )
				{
					bool foundIt = false;
					std::string s = certificateInfo.lpszSubjectInfo;
					LocalFree( certificateInfo.lpszSubjectInfo );

					int pos = s.find(0x0d);
					if (pos == std::string::npos)
						throw std::runtime_error("mc");

					std::string currString = s.substr(0,pos);

					if (currString.size() >= 11 &&
                        (currString.find(".roblox.com") != std::string::npos || currString.find(".robloxlabs.com") != std::string::npos))
						foundIt = true;

					while(!foundIt && !externalRequest)
					{
						pos = s.find(0x0d,pos + 1);
						if (pos == std::string::npos)
							throw std::runtime_error("mc");

						int nextReturn = s.find(0x0d,pos + 1);
						if(nextReturn == std::string::npos)
							nextReturn = s.size();

						currString = s.substr(pos,nextReturn - pos);
						safeToLower(currString);

                        if (currString.size() >= 11 &&
                            (currString.find(".roblox.com") != std::string::npos || currString.find(".robloxlabs.com") != std::string::npos))
                            foundIt = true;

						if(!foundIt && nextReturn == s.size())
						{
							std::string su = "u";
							su += "d";
							throw std::runtime_error(su);
						}
					}
				}
				else
					throw std::runtime_error("mi");

			} 
			else 
			{ 
					// process error
					DWORD error = GetLastError(); 
			} 
		}
	}

	void Http::setCookiesForDomainWinInet(const std::string& domain, const std::string& cookies)
	{
		std::string usedDomain = domain;
		
		if (!boost::starts_with(domain, "http"))
		{
			usedDomain = "http://" + usedDomain;
		}
		
		std::vector<std::string> cookiePairs;
		boost::split(cookiePairs, cookies, boost::is_any_of(";"));
		for (std::vector<std::string>::iterator itr = cookiePairs.begin(); itr != cookiePairs.end(); ++itr)
		{
			std::vector<std::string> cookiePair;
			boost::split(cookiePair, *itr, boost::is_any_of("="));
			if (cookiePair.size() == 2)
			{
				boost::trim(cookiePair[0]);
				boost::trim(cookiePair[1]);
				InternetSetCookie(usedDomain.c_str(), cookiePair[0].c_str(), cookiePair[1].c_str());
			}
		}
	}
}

#endif  // _WIN32
