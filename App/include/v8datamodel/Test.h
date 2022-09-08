/* Copyright 2003-2010 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "rbx/CEvent.h"
#include "V8World/World.h"
#ifdef check
#undef check
#endif
namespace RBX 
{
	class BaseScript;

	extern const char *const sTestService;
	class TestService
		: public DescribedCreatable<TestService, Instance, sTestService>
		, public Service
	{
		typedef DescribedCreatable<TestService, Instance, sTestService> Super;
		boost::function<void()> resumeFunction;
		bool running;
		int runCount;
		int scriptCount;

		int testCount;
		int warnCount;
		int errorCount;

		struct PhysicsSettings
		{
			EThrottle::EThrottleType eThrottle;
			bool allowSleep;
			bool throttleAt30Fps;
			FLog::Channel wasLogAsserts;
		};
		bool wasRunning;
		PhysicsSettings oldSettings;

	public:
		bool autoRuns;
		std::string description;
		double timeout;
		bool isPhysicsThrottled;
		bool allowSleep;
		bool is30FpsThrottleEnabled;
		float physicsSendRate; 

		// for multi-player test service
		int numberOfPlayers;
		bool clientTestService;
		double lagSimulation;

		TestService();
		~TestService();

		bool isPerformanceTest() const;

		// returns true if done, false if timed out
		void run(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

		// BOOST_MESSAGE
		void message(std::string text, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_CHECKPOINT
		void checkpoint(std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_WARN_MESSAGE
		void warn(bool condition, std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_CHECK_MESSAGE
		void check(bool condition, std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_REQUIRE_MESSAGE
		void require2(bool condition, std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_ERROR
		void error(std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		// BOOST_FAIL
		void fail(std::string description, shared_ptr<Instance> source = shared_ptr<Instance>(), int line = 0);

		void done();

		boost::function<void(std::string text, shared_ptr<Instance> source, int line)> onMessage;
		boost::function<void(bool condition, std::string description, shared_ptr<Instance> source, int line)> onWarn;
		boost::function<void(bool condition, std::string description, shared_ptr<Instance> source, int line)> onCheck;
		boost::function<void(std::string text, shared_ptr<Instance> source, int line)> onCheckpoint;
		boost::function<void(double time)> onStillWaiting;
		boost::function<void()> onDone;

		static Reflection::BoundProp<int, Reflection::READONLY> TestCount;
		static Reflection::BoundProp<int, Reflection::READONLY> WarnCount;
		static Reflection::BoundProp<int, Reflection::READONLY> ErrorCount;

#ifdef RBX_TEST_BUILD
		Verb* getVerb(std::string name);
		bool isCommandEnabled(std::string name);
		bool isCommandChecked(std::string name);
		void doCommand(std::string name);
		shared_ptr<const Reflection::ValueArray> getCommandNames();
#endif
		
		// for multi-player test service
		int getNumberOfPlayers(void) { return numberOfPlayers; }
		bool isMultiPlayerTest(void) { return numberOfPlayers > 0; }
		bool isAClient(void) { return clientTestService; }
		void setIsAClient(bool tF) { clientTestService = tF; }

		rbx::remote_signal<void(std::string, shared_ptr<Instance>, int)> serverCollectResultSignal;
		rbx::remote_signal<void(bool, std::string, shared_ptr<Instance>, int)> serverCollectConditionalResultSignal;
		void onRemoteResult( std::string text, shared_ptr<Instance> source, int line );
		void onRemoteConditionalResult( bool, std::string text, shared_ptr<Instance> source, int line );

	protected:
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		virtual bool askForbidChild(const Instance* instance) const;
	
	private:
		void onTimeout(int runCount, double timeout);
		void stillWaiting(int runCount, double timeout);
		void incrementTest();
		void incrementWarn();
		void incrementError();
		void startScripts();
		void stopScripts();
		void countScript(shared_ptr<Instance> instance);
		void startScript(shared_ptr<Instance> instance);
		void stopScript(shared_ptr<Instance> instance);
		void stop();

		void onScriptEnded(int runCount);
		void onScriptFailed(int runCount, const char* message, const char* callStack, shared_ptr<BaseScript> source, int line);

		void setConfiguration();
		void restoreConfiguration();

		std::string filterScript(const std::string& source);

		static void output(MessageType type, shared_ptr<Instance> source, int line, const std::string& message);

		rbx::signals::scoped_connection resultCollectConnection;
		rbx::signals::scoped_connection conditionalResultCollectConnection;

	};


	namespace Lua
	{
		// A primitive class that is able to parse *most* arguments in a Lua snippet.
		// You pass it code source that starts with a '(' or '['. It will then scan the
		// source until it finds the corresponding closing bracket. Along the way it emits
		// all arguments.
		//
		// Example. It can pase the following:
		//
		//     ( a , 34+s(r, 6), 'd\'fg', "sd'fs")
		//
		// Limitations. It can't parse the following, because it doesn't treat the function block as a group:
		//
		//     ( function() return 1, 2 end, "hello" )
		//
		// Workaround:
		//
		//     ( (function() return 1, 2 end), "hello" )
		//
		class ArgumentParser
		{
		private:
			template <typename Iterator>
			static Iterator parseString(Iterator first, Iterator last)
			{
				const char closing = *first++;
				for (; first != last; ++first)
				{
					if (*first == '\\')	// escape character
						++first;
					else if (*first == closing)
						return ++first;
				}
				throw std::runtime_error("Missing string closure");
			}
			static char getClosing(char opening)
			{
				switch (opening)
				{
				case '(': return ')';
				case '[': return ']';
				case '{': return '}';
				default: RBXASSERT(false); return 0;
				}
			}

			static void ignore() {}
			template <typename Iterator>
			static Iterator parse_arg(Iterator first, Iterator last, char closing)
			{
				for (Iterator iter = first; iter != last; )
				{
					if (*iter == closing)
						return iter;
					if (*iter == ',')
						return iter;

					if (*iter == '\'')
						iter = parseString(iter, last);
					else if (*iter == '"')
						iter = parseString(iter, last);
					else if (*iter == '(')
						iter = parseBracket(iter, last, boost::bind(&ignore));	// ignore the args
					else if (*iter == '[')
						iter = parseBracket(iter, last, boost::bind(&ignore));	// ignore the args
					else
						++iter;
				}
				return last;
			}

			static void appendArg(std::vector<std::string>* args, std::string::const_iterator begin, std::string::const_iterator end)
			{
				std::string arg;
				arg.assign(begin, end);
				args->push_back(arg);
			}
			static bool isWhitespace(char c)
			{
				return c == ' ' || c == '\t';
			}
		public:
			template <typename Iterator>
			static Iterator parseBracket(Iterator first, Iterator last)
			{
				return parseBracket(first, last, boost::bind(&ignore));
			}
			template <typename Iterator, typename EmitFunction>
			static Iterator parseBracket(Iterator first, Iterator last, EmitFunction emit)
			{
				// PRE: first points to the opening bracket
				// RETURNS: closing bracket
				const char closing = getClosing(*first++);

				bool foundComma = true;
				while (first != last)
				{
					if (*first == closing)
						return ++first;

					if (isWhitespace(*first))
					{
						++first;
						continue;
					}

					if (*first == ',')
					{
						foundComma = true;
						++first;
						continue;
					}

					if (!foundComma)
						throw std::runtime_error("argument not separated by comma");

					Iterator iter = parse_arg(first, last, closing);
					// trim trailing whitespace
					Iterator lastNonWhitespace = first;
					for (Iterator scan = first; scan != iter; ++scan)
					{
						if (!isWhitespace(*scan))
							lastNonWhitespace = scan + 1;
					}
					if (lastNonWhitespace != first)
						emit(first, lastNonWhitespace);
					first = iter;
					foundComma = false;
				}
				throw std::runtime_error("Missing closing bracket");
			}

			template <typename Iterator>
			static Iterator skipWhitespaces(Iterator first, Iterator end)
			{
				for (; first != end; ++first)
				{
					if (!isWhitespace(*first))
						break;
				}
				return first;
			}

			template <typename Iterator>
			static std::vector<std::string> getArgsInBracket(Iterator begin, Iterator end)
			{
				std::vector<std::string> args;
				ArgumentParser::parseBracket(begin, end, boost::bind(&appendArg, &args, _1, _2));
				return args;
			}

			static std::vector<std::string> getArgsInBracket(std::string source)
			{
				return getArgsInBracket(source.begin(), source.end());
			}
		};
	}


	// Deprecated. Use TestService instead
	extern const char* const sFunctionalTest;
	class FunctionalTest : public DescribedCreatable<FunctionalTest, Instance, sFunctionalTest>
	{
		typedef DescribedCreatable<FunctionalTest, Instance, sFunctionalTest> Super;
		// We route everything to the new TestService
		shared_ptr<TestService> testService;

	public:
		bool hasMigratedSettingsToTestService;
		std::string description;
		double timeout;
		bool isPhysicsThrottled;
		bool allowSleep;
		bool is30FpsThrottleEnabled;

		FunctionalTest();

		void pass(std::string message);
		void warn(std::string message);
		void error(std::string message);
	
		typedef enum { Passed, Warning, Error } Result;

	protected:
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		/*override*/ bool askSetParent(const Instance* instance) const
		{
			// Tests can be anywhere
			return true;
		}
	};

}	// namespace RBX
