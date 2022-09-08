#include <boost/test/unit_test.hpp>

#include "Rbx/Debug.h"
#include "Util/Base64BinaryInputStream.h"
#include "Util/Base64BinaryOutputStream.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(Base64BinaryStreamTest)

// These tests use BOOST_REQUIRE_* so that the output won't be flooded
			
BOOST_AUTO_TEST_CASE( NoExistingData ) {
	for (int numBits = 1; numBits <= 8; ++numBits) {
		for (int intValue = 0; intValue < (1 << numBits); ++intValue) {
			unsigned char value = intValue;
			Base64BinaryOutputStream bos;
			bos.WriteBits(&value, numBits);
			std::string serialized;
			bos.done(&serialized);

			Base64BinaryInputStream bis(serialized.c_str());
			unsigned char test = 0;
			bis.ReadBits(&test, numBits);

			RBXASSERT(test == value);
			BOOST_REQUIRE_EQUAL(test, value);
		}
	}
}

BOOST_AUTO_TEST_CASE( FiveBitsExistingData ) {
	for (unsigned char prefix = 0; prefix < 32; ++prefix) {
		for (int numBits = 1; numBits <= 8; ++numBits) {
			for (int intValue = 0; intValue < (1 << numBits); ++intValue) {
				unsigned char value = intValue;
				Base64BinaryOutputStream bos;
				bos.WriteBits(&prefix, 5);

				bos.WriteBits(&value, numBits);
				std::string serialized;
				bos.done(&serialized);

				Base64BinaryInputStream bis(serialized.c_str());
				unsigned char test = 0;
				bis.ReadBits(&test, 5);
				BOOST_REQUIRE_EQUAL(prefix, test);

				test = 0;
				bis.ReadBits(&test, numBits);
				BOOST_REQUIRE_EQUAL(test, value);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( LongStreamOfData ) {
	static const size_t kStreamElts = 1 << 18;
	static const int seed = 32885;
	srand(seed);

	std::vector<size_t> numBits(kStreamElts);
	std::vector<unsigned char> values(kStreamElts);

	for (int i = 0; i < kStreamElts; ++i) {
		int rand1 = rand();
		int rand2 = rand();

		numBits[i] = (rand1 & 0x7) + 1;
        // don't clean up higher order bits before write. This tests
        // to make sure that WriteBits does clean up those bits.
		values[i] = rand2;
	}

	Base64BinaryOutputStream bos;
	for (int i = 0; i < kStreamElts; ++i) {
		bos.WriteBits(&values[i], numBits[i]);
	}

	std::string serialized;
	bos.done(&serialized);
	Base64BinaryInputStream bis(serialized.c_str());

	for (int i = 0; i < kStreamElts; ++i) {
		unsigned char read = 0;
		bis.ReadBits(&read, numBits[i]);
		BOOST_REQUIRE_EQUAL(read, values[i] & ((1 << numBits[i]) - 1));
	}
}

BOOST_AUTO_TEST_SUITE_END()