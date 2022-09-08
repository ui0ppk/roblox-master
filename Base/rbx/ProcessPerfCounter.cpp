#include "rbx/ProcessPerfCounter.h"

#include "rbx/Debug.h"
#include "rbx/RbxDbgInfo.h"
#include <map>

#ifdef _WIN32
#include "PdhMsg.h"
#pragma comment (lib,"pdh.lib")

PerfCounter::PerfCounter()
{
#ifdef _WIN32
	PDH_STATUS pdhResult = PdhOpenQuery( NULL, 0, &hQuery );
#else
#warning "MACPORT - NEED TO HANDLE THIS CASE ON THE MAC"
#endif
}

CProcessPerfCounter::CProcessPerfCounter()
{
#ifdef _WIN32
	init(::GetCurrentProcessId());
#else
#warning "MACPORT - NEED TO HANDLE THIS CASE ON THE MAC"
#endif
}
CProcessPerfCounter::CProcessPerfCounter(int pid)
{
	init(pid);
}
void PerfCounter::CollectData()
{
#ifdef _WIN32
	PDH_STATUS result = PdhCollectQueryData(hQuery);
#else
#warning "MACPORT - NEED TO HANDLE THIS CASE ON THE MAC"
#endif
}

void PerfCounter::GetData2(HCOUNTER counter, long& result)
{
#ifdef _WIN32
	PDH_FMT_COUNTERVALUE stFormattedValue = {0};
	PDH_STATUS pdhResult = PdhGetFormattedCounterValue( counter
	, PDH_FMT_LONG
	, NULL
	, &stFormattedValue
	);
	result = stFormattedValue.longValue;
#else
#warning "MACPORT - NEED TO HANDLE THIS CASE ON THE MAC"
#endif
}


void PerfCounter::GetData2(HCOUNTER counter, double& result)
{
#ifdef _WIN32
	PDH_FMT_COUNTERVALUE stFormattedValue = {0};
	PDH_STATUS pdhResult = PdhGetFormattedCounterValue( counter
	, PDH_FMT_DOUBLE
	, NULL
	, &stFormattedValue
	);
	result = stFormattedValue.doubleValue;
#else
#warning "MACPORT - NEED TO HANDLE THIS CASE ON THE MAC"
#endif
}



double CProcessPerfCounter::GetProcessCores()
{
	double totalProcessorTime;	// 0%-100% of all CPUs
	GetData2(totalProcessorTimeCounter, totalProcessorTime);
	double processorTime; // 0%-100% of all processes
	GetData2(processorTimeCounter, processorTime);

	return totalProcessorTime / 100.0 * processorTime / 100.0 * (double)numCores; 
}

void CProcessPerfCounter::init(int pid)
{
	::GetSystemInfo(&systemInfo);
	numCores = systemInfo.dwNumberOfProcessors;
	RBX::RbxDbgInfo::s_instance.NumCores = numCores;

	totalProcessorTimeCounter = 0;
	processorTimeCounter = 0;
	privateBytesCounter = 0;
	pageFaultsPerSecondCounter = 0;
	pageFileBytesCounter = 0;
	virtualBytesCounter = 0;
	workingSetPrivateCounter = 0;

	TCHAR* buffer = new TCHAR[100000];
	TCHAR* instanceName;
	{
		DWORD length = 100000;
		DWORD dummy = 0;
		PDH_STATUS status = PdhEnumObjectItems(NULL, NULL, "Process", NULL, &dummy, buffer, &length, PERF_DETAIL_EXPERT, 0);
		RBXASSERT(SUCCEEDED(status) || status == PDH_MORE_DATA);

		std::map<std::string, int> instanceCount;
		instanceName = buffer;
		while (instanceName[0]!=0)
		{
			std::string name(instanceName);
			// Insert the item if it doesn't exist:
			instanceCount.insert(std::pair<std::string, int>(name, 0));

			TCHAR szCounterPath[1024];
			DWORD dwPathSize = 1024;
			PDH_COUNTER_PATH_ELEMENTS pe = {0};
			CQuery hQuery;
			PDH_STATUS pdhResult = PdhOpenQuery( NULL, 0, &hQuery );
			pe.szObjectName = "Process";
			pe.szCounterName = "ID Process";
			pe.szInstanceName = instanceName;
			pe.dwInstanceIndex = instanceCount[name]++;
			pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
			
			CQuery counter;
			pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &counter);
			pdhResult = PdhCollectQueryData(hQuery);

			PDH_FMT_COUNTERVALUE stFormattedValue = {0};
			pdhResult = PdhGetFormattedCounterValue( counter, PDH_FMT_LONG, NULL, &stFormattedValue);

			if (stFormattedValue.longValue==pid)
				break;

			instanceName += strlen(instanceName) + 1;
		}
	}

	TCHAR szCounterPath[1024];
	DWORD dwPathSize = 1024;
	PDH_COUNTER_PATH_ELEMENTS pe = {0};
	pe.szObjectName = "Process";
	pe.szCounterName = "% Processor Time";
	pe.szInstanceName = instanceName;
	PDH_STATUS pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &processorTimeCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Elapsed Time";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &elapsedTimeCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Private Bytes";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &privateBytesCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Page Faults/sec";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &pageFaultsPerSecondCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Page File Bytes";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &pageFileBytesCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Virtual Bytes";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	RBXASSERT(SUCCEEDED(pdhResult));
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &virtualBytesCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	pe.szCounterName = "Working Set - Private";
	dwPathSize = 1024;
	pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
	pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &workingSetPrivateCounter);
	if (pdhResult!=0)
	{
		// WinXP doesn't support "Working Set - Private"
		pe.szCounterName = "Working Set";
		dwPathSize = 1024;
		pdhResult = PdhMakeCounterPath(&pe, szCounterPath, &dwPathSize, 0);
//		RBXASSERT(SUCCEEDED(pdhResult));
		pdhResult = PdhAddCounter(hQuery, szCounterPath, 0, &workingSetPrivateCounter);
//		RBXASSERT(SUCCEEDED(pdhResult));
	}

	pdhResult = PdhAddCounter(hQuery, "\\Processor(_Total)\\% Processor Time", 0, &totalProcessorTimeCounter);
	RBXASSERT(SUCCEEDED(pdhResult));

	delete [] buffer;
}


#endif


