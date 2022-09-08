#pragma once

#include <sstream>

namespace RBX {

struct Base64BinaryOutputStream {
private:
	static const char* kTranslateToBase64;

	std::ostringstream result;
	unsigned char buffer;
	size_t bitsUsed;

public:

	Base64BinaryOutputStream();
	
	// NOT A REAL IMPLEMENTATION -- ONLY PRESENT TO SATISFY TEMPLATES
	size_t GetNumberOfBytesUsed() const;

	// numBitsToAdd must be >= 0 and <= 8. Only the character immediately
	// pointed to by data will be read
	void WriteBits(const unsigned char* data, size_t numBitsToAdd);

	// make sure to call this method exactly once: when you will no longer
	// call WriteBits again on this object.
	void done(std::string* out);
};

}
