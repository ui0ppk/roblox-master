#ifdef _WIN32
// To compile link with Dbghelp.lib
// The callstack in release is the same as usual, which means it isn't all that accurate.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif

// for debugging an exploit
#include "util/Analytics.h"

#include "CrashReporter.h"

#include <stdio.h>
#ifdef _WIN32
#include <DbgHelp.h>
#endif
#include <stdlib.h>
#include <time.h>


#include "../Raknet/Source/EmailSender.h"
#include "../Raknet/Source/FileList.h"
#include "../Raknet/Source/FileOperations.h"
#include "rbx/TaskScheduler.h"
#include "rbx/Log.h"
#include "rbx/CEvent.h"
#include "rbx/Debug.h"
#include "rbx/boost.hpp"
#include "FastLog.h"
#include "util/standardout.h"	

#include "boost/bind.hpp"

#ifdef _WIN32
#include "StrSafe.h"
#endif

#pragma optimize("", off)

CrashReporter* CrashReporter::singleton = NULL;

// More info at:
// http://www.codeproject.com/debug/postmortemdebug_standalone1.asp
// http://www.codeproject.com/debug/XCrashReportPt3.asp
// http://www.codeproject.com/debug/XCrashReportPt1.asp
// http://www.microsoft.com/msj/0898/bugslayer0898.aspx

#pragma warning ( disable : 4996 )			// TODO - revisit this compiler warning

LOGGROUP(HangDetection)
LOGGROUP(Crash)

DYNAMIC_FASTINTVARIABLE(WriteFullDmpPercent, 0)

CrashReporter::CrashReporter() :
	threadResult(0)
	,exceptionInfo(NULL)
	,reportCrashEvent(FALSE)
	,hangReportingEnabled(false)
	,isAlive(true)
	,deadlockCounter(0)
	,destructing(false)
	,immediateUploadEnabled(true)
{
	assert(singleton==NULL);
	singleton = this;
}

CrashReporter::~CrashReporter()
{
	destructing = true;

	if(watcherThread)
	{
		FASTLOG(FLog::Crash, "Closing crashreporter thread on destroy");
		reportCrashEvent.Set();
		watcherThread->join();
	}
}


HRESULT CrashReporter::GenerateDmpFileName(__out_ecount(cchdumpFilepath) char* dumpFilepath, int cchdumpFilepath, bool fastLog, bool fullDmp)
{
	char appDescriptor[_MAX_PATH];
	char dumpFilename[_MAX_PATH];
	HRESULT hr = S_OK;

	StringCchPrintf(appDescriptor, _MAX_PATH, "%s %s", controls.appName, controls.appVersion);

	if(FAILED(hr = StringCchCopy(dumpFilepath, cchdumpFilepath, controls.pathToMinidump)))
		return hr;
	WriteFileWithDirectories(dumpFilepath,0,0);
	AddSlash(dumpFilepath);
	unsigned i, dumpFilenameLen;
	StringCchCopy(dumpFilename, _MAX_PATH, appDescriptor);
	dumpFilenameLen=(unsigned) strlen(appDescriptor);
	for (i=0; i < dumpFilenameLen; i++)
		if (dumpFilename[i]==':')
			dumpFilename[i]='.'; // Can't have : in a filename

	if(FAILED(hr = StringCchCat(dumpFilepath, cchdumpFilepath, dumpFilename)))
		return hr;

	if (fullDmp)
	{
		if(FAILED(hr = StringCchCat(dumpFilepath, cchdumpFilepath, ".Full")))
			return hr;
	}
	
	if(FAILED(hr = StringCchCat(dumpFilepath, cchdumpFilepath, fastLog ? ".txt" : controls.crashExtention)))
		return hr;

	return S_OK;
}


LONG CrashReporter::ProcessExceptionHelper(struct _EXCEPTION_POINTERS *ExceptionInfo, bool writeFullDmp, bool noMsg, char* dumpFilepath)
{
	int dumpType = controls.minidumpType;

	if (writeFullDmp)
		dumpType |= MiniDumpWithFullMemory;

	if(FAILED(GenerateDmpFileName(dumpFilepath, _MAX_PATH, false, writeFullDmp)))
		return EXCEPTION_CONTINUE_SEARCH;

	HANDLE hFile = CreateFile(dumpFilepath,GENERIC_WRITE, FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return EXCEPTION_CONTINUE_SEARCH;

	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = GetCurrentThreadId();
	eInfo.ExceptionPointers = ExceptionInfo;
	eInfo.ClientPointers = FALSE;

	LONG result = EXCEPTION_EXECUTE_HANDLER;
	if (!MiniDumpWriteDump(
		GetCurrentProcess(), GetCurrentProcessId(),
		hFile, (MINIDUMP_TYPE)dumpType,
		ExceptionInfo ? &eInfo : NULL, NULL, NULL
		))
	{
		HRESULT hr = GetLastError();
		DWORD bytesWritten = 0;
		TCHAR msg[1024];
		StringCchPrintf(msg, ARRAYSIZE(msg), "MiniDumpWriteDump() failed with hr = 0x%08x", hr);
		WriteFile(hFile, &msg, (strlen(msg)+1) * sizeof(TCHAR), &bytesWritten, NULL);

		result = EXCEPTION_CONTINUE_SEARCH;
	}

	CloseHandle(hFile);
	return result;
}

namespace RBX{
std::string specialCrashType = "first"; // generic name in case there are multiple choices
bool gCrashIsSpecial = false;
}

LONG CrashReporter::ProcessException(struct _EXCEPTION_POINTERS *ExceptionInfo, bool noMsg)
{
    static bool allowSpecial = true;
	LONG result = EXCEPTION_EXECUTE_HANDLER;

    char dumpFilepath[_MAX_PATH] = {};

#ifdef RBX_RCC_SECURITY
    if (RBX::gCrashIsSpecial)
    {
        // generate the normal crash report.  
		result = ProcessExceptionHelper(ExceptionInfo, false, noMsg, dumpFilepath);
        // log the file name (without the path) to influx
        RBX::Analytics::InfluxDb::Points analyticsPoints;
        size_t startPoint = 0;
        for (size_t i = 0; i < _MAX_PATH; ++i)
        {
            if (dumpFilepath[i] == '\\')
            {
                startPoint = i+1;
            }
            else if (dumpFilepath[i] == 0)
            {
                break;
            }
        }
        analyticsPoints.addPoint("type", RBX::specialCrashType.c_str());
        analyticsPoints.addPoint("path", &dumpFilepath[startPoint]);
        analyticsPoints.report("report", 10000, true);
        RBX::Analytics::GoogleAnalytics::trackEventWithoutThrottling("Error", "LoggableCrash", &dumpFilepath[startPoint], 0, true);
        allowSpecial = false;
    }
    else
#endif
    {
	    if (rand() % 100 < DFInt::WriteFullDmpPercent)
	    	result = ProcessExceptionHelper(ExceptionInfo, true, noMsg, dumpFilepath);
        else
	        result = ProcessExceptionHelper(ExceptionInfo, false, noMsg, dumpFilepath);
    }

	return result;
}

LONG CrashReporter::ProcessExceptionInThead(struct _EXCEPTION_POINTERS *excpInfo)
{
	FASTLOG(FLog::Crash, "Processing exception in thread");
	// stack collection happens in it's own thread.
	// that way, we can report stacks for stack overflow exceptions.
	if(watcherThread)
	{
		exceptionInfo = excpInfo;

		reportCrashEvent.Set();

		watcherThread->join();

		watcherThread.reset(); //cleanup.

		return this->threadResult;
	}
	else
	{
		// no thread? no problem!
		return ProcessException(excpInfo, false);
	}
}

void CrashReporter::TheadFunc(struct _EXCEPTION_POINTERS *excpInfo)
{
	this->threadResult = ProcessException(excpInfo, false);
}



static LONG ProcessExceptionStatic(PEXCEPTION_POINTERS excpInfo)
{
	if (excpInfo == NULL) 
	{
		LONG result;
		// Generate exception to get proper context in dump
		__try 
		{
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
		} 
		__except(result = ProcessExceptionStatic(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) 
		{
		}
		return result;
	} 
	else
	{
		return CrashReporter::singleton->ProcessExceptionInThead(excpInfo);
	}
}

LONG WINAPI CrashExceptionFilter( struct _EXCEPTION_POINTERS *excpInfo )
{
#ifdef _DEBUG
	RBXASSERT(false);
#endif
	LONG result = ProcessExceptionStatic(excpInfo);

	// Force a hard termination of the process. 
	// exit() would call termination routines, flush buffers, etc.
	// The justification is that the program is in a bad state and
	// might deadlock rather than exiting properly.
	_exit(EXIT_FAILURE);
}



// http://blog.kalmbachnet.de/?postid=75
/*

Many programs are setting an own Unhandled-Exception-Filter , for catching unhandled exceptions and do some reporting or logging (for example creating a mini-dump ). 

Now, starting with VC8 (VS2005), MS changed the behaviour of the CRT is some security related and special situations. 
The CRT forces the call of the default-debugger (normally Dr.Watson) without informing the registered unhandled exception filter. The situations in which this happens are the following:

Calling abort if the abort-behaviour was set to _CALL_REPORTFAULT. This is the default setting for release builds! 
Failures detected by security checks (see also Compiler Security Checks In Depth ) This is also enabled by default! 
If no invalid parameter handler was defined (which is the default setting) and an invalid parameter was detected (this is done in many CRT functions by calling _invalid_parameter). 
So the conclusion is: The are many situations in which your user-defined Unhandled-Exception-Filter will never be called. This is a major change to the previous versions of the CRT and IMHO not very well documented.

The solution
If you don’t want this behavior and you will be sure that your handler will be called, you need to intercept the call to SetUnhandledExceptionFilter which is used by the CRT to disable all previously installed filters. You can achieve this for x86 with the following code:

*/
#ifndef _M_IX86
#error "The following code only works for x86!"
#endif
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(
LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) 
{
return NULL;
}
BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary("kernel32.dll");
	if (hKernel32==NULL) return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if(pOrgEntry==NULL) return FALSE;
	unsigned char newJump[ 100 ];
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;

	newJump[ 0 ] = 0xE9;  // JMP absolute
	memcpy(&newJump[ 1 ], &dwRelativeAddr, sizeof(pNewFunc));
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(), 
	  pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
	return bRet;
}

#define PROCESS_CALLBACK_FILTER_ENABLED     0x1

typedef BOOL (WINAPI* SetProcessUserModeExceptionPolicy)(__in DWORD dwFlags);
typedef BOOL (WINAPI* GetProcessUserModeExceptionPolicy)(__out LPDWORD lpFlags);

void fixExceptionsThroughKernel()
{
	// Try to make kernel not swallow exceptions on 64-bit os
	// http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/

	HMODULE hKernelDll = LoadLibrary("kernel32.dll");
	if(hKernelDll)
	{
		SetProcessUserModeExceptionPolicy set = 
			(SetProcessUserModeExceptionPolicy)GetProcAddress(hKernelDll, "SetProcessUserModeExceptionPolicy");
		GetProcessUserModeExceptionPolicy get = 
			(GetProcessUserModeExceptionPolicy)GetProcAddress(hKernelDll, "GetProcessUserModeExceptionPolicy");

		if (set != NULL && get != NULL)
		{
			FASTLOG(FLog::Crash, "Found Get/SetProcessUserModeExceptionPolicy");
			DWORD dwFlags;
			if (get(&dwFlags)) {
				FASTLOG1(FLog::Crash, "UserMode Exception Flags: %u. Setting CALLBACK_FILER off", dwFlags);
				set(dwFlags & ~PROCESS_CALLBACK_FILTER_ENABLED); // turn off bit 1
			}
		}
	}
}

void CrashReporter::Start()
{
	SetUnhandledExceptionFilter(CrashExceptionFilter);
	PreventSetUnhandledExceptionFilter();

	watcherThread.reset(new boost::thread(boost::bind(&CrashReporter::WatcherThreadFunc, CrashReporter::singleton)));
}


void CrashReporter::WatcherThreadFunc()
{
	RBX::set_thread_name("CrashReporter_WatcherThreadFunc");

	RBX::Log::current()->writeEntry(RBX::Log::Information, "WatcherThread Started");
	
	bool quit = false;
	while(!quit && !destructing)
	{
		if(!reportCrashEvent.Wait(3* 60* 1000 /*3 minutes*/))
		{
			if(isAlive)
			{
				isAlive = false; // reset to test for next notify.
			}
			else if(hangReportingEnabled)
			{
				// not responsive!
				if(deadlockCounter++ == 0)  // only report first hang in session.
				{
					logEvent("WatcherThread Detected hang.");
					FASTLOG(FLog::HangDetection, "WatcherThread Detected hang.");

					RBX::TaskScheduler::singleton().printJobs();

					LONG result;
					// Generate exception to get proper context in dump
					__try 
					{
						RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
					} 
					__except(result = ProcessException(GetExceptionInformation(), true /*silent*/), EXCEPTION_EXECUTE_HANDLER) 
					{
						LaunchUploadProcess();
					}

//					_exit(EXIT_FAILURE);
				}
			}
		}
		else if(!destructing)
		{
			FASTLOG(FLog::Crash,	"Processing non-hang");
			threadResult = ProcessException(exceptionInfo, false);
			quit = true;

			LaunchUploadProcess();
		}
	}
	logEvent("WatcherThread Exiting.");
}

void CrashReporter::EnableImmediateUpload(bool enabled)
{
	immediateUploadEnabled = enabled;
}

void CrashReporter::LaunchUploadProcess()
{
	if (!immediateUploadEnabled)
		return;

	// start new process to upload dmp
	char filename[ MAX_PATH ];
	DWORD size = GetModuleFileNameA( NULL, filename, MAX_PATH );
	if (size && !strstr(filename, "RCCService"))
	{
		FASTLOG(FLog::Crash, "Launching process to upload dmp.");
		STARTUPINFO si = {0};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		char cmd[256];
		sprintf_s(cmd, 256, "\"%s\" -d", filename);
		::CreateProcess(NULL, cmd, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	}
}
void CrashReporter::DisableHangReporting()
{
	hangReportingEnabled = false;
}


void CrashReporter::NotifyAlive()
{
	FASTLOG(FLog::HangDetection, "Letting watcher thread know we're alive");
	hangReportingEnabled = true;
	isAlive = true;
}
