#pragma once

#include <map>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

class InfluxDb
{
    typedef std::map<std::string, std::string> PointList;
	PointList points;

public:
	InfluxDb(){}
	~InfluxDb(){}

	static void init(const std::string& reporter, const std::string& _url, const std::string& _database, const std::string& _user, const std::string& _pw);
	static const std::string& getUrlHost();
	static void setLocation(const std::string& loc);
	static void setAppVersion(const std::string& ver);

	static const std::string& getUrlPath();
	static const std::string& getReportingUrl();

	std::string getJsonStringForPosting(const std::string& seriesName);
	void addPoint(const std::string& name, const rapidjson::Value& value);
};

