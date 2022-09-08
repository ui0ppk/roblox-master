#pragma once
#include <set>
#include <vector>

#include "rbx/RunningAverage.h"
#include "rbx/Declarations.h"
#include "rbx/Debug.h"
#include "rbx/ThreadSafe.h"
#include "rbx/boost.hpp"
#include "rbx/CEvent.h"
#include "rbx/atomic.h"

#include "boost/thread.hpp"
#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "boost/intrusive/list.hpp"

#ifdef _WIN32
#   undef min
#   undef max
#endif


LOGGROUP(TaskSchedulerInit)
LOGGROUP(TaskSchedulerRun)
LOGGROUP(TaskSchedulerFindJob)

namespace RBX
{	
	/// A singleton object responsible for scheduling the execution TaskScheduler.Jobs.
	class TaskScheduler
	{
		struct SleepingTag;
		typedef boost::intrusive::list_base_hook< boost::intrusive::tag<SleepingTag> > SleepingHook;
		struct WaitingTag;
		typedef boost::intrusive::list_base_hook< boost::intrusive::tag<WaitingTag> > WaitingHook;

	public:
		class Thread;
		typedef std::vector< boost::shared_ptr<Thread> > Threads;
		class Job;
		bool DataModel30fpsThrottle;
		Time lastCyclcTimestamp;
		bool cyclicExecutiveWaitForNextFrame;
		int nonCyclicJobsToDo;
		int cyclicExecutiveLoopId;
		class RBXBaseClass Arbiter
		{
		protected:
			ActivityMeter<2> activityMeter;
		public:
			virtual std::string arbiterName() = 0;
			virtual bool areExclusive(Job* job1, Job* job2) = 0;
			virtual bool isThrottled() = 0;
			virtual void preStep(TaskScheduler::Job* job) { activityMeter.increment(); }
			virtual void postStep(TaskScheduler::Job* job) { activityMeter.decrement(); }
			double getAverageActivity() { return activityMeter.averageValue(); }
			virtual Arbiter* getSyncronizationArbiter() { return this; };
            virtual int getNumPlayers() const {return 1;}
		};

		typedef enum 
		{ 
			Done,	        // The job will be removed from the TaskScheduler
			Stepped,        // Another step will be scheduled
		} StepResult;

		static TaskScheduler& singleton();

		typedef enum { LastError, AccumulatedError, FIFO } PriorityMethod;
		static PriorityMethod priorityMethod;

#ifdef RBX_TEST_BUILD
        static int findJobFPS;
        static bool updateJobPriorityOnWake;
#endif
		double threadAffinityPreference;
		typedef enum {PerCore4 = 104, PerCore3 = 103, PerCore2 = 102, PerCore1 = 101, Auto = 0, Threads1 = 1, Threads2 = 2, Threads3 = 3, Threads4 = 4, Threads8 = 8, Threads16 = 16} ThreadPoolConfig;

		bool shouldDropThread() const;
		void dropThread(Thread* thread);

		size_t getThreadCount() { return threadCount; }
		void setThreadCount(ThreadPoolConfig threadConfig);
		void disableThreads(int count, Threads& threads);
		void enableThreads(Threads& threads);

		void add(boost::shared_ptr<TaskScheduler::Job> job);
		void reschedule(boost::shared_ptr<TaskScheduler::Job> job);

		void remove(boost::shared_ptr<TaskScheduler::Job> job) { remove(job, false, NULL); }
		// This version of remove might lead to deadlocks in some cases, so try not to use it
		void removeBlocking(boost::shared_ptr<TaskScheduler::Job> job) { remove(job, true, NULL); }
		// This version of remove calls back several times a second while waiting.
		// You might use this to process events in order to avoid deadlocks.
		void removeBlocking(boost::shared_ptr<TaskScheduler::Job> job, boost::function<void()> callbackPing) { remove(job, true, callbackPing); }

		void getJobsInfo(std::vector<boost::shared_ptr<const Job> >& result);
		void getJobsByName(const std::string& name, std::vector<boost::shared_ptr<const Job> >& result);

		// Performance counters
		double numSleepingJobs() const { return sleepingJobCount.value(); }
		double numWaitingJobs() const { return waitingJobCount.value(); }
		double numRunningJobs() const { return averageRunningJobCount.value(); }
		double threadAffinity() const { return averageThreadAffinity.value(); }
		size_t threadPoolSize() const { return threads.size(); }
		double schedulerRate() const { return schedulerDutyCycle.rate(); }
		double getSchedulerDutyCyclePerThread() const;
        double getErrorCalculationRate() const { return errorCalculationPerSec.rate(); }
        double getSortFrequency() const { return sortFrequency.rate(); }
		rbx::atomic<int> taskCount;
		void printDiagnostics(bool aggregateJobs);
		void printJobs();
		void setJobsExtendedStatsWindow(double seconds); // set seconds to 0.0 to disable.
		void cancelCyclicExecutive();
		bool isCyclicExecutive() { return cyclicExecutiveEnabled; }
		void releaseCyclicExecutive(TaskScheduler::Job* job);
	private:
		// ** Here for thread saftey.  **
		struct CyclicExecutiveJob
		{
			boost::shared_ptr<TaskScheduler::Job> job;
			bool cyclicExecutiveExecuted;
			bool isRunning;

			CyclicExecutiveJob( const boost::shared_ptr<TaskScheduler::Job>& j )
			{
				job = j;
				cyclicExecutiveExecuted = false;
				isRunning = false;
			}

			// Allows for using std::find.
			bool operator==( const boost::shared_ptr<TaskScheduler::Job>& j ) { return job == j; }
			bool operator==( const TaskScheduler::Job& j ) { return job.get() == &j; }
		};

		TaskScheduler();
		~TaskScheduler();
		void endAllThreads();
		void sampleRunningJobCount();

		static bool jobCompare(const CyclicExecutiveJob& jobA, const CyclicExecutiveJob& jobB);

		void remove(boost::shared_ptr<TaskScheduler::Job> job, bool joinJob, boost::function<void()> callbackPing);
		void remove(const boost::shared_ptr<TaskScheduler::Job>& job, boost::shared_ptr<CEvent> joinEvent);

		void scheduleJob(Job& job);

		static bool areExclusive(Job* job1, Job* job2, const shared_ptr<Arbiter>& arbiterHint);
		bool conflictsWithScheduledJob(Job* item) const;

		void incrementThreadCount();
		void decrementThreadCount();

		RBX::mutex mutex;

		typedef std::set< shared_ptr<Job> > AllJobs;
		AllJobs allJobs;
		typedef boost::intrusive::list< Job, boost::intrusive::base_hook<SleepingHook> > SleepingJobs;
		SleepingJobs sleepingJobs;
		bool cyclicExecutiveEnabled;
		typedef std::vector< CyclicExecutiveJob > CyclicExecutiveJobs;
		CyclicExecutiveJobs cyclicExecutiveJobs;
		typedef boost::intrusive::list< Job, boost::intrusive::base_hook<WaitingHook> > WaitingJobs;
		WaitingJobs waitingJobs;

		shared_ptr<Job> nextScheduledJob;
		
		void wakeSleepingJobs();
		void enqueueWaitingJob(Job& job);
		Time::Interval getShortestSleepTime() const;
		boost::shared_ptr<Job> findJobToRun(boost::shared_ptr<Thread> requestingThread);
		boost::shared_ptr<Job> findJobToRunNonCyclicJobs(boost::shared_ptr<Thread> requestingThread, RBX::Time now);
		int numNonCyclicJobsWithWork(); 

		void checkStillWaitingNextFrame(Time now);

		RunningAverage<int> sleepingJobCount;
		RunningAverage<int> waitingJobCount;
		RunningAverage<int> averageRunningJobCount;
		RunningAverageDutyCycle<Time::Precise> schedulerDutyCycle; // time spent scheduling jobs
		RunningAverage<double> averageThreadAffinity;
        RunningAverageTimeInterval<> errorCalculationPerSec;
        RunningAverageTimeInterval<> sortFrequency;

        rbx::atomic<int> runningJobCount;

		Time nextWakeTime;

        Time lastSortTime;

		Threads threads;
		size_t desiredThreadCount;

		CEvent sampleRunningJobCountEvent;
		boost::scoped_ptr<boost::thread> runningJobCounterThread;

        rbx::atomic<int> threadCount;

		static rbx::thread_specific_reference<TaskScheduler::Job> currentJob;
		static void static_init();
	};


	// A simple arbiter that prevents all members of it to execute concurrently
	class ExclusiveArbiter : public TaskScheduler::Arbiter, boost::noncopyable
	{
	public:
		virtual bool areExclusive(TaskScheduler::Job* job1, TaskScheduler::Job* job2);
		virtual std::string arbiterName() { return "ExclusiveArbiter"; }
		virtual bool isThrottled() { return false; }
		static ExclusiveArbiter singleton;
	};


	
	class SimpleThrottlingArbiter : public TaskScheduler::Arbiter
	{
		mutable bool throttled;
        rbx::atomic<int> updatingThrottle;
        static rbx::atomic<int> arbiterCount;
	
	public:
		static bool isThrottlingEnabled;

		SimpleThrottlingArbiter()
			:throttled(false)
			,updatingThrottle(0)
		{
			++arbiterCount;
		}

		~SimpleThrottlingArbiter()
		{
            --arbiterCount;
		}

		virtual bool isThrottled() 
		{ 
			if (!isThrottlingEnabled)
				return false;

			long count = arbiterCount;
			if (count<=1)
				return false;
			if (updatingThrottle.swap(1) == 0)
			{
				double cutoff = ((double)RBX::TaskScheduler::singleton().getThreadCount()) / (double) count;
				// hysteresis
				if (throttled)
				{
					throttled = getAverageActivity() >= cutoff; 
				}
				else
				{
					throttled = getAverageActivity() >= 1.1 * cutoff; 
				}

				--updatingThrottle;
			}

			return throttled;
		}

	};


}
