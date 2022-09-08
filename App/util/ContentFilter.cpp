#include "stdafx.h"

#include "Util/ContentFilter.h"
#include "Util/Http.h"
#include "v8datamodel/DataModel.h"

namespace RBX
{
	const char* const sContentFilter = "ContentFilter";
	const unsigned ContentFilter::MAX_CONTENT_FILTER_SIZE = 1024;

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<ContentFilter, void(std::string)> func_setFilterUrl( &ContentFilter::setFilterUrl, "SetFilterUrl", "url", Security::LocalUser);
	static Reflection::BoundFuncDesc<ContentFilter, void(int,int)> func_setFilterLimits( &ContentFilter::setFilterLimits, "SetFilterLimits", "outstandingRequests", "cacheSize", Security::LocalUser);
    REFLECTION_END();


	ContentFilter::ContentFilter()
		:DescribedNonCreatable<ContentFilter, Instance, sContentFilter, Reflection::ClassDescriptor::RUNTIME_LOCAL>("ContentFilter")
		,url()
		,maxOutstandingRequests(10)
		,maxTableSize(1000)
	{}
	ContentFilter::~ContentFilter()
	{}

	void ContentFilter::setFilterUrl(std::string url)
	{
		this->url = url;
	}

	void ContentFilter::setFilterLimits(int outstandingRequests, int cacheSize)
	{
		this->maxOutstandingRequests = outstandingRequests;
		this->maxTableSize = cacheSize;
	}

	static void staticDoFilterRequest(weak_ptr<ContentFilter> contentFilterWeak, std::string value)
	{
		if(shared_ptr<ContentFilter> contentFilter = contentFilterWeak.lock()){
			contentFilter->doFilterRequest(value);
		}
	}
	static void staticSaveFilterResult(weak_ptr<ContentFilter> contentFilterWeak, std::string value, bool result)
	{
		if(shared_ptr<ContentFilter> contentFilter = contentFilterWeak.lock()){
			contentFilter->saveFilterResult(value, result);
		}
	}

	static void staticDoFilterResult(std::string *response, std::exception *err, weak_ptr<ContentFilter> contentFilterWeak, std::string value)
	{
		bool result;
		if(response){
			if(response->find("True") != std::string::npos) result = true;
			else if(response->find("False") != std::string::npos) result = false;
			else result = true;
		}
		else{
			//Do we fail open or fail close?
			result = true;
		}

		if(shared_ptr<ContentFilter> contentFilter = contentFilterWeak.lock()){
			DataModel* dataModel = Instance::fastDynamicCast<DataModel>(contentFilter->getParent());
			if(dataModel){
				dataModel->submitTask(boost::bind(&staticSaveFilterResult, contentFilterWeak, value, result), DataModelJob::Write);
			}
		}
	}

	void ContentFilter::truncateString(std::string& text)
	{
		if(text.size() > MAX_CONTENT_FILTER_SIZE){
			text = text.substr(0,MAX_CONTENT_FILTER_SIZE);
		}
	}
	ContentFilter::FilterResult ContentFilter::getStringState(std::string& text)
	{
		if(isContentFilterReady(text)){
			return isStringSafe(text) ? ContentFilter::Succeeded : ContentFilter::Failed;
		}
		return ContentFilter::Waiting;
	}
	void ContentFilter::cleanTable()
	{
		if(resultsDictionary.size() > maxTableSize){
			//Remove all elements with a count < 2
			for(ResultsDictionary::iterator iter = resultsDictionary.begin(); iter != resultsDictionary.end();  ){
				if(iter->second.usageCount < 2){
					resultsDictionary.erase(iter++);
				}
				else{
					iter->second.usageCount = 0;
					++iter;
				}
			}
			if(resultsDictionary.size() > (maxTableSize/2)){
				//If we're still bigger than half, we need to clean out some more entries.  We'll do it at random now
				int toRemove = resultsDictionary.size() - (maxTableSize/2);
				ResultsDictionary::iterator iter  = resultsDictionary.begin();
				while(toRemove > 0){
					resultsDictionary.erase(iter++);
					--toRemove;
				}
			}
		}

	}
	bool ContentFilter::isContentFilterReady(const std::string& value)
	{
		if(url.empty() || value.empty()){
			return true;
		}
		std::string text = value;
		truncateString(text);

		ResultsDictionary::const_iterator iter = resultsDictionary.find(text);
		if(iter != resultsDictionary.end()){
			return true;
		}

		//Check if our dictionary is too big.
		if(resultsDictionary.size() > maxTableSize){
			cleanTable();
		}

		if(requestSet.find(text) == requestSet.end() && requestSet.size() < maxOutstandingRequests){
			//Spin off a request job to start this request
			DataModel* dataModel = Instance::fastDynamicCast<DataModel>(getParent());
			if(dataModel){
				dataModel->submitTask(boost::bind(&staticDoFilterRequest, weak_from(this), text), DataModelJob::Write);
			}
		}
		return false;
	}

	bool ContentFilter::isStringSafe(std::string& value)
	{
		truncateString(value);

		if(url.empty() || value.empty()){
			return true;
		}
		ResultsDictionary::iterator iter = resultsDictionary.find(value);
		if(iter != resultsDictionary.end()){
			iter->second.usageCount++;
			return iter->second.result;
		}
		return false;
	}

	void ContentFilter::doFilterRequest(std::string value)
	{
		if(requestSet.find(value) == requestSet.end() && requestSet.size() < maxOutstandingRequests){
			requestSet.insert(value);
			Http(url, Http::WinHttp).post(value, Http::kContentTypeDefaultUnspecified, false,
				boost::bind(&staticDoFilterResult, _1, _2, weak_from(this), value));		
		}
	}
	
	void ContentFilter::saveFilterResult(std::string value, bool result)
	{
		requestSet.erase(value);
		resultsDictionary[value] = ResultEntry(result);
	}
}