#include "PersistentDataStore.h"
#include "V8DataModel/Value.h"
#include "Script/Script.h"
#include "v8xml/xmlserializer.h"
#include "v8xml/WebSerializer.h"
#include "V8DataModel/Message.h"
#include "V8DataModel/Animation.h"
#include "V8DataModel/TextLabel.h"
#include "V8DataModel/TextButton.h"
#include "V8DataModel/TextBox.h"
#include "Network/Players.h"
#include "Util/RobloxGoogleAnalytics.h"

namespace {
	static inline void sendDataPersistenceStats(int placeId)
	{
		std::ostringstream ss;
		ss << placeId;
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DataPersistence", ss.str().c_str());
	}

}

namespace RBX { namespace Network {

static int computeLimit(const Reflection::Variant& value);


static void computeInstanceLimit(shared_ptr<Instance> instance, int* result)
{
	(*result) += instance->getPersistentDataCost();
}

static void computeValueMapLimit(const std::pair<std::string, Reflection::Variant>& pair, int* result)
{
	(*result) += computeLimit(pair.second);
}
static void computeValueCollectionLimit(const Reflection::Variant& value, int* result)
{
	(*result) += computeLimit(value);
}
static int computeLimit(const Reflection::Variant& value)
{
	if(value.isType<std::string>()){
		return Instance::computeStringCost(value.get<std::string>());
	}
	if(value.isType<shared_ptr<Instance> >()){
		int result = 0;
		computeInstanceLimit(value.get<shared_ptr<Instance> >(), &result);
		return result;
	}
	if(value.isType<shared_ptr<const RBX::Reflection::ValueMap> >()){
		int sum = 1;
		shared_ptr<const RBX::Reflection::ValueMap> valueMap = value.get<shared_ptr<const RBX::Reflection::ValueMap> >();
		std::for_each(valueMap->begin(), valueMap->end(), boost::bind(&computeValueMapLimit, _1, &sum));
		return sum;
	}
	if(value.isType<shared_ptr<const Reflection::ValueArray> >()){
		int sum = 1;
		shared_ptr<const Reflection::ValueArray> valueCollection = value.get<shared_ptr<const Reflection::ValueArray> >();
		std::for_each(valueCollection->begin(), valueCollection->end(), boost::bind(&computeValueCollectionLimit, _1, &sum));
		return sum;
	}
	
	return 1;
}

PersistentDataStore::PersistentDataStore(const Reflection::ValueMap* input, const Players* players, int complexityLimit)
	:complexity(0)
	,players(players)
	,complexityLimit(complexityLimit)
	,leaderboardDirty(false)
{
	if(input){
		valueMap = (*input);
		leaderboardDirty = true;
		std::for_each(valueMap.begin(), valueMap.end(), boost::bind(&computeValueMapLimit, _1, &complexity));

		if (valueMap.size() > 0)
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			const DataModel* dm = DataModel::get(players);
			boost::call_once(boost::bind(&sendDataPersistenceStats, dm ? dm->getPlaceID() : 0), flag);
		}
	}
	
}
bool PersistentDataStore::serializeValueMap(std::string& output, const Reflection::ValueMap& valueMap)
{
	boost::scoped_ptr<XmlElement> root(WebSerializer::writeTable(valueMap));
	if(root)
	{
		std::ostringstream stream;
		TextXmlWriter machine(stream);
		machine.serialize(root.get());
		stream.flush();

		output = stream.str();

		return true;
	}
	return false;
}

bool PersistentDataStore::saveLeaderboard(std::string& output)
{
	leaderboardDirty = false;
	Reflection::ValueMap leaderboardValueMap;
	if(players){
		for(boost::unordered_set<std::string>::const_iterator iter = players->beginLeaderboardKey(), end = players->endLeaderboardKey();
			iter != end;
			++iter)
		{
			leaderboardValueMap[*iter] = getNumber(*iter);
		}
	}
	return serializeValueMap(output, leaderboardValueMap);
}

bool PersistentDataStore::save(std::string& output)
{
	return serializeValueMap(output, valueMap);
}
void PersistentDataStore::setComplexityLimit(int value)
{
	complexityLimit = value;
}
void PersistentDataStore::removeKey(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter != valueMap.end())
		complexity -= computeLimit(iter->second);
	valueMap.erase(key);
}
bool PersistentDataStore::enforceComplexity(const std::string& key)
{
	int addedComplexity = computeLimit(valueMap[key]);

	if(addedComplexity + complexity <= complexityLimit){
		complexity += addedComplexity;
		return true;
	}
	else{
		valueMap.erase(key);
		return false;
	}
}


bool PersistentDataStore::isNumber(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter != valueMap.end() && !iter->second.isType<double>())
		return false;
	return true;
}
double PersistentDataStore::getNumber(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<double>())
		return 0.0f;
	return iter->second.get<double>();
}
bool PersistentDataStore::setNumber(const std::string& key, double value)
{
	if(isNumber(key) && getNumber(key) == value){
		return true;
	}

	if(players->hasLeaderboardKey(key))
		leaderboardDirty = true;
	
	removeKey(key);

	if(value == 0)
		return true;

	valueMap[key] = value;

	return enforceComplexity(key);
}
std::string PersistentDataStore::getString(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<std::string>())
		return "";
	return iter->second.get<std::string>();
}
bool PersistentDataStore::setString(const std::string& key, const std::string& value)
{
	removeKey(key);

	if(value == "")
		return true;

	valueMap[key] = value;

	return enforceComplexity(key);
}
bool PersistentDataStore::getBoolean(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<bool>())
		return false;
	return iter->second.get<bool>();
}
bool PersistentDataStore::setBoolean(const std::string& key, bool value)
{	
	removeKey(key);

	if (value == false)
		return true;

	valueMap[key] = value;

	return enforceComplexity(key);
}
shared_ptr<Instance> PersistentDataStore::getInstance(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<shared_ptr<Instance> >())
		return shared_ptr<Instance>();

	return iter->second.get<shared_ptr<Instance> >()->clone(SerializationCreator);
}

bool PersistentDataStore::setInstance(const std::string& key, shared_ptr<Instance> value)
{
	removeKey(key);

	if(!value)
		return true;
	
	shared_ptr<Instance> clonedInstance = value->clone(SerializationCreator);
	if(clonedInstance)
		valueMap[key] = clonedInstance;
	
	return enforceComplexity(key);
}

shared_ptr<const Reflection::ValueArray> PersistentDataStore::getList(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<shared_ptr<const Reflection::ValueArray> >())
		return shared_ptr<const Reflection::ValueArray>();
	return iter->second.get<shared_ptr<const Reflection::ValueArray> >();
}

bool PersistentDataStore::setList(const std::string& key, shared_ptr<const Reflection::ValueArray> value)
{
	removeKey(key);

	if(!value || value->size() == 0)
		return true;
	
	valueMap[key] = value;

	return enforceComplexity(key);
}
shared_ptr<const RBX::Reflection::ValueMap> PersistentDataStore::getTable(const std::string& key)
{
	Reflection::ValueMap::iterator iter = valueMap.find(key);
	if(iter == valueMap.end() || !iter->second.isType<shared_ptr<const RBX::Reflection::ValueMap> >())
		return shared_ptr<const RBX::Reflection::ValueMap>();
	return iter->second.get<shared_ptr<const RBX::Reflection::ValueMap> >();
}


bool PersistentDataStore::setTable(const std::string& key, shared_ptr<const RBX::Reflection::ValueMap> value)
{
	removeKey(key);

	if(!value || value->empty())
		return true;

	valueMap[key] = value;

	return enforceComplexity(key);
}

}}