#include "rbx/boost.hpp"
#include "rbx/Thread.hpp"

#include "RbxPlatform.h"

#include <boost/type_traits.hpp>
#include <boost/math/special_functions/fpclassify.hpp>


#ifndef _WIN32
#include "pthread.h"
#endif

#ifdef __APPLE__
#include "AvailabilityMacros.h"
#endif



namespace RBX {

	bool isFinite(double value)
	{
		return boost::math::isfinite(value);
	}

	bool isFinite(int value)
	{
		return boost::math::isfinite(value);
	}

	bool nameThreads = true;

	namespace boost_detail
	{
		boost::once_flag once_init_foo = BOOST_ONCE_INIT;
		static boost::thread_specific_ptr<std::string>* foo;
		void init_foo(void)
		{
			static boost::thread_specific_ptr<std::string> value;
			foo = &value;
		}

		static boost::thread_specific_ptr<std::string>& threadName()
		{
			boost::call_once(init_foo, once_init_foo);
			return *foo;
		}
	}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:6312)
#pragma warning(disable:6322)

	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;
	
	void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
	{
		if (nameThreads)
		{
			THREADNAME_INFO info;
			info.dwType = 0x1000;
			info.szName = szThreadName;
			info.dwThreadID = dwThreadID;
			info.dwFlags = 0;
			
			__try
			{
				// Comment this out to allow AQ time to work per Erik - 5/22/07
				// Search on AQTime, Profiling, VTune, Performance, threadName - to find this
				// TODO: Put an early check in DllMain to set nameThreads off if you pass in some cmd-line argument

				RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
			}
			__except(EXCEPTION_CONTINUE_EXECUTION)
			{
			}
		}
	}
#pragma warning(pop)
#endif

	void set_thread_name(const char* name)
	{
		RBX::boost_detail::threadName().reset(new std::string(name));
#ifdef _WIN32
		RBX::SetThreadName(GetCurrentThreadId(), name);
#else
#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5
		if (nameThreads)
			::pthread_setname_np(name);
#endif
#endif
	}

	const char* get_thread_name()
	{
		static const char* none = "unnamed thread";	// an alphanumeric default string
		std::string* result = RBX::boost_detail::threadName().get();
		return result ? result->c_str() : none;
	}

	static void thread_function(const boost::function0<void>& threadfunc, std::string name)
	{
		set_thread_name(name.c_str());
		threadfunc();
	}

	boost::function0<void> thread_wrapper(const boost::function0<void>& threadfunc, const char* name)
	{
		// We wrap "name" with a std::string to ensure the buffer is not lost
		return boost::bind(&thread_function, threadfunc, std::string(name));
	}

	worker_thread::worker_thread(const boost::function0<work_result>& work_function, const char* name)
		:_data(new data())
		// Start the thread
		,t(thread_wrapper(boost::bind(&threadProc, _data, work_function), name))
	{
	}

	worker_thread::~worker_thread()
	{
		boost::mutex::scoped_lock lock(_data->sync);
		_data->endRequest = true;
		_data->wakeCondition.notify_all();
		// _data will get collected when threadProc exits
	}

	void worker_thread::join()
	{
		{
			boost::mutex::scoped_lock lock(_data->sync);
			_data->endRequest = true;
			_data->wakeCondition.notify_all();
		}
		t.join();
	}

	void worker_thread::wake()
	{
		boost::mutex::scoped_lock lock(_data->sync);
		_data->wakeCondition.notify_all();
	}

	void worker_thread::threadProc(boost::shared_ptr<data> data, const boost::function0<work_result>& work_function)
	{
		while (true)
		{
			// Do all open work unless an end is requested
			do
			{
				if (data->endRequest)
					return;
			}
			while (work_function()==more);

			// Sleep until a request for more work comes through
			{
				boost::mutex::scoped_lock lock(data->sync);
				data->wakeCondition.wait(lock);
			}
		}

		// "data" will get collected when worker_thread is destroyed
	}

}
