/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Stats.h"
#include "V8DataModel/DataModel.h"
#include "rbx/Log.h"
#include "util/http.h"
#include "util/profiling.h"
#include "util/statistics.h"
#include "util/Analytics.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/DebugSettings.h"
#include "script/ScriptContext.h"
#include "format_string.h"
#include "RobloxServicesTools.h"
#include "Network/Players.h"

#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include "Util/FileSystem.h"
#include "VersionInfo.h"
#elif __ANDROID__
namespace RBX
{
namespace JNI
{
extern std::string robloxVersion; // JNIMain.cpp
}
}
#endif

#include <boost/algorithm/string/replace.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "VMProtect/VMProtectSDK.h"
DYNAMIC_FASTINTVARIABLE(HttpInfluxHundredthsPercentage, 0)
DYNAMIC_FASTSTRINGVARIABLE(HttpInfluxURL, "https://onepointtwentyone-misterfusion-1.c.influxdb.com:8087")
DYNAMIC_FASTSTRINGVARIABLE(HttpInfluxDatabase, "Default")
DYNAMIC_FASTSTRINGVARIABLE(HttpInfluxUser, "roblox")
DYNAMIC_FASTSTRINGVARIABLE(HttpInfluxPassword, "te$tu$3r")

DYNAMIC_FASTFLAGVARIABLE(UseNewAnalyticsApi, false)

extern const char *const sTotalCountTimeIntervalItem = "TotalCountTimeIntervalItem";

class TotalCountTimeIntervalItem : public RBX::DescribedNonCreatable<TotalCountTimeIntervalItem, RBX::Stats::Item, sTotalCountTimeIntervalItem> 
{
	const RBX::TotalCountTimeInterval<>& tcti;
public:
	TotalCountTimeIntervalItem(const RBX::TotalCountTimeInterval<>* tcti) : tcti(*tcti) {}
	virtual void update() 
	{
		int v = tcti.getCount();
		if (val != v)
		{
			val = v;
			sValue = RBX::format("%d", v);
		}
	}
};

template<class T>
class RunningAverageItem : public RBX::Stats::Item 
{
	const RBX::RunningAverage<T, double>& ra;
public:
	RunningAverageItem(const RBX::RunningAverage<T, double>* ra):ra(*ra) {}

	virtual void update() {
		double v = ra.value();
		if (val != v)
		{
			val = v;
			sValue = RBX::format("%g (%.0f%%CV)", v, 100.0 * ra.coefficient_of_variation());
		}
	}
};

extern const char *const sRunningAverageItemInt = "RunningAverageItemInt";

class RunningAverageItemInt : public RBX::DescribedNonCreatable<RunningAverageItemInt, RunningAverageItem<int>, sRunningAverageItemInt> 
{
public:
	RunningAverageItemInt(const RBX::RunningAverage<int, double>* ra)
		:RBX::DescribedNonCreatable<RunningAverageItemInt, RunningAverageItem<int>, sRunningAverageItemInt>(ra) 
	{}
};


extern const char *const sRunningAverageItemDouble = "RunningAverageItemDouble";

class RunningAverageItemDouble : public RBX::DescribedNonCreatable<RunningAverageItemDouble, RunningAverageItem<double>, sRunningAverageItemDouble> 
{
public:
	RunningAverageItemDouble(const RBX::RunningAverage<double, double>* ra)
		:RBX::DescribedNonCreatable<RunningAverageItemDouble, RunningAverageItem<double>, sRunningAverageItemDouble>(ra) 
	{}
};

template<RBX::Time::SampleMethod sampleMethod = RBX::Time::Benchmark>
class RunningAverageTimeIntervalItem : public RBX::Stats::Item 
{
    const RBX::RunningAverageTimeInterval<sampleMethod>& ra;
public:
    RunningAverageTimeIntervalItem(const RBX::RunningAverageTimeInterval<sampleMethod>* ra):ra(*ra) {}

    virtual void update() {
        double v = ra.rate();
        if (val != v)
        {
            val = v;
            sValue = RBX::format("%g (%.2f%%CV)", v, 100.0 * ra.coefficient_of_variation());
        }
    }
};

extern const char *const sRunningAverageTimeInterval = "RunningAverageTimeIntervalItem";

class RunningAverageTimeIntervalItemTimeBenchmark : public RBX::DescribedNonCreatable<RunningAverageTimeIntervalItemTimeBenchmark, RunningAverageTimeIntervalItem<RBX::Time::Benchmark>, sRunningAverageTimeInterval>
{
public:
    RunningAverageTimeIntervalItemTimeBenchmark(const RBX::RunningAverageTimeInterval<RBX::Time::Benchmark>* ra)
        :RBX::DescribedNonCreatable<RunningAverageTimeIntervalItemTimeBenchmark, RunningAverageTimeIntervalItem<RBX::Time::Benchmark>, sRunningAverageTimeInterval>(ra)
    {}
};

extern const char *const sProfilingItem = "ProfilingItem";

class ProfilingItem : public RBX::DescribedNonCreatable<ProfilingItem, RBX::Stats::Item, sProfilingItem> 
{
	const RBX::Profiling::Profiler& p;
public:
	ProfilingItem(const RBX::Profiling::Profiler* p):p(*p) {}

	// Returns a Tuple of 4 numbers
	shared_ptr<const RBX::Reflection::Tuple> getTimes(double window)
	{
		RBX::Profiling::Bucket b = p.getWindow(window);
		shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(3));
		result->values[0] = b.getWallTime();
		result->values[1] = b.getSampleTime();
		result->values[2] = b.frames;
		return result;
	}

	shared_ptr<const RBX::Reflection::Tuple> getTimesForFrames(int frames)
	{
		RBX::Profiling::Bucket b = p.getFrames(frames);
		shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(3));
		result->values[0] = b.getWallTime();
		result->values[1] = b.getSampleTime();
		result->values[2] = b.frames;
		return result;
	}

	virtual void update() {
		const double window = RBX::Profiling::Profiler::profilingWindow;
		RBX::Profiling::Bucket b = p.getWindow(window);
		if (b.sampleTimeElapsed>0)
		{
			val = b.getWallTime() / b.sampleTimeElapsed;
			char buffer[256];
			if (b.frames>0)
			{
				std::string t = RBX::Log::formatTime(b.getNominalFramePeriod());
				sprintf(buffer, "%s %.3g/s nom%.3g/s %.3g%%", t.c_str(), b.getActualFPS(), b.getNominalFPS(), 100.0 * val);
			}
			else
			{
				sprintf(buffer, "%.3g%%", 100.0 * val);
			}
			sValue = buffer;
		}
		else
		{
			val = 0;
			sValue = "?";
		}
	}
};

REFLECTION_BEGIN();
static RBX::Reflection::BoundFuncDesc<ProfilingItem, shared_ptr<const RBX::Reflection::Tuple>(double)> func_GetTimes(&ProfilingItem::getTimes, "GetTimes", "window", 0.0, RBX::Security::Plugin);
static RBX::Reflection::BoundFuncDesc<ProfilingItem, shared_ptr<const RBX::Reflection::Tuple>(int)> func_GetTimesForFrames(&ProfilingItem::getTimesForFrames, "GetTimesForFrames", "frames", 1, RBX::Security::Plugin);
REFLECTION_END();


namespace RBX
{		
	namespace Stats
	{
		std::string browserTrackerId;
		std::string appVersion;
		std::string reporter;
		std::string userId;
		std::string location;

		static void HttpHelper(std::string* response, std::exception* exception, const std::string& url)
		{
			if (exception)
			{
#if !defined(RBX_PLATFORM_DURANGO) // remove this define in future?
				StandardOut::singleton()->printf(MESSAGE_ERROR, "%s: %s", url.c_str(), exception->what());
#endif
			}
		}

		static void httpPost(const std::string& url, std::string data, bool synchronous, std::string optionalContentType, bool externalRequest = false)
		{
			if (synchronous)
			{
				try 
				{
					std::string response;
					std::istringstream stream(data);
					Http http(url);
                    http.recordStatistics = false;
                    http.post(stream, optionalContentType.empty() ? Http::kContentTypeUrlEncoded : optionalContentType, data.size() > 256, response, externalRequest);
				}
				catch (std::exception& e)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "Stats http error: %s, %s", url.c_str(), e.what());	
				}
			}
            else
            {
                Http http(url);
                http.recordStatistics = false;
				http.post(data, optionalContentType.empty() ? Http::kContentTypeUrlEncoded : optionalContentType, data.size() > 256, boost::bind(&HttpHelper, _1, _2, url), externalRequest);
            }
		}

		void setUserId(int id)
		{
			userId = RBX::format("%d", id);
			RBX::Analytics::setUserId(id);
		}

		void setBrowserTrackerId(const std::string& trackerId)
		{
			browserTrackerId = trackerId;
		}

		void reportGameStatus(const std::string& status, bool blocking)
		{
			if (browserTrackerId.empty() || status.empty())
				return;

			std::string baseUrl = ::GetBaseURL();
			if (baseUrl[baseUrl.size() - 1] != '/')
				baseUrl += '/';

			httpPost(RBX::format("%sclient-status/set?browserTrackerId=%s&status=%s", baseUrl.c_str(), Http::urlEncode(browserTrackerId).c_str(), Http::urlEncode(status).c_str()), "", blocking, "*/*");
		}

		const char* const  sStats = "Stats";

        REFLECTION_BEGIN();
		static Reflection::BoundFuncDesc<StatsService, void(std::string, shared_ptr<const Reflection::ValueTable>)> func_report(&StatsService::report, "Report", "category", "data", RBX::Security::RobloxScript);
		static Reflection::BoundFuncDesc<StatsService, void(bool)> func_reportTaskScheduler(&StatsService::reportTaskScheduler, "ReportTaskScheduler", "includeJobs", false, RBX::Security::RobloxScript);
		static Reflection::BoundFuncDesc<StatsService, void()> func_reportJobsStepWindow(&StatsService::reportJobsStepWindow, "ReportJobsStepWindow", RBX::Security::RobloxScript);
		static Reflection::BoundFuncDesc<StatsService, void(std::string)> prop_reportUrl(&StatsService::setReportUrl, "SetReportUrl", "url", Security::RobloxScript);
		static Reflection::BoundProp<std::string> prop_reporterType("ReporterType", "Reporting", &StatsService::reporterType, Reflection::PropertyDescriptor::UI, Security::RobloxScript);
		static Reflection::BoundProp<double> prop_minReportInterval("MinReportInterval", "Reporting", &StatsService::minReportInterval, Reflection::PropertyDescriptor::UI, Security::RobloxScript);
		
		const char* const  sStatsItem = "StatsItem";

		static Reflection::BoundFuncDesc<Item, std::string()> func_values(&Item::getStringValue2, "GetValueString", RBX::Security::Plugin);
		static Reflection::BoundFuncDesc<Item, double()> func_value(&Item::getValue, "GetValue", RBX::Security::Plugin);
        REFLECTION_END();

		static void reportResult(std::string * message, std::exception * exception)
		{
			if (exception)
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "StatsService Report failed: %s", exception->what());
			//if (message)
			//	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "StatsService Report result: %s", message->c_str());
		}

		void JsonWriter::writeTableEntries(const Reflection::ValueTable& data)
		{
			std::for_each(data.begin(), data.end(), boost::bind(&JsonWriter::writeTableEntry, this, _1));
		}

		void JsonWriter::writeArrayEntries(const Reflection::ValueArray& data)
		{
			std::for_each(data.begin(), data.end(), boost::bind(&JsonWriter::writeArrayEntry, this, _1));
		}

		void JsonWriter::writeTableEntry(const std::pair<std::string, Reflection::Variant>& value)
		{
			if (needComma)
				s << ',';

			writeKeyValue(value);

			needComma = true;
		}

		void JsonWriter::writeKeyValue(const std::pair<std::string, Reflection::Variant>& value)
		{
			std::string key = value.first;

			// Make the key legal for MongoDB
			while (key[0] == '$')
				key.erase(0, 1);

			while (true)
			{
				size_t p = key.find('.');
				if (p == std::string::npos)
					break;
				key[p] = '_';
			}

			// TODO: escape " and \ characters
			s << " \"" << key << "\":";

			writeValue(value.second);
		}

		void JsonWriter::writeArrayEntry(const Reflection::Variant& value)
		{
			if (needComma)
				s << ',';
			else
				needComma = true;

			writeValue(value);

			needComma = true;
		}

		static std::string escape(const std::string& string) {
			std::string result = "";

			for (std::string::size_type i = 0; i < string.length(); ++i) {
				char c = string.at(i);
				switch (c) {
				case '\0':
					result += "\\0";
					break;

				case '\r':
					result += "\\r";
					break;

				case '\n':
					result += "\\n";
					break;

				case '\t':
					result += "\\t";
					break;

				case '\\':
					result += "\\\\";
					break;

				case '\"':
					result += "\\\"";
					break;

				default:
					result += c;
				}
			}

			return result;
		}

		void JsonWriter::writeValue(const Reflection::Variant& value)
		{
			if (value.isNumber())
			{
				s << value.get<std::string>();
			}
			else if (value.type() == RBX::Reflection::Type::singleton<bool>())
			{
				s << value.get<std::string>();	// v = "true" | "false"
			}
			else if (value.type() == RBX::Reflection::Type::singleton< shared_ptr<const Reflection::ValueTable> >())
			{
				s << "{ "; 
				bool needComma = false;
				JsonWriter writer(s, needComma);
				writer.writeTableEntries(*value.get< shared_ptr<const Reflection::ValueTable> >());
				s << " }";
				hasNonJsonType = writer.seenNonJsonType();
			}
			else if (value.type() == RBX::Reflection::Type::singleton< shared_ptr<const Reflection::ValueArray> >())
			{
				s << "[ "; 
				bool needComma = false;
				JsonWriter writer(s, needComma);
				writer.writeArrayEntries(*value.get< shared_ptr<const Reflection::ValueArray> >());
				s << " ]"; 
				hasNonJsonType = writer.seenNonJsonType();
			}
			else if (value.isVoid())
			{
				s << "null";
			}
			else
			{
				if(!value.isString())
					hasNonJsonType = true;

				std::string valueString = value.get<std::string>();
				std::string escaped = escape(valueString);
				s << "\"" << escaped << "\"";
			}
		}

		struct JobStepWindowWriter
		{
			JobStepWindowWriter(std::stringstream* s) : stream(s), firstTime(true) {};
			std::stringstream *stream;
			bool firstTime;
			void operator()(double dt)
			{
				if (firstTime)
					firstTime = false;
				else
					*stream << ", ";
				*stream << dt;
			}
		};

		bool StatsService::addHeader(shared_ptr<std::stringstream> stream)
		{
			*stream << "{";
			int id = 0;
			bool needComma = false;

			if (RBX::DataModel* dataModel = DataModel::get(this))
			{
				id = dataModel->getPlaceIDOrZeroInStudio();
				if (id != 0)
				{
					*stream << " \"placeId\":" << id;
					needComma = true;
				}
			}

			if (reporterType.size() > 0)
			{
				if (needComma)
					*stream << ',';
				*stream << " \"reporter\":\"" << reporterType << "\"";
				needComma = true;
			}

			return needComma;
		}

		void StatsService::addCategoryAndTable(const std::string& category,
				const Reflection::ValueTable& data, shared_ptr<std::stringstream> stream) {
			*stream << " \"category\":\"" << category << "\""; 

			bool needsComma = true;
			JsonWriter writer(*stream, needsComma);
			writer.writeTableEntries(data);
		}

static const bool jobsAsArray = true;

		std::string StatsService::getDefaultReportUrl(const std::string& baseUrlInput, const std::string& shard) {
			// copy base url, to allow modification
			std::string baseUrl = baseUrlInput;

			RBXASSERT(baseUrl.substr(0, 7) == "http://");
			if (baseUrl.find("www.roblox.com") != std::string::npos)
				baseUrl = "http://logging.service.roblox.com/";
			else
				baseUrl.insert(7, "logging.service.");	// Inserted after http://
			return baseUrl + "Gatherer/LogEntry?Shard=" + shard;
		}

		std::string StatsService::getReportUrl() const
		{
			if (customReportUrl.size()>0)
				return customReportUrl;

			if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
			{
				return getDefaultReportUrl(contentProvider->getBaseUrl(), "Client");
			}

			throw std::runtime_error("No logging url provided");
		}

		void StatsService::setReportUrl(std::string url)
		{
			customReportUrl = url;
		}

		void StatsService::postReportWithUrl(const std::string& url, shared_ptr<std::stringstream> stream)
		{
			// DEPRECATED. TODO: remove


			//Http http(url);
			//http.post(stream, Http::kContentTypeApplicationJson, false, reportResult);
		}

		void StatsService::postReport(shared_ptr<std::stringstream> stream)
		{
			postReportWithUrl(getReportUrl(), stream);
		}

		void StatsService::reportJob(boost::shared_ptr<const TaskScheduler::Job> job, shared_ptr<std::stringstream> stream, bool& isFirst)
		{
			if (job->getArbiter().get() != DataModel::get(this))
				return;

			if (isFirst)
				isFirst = false;
			else
				*stream << ',';
			if (jobsAsArray)
				*stream << " { \"name\":\"" << job->getDebugName() << "\",";
			else
				*stream << " \"" << job->getDebugName() << "\": {";
			*stream << " \"dutyCycle\": " << job->averageDutyCycle();
			*stream << ", \"stepRate\": " << job->averageStepsPerSecond();
			*stream << ", \"stepTime\": " << job->averageStepTime();
			*stream << ", \"error\": " << job->averageError();
			//*stream << ", \"isRunning\": " << job->isRunning();
			*stream << " }";
		}

		void StatsService::reportJobsStepWindow()
		{
			const char* category = "JobsStepWindow";

			if (!checkLastReport(category))
				return;

			shared_ptr<std::stringstream> stream(new std::stringstream());

			bool needComma = addHeader(stream);

			if (needComma)
				*stream << ',';

			*stream << " \"category\":\"" << category << "\"";

			*stream << ", \"ThreadPoolSize\": "     << RBX::TaskScheduler::singleton().threadPoolSize();

			std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
			TaskScheduler::singleton().getJobsInfo(jobs);

			if (jobsAsArray)
				*stream << ", \"jobs\": [ "; 
			else
				*stream << ", \"jobs\": { "; 

			// jobs loop
			for (unsigned int i = 0; i < jobs.size(); i++)
			{
				if (jobsAsArray)
					*stream << " { \"name\":\"" << jobs[i]->getDebugName() << "\",";
				else
					*stream << " \"" << jobs[i]->getDebugName() << "\": {";

				*stream << "\"stepTimes\": ";

				// step times for each job
				*stream << "[ ";
				JobStepWindowWriter writer(stream.get());
				jobs[i]->getStepStats().stepTime().iter(writer);
				*stream << " ]";

				if (i + 1 == jobs.size())
					*stream << " } ";
				else
					*stream << " }, ";
			}

			if (jobsAsArray)
				*stream << " ]"; 
			else
				*stream << " }"; 

			*stream << " }"; 

			postReport(stream);
		}

		void StatsService::reportTaskScheduler(bool includeJobs)
		{
			const char* category = "TaskScheduler";

			if (!checkLastReport(category))
				return;

			shared_ptr<std::stringstream> stream(new std::stringstream());
		
			bool needComma = addHeader(stream);

			if (needComma)
				*stream << ',';

			*stream << " \"category\":\"" << category << "\""; 

			*stream << ", \"DataModelCount\": "     << DataModel::count;
			*stream << ", \"ThreadAffinity\": "     << RBX::TaskScheduler::singleton().threadAffinity();
			*stream << ", \"NumSleepingJobs\": "    << RBX::TaskScheduler::singleton().numSleepingJobs();
			*stream << ", \"NumWaitingJobs\": "     << RBX::TaskScheduler::singleton().numWaitingJobs();
			*stream << ", \"NumRunningJobs\": "     << RBX::TaskScheduler::singleton().numRunningJobs();
			*stream << ", \"ThreadPoolSize\": "     << RBX::TaskScheduler::singleton().threadPoolSize();
			*stream << ", \"SchedulerRate\": "      << RBX::TaskScheduler::singleton().schedulerRate();
			*stream << ", \"SchedulerDutyCyclePerThread\": " << RBX::TaskScheduler::singleton().getSchedulerDutyCyclePerThread();
			*stream << ", \"PriorityMethod\": "     << RBX::TaskScheduler::priorityMethod;

			if (includeJobs)
			{
				if (jobsAsArray)
					*stream << ", \"jobs\": [ "; 
				else
					*stream << ", \"jobs\": { "; 
				std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
				TaskScheduler::singleton().getJobsInfo(jobs);
				bool isFirst = true;
				std::for_each(jobs.begin(), jobs.end(), boost::bind(&StatsService::reportJob, this, _1, stream, boost::ref(isFirst)));
				if (jobsAsArray)
					*stream << " ]"; 
				else
					*stream << " }"; 
			}
			*stream << " }"; 

			postReport(stream);
		}

		void StatsService::report(std::string category, shared_ptr<const Reflection::ValueTable> data, int percentage)
		{
			if(G3D::uniformRandomInt(0,99) >= percentage)
				return;

			this->report(category, data);
		}

		// TODO: Make this an yielding function so you can get result?
		void StatsService::report(std::string category, shared_ptr<const Reflection::ValueTable> data)
		{
			if (!checkLastReport(category))
				return;

			shared_ptr<std::stringstream> stream(new std::stringstream());
		
			if (addHeader(stream))
				*stream << ',';

			addCategoryAndTable(category, *data, stream);
			
			*stream << " }"; 

			postReport(stream);
		}

		void StatsService::report_BypassThrottlingAndCustomUrl(std::string category, const Reflection::ValueTable& data, const char* shard) {
			return;
		}

		bool StatsService::checkLastReport(const std::string& category)
		{
			const Time now(Time::now<Time::Fast>());

			LastReportTimes::iterator iter = lastReportTimes.find(category);
			
			if (iter == lastReportTimes.end())
			{
				lastReportTimes[category] = now;
				return true;	// A new value
			}

			if (iter->second + Time::Interval(minReportInterval) <= now)
			{
				iter->second = now;
				return true;
			}

			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "The report '%s' was throttled. Adhere to MinReportInterval", category.c_str());
			return false;
		}

		static void runScript(shared_ptr<DataModel> d, std::string script)
		{
			try
			{
				shared_ptr<ScriptContext> sc = shared_from(d->create<ScriptContext>());
				if (sc)
				{
					VMProtectBeginMutation("19");
					ProtectedString verifiedSource = ProtectedString::fromTrustedSource(script);
					ContentProvider::verifyRequestedScriptSignature(verifiedSource, "StatsScript", true);
					sc->executeInNewThread(Security::RobloxGameScript_, verifiedSource, "StatsReporting");
					VMProtectEnd();
				}
			}
			catch (RBX::base_exception&)
			{
                // Swallow exception to make it harder to see (security)
			}
		}

		static void onGatherScript(weak_ptr<DataModel> dm, std::string* json, std::exception* error )
		{
			if (error)
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "StatsService script url retrieval failed: %s", error->what());
			else if (json)
			{
				if (shared_ptr<DataModel> d = dm.lock())
				{
					std::string script;
					std::string url = *json;
					try
					{
						// Hacky JSON parsing until we get the good code
						static const char* key = "\"StatsGatheringScriptUrl\":\"";
						int pos = url.find(key);
						if(pos == -1)
							return;
						pos += strlen(key);
							
						url = url.substr(pos);
						pos = url.find('"');
						url = url.substr(0, pos);
						// nuke escape characters
						while (true)
						{
							pos = url.find('\\');
							if (pos == std::string::npos)
								break;
							url.erase(pos, 1);
						}
						if (url.size()==0)
							return;
                        Http http(url);
						http.get(script);
					}
					catch (RBX::base_exception& e)
					{
						RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "StatsService script %s retrieval failed: %s", url.c_str(), e.what());
						RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "StatsService script retrieval failed: %s", e.what());
						return;
					}

					try
					{
						d->submitTask(boost::bind(&runScript, d, script), DataModelJob::Write);
					}
					catch (RBX::base_exception& e)
					{
						RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "StatsService failed to run script: %s", e.what());
					}
					
				}
			}
		}

		bool StatsService::tryToStartScript( )
		{
			/*
			SettingsConsole.exe gametest get ClientSharedSettings
			SettingsConsole.exe gametest set ClientSharedSettings.StatsGatheringScriptUrl=
			SettingsConsole.exe gametest set ClientSharedSettings.StatsGatheringScriptUrl=http://logging.gametest.roblox.com/GameScript
			*/
			// TODO: This is a hack until we have a common way of getting the correct url
			std::string baseUrl;
			if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
			{
				baseUrl = contentProvider->getBaseUrl();
				if (baseUrl.empty())
					return false;
			}
			else
				return false;

			if (!Network::Players::serverIsPresent(this))
                return false;

			baseUrlConnection.disconnect();

			std::string url = GetSettingsUrl(baseUrl, "ClientSharedSettings", "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79");

            Http request(url);
			shared_ptr<DataModel> dm = shared_from(DataModel::get(this));

			request.get(boost::bind(&onGatherScript, weak_ptr<DataModel>(dm), _1, _2));

			return true;
		}

		void StatsService::onServiceProvider( ServiceProvider* oldProvider, ServiceProvider* newProvider )
		{
			Instance::onServiceProvider(oldProvider, newProvider);

			if (newProvider)
			{
				if (!tryToStartScript())
					if (ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this))
						baseUrlConnection = contentProvider->propertyChangedSignal.connect(boost::bind(&StatsService::tryToStartScript, this));
			}
		}

		void Item::update()
		{
			Item* parent = Instance::fastDynamicCast<Item>(getParent());
			if (parent)
				parent->update();
		}

		void Item::formatMem(size_t bytes)
		{
			setValue(bytes, Log::formatMem(bytes));
		}
		void Item::formatRate(const RunningAverageTimeInterval<Time::Benchmark>& interval)
		{
			formatValue(interval.rate(), "%.1f/s (%.0f%%CV)", interval.rate(), 100.0 * interval.coefficient_of_variation());
		}
		void Item::formatValue(double val, const char* fmt, ...)
		{
			va_list argList;
			va_start(argList,fmt);
			setValue(val, G3D::vformat(fmt, argList));
			va_end(argList);
		}



		Item* Item::createChildItem(const char* name)
		{
			shared_ptr<Item> item = Creatable<Instance>::create<Item>();
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		template<>
		void Item::formatValue(const double& value) {
			formatValue(value, "%.3g", value);
		}

		template<>
		void Item::formatValue(const float& value) {
			formatValue(value, "%.3g", value);
		}

		template<>
		void Item::formatValue(const int& value) {
			formatValue(value, "%d", value);
		}

		template<>
		void Item::formatValue(const long& value) {
			formatValue(value, "%d", value);
		}

		template<>
		void Item::formatValue(const unsigned long& value) {
			formatValue(value, "%d", value);
		}

		template<>
#ifdef _WIN32
		void Item::formatValue(const unsigned __int64& value) 
#else
#include <stdint.h>
		void Item::formatValue(const uint64_t& value) 
#endif		
		{
			formatValue(double(value), "%llu", value);
		}


		template<>
		void Item::formatValue(const unsigned int& value) {
			formatValue(value, "%d", value);
		}

		template<>
		void Item::formatValue(const bool& value) {
			this->sValue = value ? "true" : "false";
			this->val = value ? 1 : 0;
		}

		template<> 
		Item* Item::createBoundChildItem(const char* name, const TotalCountTimeInterval<>& ra)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< TotalCountTimeIntervalItem >(&ra);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		template<> 
		Item* Item::createBoundChildItem(const char* name, const RunningAverage<int, double>& ra)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< RunningAverageItemInt >(&ra);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		template<> 
		Item* Item::createBoundChildItem(const char* name, const RunningAverage<double, double>& ra)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< RunningAverageItemDouble >(&ra);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

        template<>
        Item* Item::createBoundChildItem(const char* name, const RunningAverageTimeInterval<RBX::Time::Benchmark>& ra)
        {
            shared_ptr<Item> item = Creatable<Instance>::create< RunningAverageTimeIntervalItemTimeBenchmark >(&ra);
            item->setName(name);
            item->setParent(this);
            return item.get();
        }

		Item* Item::createBoundChildItem(const Profiling::Profiler& p)
		{
			const Profiling::Profiler* ptr = &p;
			shared_ptr<Item> item = Creatable<Instance>::create< ProfilingItem >(ptr);
			item->setName(p.name.c_str());
			item->setParent(this);
			return item.get();

		}

		Item* Item::createBoundMemChildItem(const char* name, const size_t& v)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< TypedMemItem >(&v);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		Item* Item::createBoundPercentChildItem(const char* name, const float& v)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< TypedPercentItem >(&v);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

	}
}


RBX_REGISTER_CLASS(RBX::Stats::Item);
RBX_REGISTER_CLASS(RunningAverageItemInt);
RBX_REGISTER_CLASS(RunningAverageItemDouble);
RBX_REGISTER_CLASS(ProfilingItem);
RBX_REGISTER_CLASS(RBX::Stats::StatsService);
RBX_REGISTER_CLASS(TotalCountTimeIntervalItem);
RBX_REGISTER_CLASS(RunningAverageTimeIntervalItemTimeBenchmark);

void RBX::registerStatsClasses()
{
	// Prevent optimizer from stripping code
	RBX::Stats::StatsService::classDescriptor();
}
