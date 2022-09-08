#include "stdafx.h"
#include "Util/Base64BinaryOutputStream.h"

#include "Rbx/Debug.h"

namespace RBX {

const char* Base64BinaryOutputStream::kTranslateToBase64 = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789"
	"+/";

Base64BinaryOutputStream::Base64BinaryOutputStream() : buffer(0), bitsUsed(0) {}

size_t Base64BinaryOutputStream::GetNumberOfBytesUsed() const {
	// for the time being, only needed to satisfy templatization
	RBXASSERT(false);
    throw std::runtime_error("Base64BinaryOutputStream::GetNumberOfBytesUsed Not Implemented");
}

void Base64BinaryOutputStream::WriteBits(
		const unsigned char* doNotReadFromThisVariableAfterFirstRead,
		size_t numBitsToAdd) {
	RBXASSERT(numBitsToAdd <= 8);
    RBXASSERT(numBitsToAdd >= 1);
    RBXASSERT(doNotReadFromThisVariableAfterFirstRead);

	unsigned char dataCopy = *doNotReadFromThisVariableAfterFirstRead;

	while (numBitsToAdd > 0) {
		// clip off higher order bits: only the "numBitsToAdd" (remaining)
		// least significant bits should be put into the stream
		dataCopy &= ((1 << numBitsToAdd) - 1);

		size_t bitsThatFit = std::min(numBitsToAdd, 6 - bitsUsed);

		buffer <<= bitsThatFit;
		buffer |= dataCopy >> (numBitsToAdd - bitsThatFit);

		bitsUsed += bitsThatFit;
        RBXASSERT(bitsUsed <= 6);
		if (bitsUsed == 6) {
			result << ((const char)kTranslateToBase64[buffer]);
		
			buffer = 0;
			bitsUsed = 0;
		}

		numBitsToAdd -= bitsThatFit;
	}
}

void Base64BinaryOutputStream::done(std::string* out) {
	if (bitsUsed > 0) {
		unsigned char zero = 0;
		WriteBits(&zero, 6 - bitsUsed);
	}
	RBXASSERT(bitsUsed == 0);
	RBXASSERT(buffer == 0);

	(*out) = result.str();
}

}
