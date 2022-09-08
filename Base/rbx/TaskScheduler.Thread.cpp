
#include "rbx/TaskScheduler.h"
#include "rbx/boost.hpp"
#include "rbx/TaskScheduler.Job.h"
#include "CPUCount.h"
#include "boost/enable_shared_from_this.hpp"
#include "rbx/Log.h"
#include "rbx/Profiler.h"

using namespace RBX;
using boost::shared_ptr;

#include "CPUCount.h"

#define JOIN_TIMEOUT (1000*20) // 20 seconds

LOGGROUP(TaskSchedulerRun)
LOGGROUP(TaskSchedulerFindJob)

class TaskScheduler::Thread : public boost::enable_shared_from_this<Thread>
{
	boost::scoped_ptr<boost::thread> thread;
	volatile bool done;
	TaskScheduler* const taskScheduler;

	Thread(TaskScheduler* taskScheduler)
		:done(false)
		,enabled(true)
		,taskScheduler(taskScheduler)
	{}
public:
	shared_ptr<Job> job;
	volatile bool enabled;
	static shared_ptr<Thread> create(TaskScheduler* taskScheduler)
	{
		shared_ptr<Thread> thread(new Thread(taskScheduler));

        static rbx::atomic<int> count;
		std::string name = RBX::format("Roblox TaskScheduler Thread %d", static_cast<int>(++count));

		// loop holds a shared_ptr to the thread, so it won't be collected before the loop exits :)
		thread->thread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&Thread::loop, thread), name.c_str())));
		return thread;
	}

	void end()
	{
		done = true;
	}

	void join()
	{
		end();
		if (thread->get_id() != boost::this_thread::get_id())
			thread->timed_join(boost::posix_time::milliseconds(JOIN_TIMEOUT));
	}

	void printJobInfo()
	{
		if (RBX::Log::current() && job)
		{
			Time now = Time::now<Time::Fast>();
			std::stringstream ss;
			if (job->isRunning())
				ss << "TaskScheduler::Job: " << job->name.c_str() << ", state: " << (int)job->getState() << ", seconds spend in job: " << (now - job->stepStartTime).seconds();
			else
				ss << "TaskScheduler::Job: " << job->name.c_str() << ", state: " << (int)job->getState();

			RBX::Log::current()->writeEntry(RBX::Log::Information, ss.str().c_str());
		}
	}

	~Thread()
	{
		join();
	}

	void loop();
	void releaseJob();
	TaskScheduler::StepResult runJob();
};

void TaskScheduler::printJobs()
{
	std::for_each(threads.begin(), threads.end(), boost::bind(&Thread::printJobInfo, _1));
}

void TaskScheduler::endAllThreads()
{
	std::for_each(threads.begin(), threads.end(), boost::bind(&Thread::join, _1));
}

void TaskScheduler::setThreadCount(ThreadPoolConfig threadConfig)
{
	static const unsigned int kDefaultCoreCount = 1;
	int realCoreCount = RbxTotalUsableCoreCount(kDefaultCoreCount);

	int requestedCount;
	RBX::mutex::scoped_lock lock(mutex);
	if (threadConfig==Auto)
	{
		// automatic: 1 thread per core
		requestedCount = realCoreCount;
	}
	else if (threadConfig>=PerCore1)
	{
		requestedCount = realCoreCount * (threadConfig - PerCore1 + 1);
	}
	else
	{
		requestedCount = (int) threadConfig;
	}

	RBXASSERT(requestedCount>0);

	desiredThreadCount = (size_t)requestedCount;

	while (threads.size() < (size_t)requestedCount)
		threads.push_back(Thread::create(this));
}


TaskScheduler::StepResult TaskScheduler::Thread::runJob()
{
	job->stepStartTime = Time::now<Time::Fast>();

	TaskScheduler::Job::Stats stats(*job, job->stepStartTime);

	job->preStep();
	
	TaskScheduler::StepResult result;
	++taskScheduler->runningJobCount;
	RBXASSERT(currentJob.get()==NULL);
	currentJob.reset(job.get());

	// No need for exception handling. If an exception is thrown here
	// then we should abort the application.
	taskScheduler->taskCount++;
	result = job->step(stats);

	RBXASSERT(currentJob.get()==job.get());
	currentJob.reset(NULL);
	--taskScheduler->runningJobCount;

	job->postStep(result);

	job->lastThreadUsed = shared_from_this();

	return result;
}


bool TaskScheduler::conflictsWithScheduledJob(RBX::TaskScheduler::Job* job) const
{
	if (threads.size() == 1)
		return false;

	const shared_ptr<Arbiter>& arbiter = job->getArbiter();
	if (!arbiter)
		return false;

	for (Threads::const_iterator iter = threads.begin(); iter != threads.end(); ++iter)
	{
		RBX::TaskScheduler::Job* other = (*iter)->job.get();
		if (other)
		{
			if (Job::haveDifferentArbiters(job, other))
				continue;	// different Arbiter domains can run concurrently

			if (arbiter->areExclusive(job, other))
				return true;
		}
	}
	return false;
}


void TaskScheduler::Thread::releaseJob()
{
	job->state = Job::Unknown;

	if (job->isRemoveRequested)
	{
		if (job->joinEvent)
			job->joinEvent->Set();

		if( taskScheduler->cyclicExecutiveEnabled && job->cyclicExecutive )
		{
			taskScheduler->releaseCyclicExecutive(job.get());
		}

	}
	else
	{
		taskScheduler->scheduleJob(*job);
	}
	job.reset();
}


bool TaskScheduler::shouldDropThread() const
{
	RBXASSERT(desiredThreadCount <= threads.size());
	return desiredThreadCount < threads.size();
}
void TaskScheduler::dropThread(Thread* thread)
{
	thread->end();
	for(Threads::iterator iter = threads.begin(); iter != threads.end(); ++iter){
		if(iter->get() == thread){
			threads.erase(iter);
			break;
		}
	}
}

void TaskScheduler::disableThreads(int count, Threads& threads)
{
	for (Threads::const_iterator iter = this->threads.begin(); count>0 && iter != this->threads.end(); ++iter)
	{
		const shared_ptr<Thread>& t = *iter;
		if (t->job)
			continue;
		if (!t->enabled)
			continue;
		t->enabled = false;
		threads.push_back(t);
		count--;
	}
}

void TaskScheduler::enableThreads(Threads& threads)
{
	for (Threads::const_iterator iter = threads.begin(); iter != threads.end(); ++iter)
	{
		RBXASSERT(!(*iter)->enabled);
		(*iter)->enabled = true;
	}
	threads.clear();
}

void TaskScheduler::Thread::loop()
{
	Profiler::onThreadCreate(format("TS %p", this).c_str());

	taskScheduler->incrementThreadCount();

	TaskScheduler::Threads participatingThreads;	// threads that are turned off during a parallel run

	shared_ptr<Thread> self(shared_from_this());
	while (!done)
	{
		{
			RBX::mutex::scoped_lock lock(taskScheduler->mutex);
			FASTLOG1(FLog::TaskSchedulerFindJob,
				"Took mutex %p in thread TaskScheduler::Thread::loop", &(taskScheduler->mutex));
				
			const RBX::Time start(taskScheduler->schedulerDutyCycle.startSample());

			if (job)
			{
				taskScheduler->enableThreads(participatingThreads);
				job->allotedConcurrency = -1;
				job->notifyCoordinatorsPostStep();
				releaseJob();
			}

			if(taskScheduler->shouldDropThread())
				taskScheduler->dropThread(this);
			else
			{
				if (enabled && !done)
				{
					job = taskScheduler->findJobToRun(self);
					if (job)
					{
						RBXASSERT(!job->isDisabled());
						// This must be synchronized with findJobToRun
						// because Coordinators expect atomicity with
						// isInhibited
						job->notifyCoordinatorsPreStep();

						taskScheduler->disableThreads(job->getDesiredConcurrencyCount() - 1, participatingThreads);
						job->allotedConcurrency = int(participatingThreads.size()) + 1;
					}
				}
			}

			taskScheduler->schedulerDutyCycle.stopSample(start);

			FASTLOG1(FLog::TaskSchedulerFindJob,
				"Releasing mutex %p in TaskScheduler::Thread::loop", &(taskScheduler->mutex));

			if (done)
				break;
		}

		if (job)
		{
			if (runJob() == TaskScheduler::Done)
				job->isRemoveRequested = true;
		}
		else 
		{
			Time::Interval sleepTime = taskScheduler->getShortestSleepTime();
			if (sleepTime.seconds()>0)
			{
				// The most efficient thing is to sleep for a super-short period of time.
				// This is more efficient than waiting on a mutex, and the timespan is
				// short enough to make the system responsive.
#ifdef _WIN32
				::Sleep(1);
#else
				::usleep(1000);
#endif
			}
		}
	}

	if (job)
	{
		RBX::mutex::scoped_lock lock(taskScheduler->mutex);
		taskScheduler->enableThreads(participatingThreads);
		job->allotedConcurrency = -1;
		job->notifyCoordinatorsPostStep();
		releaseJob();
	}

	taskScheduler->decrementThreadCount();

	Profiler::onThreadExit();
}


void TaskScheduler::getJobsInfo(std::vector<shared_ptr<const Job> >& result)
{
	RBX::mutex::scoped_lock lock(mutex);
	for (AllJobs::iterator iter = allJobs.begin(); iter!=allJobs.end(); ++iter)
		result.push_back(*iter);
}

static void setJobExtendedStatsWindow(shared_ptr<TaskScheduler::Job> job, double seconds)
{
	job->getDutyCycleWindow().setMaxWindow(Time::Interval(seconds));
	if(seconds == 0.0)
	{
		job->getDutyCycleWindow().clear();
	}
}

RBX::Time::Interval maxDutyCycleWindow(0.0);

void TaskScheduler::setJobsExtendedStatsWindow(double seconds)
{
	maxDutyCycleWindow = Time::Interval(seconds);

	std::vector<boost::shared_ptr<TaskScheduler::Job> > jobs;
	{
		RBX::mutex::scoped_lock lock(mutex);
		for (AllJobs::iterator iter = allJobs.begin(); iter!=allJobs.end(); ++iter)
			jobs.push_back(*iter);
	}
	std::for_each(jobs.begin(), jobs.end(), boost::bind(setJobExtendedStatsWindow, _1, seconds));
}

void TaskScheduler::cancelCyclicExecutive()
{
	// It turns out that determining this is a server may happen late enough that a lock is needed.
	RBX::mutex::scoped_lock lock(mutex);

	cyclicExecutiveEnabled = false;
	for (CyclicExecutiveJobs::iterator i = cyclicExecutiveJobs.begin(); i != cyclicExecutiveJobs.end(); ++i)
	{
		Job* j = i->job.get();
		j->cyclicExecutive = false;
		if( i->isRunning == false )
		{
			scheduleJob( *j );
		}
	}
	cyclicExecutiveJobs.clear();

	RBXASSERT( cyclicExecutiveJobs.empty() );
}

void TaskScheduler::releaseCyclicExecutive(TaskScheduler::Job* job)
{
	RBXASSERT(std::find( cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), *job ) != cyclicExecutiveJobs.end());
	cyclicExecutiveJobs.erase(std::find( cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), *job ));
}

void TaskScheduler::getJobsByName(const std::string& name, std::vector<boost::shared_ptr<const Job> >& result)
{
	RBX::mutex::scoped_lock lock(mutex);
	for (AllJobs::iterator iter = allJobs.begin(); iter!=allJobs.end(); ++iter)
	{
		if((*iter)->name == name)
		{
			result.push_back(*iter);
		}
	}
}
