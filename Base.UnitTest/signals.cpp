
#include <boost/test/unit_test.hpp>
#include "rbx/signal.h"

class Foo {};

static void add(int i, int& result)
{
	result += i;
}

static void throw_runtime_error()
{
	throw std::runtime_error("");
}

static void exception_handler(std::exception& ex)
{
	BOOST_CHECK(false);
}

static void addConnection(int i, int& result, rbx::signal<void(int)>& sig1)
{
	add(i, result);
	sig1.connect(boost::bind(&add, _1, boost::ref(result)));
}
static void removeConnection(int i, int& result, rbx::signals::connection& connection)
{
	add(i, result);
	connection.disconnect();
}

static void append(std::string* s, const char* append)
{
	*s += append;
}

void emptyFunction()
{
}

void emptyFunction2(int)
{
}

class SignalsFixture
{
	boost::function<void(std::exception&)> slot_exception_handler;
public:
	SignalsFixture()
	{
		slot_exception_handler = rbx::signals::slot_exception_handler;
		rbx::signals::slot_exception_handler = &exception_handler;

	}
	~SignalsFixture()
	{
		rbx::signals::slot_exception_handler = slot_exception_handler;
	}
};

BOOST_FIXTURE_TEST_SUITE(Signals, SignalsFixture)

	BOOST_AUTO_TEST_CASE(SimpleTest)
	{
		rbx::signal<void(int)> sig1;

		//char m[1111];
		//sprintf(m, "%d %d", rbx::signal<void(int)>::sizeof_slot(), rbx::signal<void(int, std::string, bool)>::sizeof_slot());

		int result = 0;
		rbx::signals::connection connection = sig1.connect(boost::bind(&add, _1, boost::ref(result)));
		sig1(12);
		BOOST_CHECK_EQUAL(12, result);

		connection.disconnect();

		result = 0;
		sig1(12);
		BOOST_CHECK_EQUAL(0, result);
	}

	BOOST_AUTO_TEST_CASE(EmptyTest)
	{
		rbx::signal<void(int)> sig1;
		BOOST_CHECK(sig1.empty());

		int result = 0;
		rbx::signals::connection connection = sig1.connect(boost::bind(&add, _1, boost::ref(result)));
		BOOST_CHECK(!sig1.empty());

		rbx::signals::connection connection2 = sig1.connect(boost::bind(&add, _1, boost::ref(result)));
		BOOST_CHECK(!sig1.empty());

		connection.disconnect();
		BOOST_CHECK(!sig1.empty());

		connection2.disconnect();
		BOOST_CHECK(sig1.empty());
	}

	BOOST_AUTO_TEST_CASE(SimpleTest2)
	{
			rbx::signal<void(bool, int)> sig1;
			int result = 0;
			rbx::signals::connection connection = sig1.connect(boost::bind(&add, _2, boost::ref(result)));
			sig1(true, 12);
			BOOST_CHECK_EQUAL(12, result);

			connection.disconnect();

			result = 0;
			sig1(false, 12);
			BOOST_CHECK_EQUAL(0, result);
	}

	BOOST_AUTO_TEST_CASE(SimpleTest3)
	{
		rbx::signal<void(bool, const char*, int)> sig1;
		int result = 0;
		rbx::signals::connection connection = sig1.connect(boost::bind(&add, _3, boost::ref(result)));
		sig1(false, "", 12);
		BOOST_CHECK_EQUAL(12, result);

		connection.disconnect();

		result = 0;
		sig1(false, "", 12);
		BOOST_CHECK_EQUAL(0, result);
	}

	BOOST_AUTO_TEST_CASE(SimpleTest4)
	{
		rbx::signal<void(bool, const char*, Foo, int)> sig1;
		int result = 0;
		rbx::signals::connection connection = sig1.connect(boost::bind(&add, _4, boost::ref(result)));
		sig1(false, "", Foo(), 12);
		BOOST_CHECK_EQUAL(12, result);

		connection.disconnect();

		result = 0;
		sig1(false, "", Foo(), 12);
		BOOST_CHECK_EQUAL(0, result);
	}


	/* A simplified version of PartInstance::TouchedSignal */
	class TouchedSignal : public rbx::signal<void()>
	{
		private:
			typedef rbx::signal<void()> Super;

		class TouchedSlot
		{
			boost::function<void()> inner;
		public:
			TouchedSlot(const boost::function<void()>& inner):inner(inner)
			{
			}
			TouchedSlot(const TouchedSlot& other) : inner(other.inner)
			{
			}
			
			TouchedSlot& operator=(const TouchedSlot& other);
			~TouchedSlot()
			{
				// Try to create a deadlock
				rbx::signal<void()> sig;				
				rbx::signals::scoped_connection connection(sig.connect(boost::bind(emptyFunction)));
				sig();
			}
			void operator()() 
			{ 
				if (inner) 
					inner(); 
			}
		};

	public:
		template<typename F>
		rbx::signals::connection connect(F slot)
		{
			return Super::connect(TouchedSlot(slot));
		}
		static void disc(rbx::signals::connection* c)
		{
			c->disconnect();
		}
	};


	BOOST_AUTO_TEST_CASE(Deadlock)
	{
		// This code will deadlock if ~TouchedSignal is called within a signal mutex.
		// It happened when I tried to optimize next() by passig the shared_ptr<slot> by
		// reference. This caused the slot to be collected within the mutex, which has side
		// effects. We must avoid any side effects within the mutex to avoid deadlocks and
		// also to avoid thread starvation if the mutex is a spin mutex.
		TouchedSignal* tester = new TouchedSignal();
		rbx::signals::connection connection;
		connection = tester->connect(boost::bind(TouchedSignal::disc, &connection));
		(*tester)();
		connection.disconnect();
		delete tester;
	}
	
	BOOST_AUTO_TEST_CASE(ExceptionHandling)
	{
		int exceptionCount = 0;
		rbx::signals::slot_exception_handler = boost::bind(add, 1, boost::ref(exceptionCount));

		rbx::signal<void(int)> sig1;
		rbx::signals::connection connection = sig1.connect(boost::bind(throw_runtime_error));

		int result = 0;
		sig1.connect(boost::bind(&add, _1, boost::ref(result)));
		sig1(12);
		BOOST_CHECK_EQUAL(12, result);
		BOOST_CHECK_EQUAL(1, exceptionCount);

		connection.disconnect();

		result = 0;
		sig1(12);
		BOOST_CHECK_EQUAL(12, result);
	}
	
	BOOST_AUTO_TEST_CASE(EmptySlot)
	{
		rbx::signal<void(int)> sig1;
		int result = 0;
		rbx::signals::connection connection = sig1.connect(boost::bind(emptyFunction2, _1));
		sig1(12);
		BOOST_CHECK_EQUAL(0, result);

		connection.disconnect();

		result = 0;
		sig1(12);
		BOOST_CHECK_EQUAL(0, result);
	}

	BOOST_AUTO_TEST_CASE(RemoveWhileFiring)
	{
		rbx::signal<void(int)> sig1;
		int result = 0;

		rbx::signals::connection connectionToRemove;

		// add a dummy slot
		sig1.connect(boost::bind(&add, _1, boost::ref(result)));

		rbx::signals::connection connectionThatRemoves = sig1.connect(boost::bind(&removeConnection, _1, boost::ref(result), boost::ref(connectionToRemove)));

		// add another slot
		connectionToRemove = sig1.connect(boost::bind(&add, _1, boost::ref(result)));

		BOOST_CHECK(connectionToRemove.connected());
		result = 0;
		sig1(1);
		BOOST_CHECK(!connectionToRemove.connected());
		BOOST_CHECK_EQUAL(3, result);
		
		// Now call them again, expecting to lose one slot call
		result = 0;
		sig1(1);
		BOOST_CHECK_EQUAL(2, result);

		// Now cause the slot to remove its own connection:
		connectionToRemove = connectionThatRemoves;

		BOOST_CHECK(connectionToRemove.connected());
		result = 0;
		sig1(1);
		BOOST_CHECK(!connectionToRemove.connected());
		BOOST_CHECK_EQUAL(2, result);

		result = 0;
		sig1(1);
		BOOST_CHECK_EQUAL(1, result);
	}
	
	BOOST_AUTO_TEST_CASE(AddWhileFiring)
	{
		rbx::signal<void(int)> sig1;
		int result = 0;

		// add a dummy slot
		sig1.connect(boost::bind(&add, _1, boost::ref(result)));

		rbx::signals::connection connection = sig1.connect(boost::bind(&addConnection, _1, boost::ref(result), boost::ref(sig1)));

		result = 0;
		sig1(1);
		BOOST_CHECK_EQUAL(2, result);
		result = 0;
		sig1(1);
		BOOST_CHECK_EQUAL(3, result);
		
		connection.disconnect();

		result = 0;
		sig1(1);
		BOOST_CHECK_EQUAL(3, result);
	}

	BOOST_AUTO_TEST_CASE(FireOrder)
	{
		// This may not be guaranteed in the future, but slots are added to the head of the chain.
		// The reason for having this test is to warn that there may be breaking changes.
		rbx::signal<void(std::string*)> sig1;
		std::string result;
		sig1.connect(boost::bind(&append, _1, "World"));
		sig1.connect(boost::bind(&append, _1, "Hello"));
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "HelloWorld");
	}

	BOOST_AUTO_TEST_CASE(DoubleDisconnect)
	{
		rbx::signal<void(std::string*)> sig1;
		std::string result;
		rbx::signals::connection c1 = sig1.connect(boost::bind(&append, _1, "Hi"));
		rbx::signals::connection c2 = c1;

		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Hi");

		c1.disconnect();

		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Hi");

		c2.disconnect();

		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Hi");
	}

	BOOST_AUTO_TEST_CASE(DisconnectWhileFiring)
	{
		rbx::signal<void(rbx::signals::connection*)> sig1;
		rbx::signals::connection connection = sig1.connect(boost::bind(&rbx::signals::connection::disconnect, _1));
		BOOST_CHECK(connection.connected());
		sig1(&connection);
		BOOST_CHECK(!connection.connected());
	}

	BOOST_AUTO_TEST_CASE(AssignmentOperator)
	{
		rbx::signal<void(std::string*)> sig1;
		std::string result;

		{
			rbx::signals::scoped_connection connection(sig1.connect(boost::bind(&append, _1, "[Not Me!]")));
			connection = sig1.connect(boost::bind(&append, _1, "Hello"));
			sig1(&result);
			BOOST_CHECK_EQUAL(result, "Hello");
		}

		result.clear();
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "");
	}

	static void doubleDisconnect(rbx::signals::connection& connection, std::string* s)
	{
		*s += "Hi";
		connection.disconnect();
		connection.disconnect();
	}

	static void dont_call_me()
	{
		BOOST_CHECK(false);
	}

	static void remove_connections(rbx::signals::connection* c1, rbx::signals::connection* c2, rbx::signals::connection* c3)
	{
		c3->disconnect();
		c2->disconnect();
		c1->disconnect();
	}

	BOOST_AUTO_TEST_CASE(MultiDisconnect)
	{
		// Tests CS1502 - a bug that Simon found
		rbx::signal<void()> sig1;
		rbx::signals::connection c1 = sig1.connect(boost::bind(&dont_call_me));
		rbx::signals::connection c2 = sig1.connect(boost::bind(&dont_call_me));
		rbx::signals::connection c3;
		c3 = sig1.connect(boost::bind(remove_connections, &c1, &c2, &c3));
		sig1();
	}

	BOOST_AUTO_TEST_CASE(ReentrantDoubleDisconnect)
	{
		rbx::signal<void(std::string*)> sig1;
		std::string result;

		rbx::signals::connection connection = sig1.connect(boost::bind(doubleDisconnect, boost::ref(connection), &result));
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Hi");
	}

	static void reset(rbx::signal<void(std::string*)>& sig1, rbx::signals::scoped_connection& connection, std::string message, std::string* s)
	{
		*s += message;
		connection = sig1.connect(boost::bind(&reset, boost::ref(sig1), boost::ref(connection), message + "#", _1));
	}

	BOOST_AUTO_TEST_CASE(ReentrantResetScopedConnection)
	{
		rbx::signal<void(std::string*)> sig1;
		std::string result;

		rbx::signals::scoped_connection connection;
		reset(sig1, connection, "Reset", &result);
		BOOST_CHECK_EQUAL(result, "Reset");
		
		result.clear();
		sig1.connect(boost::bind(&append, _1, "Hello"));
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "HelloReset#");

		result.clear();
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Reset##Hello");

		result.clear();
		sig1(&result);
		BOOST_CHECK_EQUAL(result, "Reset###Hello");
	}

	BOOST_AUTO_TEST_CASE(SlotChain)
	{
		// Make sure a huge chain of slots doesn't cause a stack crash on destruction
		// See DE131
		rbx::signal<void()> signal;
		for (int i = 0; i < 1000; ++i)
			signal.connect(boost::function<void()>());
		BOOST_CHECK(true);
	}

	BOOST_AUTO_TEST_CASE(ReverseDestruction)
	{
		// Make sure it is safe to disconnect a connection after the slot has been destroyed
		rbx::signals::connection connection;
		{
			rbx::signal<void()> signal;
			connection = signal.connect(boost::function<void()>());
		}
		connection.disconnect();
		BOOST_CHECK(true);
	}

#ifndef _DEBUG
	BOOST_AUTO_TEST_CASE(PerfSignal)
	{
		rbx::signal<void(std::string, int)> sig1;
		int result = 0;
		rbx::signals::scoped_connection connection[10];
		for (int i = 0; i < 5; ++i)
			connection[i] = sig1.connect(boost::bind(&add, _2, boost::ref(result)));
		
		for (int i = 0; i < 1e6; ++i)
			sig1("Hello World", 12);
	}

	BOOST_AUTO_TEST_CASE(PerfConnect)
	{
		rbx::signal<void(bool, int)> sig1;
		int result = 0;
		for (int i = 0; i < 1e6; ++i)
		{
			rbx::signals::connection connection = sig1.connect(boost::bind(&add, _2, boost::ref(result)));
			connection.disconnect();
		}
	}
#endif

BOOST_AUTO_TEST_SUITE_END()