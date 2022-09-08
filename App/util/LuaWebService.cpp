#include "stdafx.h"

#include "Util/LuaWebService.h"
#include "V8DataModel/ContentProvider.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "v8xml/WebParser.h"
#include "RobloxServicesTools.h"


DYNAMIC_FASTINTVARIABLE(TimeBetweenCheckingApiAccessMillis, 5000)

namespace RBX {
    
	RBX_REGISTER_CLASS(Pages);
	RBX_REGISTER_CLASS(StandardPages);
	RBX_REGISTER_CLASS(FriendPages);
    
    const char* const sPages = "Pages";

    REFLECTION_BEGIN();
    static Reflection::BoundFuncDesc<Pages, shared_ptr<const Reflection::ValueArray>(void)> func_getCurrentPage
    (&Pages::getCurrentPage, "GetCurrentPage", Security::None);
    static Reflection::PropDescriptor<Pages, bool> prop_isFinished
    ("IsFinished", category_Data, &Pages::isFinished, NULL, Reflection::PropertyDescriptor::UI);

    static Reflection::BoundYieldFuncDesc<Pages, void(void)> func_advanceToNextPageAsync (&Pages::advanceToNextPageAsync, "AdvanceToNextPageAsync", Security::None);
    REFLECTION_END();
        
    Pages::Pages() : finished(false)
    {

    }
        
    void Pages::advanceToNextPageAsync(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
        if(finished)
        {
            errorFunction("No pages to advance to");
            return;
        }
        
        fetchNextChunk(resumeFunction, errorFunction);
    }

    bool Pages::isFinished() const
    {
        return finished;
    }

    shared_ptr<const Reflection::ValueArray> Pages::getCurrentPage()
    {
        return currentPage;
    }
    
    const char* const sStandardPages = "StandardPages";

    StandardPages::StandardPages(weak_ptr<DataModel> weakDM, const std::string& requestUrl, const std::string& fieldName) : weakDM(weakDM), requestUrl(requestUrl), fieldName(fieldName), pageNumber(1)
    {
    }
    
    void StandardPages::fetchNextChunk(boost::function<void ()> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
		shared_ptr<DataModel> dm = weakDM.lock();
        
		if(!dm)
		{
			errorFunction("OrderedDataStore no longer exists");
			return;
		}

		bool hasQuery = requestUrl.find('?') != std::string::npos;
		std::string url = format("%s%cpage=%i", requestUrl.c_str(), hasQuery ? '&' : '?', pageNumber);

		if (HttpRbxApiService::isAPIHttpRequest(url))
		{
			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(dm.get()))
			{
				apiService->getAsync(HttpRbxApiService::getApiUrlPath(url), true, RBX::PRIORITY_DEFAULT,
					boost::bind(&StandardPages::processFetchSuccess, shared_from(this), _1, resumeFunction, errorFunction),
					boost::bind(&StandardPages::processFetchError, shared_from(this), _1, errorFunction) );
			}
		}
		else
		{
			RBX::Http(url).get(boost::bind(&StandardPages::processFetch, shared_from(this), _1, _2, resumeFunction, errorFunction));
		}
    }
	
    template<class T> bool valueTableGet(shared_ptr<const Reflection::ValueTable> valueTable, const std::string& key, T* outValue)
    {
        Reflection::ValueTable::const_iterator it = valueTable->find(key);
        if(it == valueTable->end() || !it->second.isType<T>()) {
            return false;
        }
        
        *outValue = it->second.cast<T>();
        
        return true;
    }

	void StandardPages::processFetchSuccess(std::string response, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;

		if (!LuaWebService::parseWebJSONResponseHelper(&response, NULL, result, status))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, status);
			errorFunction(status);
			return;
		}

		bool finalPage;
		if(!valueTableGet(result, "FinalPage", &finalPage))
		{
			errorFunction("Unexpected web response");
			return;
		}

		shared_ptr<const Reflection::ValueArray> data;
		if(!valueTableGet(result, fieldName, &data)) {
			errorFunction("Unexpected web response");
			return;
		}



		if(finalPage)
			finished = true;
		else
			pageNumber++;

		currentPage = data;

		resumeFunction();
	}

	void StandardPages::processFetchError(std::string error, boost::function<void(std::string)> errorFunction)
	{
		errorFunction( RBX::format("StandardPages: Request Failed because %s", error.c_str()) );
	}
    
    void StandardPages::processFetch(std::string *response, std::exception *exception, boost::function<void ()> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
        DataModel::LegacyLock lock(weakDM.lock(), DataModelJob::Write);
		shared_ptr<const Reflection::ValueTable> result;
		std::string status;
        
        if (!LuaWebService::parseWebJSONResponseHelper(response, exception, result, status))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, status);
			errorFunction(status);
			return;
		}
        
        bool finalPage;
        if(!valueTableGet(result, "FinalPage", &finalPage))
        {
            errorFunction("Unexpected web response");
            return;
        }
        
        shared_ptr<const Reflection::ValueArray> data;
        if(!valueTableGet(result, fieldName, &data)) {
            errorFunction("Unexpected web response");
            return;
        }
        
        
        
        if(finalPage)
            finished = true;
        else
            pageNumber++;
        
        currentPage = data;
        
        resumeFunction();
    }

	const char* const sFriendPages = "FriendPages";

    FriendPages::FriendPages(weak_ptr<DataModel> weakDM, const std::string& requestUrl) 
		:weakDM(weakDM)
		,requestUrl(requestUrl)
		,pageNumber(1)
		,firstTime(true)
    {
    }
    
    void FriendPages::fetchNextChunk(boost::function<void ()> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
		shared_ptr<DataModel> dm = weakDM.lock();
        
		if(!dm)
		{
			errorFunction("DataModel no longer exists");
			return;
		}

		if (!firstTime) {
			bool hasQuery = requestUrl.find('?') != std::string::npos;
			if (nextPage) 
			{
				currentPage = nextPage;
			}
			std::string url = format("%s%cpage=%i", requestUrl.c_str(), hasQuery ? '&' : '?', pageNumber + 1);

			if (HttpRbxApiService::isAPIHttpRequest(url))
			{
				if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(dm.get()))
				{
					apiService->getAsync(HttpRbxApiService::getApiUrlPath(url), true, RBX::PRIORITY_DEFAULT,
						boost::bind(&FriendPages::processFetchSuccess, shared_from(this), _1, resumeFunction, errorFunction),
						boost::bind(&FriendPages::processFetchError, shared_from(this), _1, errorFunction) );
				}
			}
			else
			{
				RBX::Http(url).get(boost::bind(&FriendPages::processFetch, shared_from(this), _1, _2, resumeFunction, errorFunction));
			}
		} 
		else
		{
			bool hasQuery = requestUrl.find('?') != std::string::npos;
			std::string url = format("%s%cpage=%i", requestUrl.c_str(), hasQuery ? '&' : '?', pageNumber);
			if (HttpRbxApiService::isAPIHttpRequest(url))
			{
				if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(dm.get()))
				{
					apiService->getAsync(HttpRbxApiService::getApiUrlPath(url), true, RBX::PRIORITY_DEFAULT,
						boost::bind(&FriendPages::processFetchSuccess, shared_from(this), _1, resumeFunction, errorFunction),
						boost::bind(&FriendPages::processFetchError, shared_from(this), _1, errorFunction) );
				}
			}
			else
			{
				RBX::Http(url).get(boost::bind(&FriendPages::processFetch, shared_from(this), _1, _2, resumeFunction, errorFunction));
			}
		}
    }

	void FriendPages::processFetchSuccess(std::string response, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		shared_ptr<const Reflection::ValueArray> result;

		if(!WebParser::parseJSONArray(response, result))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, "Can't process JSON");
			errorFunction("Can't parse JSON");
			return;
		}

		if (!firstTime) 
		{
			if(result->begin() == result->end()) 
			{
				finished = true;
			}
			else
			{
				pageNumber++;
			}

			nextPage = result;
			resumeFunction();
		}
		else
		{
			currentPage = result;
			firstTime = false;
			fetchNextChunk(resumeFunction, errorFunction);	
		}
	}

	void FriendPages::processFetchError(std::string error, boost::function<void(std::string)> errorFunction)
	{
		errorFunction( RBX::format("FriendPages: Request Failed because %s", error.c_str()) );
	}
    
    void FriendPages::processFetch(std::string *response, std::exception *exception, boost::function<void ()> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
		DataModel::LegacyLock lock(weakDM.lock(), DataModelJob::Write);
        shared_ptr<const Reflection::ValueArray> result;

		if(!WebParser::parseJSONArray(*response, result))
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, "Can't process JSON");
			errorFunction("Can't parse JSON");
			return;
		}

		if (!firstTime) 
		{
			if(result->begin() == result->end())
			{
				finished = true;
			}
			else
			{
				pageNumber++;
			}

			nextPage = result;
			resumeFunction();
		}
		else
		{
			currentPage = result;
			firstTime = false;
			fetchNextChunk(resumeFunction, errorFunction);	
		}
    }

const char *const sLuaWebService = "LuaWebService";

bool findLocalFile(const std::string& url, std::string* filename)
{
	return true;
}

bool LuaWebService::parseWebJSONResponseHelper(std::string* response, std::exception* exception, 
										   shared_ptr<const Reflection::ValueTable>& result, std::string& status)
{
	if (response == NULL)
	{
		status = exception ? exception->what() : "Request Failed";
		return false;
	}

	bool parseResult = WebParser::parseJSONTable(*response, result);;
	
	if (!parseResult)
	{
		status = "Can't parse JSON";
		return false;
	}

	return true;
}



LuaWebService::CachedRawLuaWebServiceInfo::CachedRawLuaWebServiceInfo(shared_ptr<const std::string> data, shared_ptr<const std::string> filename)
{
	const char* copiedValue = data.get()->c_str();
	value = copiedValue;
}

LuaWebService::CachedLuaWebServiceInfo::CachedLuaWebServiceInfo(shared_ptr<const std::string> data, shared_ptr<const std::string> filename)
{
	std::auto_ptr<std::istream> stream(new std::istringstream(*data));
	if(!WebParser::parseWebGenericResponse(*stream, value)){
		throw std::runtime_error("bad xml");
	}
}
    
LuaWebService::LuaWebService()
	: DescribedNonCreatable<LuaWebService,Instance,sLuaWebService>()
	, checkApiAccess(false)
{
	webCache.reset(new AsyncHttpCache<CachedLuaWebServiceInfo, true>(this,&findLocalFile, 2, 500));
	webRawCache.reset(new AsyncHttpCache<CachedRawLuaWebServiceInfo, true>(this,&findLocalFile, 2, 500));
}

template<typename ResultType>
bool LuaWebService::TryDispatchRequest(AsyncHttpCache<LuaWebService::CachedLuaWebServiceInfo, true>* webCache, const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	LuaWebService::CachedLuaWebServiceInfo result;
	if(webCache->findCacheItem(url, &result)){
		if(result.value.isType<ResultType>())
		{
			resumeFunction(result.value.cast<ResultType>());
		}
		else{
			errorFunction("Wrong return data type");
		}
		return true;
	}
	return false;
}

template<typename ResultType>
bool LuaWebService::TryRawDispatchRequest(AsyncHttpCache<LuaWebService::CachedRawLuaWebServiceInfo, true>* webCache, const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	LuaWebService::CachedRawLuaWebServiceInfo result;
	if(webCache->findCacheItem(url, &result)){
		if(result.value.size() > 0)
		{
			resumeFunction(result.value);
		}
		else{
			errorFunction("Raw Request: string is empty");
		}
		return true;
	}
	return false;
}


template<typename Result>
bool LuaWebService::checkCache(const std::string& url, boost::function<void(Result)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(TryDispatchRequest<Result>(webCache.get(), url, resumeFunction, errorFunction))
		return true;
	return false;
}
template<typename ResultType>
void LuaWebService::Callback(boost::weak_ptr<LuaWebService> weakLuaWebService, AsyncHttpQueue::RequestResult requestResult, std::string url,
						  boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(requestResult == AsyncHttpQueue::Succeeded){
		if(boost::shared_ptr<LuaWebService> webService = weakLuaWebService.lock())
		{
			if(TryDispatchRequest<ResultType>(webService->webCache.get(), url, resumeFunction, errorFunction)){
				return;
			}
		}
	}
	
	errorFunction("LuaWebService error");
}

void LuaWebService::RawCallback(boost::weak_ptr<LuaWebService> weakLuaWebService, AsyncHttpQueue::RequestResult requestResult, std::string url,
								boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(requestResult == AsyncHttpQueue::Succeeded){
		if(boost::shared_ptr<LuaWebService> webService = weakLuaWebService.lock())
		{
			if(TryRawDispatchRequest<std::string>(webService->webRawCache.get(), url, resumeFunction, errorFunction)){
				return;
			}
		}
	}
	
	errorFunction("RawCallback error");
}

void LuaWebService::asyncRequest(const std::string& url, float priority,
	  					         boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(!checkCache(url, resumeFunction, errorFunction)){
		AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::Callback<shared_ptr<const Reflection::ValueArray> >, weak_from(this), _1, url, resumeFunction, errorFunction);
		webCache->asyncRequest(url, priority, &callback, AsyncHttpQueue::AsyncNone);
	}
}

void LuaWebService::asyncRequestNoCache(const std::string& url, float priority, boost::function<void(shared_ptr<const Reflection::ValueMap>)> resultFunction, AsyncHttpQueue::ResultJob resultJob)
{
	boost::function<void(std::string)> errorFunction = boost::bind(resultFunction, shared_ptr<const Reflection::ValueMap>());
	AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::Callback<shared_ptr<const Reflection::ValueMap> >, weak_from(this), _1, url, resultFunction, errorFunction);
	webCache->asyncRequest(url, priority, &callback, resultJob, true);
}


void LuaWebService::asyncRequest(const std::string& url, float priority,
	  					         boost::function<void(shared_ptr<const Reflection::ValueMap>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(!checkCache(url, resumeFunction, errorFunction)){
		AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::Callback<shared_ptr<const Reflection::ValueMap> >, weak_from(this), _1, url, resumeFunction, errorFunction);
		webCache->asyncRequest(url, priority, &callback, AsyncHttpQueue::AsyncNone);
	}
}



void LuaWebService::asyncRequest(const std::string& url, float priority,
	  					         boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(!checkCache(url, resumeFunction, errorFunction)){
		AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::Callback<bool>, weak_from(this), _1, url, resumeFunction, errorFunction);
		webCache->asyncRequest(url, priority, &callback, AsyncHttpQueue::AsyncNone);
	}
}

void LuaWebService::asyncRequest(const std::string& url, float priority,
						  boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(!checkCache(url, resumeFunction, errorFunction)){
		AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::Callback<int>, weak_from(this), _1, url, resumeFunction, errorFunction);
		webCache->asyncRequest(url, priority, &callback, AsyncHttpQueue::AsyncNone);
	}
}

void LuaWebService::asyncRequest(const std::string& url, float priority,
								 boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(!checkCache(url, resumeFunction, errorFunction)){
		AsyncHttpQueue::RequestCallback callback = boost::bind(&LuaWebService::RawCallback, weak_from(this), _1, url, resumeFunction, errorFunction);
		webRawCache->asyncRequest(url, priority, &callback, AsyncHttpQueue::AsyncNone);
	}
}

bool LuaWebService::isApiAccessEnabled()
{
	if (checkApiAccess)
	{
		if (!apiAccess.is_initialized() ||
			(Time::now<Time::Fast>() > timeToRecheckApiAccess))
		{
			DataModel* dm = DataModel::get(this);
			ContentProvider* cp = dm->create<ContentProvider>();

			std::string response;
			
			Http http(format("%s/universes/get-info?placeId=%d",
				::trim_trailing_slashes(cp->getUnsecureApiBaseUrl()).c_str(), dm->getPlaceID()));
			http.doNotUseCachedResponse = true;
			try
			{
				if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(dm))
				{
					apiService->get( format("universes/get-info?placeId=%d",dm->getPlaceID()), false, RBX::PRIORITY_EXTREME, response );
				}
			} 
			catch (const RBX::base_exception&) {}

			bool ableToParse = false;
			bool parsedValue = false;

			if (!response.empty())
			{
				boost::shared_ptr<const Reflection::ValueTable> v;
				if (WebParser::parseJSONTable(response, v))
				{
					RBX::Reflection::ValueTable::const_iterator itr = v->find("StudioAccessToApisAllowed");
					if (itr != v->end() && itr->second.isType<bool>())
					{
						ableToParse = true;
						parsedValue = itr->second.cast<bool>();
					}
				}
			}

			apiAccess = ableToParse && parsedValue;
			
			timeToRecheckApiAccess = Time::now<Time::Fast>() +
				Time::Interval::from_milliseconds(DFInt::TimeBetweenCheckingApiAccessMillis);
		}
		return apiAccess.get();
	}
	else
	{
		return true;
	}
}

void LuaWebService::setCheckApiAccessBecauseInStudio()
{
	checkApiAccess = true;
}

}