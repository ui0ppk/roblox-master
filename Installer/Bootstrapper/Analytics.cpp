#include "stdafx.h"

#include "Analytics.h"
#include "format_string.h"
#include "SharedHelpers.h"

#include "wininet.h"
#include "atlutil.h"

namespace Analytics {

	static inline bool postImpl(const std::string& url, const std::string& params)
	{
		std::string postData = params;

		CString s;
		CUrl u;
		
#ifdef UNICODE
		u.CrackUrl(convert_s2w(url).c_str());
#else
		u.CrackUrl(url.c_str());
#endif

		const bool isSecure = u.GetScheme() == ATL_URL_SCHEME_HTTPS;

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
		if (isSecure)
			flags |= INTERNET_FLAG_SECURE;

		HINTERNET request = ::HttpOpenRequest(connection, _T("POST"), s, HTTP_VERSION, _T(""), NULL, flags, 0); 
		if (!request) 
			goto Error;

		::HttpAddRequestHeaders(request, _T("Content-Type: application/json"), -1L, HTTP_ADDREQ_FLAG_ADD);

		DWORD timeout = 5 * 1000; // milliseconds
		InternetSetOption(request, INTERNET_OPTION_CONNECT_TIMEOUT, (void*)&timeout, sizeof(timeout));
		InternetSetOption(request, INTERNET_OPTION_RECEIVE_TIMEOUT, (void*)&timeout, sizeof(timeout));
		InternetSetOption(request, INTERNET_OPTION_SEND_TIMEOUT, (void*)&timeout, sizeof(timeout));

		DWORD httpSendResult = ::HttpSendRequest(request, NULL, 0, (LPVOID)postData.c_str(), postData.length());
		if (!httpSendResult)
			goto Error;

		return true;

Error:
		if (session) 
			InternetCloseHandle(session);

		if (connection) 
			InternetCloseHandle(connection);

		if (request) 
			InternetCloseHandle(request);

		return false;
	}

	void post(const std::string& url, const std::string& data, bool blocking)
	{
		if (blocking)
			postImpl(url, data);
		else
		{
			// TODO:
		}
	}

namespace InfluxDb {

	std::string reporter;
	std::string hostName;
	std::string database;
	std::string user;
	std::string password;
	bool canSend = false;

	void init(const std::string& _reporter, const std::string& _url, const std::string& _database, const std::string& _user, const std::string& _pw)
	{
		reporter = _reporter;
		hostName = _url;
		database = _database;
		user = _user;
		password = _pw;
		canSend = true;
	}

	using namespace Analytics::InfluxDb;

	void reportPoints(const std::string& resource, const PointList& points)
	{
		if (!canSend || points.empty())
			return;

		// Pushes to InfluxDB.
		std::string url = hostName + "/db/" + database + "/series?u=" + user + "&p=" + password;

		std::string json = "[{";
		json += "\"name\":\"" + resource + "\",";
		json += "\"columns\":[\"reporter\"";
		for (PointList::const_iterator it = points.begin(); points.end() != it; ++it)
		{
			json += ",\"" + it->first + "\"";
		}
		json += "],";

		// two levels are here because we are actually sending a list of several points,
		// even though this function only handles one row of points at a time
		json += "\"points\":[[";
		json += "\"" +  reporter + "\"";
		for (PointList::const_iterator it = points.begin(); points.end() != it; ++it)
		{
			json += "," + it->second;
		}
		json += "]]";

		json += "}]";

		post(url, json, true);
	}


} // namespace InfluxDb



} // namespace Analytics

