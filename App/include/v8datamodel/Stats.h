#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "rbx/RunningAverage.h"
#include "rbx/TaskScheduler.h"
#include "util/Analytics.h"

#include <rapidjson/document.h>

namespace RBX {

    class DataModel;
    
	void registerStatsClasses();

	namespace Profiling
	{
		class Profiler;
	}

	namespace Stats
	{
		extern const char* const sStats;
		extern const char* const sStatsItem;
		static std::string countersApiKey =  "76E5A40C-3AE1-4028-9F10-7C62520BD94F";

		class Item : public DescribedNonCreatable<Item, Instance, sStatsItem>
		{
		protected:
			double val;
			std::string sValue;
			virtual void update();
		public:
			Item():val(0) {}
			Item(const char* name):DescribedNonCreatable<Item, Instance, sStatsItem>(name),val(0) {}
			const std::string& getStringValue() {
				update();
				return sValue;
			}
			double getValue() { 
				update();
				return val; 
			}
			// Slower version of getStringValue() for Reflection
			std::string getStringValue2() {
				return getStringValue();
			}
			void setValue(double val, const std::string& sValue) {
				this->val = val;
				this->sValue = sValue;
			}
			void formatMem(size_t bytes);
			void formatValue(double val, const char* fmt, ...);
			void formatRate(const RunningAverageTimeInterval<Time::Benchmark>& interval);
			void formatPercent(double val) {
				formatValue(val, "%.1f%%", 100.0f * val);
			}

			// TODO: Merge with StringConverter classes?  Reflection code?
			template<class V>
			void formatValue(const V& value);

			// Helper functions:
			Item* createChildItem(const char* name);
			template<class T>
			Item* createChildItem(const char* name, boost::function0<T> func);
			template<class V>
			Item* createBoundChildItem(const char* name, const V& value);
			Item* createBoundChildItem(const Profiling::Profiler& p);

			template<typename ValueType, typename AverageType> 
			Item* createBoundChildItem(const char* name, const RunningAverage<ValueType, AverageType>& ra);
			template<typename ValueType, Time::SampleMethod sampleMethod> 
			Item* createBoundChildItem(const char* name, const TotalCountTimeInterval<ValueType, sampleMethod>& ra);
            template<Time::SampleMethod sampleMethod> 
            Item* createBoundChildItem(const char* name, const RunningAverageTimeInterval<sampleMethod>& ra);

			Item* createBoundMemChildItem(const char* name, const size_t& v);
			Item* createBoundPercentChildItem(const char* name, const float& v);

		protected:
			bool askAddChild(const Instance* instance) const
			{
				return Instance::fastDynamicCast<Item>(instance)!=NULL;
			}

		};

		template<class T>
		class TypedStatsItem : public Item
		{
			static const T& deref(const T* value)
			{
				return *value;
			}
		protected:
			const boost::function0<T> func;
		public:
			TypedStatsItem(boost::function0<T> func)
				:func(func) 
			{}
			TypedStatsItem(const T* value)
				// TODO: Does boost have a way of turning a value ref to a function0?
				:func(boost::bind(&TypedStatsItem::deref, value))
			{}

			void update()
			{
				formatValue(func());
			}
		};


		class TypedMemItem : public TypedStatsItem<size_t>
		{
		public:
			TypedMemItem(const size_t* value)
				:TypedStatsItem<size_t>(value)
			{
			}
			virtual void update()
			{
				formatMem(func());
			}
		};


		class TypedPercentItem : public TypedStatsItem<float>
		{
		public:
			TypedPercentItem(const float* value)
				:TypedStatsItem<float>(value)
			{
			}
			virtual void update()
			{
				formatPercent(func());
			}
		};

		class JsonWriter
		{
			std::stringstream& s;
			bool& needComma;

			bool hasNonJsonType;

		public:
			JsonWriter(std::stringstream& s, bool& needComma) : s(s), needComma(needComma), hasNonJsonType(false) {};
			void writeTableEntries(const Reflection::ValueTable& data);
			void writeArrayEntries(const Reflection::ValueArray& data);
			void writeTableEntry(const std::pair<std::string, Reflection::Variant>& value);
			void writeKeyValue(const std::pair<std::string, Reflection::Variant>& value);
			void writeArrayEntry(const Reflection::Variant& value);
			void writeValue(const Reflection::Variant& value);
			bool seenNonJsonType() { return hasNonJsonType; }
		};

		// A generic mechanism for displaying stats (like 3D FPS, network traffic, etc.)
		class StatsService 
			: public DescribedNonCreatable<StatsService, Instance, sStats, Reflection::ClassDescriptor::INTERNAL_LOCAL, RBX::Security::LocalUser>
			, public Service
		{
			std::string customReportUrl;
			typedef boost::unordered_map<std::string, RBX::Time> LastReportTimes;
			LastReportTimes lastReportTimes;
			rbx::signals::scoped_connection baseUrlConnection;
		public:
			StatsService():DescribedNonCreatable<StatsService, Instance, sStats, Reflection::ClassDescriptor::INTERNAL_LOCAL, RBX::Security::LocalUser>("Stats") 
				,minReportInterval(10)
			{}
			std::string reporterType;
			double minReportInterval;
			std::string getReportUrl() const;
			void setReportUrl(std::string url);
			void report(std::string category, shared_ptr<const Reflection::ValueTable> data);

			void report(std::string category, shared_ptr<const Reflection::ValueTable> data, int percentage);

			// This message reports without doing any local throttling. Do not
			// use this method if the messages are spammy/frequent. If you are
			// unsure, start by using the throttled report() method to get a
			// sense for the frequency of these messages.
			static void report_BypassThrottlingAndCustomUrl(std::string category, const Reflection::ValueTable& data, const char* optional_Shard = "Client");
			void reportTaskScheduler(bool includeJobs);
			void reportJobsStepWindow();

		protected:
			bool askAddChild(const Instance* instance) const
			{
				return Instance::fastDynamicCast<Item>(instance)!=NULL;
			}
			/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		private:
			void reportJob(boost::shared_ptr<const TaskScheduler::Job> job, shared_ptr<std::stringstream> stream, bool& isFirst);
			bool addHeader(shared_ptr<std::stringstream> stream);
			static void addCategoryAndTable(const std::string& category,
				const Reflection::ValueTable& data, shared_ptr<std::stringstream> stream);
			bool checkLastReport(const std::string& category);
			bool tryToStartScript();
			static std::string getDefaultReportUrl(const std::string& baseUrl, const std::string& shard);
			static void postReportWithUrl(const std::string& url, shared_ptr<std::stringstream> stream);
			void postReport(shared_ptr<std::stringstream> stream);
		};

		template<class T>
		Item* Item::createChildItem(const char* name, boost::function0<T> func)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< TypedStatsItem<T> >(func);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		template<class V>
		Item* Item::createBoundChildItem(const char* name, const V& v)
		{
			shared_ptr<Item> item = Creatable<Instance>::create< TypedStatsItem<V> >(&v);
			item->setName(name);
			item->setParent(this);
			return item.get();
		}

		
		void setBrowserTrackerId(const std::string& trackerId);
		void reportGameStatus(const std::string& status, bool blocking = false);

	}
} // namespace
