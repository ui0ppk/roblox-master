#pragma once

#include "rbx/threadsafe.h"

namespace RBX
{
	
	// TODO: Implement shutdown policies: WaitForPendingTasks, InterruptTasks, etc.
	class BaseThreadPool
	{
	public:
		enum ShutdownPolicy
		{
			WaitForRunningTasks,
			WaitForRunningTasksWithTimeout,
			LockAndKill,
			NoAction
		};

		struct PoolData
		{			
			volatile bool done;
			volatile bool fired;	// a task has been scheduled
			boost::condition_variable cond;
			boost::mutex mut;
			PoolData()
				:done(false) 
				,fired(false)
			{}
			virtual ~PoolData()
			{}
			virtual bool shouldSchedule(const BaseThreadPool *targetThreadPool) const = 0;

			virtual bool getNextTask(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)>& task) = 0;
		};
	private:
		int count;
		const size_t kMaxScheduleSize;
		std::vector< boost::shared_ptr<rbx::spin_mutex> > poolLocks;
		std::vector< boost::shared_ptr<boost::thread> > pool;
		ShutdownPolicy shutdownPolicy;

		static void loop(boost::shared_ptr<PoolData> poolData, boost::shared_ptr<rbx::spin_mutex> lock, ShutdownPolicy shutdownPolicy);

	protected:
		boost::shared_ptr<PoolData> poolData;
		bool shouldSchedule(const boost::shared_ptr<PoolData> &poolData) const
		{
			return poolData->shouldSchedule(this);
		}

		void taskAdded();
		
	public:
		BaseThreadPool(int count, ShutdownPolicy shutdownPolicy,
					   PoolData* poolData, size_t maxScheduleSize);
		int getThreadCount() const;
		size_t getMaxScheduleSize() const
		{
			return kMaxScheduleSize;
		}
		virtual ~BaseThreadPool();
	};

	//A ThreadPool with fifo ordering
	class ThreadPool:
		public BaseThreadPool
	{
	private:
		struct ThreadPoolData
			: public PoolData
		{
			typedef rbx::safe_queue<boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> > Queue;
			Queue queue;

			/*override*/ bool getNextTask(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)>& task)
			{
				return queue.pop_if_present(task);
			}

			/*override*/ bool shouldSchedule(const BaseThreadPool *targetThreadPool) const
			{
				return 0 == targetThreadPool->getMaxScheduleSize() || queue.size() < targetThreadPool->getMaxScheduleSize();
			}
		};
	protected:
		
	public:
		ThreadPool(int count, ShutdownPolicy shutdownPolicy = NoAction, size_t maxScheduleSize = 0);

		bool schedule(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> task);
	};

	//A ThreadPool with job priorities
	class PriorityThreadPool:
		public BaseThreadPool
	{
	private:
		struct PriorityTask
		{
			boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> func;
			float priority;
			PriorityTask(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> func, float priority)
				: func(func)
				, priority(priority)
			{}
			PriorityTask()
			{}

			bool operator<(const PriorityTask& rhs) const
			{
				return this->priority > rhs.priority;
			}
		};
		struct PriorityThreadPoolData
			: public PoolData
		{
			typedef rbx::safe_heap<PriorityTask> Heap;
			Heap heap;

			/*override*/ bool getNextTask(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)>& task);

			/*override*/ bool shouldSchedule(const BaseThreadPool *targetThreadPool) const
			{
				return 0 ==  targetThreadPool->getMaxScheduleSize() || heap.size() < targetThreadPool->getMaxScheduleSize();
			}
		};
	protected:
		
	public:
		PriorityThreadPool(int count, ShutdownPolicy shutdownPolicy = NoAction, size_t maxScheduleSize = 0 );

		bool schedule(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> task, float priority);
	};
}
