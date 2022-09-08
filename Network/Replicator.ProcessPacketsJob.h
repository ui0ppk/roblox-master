#pragma once

#include "Replicator.h"
#include "rbx/Debug.h"
#include "V8DataModel/DataModel.h"
#include "Network/Players.h"
#include "util/MemoryStats.h"
#include "Replicator.StreamJob.h"
LOGGROUP(NetworkStepsMultipliers)
LOGGROUP(MaxNetworkReadTimeInCS)
DYNAMIC_FASTINTVARIABLE(MaxWaitTimeBeforeForcePacketProcessMS, 0)
DYNAMIC_FASTINTVARIABLE(MaxProcessPacketsStepsPerCyclic, 5)
DYNAMIC_FASTINTVARIABLE(MaxProcessPacketsStepsAccumulated, 15);
DYNAMIC_FASTINTVARIABLE(MaxProcessPacketsJobScaling, 10);

namespace RBX { namespace Network {

class Replicator::ProcessPacketsJob : public ReplicatorJob
{
	rbx::atomic<int> isAwake;
    DataModel* dm;
	
public:
	ProcessPacketsJob(Replicator& replicator)
		:ReplicatorJob("Replicator ProcessPackets", replicator, DataModelJob::DataIn)
	{
		RBXASSERT(getArbiter());
		isAwake = 1;
        dm = DataModel::get(&replicator);
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessIncoming;
	}

	// wake and sleep are used for testing and debugging.
	void wake() { 
		if (!isAwake.compare_and_swap(1, 0)) 
			RBX::TaskScheduler::singleton().reschedule(shared_from_this());
	}
	void sleep() { isAwake = false; }

private:
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		if (!isAwake)
			return RBX::Time::Interval::max();

		if(replicator){
			return replicator->incomingPacketsCount()>0
				? computeStandardSleepTime(stats, replicator->settings().getReceiveRate())
				: Time::Interval(0.1); 
		}
		return Time::Interval(0.1); 
	}

	virtual Error error(const Stats& stats)
	{
		if (!isAwake)
			return Job::Error();

		Error result;
		result.error = 0;		
		if(replicator)
		{
            if (dm->getIsShuttingDown())
                return Job::Error();
            
			if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
			{
				if (replicator->incomingPacketsCount())
				{
					result.error = 1.0f;
				}
				else
				{
					result.error = 0.0f;
				}
			}
			else if (replicator->settings().isQueueErrorComputed)
			{
				double waitTime = replicator->incomingPacketsCountHeadWaitTimeSec(stats.timeNow);
				result.error = waitTime * replicator->settings().getReceiveRate();
			}
			else
			{
				const int count = replicator->incomingPacketsCount();
				result.error = count * stats.timespanSinceLastStep.seconds() *
						replicator->settings().getReceiveRate();
			}
		}
		return result;
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if (!isAwake)
			return TaskScheduler::Stepped;

		if(replicator) {
			FASTLOG(FLog::NetworkStepsMultipliers, "ProcessPacketsJob running");
			DataModel::scoped_write_request request(replicator.get());

			Time t;
			if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
			{
				float stepLength = updateStepsRequiredForCyclicExecutive( 
					stats.timespanSinceLastStep.seconds(),
					replicator->settings().getReceiveRate(),
					(float)DFInt::MaxProcessPacketsStepsPerCyclic,
					(float)DFInt::MaxProcessPacketsStepsAccumulated);

				int processTimeMultiplier = std::min<int>(5, replicator->incomingPacketsCount()); 
				stepLength = std::max<float>(1.0, stepLength);
				float processTimeScale = std::min<float>((float)DFInt::MaxProcessPacketsJobScaling, stepLength * processTimeMultiplier);
				t = Time::now<Time::Fast>() + Time::Interval( (processTimeScale) * (0.5 / replicator->settings().getReceiveRate()));
			}
			else
			{
				t = Time::now<Time::Fast>() + Time::Interval(0.5 / replicator->settings().getReceiveRate());
			}

			FASTLOG2(FLog::NetworkStepsMultipliers, "ProcessPacketsJob: StepOnce, processAllPackets: %d, packets in Queue: %d", (int)replicator->processAllPacketsPerStep, replicator->incomingPackets.size());
			Time maxTime = (Time::now<Time::Fast>() + Time::Interval(0.01 * FLog::MaxNetworkReadTimeInCS)); // convert centisecond to second
            while (!(dm->getIsShuttingDown()) && ( replicator->processNextIncomingPacket()))
			{
				if (replicator->processAllPacketsPerStep)
				{
					// break if we exceed our maximum time here
					if (FLog::MaxNetworkReadTimeInCS > 0)
						if (Time::now<Time::Fast>() > maxTime)
							break;
				}
				/// Process ALL Packets if there is anything thats been there for longer than 200ms
				else if (Time::now<Time::Fast>() > t && 
					!(replicator->incomingPacketsCountHeadWaitTimeSec(stats.timeNow) > (float)DFInt::MaxWaitTimeBeforeForcePacketProcessMS/1000.0f))
					break;
			}
	
			// post processing
			replicator->postProcessPacket();
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};

}}

