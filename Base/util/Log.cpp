#include "rbx/Log.h"
#include "RbxFormat.h"
#ifdef _WIN32
#include <Windows.h>
#define sprints sprintf_s
#define wcstombs wcstombs_s
#else
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/gregorian/greg_year.hpp"

using namespace RBX;

#include "rbx/rbxTime.h"

ILogProvider* Log::provider = NULL;

Log::Severity Log::aggregateWorstSeverity = Log::Information;

void Log::timeStamp(std::ofstream& stream, bool includeDate)
{
	boost::posix_time::ptime stime(boost::posix_time::second_clock::local_time());
	char s[256];

	if (includeDate)
	{
		boost::gregorian::date date(stime.date());
		snprintf(s, ARRAYSIZE(s), "%02u.%02u.%u ", date.day().as_number(), date.month().as_number(), (unsigned short)date.year());
		stream << s;
	}
	boost::posix_time::time_duration dur(stime.time_of_day());
	snprintf(s, ARRAYSIZE(s), "%02u:%02u:%02u.%03u (%03.07f)", dur.hours(), dur.minutes(), dur.seconds(), (unsigned short)dur.total_milliseconds(), Time::nowFastSec());
	stream << s;
	stream.flush();
}

Log::Log(const char* logFile, const char* name)
	:stream(logFile)
	,logFile(logFile)
	,worstSeverity(Information)
	,name(name)
{
	Log::timeStamp(stream, true);
	stream << "Log \"" << name << "\"\n";
	stream.flush();
}

Log::~Log(void)
{
	Log::timeStamp(stream, true);
	stream << "End Log\n";
}

void Log::setLogProvider(ILogProvider* provider)
{
	Log::provider = provider;
}

void Log::writeEntry(Severity severity, const wchar_t* message)
{
   // Convert to a char*
    size_t origsize = wcslen(message) + 1;
    const size_t newsize = origsize+100;
    size_t convertedChars = 0;
    char* nstring = new char[newsize];
#ifdef _WIN32
	wcstombs_s(&convertedChars, nstring, origsize, message, _TRUNCATE);
#else
    convertedChars = wcstombs(nstring, message, origsize);
#endif	
	if ( convertedChars >= origsize-1 )
		nstring[origsize-1] = '\0';
	writeEntry(severity, nstring);
	delete [] nstring;
}


void Log::writeEntry(Severity severity, const char* message)
{
	static const char* error =       " Error:   ";
	static const char* warning =     " Warning: ";
	static const char* information = "          ";
	Log::timeStamp(stream, false);
	switch (severity)
	{
		case Log::Error:
			stream << error;
			break;
		case Log::Warning:
			stream << warning;
			break;
		case Log::Information:
			stream << information;
			break;
	}
	stream << message;
	stream << '\n';
	stream.flush();
}

std::string Log::formatMem(unsigned int bytes)
{
	char buffer[64];
	if (bytes<1000)

		snprintf(buffer, ARRAYSIZE(buffer), "%dB", bytes);
	else if (bytes<100000)
		snprintf(buffer, ARRAYSIZE(buffer), "%.1fKB", ((double)bytes)/1000.0);
	else if (bytes<1000000)

		snprintf(buffer, ARRAYSIZE(buffer), "%.0fKB", ((double)bytes)/1000.0);
	else if (bytes<100000000)
		snprintf(buffer, ARRAYSIZE(buffer), "%.1fMB", ((double)bytes)/1000000.0);
	else if (bytes<1000000000)

		snprintf(buffer, ARRAYSIZE(buffer), "%.0fMB", ((double)bytes)/1000000.0);
#ifdef _WIN32
	// NOTE: 100000000000 is too big to fit in uint. EL
	else if (bytes<100000000000)
		snprintf(buffer, ARRAYSIZE(buffer), "%.1fGB", ((double)bytes)/1000000000.0);
#endif
	else
		snprintf(buffer, ARRAYSIZE(buffer), "%.0fGB", ((double)bytes)/1000000000.0);
	return buffer;
}

std::string Log::formatTime(double time)
{
	char buffer[64];
	if (time==0.0)
		snprintf(buffer, ARRAYSIZE(buffer), "0s");
	if (time<0.0)
		snprintf(buffer, ARRAYSIZE(buffer), "%.3gs", time);
	else if (time>=0.1)
		snprintf(buffer, ARRAYSIZE(buffer), "%.3gs", time);
	else
		snprintf(buffer, ARRAYSIZE(buffer), "%.3gms", time*1000.0);
	return buffer;
}



void Log::timeStamp(bool includeDate)
{
	Log::timeStamp(Log::currentStream(), includeDate);
}

LOGVARIABLE(Crash, 1)
LOGVARIABLE(HangDetection, 0)
LOGVARIABLE(ContentProviderCleanup, 0)
LOGVARIABLE(ISteppedLifetime, 0)
LOGVARIABLE(MutexLifetime, 0)
LOGVARIABLE(TaskScheduler, 0)
LOGVARIABLE(TaskSchedulerInit, 0)
LOGVARIABLE(TaskSchedulerRun, 0)
LOGVARIABLE(TaskSchedulerFindJob, 0)
LOGVARIABLE(TaskSchedulerSteps, 0)
LOGVARIABLE(Asserts, 0)
LOGVARIABLE(FWLifetime, 0)
LOGVARIABLE(FWUpdate, 0)
LOGVARIABLE(KernelStats, 0)

void initBaseLog()
{
}
