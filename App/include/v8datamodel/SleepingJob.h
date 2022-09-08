#pragma once

#include "V8DataModel/DataModelJob.h"

namespace RBX {

class SleepingJob : public DataModelJob
{
private:
	rbx::atomic<int> isAwake;
	const double desiredFps;
	RBX::Time lastWakeTime;

public:
	SleepingJob(const char* name, TaskType taskType, bool isPerPlayer,
		shared_ptr<RBX::DataModelArbiter> arbiter, RBX::Time::Interval stepBudget,
		double desiredFps);

	void wake();
	void sleep();

	virtual RBX::Time::Interval sleepTime(const Stats&);

	virtual RBX::TaskScheduler::Job::Error error(const Stats& stats);
};

}
