
#include "rbx/TaskScheduler.Job.h"
#include "rbx/Tasks/Coordinator.h"
#include "rbx/Log.h"
#include "FastLog.h"

using namespace RBX;

LOGGROUP(TaskSchedulerSteps)

#if HANG_DETECTION
#include <../../App/include/util/standardout.h>
#define STEPTIME_SAMPLE_INTEVAL 60.0
double RBX::TaskScheduler::Job::stepTimeThreshold = 0.0;
#endif

TaskScheduler::Job::SleepAdjustMethod TaskScheduler::Job::sleepAdjustMethod(AverageInterval);
double RBX::TaskScheduler::Job::throttledSleepTime = 0.01;

double TaskScheduler::Job::averageDutyCycle() const 
{ 
	return dutyCycle.dutyCycle();
} 

double TaskScheduler::Job::averageSleepRate() const 
{ 
	return sleepRate.value(); 
} 

double TaskScheduler::Job::averageStepsPerSecond() const 
{ 
	return dutyCycle.stepInterval().rate();
} 

double TaskScheduler::Job::averageStepTime() const 
{ 
	return dutyCycle.stepTime().value(); 
} 

Time::Interval TaskScheduler::Job::getSleepingTime() const
{
	if (state!=Sleeping)
		return Time::Interval::zero();
	else
		return Time::now<Time::Fast>() - timeofLastSleep;
} 

double TaskScheduler::Job::averageError() const 
{ 
	return runningAverageError.value();
}


void TaskScheduler::Job::removeCoordinator(class boost::shared_ptr<class RBX::Tasks::Coordinator> coordinator)
{
	coordinator->onRemoved(this);
	{
		RBX::mutex::scoped_lock lock(coordinatorMutex);
		coordinators.erase(std::find(coordinators.begin(), coordinators.end(), coordinator));
	}
}


void TaskScheduler::Job::addCoordinator(class boost::shared_ptr<class RBX::Tasks::Coordinator> coordinator)
{
	{
		RBX::mutex::scoped_lock lock(coordinatorMutex);
		coordinators.push_back(coordinator);
	}
	coordinator->onAdded(this);
}

bool TaskScheduler::Job::isDisabled() 
{ 
	if (coordinators.size()==0)
		return false;

	RBX::mutex::scoped_lock lock(coordinatorMutex);
	// TODO: Write more efficiently without boost bind
	return std::find_if(
		coordinators.begin(), 
		coordinators.end(), 
		boost::bind(&Tasks::Coordinator::isInhibited, _1, this)
		) != coordinators.end();
} 

const double lerpJob = 0.05;
extern RBX::Time::Interval maxDutyCycleWindow;

TaskScheduler::Job::Job(const char* name, shared_ptr<TaskScheduler::Arbiter> arbiter, Time::Interval stepBudget)
	:name(name)
	,sharedArbiter(arbiter)
	,weakArbiter(boost::weak_ptr<Arbiter>())
	,baldArbiter(NULL)
	,state(Unknown)
	,sleepRate(lerpJob)
	,dutyCycle(lerpJob, 8)
	,runningAverageError(lerpJob)
	,timespanOfLastStep(0.0) 
	,isRemoveRequested(false)
	,dutyCycleWindow(maxDutyCycleWindow)
	,stepBudget(stepBudget)
	,overStepTimeThresholdCount(0)
	,allotedConcurrency(-1)
	,cyclicExecutive(false)
	,cyclicPriority(CyclicExecutiveJobPriority_Default)
{
	FASTLOG2(FLog::TaskSchedulerInit, "Job Created - this(%p) arbiter(%p)", this, arbiter.get());
	FASTLOGS(FLog::TaskSchedulerInit, "JobName(%s)", name);
}

TaskScheduler::Job::~Job()
{
	RBXASSERT(!TaskScheduler::SleepingHook::is_linked());
	RBXASSERT(!TaskScheduler::WaitingHook::is_linked());
	while (coordinators.size()>0)
	{
		coordinators.back()->onRemoved(this);
		coordinators.pop_back();
	}

	FASTLOG1(FLog::TaskSchedulerInit, "Job Destroyed - this(%p)", this);
	FASTLOGS(FLog::TaskSchedulerInit, "JobName(%s)", name);
}

// Use this to generate the error function if you just want to try to track the desiredHz
TaskScheduler::Job::Error TaskScheduler::Job::computeStandardError(const Stats& stats, double desiredHz) 
{
	return stats.timespanSinceLastStep.seconds() * desiredHz;
}

// Same as computeStandardError but incorporates sleeping in Cyclic Executive.
TaskScheduler::Job::Error TaskScheduler::Job::computeStandardErrorCyclicExecutiveSleeping(const Stats& stats, double desiredHz) 
{
	TaskScheduler::Job::Error error = computeStandardError(stats, desiredHz);
	if(RBX::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
	{
		// Waking up at error >= 1.0 means that a task always runs late.  In practice
		// there is always scheduling jitter, however in the spirit of minimally
		// intrusive changes allow the task to wake up just a tiny bit early.
		if( error.error < 0.98 )
		{
			error.error = 0.0; // Sleep until ready.
		}
	}

	return error;
}

Time::Interval TaskScheduler::Job::computeStandardSleepTime(const Stats& stats, double desiredHz) 
{
//	bool schedulerIsCyclicExecutive = RBX::TaskScheduler::singleton().isCyclicExecutive();
//	RBXASSERT( !schedulerIsCyclicExecutive || cyclicExecutive || desiredHz <= 5 );

	shared_ptr<Arbiter> ar(getArbiter());
	Time::Interval minTime = ar && ar->isThrottled() ? Time::Interval(throttledSleepTime) : Time::Interval::zero();

	const double interval = 1.0 / desiredHz;
	
	switch (sleepAdjustMethod)
	{
	case AverageInterval:
		// If we're not being called as frequently as we'd like, then don't sleep
		if (dutyCycle.stepInterval().value().seconds() > 1.05 * interval)
			return minTime;
		break;

	case LastSample:
		if (dutyCycle.lastStepInterval().seconds() > 1.05 * interval)
			return minTime;
		break;
            
    default:
        break;
	}

	Time::Interval result = Time::Interval(interval) - stats.timespanSinceLastStep;
	if (result < minTime)
		return minTime;
	return result;
}

TaskScheduler::Job::Stats::Stats(Job& job, Time now)
{
    timeNow = now;
	timespanSinceLastStep = now - job.timeofLastStep;
	timespanOfLastStep = job.timespanOfLastStep;
}

void TaskScheduler::Job::startWaiting()
{
	state = Waiting;
}

void TaskScheduler::Job::startSleeping()
{
	state = Sleeping;
	timeofLastSleep = Time::now<Time::Fast>();
}

void TaskScheduler::Job::updateWakeTime()
{
	Time now = Time::now<Time::Fast>();
	Job::Stats stats(*this, now);
	wakeTime = now + sleepTime(stats);
}

void TaskScheduler::Job::updateError(const Time& now)
{
	Job::Stats stats(*this, now);
	currentError = error(stats);
	RBXASSERT(currentError.error < std::numeric_limits<double>::max());
	if (currentError.error>0)
		runningAverageError.sample(currentError.error);
}


void TaskScheduler::Job::notifyCoordinatorsPreStep()
{
	if (coordinators.size()>0)
	{
		RBX::mutex::scoped_lock lock(coordinatorMutex);
		std::for_each(
			coordinators.begin(), 
			coordinators.end(), 
			boost::bind(&Tasks::Coordinator::onPreStep, _1, this)
			);
	}
}

void TaskScheduler::Job::preStep()
{
	state = Running;

	FASTLOG4(FLog::TaskSchedulerRun, "JobStart. this: %p arbiter: %p time: %u error: %d", this, getArbiter().get(), (unsigned)stepStartTime.timestampSeconds(),
        currentError.urgent ? -1 : (int)currentError.error);
    FASTLOGS(FLog::TaskSchedulerRun, "JobStart %s", name.c_str());


	shared_ptr<Arbiter> ar(getArbiter());
	if (ar)
		ar->preStep(this);
}

void TaskScheduler::Job::postStep(StepResult result)
{
	state = Unknown;
	timeofLastStep = stepStartTime;
	const Time now = Time::now<Time::Fast>();
	timespanOfLastStep = now - stepStartTime;

	// TODO: extract stepTime from dutyCycle?
	dutyCycle.sample(timespanOfLastStep);

	if(!stepBudget.isZero() && timespanOfLastStep > stepBudget){
		//We were over budget
	}
	
#if HANG_DETECTION
	if((stepTimeThreshold > 0) && (timespanOfLastStep.seconds() > stepTimeThreshold))
	{
		overStepTimeThresholdCount++;
		StandardOut::singleton()->printf(MESSAGE_WARNING, "TaskScheduler::Job: %s step time over threshold", name.c_str());
		
		// send to log
		if (RBX::Log::current() && ((Time::now<Time::Fast>() - stepTimeSampleTime).seconds() > STEPTIME_SAMPLE_INTEVAL))
		{
			std::string msg = "TaskScheduler::Job: " + name + " step time over threshold.";
			RBX::Log::current()->writeEntry(RBX::Log::Error, msg.c_str());

			stepTimeSampleTime = Time::now<Time::Fast>();
		}
	}
#endif

	if(dutyCycleWindow.getMaxWindow().seconds() > 0)
	{
		dutyCycleWindow.sample(timespanOfLastStep);
	}

	FASTLOG3(FLog::TaskSchedulerRun, "JobStop. This: %p arbiter: %p time: %u", this, getArbiter().get(), (unsigned)now.timestampSeconds());
    FASTLOGS(FLog::TaskSchedulerRun, "JobStop %s", name.c_str());

	shared_ptr<Arbiter> ar(getArbiter());
	if (ar)
		ar->postStep(this);
}


void TaskScheduler::Job::notifyCoordinatorsPostStep()
{
	if (coordinators.size()>0)
	{
		RBX::mutex::scoped_lock lock(coordinatorMutex);
		std::for_each(
			coordinators.begin(), 
			coordinators.end(), 
			boost::bind(&Tasks::Coordinator::onPostStep, _1, this)
			);
	}
}

void TaskScheduler::Job::updatePriority()
{
	switch (TaskScheduler::priorityMethod)
	{
		case FIFO:
			priority = 1.0;
			break;

		case LastError:
			priority = currentError.error;
			break;

		case AccumulatedError:
			double avg = averageError();
			if (avg>0)
				priority = avg;
			else
				priority = currentError.error;

			priority *= getPriorityFactor();

			double st = std::max(0.01, averageDutyCycle());
			priority /= st;
			break;
	}
}
