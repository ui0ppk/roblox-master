#include <boost/test/unit_test.hpp>

#include <sstream>

#include "V8Xml/XmlElement.h"
#include "V8Xml/XmlSerializer.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(Xml)

BOOST_AUTO_TEST_CASE(Parse)
{
	std::stringstream stream;
	stream << "<node name=\"egad\">&quot;hello world&quot;</node>";
	TextXmlParser machine(stream.rdbuf());

	std::auto_ptr<XmlElement> node = machine.parse();

	std::string value;
	BOOST_REQUIRE(node->getValue(value));
	BOOST_CHECK_EQUAL(value, "\"hello world\"");

	XmlAttribute* attribute = node->findAttribute(name_name);
	BOOST_REQUIRE(attribute);
	BOOST_REQUIRE(attribute->getValue(value));
	BOOST_CHECK_EQUAL(value, "egad");
}

#if 0
#ifndef _DEBUG
BOOST_FIXTURE_TEST_CASE(Perf, RBX::Test::PerformanceTestFixture)
{
	std::ifstream stream("C:\\Users\\erik.cassel\\Documents\\512balls_128chars_8000fixed.rbxl", std::ios_base::in | std::ios_base::binary);
	TextXmlParser machine(stream.rdbuf());

	BOOST_CHECK(machine.parse().get());
}
#endif
#endif

BOOST_AUTO_TEST_CASE(StringToInt)
{
	XmlNameValuePair element(RBX::Name::getNullName(), "-12");
	int i;
	BOOST_CHECK(element.getValue(i));
	BOOST_CHECK_EQUAL(i, -12);

	XmlNameValuePair element2(RBX::Name::getNullName(), "x12");
	BOOST_CHECK(!element2.getValue(i));

	XmlNameValuePair element3(RBX::Name::getNullName(), "12x");
	BOOST_CHECK(!element3.getValue(i));
}

BOOST_AUTO_TEST_SUITE_END()

