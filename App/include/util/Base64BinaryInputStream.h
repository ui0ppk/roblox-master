#pragma once

#include "boost/cstdint.hpp"

namespace RBX {

struct Base64BinaryInputStream {
private:
	const char* source;
	boost::uint16_t buffer;
	size_t readableBitsInBuffer;
	static unsigned char decode(unsigned char charFromString);

public:
	Base64BinaryInputStream(const char* source);

	// numBitsToRead needs to be >= 1 and <= 8.
	void ReadBits(unsigned char* out, size_t numBitsToRead);
};

}
