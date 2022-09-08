#include "Replicator.StatsItem.h"
#include "script/ScriptContext.h"
#include "ClientReplicator.h"


using namespace RBX;
using namespace Network;

struct JobInfo
{
	double dutyCycle;
	double stepsPerSecond;
	double stepTime;
	int count;

	JobInfo() : dutyCycle(0), stepTime(0), stepsPerSecond(0), count(0) {}
};

void DeserializedStatsItem::process(Replicator& replicator) 
{
	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
	rep->statsReceivedSignal(stats);
}

void writeTaskSchedulerStats(DataModel* dm, RakNet::BitStream& bitStream)
{
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsInfo(jobs);

	boost::unordered_map<std::string, JobInfo> aggregatedJobInfos;

	// BitStream:
	// end flag (bool)
	// job name (string)
	// duty cycle (float)
	// steps per second (float)
	// step time (float)

	for (std::vector<boost::shared_ptr<const TaskScheduler::Job> >::iterator iter = jobs.begin(); iter != jobs.end(); iter++)
	{
		shared_ptr<const TaskScheduler::Job> job = *iter;
		if (job->getArbiter().get() != dm)
			continue;

		const std::string& name = job->name;

		JobInfo& info = aggregatedJobInfos[name];
		info.dutyCycle += job->averageDutyCycle();
		info.stepsPerSecond += job->averageStepsPerSecond();
		info.stepTime += job->averageStepTime();
		info.count++;
	}

	// calculate replicator job averages and write to bit stream
	for (boost::unordered_map<std::string, JobInfo>::iterator iter = aggregatedJobInfos.begin(); iter != aggregatedJobInfos.end(); iter++)
	{
		bitStream << false; // more items

		bitStream << iter->first;
		bitStream << (float)(iter->second.dutyCycle / iter->second.count);
		bitStream << (float)(iter->second.stepsPerSecond / iter->second.count);
		bitStream << (float)(iter->second.stepTime / iter->second.count);
	}
	bitStream << true; // done with jobs
}

void writeScriptStats(DataModel*dm, RakNet::BitStream& bitStream)
{
	ScriptContext* sc = dm->find<ScriptContext>();
	std::string scriptsJson;
	sc->setCollectScriptStats(true);

	std::vector<ScriptContext::ScriptStat> scriptStats;
	sc->getScriptStatsTyped(scriptStats);

	for (std::vector<ScriptContext::ScriptStat>::iterator iter = scriptStats.begin(); iter != scriptStats.end(); iter++)
	{
		bitStream << false; // more stats info

		bitStream << iter->name;
		bitStream << (float)iter->activity;
		bitStream << (int)iter->invocationCount;
	}
	bitStream << true; // done with stats info
}

bool Replicator::StatsItem::write(RakNet::BitStream& bitStream)
{
	writeItemType(bitStream, Item::ItemTypeStats);

	float totalPhysicsFPS = 0;
	float dataPacketsTotal = 0;
	float dataSizeTotal = 0;
	float physicsPacketsTotal = 0;
	float physicsSizeTotal = 0;
	float totalPing = 0;
	float totalNewDataItems = 0;
	float totalSentDataItems = 0;

	Instances::const_iterator end = replicator.getParent()->getChildren()->end();
	for (Instances::const_iterator iter = replicator.getParent()->getChildren()->begin(); iter != end; ++iter)
	{
		// accumulate stats for all replicator
		if (Replicator* r = Instance::fastDynamicCast<Replicator>(iter->get()))
		{
			const ReplicatorStats& s = r->stats();
			totalPhysicsFPS += r->stats().physicsSenderStats.physicsPacketsSent.rate();
			dataPacketsTotal += r->stats().dataPacketsSent.rate();
			dataSizeTotal += r->stats().dataPacketsSentSize.value();
			physicsPacketsTotal += r->stats().physicsSenderStats.physicsPacketsSent.rate();
			physicsSizeTotal += r->stats().physicsSenderStats.physicsPacketsSentSize.value();
			totalPing += r->stats().dataPing.value();
			totalNewDataItems += (float)s.dataNewItemsPerSec.getCount();
			totalSentDataItems += (float)s.dataItemsSentPerSec.getCount();
		}
	}

	int numChildren = replicator.getParent()->numChildren();

	switch (version)
	{
	case 2:
		{
			DataModel* dm = DataModel::get(&replicator);
			writeTaskSchedulerStats(dm, bitStream);
			writeScriptStats(dm, bitStream);
		}
		// fall through
	case 1:
		{
			bitStream << (totalPing / numChildren); // average ping
			bitStream << (totalPhysicsFPS / numChildren); // average physics sender fps
			bitStream << dataPacketsTotal * dataSizeTotal / 1000.0f; // total data KB per sec
			bitStream << physicsPacketsTotal * physicsSizeTotal / 1000.0f; // total physics KB per sec
			bitStream << totalSentDataItems / totalNewDataItems; // data throughput
		}
	default:
		break;
	}
	
	return true;
}

shared_ptr<DeserializedItem> Replicator::StatsItem::read(Replicator& replicator, RakNet::BitStream& bitStream)
{
	shared_ptr<DeserializedStatsItem> deserializedData(new DeserializedStatsItem());

	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
	deserializedData->stats = rep->readStats(bitStream);
	
	return deserializedData;
}


