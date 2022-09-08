#include "stdafx.h"

#include "Util/Analytics.h"
#include "util/ThreadPool.h"
#include "Util/Http.h"
#include "Util/Statistics.h"
#include "Util/RobloxGoogleAnalytics.h"

#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/DebugSettings.h"

#include "RobloxServicesTools.h"

#include <boost/algorithm/string.hpp>

using namespace RBX;

DYNAMIC_LOGVARIABLE(AnalyticsLog, 0)

// google analytics
DYNAMIC_LOGGROUP(GoogleAnalyticsTracking)
FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAG(DebugAnalyticsForceLotteryWin)
DYNAMIC_FASTSTRING(RobloxAnalyticsURL)
DYNAMIC_FASTFLAG(RobloxAnalyticsTrackingEnabled)
DYNAMIC_FASTFLAG(DebugAnalyticsSendUserId)
FASTFLAG(SendStudioEventsWithStudioSID)
FASTFLAG(UseBuildGenericGameUrl)

// influx
DYNAMIC_FASTSTRING(HttpInfluxURL)
DYNAMIC_FASTSTRING(HttpInfluxDatabase)
DYNAMIC_FASTSTRING(HttpInfluxUser)
DYNAMIC_FASTSTRING(HttpInfluxPassword)
DYNAMIC_FASTFLAGVARIABLE(InfluxDb09Enabled, false)

namespace { // http utils

	const int kNumberThreadPoolThreads = 16;
	ThreadPool defaultThreadPool(kNumberThreadPoolThreads, BaseThreadPool::NoAction, 500);

	void httpHandler(std::string url, std::string params, std::string optionalContentType, bool isGet, bool externalRequest)
	{
		std::istringstream sparams(params);
		std::string response;

		std::string fullUrl = isGet ? format("%s?%s", url.c_str(), params.c_str()) : url;

		RBX::Http http(fullUrl);
		http.recordStatistics = false;
		http.doNotUseCachedResponse = true;
		try
		{
			if(isGet)
				http.get(response);
			else
				http.post(sparams, optionalContentType.empty() ? Http::kContentTypeUrlEncoded : optionalContentType, params.size() > 256, response, externalRequest);
		}
		catch (const RBX::base_exception &ex)
		{
			FASTLOGS(DFLog::AnalyticsLog, "Exception in analytics httpHandler: %s", ex.what());
		}
	}

	void postOrGet(ThreadPool& threadPool, const std::string& url, const std::string& data, const std::string& optionalContentType, bool blocking, bool isGet, bool externalRequest)
	{
		if (blocking)
		{
			httpHandler(url, data, optionalContentType, isGet, externalRequest);
		}
		else
		{
			bool didSchedule = threadPool.schedule(boost::bind(&httpHandler, url, data, optionalContentType, isGet, externalRequest));
			RBXASSERT(didSchedule);
			if (!didSchedule)
			{
				FASTLOGS(DFLog::AnalyticsLog, "Analytics task was not scheduled in the threadpool: %s", data.c_str());
			}
		}
	}
}

namespace RBX {
namespace Analytics {

	static std::string appVersion;
	static std::string reporter;
	static std::string location;
	static std::string userId;
	static std::string placeId;

	std::string sanitizeParam(const std::string& p)
	{
		std::string result = p;
		result.erase(std::remove_if(result.begin(), result.end(), boost::is_any_of(", ")), result.end());
		return result;
	}

	void setUserId(int id)
	{
		userId = RBX::format("%d", id);
	}

	void setPlaceId(int id)
	{
		placeId = RBX::format("%d", id);
	}

	void setAppVersion(const std::string& version)
	{
		appVersion = sanitizeParam(version);
	}

	void setLocation(const std::string& loc)
	{
		location = sanitizeParam(loc);
	}

	void setReporter(const std::string& rep)
	{
		reporter = sanitizeParam(rep);
	}

namespace EphemeralCounter
{
	const std::string countersApiKey =  "76E5A40C-3AE1-4028-9F10-7C62520BD94F";
	ThreadPool countersThreadPool(kNumberThreadPoolThreads, BaseThreadPool::NoAction, 500);

	void reportStats(const std::string& category, float value, bool blocking)
	{
		std::string baseUrl = ::GetBaseURL();
		if (baseUrl[baseUrl.size() - 1] != '/')
			baseUrl += '/';

		std::string valueStr = RBX::format("%f", value);
        std::string url;
		if (FFlag::UseBuildGenericGameUrl)
		{
			url = BuildGenericGameUrl(baseUrl, RBX::format("game/report-stats?name=%s&value=%s", Http::urlEncode(category).c_str(), Http::urlEncode(valueStr).c_str()));
		}
		else
		{
			url = RBX::format("%sgame/report-stats?name=%s&value=%s", baseUrl.c_str(), Http::urlEncode(category).c_str(), Http::urlEncode(valueStr).c_str());
		}

		// post
		postOrGet(countersThreadPool, url, "", Http::kContentTypeUrlEncoded, blocking, false, false);
	}

	void reportCountersCSV(const std::string& counterNamesCSV, bool blocking)
	{
		std::string counterUrl = GetCountersMultiIncrementUrl(::GetBaseURL(), countersApiKey);
		std::string data = "counterNamesCsv=" + counterNamesCSV;

		// post
		postOrGet(countersThreadPool, counterUrl, data, Http::kContentTypeUrlEncoded, blocking, false, false);
	}

	void reportCounter(const std::string& counterName, int amount, bool blocking)
	{
		std::string counterUrl = GetCountersUrl(::GetBaseURL(), countersApiKey);
		std::string url = RBX::format("%s&counterName=%s&amount=%d", counterUrl.c_str(), counterName.c_str(), amount);

		// post
		postOrGet(countersThreadPool, url, "", Http::kContentTypeUrlEncoded, blocking, false, false);
	}

} // namespace EphemeralCounter

namespace GoogleAnalytics {

	bool initialized = false;
	bool canUseGA = false;
	bool canUserRobloxEvents = false;
	std::string googleClientID;
	std::string googleAccountPropertyID;
	std::string robloxProductName;
	std::string experimentString;
	std::string robloxSessionKey;
	static rbx::signal<void()> analyticTrackedSignalVar;

	bool atteptedToUseBeforeInit = false;

	std::string generateClientID()
	{
		RBX::Guid guid;
		std::string clientGUID;
		guid.generateStandardGUID(clientGUID);

		// OS Generated GUIDs come in braced form ie "{f81d4fae-7dec-11d0-a765-00a0c91e6bf6}"
		// whereas RFC 4122 http://www.ietf.org/rfc/rfc4122.txt is non-braced.
		clientGUID.erase(std::find(clientGUID.begin(), clientGUID.end(), '{'));
		clientGUID.erase(std::find(clientGUID.begin(), clientGUID.end(), '}'));

		return clientGUID;
	}

	inline std::string googleCollectionParams(const std::string &hitType)
	{
		std::stringstream ss;
		ss
			<< "v=1"
			<< "&tid=" << googleAccountPropertyID
			<< "&cid=" << googleClientID
			<< "&t=" << hitType;

		if (DFFlag::DebugAnalyticsSendUserId && userId.length() > 0)
			ss << "&userId=" << userId;

		return ss.str();
	}

	void lotteryInit(const std::string &accountPropertyID, int lotteryThreshold, const std::string& productName /*= ""*/, int robloxAnalyticsLottery /* = -1 */, const std::string &sessionKey /* = "sessionID=" */)
	{
		GoogleAnalytics::init(accountPropertyID, productName);

		robloxSessionKey = sessionKey;

		if(robloxAnalyticsLottery == -1)
		{
			robloxAnalyticsLottery = lotteryThreshold;
		}

		std::size_t lottery = boost::hash_value(googleClientID) % 100;
		FASTLOG1(DFLog::GoogleAnalyticsTracking, "Google analytics lottery number = %d", lottery);
		// initialize google analytics
		if (lottery < (std::size_t)lotteryThreshold || FFlag::DebugAnalyticsForceLotteryWin)
		{
			canUseGA = true;
		}
		if (lottery < (std::size_t)robloxAnalyticsLottery || FFlag::DebugAnalyticsForceLotteryWin)
		{
			canUserRobloxEvents = true;
		}
	}

	void init(const std::string &accountPropertyID, const std::string& productName)
	{
		if(atteptedToUseBeforeInit)
			RBXASSERT(false); // Did you try to send events before analytics is initialized? Set breakpoints to trackEvent* to find out.

		if (initialized)
		{
			FASTLOGS(DFLog::GoogleAnalyticsTracking, "%s", "Google analytics already initialized!");
			return;
		}

		robloxProductName = productName;

		googleAccountPropertyID = accountPropertyID;
		RBXASSERT(googleAccountPropertyID.length() > 0);

		std::string clientId = GameBasicSettings::singleton().getGoogleAnalyticsClientId();
		if (clientId.length() == 0)
		{
			// generate and store new client ID
			clientId = generateClientID();
			GameBasicSettings::singleton().setGoogleAnalyticsClientId(clientId);
		}

		googleClientID = clientId;
		RBXASSERT(googleClientID.length() > 0);

		initialized = true;
	}

	bool getCanUse()
	{
		return canUseGA || canUserRobloxEvents;
	}

	void setCanUse()
	{
		canUseGA = true;
		canUserRobloxEvents = true;
	}

	void sendEventGA(const char* category,
		const char* action,
		const char* label,
		int value,
		bool sync)
	{
		std::stringstream params;
		params
			<< googleCollectionParams("event")
			<< "&ec=" << category
			<< "&ea=" << action
			<< "&ev=" << value
			<< "&el=" << label;

		postOrGet(defaultThreadPool, RobloxGoogleAnalytics::kGoogleAnalyticsBaseURL, params.str(), Http::kContentTypeDefaultUnspecified, sync, false, true);
	}

	void sendEventRoblox(const char* category,
		const char* action,
		const char* label,
		int value,
		bool sync)
	{
		if (!initialized)
		{
			atteptedToUseBeforeInit = true;
			return;
		}

		if(DFFlag::RobloxAnalyticsTrackingEnabled && !robloxProductName.empty())
		{
			std::stringstream params;
			params
				<< (FFlag::SendStudioEventsWithStudioSID ? robloxSessionKey : "sessionId=") << googleClientID
				<< "&userID=" << userId
				<< "&placeID=" << placeId
				<< "&category=" << category
				<< "&evt=" << action
				<< "&label=" << label
				<< "&value=" << value
				<< experimentString;		

			postOrGet(defaultThreadPool, RBX::format("%s%s/e.png",DFString::RobloxAnalyticsURL.c_str(), robloxProductName.c_str()), params.str(), Http::kContentTypeDefaultUnspecified, sync, true, true);
		}
	}

	void trackEvent(const char *category,
		const char *action,
		const char *label,
		int value,
		bool sync)
	{
		if (!initialized)
		{
			atteptedToUseBeforeInit = true;
			return;
		}

		if(canUseGA)
		{
			sendEventGA(category, action, label, value, sync);
		}

		if(canUserRobloxEvents)
		{
			sendEventRoblox(category, action, label, value, sync);
		}

		analyticTrackedSignalVar();
	}

	void trackEventWithoutThrottling(const char* category,
		const char* action,
		const char* label,
		int value,
		bool sync)
	{
		if (!initialized)
		{
			atteptedToUseBeforeInit = true;
			return;
		}

		sendEventGA(category, action, label, value, sync);
		sendEventRoblox(category, action, label, value, sync);

		analyticTrackedSignalVar();
	}

	void trackUserTiming(const char *category,
		const char *variable,
		int milliseconds,
		const char *label,
		bool sync)
	{
		if (!initialized || !canUseGA)
		{
			return;
		}

		std::stringstream params;
		params
			<< googleCollectionParams("timing")
			<< "&utc=" << category
			<< "&utv=" << variable
			<< "&utt=" << milliseconds
			<< "&utl=" << label;

		postOrGet(defaultThreadPool, RobloxGoogleAnalytics::kGoogleAnalyticsBaseURL, params.str(), Http::kContentTypeDefaultUnspecified, sync, false, true);

		analyticTrackedSignalVar();
	}

	rbx::signal<void()>& analyticTrackedSignal()
	{
		return analyticTrackedSignalVar;
	}

	void setExperimentVariation(const std::string& name, int value)
	{
		std::stringstream s;
		s << "&experiment=" << name << "&variation=" << value;
		experimentString = s.str();
	}

	const std::string& getSessionId()
	{
		return googleClientID;
	}

} // namespace GoogleAnalytics

namespace InfluxDb {

	int r = 10000; // throttle 

	std::size_t hash_value(const Point& p)
	{
		return boost::hash<std::string>()(p.name);
	}	

	void init()
	{
		r = rand() % 10000;
	}

	bool canSend(int throttleHundredthsPercentage)
	{
		return r < throttleHundredthsPercentage && !DFString::HttpInfluxURL.empty();
	}
    
    std::string escapeTag(std::string tag)
    {
        std::string s = tag;
        boost::replace_all(s, " ", "\\ ");
        boost::replace_all(s, ",", "\\,");
        return s;
    }

	using namespace RBX::Analytics::InfluxDb;

	void reportPointsV2(const std::string& resource, const boost::unordered_set<Point>& points, int throttleHundredthsPercentage, bool blocking, const std::string& userIdOverride)
	{
		if (!canSend(throttleHundredthsPercentage))
			return;

		RBXASSERT(!points.empty());

		// Pushes to InfluxDB
        std::string url = DFString::HttpInfluxURL + "/write?db=" + DFString::HttpInfluxDatabase + "&u=" + DFString::HttpInfluxUser + "&p=" + DFString::HttpInfluxPassword;

		// format: https://docs.influxdata.com/influxdb/v0.10/write_protocols/line/
		// <measurement>[,<tag-key>=<tag-value>...] <field-key>=<field-value>[,<field2-key>=<field2-value>...] [unix-nano-timestamp]

		// convert all guest IDs to -1
		std::string uid = (userIdOverride.empty() ? (!userId.empty() ? userId.c_str() : "0") : userIdOverride.c_str());
		if (atoi(uid.c_str()) < 0)
			uid = "-1";

        // tags
        std::stringstream ss;
		ss << RBX::format("%s,placeid=%s,userid=%s,reporter=%s,platform=%s,device=%s,appversion=%s,location=%s,osversion=%s ",
			resource.c_str(), 
			(!placeId.empty() ? placeId.c_str() : "0"), 
			uid.c_str(),
			escapeTag(reporter).c_str(),
			escapeTag(DebugSettings::singleton().osPlatform()).c_str(),
			escapeTag(DebugSettings::singleton().deviceName()).c_str(),
			escapeTag(appVersion).c_str(),
			escapeTag(location).c_str(),
			escapeTag(DebugSettings::singleton().osVer()).c_str());

		// fields
		boost::unordered_set<Point>::const_iterator it = points.begin();
		for (int index = 0; points.end() != it; ++it, ++index)
		{
            if (index > 0)
                ss << ',';
			ss << it->name << '=' << it->json;
		}

		postOrGet(defaultThreadPool, url, ss.str(),  "", blocking, false, true);
	}

	void reportPoints(const std::string& resource, const boost::unordered_set<Point>& points, int throttleHundredthsPercentage, bool blocking, const std::string& userIdOverride)
	{
		if (DFFlag::InfluxDb09Enabled)
		{
			reportPointsV2(resource, points, throttleHundredthsPercentage, blocking, userIdOverride);
			return;
		}

		if (!canSend(throttleHundredthsPercentage))
			return;

		RBXASSERT(!points.empty());

		// Pushes to InfluxDB.
		std::string url = DFString::HttpInfluxURL + "/db/" + DFString::HttpInfluxDatabase + "/series?u=" + DFString::HttpInfluxUser + "&p=" + Http::urlEncode(DFString::HttpInfluxPassword);

		std::string json = "[{";
		json += "\"name\":\"" + resource + "\",";
		json += "\"columns\":[\"placeid\",\"platform\",\"device\",\"osversion\",\"appversion\",\"userid\",\"reporter\",\"location\"";
		for (boost::unordered_set<Point>::const_iterator it = points.begin(); points.end() != it; ++it)
		{
			json += ",\"" + it->name + "\"";
		}
		json += "],";

		// two levels are here because we are actually sending a list of several points,
		// even though this function only handles one row of points at a time
		json += "\"points\":[[";
		json += (!placeId.empty() ? placeId : "0");
		json += ",\"" + DebugSettings::singleton().osPlatform() + "\"";
		json += ",\"" + DebugSettings::singleton().deviceName() + "\"";
		json += ",\"" + DebugSettings::singleton().osVer() + "\"";
		json += ",\"" + appVersion + "\"";

		if (!userIdOverride.empty())
			json += "," + userIdOverride;
		else
			json += "," + (!userId.empty() ? userId : "0");
		
		json += ",\"" + reporter + "\"";
		json += ",\"" + location + "\"";
		for (boost::unordered_set<Point>::const_iterator it = points.begin(); points.end() != it; ++it)
		{
			json += "," + it->json;
		}
		json += "]]";

		json += "}]";

		postOrGet(defaultThreadPool, url, json,  Http::kContentTypeApplicationJson, blocking, false, true);
	}

} // namespace InfluxDb
} // namespace ExternalAnalytics
} // namespace RBX
