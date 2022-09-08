#include "stdafx.h"

#include "CountersClient.h"
#include "windows.h"
#include "wininet.h"
#include "atlutil.h"
#include "RobloxServicesTools.h"

std::set<std::wstring> CountersClient::_events;

CountersClient::CountersClient(std::string baseUrl, std::string key, simple_logger<wchar_t>* logger) :
_logger(logger)
{
	_url = GetCountersMultiIncrementUrl(baseUrl, key);
	//TODO use LOG_ENTRY macros here
	if (logger)
		logger->write_logentry("CountersClient::CountersClient baseUrl=%s, url=%s", baseUrl.c_str(), _url.c_str());
}

void CountersClient::registerEvent(std::wstring eventName, bool fireImmediately)
{
	if(fireImmediately)
	{
		std::set<std::wstring> eventList;
		eventList.insert(eventName);
		reportEvents(eventList);
		return;
	}

	std::set<std::wstring>::iterator i = _events.find(eventName);
	if (_events.end() == i) 
	{
		_events.insert(eventName);
	}
}

void CountersClient::reportEvents()
{
	reportEvents(_events);
}

void CountersClient::reportEvents(std::set<std::wstring> &events)
{
	if (events.empty())
	{
		return;
	}

	CString s;

	std::string postData = "counterNamesCsv=";
	bool append = false;
	for (std::set<std::wstring>::iterator i = events.begin();i != events.end();i ++)
	{
		if (append)
		{
			postData += ",";
		}

		postData += convert_w2s(*i);

		append = true;
	}

	events.clear();
	CUrl u;

#ifdef UNICODE
	u.CrackUrl(convert_s2w(_url).c_str());
#else
	u.CrackUrl(_url.c_str());
#endif

    const bool isSecure = u.GetScheme() == ATL_URL_SCHEME_HTTPS;

	// Initialize the User Agent
#ifdef UNICODE
	HINTERNET session = InternetOpen(L"Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
#else
	HINTERNET session = InternetOpen("Roblox/WinInet", PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
#endif
	if (!session) 
	{
		goto Error;
	}

	HINTERNET connection = ::InternetConnect(session, u.GetHostName(), u.GetPortNumber(), u.GetUserName(), u.GetPassword(), INTERNET_SERVICE_HTTP, 0, 0); 
	if (!connection) 
	{
		goto Error;
	}

	s = u.GetUrlPath();
	s += u.GetExtraInfo();
	HINTERNET request = ::HttpOpenRequest(connection, _T("POST"), s, HTTP_VERSION, _T(""), NULL, isSecure ? INTERNET_FLAG_SECURE : 0, 0); 
	if (!request) 
	{
		goto Error;
	}

	::HttpAddRequestHeaders(request, _T("Content-Type: application/x-www-form-urlencoded"), -1L, HTTP_ADDREQ_FLAG_ADD);

	DWORD timeout = 5 * 1000; // milliseconds
	InternetSetOption(request, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&timeout, sizeof(timeout));
	InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, (void*)&timeout, sizeof(timeout));
	InternetSetOption(request, INTERNET_OPTION_SEND_TIMEOUT, (void*)&timeout, sizeof(timeout));

	DWORD httpSendResult = ::HttpSendRequest(request, NULL, 0, (LPVOID)postData.c_str(), postData.length());
	if (!httpSendResult) 
	{
		goto Error;
	}

Error:
	if (session) 
	{
		InternetCloseHandle(session);
	}

	if (connection) 
	{
		InternetCloseHandle(connection);
	}

	if (request) 
	{
		InternetCloseHandle(request);
	}
}
