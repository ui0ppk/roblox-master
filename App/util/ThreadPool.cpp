#include "stdafx.h"

#include "Util/ThreadPool.h"
#include "rbx/Debug.h"
#include "boost/cast.hpp"

namespace RBX
{
	BaseThreadPool::BaseThreadPool(int count, ShutdownPolicy shutdownPolicy,  PoolData* poolDataRaw, size_t maxScheduleSize)
		:poolData(poolDataRaw)
		,shutdownPolicy(shutdownPolicy)
		,count(count)
		,kMaxScheduleSize(maxScheduleSize)
	{
		pool.resize(count);
		poolLocks.resize(count);
		
		for (int i=0; i<count; ++i){
			poolLocks[i].reset(new rbx::spin_mutex());
			pool[i].reset(new boost::thread(boost::bind(&loop, poolData, poolLocks[i], shutdownPolicy)));
		}
	}
	int BaseThreadPool::getThreadCount() const
	{
		return count;
	}
	static void join(boost::shared_ptr<boost::thread> thread)
	{
		thread->join();
	}
	static void timed_join(boost::shared_ptr<boost::thread> thread, boost::posix_time::milliseconds relTime)
	{
		thread->timed_join(relTime);
	}
	BaseThreadPool::~BaseThreadPool()
	{
		{
			boost::unique_lock<boost::mutex> lock(poolData->mut);
			poolData->done = true;
			poolData->cond.notify_all();
		}

		switch(shutdownPolicy)
		{
		case WaitForRunningTasks:
			std::for_each(pool.begin(), pool.end(), boost::bind(&join, _1));
			break;
		case WaitForRunningTasksWithTimeout:
		{
			boost::posix_time::milliseconds timeout(100);
			std::for_each(pool.begin(), pool.end(), boost::bind(&timed_join, _1, timeout));
			break;
		}
		case LockAndKill:
		{
			RBXASSERT(pool.size() == poolLocks.size());
			boost::posix_time::milliseconds timeout(50);
			for(unsigned i = 0; i<pool.size(); ++i){
				rbx::spin_mutex::scoped_lock lock(*poolLocks[i]);
				pool[i]->timed_join(timeout);
			}
			break;
		}
		case NoAction:
		default:
			break;
		}
	}

	void BaseThreadPool::loop(boost::shared_ptr<PoolData> poolData, boost::shared_ptr<rbx::spin_mutex> lock, ShutdownPolicy shutdownPolicy)
	{
		RBX::set_thread_name("rbx_BaseThreadPool");
		while (true)
		{
			{
				boost::unique_lock<boost::mutex> lock2(poolData->mut);

				if (poolData->done)
					return;

				if (!poolData->fired)
					poolData->cond.wait(lock2);

				poolData->fired = false;
			}
			{
				boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> task;
				while (poolData->getNextTask(task))
				{
                    // Discard remaining tasks on shutdown
					if ((shutdownPolicy == WaitForRunningTasks || shutdownPolicy == WaitForRunningTasksWithTimeout) && poolData->done)
                        return;

					task(lock);
				}
			}
		}
	}

	void BaseThreadPool::taskAdded()
	{
		boost::unique_lock<boost::mutex> lock(poolData->mut);
		poolData->fired = true;
		poolData->cond.notify_one();
	}


	ThreadPool::ThreadPool(int count, ShutdownPolicy shutdownPolicy, size_t maxScheduleSize)
		:BaseThreadPool(count, shutdownPolicy, new ThreadPoolData(), maxScheduleSize)
	{}

	bool ThreadPool::schedule(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> task)
	{
		if (!shouldSchedule(poolData))
		{
			return false;
		}

		boost::polymorphic_downcast<ThreadPoolData*>(poolData.get())->queue.push(task);
		taskAdded();
		return true;
	}


	
	PriorityThreadPool::PriorityThreadPool(int count, ShutdownPolicy shutdownPolicy, size_t maxScheduleSize)
		:BaseThreadPool(count, shutdownPolicy, new PriorityThreadPoolData(), maxScheduleSize)
	{}

	bool PriorityThreadPool::schedule(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)> task, float priority)
	{
		if (!shouldSchedule(poolData))
		{
			return false;
		}

		priority = std::max(0.0f, priority);
		boost::polymorphic_downcast<PriorityThreadPoolData*>(poolData.get())->heap.push_heap(PriorityTask(task, priority));
		taskAdded();
		return true;
	}

	bool PriorityThreadPool::PriorityThreadPoolData::getNextTask(boost::function<void(boost::shared_ptr<rbx::spin_mutex>)>& task)
	{
		PriorityTask priTask;
		bool result = heap.pop_heap_if_present(priTask);
		if(result)
			task = priTask.func;
		return result;
	}


}
