/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/TaskScheduler.Job.h"

LOGGROUP(DataModelJobs)

namespace RBX {

	class DataModelArbiter;

	class DataModelJob : public TaskScheduler::Job
	{
	public:
		typedef enum { 
			Read = 0,		// general read-only access to the DataModel 
			Write,			// general read-writes access to the DataModel 
			Render, 
			Physics, 
			DataOut, 
			PhysicsOut, 
			PhysicsOutSort,
			DataIn, 
			PhysicsIn,
			RaknetPeer,
			None,
			TaskTypeMax
		} 
		TaskType;
		static const int TaskTypeCount = TaskTypeMax;
		TaskType taskType;

		virtual TaskScheduler::StepResult step(const Stats& stats);

	protected:
		virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) = 0;
		virtual double updateStepsRequiredForCyclicExecutive(float stepDt, float desiredHz, float maxStepsPerCycle, float maxStepsAccumulated);
		double stepsAccumulated;
		const bool isPerPlayer;
		unsigned long long profilingToken;
		DataModelJob(const char* name, TaskType taskType, bool isPerPlayer, shared_ptr<DataModelArbiter> arbiter, Time::Interval stepBudget);
		/*implement*/ double getPriorityFactor();	
	};

	class DataModelArbiter : public SimpleThrottlingArbiter
	{
	public:
		typedef enum
		{
			Serial = 0,
			Safe,
			Logical,
			Empirical
		}
		ConcurrencyModel;
		static const int ConcurrencyModelCount = 4;
		static ConcurrencyModel concurrencyModel;
		DataModelArbiter();
		virtual ~DataModelArbiter();
		virtual bool areExclusive(TaskScheduler::Job* job1, TaskScheduler::Job* job2);
		virtual int getNumPlayers() const = 0;
		bool areExclusive(DataModelJob::TaskType task1, DataModelJob::TaskType task2);

		virtual void preStep(TaskScheduler::Job* job);
		virtual void postStep(TaskScheduler::Job* job);

	private:
		bool lookup[ConcurrencyModelCount][DataModelJob::TaskTypeCount][DataModelJob::TaskTypeCount];
	};


} // namespace
