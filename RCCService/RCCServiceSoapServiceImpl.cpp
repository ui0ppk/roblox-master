
#include "stdafx.h"

#include <atlsync.h>

#include "gSOAP/generated/soapRCCServiceSoapService.h"
#include "rbx/ProcessPerfCounter.h"
#include "rbx/Debug.h"
#include "util/ProtectedString.h"
#include "util/StandardOut.h"
#include "boost/noncopyable.hpp"
#include "boost/thread/xtime.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/foreach.hpp"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")



#include "v8datamodel/workspace.h"
#include "v8datamodel/contentprovider.h"
#include "v8datamodel/datamodel.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/FastLogSettings.h"
#include "script/LuaSettings.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/MarketplaceService.h"
#include "v8datamodel/AdService.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "v8datamodel/NonReplicatedCSGDictionaryService.h"
#include "v8datamodel/Message.h"
#include "RobloxServicesTools.h"
#include "script/scriptcontext.h"
#include "ThumbnailGenerator.h"
#include <string>
#include <sstream>
#include "V8DataModel/FactoryRegistration.h"
#include "ThumbnailGenerator.h"
#include "util/profiling.h"
#include "util/SoundService.h"
#include "Util/Guid.h"
#include "Util/Http.h"
#include "Util/Statistics.h"
#include "util/rbxrandom.h"
#include "network/api.h"
#include "VersionInfo.h"
#include "LogManager.h"
#include <queue>
#include "shlobj.h"
#include "DumpErrorUploader.h"
#include "gui/ProfanityFilter.h"
#include "GfxBase/ViewBase.h"
#include "util/FileSystem.h"
#include "Network/Players.h"
#include "v8xml/XmlSerializer.h"
#include "v8xml/WebParser.h"
#include "RobloxServicesTools.h"
#include "util/Utilities.h"
#include "Network/ChatFilter.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "network/WebChatFilter.h"

#include "CountersClient.h"

#include "SimpleJSON.h"
#include "RbxFormat.h"

#include <tlhelp32.h>
#include <psapi.h>
#include "util/Analytics.h"
#include "OperationalSecurity.h"


#define BOOST_DATE_TIME_NO_LIB
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/algorithm/string.hpp>

#include <strsafe.h>

long diagCount=0;
long batchJobCount=0;
long openJobCount=0;
long closeJobCount=0;
long helloWorldCount=0;
long getVersionCount=0;
long renewLeaseCount=0;
long executeCount=0;
long getExpirationCount=0;
long getStatusCount=0;
long getAllJobsCount=0;
long closeExpiredJobsCount=0;
long closeAllJobsCount=0;

//#define DIAGNOSTICS

#ifdef DIAGNOSTICS
#define BEGIN_PRINT(func, msg) \
static int func = 0; \
int counter = func++; \
RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Begin-%s%d\r\n", msg,counter)
#define END_PRINT(func, msg) \
RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "End---%s%d\r\n", msg,counter)
#else
#define BEGIN_PRINT(func, msg)
#define END_PRINT(func, msg)
#endif

LOGVARIABLE(RCCServiceInit, 1);
LOGVARIABLE(RCCServiceJobs, 1);
LOGVARIABLE(RCCDataModelInit, 1);
// See TaskScheduler::ThreadPoolConfig enum for valid values
DYNAMIC_FASTINTVARIABLE(TaskSchedulerThreadCountEnum, 1)

DYNAMIC_FASTFLAGVARIABLE(DebugCrashOnFailToLoadClientSettings, false)

DYNAMIC_FASTFLAGVARIABLE(UseNewSecurityKeyApi, false);
DYNAMIC_FASTFLAGVARIABLE(UseNewMemHashApi, false);
DYNAMIC_FASTSTRINGVARIABLE(MemHashConfig, "");
FASTINTVARIABLE(RCCServiceThreadCount, RBX::TaskScheduler::Threads1)
DYNAMIC_FASTFLAGVARIABLE(US30476, false);
FASTFLAGVARIABLE(UseDataDomain, true);

FASTFLAGVARIABLE(Dep, true)

namespace RBX
{
	class Explosion;
	class Cofm;
	class NormalBreakConnector;
	class ContactConnector;
	class RevoluteLink;
	class SimBody;
	class BallBallContact;
	class BallBlockContact;
	class BlockBlockContact;

}

namespace
{
    static bool isServiceInstalled(const char* svcName);
}

class CrashAfterTimeout
{
	shared_ptr<boost::condition_variable_any> event;
	shared_ptr<boost::mutex> mutex;

	static void run(shared_ptr<boost::condition_variable_any> event, int timeount, shared_ptr<boost::mutex> mutex)
	{
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC_);
		xt.sec += static_cast<boost::xtime::xtime_sec_t>(timeount);
		boost::mutex::scoped_lock lock(*mutex);
		bool set = event->timed_wait(lock, xt);
		if (!set)
			RBXCRASH();
	}
public:
	CrashAfterTimeout(int seconds):event(new boost::condition_variable_any()),mutex(new boost::mutex())
	{
	boost::thread thread(boost::bind(&CrashAfterTimeout::run, event, seconds, mutex));
	}
	~CrashAfterTimeout()
	{
		boost::mutex::scoped_lock lock(*mutex);
		event->notify_all();
	}



};

class RCCServiceSettings : public RBX::FastLogJSON
{
public:
	START_DATA_MAP(RCCServiceSettings);
	DECLARE_DATA_STRING(WindowsMD5);
	DECLARE_DATA_STRING(WindowsPlayerBetaMD5);
	DECLARE_DATA_STRING(MacMD5);
	DECLARE_DATA_INT(SecurityDataTimer);
	DECLARE_DATA_INT(ClientSettingsTimer);
	DECLARE_DATA_STRING(GoogleAnalyticsAccountPropertyID);
	DECLARE_DATA_INT(GoogleAnalyticsThreadPoolMaxScheduleSize);
	DECLARE_DATA_INT(GoogleAnalyticsLoad);
	DECLARE_DATA_BOOL(GoogleAnalyticsInitFix);
	DECLARE_DATA_INT(HttpUseCurlPercentageRCC);
	END_DATA_MAP();
};

class SecurityDataUpdater
{
	std::string apiUrl;

protected:
	std::string data;

	virtual void processDataArray(shared_ptr<const RBX::Reflection::ValueArray> dataArray) = 0;
	bool fetchData()
	{
		std::string newData;
		try 
		{
			RBX::Http request(apiUrl);
			
			request.get(newData);

			// no need to continue if data did not change
			if (newData == data)
				return false;
		}
		catch (std::exception& ex)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "SecurityDataUpdater failed to fetch data from %s, %s", apiUrl.c_str(), ex.what());
			return false;
		}

		data = newData;
		return true;
	}

public:
	SecurityDataUpdater(const std::string& url) : apiUrl(url) {}
	virtual ~SecurityDataUpdater() {}

	void run()
	{
		if (!fetchData())
			return;

		shared_ptr<const RBX::Reflection::ValueTable> jsonResult(rbx::make_shared<const RBX::Reflection::ValueTable>());
		if(RBX::WebParser::parseJSONTable(data, jsonResult))
		{
			RBX::Reflection::ValueTable::const_iterator iter = jsonResult->find("data");
			if (iter != jsonResult->end())
			{
				shared_ptr<const RBX::Reflection::ValueArray> dataArray = iter->second.cast<shared_ptr<const RBX::Reflection::ValueArray> >();
				processDataArray(dataArray);
			}
		}
	}
};

class MD5Updater : public SecurityDataUpdater
{
protected:
	void processDataArray(shared_ptr<const RBX::Reflection::ValueArray> dataArray)
	{
		std::set<std::string> hashes;
		for (RBX::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
		{
			std::string value = it->get<std::string>();
			hashes.insert(value);
		}

		// always add hash for ios
		hashes.insert("ios,ios");

		RBX::Network::Players::setGoldenHashes2(hashes);
	}

public:
	MD5Updater(const std::string& url) : SecurityDataUpdater(url) {}
	~MD5Updater() {}
};

class SecurityKeyUpdater : public SecurityDataUpdater
{
protected:
	void processDataArray(shared_ptr<const RBX::Reflection::ValueArray> dataArray)
	{
		std::vector<std::string> versions;
		for (RBX::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
		{
			// version = data + salt
			std::string value = it->get<std::string>();
			std::string version = RBX::sha1(value + "askljfLUZF");
			versions.push_back(version);
		}

		RBX::Network::setSecurityVersions(versions);
	}

public:
	SecurityKeyUpdater(const std::string& url) : SecurityDataUpdater(url) {}
	~SecurityKeyUpdater() {}
};

class MemHashUpdater : public SecurityDataUpdater
{
public:
    // this is public until we get a web api to do this better.
    static void populateMemHashConfigs(RBX::Network::MemHashConfigs& hashConfigs, const std::string& cfgStr)
    {
        hashConfigs.push_back(RBX::Network::MemHashVector());
        RBX::Network::MemHashVector& thisHashVector = hashConfigs.back();

    	std::vector<std::string> hashStrInfo;
    	boost::split(hashStrInfo, cfgStr, boost::is_any_of(";"));
        for (std::vector<std::string>::const_iterator hpIt = hashStrInfo.begin(); hpIt != hashStrInfo.end(); ++hpIt)
        {
            std::vector<std::string> args;
            boost::split(args, *hpIt, boost::is_any_of(","));
            if (args.size() >= 3)
            {
                RBX::Network::MemHash thisHash;
                thisHash.checkIdx = boost::lexical_cast<unsigned int>(args[0]);
                thisHash.value = boost::lexical_cast<unsigned int>(args[1]);
                thisHash.failMask = boost::lexical_cast<unsigned int>(args[2]);
                thisHashVector.push_back(thisHash);
            }
        }
    };
protected:
	void processDataArray(shared_ptr<const RBX::Reflection::ValueArray> dataArray)
	{
        RBX::Network::MemHashConfigs hashConfigs;
		for (RBX::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
		{
            populateMemHashConfigs(hashConfigs, it->get<std::string>());
		}

		RBX::Network::Players::setGoldMemHashes(hashConfigs);
	}

public:
	MemHashUpdater(const std::string& url) : SecurityDataUpdater(url) {}
	~MemHashUpdater() {}
};


class RCCServiceDynamicSettings : public RBX::FastLogJSON
{
	typedef std::map<std::string, std::string> FVariables;
	FVariables fVars;

public:
	virtual void ProcessVariable(const std::string& valueName, const std::string& valueData, bool dynamic)
	{
		RBXASSERT(dynamic);
		fVars.insert(std::make_pair(valueName, valueData));
	}

	virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData)
	{
		// only process dynamic flags and logs
		if (valueName[0] == 'D')
			return FastLogJSON::DefaultHandler(valueName, valueData);

		return false;
	}

	void UpdateSettings()
	{
		for (FVariables::iterator i = fVars.begin(); i != fVars.end(); i++)
			FLog::SetValue(i->first, i->second, FASTVARTYPE_DYNAMIC, false);
		fVars.clear();
	}
};


DATA_MAP_IMPL_START(RCCServiceSettings)
IMPL_DATA(WindowsMD5, "");
IMPL_DATA(MacMD5, "");
IMPL_DATA(WindowsPlayerBetaMD5, "");
IMPL_DATA(SecurityDataTimer, 300);	// in seconds, default 5 min
IMPL_DATA(ClientSettingsTimer, 120);	// 2 min
IMPL_DATA(GoogleAnalyticsAccountPropertyID, "UA-43420590-13"); // Google Analytics Game Server Analytics Test account
IMPL_DATA(GoogleAnalyticsThreadPoolMaxScheduleSize, 500); // max items to be scheduled at any one time by the G.A. threadpool
IMPL_DATA(GoogleAnalyticsLoad, 10); // percent probability of using google analytics
IMPL_DATA(GoogleAnalyticsInitFix, true);
IMPL_DATA(HttpUseCurlPercentageRCC, 0); // do not use CURL by default
DATA_MAP_IMPL_END()

static boost::scoped_ptr<MainLogManager> mainLogManager(new MainLogManager("Roblox Web Service", ".dmp", ".crashevent"));

#ifdef RBX_TEST_BUILD
std::string RCCServiceSettingsKeyOverwrite;
#endif

class CWebService
{
	boost::shared_ptr<CProcessPerfCounter> s_perfCounter;
	boost::shared_ptr<RBX::ProfanityFilter> s_profanityFilter;
	boost::scoped_ptr<boost::thread> perfData;
	boost::scoped_ptr<boost::thread> fetchSecurityDataThread;
	boost::scoped_ptr<boost::thread> fetchClientSettingsThread;
	ATL::CEvent doneEvent;
	RCCServiceSettings rccSettings;
	RCCServiceDynamicSettings rccDynamicSettings;
	std::string settingsKey;
	std::string securityVersionData;
	std::string clientSettingsData;

	std::string thumbnailSettingsData;
    bool isThumbnailer;

	boost::scoped_ptr<MD5Updater> md5Updater;
	boost::scoped_ptr<SecurityDataUpdater> securityKeyUpdater;
	boost::scoped_ptr<MemHashUpdater> memHashUpdater;

	boost::scoped_ptr<CountersClient> counters;

public:
	struct JobItem : boost::noncopyable
	{
		enum JobItemRunStatus { RUNNING_JOB, JOB_DONE, JOB_ERROR };
		const std::string id;
		shared_ptr<RBX::DataModel> dataModel;
		RBX::Time expirationTime;
		int category;
		double cores;
		ATL::CEvent jobCheckLeaseEvent;
		rbx::signals::connection notifyAliveConnection;

		JobItemRunStatus status;
		std::string errorMessage;

		JobItem(const char* id):id(id),jobCheckLeaseEvent(TRUE, FALSE), status(RUNNING_JOB) {}

		void touch(double seconds);
		double secondsToTimeout() const;
	};

	typedef std::map<std::string, boost::shared_ptr<JobItem> > JobMap;
	JobMap jobs;
	boost::mutex sync;
	long dataModelCount;
	boost::mutex currentlyClosingMutex;
public:
	CWebService(bool crashUploadOnly);
	~CWebService();
private:
	void CWebService::collectPerfData();
	void LoadAppSettings();
	void LoadClientSettings(RCCServiceSettings& dest);
	void LoadClientSettings(std::string& clientDest, std::string& thumbnailDest);
	std::string GetSettingsKey();

	std::vector<std::string> fetchAllowedSecurityVersions();
public:
	static boost::scoped_ptr<CWebService> singleton;
	JobMap::iterator getJob(const std::string& jobID)
	{
		JobMap::iterator iter = jobs.find(jobID);
		if (iter==jobs.end())
			throw RBX::runtime_error("JobItem %s not found", jobID.c_str());
		return iter;
	}

	void validateSecurityData()
	{
		while (::WaitForSingleObject(doneEvent.m_h, rccSettings.GetValueSecurityDataTimer() * 1000) == WAIT_TIMEOUT) 
		{
			if (DFFlag::UseNewSecurityKeyApi)
			{
				if (securityKeyUpdater)
					securityKeyUpdater->run();
			}
			else
			{
				std::string prevVersionData = securityVersionData;
				std::vector<std::string> versions = fetchAllowedSecurityVersions();

				// security versions changed, update all jobs with new versions
				if (prevVersionData != securityVersionData)
					RBX::Network::setSecurityVersions(versions);
			}

	        if (DFFlag::UseNewMemHashApi)
	        {
                if (memHashUpdater)
                {
                    if (DFString::MemHashConfig.size() < 3)
                    {
		                memHashUpdater->run();
                    }
                    else
                    {
                        RBX::Network::MemHashConfigs hashConfigs;
                        MemHashUpdater::populateMemHashConfigs(hashConfigs, DFString::MemHashConfig);
		                RBX::Network::Players::setGoldMemHashes(hashConfigs);
                    }
                }
	        }

			if (md5Updater)
				md5Updater->run();

		}
	}

	void validateClientSettings()
	{
		while (::WaitForSingleObject(doneEvent.m_h, rccSettings.GetValueClientSettingsTimer() * 1000) == WAIT_TIMEOUT) 
		{
			std::string prevClientSettings = clientSettingsData;
			std::string prevThumbnailSettings = thumbnailSettingsData;

			LoadClientSettings(clientSettingsData, thumbnailSettingsData);

            bool clientChanged = (prevClientSettings != clientSettingsData);
            bool thumbnailChanged = isThumbnailer && (prevThumbnailSettings != thumbnailSettingsData);
			if (clientChanged || thumbnailChanged)
			{
				// collect all dynamic settings
				rccDynamicSettings.ReadFromStream(clientSettingsData.c_str());
                if (isThumbnailer)
                {
				    rccDynamicSettings.ReadFromStream(thumbnailSettingsData.c_str());
                }
				
				// submit datamodel write task to update all dynamic settings
				boost::mutex::scoped_lock lock(sync);
				if (jobs.size() > 0)
				{
					// HACK: Client settings are global, meaning all datamodels use the same set,
					// current we only have 1 datamodel running per rccservice, so submit write task just on first datamodel
					shared_ptr<JobItem> job = jobs.begin()->second;
					job->dataModel->submitTask(boost::bind(&RCCServiceDynamicSettings::UpdateSettings, &rccDynamicSettings), RBX::DataModelJob::Write);
				}
				else
					rccDynamicSettings.UpdateSettings();
			}
		}
	}

	void doCheckLease(boost::shared_ptr<JobItem> job)
	{
		try
		{
			while (true)
			{
				double sleepTimeInSeconds = job->secondsToTimeout();
				if (sleepTimeInSeconds<=0)
				{
					closeJob(job->id);
					return;
				}
				else if (::WaitForSingleObject(job->jobCheckLeaseEvent.m_h, (DWORD)(sleepTimeInSeconds * 1000.0) + 3)!=WAIT_TIMEOUT)
					return;
			}
		}
		catch (std::exception& e)
		{
			assert(false);
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "doCheckLease failure: %s", e.what());
		}
	}

	void renewLease(const std::string& id, double expirationInSeconds)
	{
		boost::mutex::scoped_lock lock(sync);
		getJob(id)->second->touch(expirationInSeconds);
	}

	static void convertToLua(const RBX::Reflection::Variant& source, ns1__LuaValue& dest, soap* soap)
	{
		assert(dest.type == ns1__LuaType__LUA_USCORETNIL);
		assert(dest.value == NULL);

		if (source.isType<void>())
		{
			dest.type = ns1__LuaType__LUA_USCORETNIL;
			return;
		}

		if (source.isType<bool>())
		{
			dest.type = ns1__LuaType__LUA_USCORETBOOLEAN;
			dest.value = soap_new_std__string(soap, -1);
			*dest.value = source.get<std::string>();
			return;
		}

		if (source.isNumber())
		{
			dest.type = ns1__LuaType__LUA_USCORETNUMBER;
			dest.value = soap_new_std__string(soap, -1);
			*dest.value = source.get<std::string>();
			return;
		}

		if (source.isType<std::string>())
		{
			dest.type = ns1__LuaType__LUA_USCORETSTRING;
			dest.value = soap_new_std__string(soap, -1);
			*dest.value = source.get<std::string>();
			return;
		}

		if (source.isType<RBX::ContentId>())
		{
			dest.type = ns1__LuaType__LUA_USCORETSTRING;
			dest.value = soap_new_std__string(soap, -1);
			*dest.value = source.get<std::string>();
			return;
		}

		if (source.isType<shared_ptr<const RBX::Reflection::ValueArray> >())
		{
			shared_ptr<const RBX::Reflection::ValueArray> collection = source.cast<shared_ptr<const RBX::Reflection::ValueArray> >();
			dest.type = ns1__LuaType__LUA_USCORETTABLE;
			dest.table = soap_new_ns1__ArrayOfLuaValue(soap, -1);
			dest.table->LuaValue.resize(collection->size());
			for (size_t i = 0; i<collection->size(); ++i)
			{
				dest.table->LuaValue[i] = soap_new_ns1__LuaValue(soap, -1);
				convertToLua((*collection)[i], *dest.table->LuaValue[i], soap);
			}
			return;
		}

		// TODO: enums!
		dest.type = ns1__LuaType__LUA_USCORETNIL;
	}

	static void convert(const ns1__LuaValue* source, RBX::Reflection::Variant& dest)
	{
		switch (source->type)
		{
		case ns1__LuaType__LUA_USCORETNIL:
			break;

		case ns1__LuaType__LUA_USCORETNUMBER:
			dest = *source->value;
			dest.convert<double>();
			break;

		case ns1__LuaType__LUA_USCORETBOOLEAN:
			dest = *source->value;
			dest.convert<bool>();
			break;

		case ns1__LuaType__LUA_USCORETSTRING:
			dest = *source->value;
			break;

		case ns1__LuaType__LUA_USCORETTABLE:
			{
				const size_t count = source->table->LuaValue.size();

				// Create a collection so that we can populate it
				shared_ptr<RBX::Reflection::ValueArray> table(new RBX::Reflection::ValueArray(count));
				
				for (size_t i=0; i<count; ++i)
					convert(source->table->LuaValue[i], (*table)[i]);

				// Set the value to a ValueArray type
				dest = shared_ptr<const RBX::Reflection::ValueArray>(table);
			}
			break;
		}
	}

	// Gathers all non-global Instances that are in the heap
	void arbiterActivityDump(RBX::Reflection::ValueArray& result)
	{
		boost::mutex::scoped_lock lock(sync);
		for (JobMap::iterator iter = jobs.begin(); iter != jobs.end(); ++iter)
		{
			shared_ptr<RBX::DataModel> dataModel = iter->second->dataModel;
			shared_ptr<RBX::Reflection::ValueArray> tuple(new RBX::Reflection::ValueArray());
			tuple->push_back(dataModel->arbiterName());
			tuple->push_back(dataModel->getAverageActivity());
			result.push_back(shared_ptr<const RBX::Reflection::ValueArray>(tuple));
		}
	}

	// Gathers all non-global Instances that are in the heap
	void leakDump(RBX::Reflection::ValueArray& result)
	{
	}

	// Gathers diagnostic data. Does NOT require locks on datamodel :)
	const RBX::Reflection::ValueArray* diag(int type, shared_ptr<RBX::DataModel> dataModel)
	{
		std::auto_ptr<RBX::Reflection::ValueArray> tuple(new RBX::Reflection::ValueArray());
		
		/* This is the format of the Diag data:

			type == 0
				DataModel Count in this process
				PerfCounter data
				Task Scheduler
					(obsolete entry)
					double threadAffinity
					double numQueuedJobs
					double numScheduledJobs
					double numRunningJobs
					long threadPoolSize
					double messageRate
					double messagePumpDutyCycle					
				DataModel Jobs Info
				Machine configuration
				Memory Leak Detection
			type & 1
				leak dump
			type & 2
				attempt to allocate 500k. if success, then true else false
			type & 4
				DataModel dutyCycles
		*/
		tuple->push_back(dataModelCount);

		{
			shared_ptr<RBX::Reflection::ValueArray> perfCounterData(new RBX::Reflection::ValueArray());
			perfCounterData->push_back(s_perfCounter->GetProcessCores());
			perfCounterData->push_back(s_perfCounter->GetTotalProcessorTime());
			perfCounterData->push_back(s_perfCounter->GetProcessorTime());
			perfCounterData->push_back(s_perfCounter->GetPrivateBytes());
			perfCounterData->push_back(s_perfCounter->GetPrivateWorkingSetBytes());
			perfCounterData->push_back(-1);
			perfCounterData->push_back(-1);
			perfCounterData->push_back(s_perfCounter->GetElapsedTime());
			perfCounterData->push_back(s_perfCounter->GetVirtualBytes());
			perfCounterData->push_back(s_perfCounter->GetPageFileBytes());
			perfCounterData->push_back(s_perfCounter->GetPageFaultsPerSecond());

			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(perfCounterData));
		}
		{
			shared_ptr<RBX::Reflection::ValueArray> taskSchedulerData(new RBX::Reflection::ValueArray());
			taskSchedulerData->push_back(0);	// obsolete
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().threadAffinity());
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().numSleepingJobs());
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().numWaitingJobs());
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().numRunningJobs());
			taskSchedulerData->push_back((long) RBX::TaskScheduler::singleton().threadPoolSize());
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().schedulerRate());
			taskSchedulerData->push_back(RBX::TaskScheduler::singleton().getSchedulerDutyCyclePerThread());
			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(taskSchedulerData));
		}

		if (dataModel)
			tuple->push_back(dataModel->getJobsInfo());
		else
			tuple->push_back(0);

		{
			shared_ptr<RBX::Reflection::ValueArray> machineData(new RBX::Reflection::ValueArray());
			machineData->push_back(RBX::DebugSettings::singleton().totalPhysicalMemory());
			machineData->push_back(RBX::DebugSettings::singleton().cpu());
			machineData->push_back(RBX::DebugSettings::singleton().cpuCount());
			machineData->push_back(RBX::DebugSettings::singleton().cpuSpeed());
			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(machineData));
		}

		{
            // TODO, use RBX::poolAllocationList and RBX::poolAvailablityList to get complete stats
			shared_ptr<RBX::Reflection::ValueArray> memCounters(new RBX::Reflection::ValueArray());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::Instance>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::PartInstance>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::ModelInstance>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::Explosion>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::Soundscape::SoundChannel>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<rbx::signals::connection>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<rbx::signals::connection::islot>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::Reflection::GenericSlotWrapper>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::TaskScheduler::Job>::getCount());
			memCounters->push_back(RBX::Diagnostics::Countable<RBX::Network::Player>::getCount());
#ifdef RBX_ALLOCATOR_COUNTS
			memCounters->push_back(RBX::Allocator<RBX::Body>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::Cofm>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::NormalBreakConnector>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::ContactConnector>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::RevoluteLink>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::SimBody>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::BallBallContact>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::BallBlockContact>::getCount());
			memCounters->push_back(RBX::Allocator<RBX::BlockBlockContact>::getCount());
			memCounters->push_back(RBX::Allocator<XmlAttribute>::getCount());
			memCounters->push_back(RBX::Allocator<XmlElement>::getCount());
#else
			for(int i = 0; i<11; ++i)
				memCounters->push_back(-1);
#endif
			memCounters->push_back((int)ThumbnailGenerator::totalCount);
			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(memCounters));
		}

		if (type & 1)
		{
			shared_ptr<RBX::Reflection::ValueArray> result(new RBX::Reflection::ValueArray());
			leakDump(*result);
			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(result));
		}

		if (type & 2)
		{
			try
			{
				{
					std::vector<char> v1(100000);
					std::vector<char> v2(100000);
					std::vector<char> v3(100000);
					std::vector<char> v4(100000);
					std::vector<char> v5(100000);
				}
				tuple->push_back(true);
			}
			catch (...)
			{
				tuple->push_back(false);
			}
		}

		if (type & 4)
		{
			shared_ptr<RBX::Reflection::ValueArray> result(new RBX::Reflection::ValueArray());
			arbiterActivityDump(*result);
			tuple->push_back(shared_ptr<const RBX::Reflection::ValueArray>(result));
		}

		return tuple.release();
	}

	void execute(const std::string& jobID, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*> * result, soap* soap)
	{
		std::string code = *script->script;
		if (RBX::ContentProvider::isHttpUrl(code))
		{
			RBX::Http http(code);
			http.get(code);
		}

		shared_ptr<RBX::DataModel> dataModel;
		{
			boost::mutex::scoped_lock lock(sync);
			JobMap::iterator iter = getJob(jobID);
			dataModel = iter->second->dataModel;
		}

		std::auto_ptr<const RBX::Reflection::Tuple> tuple;
		{
			const size_t count = script->arguments ? script->arguments->LuaValue.size() : 0;
			RBX::Reflection::Tuple args(count);
			for (size_t i=0; i<count; ++i)
				convert(script->arguments->LuaValue[i], args.values[i]);

			RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
			if (dataModel->isClosed())
				throw std::runtime_error("The DataModel is closed");
			RBX::ScriptContext* scriptContext = RBX::ServiceProvider::create<RBX::ScriptContext>(dataModel.get());
			tuple = scriptContext->executeInNewThread(RBX::Security::WebService, RBX::ProtectedString::fromTrustedSource(code), script->name->c_str(), args);
		}

		result->resize(tuple->values.size());
		for (size_t i = 0; i<tuple->values.size(); ++i)
		{
			(*result)[i] = soap_new_ns1__LuaValue(soap, -1);
			convertToLua(tuple->values[i], *(*result)[i], soap);
		}
	}

	void diag(int type, std::string jobID, std::vector<ns1__LuaValue*>* result, soap* soap)
	{
		std::auto_ptr<const RBX::Reflection::ValueArray> tuple;

		{
			shared_ptr<RBX::DataModel> dataModel;
			if (!jobID.empty())
			{
				boost::mutex::scoped_lock lock(sync);
				JobMap::iterator iter = getJob(jobID);
				dataModel = iter->second->dataModel;
			}
			tuple.reset(diag(type, dataModel));
		}

		result->resize(tuple->size());
		for (size_t i = 0; i<tuple->size(); ++i)
		{
			(*result)[i] = soap_new_ns1__LuaValue(soap, -1);
			convertToLua((*tuple)[i], *(*result)[i], soap);
		}
	}

	void contentDataLoaded(shared_ptr<RBX::DataModel>& dataModel)
	{
		RBX::CSGDictionaryService* dictionaryService = RBX::ServiceProvider::create<RBX::CSGDictionaryService>(dataModel.get());
		RBX::NonReplicatedCSGDictionaryService* nrDictionaryService = RBX::ServiceProvider::create<RBX::NonReplicatedCSGDictionaryService>(dataModel.get());

		dictionaryService->reparentAllChildData();
	}

	void setupServerConnections(RBX::DataModel* dataModel)
	{
		if (!dataModel)
		{
			return;
		}

		if (RBX::AdService* adService = dataModel->create<RBX::AdService>())
		{
			adService->sendServerVideoAdVerification.connect(boost::bind(&RBX::AdService::checkCanPlayVideoAd,adService,_1, _2));
			adService->sendServerRecordImpression.connect(boost::bind(&RBX::AdService::sendAdImpression,adService,_1,_2,_3));
		}
	}

	shared_ptr<JobItem> createJob(const ns1__Job& job, bool startHeartbeat, shared_ptr<RBX::DataModel>& dataModel)
	{
		srand(RBX::randomSeed()); // make sure this thread is seeded
		std::string id = job.id;

		dataModel = RBX::DataModel::createDataModel(startHeartbeat, new RBX::NullVerb(NULL,""), false);
        setupServerConnections(dataModel.get());

		RBX::Network::Players* players = dataModel->find<RBX::Network::Players>();
		if(players)
		{
			LoadClientSettings(rccSettings);
            if (rccSettings.GetError())
            {
                RBXCRASH(rccSettings.GetErrorString().c_str());
            }

			{
				bool useCurl = rand() % 100 < rccSettings.GetValueHttpUseCurlPercentageRCC();
				FASTLOG1(FLog::RCCDataModelInit, "Using CURL = %d", useCurl);
				RBX::Http::SetUseCurl(useCurl);

                RBX::Http::SetUseStatistics(true);
			}

			FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Windows MD5: %s", rccSettings.GetValueWindowsMD5());
			FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Mac MD5: %s", rccSettings.GetValueMacMD5());
			FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Windows Player Beta MD5: %s", rccSettings.GetValueWindowsPlayerBetaMD5());
			players->setGoldenHashes(rccSettings.GetValueWindowsMD5(), rccSettings.GetValueMacMD5(), rccSettings.GetValueWindowsPlayerBetaMD5());

			if (rccSettings.GetValueGoogleAnalyticsInitFix())
			{
				RBX::Analytics::GoogleAnalytics::lotteryInit(rccSettings.GetValueGoogleAnalyticsAccountPropertyID(), rccSettings.GetValueGoogleAnalyticsLoad());
			}
			else
			{
				int lottery = rand() % 100;
				FASTLOG1(FLog::RCCDataModelInit, "Google analytics lottery number = %d", lottery);
				if (lottery < rccSettings.GetValueGoogleAnalyticsLoad())
				{
					FASTLOG1(FLog::RCCDataModelInit, "Setting Google Analytics ThreadPool Max Schedule Size: %d", rccSettings.GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());
					FASTLOGS(FLog::RCCDataModelInit, "Setting Google Analytics Account Property ID: %s", rccSettings.GetValueGoogleAnalyticsAccountPropertyID());
					RBX::RobloxGoogleAnalytics::setCanUseAnalytics();
					RBX::RobloxGoogleAnalytics::init(rccSettings.GetValueGoogleAnalyticsAccountPropertyID(), rccSettings.GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());
				}
			}

			RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
			dataModel->create<RBX::Network::WebChatFilter>();
		}

		dataModel->workspaceLoadedSignal.connect(boost::bind(&CWebService::contentDataLoaded, this, dataModel));

		dataModel->jobId = id;
		shared_ptr<JobItem> j(new JobItem(id.c_str()));
		j->dataModel = dataModel;
		j->category = job.category;
		j->cores = job.cores;
		j->touch(job.expirationInSeconds);

		{
			boost::mutex::scoped_lock lock(sync);
			if (jobs.find(id)!=jobs.end())
				throw RBX::runtime_error("JobItem %s already exists", id.c_str());
			jobs[id] = j;
		}

		return j;
	}

	void batchJob(const ns1__Job& job, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap)
	{
		std::string id = job.id;

		shared_ptr<RBX::DataModel> dataModel;
		::InterlockedIncrement(&dataModelCount);
		try
		{
			shared_ptr<JobItem> j = createJob(job, false, dataModel);

			FASTLOGS(FLog::RCCServiceJobs, "Opened Batch JobItem %s", id.c_str());
			FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

			//Spin off a thread for the BatchJob to execute in
			boost::thread(RBX::thread_wrapper(boost::bind(&CWebService::asyncExecute, this, id, script, result, soap), "AsyncExecute"));

			while (true)
			{
				double sleepTimeInSeconds = j->secondsToTimeout();

				if (sleepTimeInSeconds <= 0) // This case seems like an edge case (we already ran out of time)
				{
					if (::WaitForSingleObject(j->jobCheckLeaseEvent.m_h, 1)!=WAIT_TIMEOUT){
						if(j->status == JobItem::JOB_ERROR)
							throw RBX::runtime_error(j->errorMessage.c_str());
					}

					closeJob(id);
					throw RBX::runtime_error("BatchJob Timeout");
				}
				else if (::WaitForSingleObject(j->jobCheckLeaseEvent.m_h, (DWORD)(sleepTimeInSeconds * 1000.0) + 3) == WAIT_TIMEOUT)
				{
					// do nothing, just continue looping. Probably sleepTimeInSeconds will be negative, and we'll throw
				}
				else
				{
					// jobCheckLeaseEvent was set
					if(j->status == JobItem::JOB_ERROR)
						throw RBX::runtime_error(j->errorMessage.c_str());

					// TODO: Shouldn't this be called BEFORE we throw??????
					closeJob(id);
					return;
				}
			}
		}
		catch (std::exception&)
		{
			throw;
		}
	}

	void asyncExecute(const std::string& id, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*> * result, soap* soap)
	{
		try
		{
			this->execute(id, script, result, soap);
			closeJob(id);
		}
		catch (std::exception& e)
		{
			char szMsg[1024];
			StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());

			closeJob(id, szMsg);
		}
	}
	void openJob(const ns1__Job& job, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap, bool startHeartbeat)
	{
		std::string id = job.id;
		RBX::Http::gameID = job.id;

		shared_ptr<RBX::DataModel> dataModel;
		::InterlockedIncrement(&dataModelCount);
		try
		{
			shared_ptr<JobItem> j = createJob(job, startHeartbeat, dataModel);

			try
			{
				RBX::DataModel *pDataModel = dataModel.get();

				// Monitor the job and close it if needed
				boost::thread(RBX::thread_wrapper(boost::bind(&CWebService::doCheckLease, this, j), "Check Expiration"));

				FASTLOGS(FLog::RCCServiceJobs, "Opened JobItem %s", id.c_str());
				FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", pDataModel);

				this->execute(id, script, result, soap);

				RBX::RunService* runService = RBX::ServiceProvider::find<RBX::RunService>(pDataModel);
				RBXASSERT(runService != NULL);
				j->notifyAliveConnection = runService->heartbeatSignal.connect(boost::bind(&CWebService::notifyAlive, this, _1));

				{
					RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);
					pDataModel->loadCoreScripts();
				}
			}
			catch (std::exception& e)
			{
				char szMsg[1024];
				StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());

				boost::mutex::scoped_lock lock(sync);
				jobs.erase(id);
				throw;
			}
		}
		catch (std::exception& e)
		{
			char szMsg[1024];
			StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());

			FASTLOGS(FLog::RCCServiceJobs, "Closing DataModel due to exception: %s", szMsg);
			FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

			closeDataModel(dataModel);
			dataModel.reset();
			::InterlockedDecrement(&dataModelCount);
			throw;
		}
	}

	void closeDataModel(shared_ptr<RBX::DataModel> dataModel)
	{
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		//CrashAfterTimeout crash(90);	// If after 90 seconds the datamodel doesn't close, then CRASH!!!!
		RBX::DataModel::closeDataModel(dataModel);
	}

	void notifyAlive(const RBX::Heartbeat& h)
	{
		mainLogManager->NotifyFGThreadAlive();
	}
	
	void closeJob(const std::string& jobID, const char* errorMessage = NULL)
	{
		// Take a mutex for the duration of closeJob here. The arbiter thinks that as 
		// soon as closeJob returns it is safe to force kill the rcc process, so make
		// sure that if it is in the process of closing the datamodel any parallel requests
		// to close the data model block at the mutex until the first one finishes.
		boost::mutex::scoped_lock closeLock(currentlyClosingMutex);

		shared_ptr<RBX::DataModel> dataModel;
		{
			boost::mutex::scoped_lock lock(sync);
			JobMap::iterator iter = jobs.find(jobID);
			if (iter != jobs.end())
			{
				if(errorMessage){
					iter->second->errorMessage = errorMessage;
					iter->second->status = JobItem::JOB_ERROR;
				}
				else{
					iter->second->status = JobItem::JOB_DONE;
				}
				iter->second->jobCheckLeaseEvent.Set();
				dataModel = iter->second->dataModel;
				iter->second->notifyAliveConnection.disconnect();
				jobs.erase(iter);
			}
		}

		if (dataModel)
		{
			FASTLOGS(FLog::RCCServiceJobs, "Closing JobItem %s", jobID.c_str());
			FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

			closeDataModel(dataModel);
			dataModel.reset();
			if (::InterlockedDecrement(&dataModelCount) == 0)
				mainLogManager->DisableHangReporting();
		}
	}

	int jobCount() {
		return jobs.size();
	}


	void getExpiration(const std::string& jobID, double * timeout)
	{
		boost::mutex::scoped_lock lock(sync);
		*timeout = getJob(jobID)->second->secondsToTimeout();
	}

	void closeExpiredJobs(int* result)
	{
		std::vector<std::string> jobsToClose;
		{
			boost::mutex::scoped_lock lock(sync);
			JobMap::iterator end = jobs.end();
			for (JobMap::iterator iter = jobs.begin(); iter!=end; ++iter)
				if (iter->second->secondsToTimeout()<=0)
					jobsToClose.push_back(iter->first);
		}
		*result = jobsToClose.size();
		std::for_each(jobsToClose.begin(), jobsToClose.end(), boost::bind(&CWebService::closeJob, this, _1, (const char*)NULL));
	}

	void closeAllJobs(int* result)
	{
		FASTLOG(FLog::RCCServiceJobs, "Closing all jobs command");

		std::vector<std::string> jobsToClose;
		{
			boost::mutex::scoped_lock lock(sync);
			JobMap::iterator end = jobs.end();
			for (JobMap::iterator iter = jobs.begin(); iter!=end; ++iter)
				jobsToClose.push_back(iter->first);
		}
		*result = jobsToClose.size();
		std::for_each(jobsToClose.begin(), jobsToClose.end(), boost::bind(&CWebService::closeJob, this, _1, (const char*)NULL));
	}

	void getAllJobs(std::vector<ns1__Job * >& result, soap* soap)
	{
		boost::mutex::scoped_lock lock(sync);
		result.resize(jobs.size());

		int i = 0;
		JobMap::iterator iter = jobs.begin();
		JobMap::iterator end = jobs.end();
		while (iter!=end)
		{
			ns1__Job* job = soap_new_ns1__Job(soap, -1);
			job->expirationInSeconds = iter->second->secondsToTimeout();
			job->category = iter->second->category;
			job->cores = iter->second->cores;
			job->id = iter->first;
			result[i] = job;
			++iter;
			++i;
		}
	}


};

boost::scoped_ptr<CWebService> CWebService::singleton;

void stop_CWebService()
{
	CWebService::singleton.reset();
}

void start_CWebService(LPCTSTR contentpath, bool crashUploaderOnly)
{
	if(PathIsRelative(contentpath))
	{
		TCHAR name[500];
		::GetModuleFileName(_AtlBaseModule.m_hInst, name, 500);
		CPath path = name;
		path.RemoveFileSpec();
		RBX::ContentProvider::setAssetFolder(path.m_strPath + "\\" + contentpath);
	}
	else
	{
		RBX::ContentProvider::setAssetFolder(contentpath);
	}

	CWebService::singleton.reset(new CWebService(crashUploaderOnly));
}

RBX_REGISTER_CLASS(ThumbnailGenerator);

void CWebService::collectPerfData()
{
	while (::WaitForSingleObject(doneEvent.m_h, 1000)==WAIT_TIMEOUT) // used as an interruptible sleep.
		s_perfCounter->CollectData();
}

CWebService::CWebService(bool crashUploadOnly) :
   doneEvent(TRUE, FALSE),
   dataModelCount(0)
{
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Intializing Roblox Web Service");

	{
		CVersionInfo vi;
		vi.Load(_AtlBaseModule.m_hInst);
		RBX::DebugSettings::robloxVersion = vi.GetFileVersionAsDotString();

		RBX::Analytics::setReporter("RCCService");
		RBX::Analytics::setAppVersion(vi.GetFileVersionAsString());
	}

	s_perfCounter = CProcessPerfCounter::getInstance();
	s_profanityFilter = RBX::ProfanityFilter::getInstance();
	perfData.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&CWebService::collectPerfData, this), "CWebService::collectPerfData")));

	RBX::Http::init(RBX::Http::WinHttp, RBX::Http::CookieSharingSingleProcessMultipleThreads);
	RBX::Http::requester = "Server";

	RBX::Profiling::init(false);
	static RBX::FactoryRegistrator registerFactoryObjects; // this needs to be here so srand is called before rand

	RBX::Analytics::InfluxDb::init();	// calls rand()

	RobloxCrashReporter::silent = true;
	mainLogManager->WriteCrashDump();

    isThumbnailer = ::isServiceInstalled("Roblox.Thumbnails.Relay");
	LoadAppSettings();

	LoadClientSettings(rccSettings);
    if (rccSettings.GetError())
    {
        RBXCRASH(rccSettings.GetErrorString().c_str());
    }

	RBX::TaskSchedulerSettings::singleton();
	RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskScheduler::ThreadPoolConfig(FInt::RCCServiceThreadCount));

	// Force loading of settings classes
	RBX::GameSettings::singleton();
	RBX::LuaSettings::singleton();
	RBX::DebugSettings::singleton();
	RBX::PhysicsSettings::singleton();

	RBX::Soundscape::SoundService::soundDisabled = true;

	// Initialize the network code
	RBX::Network::initWithServerSecurity();

	//If crashUploadOnly = true, don't create a separate thread of control for uploading
	static DumpErrorUploader dumpErrorUploader(!crashUploadOnly, "RCCService");

	std::string dmpHandlerUrl = GetGridUrl(::GetBaseURL(), FFlag::UseDataDomain);
	dumpErrorUploader.InitCrashEvent(dmpHandlerUrl, mainLogManager->getCrashEventName());
	dumpErrorUploader.Upload(dmpHandlerUrl);


	fetchClientSettingsThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&CWebService::validateClientSettings, this), "CWebService::validateClientSettings")));

	// load and set security versions immediately at start up so it's guaranteed to be there when server is launched
	securityKeyUpdater.reset(new SecurityKeyUpdater(GetSecurityKeyUrl2(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));
	md5Updater.reset(new MD5Updater(GetMD5HashUrl(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));
	memHashUpdater.reset(new MemHashUpdater(GetMemHashUrl(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));

	if (DFFlag::UseNewSecurityKeyApi)
	{
		securityKeyUpdater->run();
	}
	else
	{
		std::vector<std::string> versions = fetchAllowedSecurityVersions();
		RBX::Network::setSecurityVersions(versions);
	}

	md5Updater->run();

	if (DFFlag::UseNewMemHashApi)
	{
        if (DFString::MemHashConfig.size() < 3)
        {
		    memHashUpdater->run();
        }
        else
        {
            RBX::Network::MemHashConfigs hashConfigs;
            MemHashUpdater::populateMemHashConfigs(hashConfigs, DFString::MemHashConfig);
		    RBX::Network::Players::setGoldMemHashes(hashConfigs);
        }
	}

	// this thread uses client setting values, so it must be started AFTER client settings are loaded
	// create a thread to periodically check for security key changes
	fetchSecurityDataThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&CWebService::validateSecurityData, this), "CWebService::validateSecurityData")));

	counters.reset(new CountersClient(GetBaseURL().c_str(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", NULL));
	
	RBX::ViewBase::InitPluginModules();

    if (DFFlag::US30476)
    {
        RBX::initAntiMemDump();
    }
    RBX::initLuaReadOnly();
    RBX::initHwbpVeh();

    if (FFlag::Dep)
    {
        typedef BOOL (WINAPI *SetProcessDEPPolicyPfn)(DWORD);
        SetProcessDEPPolicyPfn pfn= reinterpret_cast<SetProcessDEPPolicyPfn>(GetProcAddress(GetModuleHandle("Kernel32"), "SetProcessDEPPolicy"));
        if (pfn)
        {
            static const DWORD kEnable = 1;
            pfn(kEnable);
        }
    }
}

CWebService::~CWebService()
{
	doneEvent.Set();
	perfData->join();
	fetchSecurityDataThread->join();
	fetchClientSettingsThread->join();

	// TODO: this line crashes sometimes :-(
	int result;
	closeAllJobs(&result);

	RBX::ViewBase::ShutdownPluginModules();
}

std::string CWebService::GetSettingsKey()
{
#ifdef RBX_TEST_BUILD
	if (RCCServiceSettingsKeyOverwrite.length() > 0)
		return RCCServiceSettingsKeyOverwrite;
#endif

	if (settingsKey.length() == 0)
	{
		CRegKey key;
		if (SUCCEEDED(key.Open(HKEY_LOCAL_MACHINE, "Software\\ROBLOX Corporation\\Roblox\\", KEY_READ))) 
		{
			CHAR keyData[MAX_PATH];
			ULONG bufLen = MAX_PATH-1;
			if (SUCCEEDED(key.QueryStringValue("SettingsKey", keyData, &bufLen))) 
			{
				keyData[bufLen] = 0;
				settingsKey = std::string(keyData);
				FASTLOGS(FLog::RCCServiceInit, "Read settings key: %s", settingsKey);
			}
		}
		
		if (settingsKey.length() != 0)
			settingsKey = "RCCService" + settingsKey;
	}

	return settingsKey;
}
void CWebService::LoadClientSettings(RCCServiceSettings& dest)
{
	// cache settings in a string before processing
	LoadClientSettings(clientSettingsData, thumbnailSettingsData);
	LoadClientSettingsFromString(GetSettingsKey().c_str(), clientSettingsData, &dest);
    if (isThumbnailer)
    {
	    LoadClientSettingsFromString("RccThumbnailers", thumbnailSettingsData, &dest);
    }
}

void CWebService::LoadClientSettings(std::string& clientDest, std::string& thumbnailDest)
{
	clientDest.clear();
	thumbnailDest.clear();

	std::string key = GetSettingsKey();
    if (key.length() != 0)
	{
		FetchClientSettingsData(key.c_str(), "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", &clientDest);
	}
    if (isThumbnailer)
    {
	    FetchClientSettingsData("RccThumbnailers", "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", &thumbnailDest);
    }

    bool invalidClientSettings = (clientDest.empty() || key.length() == 0);
    bool invalidThumbnailerSettings = (thumbnailDest.empty() && isThumbnailer);
	if (invalidClientSettings || invalidThumbnailerSettings)
	{
		// hack: we want to log failure to load client settings in GA, but GA init require client setting...
		// so just init GA with default settings, which points to "test" account
		RBX::RobloxGoogleAnalytics::setCanUseAnalytics();
		RBX::RobloxGoogleAnalytics::init(rccSettings.GetValueGoogleAnalyticsAccountPropertyID(), rccSettings.GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());

		TCHAR computerName[1024];
		DWORD size = 1024;
		GetComputerName(computerName, &size);

        if (invalidClientSettings)
        {
		    RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, key.length() ? "Empty settings string" : "Empty settings key", computerName);
        }
        else
        {
		    RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "Empty thumbnailer settings string", computerName);
        }

		if (DFFlag::DebugCrashOnFailToLoadClientSettings)
			RBXCRASH();
	}
}

void CWebService::LoadAppSettings()
{
	boost::filesystem::path exePath = RBX::FileSystem::getUserDirectory(false, RBX::DirExe, NULL);

	FASTLOGS(FLog::RCCServiceInit, "Loading AppSettings.xml, current path: %s", exePath.string());

	std::ifstream stream((exePath / "AppSettings.xml").native().c_str());

	TextXmlParser machine(stream.rdbuf());
	std::auto_ptr<XmlElement> root = machine.parse();

	const XmlElement* baseURLNode = root->findFirstChildByTag(RBX::Name::declare("BaseUrl"));
	if(baseURLNode)
	{
		std::string baseURL;
		baseURLNode->getValue(baseURL);
		FASTLOGS(FLog::RCCServiceInit, "Got Base url: %s", baseURL);
		SetBaseURL(baseURL);
	}


}

std::vector<std::string> CWebService::fetchAllowedSecurityVersions()
{
	const std::string& baseUrl = GetBaseURL();
	if (baseUrl.size()==0)
		RBXCRASH();	// y u no set BaseURL??

	std::vector<std::string> versions;

	std::string url = GetSecurityKeyUrl(baseUrl, "2b4ba7fc-5843-44cf-b107-ba22d3319dcd");

	RBX::Http request(url);

	std::string allowedSecurityVerionsData = "";
	try 
	{
		request.get(allowedSecurityVerionsData);

		// no need to continue if security version did not change
		if (allowedSecurityVerionsData == securityVersionData)
			return std::vector<std::string>();
		
		// parse json data
		boost::property_tree::ptree pt;
		std::stringstream stream(allowedSecurityVerionsData);
		read_json(stream, pt);

		BOOST_FOREACH(const boost::property_tree::ptree::value_type& child, pt.get_child("data"))
		{
			// version = data + salt
			std::string version = RBX::sha1(child.second.data() + "askljfLUZF");
			versions.push_back(version);
		}

		securityVersionData = allowedSecurityVerionsData;

	}
	catch (std::exception ex)
	{
		FASTLOG(FLog::Always, "LoadAllowedPlayerVersions exception");
	}

	return versions;
}

void CWebService::JobItem::touch(double seconds)
{
	expirationTime = RBX::Time::now<RBX::Time::Fast>() + RBX::Time::Interval(seconds);
}

double CWebService::JobItem::secondsToTimeout() const
{
	return (expirationTime - RBX::Time::now<RBX::Time::Fast>()).seconds();
}


int RCCServiceSoapService::HelloWorld(_ns1__HelloWorld *ns1__HelloWorld, _ns1__HelloWorldResponse *ns1__HelloWorldResponse)
{
	BEGIN_PRINT(HelloWorld,"HelloWorld");

	::InterlockedIncrement(&helloWorldCount);
	ns1__HelloWorldResponse->HelloWorldResult = soap_new_std__string(this, -1);
	*ns1__HelloWorldResponse->HelloWorldResult = "Hello World";
	::InterlockedDecrement(&helloWorldCount);
	END_PRINT(HelloWorld,"HelloWorld");
	return 0;
}

int RCCServiceSoapService::Diag(_ns1__Diag *ns1__Diag, _ns1__DiagResponse *ns1__DiagResponse)
{
	BEGIN_PRINT(Diag,"Diag");

	::InterlockedIncrement(&diagCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	CWebService::singleton->diag(ns1__Diag->type, *ns1__Diag->jobID, &ns1__DiagResponse->DiagResult, this);
	::InterlockedDecrement(&diagCount);

	END_PRINT(Diag,"Diag");
	return 0;
}
int RCCServiceSoapService::DiagEx(_ns1__DiagEx *ns1__DiagEx, _ns1__DiagExResponse *ns1__DiagExResponse)
{
	BEGIN_PRINT(DiagEx,"DiagEx");
	::InterlockedIncrement(&diagCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	ns1__DiagExResponse->DiagExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
	CWebService::singleton->diag(ns1__DiagEx->type, *ns1__DiagEx->jobID, &ns1__DiagExResponse->DiagExResult->LuaValue, this);
	::InterlockedDecrement(&diagCount);
	END_PRINT(DiagEx,"DiagEx");
	return 0;
}

int RCCServiceSoapService::GetVersion(_ns1__GetVersion *ns1__GetVersion, _ns1__GetVersionResponse *ns1__GetVersionResponse)
{
	BEGIN_PRINT(GetVersion,"GetVersion");
	::InterlockedIncrement(&getVersionCount);
	ns1__GetVersionResponse->GetVersionResult = RBX::DebugSettings::robloxVersion.c_str();
	::InterlockedDecrement(&getVersionCount);
	END_PRINT(GetVersion,"GetVersion");
	return 0;
}

int RCCServiceSoapService::GetStatus(_ns1__GetStatus *ns1__GetStatus, _ns1__GetStatusResponse *ns1__GetStatusResponse)
{
	BEGIN_PRINT(GetStatus,"GetStatus");

	::InterlockedIncrement(&getStatusCount);
	ns1__GetStatusResponse->GetStatusResult = soap_new_ns1__Status(this, -1);
	ns1__GetStatusResponse->GetStatusResult->version = soap_new_std__string(this, -1);
	*ns1__GetStatusResponse->GetStatusResult->version = RBX::DebugSettings::robloxVersion.c_str();
	ns1__GetStatusResponse->GetStatusResult->environmentCount = CWebService::singleton->jobCount();
	::InterlockedDecrement(&getStatusCount);
	END_PRINT(GetStatus,"GetStatus");
	return 0;
}

int RCCServiceSoapService::OpenJob(_ns1__OpenJob *ns1__OpenJob, _ns1__OpenJobResponse *ns1__OpenJobResponse)
{
	BEGIN_PRINT(OpenJob,"OpenJob");
	::InterlockedIncrement(&openJobCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	CWebService::singleton->openJob(*ns1__OpenJob->job, ns1__OpenJob->script, &ns1__OpenJobResponse->OpenJobResult, this, true);
	::InterlockedDecrement(&openJobCount);
	END_PRINT(OpenJob,"OpenJob");

	return 0;
}

int RCCServiceSoapService::OpenJobEx(_ns1__OpenJobEx *ns1__OpenJobEx, _ns1__OpenJobExResponse *ns1__OpenJobExResponse)
{
	BEGIN_PRINT(OpenJobEx,"OpenJobEx");

	::InterlockedIncrement(&openJobCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	ns1__OpenJobExResponse->OpenJobExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
	CWebService::singleton->openJob(*ns1__OpenJobEx->job, ns1__OpenJobEx->script, &ns1__OpenJobExResponse->OpenJobExResult->LuaValue, this, true);
	::InterlockedDecrement(&openJobCount);

	END_PRINT(OpenJobEx,"OpenJobEx");
	return 0;
}

int RCCServiceSoapService::Execute(_ns1__Execute *ns1__Execute, _ns1__ExecuteResponse *ns1__ExecuteResponse)
{
	BEGIN_PRINT(Execute,"Execute");

	::InterlockedIncrement(&executeCount);
	try
	{
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		CWebService::singleton->execute(ns1__Execute->jobID, ns1__Execute->script, &ns1__ExecuteResponse->ExecuteResult, this);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&executeCount);
		throw;
	}
	::InterlockedDecrement(&executeCount);
	END_PRINT(Execute,"Execute");
	return 0;
}

int RCCServiceSoapService::ExecuteEx(_ns1__ExecuteEx *ns1__ExecuteEx, _ns1__ExecuteExResponse *ns1__ExecuteExResponse)
{
	BEGIN_PRINT(ExecuteEx,"ExecuteEx");

	::InterlockedIncrement(&executeCount);
	try
	{
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		ns1__ExecuteExResponse->ExecuteExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
		CWebService::singleton->execute(ns1__ExecuteEx->jobID, ns1__ExecuteEx->script, &ns1__ExecuteExResponse->ExecuteExResult->LuaValue, this);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&executeCount);
		throw;
	}

	::InterlockedDecrement(&executeCount);
	END_PRINT(ExecuteEx,"ExecuteEx");

	return 0;
}


int RCCServiceSoapService::CloseJob(_ns1__CloseJob *ns1__CloseJob, _ns1__CloseJobResponse *ns1__CloseJobResponse)
{
	BEGIN_PRINT(CloseJob,"CloseJob");
	::InterlockedIncrement(&closeJobCount);
	try
	{
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		CWebService::singleton->closeJob(ns1__CloseJob->jobID);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&closeJobCount);
		throw;
	}
	::InterlockedDecrement(&closeJobCount);
	END_PRINT(CloseJob,"CloseJob");
	return 0;
}

int RCCServiceSoapService::RenewLease(_ns1__RenewLease *ns1__RenewLease, _ns1__RenewLeaseResponse *ns1__RenewLeaseResponse)
{
	BEGIN_PRINT(RenewLease,"RenewLease");

	::InterlockedIncrement(&renewLeaseCount);
	try
	{
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		CWebService::singleton->renewLease(ns1__RenewLease->jobID, ns1__RenewLease->expirationInSeconds);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&renewLeaseCount);
		throw;
	}
	::InterlockedDecrement(&renewLeaseCount);
	END_PRINT(RenewLease,"RenewLease");
	return 0;
}

int RCCServiceSoapService::BatchJob(_ns1__BatchJob *ns1__BatchJob, _ns1__BatchJobResponse *ns1__BatchJobResponse)
{
	BEGIN_PRINT(BatchJob,"BatchJob");

	::InterlockedIncrement(&batchJobCount);
 	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	// Batch jobs are completed synchronously, so there is no need to start the heartbeat
	try
	{
		CWebService::singleton->batchJob(*ns1__BatchJob->job, ns1__BatchJob->script, &ns1__BatchJobResponse->BatchJobResult, this);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&batchJobCount);
		throw;
	}
	::InterlockedDecrement(&batchJobCount);
	END_PRINT(BatchJob,"BatchJob");

	return 0;
}


int RCCServiceSoapService::BatchJobEx(_ns1__BatchJobEx *ns1__BatchJobEx, _ns1__BatchJobExResponse *ns1__BatchJobExResponse)
{
	BEGIN_PRINT(BatchJobEx,"BatchJobEx");

	::InterlockedIncrement(&batchJobCount);
 	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	ns1__BatchJobExResponse->BatchJobExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
   // Batch jobs are completed synchronously, so there is no need to start the heartbeat
	try
	{
		CWebService::singleton->batchJob(*ns1__BatchJobEx->job, ns1__BatchJobEx->script, &ns1__BatchJobExResponse->BatchJobExResult->LuaValue, this);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&batchJobCount);
		throw;
	}

	::InterlockedDecrement(&batchJobCount);
	END_PRINT(BatchJobEx,"BatchJobEx");
	return 0;
}

int RCCServiceSoapService::GetExpiration(_ns1__GetExpiration *ns1__GetExpiration, _ns1__GetExpirationResponse *ns1__GetExpirationResponse)
{
	BEGIN_PRINT(GetExpiration,"GetExpiration");

	::InterlockedIncrement(&getExpirationCount);
	try{
		CWebService::singleton->getExpiration(ns1__GetExpiration->jobID, &ns1__GetExpirationResponse->GetExpirationResult);
	}
	catch(std::exception&)
	{
		::InterlockedDecrement(&getExpirationCount);
		throw;
	}
	::InterlockedDecrement(&getExpirationCount);
	END_PRINT(GetExpiration,"GetExpiration");
	return 0;
}

int RCCServiceSoapService::GetAllJobs(_ns1__GetAllJobs *ns1__GetAllJobs, _ns1__GetAllJobsResponse *ns1__GetAllJobsResponse)
{
	BEGIN_PRINT(GetAllJobs,"GetAllJobs");

	::InterlockedIncrement(&getAllJobsCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	CWebService::singleton->getAllJobs(ns1__GetAllJobsResponse->GetAllJobsResult, this);
	::InterlockedDecrement(&getAllJobsCount);
	END_PRINT(GetAllJobs,"GetAllJobs");
	return 0;
}

int RCCServiceSoapService::GetAllJobsEx(_ns1__GetAllJobsEx *ns1__GetAllJobsEx, _ns1__GetAllJobsExResponse *ns1__GetAllJobsExResponse)
{
	BEGIN_PRINT(GetAllJobsEx,"GetAllJobsEx");

	::InterlockedIncrement(&getAllJobsCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	ns1__GetAllJobsExResponse->GetAllJobsExResult = soap_new_ns1__ArrayOfJob(this, -1);
	CWebService::singleton->getAllJobs(ns1__GetAllJobsExResponse->GetAllJobsExResult->Job, this);
	::InterlockedDecrement(&getAllJobsCount);
	END_PRINT(GetAllJobsEx,"GetAllJobsEx");
	return 0;
}

int RCCServiceSoapService::CloseExpiredJobs(_ns1__CloseExpiredJobs *ns1__CloseExpiredJobs, _ns1__CloseExpiredJobsResponse *ns1__CloseExpiredJobsResponse)
{
	BEGIN_PRINT(GetAllJobs,"CloseExpiredJobs");

	::InterlockedIncrement(&closeExpiredJobsCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	CWebService::singleton->closeExpiredJobs(&ns1__CloseExpiredJobsResponse->CloseExpiredJobsResult);
	::InterlockedDecrement(&closeExpiredJobsCount);
	END_PRINT(GetAllJobs,"CloseExpiredJobs");
	return 0;
}

int RCCServiceSoapService::CloseAllJobs(_ns1__CloseAllJobs *ns1__CloseAllJobs, _ns1__CloseAllJobsResponse *ns1__CloseAllJobsResponse)
{
	BEGIN_PRINT(GetAllJobs,"CloseAllJobs");

	::InterlockedIncrement(&closeAllJobsCount);
	RBX::Security::Impersonator impersonate(RBX::Security::WebService);
	CWebService::singleton->closeAllJobs(&ns1__CloseAllJobsResponse->CloseAllJobsResult);
	::InterlockedDecrement(&closeAllJobsCount);
	END_PRINT(GetAllJobs,"CloseAllJobs");
	return 0;
}

namespace
{
    static bool isServiceInstalled(const char* svcName)
    {
        bool result = false;
        SC_HANDLE scm = OpenSCManager(NULL, NULL, NULL);
        SC_HANDLE thumbService = OpenService(scm, svcName, SERVICE_QUERY_STATUS);
        if (thumbService)
        {
            result = true;
            CloseServiceHandle(thumbService);
        }
        if (scm)
        {
            CloseServiceHandle(scm);
        }
        return result;
    }
}

