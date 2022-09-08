#include "stdafx.h"
#include "Script/ScriptStats.h"
#include "Script/ScriptEvent.h"

namespace RBX
{
	ScriptStats::ScriptStats()
	{}

	void ScriptStats::stopCollection(const std::string& scriptHash)
	{
		ScriptActivityMeterMap::iterator iter = scriptActivityMap.find(scriptHash);
		if(iter != scriptActivityMap.end()){
			iter->second.activity->decrement();
		}
		else{
			//We lost this sample somehow
		}
	}
	void ScriptStats::startCollection(const std::string& scriptHash, bool firstTime)
	{
		ScriptActivityMeterMap::iterator iter = scriptActivityMap.find(scriptHash);
		if(iter != scriptActivityMap.end()){
			iter->second.activity->increment();
			if(firstTime)
				iter->second.invocations->increment();
		}
		else{
			StatCollection stats;
			stats.activity = boost::shared_ptr<ActivityMeter<2> >(new ActivityMeter<2>());
			stats.invocations = boost::shared_ptr<InvocationMeter<2> >(new InvocationMeter<2>());
			std::pair<ScriptActivityMeterMap::iterator, bool> newEntry = scriptActivityMap.insert(ScriptActivityMeterMap::value_type(scriptHash,stats));
			if(newEntry.second){
				newEntry.first->second.activity->increment();
				if(firstTime)
					newEntry.first->second.invocations->increment();
			}
		}
	}

	void ScriptStats::scriptResumeStarted(const std::string& scriptHash)
	{
		if(!scriptStack.empty()){
			stopCollection(scriptStack.top());
		}
		startCollection(scriptHash, true);
		scriptStack.push(scriptHash);
	}

	void ScriptStats::scriptResumeStopped(const std::string& scriptHash)
	{
		RBXASSERT(!scriptStack.empty());
		RBXASSERT(scriptStack.top() == scriptHash);
		stopCollection(scriptHash);
		scriptStack.pop();
		if(!scriptStack.empty()){
			startCollection(scriptStack.top(), false);
		}
	}

	void LuaStatsItem::init()
	{
		createBoundChildItem("disabled", scriptContext->scriptsDisabled);
		createChildItem<int>("threads", boost::bind(&ScriptContext::getThreadCount, scriptContext));

		resumedThreads = createChildItem("ThreadsResumed");
		deferredThreads = createBoundChildItem("ThreadsThrottled", scriptContext->throttlingThreads);

		averageGcInterval = createChildItem("AverageGcInterval");
		averageGcTime = createChildItem("AverageGcTime");
	}

	void LuaStatsItem::update()
	{
		averageGcInterval->formatValue(scriptContext->getAvgLuaGcInterval(), "%.2f msec", scriptContext->getAvgLuaGcInterval());
		averageGcTime->formatValue(scriptContext->getAvgLuaGcTime(), "%.4f msec", scriptContext->getAvgLuaGcTime());
		resumedThreads->formatRate(scriptContext->resumedThreads);
	}

}