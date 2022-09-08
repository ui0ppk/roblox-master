#pragma once

#include "v8tree/instance.h"
#include <boost/noncopyable.hpp>
namespace RBX
{
namespace Network
{
	class Players;

	class PersistentDataStore:
		boost::noncopyable
	{
	private:
		Reflection::ValueMap valueMap;
		const Players* players;

		bool leaderboardDirty;
		int complexity;
		int complexityLimit;
	
		void removeKey(const std::string& key);
		bool enforceComplexity(const std::string& key);
		
		static bool serializeValueMap(std::string& output, const Reflection::ValueMap& valueMap);

		bool isNumber(const std::string& key);
	public:
		PersistentDataStore(const Reflection::ValueMap* input, const Players* players, int complexityLimit);

		bool empty() const { return valueMap.empty(); }
		bool isLeaderboardDirty() const { return leaderboardDirty; }

		bool save(std::string& output);
		bool saveLeaderboard(std::string& output);

		int getComplexity() const	   { return complexity;}
		int getComplexityLimit() const { return complexityLimit;}
		void setComplexityLimit(int value);

		float getLeaderboard(const std::string& key);
		bool setLeaderboard(const std::string& key, float value);

		double getNumber(const std::string& key);
		bool setNumber(const std::string& key, double value);
		std::string getString(const std::string& key);
		bool setString(const std::string& key, const std::string& value);
		bool getBoolean(const std::string& key);
		bool setBoolean(const std::string& key, bool value);
		shared_ptr<Instance> getInstance(const std::string& key);
		bool setInstance(const std::string&, shared_ptr<Instance> value);

		shared_ptr<const Reflection::ValueArray> getList(const std::string& key);
		bool setList(const std::string& , shared_ptr<const Reflection::ValueArray> value);
		shared_ptr<const RBX::Reflection::ValueMap> getTable(const std::string& key);
		bool setTable(const std::string& , shared_ptr<const RBX::Reflection::ValueMap> value);
	};
}
}