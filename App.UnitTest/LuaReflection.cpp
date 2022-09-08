#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/test/Base.UnitTest.Lib.h"
#include "rbx/test/ScopedFastFlagSetting.h"

#include "script/ScriptContext.h"
#include "script/Script.h"
#include "util/ProtectedString.h"
#include "util/udim.h"
#include "util/axes.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Surface.h"
#include "v8datamodel/ModelInstance.h"
#include "rbx/make_shared.h"

#include "Util/CellID.h"

#include "script/LuaArguments.h"

enum TestEnum
{
	NoReverb = 0,
	GenericReverb,
	PaddedCell,
	Room,
	Bathroom,
	LivingRoom,
	StoneRoom,
	Auditorium,
	ConcertHall,
	Cave,
	Arena,
	Hangar,
};

namespace RBX
{
	template<>
	bool RBX::StringConverter<TestEnum>::convertToValue(const std::string& text, TestEnum& value)
	{
		return RBX::Reflection::EnumDesc<TestEnum>::singleton().convertToValue(text.c_str(),value);
	}

	namespace Reflection
	{
		template<>
		EnumDesc<TestEnum>::EnumDesc()
			:EnumDescriptor("TestEnum")
		{
			addPair(NoReverb, "NoReverb");
			addPair(GenericReverb, "GenericReverb");
			addPair(PaddedCell, "PaddedCell");
			addPair(Room, "Room");
			addPair(Bathroom, "Bathroom");
			addPair(LivingRoom, "LivingRoom");
			addPair(StoneRoom, "StoneRoom");
			addPair(Auditorium, "Auditorium");
			addPair(ConcertHall, "ConcertHall");
			addPair(Cave, "Cave");
			addPair(Arena, "Arena");
			addPair(Hangar, "Hangar");
		}

		template<>
		TestEnum& Variant::convert<TestEnum>(void)
		{
			return genericConvert<TestEnum>();
		}
	}}



RBX_REGISTER_ENUM(TestEnum);

using namespace RBX;

extern const char* const sLuaReflectionTest;
class LuaReflectionTest : public DescribedCreatable<LuaReflectionTest, Instance, sLuaReflectionTest>
{
	static double hypot(double x, double y) { return sqrt(x*x + y*y); }
public:
	rbx::signal<void(int)> sig;

	boost::function<double(double, double)> hypotCallback;
	int dummy(int x, int y, int z)
	{
		return 11;
	}
	std::string dummyString(std::string s)
	{
		return s;
	}
	shared_ptr<Instance> instance(shared_ptr<Instance> s)
	{
		return s;
	}
	shared_ptr<const RBX::Reflection::Tuple> tuple(int x, int y, int z)
	{
		shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(3));
		result->values[0] = x;
		result->values[1] = y;
		result->values[2] = z;
		return result;
	}

	shared_ptr<const RBX::Reflection::Tuple> tupleTest(shared_ptr<const RBX::Reflection::Tuple> inout)
	{
		// We could just pass inout as the return value. However, that is unrealistic.
		// In most cases we'd return a new tuple
#if 0
		// TODO: Why won't this build on Mac?
		return rbx::make_shared<const RBX::Reflection::Tuple>(inout);
#else
		shared_ptr<RBX::Reflection::Tuple> result = rbx::make_shared<RBX::Reflection::Tuple>(inout->values.size());
		for (size_t i=0; i<result->values.size(); ++i)
			result->values[i] = inout->values[i];
		return result;
#endif
	}

	shared_ptr<const RBX::Reflection::Tuple> enumtuple(int x, int y, int z)
	{
		shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(3));
		result->values[0] = RBX::PartInstance::SYMETRIC;
		result->values[1] = RBX::PartInstance::SYMETRIC;
		result->values[2] = RBX::PartInstance::PLATE;
		return result;
	}
	void voidCall()
	{
	}
	TestEnum dummyEnum(TestEnum x, TestEnum y, TestEnum z)
	{
		return Cave;
	}
	double callHypot(double x, double y)
	{
		return hypotCallback(x, y);
	}
	LuaReflectionTest()
	{
		setName("LuaReflectionTest");
		hypotCallback = hypot;
	}

	void yieldInt1(int arg1, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		resumeFunction(2 * arg1);
	}

	void yieldVoid1(int arg1, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		resumeFunction();
	}

	void asyncDivide(double a, double b, boost::function<void(double)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (b != 0.0)
			DataModel::get(this)->submitTask(boost::bind(resumeFunction, a/b), DataModelJob::Write);
		else
			DataModel::get(this)->submitTask(boost::bind(errorFunction, "failure"), DataModelJob::Write);
	}

	void asyncImmediateDivide(double a, double b, boost::function<void(double)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
#if 1
		if (b != 0.0)
			resumeFunction(a/b);
		else
			errorFunction("failure");
#else
		if (b == 0.0)
			throw std::runtime_error("failure");
		resumeFunction(a/b);
#endif
	}

	Reflection::Variant variant(Reflection::Variant value)
	{
		return value;
	}

	int add(lua_State* L)
	{
		// skip "this"
		Lua::LuaArguments args(L, 1);

		double x;
		if (!args.getDouble(1, x))
			throw std::runtime_error("Argument 1 missing or nil");

		double y;
		if (!args.getDouble(2, y))
			throw std::runtime_error("Argument 2 missing or nil");

		lua_pushnumber(L, x + y);

		return 1;
	}
};


const char* const sLuaReflectionTest = "LuaReflectionTest";
static Reflection::BoundCallbackDesc<double(double, double)> hypotCallback("HypotCallback", &LuaReflectionTest::hypotCallback, "x", "y");
static Reflection::BoundFuncDesc<LuaReflectionTest, double(double, double)> callHypot(&LuaReflectionTest::callHypot, "CallHypot", "x", "y", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, int(int, int, int)> callDummy(&LuaReflectionTest::dummy, "Dummy", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, shared_ptr<const RBX::Reflection::Tuple>(shared_ptr<const RBX::Reflection::Tuple>)> func_TupleTest(&LuaReflectionTest::tupleTest, "TupleTest", "inout", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, shared_ptr<const RBX::Reflection::Tuple>(int, int, int)> callTuple(&LuaReflectionTest::tuple, "Tuple", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, shared_ptr<const RBX::Reflection::Tuple>(int, int, int)> callEnumTuple(&LuaReflectionTest::enumtuple, "EnumTuple", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, void()> callVoid(&LuaReflectionTest::voidCall, "Void", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, std::string(std::string)> callString(&LuaReflectionTest::dummyString, "String", "s", "HelloWorld", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, shared_ptr<Instance>(shared_ptr<Instance>)> callInstance(&LuaReflectionTest::instance, "Instance", "instance", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, TestEnum(TestEnum, TestEnum, TestEnum)> callDummyEnum(&LuaReflectionTest::dummyEnum, "EnumDummy", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<LuaReflectionTest, Reflection::Variant(Reflection::Variant)> desc_Variant(&LuaReflectionTest::variant, "Variant", "value", Security::None);

static Reflection::EventDesc<LuaReflectionTest, void(int)> event_Sig(&LuaReflectionTest::sig, "Sig", "arg");

static Reflection::BoundYieldFuncDesc<LuaReflectionTest, void(int)>	func_YieldVoid1(&LuaReflectionTest::yieldVoid1, "YieldVoid1", "arg1", Security::None);
static Reflection::BoundYieldFuncDesc<LuaReflectionTest, int(int)>	func_YieldInt1(&LuaReflectionTest::yieldInt1, "YieldInt1", "arg1", Security::None);

static Reflection::BoundYieldFuncDesc<LuaReflectionTest, double(double, double)>	func_AsyncSuccess(&LuaReflectionTest::asyncDivide, "AsyncDivide", "a", "b", Security::None);
static Reflection::BoundYieldFuncDesc<LuaReflectionTest, double(double, double)>	func_AsyncImmediateSuccess(&LuaReflectionTest::asyncImmediateDivide, "AsyncImmediateDivide", "a", "b", Security::None);

static Reflection::CustomBoundFuncDesc<LuaReflectionTest, double(double, double)> func_Add(&LuaReflectionTest::add, "Add", "x", "y", Security::None);

RBX_REGISTER_CLASS(LuaReflectionTest);

BOOST_AUTO_TEST_SUITE(LuaReflection)

BOOST_FIXTURE_TEST_CASE(Inst, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest', workspace) \n"
		"return l:Instance(l) "
		);

	BOOST_REQUIRE_EQUAL(result->values.size(), 1);
	BOOST_CHECK_NE(result->at(0).convert< shared_ptr<Instance> >(), shared_ptr<Instance>());
}

BOOST_FIXTURE_TEST_CASE(ypcallSuccess, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 100; ++i)
	{
		std::auto_ptr<RBX::Reflection::Tuple> result = execute
			(
			"return ypcall(function(a, b) return a, b, a .. b end, 'a', 'b') "
			);
		BOOST_REQUIRE_EQUAL(result->values.size(), 4);
		BOOST_CHECK_EQUAL(result->values.at(0).get<bool>(), true);
		BOOST_CHECK_EQUAL(result->values.at(1).get<std::string>(), "a");
		BOOST_CHECK_EQUAL(result->values.at(2).get<std::string>(), "b");
		BOOST_CHECK_EQUAL(result->values.at(3).get<std::string>(), "ab");
	}
};

BOOST_FIXTURE_TEST_CASE(ypcallFailure, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 100; ++i)
	{
		std::auto_ptr<RBX::Reflection::Tuple> result = execute
			(
			"return ypcall(function() error('failure') end) "
			);
		BOOST_REQUIRE_EQUAL(result->values.size(), 2);
		BOOST_CHECK_EQUAL(result->values.at(0).get<bool>(), false);
		BOOST_CHECK(result->values.at(1).get<std::string>().find("failure") != std::string::npos);
	}
};

static shared_ptr<RBX::Reflection::Tuple> check_pcall_aux(RBX::Reflection::Tuple* args, double a, double b, shared_ptr<RBX::CEvent> event)
{
	BOOST_REQUIRE_GE(args->values.size(), (size_t) 1);
	BOOST_CHECK_EQUAL(args->at(0).get<bool>(), b != 0.0);
	BOOST_REQUIRE_EQUAL(args->values.size(), (size_t) 2);
	if (b == 0.0)
	{
		std::string message = args->at(1).get<std::string>();
		BOOST_CHECK(message.find("failure") != std::string::npos);
	}
	else
	{
		double result = args->at(1).get<double>();
		BOOST_CHECK_EQUAL(result, a/b);
	}

	if (event)
		event->Set();
	return shared_ptr<RBX::Reflection::Tuple>();
};

static shared_ptr<RBX::Reflection::Tuple> check_pcall(shared_ptr<const RBX::Reflection::Tuple> args, double a, double b, shared_ptr<RBX::CEvent> event)
{
	BOOST_REQUIRE_GE(args->values.size(), (size_t) 1);
	BOOST_CHECK_EQUAL(args->at(0).get<bool>(), b != 0.0);
	BOOST_REQUIRE_EQUAL(args->values.size(), (size_t) 2);
	if (b == 0.0)
	{
		std::string message = args->at(1).get<std::string>();
		BOOST_CHECK(message.find("failure") != std::string::npos);
	}
	else
	{
		double result = args->at(1).get<double>();
		BOOST_CHECK_EQUAL(result, a/b);
	}

	if (event)
		event->Set();
	return shared_ptr<RBX::Reflection::Tuple>();
};

static shared_ptr<RBX::Reflection::Tuple> check_old_style_failure(shared_ptr<const RBX::Reflection::Tuple> args, shared_ptr<RBX::CEvent> event)
{
	BOOST_REQUIRE_EQUAL(args->values.size(), (size_t) 1);
	std::string message = args->at(0).get<std::string>();
	BOOST_CHECK(message.find("failure") != std::string::npos);

	if (event)
		event->Set();
	return shared_ptr<RBX::Reflection::Tuple>();
};

BOOST_FIXTURE_TEST_CASE(ypcallAsyncImmediateSuccess, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = Workspace \n"
		" return ypcall(function() l:AsyncImmediateDivide(2, 3) return l:AsyncImmediateDivide(1, 2) end) "
		);
	check_pcall_aux(result.get(), 1, 2, shared_ptr<CEvent>());
};

BOOST_FIXTURE_TEST_CASE(ypcallAsyncImmediateFailure, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = Workspace \n"
		"return ypcall(function() l:AsyncImmediateDivide(4, 4) return l:AsyncImmediateDivide(2, 0) end) "
		);
	check_pcall_aux(result.get(), 2, 0, shared_ptr<CEvent>());
};

BOOST_FIXTURE_TEST_CASE(ypcallAsyncSuccess, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 100; ++i)
	{
		shared_ptr<RBX::CEvent> event(rbx::make_shared<CEvent>(true));
		RBX::Reflection::Tuple args(1);
		args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&check_pcall, _1, 1, 2, event)));
		execute
			(
			"callback = ... \n"
			"callback(ypcall(function() l:AsyncDivide(2, 3) return l:AsyncDivide(1, 2) end)) ",
			args
			);
		BOOST_REQUIRE(event->Wait(3000));
	}
};


BOOST_FIXTURE_TEST_CASE(ypcallAsyncSuccessNested, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 100; ++i)
	{
		shared_ptr<RBX::CEvent> event(rbx::make_shared<CEvent>(true));
		RBX::Reflection::Tuple args(1);
		args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&check_pcall, _1, 1, 2, event)));
		execute
			(
			"callback = ...\n"
			"callback(ypcall(function()\n"
			"s, two = ypcall(function() wait(0.01) return l:AsyncDivide(2, 1) end)\n"
			"return l:AsyncDivide(1, two)\n"
			"end)) ",
			args
			);
		BOOST_REQUIRE(event->Wait(3000));
	}
};

BOOST_FIXTURE_TEST_CASE(ypcallBindableFunction, DataModelFixture)
{
	execute("f = Instance.new('BindableFunction') f.Parent = Workspace");
	for (int i = 0; i < 10; ++i)
	{
		shared_ptr<RBX::CEvent> event(rbx::make_shared<CEvent>(true));
		RBX::Reflection::Tuple args(1);
		args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&check_pcall, _1, 1, 2, event)));
		execute(
			"callback = ...\n"
			"Delay(0.3, function() f.OnInvoke = function() end end)\n"
			"callback(ypcall(function (a , b)\n"
			"   f:Invoke()\n"
			"   f.OnInvoke = nil\n"
			"   return a/b\n"
			"end, 1, 2))\n",
			args
			);
		BOOST_CHECK(event->Wait(5000));
	}
};

BOOST_FIXTURE_TEST_CASE(ypcallAsyncFailure, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 100; ++i)
	{
		shared_ptr<RBX::CEvent> event(rbx::make_shared<CEvent>(true));
		RBX::Reflection::Tuple args(1);
		args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&check_pcall, _1, 2, 0, event)));
		execute
			(
			"callback = ... \n"
			"callback(ypcall(function() l:AsyncDivide(2, 3) return l:AsyncDivide(2, 0) end)) ",
			args
			);
		BOOST_REQUIRE(event->Wait(3000));
	}
};

BOOST_FIXTURE_TEST_CASE(AsyncFailure, DataModelFixture)
{
	shared_ptr<RBX::CEvent> event(rbx::make_shared<CEvent>(true));
	RBX::Reflection::Tuple args(1);
	args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&check_old_style_failure, _1, event)));
	execute
		(
		"callback = ... \n"
		"l = Instance.new('LuaReflectionTest', Workspace) " \
		"callback(l:AsyncDivide(2, 0)) ",
		args
		);

    BOOST_CHECK(!event->Wait(500)); // Make sure thread execution is halted because of failure
};

static shared_ptr<RBX::Reflection::Tuple> yieldResults(shared_ptr<const RBX::Reflection::Tuple> args, RBX::CEvent* event)
{
	BOOST_REQUIRE_EQUAL(args->values.size(),2);
	BOOST_CHECK_EQUAL(args->at(1).get<int>(), 6);
	BOOST_CHECK_EQUAL(args->at(0).type(), Reflection::Type::singleton<void>());
	event->Set();
	return shared_ptr<RBX::Reflection::Tuple>();
};

BOOST_FIXTURE_TEST_CASE(YieldFunctions, DataModelFixture)
{
	RBX::CEvent event(true);
	RBX::Reflection::Tuple args(1);
	args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&yieldResults, _1, &event)));
	execute
		(
		"result = ... \n"
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = Workspace \n"
		"a = l:YieldVoid1(7)\n"
		"b = l:YieldInt1(3) \n"
		"result(a, b)",
		args
		);
	event.Wait();
}



static shared_ptr<RBX::Reflection::Tuple> yieldResults2(shared_ptr<const RBX::Reflection::Tuple> args, RBX::CEvent* event)
{
	BOOST_REQUIRE_EQUAL(args->values.size(), 1);
	BOOST_CHECK_EQUAL(args->at(0).type(), Reflection::Type::singleton<void>());
	event->Set();
	return shared_ptr<RBX::Reflection::Tuple>();
};

BOOST_FIXTURE_TEST_CASE(YieldFunctions2, DataModelFixture)
{
	RBX::CEvent event(true);
	RBX::Reflection::Tuple args(1);
	args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&yieldResults2, _1, &event)));
	execute
		(
		"result = ... \n"
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = Workspace \n"
		"a = l:YieldVoid1(7)\n"
		"result(a)",
		args
		);
	event.Wait();
}




#ifndef _DEBUG
BOOST_FIXTURE_TEST_CASE(PerfAsyncImmediateSuccess, DataModelFixture)
{
	execute("l = Instance.new('LuaReflectionTest') l.Parent = Workspace");
	for (int i = 0; i < 4000; ++i)
	{
		std::auto_ptr<RBX::Reflection::Tuple> result = execute("return true, l:AsyncImmediateDivide(1, 2)");
		check_pcall_aux(result.get(), 1, 2, shared_ptr<CEvent>());
	}
};

BOOST_FIXTURE_TEST_CASE(Perf_pcall, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"for i = 1, 1000000 do \n"
		" _, j = pcall(function(x) hello = 'hello' return x end, i) \n"
		"end \n"
		"return j, hello "
		);
	BOOST_REQUIRE_EQUAL(result->values.size(), 2);
	BOOST_CHECK_EQUAL(result->values.at(0).convert<int>(), 1000000);
	BOOST_CHECK_EQUAL(result->values.at(1).convert<std::string>(), "hello");
};

// ypcall is slower than pcall.
BOOST_FIXTURE_TEST_CASE(Perf_ypcall, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"for i = 1, 1000000 do \n"
		" _, j = ypcall(function(x) hello = 'hello' return x end, i) \n"
		"end \n"
		"return j, hello "
		);
	BOOST_REQUIRE_EQUAL(result->values.size(), 2);
	BOOST_CHECK_EQUAL(result->values.at(0).convert<int>(), 1000000);
	BOOST_CHECK_EQUAL(result->values.at(1).convert<std::string>(), "hello");
};
#endif

BOOST_FIXTURE_TEST_CASE(TupleTest, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result;

	try
	{
		result = execute("return Instance.new('LuaReflectionTest'):TupleTest('a','b','c',1,2,3)");
	}
	catch (std::exception&)
	{
		BOOST_WARN_MESSAGE(false, "Tuples not properly supported");
		BOOST_CHECK(true);	// dummy test
		return;
	}

	BOOST_REQUIRE_EQUAL(result->values.size(), 6);
	BOOST_CHECK_EQUAL(result->values.at(0).convert<std::string>(),"a");
	BOOST_CHECK_EQUAL(result->values.at(1).convert<std::string>(),"b");
	BOOST_CHECK_EQUAL(result->values.at(2).convert<std::string>(),"c");
	BOOST_CHECK_EQUAL(result->values.at(3).convert<int>(),1);
	BOOST_CHECK_EQUAL(result->values.at(4).convert<int>(),2);
	BOOST_CHECK_EQUAL(result->values.at(5).convert<int>(),3);
}

BOOST_FIXTURE_TEST_CASE(EmptyDataModel, DataModelFixture)
{
	DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
	run();
}

BOOST_FIXTURE_TEST_CASE(Callbacks, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"a = l:CallHypot(3, 4) \n"
		"l.HypotCallback = function(x,y) return x*x + y*y end \n"
		"b = l:CallHypot(3, 4) \n"
		"return a, b"
		);

	BOOST_CHECK_CLOSE(result->at(0).convert<double>(), 5.0, 0.001);
	BOOST_CHECK_CLOSE(result->at(1).convert<double>(), 25.0, 0.001);

	result = execute
		(
		"l = workspace.LuaReflectionTest \n"
		"b = l:CallHypot(3, 4) \n"
		"return b"
		);

	BOOST_CHECK_CLOSE(result->at(0).convert<double>(), 25.0, 0.001);
}

static int eventValue = 0;
static shared_ptr<Reflection::Tuple> check_Event(shared_ptr<const Reflection::Tuple> args)
{
	eventValue = args->at(0).get<int>();
	return shared_ptr<Reflection::Tuple>();
}

BOOST_FIXTURE_TEST_CASE(Events, DataModelFixture)
{
	RBX::Reflection::Tuple args(1);
	args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(&check_Event));

	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest', workspace)\n"
		"l.Sig:connect(...)\n"
		"return l\n",
		args
		);

	shared_ptr<LuaReflectionTest> i = shared_dynamic_cast<LuaReflectionTest>(result->at(0).convert< shared_ptr<Instance> >());

	DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
	i->sig(1);

	BOOST_CHECK_EQUAL(eventValue, 1);

}

static int event2Value = 0;
static double check_Event2(double x, double)
{
	event2Value = x;
	return 0;
}

BOOST_FIXTURE_TEST_CASE(Events2, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute("return Instance.new('LuaReflectionTest', workspace)\n");

	DataModel::LegacyLock lock(dataModel, DataModelJob::Write);

	run();

	shared_ptr<LuaReflectionTest> i = Creatable<Instance>::create<LuaReflectionTest>();
	i->hypotCallback = check_Event2;
	i->setName("FooBar");
	i->setParent(dataModel->getWorkspace());

	shared_ptr<RBX::Script> script = Creatable<Instance>::create<Script>();
	script->setEmbeddedCode(ProtectedString::fromTestSource(
		"l = workspace.FooBar\n"
		"l:CallHypot(2, 0)\n"
		"l.Sig:connect(function(x)\n"
		"   l:CallHypot(x, 0)\n"
		"   error('ha!')\n"
		"end)\n"
		));
#if 0
	execute(script->getEmbeddedCode().get().c_str());
#else
	script->setParent(dataModel->getWorkspace());
#endif

	BOOST_CHECK_EQUAL(event2Value, 2);

	i->sig(1);
	BOOST_CHECK_EQUAL(event2Value, 1);

	i->sig(8);
    BOOST_CHECK_EQUAL(event2Value, 8);

	script->setParent(NULL);
	
	i->sig(3);
	BOOST_CHECK_NE(event2Value, 3);

}



static void checkForMissingType(const RBX::Reflection::Type* type, const RBX::Reflection::ValueArray& args)
{
	using namespace RBX;
	using namespace Reflection;

	if (type->isType<void>())
		return;

	if (type->isType<shared_ptr<const Tuple> >()) // This is being tested because it contains the others in the test
		return;

	if (type->isType<RBX::SystemAddress>()) // Not scriptable
		return;

	if (type->isType<RBX::Surface>()) // Not scriptable
		return;

	if (type->name.str == "Variant")  // everything being tested is turned into a Variant, so we can't test this
		return;

	if (type->name.str == "Object")
		return;

	if (dynamic_cast<const EnumDescriptor*>(type))
		return;

	if (type->isType<Lua::WeakFunctionRef>())
		return;

	for (RBX::Reflection::ValueArray::const_iterator iter = args.begin(); iter != args.end(); ++iter)
	{
		if (&iter->type() == type)
			return;
	}
	std::stringstream str;
	str << "Missing Type: " << *type;
	BOOST_WARN_MESSAGE(false, str.str());
}

static void checkForMissingTypes(const RBX::Reflection::ValueArray& args)
{
	std::for_each(RBX::Reflection::Type::getAllTypes().begin(), RBX::Reflection::Type::getAllTypes().end(), boost::bind(&checkForMissingType, _1, args));
}

static RBX::Reflection::ValueArray createAllArgs()
{
	using namespace RBX;
	using namespace Reflection;

	ValueArray args(32);

	args[0] = (std::string) "Hello world!";
	args[1] = true;
	args[2] = (int) 2;
	args[3] = (double) 2;
	args[4] = (float) 2;
	args[5] = (long) 2;
	args[6] = RBX::ProtectedString::fromTestSource("Hi");
	shared_ptr<Instance> instance = Creatable<Instance>::create<LuaReflectionTest>();
	args[7] = instance;
	shared_ptr<Instances> instances = rbx::make_shared<Instances>();
	instances->push_back(instance);
	args[8] = shared_ptr<const Instances>(instances);
	Reflection::ValueArray array;
	array.push_back(1);
	args[9] = shared_ptr<const Reflection::ValueArray>(new ValueArray(array));
	args[10] = shared_ptr<Lua::GenericFunction>();
	args[11] = shared_ptr<Lua::GenericAsyncFunction>();
	args[12] = G3D::Vector3int16();
	args[13] = G3D::Vector3();
	args[14] = RBX::Vector2();
	args[15] = RBX::RbxRay();

	args[16] = G3D::CoordinateFrame();
	args[17] = G3D::Color3();
	args[18] = BrickColor();
	args[19] = RBX::Region3();
	args[20] = RBX::Region3int16();
	args[21] = RBX::UDim();
	args[22] = RBX::UDim2();
	args[23] = Faces();
	args[24] = RBX::Axes();
	args[25] = ContentId("Hi");
	args[26] = (const Reflection::PropertyDescriptor*)&Instance::desc_Name;
	args[27] = G3D::Vector2int16();

	ValueMap map;
	map["hello"] = 1;
	args[28] = shared_ptr<const ValueMap>(new ValueMap(map));
	ValueTable table;
	table["hi"] = 34;
	args[29] = shared_ptr<const ValueTable>(new ValueTable(table));
	args[30] = RBX::CellID(false, Vector3(1,2,3), shared_ptr<Instance>());
    args[31] = G3D::Rect2D();
    
	static bool firstTime = true;
	if (firstTime)
		checkForMissingTypes(args);
	firstTime = false;
	return args;
}

static void mapAllArgs(RBX::Reflection::ValueArray& args)
{
	// map those that Lua maps:
	args[2] = (double) 2;
	args[3] = (double) 2;
	args[4] = (double) 2;
	args[5] = (double) 2;
	args[6] = std::string("Hi");
	args[8] = shared_ptr<const Reflection::ValueArray>();
	args[10] = RBX::Lua::WeakFunctionRef();
	args[11] = RBX::Lua::WeakFunctionRef();

	args[25] = std::string("Hi");
	args[26] = std::string(Instance::desc_Name.name.c_str());

	args[28] = args[29];
}

static bool testArgs(DataModelFixture& fixture, size_t i, size_t mx)
{
    // When we destroy Thread objects we need the legacy lock since dtor works with Lua stack
   	DataModel::LegacyLock lock(fixture.dataModel.get(), DataModelJob::Write);

	RBX::Reflection::ValueArray args = createAllArgs();
	if (i >= args.size())
		return false;

	mx = std::min(args.size(), mx);

	RBX::Reflection::Tuple tuple(mx - i);
	std::copy(args.begin() + i, args.begin() + mx, tuple.values.begin());

	std::auto_ptr<RBX::Reflection::Tuple> result = fixture.execute("return ...", tuple);

	mapAllArgs(args);
	std::copy(args.begin() + i, args.begin() + mx, tuple.values.begin());

	BOOST_REQUIRE_EQUAL(tuple.values.size(), result->values.size());

	for (size_t i = 0; i < tuple.values.size(); ++i)
		BOOST_CHECK_EQUAL(result->values[i].type(), tuple.values[i].type());

	// TODO: It would be create to check value equality. Some day variant should support ==

	return true;
}

BOOST_FIXTURE_TEST_CASE(ArgTypes, DataModelFixture)
{
#if 1
	for (size_t i = 0; ; i += 10)
		if (!testArgs(*this, i, i + 10))
			return;
#else
	testArgs(*this, 12, 13);
#endif
}

BOOST_FIXTURE_TEST_CASE(ArgVariant, DataModelFixture)
{
#if 1
	std::auto_ptr<RBX::Reflection::Tuple> result = execute("t = Instance.new('LuaReflectionTest') return t:Variant('hello')");

	BOOST_REQUIRE_EQUAL(result->values.size(), 1);
	BOOST_CHECK_EQUAL(result->values.at(0).convert<std::string>(),"hello");
#else
	std::auto_ptr<RBX::Reflection::Tuple> result = execute("t = Instance.new('LuaReflectionTest') t.Value = 'world' return t:Variant('hello'), t.Value");

	BOOST_REQUIRE_EQUAL(result->values.size(), 2);
	BOOST_CHECK_EQUAL(result->values.at(0).convert<std::string>(),"hello");
	BOOST_CHECK_EQUAL(result->values.at(1).convert<std::string>(),"world");
#endif
}

BOOST_FIXTURE_TEST_CASE(CyclicTable1, DataModelFixture)
{
	bool exceptionChecked = false;

	try
	{
		execute("local t = {} t[1] = t Instance.new('BindableFunction'):Invoke(t)");
	}

	catch (std::exception& e)
	{
		BOOST_CHECK_EQUAL("tables cannot be cyclic", e.what());
		exceptionChecked = true;
	}

	BOOST_CHECK(exceptionChecked);
}

BOOST_FIXTURE_TEST_CASE(CyclicTable2, DataModelFixture)
{
	bool exceptionChecked = false;

	try
	{
		execute
			(
			"t1 = {}\n"
			"t2 = {}\n"
			"t1['t2'] = t2\n"
			"t2['t1'] = t1\n"
			"Instance.new('BindableFunction'):Invoke(t1)");
	}

	catch (std::exception& e)
	{
		BOOST_CHECK_EQUAL("tables cannot be cyclic", e.what());
		exceptionChecked = true;
	}

	BOOST_CHECK(exceptionChecked);
}

BOOST_FIXTURE_TEST_CASE(DuplicateTable, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"local bindableFunction = Instance.new('BindableFunction')\n"
		"bindableFunction.OnInvoke = function()\n"
		"  local t = {}\n"
		"  return t, t\n"
        "end\n"
		"return bindableFunction:Invoke()");

	BOOST_REQUIRE_EQUAL(result->values.size(), 2);
	BOOST_CHECK(result->values.at(0).isType<shared_ptr<const Reflection::ValueArray> >());
	BOOST_CHECK(result->values.at(1).isType<shared_ptr<const Reflection::ValueArray> >());
	// as of now we get different tables from C++ for the same table returned in Lua
	BOOST_CHECK_NE(result->values.at(0).convert<shared_ptr<const Reflection::ValueArray> >(), result->values.at(1).convert<shared_ptr<const Reflection::ValueArray> >());
}

BOOST_FIXTURE_TEST_CASE(CustomCalls, DataModelFixture)
{
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"return l:Add(1, 2)\n"
		);

	BOOST_CHECK_CLOSE(result->at(0).convert<double>(), 3.0, 0.001);
}

#ifndef _DEBUG
BOOST_FIXTURE_TEST_CASE(PerfNoCall, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"end \n"
		"return 0" 
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfVoidCall, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"   l:Void() \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfVoidCallFaster, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"local void = l.Void \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"   void(l) \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->values[0].convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfVoidCallFasterNonLocal, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"void = l.Void \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"   void(l) \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->values[0].convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfFunctionCallInt, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"   l:Dummy(x,y,z) \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfTupleCall, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 4000000 do\n"
		"   x=1 y=2 z=3 \n"
		"   l:Tuple(x,y,z) \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}
BOOST_FIXTURE_TEST_CASE(PerfEnumTupleCall, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 4000000 do \n"
		"x, y, z = l:EnumTuple(1,2,3) \n"
		"end \n"
		"return 0"
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}

BOOST_FIXTURE_TEST_CASE(PerfString, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	std::auto_ptr<RBX::Reflection::Tuple> result = execute
		(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"for i = 1, 2000000 do \n"
		"s = l:String('a') \n"
		"s = s .. l:String() \n"
		"end \n"
		"return s"
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<std::string>(), "aHelloWorld");
}

BOOST_FIXTURE_TEST_CASE(PerfFunctionCallEnum, DataModelFixture)
{
	Test::PerformanceTestFixture fixture;
	DataModel::LegacyLock lock(dataModel, DataModelJob::Write);

	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dataModel.get());

	std::auto_ptr<RBX::Reflection::Tuple> result = scriptContext->executeInNewThread
		(
		RBX::Security::GameScript_, ProtectedString::fromTestSource(
		"l = Instance.new('LuaReflectionTest') \n"
		"l.Parent = workspace \n"
		"a = Enum.TestEnum.Room b = Enum.TestEnum.Cave c = Enum.TestEnum.Bathroom \n"
		"for i = 1, 4000000 do \n"
		"   l:EnumDummy(a, b, c) \n"
		"end \n"
		"return 0"), 
		"test2", 
		RBX::Reflection::Tuple()
		);

	BOOST_CHECK_EQUAL(result->at(0).convert<int>(),0);
}

BOOST_AUTO_TEST_CASE(PerfClone)
{
	Test::PerformanceTestFixture fixture;
	shared_ptr<Instance> root = Creatable<Instance>::create<ModelInstance>();
	for (int i = 0; i<300000; ++i)
		Creatable<Instance>::create<ModelInstance>()->setParent(root.get());
	root->clone(ScriptingCreator);
}
#endif

BOOST_AUTO_TEST_SUITE_END()

