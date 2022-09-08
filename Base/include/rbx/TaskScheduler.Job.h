
#pragma once

#include "rbx/Countable.h"
#include "rbx/TaskScheduler.h"
#include "boost/enable_shared_from_this.hpp"
#include "boost/weak_ptr.hpp"

#define HANG_DETECTION 0

namespace RBX
{
	namespace Tasks
	{
		class Coordinator;
	}

	enum CyclicExecutiveJobPriority
	{
		CyclicExecutiveJobPriority_EarlyRendering,
		CyclicExecutiveJobPriority_Network_ReceiveIncoming,
		CyclicExecutiveJobPriority_Network_ProcessIncoming,
		CyclicExecutiveJobPriority_Default,
		CyclicExecutiveJobPriority_Physics,
		CyclicExecutiveJobPriority_Heartbeat,
		CyclicExecutiveJobPriority_Network_ProcessOutgoing,
		CyclicExecutiveJobPriority_Render
	};

	class RBXBaseClass TaskScheduler::Job 
		: boost::noncopyable
		, public boost::enable_shared_from_this<Job>
		, RBX::Diagnostics::Countable<Job>
		, public TaskScheduler::SleepingHook
		, public TaskScheduler::WaitingHook
	{
		friend class TaskScheduler;
		boost::weak_ptr<Thread> lastThreadUsed;	// attempt to re-use a thread for thread affinity

		RBX::mutex coordinatorMutex;
		std::vector<boost::shared_ptr<RBX::Tasks::Coordinator> > coordinators;
		boost::shared_ptr<TaskScheduler::Arbiter> const sharedArbiter;
		boost::weak_ptr<TaskScheduler::Arbiter> const weakArbiter;
		TaskScheduler::Arbiter* const baldArbiter;

		//stepBudget of 0 means no budget
	public:
		const std::string name;
		static double throttledSleepTime;
		const Time::Interval stepBudget;
		Time stepStartTime;
		int allotedConcurrency;
		bool cyclicExecutive;
		CyclicExecutiveJobPriority cyclicPriority;

#if HANG_DETECTION
		static double stepTimeThreshold;	// in seconds. A count is incremented when a job's stepTime is over this value. A value of 0 means disable counting.
		Time stepTimeSampleTime;
#endif

		struct Stats
		{
			Stats(Job& job, Time now);
            Time timeNow;
			Time::Interval timespanSinceLastStep;
			Time::Interval timespanOfLastStep;
		};

		struct Error
		{
			double error;

			bool urgent;	// experimental feature to prevent UI deadlocks (Essentially bumps the Job up in the priority queue)

			Error():error(0.0),urgent(false)
			{
			}
			Error(double error):error(error),urgent(false)
			{ 
			}
			bool isDefault() { return error == 0.0 && !urgent; } // Generic jobs are flagged as urgent for Cyclic Executive.
		};

		void addCoordinator(shared_ptr<Tasks::Coordinator> coordinator);
		void removeCoordinator(shared_ptr<Tasks::Coordinator> coordinator);

		Time::Interval getSleepingTime() const;	// returns > 0 if the job is asleep
		double averageDutyCycle() const;
		const RunningAverageDutyCycle<>& getStepStats() const { return dutyCycle; }
		double averageSleepRate() const;
		double averageStepsPerSecond() const;
		double averageStepTime() const;
		double averageError() const;
		bool isRunning() const { return state==Running; }
		bool isDisabled();

		typedef enum { None, LastSample, AverageInterval } SleepAdjustMethod;
		static SleepAdjustMethod sleepAdjustMethod;

		typedef enum { Unknown, Sleeping, Waiting, Running } State;
		State getState() const { return state; }
		Time getWakeTime() const { return wakeTime; }
		Time::Interval getWake() const { return wakeTime - Time::now<Time::Fast>(); }
		double getPriority() const { return priority; }

		std::string getDebugName() const
		{
			shared_ptr<Arbiter> ar(getArbiter());
			if (ar)
				return RBX::format("%s:%s", ar->arbiterName().c_str(), name.c_str());
			else
				return name;
		}

		static bool isLowerWakeTime(const TaskScheduler::Job& job1, const TaskScheduler::Job& job2)
		{
			return job1.wakeTime < job2.wakeTime;
		}

		WindowAverageDutyCycle<>& getDutyCycleWindow() { return dutyCycleWindow; }
		const WindowAverageDutyCycle<>& getDutyCycleWindow() const { return dutyCycleWindow; }

	protected:
		Job(const char* name, shared_ptr<TaskScheduler::Arbiter> arbiter, Time::Interval stepBudget = Time::Interval(0));
		virtual ~Job();

	public:
		inline const shared_ptr<TaskScheduler::Arbiter>& getArbiter() const
		{
			return sharedArbiter;
		}
		inline bool hasArbiter(TaskScheduler::Arbiter* test) const
		{
			return sharedArbiter.get() == test || sharedArbiter->getSyncronizationArbiter() == test; 
		};

		inline static bool haveDifferentArbiters(const Job* job1, const Job* job2) {
            Arbiter* a1 = job1->sharedArbiter.get();
            if(a1)
                a1 = a1->getSyncronizationArbiter();

            Arbiter* a2 = job2->sharedArbiter.get();
            if(a2)
                a2 = a2->getSyncronizationArbiter();
            
			return a1 != a2;
		}

		//////////////////////////////////////////////////////////////////
		// Abstract functions
	private:
		// sleepTime>0 means the job will sleep
		virtual Time::Interval sleepTime(const Stats& stats) = 0;	

		// error==0 means the job won't be scheduled
		virtual Error error(const Stats& stats) = 0;	

		// Used in Cyclic Executive to decide if we should re-run entire TaskScheduler loop
		// This is used to let LegacyLocks clear in Studio when waiting for Render job.
		virtual bool tryJobAgain() { return false; };

		// Used to determine which job gets priority. The priority is multiplied by this factor
		virtual double getPriorityFactor() = 0;	

		// The Job is being asked to step.
		virtual StepResult step(const Stats& stats) = 0;

		virtual int getDesiredConcurrencyCount() const 
		{
			// return >1 if the job intends to do parallel work (using multiple threads)
			// During the step function, query the number of threads alloted by checking
			// allotedConcurrency
			return 1;
		}
	protected:
		// Use this to generate the error function if you just want to try to track the desiredHz
		Error computeStandardError(const Stats& stats, double desiredHz);
		Error computeStandardErrorCyclicExecutiveSleeping(const Stats& stats, double desiredHz);
		Time::Interval computeStandardSleepTime(const Stats& stats, double desiredHz);

	private:
		State state;
		bool isRemoveRequested;
		Time timeofLastStep;
		Time timeofLastSleep;
		Time::Interval timespanOfLastStep;
		Error currentError;
		double priority;
		Time wakeTime;
		int overStepTimeThresholdCount;

		boost::shared_ptr<CEvent> joinEvent;	// Used when joining to the event after it is removed
		RunningAverageDutyCycle<> dutyCycle;
		RunningAverage<double> sleepRate;
		RunningAverage<double> runningAverageError;
		WindowAverageDutyCycle<> dutyCycleWindow;

		void updateError(const Time& time);
		void notifyCoordinatorsPreStep();
		void preStep();
		void postStep(StepResult result);
		void notifyCoordinatorsPostStep();
		void updatePriority();
		void updateWakeTime();
		void startSleeping();
		void startWaiting();
	};
}
