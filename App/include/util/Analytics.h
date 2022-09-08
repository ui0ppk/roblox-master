#pragma once

#include <string>
#include <sstream>
#include <rapidjson/document.h>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "FastLog.h"
#include "RbxFormat.h"

DYNAMIC_FASTFLAG(InfluxDb09Enabled)

namespace RBX {

namespace Analytics {

	void setUserId(int id);
	void setPlaceId(int id);
	void setAppVersion(const std::string& version);
	void setLocation(const std::string& loc);
	void setReporter(const std::string& rep);

namespace EphemeralCounter
{
	void reportStats(const std::string& category, float value, bool blocking = false);
	void reportCountersCSV(const std::string& counterNamesCSV, bool blocking = false);
	void reportCounter(const std::string& counterName, int amount, bool blocking = false);
}

namespace GoogleAnalytics
{
	// Allow for easy initialization based on a lottery number.
	// Calls setCanUseAnalytics and init.
	void lotteryInit(const std::string &accountPropertyID, int lotteryThreshold, const std::string& productName = "", int robloxAnalyticsLottery = -1, const std::string &sessionKey = "sessionID=");

	// Must be called before using the singleton.
	void init(const std::string &accountPropertyID, const std::string& productName = "");

	bool getCanUse();
	void setCanUse();

	void sendEventRoblox(const char* category, const char* action = "custom", const char* label = "none", int value = 0, bool sync = false);

	void trackEvent(const char *category, const char *action = "custom", const char *label = "none", int value = 0, bool sync = false);
	void trackEventWithoutThrottling(const char *category, const char *action = "custom", const char *label = "none", int value = 0, bool sync = false);
	void trackUserTiming(const char *category, const char *variable, int milliseconds, const char *label = "none", bool sync = false);

	const std::string& getSessionId();

} // namespace GoogleAnalytics


namespace InfluxDb {

	struct Point
	{
		std::string name;
		std::string json;

		Point(const std::string& name_, const rapidjson::Value& value) : name(name_)
		{
			using namespace rapidjson;

			switch (value.GetType())
			{
			case kNullType:
				json = "null";
				break;
			case kFalseType:
				json = "false";
				break;
			case kTrueType:
				json = "true";
				break;
			case kObjectType:
			case kArrayType:
				throw std::runtime_error("Arrays and objects are not valid value types.");
				break;
			case kStringType:
                if (DFFlag::InfluxDb09Enabled)
                {
                    // we must escape double quotes
                    std::string sval = value.GetString();
                    boost::replace_all(sval, "\"", "\\\"");
                    json = std::string("\"") + sval + "\"";
                }
                else
                {
                    json = std::string("\"") + value.GetString() + "\"";
                }
				break;
			case kNumberType:
				{
					const rapidjson::Value& v = value;

					std::stringstream ss;
					if (v.IsDouble())
					{
						ss << v.GetDouble();
					}
					else if (v.IsInt())
					{
						ss << v.GetInt();
                        if (DFFlag::InfluxDb09Enabled)
                        {
                            ss << 'i'; // signals integer type
                        }
					}
					else if (v.IsInt64())
					{
						ss << v.GetInt64();
                        if (DFFlag::InfluxDb09Enabled)
                        {
                            ss << 'i'; // signals integer type
                        }
					}
					else if (v.IsUint())
					{
						ss << v.GetUint();
                        if (DFFlag::InfluxDb09Enabled)
                        {
                            ss << 'i'; // signals integer type
                        }
					}
					else if (v.IsUint64())
					{
						ss << v.GetUint64();
                        if (DFFlag::InfluxDb09Enabled)
                        {
                            ss << 'i'; // signals integer type
                        }
					}
					else
					{
						throw std::runtime_error("Unknown number type.");
					}

					ss >> json;
				}
				break;
			default:
				throw std::runtime_error("Unknown rapidjson value type.");
				break;
			}
		}

		bool operator==(const Point& other) const {
			return this->name == other.name;
		}
	};
	std::size_t hash_value(const Point& p);
	
	void init();
	void reportPoints(const std::string& resource, const boost::unordered_set<Point>& points, int throttleHundredthsPercentage, bool blocking = false, const std::string& userIdOverride = "");
	void reportPointsV2(const std::string& resource, const boost::unordered_set<Point>& points, int throttleHundredthsPercentage, bool blocking = false, const std::string& userIdOverride = "");

	class Points
	{
		boost::unordered_set<Point> pointList;
		std::string userIdOverride;

	public:
		Points() {}
		~Points() {}

		void setUserIdOverride(const int id)
		{
			userIdOverride = RBX::format("%d", id);
		}

		void addPoint(const std::string& name, const rapidjson::Value& value, bool override = false)
		{
			Point newPoint = Point(name, value);
			std::pair<boost::unordered_set<Point>::iterator, bool> res = pointList.insert(newPoint);

			if (override && !res.second)
			{
				pointList.erase(res.first);
				pointList.insert(newPoint);
			}
		}

		void report(const std::string& resource, int throttleHundredthsPercentage, bool blocking = false)
		{
			if (!pointList.empty())
			{
				if (DFFlag::InfluxDb09Enabled)
					reportPointsV2(resource, pointList, throttleHundredthsPercentage, blocking, userIdOverride);
				else
					reportPoints(resource, pointList, throttleHundredthsPercentage, blocking, userIdOverride);
				pointList.clear();
			}
		}

		const boost::unordered_set<Point>& getPoints() { return pointList; }
	};


} // namespace InfluxDb

} // namespace Analytics

} // namespace RBX