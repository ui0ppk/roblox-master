
#ifndef _A1177B91C54B40259B36DD55E5DF7726
#define _A1177B91C54B40259B36DD55E5DF7726

#include <list>
#include <string>

#include "rbx/Debug.h"

#include <sstream>
#include <fstream>

namespace RBX {

	class Log;

	// Returns a Log instance. Multithreaded apps should return a different
	// instance for each thread, so that Scope objects don't interact with each other
	class RBXInterface ILogProvider
	{
	public:
		virtual Log* provideLog() = 0;
	};

	class Log
	{
		const std::string name;
	public:
		// return a string representing the amount of memory
		static std::string formatMem(unsigned int bytes);
		static std::string formatTime(double time);

		enum Severity { Information=0, Warning=1, Error=2 };
		static Severity aggregateWorstSeverity;	 // The worst severity level reported by any Log
		Severity worstSeverity;	                 // The worst severity level reported by this Log

		void writeEntry(Severity severity, const char* message);
		void writeEntry(Severity severity, const wchar_t* message);
		void timeStamp(bool includeDate);

		static void setLogProvider(ILogProvider* provider);

		Log(const char* logFile, const char* name);
		virtual ~Log(void);

		const std::string logFile;

		static inline Log* current()
		{
			return provider ? provider->provideLog() : NULL;
		}

		static void timeStamp(std::ofstream& stream, bool includeDate);
	private:
		std::ofstream stream;


		static ILogProvider* provider;
		static inline std::ofstream& currentStream()
		{
			RBXASSERT(provider->provideLog()!=NULL);
			return provider->provideLog()->stream;
		}

		friend class Entry;
	};

}

#endif