#pragma once

#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#include "rbx/threadsafe.h"


class Marshaller
{
    typedef boost::function<void()>  Func;

public:

    Marshaller();
    ~Marshaller();

    // Executes the given function.
    void execute(Func job);

    // Submits a function to be executed by a separate thread.
    void submit(Func job);

    // blocks until something interesting happens
    void waitEvents();

    // processes all pending callbacks
    void processEvents();

    // returns marshaller thread
    DWORD getThreadId() const           { return mainThread; }

private:
    struct JobResult
    {
        bool                 success;
        std::string          except;
        void*                sync;
    };

    struct JobDesc
    {
        Func                 job;
        JobResult* volatile  result;
    };

    unsigned                 mainThread; // not the 'main' main thread
    void*                    hmainThread; // handle to the main thread

	RBX::mutex               jqMutex;
	std::vector< JobDesc >   jobQueue;
    RBX::mutex               cleanupMutex;
    std::vector< void** >    cleanup;

    void*    getSyncEvent();
	bool     runJob();
};

extern void main_processWinRTEvents(); // used to call into WinRT crap, defined in main.cpp
