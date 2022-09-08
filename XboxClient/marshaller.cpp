
#include "marshaller.h"

#include <xdk.h>
#include <windows.h>
#include <synchapi.h>

#include "rbx/Debug.h"
#include "rbx/Profiler.h"

__declspec(thread) static void*  g_syncEvent; // per thread
extern void dprintf( const char* fmt, ... );

void* Marshaller::getSyncEvent()
{
    if( !g_syncEvent ) 
    {
        g_syncEvent = CreateEvent(0, 0, 0, 0);
        RBX::mutex::scoped_lock lck(cleanupMutex);
        cleanup.push_back(&g_syncEvent);
    }
    return g_syncEvent;
}


Marshaller::Marshaller()
{
    mainThread = GetCurrentThreadId();
    hmainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, mainThread);
    RBXASSERT(hmainThread);
    
    dprintf( "Marshaller started on thread %I64x (%I64d)\n", mainThread, mainThread );
}

Marshaller::~Marshaller()
{
    CloseHandle(hmainThread);
    for( int j=0, e=(int)cleanup.size(); j<e; ++j )
    {
        CloseHandle(*cleanup[j]);
        *cleanup[j] = 0;
    }
}


void Marshaller::submit(Func job)
{
    JobDesc desc = JobDesc();
    desc.job = job;
    desc.result = 0;
	RBX::mutex::scoped_lock lock (jqMutex);
	jobQueue.push_back(desc);
}



void Marshaller::execute(Func job)
{
    if( GetCurrentThreadId() == mainThread )
    {
        job();
        return;
    }

    void* syncEvent = getSyncEvent();
    ResetEvent(syncEvent); // just in case

    JobResult result = {};
    result.sync = syncEvent;

    JobDesc desc = JobDesc();
    desc.job = job;
    desc.result = &result;
	if(1)
	{
		RBX::mutex::scoped_lock lock (jqMutex);
		jobQueue.push_back(desc);
	}

	WaitForSingleObject( syncEvent, INFINITE ); // wait for the result

    if (!result.success)
    {
        throw RBX::runtime_error( "Marshaller cross-thread exception: '%s'", result.except.c_str() );
    }
}

void Marshaller::waitEvents()
{
    RBXPROFILER_SCOPE("xbox", __FUNCTION__);/*
	for( int j=0; j<20; ++j )
	{
		if( runJob() ) break;
		SleepEx(100, TRUE);
	}
	*/
}



void Marshaller::processEvents()
{
    RBXPROFILER_SCOPE("xbox", __FUNCTION__);
	while( runJob() ) {}
}

bool Marshaller::runJob()
{
	JobDesc desc;
	if(1)
	{
		RBX::mutex::scoped_lock lock (jqMutex);
		if(jobQueue.empty()) return false;
		desc = jobQueue.front();
		jobQueue.erase( jobQueue.begin() );
	}

    try
    {
        desc.job();
        if (desc.result)
            desc.result->success = true;
    }
    catch (const std::exception& e)
    {
        if (desc.result)
        {
            desc.result->success = false;
            desc.result->except = e.what();
        }
        else
        {
            // We've got nothing to report to, just dump it somewhere
            // TODO: replace with debugging stuff
            dprintf("%s\n", e.what());
            RBXASSERT(0); // haha, exceptions suck
        }
    }

    if (desc.result && desc.result->sync)
    {
        SetEvent(desc.result->sync);
    }

	return true;
}
