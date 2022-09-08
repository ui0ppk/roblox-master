
#pragma once

#include "rbx/TaskScheduler.h"
#include <map>

namespace RBX
{
	namespace Tasks
	{
		// Prevents jobs from running according to some coordination logic.
		// Generally a Coordinator will affect execution order but not
		// affect parallelism. It may be more efficient to enforce resource
		// locks by specializing the TaskScheduler. See the DataModel scheduler.
		class RBXBaseClass Coordinator
		{
		public:
			// These functions must be written in a thread-safe manner
			// However, no 2 threads will call a function with the same job

			virtual bool isInhibited(TaskScheduler::Job* job) = 0;
			virtual void onPreStep(TaskScheduler::Job* job) {}	
			virtual void onPostStep(TaskScheduler::Job* job) {}
			virtual void onAdded(TaskScheduler::Job* job) {}
			virtual void onRemoved(TaskScheduler::Job* job) {}
		};

		// Prevents jobs from running in parallel
		class Exclusive : public Coordinator
		{
			volatile TaskScheduler::Job* runningJob;
		public:
			Exclusive():runningJob(NULL) {}
			virtual bool isInhibited(TaskScheduler::Job* job);
			virtual void onPreStep(TaskScheduler::Job* job);
			virtual void onPostStep(TaskScheduler::Job* job);
			virtual void onAdded(TaskScheduler::Job* job) {}
			virtual void onRemoved(TaskScheduler::Job* job) {}
		};

		// Requires that all coordinated jobs finish stepping before any job steps again.
		// It is the task-equivalent of a thread barrier.
		class Barrier : public Coordinator
		{
			unsigned int counter;
			unsigned int remainingTasks;
			RBX::mutex mutex;
			std::map<TaskScheduler::Job*, unsigned int> jobs;

			void releaseBarrier();
		public:
			Barrier():counter(0),remainingTasks(0) {}
			virtual bool isInhibited(TaskScheduler::Job* job);
			virtual void onPostStep(TaskScheduler::Job* job);
			virtual void onAdded(TaskScheduler::Job* job);
			virtual void onRemoved(TaskScheduler::Job* job);
		};

		class SequenceBase : public Coordinator 
		{
		private:
			unsigned int nextJobIndex;
			RBX::mutex mutex;
			std::vector<TaskScheduler::Job*> jobs;
		protected:
			void advance();
		public:
			SequenceBase():nextJobIndex(0) {}
			virtual bool isInhibited(TaskScheduler::Job* job);
			virtual void onAdded(TaskScheduler::Job* job);
			virtual void onRemoved(TaskScheduler::Job* job);
		};		
		
		// Requires that all coordinated jobs execute in sequence.
		// Jobs are allowed to run in parallel, but they must start
		// execution in the sequence in which they are added to the
		// coordinator
		class Sequence : public SequenceBase
		{
		public:
			virtual void onPreStep(TaskScheduler::Job* job) {
				advance();
			}
		};

		// Requires that all coordinated jobs execute in sequence.
		// Jobs are *not* allowed to run in parallel.
		// This is equivalent to Exclusive and Sequence combined
		class ExclusiveSequence : public SequenceBase
		{
		public:
			virtual void onPostStep(TaskScheduler::Job* job) {
				advance();
			}
		};


	}
}