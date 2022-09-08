#include "stdafx.h"

#ifdef _WIN32

#define NOMINMAX

// Grrrr! Need to include this before windows.h
#include "winsock2.h"
#include "windows.h"
#include "Util/Http.h"

#include "winhttp.h"

#pragma comment (lib , "Winhttp.lib")
#include "atlutil.h"
#include "g3d/format.h"
#include "Util/MD5Hasher.h"

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

class WINHTTPHINTERNET : boost::noncopyable
{
	HINTERNET handle;
public:
	WINHTTPHINTERNET(HINTERNET handle):handle(handle) {}
	WINHTTPHINTERNET():handle(0) {}
	WINHTTPHINTERNET& operator = (HINTERNET handle)
	{
		::WinHttpCloseHandle(handle);
		this->handle = handle;
		return *this;
	}
	operator bool() { return handle!=0; }
	operator HINTERNET() { return handle; }
	~WINHTTPHINTERNET()
	{
		::WinHttpCloseHandle(handle);
	}
};



class WinHttpRequest_source : public io::source {
	HINTERNET request;
public:
	WinHttpRequest_source(HINTERNET request):request(request) 
	{}

	std::streamsize read(char* s, std::streamsize n) {

		DWORD numBytes;
		if (!::WinHttpQueryDataAvailable(request, &numBytes))
			numBytes = 0;
		if (numBytes==0)
			return -1; // EOF

		DWORD bytesRead;
		if (!::WinHttpReadData(request, (LPVOID) s, n, &bytesRead))
			throw std::runtime_error("InternetReadFile failed");
		return bytesRead;
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



namespace RBX
{
	static VOID CALLBACK redirectCallback(
	  __in  HINTERNET hInternet,
	  __in  DWORD_PTR dwContext,
	  __in  DWORD dwInternetStatus,
	  __in  LPVOID lpvStatusInformation,
	  __in  DWORD dwStatusInformationLength
	)
	{
		CString s = (LPWSTR) lpvStatusInformation;
		Http* http = reinterpret_cast<Http*>(dwContext);
		if (http)
			http->onWinHttpRedirect(dwInternetStatus, (LPCTSTR) s);
	}

	void Http::httpGetPostWinHttp(bool isPost, std::istream& data, const std::string& contentType, bool compressData, const HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response)
	{
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

		if (u.GetHostNameLength()==0)
			throw RBX::runtime_error("'%s' is missing a hostName", theUrl.c_str());

		// Use WinHttpOpen to obtain a session handle.
		WINHTTPHINTERNET hSession = ::WinHttpOpen( L"Roblox/WinHttp", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );	
		ThrowIfFailure(hSession!=NULL, "WinHttpOpen");

		bool isHttps = u.GetScheme()==ATL_URL_SCHEME_HTTPS;


		// Specify an HTTP server.
		WINHTTPHINTERNET hConnect = ::WinHttpConnect( hSession, CComBSTR(CString(u.GetHostName())), u.GetPortNumber(), 0 );
		ThrowIfFailure(hConnect!=NULL, "WinHttpConnect" );

		std::string p = u.GetUrlPath();
		p += u.GetExtraInfo();
		WINHTTPHINTERNET hRequest = ::WinHttpOpenRequest( hConnect, isPost ? L"POST" : L"GET", CComBSTR(p.c_str()), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, isHttps ? WINHTTP_FLAG_SECURE : 0);
		ThrowIfFailure(hRequest!=NULL, "WinHttpOpenRequest");

		DWORD optionValue = WINHTTP_DISABLE_COOKIES;
		ThrowIfFailure(TRUE==::WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &optionValue, sizeof(optionValue)),"WinHttpSetOption");

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
				ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, CComBSTR(headers.c_str()), -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders");
		}

		if (externalRequest)
		{
			std::string header = "accesskey: UserRequest\r\n";
			ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, CComBSTR(header.c_str()), -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders");

			DWORD responseTimeout = DFInt::ExternalHttpResponseTimeoutMillis;
			ThrowIfFailure(TRUE==::WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT,
				&responseTimeout, sizeof(DWORD)), "SetTimeoutExternalHttpRequest");
		}
		else if (!Http::accessKey.empty())
		{
			std::string header = "accesskey: " + Http::accessKey + "\r\n";
			ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, CComBSTR(header.c_str()), -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders");
		}

		ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, L"Accept-Encoding: gzip\r\n", -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders failed");

		const io::zlib_params zlibParams = io::zlib::default_compression;
		const io::gzip_params gzipParams = io::gzip::default_compression;

		if (!isPost)
		{
			ThrowIfFailure(WINHTTP_INVALID_STATUS_CALLBACK != WinHttpSetStatusCallback(hRequest, &redirectCallback, WINHTTP_CALLBACK_FLAG_REDIRECT, 0), "WinHttpSetStatusCallback");
			ThrowIfFailure(TRUE==::WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR) this), "WinHttpSendRequest failed");
		}
		else
		{
			std::string uploadData;
			try
			{
				io::filtering_ostream out;
				if (compressData)
				{
					ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, L"Content-Encoding: gzip\r\n", -1, WINHTTP_ADDREQ_FLAG_ADD), "HttpAddRequestHeaders failed");
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
				ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, CComBSTR(contentSizeHeader.c_str()), -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders failed");

				boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
				hasher->addData(uploadData);
				std::string contentHashHeader = RBX::format("Roblox-Content-Hash: %s\r\n", hasher->c_str());
				ThrowIfFailure(TRUE==::WinHttpAddRequestHeaders(hRequest, CComBSTR(contentHashHeader.c_str()), -1, WINHTTP_ADDREQ_FLAG_ADD), "WinHttpAddRequestHeaders failed");
			}

			// Send the request
			ThrowIfFailure(TRUE==::WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)uploadData.c_str(), uploadSize, uploadSize, 0), "WinHttpSendRequest failed");
		}

		ThrowIfFailure(TRUE==::WinHttpReceiveResponse( hRequest, NULL ), "WinHttpReceiveResponse");

		if (externalRequest)
		{
			// Check for non-identity transfer encoding
			WCHAR transferCoding[256];
			DWORD transferCodingLength = sizeof(transferCoding);
			if (::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING,
				NULL, (LPVOID) transferCoding, &transferCodingLength, 0))
			{
				if (CString(transferCoding) != "identity")
				{
					throw RBX::runtime_error("External http request uses unsupported transfer encoding.");
				}
			}

			DWORD contentLength = 0;
			DWORD contentLengthLength = sizeof(contentLength);
			if (::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
				NULL, &contentLength, &contentLengthLength, 0) &&
				(contentLength / 1024) >= static_cast<DWORD>(DFInt::ExternalHttpResponseSizeLimitKB))
			{
				throw RBX::runtime_error(
					"Response exceeded size limit. Current limit: %d KB. Response size: %d KB",
					DFInt::ExternalHttpResponseSizeLimitKB, (contentLength / 1024));
			}
		}

		DWORD statusCode = -1;
		{
			DWORD dwSize = sizeof(DWORD);
			//get status code back from the response
			ThrowIfFailure(TRUE==::WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &dwSize, WINHTTP_NO_HEADER_INDEX ), "WinHttpQueryHeaders");
		}

		try
		{
			if (statusCode != 204)
			{
				io::filtering_istream in;
				{
					WCHAR buffer[256];
					DWORD length = 256;
					if (::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_ENCODING, NULL, (LPVOID) buffer, &length, 0))
						if (CString(buffer)=="gzip")
							in.push(io::gzip_decompressor());
				}
				in.push(WinHttpRequest_source(hRequest));
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

		if (statusCode < 200 || statusCode >= 300)
		{
			if (useCsrfHeaders && statusCode == 403)
			{
				// check to see if there is a csrf token header in the response that is different from the
				// token in the original request, and if so update the csrf token and retry the request
				TCHAR headersOut[128];
				DWORD headersOutLength = 128;
				DWORD headerIndex;

				// WinHttpQueryHeaders will return false if the requested header does not exist.
				if (::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"X-CSRF-TOKEN", headersOut, &headersOutLength, &headerIndex))
				{
					std::string newToken(headersOut);
					if (newToken != csrfTokenForRequest)
					{
						setLastCsrfToken(newToken);
						response.clear();
						return httpGetPostWinHttp(isPost, data, contentType, compressData, additionalHeaders, externalRequest, response);
					}
				}
			}

			WCHAR buffer[512];
			DWORD length = 512;
			if (::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_TEXT, NULL, (LPVOID) buffer, &length, WINHTTP_NO_HEADER_INDEX))
				throw RBX::http_status_error(statusCode, (LPCTSTR)CString(buffer));
			else
				throw RBX::http_status_error(statusCode);
		}

	}
}

#endif  // _WIN32
