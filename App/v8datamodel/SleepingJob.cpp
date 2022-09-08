#include "stdafx.h"

#include "V8DataModel/SleepingJob.h"

namespace RBX {

SleepingJob::SleepingJob(const char* name, TaskType taskType, bool isPerPlayer,
		shared_ptr<RBX::DataModelArbiter> arbiter, RBX::Time::Interval stepBudget,
		double desiredFps)
	: RBX::DataModelJob(name, taskType, isPerPlayer, arbiter, stepBudget)
	, desiredFps(desiredFps) {
	isAwake = false;
}

void SleepingJob::wake() { 
	if (!isAwake.compare_and_swap(1,0))
	{
		lastWakeTime = RBX::Time::now<RBX::Time::Fast>();
		RBX::TaskScheduler::singleton().reschedule(shared_from_this());
	}
}

void SleepingJob::sleep() {
	isAwake = false;
}

Time::Interval SleepingJob::sleepTime(const Stats&) {
	return isAwake ? RBX::Time::Interval(0) : RBX::Time::Interval::max();
}

TaskScheduler::Job::Error SleepingJob::error(const Stats& stats) {
	if (!isAwake)
		return Job::Error();
	
	Stats fakedStats = stats;
	RBX::Time::Interval timeSinceAwoke = RBX::Time::now<RBX::Time::Fast>() - lastWakeTime;
	if (timeSinceAwoke < fakedStats.timespanSinceLastStep) {
		fakedStats.timespanSinceLastStep = timeSinceAwoke;
	}

	return computeStandardError(fakedStats, desiredFps);
}

}