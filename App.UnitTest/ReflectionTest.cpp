#include <boost/test/unit_test.hpp>

#include "reflection/object.h"
#include "reflection/reflection.h"

using namespace RBX;

static int f0()
{
	return 12;
}

static int f1(bool b)
{
	return b ? 1 : 0;
}

static std::string f2(std::string s1, std::string s2)
{
	return s1 + s2;
}

static int f3(int i1, int i2, int i3)
{
	return i1 + i2 + i3;
}

static int f4(int i1, int i2, int i3, int i4)
{
	return i1 + i2 + i3 + i4;
}

static shared_ptr<Reflection::Tuple> g0(shared_ptr<const Reflection::Tuple> args)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	result->values.push_back(23);
	return result;
}

static shared_ptr<Reflection::Tuple> g1(shared_ptr<const Reflection::Tuple> args)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	result->values.push_back(args->values[0].get<bool>() ? 13 : 0);
	return result;
}

static shared_ptr<Reflection::Tuple> g2(shared_ptr<const Reflection::Tuple> args)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	result->values.push_back(args->values[0].get<std::string>() + args->values[1].get<std::string>());
	return result;
}

static shared_ptr<Reflection::Tuple> g3(shared_ptr<const Reflection::Tuple> args)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	result->values.push_back(args->values[0].get<int>() + args->values[1].get<int>() + args->values[2].get<int>());
	return result;
}

static shared_ptr<Reflection::Tuple> g4(shared_ptr<const Reflection::Tuple> args)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	result->values.push_back(args->values[0].get<int>() + args->values[1].get<int>() + args->values[2].get<int>() + args->values[3].get<int>());
	return result;
}

extern const char* const sReflectionTestClass;
class ReflectionTestClass : public Reflection::Described<ReflectionTestClass, sReflectionTestClass>
{
public:
	ReflectionTestClass()
	{
		p = 12;
	}
	boost::function<int()> func0;
	boost::function<int(bool)> func1;
	boost::function<std::string(std::string, std::string)> func2;
	boost::function<int(int, int, int)> func3;
	boost::function<int(int, int, int, int)> func4;

	float p;

	void raisePropertyChanged(const RBX::Reflection::PropertyDescriptor& descriptor)
	{
	}

	const Name& getClassName() const
	{
		return Name::declare("ReflectionTestClass");
	}
};
const char* const sReflectionTestClass = "ReflectionTestClass";

static Reflection::BoundCallbackDesc<int()> desc0("F0", &ReflectionTestClass::func0);
static Reflection::BoundCallbackDesc<int(bool)> desc1("F1", &ReflectionTestClass::func1, "yesno");
static Reflection::BoundCallbackDesc<std::string(std::string, std::string)> desc2("F2", &ReflectionTestClass::func2, "string1", "string2");
static Reflection::BoundCallbackDesc<int(int, int, int)> desc3("F3", &ReflectionTestClass::func3, "i1", "i2", "i3");
static Reflection::BoundCallbackDesc<int(int, int, int, int)> desc4("F4", &ReflectionTestClass::func4, "i1", "i2", "i3", "i4");



extern const char* const sReflectionTestSubclass;
class ReflectionTestSubclass : public Reflection::Described<ReflectionTestSubclass, sReflectionTestSubclass, ReflectionTestClass>
{
public:
	ReflectionTestSubclass() { q = 13; }
	float q;
};
const char* const sReflectionTestSubclass = "ReflectionTestSubclass";

// Hides prop0
static Reflection::BoundProp<float> prop0("P", "Foo", &ReflectionTestClass::p);
static Reflection::BoundProp<float> prop1("P", "Foo", &ReflectionTestSubclass::q);

RBX_REGISTER_CLASS(ReflectionTestSubclass);
RBX_REGISTER_CLASS(ReflectionTestClass);

BOOST_AUTO_TEST_SUITE(Reflection)

	BOOST_AUTO_TEST_CASE(HideMember)
	{
		{
			ReflectionTestClass test;
			RBX::Reflection::Property prop(*test.findPropertyDescriptor("P"), &test);
			BOOST_CHECK_EQUAL(&prop.getDescriptor(), &prop0);
		}
		{
			ReflectionTestSubclass sub;
			RBX::Reflection::Property prop(*sub.findPropertyDescriptor("P"), &sub);
			BOOST_CHECK_EQUAL(&prop.getDescriptor(), &prop1);
		}
	}

	BOOST_AUTO_TEST_CASE(Callbacks)
{
	ReflectionTestClass test;

	// Wire up the callbacks
	desc0.setCallback(&test, f0);
	desc1.setCallback(&test, f1);
	desc2.setCallback(&test, f2);
	desc3.setCallback(&test, f3);
	desc4.setCallback(&test, f4);

	// Confirm the callbacks are correct
	BOOST_CHECK_EQUAL(test.func0, f0);
	BOOST_CHECK_EQUAL(test.func1, f1);
	BOOST_CHECK_EQUAL(test.func2, f2);
	BOOST_CHECK_EQUAL(test.func3, f3);
	BOOST_CHECK_EQUAL(test.func4, f4);

	// Call a callback
	BOOST_CHECK_EQUAL(test.func4(1, 2, 3, 4), 10);
}

BOOST_AUTO_TEST_CASE(GenericCallbacks)
{
	ReflectionTestClass test;

	// Wire up the callbacks
	desc0.setGenericCallbackHelper(&test, g0);
	desc1.setGenericCallbackHelper(&test, g1);
	desc2.setGenericCallbackHelper(&test, g2);
	desc3.setGenericCallbackHelper(&test, g3);
	desc4.setGenericCallbackHelper(&test, g4);

	// Call a callback
	BOOST_CHECK_EQUAL(test.func0(), 23);
	BOOST_CHECK_EQUAL(test.func1(true), 13);
	BOOST_CHECK_EQUAL(test.func2("hello ", "world"), "hello world");
	BOOST_CHECK_EQUAL(test.func3(1, 2, 3), 6);
	BOOST_CHECK_EQUAL(test.func4(1, 2, 3, 4), 10);
}

BOOST_AUTO_TEST_SUITE_END()

