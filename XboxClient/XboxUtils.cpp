#include "XboxUtils.h"

#include <fstream>

#include "v8xml/WebParser.h"
#include "RobloxServicesTools.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/DataModel.h"
#include "util/http.h"

#include "v8datamodel/DataModelJob.h"

#define XBOX_APP_SETTINGS_STRING  "XboxAppSettings"


//////////////////////////////////////////////////////////////////////////
// fast flags

bool loadLocalFFlags()
{
    std::string s;
    std::copy( std::istream_iterator<char>( std::ifstream("fflags.json") ), std::istream_iterator<char>(), std::back_inserter(s) );
    LoadClientSettingsFromString( CLIENT_APP_SETTINGS_STRING, s, &RBX::ClientAppSettings::singleton() );
    return true;
}

static std::string getFFlagUrl( std::string baseUrl, std::string group, std::string apiKey)
{
    auto pos = baseUrl.find_first_of("://www.");
    if( pos == std::string::npos )
    {
        RBXASSERT( !"baseurl is missing ://www." ); // rewrite the function!
        return "";
    }

    baseUrl.replace( pos, 7, "://clientsettings.api." );

    return RBX::format( "%sSetting/QuietGet/%s/?apiKey=%s", baseUrl.c_str(), group.c_str(), apiKey.c_str() );
}

bool fetchFFlags(const char* baseUrl)
{
	std::string fflagsClient;
	std::string fflagsXbox;
	
    std::string fflagClientUrl = getFFlagUrl(baseUrl, CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY);
	std::string fflagXboxUrl  = getFFlagUrl(baseUrl, XBOX_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY);

    enum { kSleepMs = 100, kTimeoutMs = 30000 };
    try
    {
        RBX::HttpFuture result1 = RBX::HttpAsync::get(fflagClientUrl);
		RBX::HttpFuture result2 = RBX::HttpAsync::get(fflagXboxUrl);
        for( int j=0; ; j += kSleepMs )
        {
            if( j>kTimeoutMs )
            {
                dprintf("FFlag fetch timeout\n");
                return false; // timeout
            }

            if( result1.get_state() == boost::future_state::ready &&
				result2.get_state() == boost::future_state::ready )
            {
                fflagsClient = result1.get();
				fflagsXbox	 = result2.get();
                break;
            }

            ::SleepEx(kSleepMs, TRUE);
        }

        if( fflagsClient.empty() || fflagsXbox.empty())
        {
            dprintf("FFlag fetch failed!\n");
            return false;
        }

        LoadClientSettingsFromString( CLIENT_APP_SETTINGS_STRING, result1.get(), &RBX::ClientAppSettings::singleton() );
		LoadClientSettingsFromString( XBOX_APP_SETTINGS_STRING, result2.get(), &RBX::ClientAppSettings::singleton() );
		

        return true;
    } 
    catch (const std::exception& e)
    {
       dprintf("FFlag fetch exception: %s\n", e.what());
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
// join script


static std::string readStringValue(shared_ptr<const RBX::Reflection::ValueTable> jsonResult, std::string name) 
{
    RBX::Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<std::string>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected string result for %s", name.c_str()));
    }
}

static int readIntValue(shared_ptr<const RBX::Reflection::ValueTable> jsonResult, std::string name)
{
    RBX::Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<int>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected int result for %s", name.c_str()));
    }
}

PlaceLauncherResult requestPlaceInfo(const std::string url, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl)
{
    try
    {
        std::string response;
        RBX::Http(url).get(response);

        std::stringstream jsonStream;
        jsonStream << response;
        shared_ptr<const RBX::Reflection::ValueTable> jsonResult(rbx::make_shared<const RBX::Reflection::ValueTable>());
        bool parseResult = RBX::WebParser::parseJSONTable(jsonStream.str(), jsonResult);
        if (!parseResult)
            return PlaceLaunch_SomethingReallyBad;

        int status = readIntValue(jsonResult, "status");
        if (status != PlaceLaunch_Joining)
            return (PlaceLauncherResult)status;

        authenticationUrl = readStringValue(jsonResult, "authenticationUrl");
        ticket = readStringValue(jsonResult, "authenticationTicket");
        scriptUrl = readStringValue(jsonResult, "joinScriptUrl");
        return PlaceLaunch_Joining;
    }
    catch (RBX::base_exception& e)
    {
        dprintf( "Exception when requesting place info: %s. ", e.what() );
    }
    return PlaceLaunch_SomethingReallyBad;
}

//  Case-sensitive comparison of two Platform::String^ objects
bool isStringEqual(Platform::String^ val1,Platform::String^ val2)
{
	return (wcscmp(val1->Data(), val2->Data() ) == 0);
}

//  Case-insensitive comparison of two Platform::String^ objects
bool isStringEqualCaseInsensitive(Platform::String^ val1,Platform::String^ val2)
{
	return (_wcsicmp(val1->Data(), val2->Data()) == 0);
}

std::string ws2s(const wchar_t* data)
{
    // NOTE: had to hack it because std::codecvt is so BAD, it throws up if it encounters a unicode symbol
    std::string result;
    if(data)
    {
        for (const wchar_t* p = data; *p; ++p) 
            if ((unsigned)*p<128)
                result.push_back(*p);
    }
    return result;
}

std::wstring s2ws(const std::string* data)
{
	std::wstring ws(data->begin(), data->end());
	return ws;
}

//////////////////////////////////////////////////////////////////////////
struct GenericDataModelJob : RBX::DataModelJob
{
    double per;
    std::function<void()> stepfn;

    GenericDataModelJob( const char* name, int type, shared_ptr< RBX::DataModel> dm, double period, std::function<void()> step )
        : RBX::DataModelJob(name, (TaskType)type, false, dm, RBX::Time::Interval(period) )
    {
        per = period;
        stepfn = step;
    }

    virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats) override
    {
        if( stepfn ) stepfn();
        return RBX::TaskScheduler::Stepped;
    }

    virtual RBX::Time::Interval sleepTime(const Stats& stats) override
    {
        return computeStandardSleepTime(stats, 1/per);
    }
    virtual RBX::TaskScheduler::Job::Error error(const Stats& stats) override
    {
        return computeStandardError(stats, 1/per);
    }
};


shared_ptr<RBX::DataModelJob> addGenericDataModelJob(  const char* name, int type, shared_ptr< RBX::DataModel> dm, double period, std::function<void()> step )
{
    shared_ptr< RBX::DataModelJob > job ( new GenericDataModelJob(name, type, dm, period, step) );
    RBX::TaskScheduler::singleton().add( job );
    return job;
}

void removeGenericDataModelJob( shared_ptr< RBX::DataModelJob >& job, std::function<void()> spinfunc )
{
    if( !job ) return;
    RBX::TaskScheduler::singleton().removeBlocking(job, [=]()->void { spinfunc(); } );
    job.reset();
}
