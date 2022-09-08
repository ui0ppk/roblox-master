#include "stdafx.h"

#include <sstream>
#include <stdexcept>

#include "InfluxDbHelper.h"
#include "format_string.h"

// appended to all reports
std::string reporter;
std::string location;
std::string appVersion;

std::string hostName;
std::string database;
std::string user;
std::string password;
bool canSend = false;
std::string url;
std::string path;

void InfluxDb::init(const std::string& _reporter, const std::string& _url, const std::string& _database, const std::string& _user, const std::string& _pw)
{
	reporter = _reporter;
	hostName = _url;
	database = _database;
	user = _user;
	password = _pw;
	canSend = true;

	if (!database.empty() && !user.empty() && !password.empty())
		path = "/db/" + database + "/series?u=" + user + "&p=" + password;

	if (!hostName.empty() && !path.empty())
		url = hostName + path;
}

void InfluxDb::setLocation(const std::string& loc)
{
	location = loc;
}

void InfluxDb::setAppVersion(const std::string& ver)
{
	appVersion = ver;
}

const std::string& InfluxDb::getReportingUrl()
{
	return url;
}

const std::string& InfluxDb::getUrlHost()
{
	return hostName;
}

const std::string& InfluxDb::getUrlPath()
{
	return path;
}

void InfluxDb::addPoint(const std::string& name, const rapidjson::Value& value)
{
	using namespace rapidjson;
	std::string jsonValue;

	switch (value.GetType())
	{
	case kNullType:
		jsonValue = "null";
		break;
	case kFalseType:
		jsonValue = "false";
		break;
	case kTrueType:
		jsonValue = "true";
		break;
	case kObjectType:
	case kArrayType:
		throw std::runtime_error("Arrays and objects are not valid value types.");
		break;
	case kStringType:
		jsonValue = std::string("\"") + value.GetString() + "\"";
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
			}
			else if (v.IsInt64())
			{
				ss << v.GetInt64();
			}
			else if (v.IsUint())
			{
				ss << v.GetUint();
			}
			else if (v.IsUint64())
			{
				ss << v.GetUint64();
			}
			else
			{
				throw std::runtime_error("Unknown number type.");
			}

			ss >> jsonValue;
		}
		break;
	default:
		throw std::runtime_error("Unknown rapidjson value type.");
		break;
	}

	points[name] = jsonValue;
}

std::string InfluxDb::getJsonStringForPosting(const std::string& seriesName)
{
	std::string json = "[{";
	json += "\"name\":\"" + seriesName + "\",";
	json += "\"columns\":[\"reporter\",\"version\",\"location\"";
	for (PointList::const_iterator it = points.begin(); points.end() != it; ++it)
	{
		json += ",\"" + it->first + "\"";
	}
	json += "],";

	// two levels are here because we are actually sending a list of several points,
	// even though this function only handles one row of points at a time
	json += "\"points\":[[";
	json += "\"" +  reporter + "\"";
	json += ",\"" +  appVersion + "\"";
	json += ",\"" +  location + "\"";
	for (PointList::const_iterator it = points.begin(); points.end() != it; ++it)
	{
		json += "," + it->second;
	}
	json += "]]";

	json += "}]";

	return json;
}


