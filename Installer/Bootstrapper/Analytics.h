#pragma once

#include <sstream>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <boost/unordered_map.hpp>

namespace Analytics
{
	namespace InfluxDb
	{
		typedef boost::unordered_map<std::string, std::string> PointList;

		void init(const std::string& reporter, const std::string& _url, const std::string& _database, const std::string& _user, const std::string& _pw);
		void reportPoints(const std::string& resource, const PointList& points);

		class Points
		{
			PointList pointList;

		public:
			Points() {}
			~Points() {}

			void addPoint(const std::string& name, const rapidjson::Value& value)
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

				pointList[name] = jsonValue;
			}

			// blocking
			void report(const std::string& resource, bool clearPoints = false)
			{
				if (!pointList.empty())
				{
					reportPoints(resource, pointList);

					if (clearPoints)
						pointList.clear();
				}
			}

			const PointList& getPoints() { return pointList; }
		};
	}
}
