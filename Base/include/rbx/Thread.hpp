/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

namespace RBX
{
	// Returns a function that executes threadfunc with a given name
	boost::function0<void> thread_wrapper(const boost::function0<void>& threadfunc, const char* name);

	void set_thread_name(const char* name);

	// Returns the name of a thread if it was created with one of the above functions
	const char* get_thread_name();

	// The worker thread runs a process in a low-priority thread
	// The function you provide is called:
	// 1) Once after worker_thread is constructed
	// 2) Immediately after the function returns work_result::more
	// 3) After wake() is called
	//
	// The thread ends execution after worker_thread is deleted, but it doesn't
	// interrupt processing of work_function
	//
	// The client is responsible for ensuring that any data used by the work_function
	// is still valid. Note that the work_function might continue to execute for a
	// period of time after the owning worker_thread is destroyed.
	// 
	class worker_thread : public boost::noncopyable
	{
		struct data
		{
			boost::mutex sync;
			boost::condition_variable_any wakeCondition;	// TODO: condition_variable?
			bool endRequest;

			data():endRequest(false) {}
		};
		boost::shared_ptr<data> _data;
		boost::thread t;
	public:
		enum work_result { done, more };
		worker_thread(const boost::function0<work_result>& work_function, const char* name);
		~worker_thread();
		void wake();	// causes the work_function to be called (if the thread had been sleeping)
		void join();	// asks the thread to stop and then joins it
	private:
		static void threadProc(boost::shared_ptr<data> data, const boost::function0<work_result>& work_function);
	};

}
