#include "stdafx.h"

#include "v8datamodel/DataStore.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DataStoreService.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Stats.h"
#include "util/LuaWebService.h"
#include "util/standardout.h"
#include "v8xml/WebParser.h"

#include "RobloxServicesTools.h"

LOGVARIABLE(DataStore, 0);
DYNAMIC_FASTINTVARIABLE(DataStoreMaxKeysToFetch, 100); 
DYNAMIC_FASTINTVARIABLE(DataStoreKeyLengthLimit, 50);
DYNAMIC_FASTINTVARIABLE(DataStoreMaxPageSize, 100);

DYNAMIC_FASTINTVARIABLE(DataStoreMaxValueSize, 64*1024);

DYNAMIC_FASTINTVARIABLE(DataStoreTouchTimeoutInSeconds, 5);

DYNAMIC_FASTINTVARIABLE(DataStoreSameKeyPerMinute, 10);

DYNAMIC_FASTFLAGVARIABLE(UseNewPersistenceSubdomain, true);

namespace RBX {
	RBX_REGISTER_CLASS(DataStore);
	RBX_REGISTER_CLASS(OrderedDataStore);
	RBX_REGISTER_CLASS(DataStorePages);

	const char* const sGlobalDataStore = "GlobalDataStore";
	
    REFLECTION_BEGIN();
	static Reflection::BoundYieldFuncDesc<DataStore, Reflection::Variant(std::string)> func_getAsync(&DataStore::getAsync, "GetAsync", "key", Security::None);
	static Reflection::BoundYieldFuncDesc<DataStore, void(std::string, Reflection::Variant)> func_setAsync(&DataStore::setAsync, "SetAsync", "key", "value", Security::None);
	static Reflection::BoundYieldFuncDesc<DataStore, shared_ptr<const Reflection::Tuple>(std::string, Lua::WeakFunctionRef)>
		func_updateAsync(&DataStore::updateAsync, "UpdateAsync", "key", "transformFunction", Security::None);
	static Reflection::BoundYieldFuncDesc<DataStore, Reflection::Variant(std::string, int)> 
		func_incrementAsync(&DataStore::incrementAsync, "IncrementAsync", "key", "delta", 1, Security::None);
	static Reflection::BoundFuncDesc<DataStore, rbx::signals::connection(std::string, Lua::WeakFunctionRef)> func_onUpdate(&DataStore::onUpdate, "OnUpdate", "key", "callback", Security::None);

 	static Reflection::BoundYieldFuncDesc<OrderedDataStore, shared_ptr<Instance>(bool, int, Reflection::Variant, Reflection::Variant)> 
 		func_getSortedAsync(&OrderedDataStore::getSortedAsync, "GetSortedAsync", "ascending", "pagesize", "minValue", Reflection::Variant(), "maxValue", Reflection::Variant(), Security::None);
    REFLECTION_END();

	namespace Lua {
		shared_ptr<Reflection::Tuple> callCallback(Lua::WeakFunctionRef function, shared_ptr<const Reflection::Tuple> args, boost::intrusive_ptr<WeakThreadRef> cachedCallbackThread);

	}
	
	void DataStore::CachedRecord::update(const Reflection::Variant& variant, const std::string& serialized)
	{
		this->variant = variant;
		this->serialized = serialized;
	}

	const Reflection::Variant& DataStore::CachedRecord::getVariant(bool touch /* = true */)
	{
		if(touch)
			accessTimeStamp = Time::nowFast();

		return variant;
	}

	std::string DataStore::serializeVariant(const Reflection::Variant& variant, bool* hasNonJsonType)
	{
		std::string result;
		bool parseSuccess = WebParser::writeJSON(variant,result, WebParser::FailOnNonJSON);

		if (hasNonJsonType)
			*hasNonJsonType = !parseSuccess;

		return result;
	}

	bool DataStore::checkAccess(const std::string& key, boost::function<void(std::string)>* errorFunction)
	{
		if (key.size() == 0)
		{
			if (errorFunction)
				(*errorFunction)("Key name can't be empty");
			return false;
		}

		if (key.size() >= (unsigned)DFInt::DataStoreKeyLengthLimit)
		{
			if (errorFunction)
				(*errorFunction)("Key name is too long");
			return false;
		}

		return true;
	}

	bool DataStore::checkStudioApiAccess(boost::function<void(std::string)> errorFunction)
	{
		DataModel* dm = DataModel::get(this);
		RBXASSERT(dm);
		if (dm)
			if (LuaWebService* lws = dm->create<LuaWebService>())
				if (!lws->isApiAccessEnabled())
				{
					if (errorFunction)
						errorFunction("Cannot write to DataStore from studio if API access is not enabled. Enable it by going to the Game Settings page.");
					return false;
				}
		return true;
	}
	
	DataStore::DataStore(const std::string& name, const std::string& scope, bool legacy ) :
		name(name),
		scope(scope),
		isLegacy(legacy),
		backendProcessing(false),
		nextKeyToRefetch(""),
		refetchState(RefetchOnUpdateKeys)
	{
  	}
    
    void DataStore::runResumeFunction(std::string key, boost::function<void(Reflection::Variant)> resumeFunction)
    {
        FASTLOGS(FLog::DataStore, "Returning fetched value for key %s", key);
        resumeFunction(cachedKeys[key].getVariant());
    }

	DataStoreService* DataStore::getParentDataStoreService()
	{
		return fastDynamicCast<DataStoreService>(this->getParent());
	}

	void DataStore::getAsync(std::string key, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!checkAccess(key, &errorFunction))
			return;

		FASTLOGS(FLog::DataStore, "GetAsync on key %s", key);
		CachedKeys::iterator it = cachedKeys.find(key);

		Time now = Time::nowFast();

		if (it != cachedKeys.end() && (now - it->second.getTime()).seconds() < DFInt::DataStoreTouchTimeoutInSeconds)
		{
			FASTLOGS(FLog::DataStore, "Got cached version, returning %s", it->second.getSerialized());

			getParentDataStoreService()->reportCachedRequestGet();
			
			resumeFunction(it->second.getVariant());
			return;
		}

		DataStoreService::HttpRequest request;
		createFetchNewKeyRequest(key, boost::bind(&DataStore::runResumeFunction, shared_from(this), key, resumeFunction), errorFunction, request);
		request.requestStartTime = boost::posix_time::microsec_clock::local_time();
		request.requestType = DataStoreService::HttpRequest::RequestType::GET_ASYNC;
		if (!DataStoreService::queueOrExecuteGet(this, request))
		{
			errorFunction("Request limit exceeded on get");
		}
	}

    void logLongValue(const std::string& value)
    {
        FASTLOGS(FLog::DataStore, "Value: %s", value);
        if(value.length() > 200)
        {
            std::string tail = value.substr(value.size() - 200);
            FASTLOGS(FLog::DataStore, "Value end: %s", tail);
            FASTLOG1(FLog::DataStore, "Value length: %u", value.length());
        }
    }

	void DataStore::setAsync(std::string key, Reflection::Variant value, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!checkAccess(key, &errorFunction))
			return;

		if (!checkValueIsAllowed(value))
		{
			errorFunction(value.type().tag.str + " is not allowed in DataStore");
			return;
		}
		
		bool hasNonJsonType;
		std::string v = serializeVariant(value, &hasNonJsonType);
		if (hasNonJsonType)
		{
			errorFunction("Cannot store " + value.type().tag.str + " in DataStore");
			return;
		}

		if((int)v.size() >= DFInt::DataStoreMaxValueSize)
		{
			errorFunction("Value is too large");
			return;
		}

		if (!checkStudioApiAccess(errorFunction))
			return;

		DataStoreService::HttpRequest request;
		request.url = constructSetUrl(key, v.length());
		request.owner = shared_from(this);
		request.key = key;
		request.requestStartTime = boost::posix_time::microsec_clock::local_time();
		request.requestType = DataStoreService::HttpRequest::RequestType::SET_ASYNC;

		std::stringstream postData;
		postData << "value=";
		postData << RBX::Http::urlEncode(v);
		request.postData = postData.str();

		FASTLOGS(FLog::DataStore, "SetAsync on key: %s", key);
		logLongValue(v);
		FASTLOG(FLog::DataStore, "Url encoded:");
		logLongValue(request.postData);

		request.handler = boost::bind(&DataStore::processFetchSingleKey, shared_from(this), _1, _2, key, /*expectSubKey*/ false, resumeFunction, errorFunction);

		if (!queueOrExecuteSet(request))
		{
			errorFunction("Request limit exceeded on set");
		}
	}

	void DataStore::incrementAsync(std::string key, int delta, 
		boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!checkAccess(key, &errorFunction))
			return;

		if (!checkStudioApiAccess(errorFunction))
			return;

		boost::function<void()> callback = boost::bind(&DataStore::runResumeFunction, shared_from(this), key, resumeFunction);

		DataStoreService::HttpRequest request;
		request.url = constructIncrementUrl(key, delta);
		request.owner = shared_from(this);
		request.key = key;
		request.handler = boost::bind(&DataStore::processFetchSingleKey, shared_from(this), _1, _2, key, /*expectSubKey*/ false, callback, errorFunction);
		request.requestStartTime = boost::posix_time::microsec_clock::local_time();
		request.requestType = DataStoreService::HttpRequest::RequestType::INCREMENT_ASYNC;

		if (!queueOrExecuteSet(request))
		{
			errorFunction("Request limit exceeded on increment");
		}
	}

	void DataStore::updateAsync(std::string key, Lua::WeakFunctionRef transformFunc, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!checkAccess(key, &errorFunction))
			return;

		FASTLOGS(FLog::DataStore, "Updating key %s", key);

		shared_ptr<Lua::WeakFunctionRef> transform = rbx::make_shared<Lua::WeakFunctionRef>(transformFunc);

		CachedKeys::iterator it = cachedKeys.find(key);
		if (it == cachedKeys.end())
		{
			DataStoreService::HttpRequest request;
			createFetchNewKeyRequest(key, boost::bind(&DataStore::runTransformFunction, shared_from(this), key, transform, resumeFunction, errorFunction), errorFunction,
				request);
			request.requestStartTime = boost::posix_time::microsec_clock::local_time();
			request.requestType = DataStoreService::HttpRequest::RequestType::GET_ASYNC;

			if (!DataStoreService::queueOrExecuteGet(this, request))
			{
				errorFunction("Request limit exceeded on update");
			}
			return;
		}

		if (!checkStudioApiAccess(errorFunction))
			return;

		runTransformFunction(key, transform, resumeFunction, errorFunction);
	}

	void DataStore::runTransformFunction(std::string key, shared_ptr<Lua::WeakFunctionRef> transform, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		CachedKeys::iterator it = cachedKeys.find(key);
		RBXASSERT(it != cachedKeys.end());

		if (DataModel::get(this) == NULL) {
			FASTLOG(FLog::DataStore, "Data model is destroyed, cancel transform");
			return;
		}

		shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
		args->values.push_back(it->second.getVariant());

		FASTLOGS(FLog::DataStore, "Running transform function, input: %s", it->second.getSerialized());

		shared_ptr<Reflection::Tuple> result;
		try 
		{
			result = Lua::callCallback(*transform, args, new Lua::WeakThreadRef());
		}
		catch(const std::runtime_error& e)
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, e.what());
		}

		if (result.get() == NULL || result->values.size() == 0 || result->at(0).isVoid()) {
			FASTLOG(FLog::DataStore, "Transform function returned nil, update is cancelled");
			resumeFunction(result);
			return;
		}

		if(!checkValueIsAllowed(result->at(0)))
		{
			errorFunction(result->at(0).type().tag.str + " is not allowed in DataStore");
			return;
		}

		bool hasNonJsonType;
		std::string newValue = serializeVariant(result->at(0), &hasNonJsonType);
		if (hasNonJsonType)
		{
			errorFunction("Cannot store " + result->at(0).type().tag.str + " in DataStore");
			return;
		}

		if((int)newValue.size() > DFInt::DataStoreMaxValueSize)
		{
			errorFunction("Value is too large");
			return;
		}

		const std::string& expectedValue = it->second.getVariant(false).isVoid() ? "" : it->second.getSerialized();

		std::stringstream postData;
		postData << "value=";
		postData << RBX::Http::urlEncode(newValue);
		postData << "&expectedValue=";
		postData << RBX::Http::urlEncode(expectedValue);

		std::string postDataFinal = postData.str();

		DataStoreService::HttpRequest request;
		request.url = constructSetIfUrl(key, newValue.length(), expectedValue.length());
		request.postData = postData.str();
		request.owner = shared_from(this);
		request.key = key;
		request.requestStartTime = boost::posix_time::microsec_clock::local_time();
		request.requestType = DataStoreService::HttpRequest::RequestType::UPDATE_ASYNC;
		request.handler = boost::bind(&DataStore::processSetIf, shared_from(this), key, transform, _1, _2, resumeFunction, errorFunction);

		FASTLOGS(FLog::DataStore, "SetIf on key: %s", key);
		logLongValue(newValue);
		FASTLOG(FLog::DataStore, "Url encoded:");
		logLongValue(request.postData);
		if (!queueOrExecuteSet(request))
		{
			errorFunction("Request limit exceeded on transform");
		}
	}

	void DataStore::processSetIf(std::string key, shared_ptr<Lua::WeakFunctionRef> transform, std::string* response, std::exception* exception,  boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (response)
			FASTLOGS(FLog::DataStore, "SetIf returned %s", *response);

		DataModel::processHttpRequestResponseOnLock(
			DataModel::get(this),
			response, 
			exception,
			boost::bind(&DataStore::lockAcquiredProcessSetIf,shared_from(this),key,transform,_1,_2,resumeFunction,errorFunction));
	}

	void DataStore::lockAcquiredProcessSetIf(std::string key, shared_ptr<Lua::WeakFunctionRef> transform, shared_ptr<std::string> response, shared_ptr<std::exception> exception,  boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;
		if (!LuaWebService::parseWebJSONResponseHelper(response.get(), exception.get(), result, status))
		{
			errorFunction(status);
			return;
		}

		Reflection::ValueTable::const_iterator itError = result->find("error");
		if (itError == result->end())
		{
			Reflection::ValueTable::const_iterator itData = result->find("data");
			if (itData == result->end())
			{
				RBXASSERT(0);
				errorFunction("Can't parse response");
				return;
			}

			FASTLOG(FLog::DataStore, "Our change won, lets update final value");
			if (!updateCachedKey(key, itData->second)) {
				RBXASSERT(0);
				errorFunction("Can't parse response");
				return;
			}

			shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
			args->values.push_back(cachedKeys[key].getVariant());
			resumeFunction(args);
			return;
		}
		else 
		{
			Reflection::ValueTable::const_iterator itCurrentValue = result->find("currentValue");
			if (itCurrentValue == result->end())
			{
				RBXASSERT(0);
				errorFunction("Can't parse response");
				return;
			}

			// Concurrent modification where "our change" is rejected.
			// Use recieved value as "old", and re-apply our transformation.
			FASTLOG(FLog::DataStore, "Our change is rejected, update local value");
			updateCachedKey(key, itCurrentValue->second);
			runTransformFunction(key, transform, resumeFunction, errorFunction);
		}
	}
    
    bool DataStore::queueOrExecuteSet(DataStoreService::HttpRequest& request)
    {
        return DataStoreService::queueOrExecuteSet(this, request);
    }

	void DataStore::processFetchSingleKey(std::string* response, std::exception* exception, std::string key, bool expectSubKey, 
		boost::function<void()> callback, boost::function<void(std::string)> errorFunction)
	{
		FASTLOGS(FLog::DataStore, "Fetched key %s from the service", key);
		
		DataModel::processHttpRequestResponseOnLock(
			DataModel::get(this),
			response, 
			exception, 
			boost::bind(&DataStore::lockAcquiredProcessFetchSingleKey,shared_from(this),_1,_2,key,expectSubKey,callback,errorFunction));
	}

	void DataStore::lockAcquiredProcessFetchSingleKey(shared_ptr<std::string> response, shared_ptr<std::exception> exception, std::string key, bool expectSubKey, boost::function<void()> callback, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;
		if (!LuaWebService::parseWebJSONResponseHelper(response.get(), exception.get(), result, status))
		{
			FASTLOGS(FLog::DataStore, "Failed to parse: %s", response ? *response : "Null string");
			errorFunction(status);
			return;
		}

		Reflection::ValueTable::const_iterator itError = result->find("error");
		Reflection::ValueTable::const_iterator itData = result->find("data");
		if (itError != result->end())
		{
			const Reflection::Variant errorValue = itError->second;
			std::string errorMessage = errorValue.isString() ? errorValue.cast<std::string>() : "Failed to retrieve key";
			FASTLOGS(FLog::DataStore, "Failed, error message: %s", errorMessage);
			errorFunction("Request rejected");
			return;
		}

		if (itData == result->end())
		{
			errorFunction("Failed to retrieve key");
			return;
		}
		
		Reflection::Variant value;
		if (expectSubKey)
		{
			// Expected format is:
			//	{ "data" : 
			//		[
			//			{	"Value" : value,
			//				"Scope" : scope,							
			//				"Key" : key,
			//				"Target" : target
			//			}
			//		]
			//	}
			// or for non-existing key:
			// { "data": [] }

			if (itData->second.isType<shared_ptr<const Reflection::ValueArray> >())
			{
				shared_ptr<const Reflection::ValueArray> subArray = 
					itData->second.cast<shared_ptr<const Reflection::ValueArray> >();

				if (subArray->size() != 0)
				{
					Reflection::ValueArray::const_iterator itEntry = subArray->begin();
					if(!itEntry->isType<shared_ptr<const Reflection::ValueTable> > ())
					{
						errorFunction("Unexpected value of entry");
						return;
					}

					shared_ptr<const Reflection::ValueTable> keyValueTable = 
						itEntry->cast<shared_ptr<const Reflection::ValueTable> >();

					Reflection::ValueTable::const_iterator itValue = keyValueTable->find("Value");

					if(itValue == keyValueTable->end())
					{
						errorFunction("Unexpected of entry");
						return;
					}
					
					value = itValue->second;
				}
				else
				{
					value = Reflection::Variant();
				}
			} 
			else 
			{
				errorFunction("Unexpected format");
				return;
			}
		}
		else
		{
			// Expected format is:
			//	{ "data" : value }

			value = itData->second;
		}

		updateCachedKey(key, value);
		callback();
	}


	bool DataStore::deserializeVariant(const std::string& webValue, Reflection::Variant& result)
	{
		result = Reflection::Variant();
		if (webValue.size() == 0)
			return true;

		std::stringstream jsonStream;
		jsonStream << "{ \"data\": ";
		jsonStream << webValue;
		jsonStream << "}";

		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());

		bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
		if (!parseResult)
		{
			RBXASSERT(0);
			return false;
		}

		Reflection::ValueTable::const_iterator itData = jsonResult->find("data");
		if (itData == jsonResult->end())
		{
			RBXASSERT(0);
			return false;
		}

		result = itData->second;
		return true;
	}

	bool DataStore::updateCachedKey(const std::string& key, const Reflection::Variant& rawValue)
	{
		Reflection::Variant value;
		std::string serializedValue;

		if (rawValue.isString())
		{
			serializedValue = rawValue.get<std::string>();
			FASTLOGS(FLog::DataStore, "Updating based on web string for key %s", key);
			logLongValue(serializedValue);
			if (!deserializeVariant(serializedValue, value)) {
				FASTLOG(FLog::DataStore, "Can't decode returned value");
				return false;
			}
		}
		else
		{
			value = rawValue;
			bool hasNonJsonType = false;
			serializedValue = serializeVariant(value, &hasNonJsonType);
			RBXASSERT(hasNonJsonType == false);
		}

		OnUpdateKeys::iterator itSignal = onUpdateKeys.find(key);
		CachedKeys::iterator itCached = cachedKeys.find(key);

		if (itSignal == onUpdateKeys.end() || itCached == cachedKeys.end())
		{
			FASTLOGS(FLog::DataStore, "Key is not cached, can just store it directly: %s", serializedValue);
			cachedKeys[key].update(value, serializedValue);

			if (itSignal != onUpdateKeys.end())
			{
				FASTLOGS(FLog::DataStore, "Triggering callback: %s", serializedValue);
				(*itSignal->second)(value);
			}

			return true;
		}

		if (serializedValue == itCached->second.getSerialized())
			return true;

		FASTLOGS(FLog::DataStore, "Updating value and triggering: %s", serializedValue);

		itCached->second.update(value, serializedValue);
		(*itSignal->second)(value);

		return true;
	}

	std::string DataStore::urlEncodeIfNeeded(const std::string& input)
	{
		DataStoreService* dsService = getParentDataStoreService();
		RBXASSERT(dsService);
		if(dsService && !dsService->isUrlEncodingDisabled())
		{
			return RBX::Http::urlEncode(input);
		}
		return input;
	}

	
	void DataStore::createFetchNewKeyRequest(
			const std::string& key, 
			boost::function<void()> callback, 
			boost::function<void(std::string)> errorFunction, 
			DataStoreService::HttpRequest& request)
	{
		request.url = constructGetUrl();
		request.postData = constructPostDataForKey(key);
		request.handler = boost::bind(&DataStore::processFetchSingleKey, shared_from(this), _1, _2, key, /*expectSubKey*/ true, callback, errorFunction);
		request.owner = shared_from(this);
	}

	std::string DataStore::constructPostDataForKey(const std::string& key, unsigned index)
	{
		return isLegacy ? 
			format("&qkeys[%u].scope=%s&qkeys[%u].target=&qkeys[%u].key=%s", 
			index, scopeUrlEncodedIfNeeded.c_str(), index, index, urlEncodeIfNeeded(key).c_str()) :
			format("&qkeys[%u].scope=%s&qkeys[%u].target=%s&qkeys[%u].key=%s", 
			index, scopeUrlEncodedIfNeeded.c_str(), index, urlEncodeIfNeeded(key).c_str(), index, nameUrlEncodedIfNeeded.c_str());
	}

	std::string DataStore::constructGetUrl()
	{
		int placeId = DataModel::get(this)->getPlaceID();
		return format("%sgetV2?placeId=%i&type=%s&scope=%s", serviceUrl.c_str(), placeId, getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str());
	}

	std::string DataStore::constructSetUrl(const std::string& key, unsigned valueLength)
	{
		int placeId = DataModel::get(this)->getPlaceID();
		return isLegacy ? 
			format("%sset?placeId=%i&key=%s&&type=%s&scope=%s&target=&valueLength=%u", 
			serviceUrl.c_str(), placeId, urlEncodeIfNeeded(key).c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), valueLength) :
			format("%sset?placeId=%i&key=%s&&type=%s&scope=%s&target=%s&valueLength=%u", 
			serviceUrl.c_str(), placeId, nameUrlEncodedIfNeeded.c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), urlEncodeIfNeeded(key).c_str(), valueLength);
	}

	std::string DataStore::constructSetIfUrl(const std::string& key, unsigned valueLength, unsigned expectedValueLength)
	{
		int placeId = DataModel::get(this)->getPlaceID();
		return isLegacy ? 
			format("%sset?placeId=%i&key=%s&type=%s&scope=%s&target=&valueLength=%u&expectedValueLength=%u", 
				serviceUrl.c_str(), placeId, urlEncodeIfNeeded(key).c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), valueLength, expectedValueLength):
			format("%sset?placeId=%i&key=%s&type=%s&scope=%s&target=%s&valueLength=%u&expectedValueLength=%u", 
				serviceUrl.c_str(), placeId, nameUrlEncodedIfNeeded.c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), urlEncodeIfNeeded(key).c_str(), valueLength, expectedValueLength);
	}

	std::string DataStore::constructIncrementUrl(const std::string& key, int delta)
	{
		int placeId = DataModel::get(this)->getPlaceID();
		return isLegacy ? 
			format("%sincrement?placeId=%i&key=%s&type=%s&scope=%s&target=&value=%i", 
				serviceUrl.c_str(), placeId, urlEncodeIfNeeded(key).c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), delta) :
			format("%sincrement?placeId=%i&key=%s&type=%s&scope=%s&target=%s&value=%i", 
				serviceUrl.c_str(), placeId, nameUrlEncodedIfNeeded.c_str(), getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), urlEncodeIfNeeded(key).c_str(), delta);
	}

	
	static void dummyAction()
	{
	}

	static void dummyError(std::string)
	{
	}

	DataStore::EventSlot::EventSlot(Lua::WeakFunctionRef callback) : callback(callback)
	{
	}

	void DataStore::EventSlot::fire(Reflection::Variant value)
	{
		if (value.isVoid())
			return;

		try
		{
			shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();
			args->values.push_back(value);
			callCallback(callback, args, new Lua::WeakThreadRef());
		}
		catch(const std::runtime_error& error)
		{
			// TODO: Disconnect?
			StandardOut::singleton()->print(MESSAGE_ERROR, error.what());
		}
	}


	rbx::signals::connection DataStore::onUpdate(std::string key, Lua::WeakFunctionRef callback)
	{
		if (!checkAccess(key, NULL))
			return rbx::signals::connection();

		FASTLOGS(FLog::DataStore, "Subscribed to key %s", key);
		shared_ptr<rbx::signal<void(Reflection::Variant) > >& signal = onUpdateKeys[key];
		if (!signal)
			signal = rbx::make_shared<rbx::signal<void(Reflection::Variant)> >();

		shared_ptr<EventSlot> slot = rbx::make_shared<EventSlot>(callback);
		rbx::signals::connection conn = signal->connect(
			boost::bind(&DataStore::EventSlot::fire, slot, _1));

		if (cachedKeys.find(key) == cachedKeys.end())
		{
			DataStoreService::HttpRequest request;
			createFetchNewKeyRequest(key, dummyAction, dummyError, request);
			request.requestType = DataStoreService::HttpRequest::RequestType::GET_ASYNC;
			request.execute(getParentDataStoreService());
		}

		return conn;
	}

	void DataStore::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) 
	{
		Super::onServiceProvider(oldProvider, newProvider);

		if (newProvider)
		{
			ContentProvider* cp = newProvider->find<ContentProvider>();
			
            if (cp)
            {
                if (DFFlag::UseNewPersistenceSubdomain)
                {
                    serviceUrl = BuildGenericPersistenceUrl(cp->getBaseUrl(), urlApiPath());
                }
                else
                {
                    //https://api.gametest1.robloxlabs.com//persistence/set?placeId=124921244&key=BF2%5Fds%5Ftest&&type=standard&scope=global&target=BF2%5Fds%5Fkey%5Ftmp&valueLength=31
                    serviceUrl = cp->getApiBaseUrl() + urlApiPath() + '/';
                }
                
            }
            
			FASTLOGS(FLog::DataStore, "Initialized Data Store, url: %s", serviceUrl);

			scopeUrlEncodedIfNeeded = urlEncodeIfNeeded(scope);
			nameUrlEncodedIfNeeded = urlEncodeIfNeeded(name);
		}
	}

	void DataStore::processFetchCachedKeys(std::string* response, std::exception* exception)
	{
		FASTLOG(FLog::DataStore, "Fetched keys from the service");

		DataModel::processHttpRequestResponseOnLock(
			DataModel::get(this),
			response, 
			exception, 
			boost::bind(&DataStore::lockAcquiredProcessFetchCachedKeys,shared_from(this),_1,_2));

	}

	void DataStore::lockAcquiredProcessFetchCachedKeys(shared_ptr<std::string> response, shared_ptr<std::exception> exception)
	{
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;
		if (!LuaWebService::parseWebJSONResponseHelper(response.get(), exception.get(), result, status))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, status);
			return;
		}

		Reflection::ValueTable::const_iterator itError = result->find("error");
		Reflection::ValueTable::const_iterator itData = result->find("data");
		if (itError != result->end())
		{
			const Reflection::Variant errorValue = itError->second;
			std::string errorMessage = errorValue.isType<std::string>() ? 
				errorValue.cast<std::string>() : "Failed to retrieve key";
			FASTLOGS(FLog::DataStore, "Failed, error message: %s", errorMessage);
			return;
		}

		if (itData == result->end())
		{
			FASTLOG(FLog::DataStore, "Can't find data in the response");
			RBXASSERT(false);
			return;
		}

		if (!itData->second.isType<shared_ptr<const Reflection::ValueArray> >())
		{
			FASTLOG(FLog::DataStore, "No keys updated");
			return;
		}

		shared_ptr<const Reflection::ValueArray> keyValueArray = 
			itData->second.cast<shared_ptr<const RBX::Reflection::ValueArray> >();

		for (Reflection::ValueArray::const_iterator it = keyValueArray->begin(); it != keyValueArray->end(); ++it)
		{
			if(!it->isType<shared_ptr<const Reflection::ValueTable> >()) {
				FASTLOG(FLog::DataStore, "Unexpected member of the array");
				continue;
			}

			shared_ptr<const Reflection::ValueTable> keyValueEntry = 
				it->cast<shared_ptr<const RBX::Reflection::ValueTable> >();

			Reflection::ValueTable::const_iterator itKey = isLegacy ?
				keyValueEntry->find("Key") : keyValueEntry->find("Target");

			Reflection::ValueTable::const_iterator itValue = keyValueEntry->find("Value");

			if(itKey == keyValueEntry->end() || !itKey->second.isString() ||
				itValue == keyValueEntry->end())
			{
				FASTLOG(FLog::DataStore, "Unexpected structure of entry");
				continue;
			}

			updateCachedKey(itKey->second.cast<std::string>(), itValue->second);
		}
	}

	void DataStore::sendBatchGet(std::stringstream& keysList)
	{
		std::string finalKeyList = keysList.str();
		FASTLOGS(FLog::DataStore, "Fetching keys: %s", finalKeyList);
		Http http(constructGetUrl());
		http.additionalHeaders["Cache-Control"] = "no-cache";
		http.doNotUseCachedResponse = true;
		http.post(finalKeyList, RBX::Http::kContentTypeUrlEncoded, false, boost::bind(&DataStore::processFetchCachedKeys, shared_from(this), _1, _2));
	}

	void DataStore::accumulateKeyToFetch(const std::string& key, std::stringstream& keysList, int& counter)
	{
		keysList << constructPostDataForKey(key, counter);

		counter++;
		if(counter == DFInt::DataStoreMaxKeysToFetch)
		{
			sendBatchGet(keysList);
			counter = 0;
			keysList.str(std::string());
			keysList.seekg(0, keysList.end);
		}
	}

	void DataStore::refetchCachedKeys(int* budget)
	{
		FASTLOG4(FLog::DataStore, "Refetching keys, budget: %i, onUpdateSize: %u, cachedKeys size: %u, state: %u", *budget, onUpdateKeys.size(), cachedKeys.size(), refetchState);

		std::stringstream keysList;
		int counter = 0;

		int totalOnUpdateKeysFetched = 0;

		if(refetchState == RefetchOnUpdateKeys) {
			FASTLOGS(FLog::DataStore, "Next key to fetch: %s", nextKeyToRefetch);
			OnUpdateKeys::iterator it = onUpdateKeys.lower_bound(nextKeyToRefetch);

			for(; it != onUpdateKeys.end(); ++it)
			{
				if(!it->second->empty())
				{
					if (--(*budget) < 0) 
					{
						nextKeyToRefetch = it->first;
						break;
					}
					accumulateKeyToFetch(it->first, keysList, counter);
					totalOnUpdateKeysFetched++;
				}
			}

			if(it == onUpdateKeys.end())
			{
				FASTLOG(FLog::DataStore, "Still have budget after on update keys");
				refetchState = RefetchCachedKeys;
				nextKeyToRefetch = "";
			}
		}
		FASTLOG1(FLog::DataStore, "Next %u onUpdate keys", totalOnUpdateKeysFetched);

		int totalCachedKeysFetched = 0;

		if (refetchState == RefetchCachedKeys) 
		{
			double touchTimeout = DFInt::DataStoreTouchTimeoutInSeconds;
			Time now = Time::nowFast();
			CachedKeys::iterator it = cachedKeys.lower_bound(nextKeyToRefetch);
			for(; it != cachedKeys.end(); ++it)
			{
				Time::Interval interval = now - it->second.getTime();
				if(interval.seconds() > touchTimeout) 
					continue;

				OnUpdateKeys::iterator itOnUpdates = onUpdateKeys.find(it->first) ;
				if (itOnUpdates != onUpdateKeys.end() && !(itOnUpdates->second->empty()))
					continue;

				if (--(*budget) < 0) 
				{
					nextKeyToRefetch = it->first;
					break;
				}

				accumulateKeyToFetch(it->first, keysList, counter);
				totalCachedKeysFetched++;
			}

			if(it == cachedKeys.end())
			{
				FASTLOG(FLog::DataStore, "Still have budget after cached keys");
				refetchState = RefetchDone;
				nextKeyToRefetch = "";
			}
		}

		if(counter > 0)
			sendBatchGet(keysList);

		FASTLOG2(FLog::DataStore, "Requested update, %u for onUpdateKeys, %u for cachedKeys", totalOnUpdateKeysFetched, totalCachedKeysFetched);
	}

	void DataStore::resetRefetch()
	{
		refetchState = RefetchOnUpdateKeys;		
		nextKeyToRefetch = "";
	}

	bool DataStore::isKeyThrottled(const std::string& key, Time timestamp)
	{
		KeyTimestamps::iterator it = lastSetByKey.find(key);
		if (it == lastSetByKey.end())
			return false;

		FASTLOG2F(FLog::DataStore, "Key timestamp - %f, current - %f", timestamp.timestampSeconds(), it->second.timestampSeconds());

		if ((timestamp - it->second) < Time::Interval(60.0 / DFInt::DataStoreSameKeyPerMinute)) 
		{
			FASTLOGS(FLog::DataStore, "Key %s throttled, moving over", key);
			return true;
		}
		return false;
	}

	void DataStore::setKeySetTimestamp(const std::string& key, Time timestamp)
	{
		FASTLOGS(FLog::DataStore, "Setting key %s timestamp", key);
		FASTLOG1F(FLog::DataStore, "Timestamp: %f ", timestamp.timestampSeconds());
		lastSetByKey[key] = timestamp;
	}


	//////////////////////////////////////////////////////////////////////////

	const char* const sOrderedDataStore = "OrderedDataStore";

	OrderedDataStore::OrderedDataStore(const std::string& name, const std::string& scope)
		: DescribedNonCreatable<OrderedDataStore, DataStore, sOrderedDataStore, Reflection::ClassDescriptor::RUNTIME_LOCAL>(name, scope, false)
	{
	}
	
	bool OrderedDataStore::checkValueIsAllowed(const Reflection::Variant& v)
	{
		if(!v.isFloat())
			return false;

		double value = v.get<double>();
		return value == floor(value);
	}

	bool OrderedDataStore::queueOrExecuteSet(DataStoreService::HttpRequest& request)
	{
		return DataStoreService::queueOrExecuteOrderedSet(this, request);
	}

	void OrderedDataStore::getSortedAsync(bool isAscending, int pageSize, Reflection::Variant minValue, Reflection::Variant maxValue, 
		boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!minValue.isVoid() && !checkValueIsAllowed(minValue))
		{
			errorFunction("MinValue has to be integer or nil");
			return;
		}

		if(!maxValue.isVoid() && !checkValueIsAllowed(maxValue))
		{
			errorFunction("MaxValue has to be integer or nil");
			return;
		}

		if(pageSize < 0)
		{
			errorFunction("PageSize has to be more or equal to zero");
			return;
		}

		if(pageSize > DFInt::DataStoreMaxPageSize)
		{
			errorFunction("PageSize is too large");
			return;
		}

		double minValueHolder, maxValueHolder;

		double* minValueP = NULL, *maxValueP = NULL;

		if(!minValue.isVoid()) {
			minValueHolder = minValue.get<double>();
			minValueP = &minValueHolder;
		}

		if(!maxValue.isVoid()) {
			maxValueHolder = maxValue.get<double>();
			maxValueP = &maxValueHolder;
		}

		std::string url = constructGetSortedUrl(isAscending, pageSize, minValueP, maxValueP);

		shared_ptr<Pages> pagination = Creatable<Instance>::create<DataStorePages>(weak_from(this), url);
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
	}

	std::string doubleToIntegerString(double d) // Make sure this matches rapidJSON implementation
	{
		char buffer[100];
#if _MSC_VER
		sprintf_s(buffer, sizeof(buffer), "%.30g", d);
#else
		snprintf(buffer, sizeof(buffer), "%.30g", d);
#endif
		return buffer;
	}

	std::string OrderedDataStore::constructGetSortedUrl(bool isAscending, int pagesize, const double* minValue, const double* maxValue)
	{
		int placeId = DataModel::get(this)->getPlaceID();
		std::stringstream url;
		url << format("%sgetSortedValues?placeId=%i&type=%s&scope=%s&key=%s&pageSize=%i&ascending=%s", 
			serviceUrl.c_str(), placeId, getDataStoreTypeString(), scopeUrlEncodedIfNeeded.c_str(), nameUrlEncodedIfNeeded.c_str(), pagesize, isAscending? "True" : "False");

		if(minValue)
			url << "&inclusiveMinValue=" << doubleToIntegerString(*minValue);

		if(maxValue)
			url << "&inclusiveMaxValue=" << doubleToIntegerString(*maxValue);

		return url.str();
	}

	// Pages
    const char* const sDataStorePages = "DataStorePages";

	DataStorePages::DataStorePages(weak_ptr<OrderedDataStore> ds, const std::string& requestUrl) :
		ds(ds), requestUrl(requestUrl)
	{
	}

	void DataStorePages::processFetch(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<OrderedDataStore> ods = ds.lock();
		if(!ods)
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, "Datastore no longer exists");
			return;
		}

		DataModel::processHttpRequestResponseOnLock(
			DataModel::get(ods.get()),
			response, 
			exception, 
			boost::bind(&DataStorePages::lockAcquiredProcessFetch,shared_from(this),_1,_2,resumeFunction,errorFunction));
	}

	void DataStorePages::lockAcquiredProcessFetch(shared_ptr<std::string> response, shared_ptr<std::exception> exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;
		if (!LuaWebService::parseWebJSONResponseHelper(response.get(), exception.get(), result, status))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, status);
			errorFunction(status);
			return;
		}

//		Expected format:
// 		{ "data": { "Entries": [{
// 			"Target": "player_1552168488",
// 				"Value": 0
// 		}, {
// 			"Target": "player_1931069815",
// 				"Value": 1
// 		}, {
// 			"Target": "player_221169702",
// 				"Value": 2
// 			}, {
// 				"Target": "player_299777679",
// 					"Value": 3
// 			}, {
// 				"Target": "player_1692004196",
// 					"Value": 4
// 			}],
// 			"ExclusiveStartKey": "player_1692004196$4" }}

		Reflection::ValueTable::const_iterator itError = result->find("error");
		Reflection::ValueTable::const_iterator itData = result->find("data");
		if (itError != result->end())
		{
			const Reflection::Variant errorValue = itError->second;
			std::string errorMessage = errorValue.isType<std::string>() ? 
				errorValue.cast<std::string>() : "Failed to retrieve key";
			FASTLOGS(FLog::DataStore, "Failed, error message: %s", errorMessage);
			errorFunction("Request rejected");
			return;
		}

		if (itData == result->end() || !itData->second.isType<shared_ptr<const Reflection::ValueTable> >())
		{
			errorFunction("Unexpected data in response");
			RBXASSERT(false);
			return;
		}

		shared_ptr<const Reflection::ValueTable> data = 
			itData->second.cast<shared_ptr<const RBX::Reflection::ValueTable> >();

		Reflection::ValueTable::const_iterator itEntries = data->find("Entries");
		if (itEntries == data->end() || !itEntries->second.isType<shared_ptr<const Reflection::ValueArray> >())
		{
			errorFunction("Unexpected entries in response");
			RBXASSERT(false);
			return;
		}

		shared_ptr<Reflection::ValueArray> page = rbx::make_shared<Reflection::ValueArray>();
		shared_ptr<const Reflection::ValueArray> entries = itEntries->second.cast<shared_ptr<const RBX::Reflection::ValueArray> >();

		for(Reflection::ValueArray::const_iterator it = entries->begin(); it != entries->end(); ++it)
		{
			if(!it->isType<shared_ptr<const Reflection::ValueTable> >()) {
				RBXASSERT(false);
				continue;
			}

			shared_ptr<const Reflection::ValueTable> keyValueEntry = 
				it->cast<shared_ptr<const RBX::Reflection::ValueTable> >();

			Reflection::ValueTable::const_iterator itKey = keyValueEntry->find("Target");
			Reflection::ValueTable::const_iterator itValue = keyValueEntry->find("Value");

			if(itKey == keyValueEntry->end() || !itKey->second.isString() ||
				itValue == keyValueEntry->end())
			{
				FASTLOG(FLog::DataStore, "Unexpected structure of entry");
				continue;
			}

			shared_ptr<Reflection::ValueTable> pageEntry = rbx::make_shared<Reflection::ValueTable>();
			(*pageEntry)["key"] = itKey->second.cast<std::string>();
			(*pageEntry)["value"] = itValue->second;
			page->push_back(Reflection::Variant(shared_ptr<const Reflection::ValueTable>(pageEntry)));
		}

		currentPage = shared_ptr<const Reflection::ValueArray>(page);

		Reflection::ValueTable::const_iterator itExclusiveStartKey = data->find("ExclusiveStartKey");

		exclusiveStartKey = "";
		finished = true;

		if(itExclusiveStartKey != data->end() && itExclusiveStartKey->second.isString())
		{
			exclusiveStartKey = itExclusiveStartKey->second.get<std::string>();
			finished = false;
		}

		resumeFunction();
	}

	void DataStorePages::fetchNextChunk(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		DataStoreService::HttpRequest httpRequest;

		shared_ptr<OrderedDataStore> ods = ds.lock();

		if(!ods)
		{
			errorFunction("OrderedDataStore no longer exists");
			return;
		}

		httpRequest.url = exclusiveStartKey.size() == 0 ? 
			requestUrl : 
			format("%s&exclusiveStartKey=%s", requestUrl.c_str(), exclusiveStartKey.c_str());
		httpRequest.handler = boost::bind(&DataStorePages::processFetch, shared_from(this), _1, _2, resumeFunction, errorFunction);
		httpRequest.requestStartTime = boost::posix_time::microsec_clock::local_time();
		httpRequest.requestType = DataStoreService::HttpRequest::RequestType::GET_SORTED_ASYNC_PAGE;
		httpRequest.owner = ods;

		if(!DataStoreService::queueOrExecuteGetSorted(ods.get(), httpRequest))
			errorFunction("Request limit exceeded for get sorted");
	}
}
  
