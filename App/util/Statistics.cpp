#include "stdafx.h"

#undef min
#undef max

#include "Util/Statistics.h"

#include <sstream>
#include <algorithm>

#include "RbxFormat.h"
#include "util/StandardOut.h"
#include "util/Http.h"
#include "format_string.h"
#include "SimpleJSON.h"
#include "util/FileSystem.h"
#include "rbx/rbxTime.h"
#include "RobloxServicesTools.h"

#include "v8xml/WebParser.h"
#include "util/RobloxGoogleAnalytics.h"

#include <boost/algorithm/string/predicate.hpp>

LOGGROUP(ClientSettings);

ABTEST_NEWSTUDIOUSERS_VARIABLE(DummyTest);

#ifdef RBX_TEST_BUILD
static std::string defaultFilePath;

void SetDefaultFilePath(const std::string &path)
{
	defaultFilePath = path;
}

const std::string &GetDefaultFilePath()
{
	return defaultFilePath;
}
#endif

static std::string defaultBaseUrl;
void SetBaseURL(const std::string& baseUrl)
{
	defaultBaseUrl = baseUrl;
}

const std::string& GetBaseURL()
{
	return defaultBaseUrl;
}

std::string RBX::Http::urlEncode(const std::string& url)
{
	std::string result;

	const size_t strLen = url.size();
	for (unsigned i=0; i < strLen; i++)
	{
		unsigned char c = url[i];
		if (
			(c<=47) ||
			(c>=58 && c<=64) ||
			(c>=91 && c<=96) ||
			(c>=123)
			)
		{
			result += RBX::format("%%%02X", c);
		}
		else
			result += c;
	}

	return result;
}

std::string RBX::Http::urlDecode(const std::string& fragment)
{
	std::string result;

	const size_t strLen = fragment.size();
	std::string::size_type i = 0;
	char hexBuffer[3];
	while (i < strLen)
	{
		unsigned char c = fragment[i];
		if (c == '%')
		{
			RBXASSERT((i + 3) <= strLen);
			hexBuffer[0] = fragment[i + 1];
			hexBuffer[1] = fragment[i + 2];
			hexBuffer[2] = '\0';
			result += (char)strtol(hexBuffer, NULL, 16);
			i += 3;
		}
		else
		{
			result += c;
			++i;
		}
	}
	return result;
}

std::string UploadLogFile(const std::string& baseUrl, const std::string& data)
{
	try
	{
		std::stringstream request;
		request << baseUrl + "/Analytics/LogFile.ashx" << char(0);

		std::stringstream dataStream;
		dataStream << data << char(0);

		std::string result;
		RBX::Http(request.str()).post(dataStream, RBX::Http::kContentTypeDefaultUnspecified, true, result);
		return result;
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(RBX::base_exception&)
	{
		return "";
	}
}

bool FetchLocalClientSettingsData(const char* group, SimpleJSON* dest)
{
	std::string localSettingsData;
	// Load from file
	boost::filesystem::path localFilename = RBX::FileSystem::getUserDirectory(false, RBX::DirExe, "ClientSettings") / (group + std::string(".json"));
	std::ifstream localConfigFile(localFilename.native().c_str());

	if(localConfigFile.is_open())
	{
		std::stringstream localConfigBuffer;
		localConfigBuffer << localConfigFile.rdbuf();
		localSettingsData = localConfigBuffer.str();
		if(localSettingsData.length() > 0) {
			FASTLOG(FLog::ClientSettings, "Found local json file");
			dest->ReadFromStream(localSettingsData.c_str());
			return true;
		}
	}

	return false;
}

void LoadClientSettingsFromString(const char* group, const std::string& settingsData, SimpleJSON* dest)
{
	FASTLOGS(FLog::ClientSettings, "Loading group %s", group);
	
	if(settingsData.length() > 0) {
		dest->ReadFromStream(settingsData.c_str());
	}

	bool localDataFetched = FetchLocalClientSettingsData(group, dest);

	if(localDataFetched && settingsData.length() == 0) {
		FASTLOG(FLog::ClientSettings, "Couldn't find any data to fetch");
	}
}

// returns if the fetch from web was successful, useful if you want to terminate the operation if web fetch fails
bool FetchClientSettingsData(const char* group, const char* apiKey, SimpleJSON* dest)
{
    bool fetchedFromWeb = false;
	std::string settingsData = "";
	FetchClientSettingsData(group, apiKey, &settingsData);
    
    if (settingsData.length()) fetchedFromWeb = true;
    
    LoadClientSettingsFromString(group, settingsData, dest);
    
    return fetchedFromWeb;
}

void FetchClientSettingsData(const char* group, const char* apiKey, std::string* dest)
{
	FASTLOGS(FLog::ClientSettings, "Loading group %s to string", group);

	const std::string& baseUrl = GetBaseURL();
	if (baseUrl.size()==0)
		RBXCRASH();	// You didn't set BaseURL before loading settings!

	std::string url = GetSettingsUrl(baseUrl, group, apiKey);

	RBX::Http request(url);

	try 
	{
		request.get(*dest);
	}
	catch (RBX::base_exception&)
	{
		FASTLOG(FLog::Always, "FetchClientSettingsData exception");
	}
}


RBX::HttpFuture FetchClientSettingsDataAsync(const char* group, const char* apiKey)
{
	FASTLOGS(FLog::ClientSettings, "Loading group %s to string", group);

	const std::string& baseUrl = GetBaseURL();
	if (baseUrl.size()==0)
		RBXCRASH();	// You didn't set BaseURL before loading settings!

	std::string url = GetSettingsUrl(baseUrl, group, apiKey);

	return RBX::HttpAsync::get(url);
}

struct ABPass {
	std::string entry;
	shared_ptr<RBX::Reflection::ValueArray> experiments;
};

void addABTest(const std::string& name, const std::string& value, void* context)
{
	ABPass* pass = (ABPass*)context;

	shared_ptr<RBX::Reflection::ValueTable> experimentTable = rbx::make_shared<RBX::Reflection::ValueTable>();
	(*experimentTable)["Name"] = name;
	(*experimentTable)["Type"] = pass->entry;

	pass->experiments->push_back(shared_ptr<const RBX::Reflection::ValueTable>(experimentTable));
}

RBX::HttpFuture FetchABTestDataAsync(const std::string& url)
{
	FASTLOG(FLog::ClientSettings, "Loading AB tests to http future");

	shared_ptr<RBX::Reflection::ValueArray> experiments = rbx::make_shared<RBX::Reflection::ValueArray>();
	ABPass passData;
	passData.experiments = experiments;

	passData.entry = "NewUsers";
	FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_NEWUSERS);
	passData.entry = "NewStudioUsers";
	FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_NEWSTUDIOUSERS);
	passData.entry = "AllUsers";
	FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_ALLUSERS);

	std::string jsonRequest;
	RBX::WebParser::writeJSON(shared_ptr<const RBX::Reflection::ValueArray>(experiments), jsonRequest);

	RBX::HttpPostData postData(jsonRequest, RBX::Http::kContentTypeApplicationJson, false);

	return RBX::HttpAsync::post(url, postData);
}

std::string LoadABTestFromString(const std::string& responseData)
{
	std::string trackerId = "";
	shared_ptr<const RBX::Reflection::ValueTable> jsonResponse;
	bool result = RBX::WebParser::parseJSONTable(responseData, jsonResponse);
	RBXASSERT(result);
	if(!result)
		return trackerId;

	try 
	{
		shared_ptr<const RBX::Reflection::ValueTable> experiments = jsonResponse->at("Experiments").cast<shared_ptr<const RBX::Reflection::ValueTable> >();

		for(RBX::Reflection::ValueTable::const_iterator it = experiments->begin(); it != experiments->end(); ++it)
		{
			shared_ptr<const RBX::Reflection::ValueTable> experimentDesc = it->second.cast<shared_ptr<const RBX::Reflection::ValueTable> >();
			int value = experimentDesc->at("Variation").cast<int>();
			bool locked = experimentDesc->at("IsLocked").cast<bool>();		

			// -1, 0, 1 and 2 are control groups, 3+ are variations
			int variation = std::max(value - 2, 0);
			FLog::SetValue(it->first, RBX::StringConverter<int>::convertToString(variation));

 			if(value > 0 && !locked) 
 			{
 				RBX::RobloxGoogleAnalytics::setExperimentVariation(it->first, value);
 			}
		}

		if(jsonResponse->find("BrowserTrackerId") != jsonResponse->end())
			trackerId = jsonResponse->at("BrowserTrackerId").cast<std::string>();
	}
	catch(std::exception& e)
	{
		RBXASSERT(false);
		FASTLOGS(FLog::ClientSettings, "Failed to parse AB test JSON - %s", e.what());
	}

	return trackerId;
}


void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData,
					 const char* primaryFilterName, const char* primaryFilterValue,
					 const char* secondaryFilterName, const char* secondaryFilterValue);

void ReportStatisticWithMessage(const std::string& baseUrl, const std::string& id, const std::string& simpleMessage, 
					 const char* secondaryFilterName, const char* secondaryFilterValue)
{
	std::stringstream data;
	if(simpleMessage.length() > 0){
		std::string messageStr(simpleMessage);
		while(messageStr.find("\n") != std::string::npos){
			messageStr.replace(messageStr.find("\n"),1,"%20");
		}
		while(messageStr.find("\t") != std::string::npos){
			messageStr.replace(messageStr.find("\t"),1,"%20");
		}

		data << "string\tmessage\t" << messageStr;
	}
	data << char(0);

	ReportStatisticPost(baseUrl, id, data.str(), NULL, NULL, secondaryFilterName, secondaryFilterValue);
}

void ReportStatistic(const std::string& baseUrl, const std::string& id,
					 const std::string& primaryFilterName, const std::string& primaryFilterValue,
					 const std::string& secondaryFilterName, const std::string& secondaryFilterValue)
{
	ReportStatisticPost(baseUrl, id, "", 
		primaryFilterName.empty() ? 0 : primaryFilterName.c_str(),	// 0 means use IPFilter! 
		primaryFilterValue.c_str(), 
		secondaryFilterName.c_str(), secondaryFilterValue.c_str());
}

void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData,
					     const char* secondaryFilterName, const char* secondaryFilterValue)
{
	ReportStatisticPost(baseUrl, id, postData, NULL, NULL, secondaryFilterName, secondaryFilterValue);
}

void DontCareResponse(std::string* response, std::exception* exception)
{

}
void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData,
					 const char* primaryFilterName, const char* primaryFilterValue,
					 const char* secondaryFilterName, const char* secondaryFilterValue)
{
	try
	{
		std::string idStr = std::string("ROBLOXAPP ") + std::string(id);
		idStr = RBX::Http::urlEncode(idStr);
		
		std::stringstream request;
		request << baseUrl + "/Analytics/Measurement.ashx?Type=" << idStr;
		if (primaryFilterName)
		{
			request << "&FilterName=" << primaryFilterName << "&FilterValue=" << primaryFilterValue;
		}
		else
			request << "&IpFilter=primary";

		if (secondaryFilterName)
			request << "&SecondaryFilterName=" << secondaryFilterName << "&SecondaryFilterValue=" << secondaryFilterValue;
		
		request << char(0);

		std::string result;
		
		if(postData.length() > 0){
			RBX::Http(request.str()).post(postData, RBX::Http::kContentTypeDefaultUnspecified,
				true, &DontCareResponse);
		}
		else{
			RBX::Http(request.str()).get(&DontCareResponse);
		}
	}
	catch (RBX::base_exception& e)
	{
		std::stringstream log;
		log << "ReportStat '" << id << "' failed. " << e.what() << '\n';
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"%s",log.str().c_str());
	}
}