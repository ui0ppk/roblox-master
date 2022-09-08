#pragma once

#include "rbx/boost.hpp"
#include "util/ScopedSingleton.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include <pdh.h>

class CQuery
{
	HQUERY handle;
public:
	CQuery(HQUERY handle):handle(handle)
	{
	}	
	CQuery():handle(0)
	{
	}
	HQUERY* operator&() { return &handle; }
	operator HQUERY() const { return handle; }
	~CQuery()
	{
		PdhCloseQuery(handle);
	}
};

class PerfCounter
{
protected:
	PerfCounter();
	CQuery hQuery;
	static void GetData2(HCOUNTER counter, long& result);
	static void GetData2(HCOUNTER counter, double& result);
public:
	void CollectData();
};

class CProcessPerfCounter : public PerfCounter, public RBX::ScopedSingleton<CProcessPerfCounter>
{
public:
	CProcessPerfCounter();
	CProcessPerfCounter(int pid);
	// The number of cores used by the process
	double GetProcessCores();
	double GetElapsedTime() { double result; PerfCounter::GetData2(elapsedTimeCounter, result); return result; }
	long GetTotalProcessorTime() { long result; PerfCounter::GetData2(totalProcessorTimeCounter, result); return result; }
	long GetProcessorTime() { long result; PerfCounter::GetData2(processorTimeCounter, result); return result; }
	long GetPrivateBytes() { long result; PerfCounter::GetData2(privateBytesCounter, result); return result; }
	long GetPageFaultsPerSecond() { long result; PerfCounter::GetData2(pageFaultsPerSecondCounter, result); return result; }
	long GetPageFileBytes() { long result; PerfCounter::GetData2(pageFileBytesCounter, result); return result; }
	long GetVirtualBytes() { long result; PerfCounter::GetData2(virtualBytesCounter, result); return result; }
	long GetPrivateWorkingSetBytes() { long result; PerfCounter::GetData2(workingSetPrivateCounter, result); return result; }

private:
	unsigned int numCores;
	SYSTEM_INFO systemInfo;
	HCOUNTER elapsedTimeCounter;
	HCOUNTER totalProcessorTimeCounter;
	HCOUNTER processorTimeCounter;
	HCOUNTER privateBytesCounter;
	HCOUNTER pageFaultsPerSecondCounter;
	HCOUNTER pageFileBytesCounter;
	HCOUNTER virtualBytesCounter;
	HCOUNTER workingSetPrivateCounter;
	void init(int pid);
};


#endif

