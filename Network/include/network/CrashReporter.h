#ifndef __CRASH_REPORTER_H
#define __CRASH_REPORTER_H

#include <assert.h>
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "rbx/CEvent.h"


/// Holds all the parameters to CrashReporter::Start
struct CrashReportControls
{
	// Used to generate the dump filename.  Required with AOC_EMAIL_WITH_ATTACHMENT or AOC_WRITE_TO_DISK
	char appName[128];
	char appVersion[128];

	// Used with AOC_WRITE_TO_DISK .  Path to write to.  Not the filename, just the path. Empty string means the current directory.
	char pathToMinidump[260];
	char crashExtention[64];

	// How much memory to write. MiniDumpNormal is the least but doesn't seem to give correct globals. MiniDumpWithDataSegs gives more.
	int minidumpType;
};

/// \brief On an unhandled exception, will save a minidump and email it.
/// A minidump can be opened in visual studio to give the callstack and local variables at the time of the crash.
/// It has the same amount of information as if you crashed while debugging in the relevant mode.  So Debug tends to give
/// accurate stacks and info while Release does not.
///
/// Minidumps are only accurate for the code as it was compiled at the date of the release.  So you should label releases in source control
/// and put that label number in the 'appVersion' field.

void fixExceptionsThroughKernel();

class CrashReporter
{
private:
	LONG threadResult;
	struct _EXCEPTION_POINTERS *exceptionInfo;
	RBX::CEvent reportCrashEvent;
	boost::scoped_ptr<boost::thread> watcherThread;
	bool hangReportingEnabled;
	bool isAlive;
	LONG deadlockCounter;
	bool destructing;
	bool immediateUploadEnabled;

	void LaunchUploadProcess();

	LONG ProcessExceptionHelper(struct _EXCEPTION_POINTERS *ExceptionInfo, bool writeFullDmp, bool noMsg, char* dumpFilepath);

protected:
	bool silentCrashReporting;
	virtual void logEvent(const char* msg) {};
public:
	static CrashReporter* singleton;
	CrashReportControls controls;
	CrashReporter();
	~CrashReporter();
	void Start();
	void WatcherThreadFunc();
	virtual LONG ProcessException(struct _EXCEPTION_POINTERS *ExceptionInfo, bool noMsg);
	LONG ProcessExceptionInThead(struct _EXCEPTION_POINTERS *ExceptionInfo);
	void TheadFunc(struct _EXCEPTION_POINTERS *ExceptionInfo);
	void DisableHangReporting();
	void EnableImmediateUpload(bool enabled);

	// call every second or so from FG thread to signal responsive app.
	// must call at least once for hang reporting to be enabled.
	void NotifyAlive(); 

	HRESULT GenerateDmpFileName(__out_ecount(cchdumpFilepath) char* dumpFilepath, int cchdumpFilepath, bool fastLog = false, bool fullDmp = false);

};

#endif
