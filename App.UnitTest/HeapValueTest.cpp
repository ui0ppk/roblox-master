#include <boost/test/unit_test.hpp>

#include "Util/HeapValue.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(HeapValueTest)

BOOST_AUTO_TEST_CASE(OperatorOverloads) {
	static const float kOriginalValue = 4.5f;
	HeapValue<float> heapValue(4.5f);

	float nonRef = heapValue;
	
	BOOST_CHECK_EQUAL(nonRef, heapValue);
	BOOST_CHECK_EQUAL(nonRef, kOriginalValue);
	
	heapValue = 9.5f;

	BOOST_CHECK(nonRef != heapValue);
	BOOST_CHECK_EQUAL(nonRef, kOriginalValue);
}

BOOST_AUTO_TEST_SUITE_END()
