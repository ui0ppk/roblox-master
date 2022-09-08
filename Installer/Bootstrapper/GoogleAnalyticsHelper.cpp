#include "stdafx.h"
#include "GoogleAnalyticsHelper.h"
#include "HttpTools.h"
#include <sstream>
#include "windows.h"
#include "wininet.h"
#include "atlutil.h"
#include "SharedHelpers.h"

#include <iphlpapi.h>

namespace
{
	static bool initialized = false;
	static const std::string kGoogleAnalyticsBaseURL = "http://www.google-analytics.com/collect";
	static std::string googleAccountPropertyID;
	static std::string googleClientID;

	static inline std::string googleCollectionParams(const std::string &hitType)
	{
		std::stringstream ss;
		ss
			<< "v=1"
			<< "&tid=" << googleAccountPropertyID
			<< "&cid=" << googleClientID
			<< "&t=" << hitType;
		return ss.str();
	}

	static inline bool post(simple_logger<wchar_t> &logger, const std::string& params)
	{

		std::string postData = params;

		CString s;
		CUrl u;
		
#ifdef UNICODE
		u.CrackUrl(convert_s2w(kGoogleAnalyticsBaseURL).c_str());
#else
		u.CrackUrl(kGoogleAnalyticsBaseURL.c_str());
#endif

		// Initialize the User Agent
#ifdef UNICODE
		HINTERNET session = InternetOpen(L"Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
#else
		HINTERNET session = InternetOpen("Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
#endif
		if (!session) 
			goto Error;
		
		HINTERNET connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 0); 
		if (!connection) 
			goto Error;

		s = u.GetUrlPath();
		s += u.GetExtraInfo();
		DWORD flags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NEED_FILE | INTERNET_FLAG_NO_COOKIES;
		HINTERNET request = ::HttpOpenRequest(connection, _T("POST"), s, HTTP_VERSION, _T(""), NULL, flags, 0); 
		if (!request) 
			goto Error;

		::HttpAddRequestHeaders(request, _T("Content-Type: */*"), -1L, HTTP_ADDREQ_FLAG_ADD);

		DWORD timeout = 5 * 1000; // milliseconds
		InternetSetOption(request, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&timeout, sizeof(timeout));
		InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, (void*)&timeout, sizeof(timeout));
		InternetSetOption(request, INTERNET_OPTION_SEND_TIMEOUT, (void*)&timeout, sizeof(timeout));

		DWORD httpSendResult = ::HttpSendRequest(request, NULL, 0, (LPVOID)postData.c_str(), postData.length());
		if (!httpSendResult)
			goto Error;

		LLOG_ENTRY2(logger, "Google Analytics: %s, status: %d", params.c_str(), httpSendResult);

		return true;

Error:
		if (session) 
			InternetCloseHandle(session);

		if (connection) 
			InternetCloseHandle(connection);

		if (request) 
			InternetCloseHandle(request);

		LLOG_ENTRY2(logger, "WARNING: Google Analytics FAILED: %s, %S", params.c_str(), (LPCTSTR)GetLastErrorDescription());

		return false;
	}

	std::string getFirstMacAddress()
	{
		IP_ADAPTER_INFO AdapterInfo[16];
		DWORD dwBufLen = sizeof(AdapterInfo);

		DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
		assert(dwStatus == ERROR_SUCCESS);

		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

		do 
		{		
			if ((pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET) || (pAdapterInfo->Type == IF_TYPE_IEEE80211))
				return format_string("%02x%02x%02x%02x%02x%02x%02x%02x", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2], pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5], pAdapterInfo->Address[6], pAdapterInfo->Address[7]);

			pAdapterInfo = pAdapterInfo->Next;
		} while(pAdapterInfo);

		GUID g = {0};
		::CoCreateGuid(&g);
		return format_string("%08X",g.Data1);
	}
}

namespace GoogleAnalyticsHelper
{
	void init(const std::string &accountPropertyID)
	{
		if (!initialized)
		{
			googleAccountPropertyID = accountPropertyID;
			googleClientID = getFirstMacAddress();

			initialized = true;
		}
	}

	bool trackEvent(simple_logger<wchar_t> &logger, const char *category, const char *action, const char *label, int value)
	{
		if (!initialized)
			return false;

		std::stringstream params;
		params
			<< googleCollectionParams("event")
			<< "&ec=" << category
			<< "&ea=" << action
			<< "&ev=" << value
			<< "&el=" << label;

		return post(logger, params.str());
	}

	bool trackUserTiming(simple_logger<wchar_t> &logger, const char *category, const char *variable, int milliseconds, const char *label)
	{
		if (!initialized)
			return false;

		std::stringstream params;
		params
			<< googleCollectionParams("timing")
			<< "&utc=" << category
			<< "&utv=" << variable
			<< "&utt=" << milliseconds
			<< "&utl=" << label;

		return post(logger, params.str());

	}
}