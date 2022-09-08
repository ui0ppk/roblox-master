/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <queue>
#include "rbx/boost.hpp"
#include "rbx/Thread.hpp"
#include "rbx/rbxtime.h"
#include "rbx/atomic.h"
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "RbxFormat.h"
#include "FastLog.h"

#include "RbxPlatform.h"

#include "boost/thread/mutex.hpp"

using boost::shared_ptr;

LOGGROUP(MutexLifetime);

namespace RBX
{
	// A lightweight mutex that uses CRITICAL_SECTION under Windows.
	// This mutex is non-recursive.
	class mutex
	{
#ifdef _WIN32
		CRITICAL_SECTION cs;
	public:
		mutex()
		{
			::InitializeCriticalSection( &cs );
			FASTLOG1(FLog::MutexLifetime, "RBX::mutext init m = 0x%x", this);
		}		
		~mutex()
		{
			::DeleteCriticalSection(&cs);
			FASTLOG1(FLog::MutexLifetime, "RBX::mutext destroy m = 0x%x", this);
		}
		class scoped_lock : boost::noncopyable
		{
		public:
			scoped_lock(mutex& m):m(m)
			{
				::EnterCriticalSection(&m.cs);
			}
			~scoped_lock()
			{
				::LeaveCriticalSection(&m.cs);
			}
		private:
			mutex& m;
		};
#else
		pthread_mutex_t sl;

	public:
		mutex()
		{
			if ( pthread_mutex_init(&sl,NULL) != 0 )
				throw std::runtime_error("failed in mutex to initialize pthread_mutex_init.");
		}
		~mutex()
		{
			if(pthread_mutex_destroy(&sl) != 0){
				//printf("Error at pthread_spin_destroy()");
			}
		}
		class scoped_lock : boost::noncopyable
		{
		public:
			scoped_lock(mutex& m0):m(m0){
				int rc = pthread_mutex_lock(&m.sl);
				if(rc != 0)
				{
					//fprintf(stderr,"Test FAILED: child failed to get spin lock,error code:%d\n" , rc);
				}
				//::EnterCriticalSection(&m.sl);
			}
			~scoped_lock(){
				if(pthread_mutex_unlock(&m.sl)!=0)
				{
					//fprintf(stderr,"child: Error at pthread_spin_unlock()\n");
				}
				//::LeaveCriticalSection(&m.sl);
			}
		private:
			mutex& m;
		};		
#endif	
	};

	// calls RBXCRASH() on contention.
	class concurrency_catcher : boost::noncopyable
	{
        rbx::atomic<int> value;
		static const long unlocked = 0;
		static const long locked = 1;
	public:
		concurrency_catcher():value(unlocked) {}
		class scoped_lock : boost::noncopyable
		{
		public:
			scoped_lock(concurrency_catcher& m);
			~scoped_lock();
		private:
			concurrency_catcher& m;
		};
	};

	
	// calls RBXCRASH() on contention.
	struct reentrant_concurrency_catcher : boost::noncopyable
	{
        rbx::atomic<int> value;
		volatile unsigned long threadId;
		static const long unlocked = 0;
		static const long locked = 1;
		static const unsigned long noThreadId;
	public:
		reentrant_concurrency_catcher():value(unlocked),threadId(noThreadId) {}
		class scoped_lock : boost::noncopyable
		{
		public:
			scoped_lock(reentrant_concurrency_catcher& m);
			~scoped_lock();
		private:
			bool isChild;
			reentrant_concurrency_catcher& m;
		};
	};


	class readwrite_concurrency_catcher : boost::noncopyable
	{
		friend class scoped_write_request;
		friend class scoped_read_request;
        rbx::atomic<int> write_requested;
        rbx::atomic<int> read_requested;
		static const long unlocked = 0;
		static const long locked = 1;
	public:

		readwrite_concurrency_catcher() : write_requested(unlocked), read_requested(0) {};
		class scoped_write_request
		{
			readwrite_concurrency_catcher& m;
		public:
			// Place this code around tasks that write to a DataModel
			scoped_write_request(readwrite_concurrency_catcher& mt);
			~scoped_write_request();
		};
		class scoped_read_request
		{
			readwrite_concurrency_catcher& m;
		public:
			// Place this code around tasks that write to a DataModel
			scoped_read_request(readwrite_concurrency_catcher& m);
			~scoped_read_request();
		};	
	};

}

namespace rbx
{

	class spin_mutex
	{
        rbx::atomic<int> sl;

	public:
		spin_mutex()
		{
			// init			
		}

		~spin_mutex()
		{
			// destroy
		}
        
        bool try_lock(){
			return sl.compare_and_swap(1,0) == 0;
        }
		
		void lock(){
			while(sl.compare_and_swap(1,0)!=0){}
		}
        void unlock(){
            sl.compare_and_swap(0,1);
        }

		class scoped_lock : boost::noncopyable
		{
		public:
			scoped_lock(spin_mutex& m0) : m(m0)
			{
				for(;;){
					if(m.try_lock()) break;
				}
			}

			~scoped_lock()
			{
				m.unlock();
			}
		private:
			spin_mutex& m;
		};
	};

	// Use this queue when you want fast performance, low
	// resource usage, and the queue is not very busy.
	// For very busy queues, use tbb::concurrent_queue (correction, we dont have tbb anymore).
	template<typename T>
	class safe_queue : boost::noncopyable
	{
	protected:
		std::queue<T> queue;
		// TODO: spin_mutex is possibly a bad choice for expensive T types
		typedef spin_mutex mutex;
		mutex m;
	public:
		void clear()
		{
			mutex::scoped_lock lock(m);
			
			while (!queue.empty())
				queue.pop();
		}
		void push(const T& value)
		{
			mutex::scoped_lock lock(m);
			queue.push(value);
		}
		bool pop_if_present(T& value)
		{
			mutex::scoped_lock lock(m);
			if (!queue.empty())
			{
				value = queue.front();
				queue.pop();
				return true;
			}
			else
				return false;
		}

		bool pop_if_present()
		{
			mutex::scoped_lock lock(m);
			if (!queue.empty())
			{
				queue.pop();
				return true;
			}
			else
				return false;
		}

		// WARNING: Peeking has side effects, if T has copy constructors and destructors
		bool peek_if_present(T& value)
		{
			mutex::scoped_lock lock(m);
			if (!queue.empty())
			{
				value = queue.front();
				return true;
			}
			else
				return false;
		}

		// Lock and spin-free calls:
		inline size_t size() const { return queue.size(); }
		inline bool empty() const { return queue.empty(); }
	};

	namespace implementation
	{
		template<typename T>
		struct timestamped_safe_queue_item
		{
			T value;
			RBX::Time timestamp;
			timestamped_safe_queue_item() {}
			timestamped_safe_queue_item(const T& value)
				:timestamp(RBX::Time::now<RBX::Time::Fast>())
				,value(value)
			{}
		};
	}
	template<typename T>
	class timestamped_safe_queue : protected safe_queue< implementation::timestamped_safe_queue_item<T> >
	{
		typedef safe_queue< implementation::timestamped_safe_queue_item<T> > Super;
        double headTimestamp;
#ifndef _WIN32
		// GCC won't inherit the mutex type defined in Super. Therefore we redeclare it here. Yuck!
		typedef spin_mutex mutex;
#endif
	public:
		void clear()
		{
            headTimestamp = 0.f;
			Super::clear();
		}

		void push(const T& value)
		{
			implementation::timestamped_safe_queue_item<T> item(value);
			Super::push(item);
            headTimestamp = item.timestamp.timestampSeconds();
		}

		bool pop_if_present(T& value)
		{
			mutex::scoped_lock lock(this->m);
			if (!this->queue.empty())
			{
                value = this->queue.front().value;
				this->queue.pop();
                if (!this->queue.empty())
                {
                    headTimestamp = this->queue.front().timestamp.timestampSeconds();
                }
                else
                {
                    headTimestamp = 0.f;
                }
				return true;
			}
			else
				return false;
		}

		// pops the head item if it has been waiting at least waitTime
		bool pop_if_waited(RBX::Time::Interval waitTime, T& value)
		{
			mutex::scoped_lock lock(this->m);
			if (this->queue.empty())
				return false;
			if (RBX::Time::now<RBX::Time::Fast>() < this->queue.front().timestamp + waitTime)
				return false;
			value = this->queue.front().value;
			this->queue.pop();
            if (!this->queue.empty())
            {
                headTimestamp = this->queue.front().timestamp.timestampSeconds();
            }
            else
            {
                headTimestamp = 0.f;
            }
			return true;
		}

		// Returns the time that the head item has been waiting or zero.
		double head_waittime_sec(const RBX::Time& timeNow) const
		{
			if (headTimestamp > 0.f)
            {
                return timeNow.timestampSeconds() - headTimestamp;
            }
			else
				return 0.f;
		}

		inline size_t size() const { return this->queue.size(); }
		inline bool empty() const { return this->queue.empty(); }
	};


	template<typename T>
	class safe_heap : boost::noncopyable
	{
		std::vector<T> vector;
		// TODO: spin_mutex is possibly a bad choice for expensive T types
		typedef spin_mutex mutex;
		mutex m;
	public:
		void clear()
		{
			mutex::scoped_lock lock(m);
			vector.clear();
		}
		void push_heap(const T& value)
		{
			mutex::scoped_lock lock(m);
			vector.push_back(value);
			std::push_heap(vector.begin(), vector.end());
		}
		bool pop_heap_if_present(T& value)
		{
			mutex::scoped_lock lock(m);
			if (!vector.empty())
			{
				std::pop_heap(vector.begin(), vector.end());
				value = vector.back();
				vector.pop_back();
				return true;
			}
			else
				return false;
		}

		bool pop_heap_if_present()
		{
			mutex::scoped_lock lock(m);
			if (!vector.empty())
			{
				std::pop_heap(vector.begin(), vector.end());
				vector.pop_back();
				return true;
			}
			else
				return false;
		}

		// Lock and spin-free calls:
		inline size_t size() const { return vector.size(); }
		inline bool empty() const { return vector.empty(); }
	};


#define SAFE_STATIC(TYPE,NAME) \
    static TYPE* safe_static_do_get_##NAME() { static TYPE value; return &value; }\
    static void safe_static_init_##NAME() { safe_static_do_get_##NAME(); }\
	static TYPE& NAME()\
	{\
		static boost::once_flag once_init_##NAME = BOOST_ONCE_INIT;\
		boost::call_once(safe_static_init_##NAME, once_init_##NAME);\
		return *safe_static_do_get_##NAME();\
	}
	
#define SAFE_HEAP_STATIC(TYPE,NAME) \
static TYPE* safe_static_do_get_##NAME() { static TYPE* value = new TYPE; return value; }\
static void safe_static_init_##NAME() { safe_static_do_get_##NAME(); }\
static TYPE& NAME()\
{\
static boost::once_flag once_init_##NAME = BOOST_ONCE_INIT;\
boost::call_once(safe_static_init_##NAME, once_init_##NAME);\
return *safe_static_do_get_##NAME();\
}


	// A wrapper around thread_specific_ptr that lets you
	// have a thread-specific reference to an object
	template<typename T>
	class thread_specific_reference
	{
		typedef T* TPTR;
		boost::thread_specific_ptr<TPTR> ptr;
	public:
		T* get() 
		{ 
			TPTR* p = ptr.get();
			if (p)
				return *p;
			else
				return 0;
		}
		void reset(T* value)
		{
			TPTR* p = new TPTR(value);
			ptr.reset(p);
		}
	};


	// A wrapper around thread_specific_ptr that lets you
	// have a thread-specific shared_ptr to an object
	template<typename T>
	class thread_specific_shared_ptr : boost::noncopyable
	{
		typedef shared_ptr<T> TPTR;
		boost::thread_specific_ptr<TPTR> ptr;
	public:
		operator shared_ptr<T>() const
		{ 
			TPTR* p = ptr.get();
			if (p)
				return *p;
			else
				return shared_ptr<T>();
		}

		void reset(shared_ptr<T> value)
		{
			TPTR* p = new TPTR(value);
			ptr.reset(p);
		}
	};


}
