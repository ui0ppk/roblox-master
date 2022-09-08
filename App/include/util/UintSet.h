#pragma once

#include "Util/DoubleEndedVector.h"
#include "boost/cstdint.hpp"

namespace RBX {

/**
 * Attempts to outperform std::set<unsigned int> by using a bitset. This will
 * use less memory than a tree set when the numbers stored are close together
 * (roughly, if the average spacing is less than 24 * sizeof(void*) then this
 * implementation will likely use less memory than a traditional set).
 */
struct UintSet {
	typedef boost::uint32_t BitGroup;

	// these two constants are public for testing
	static const unsigned int kShiftForGroup;
	static const BitGroup kBitInGroupMask;

private:
	struct UpdateBitInfo {
		const BitGroup bitGroup;
		const BitGroup bitPosition; // 0-indexed position (not a bitmask)
		const BitGroup mask;

		UpdateBitInfo(unsigned int intVal);
	};

	// min group offset, group for data in bitSet[0]
	unsigned int minBitGroup;
	// max group offset inclusive (max valid index for bitSet)
	unsigned int maxBitGroup;
	// number of uints stored in this BitSet
	size_t internalSize;
	bool isEmpty;
	// actual bitset
	DoubleEndedVector<BitGroup> bitSet;

public:
	UintSet();

	size_t size() const;

	bool insert(const unsigned int intVal);

	bool contains(const unsigned int intVal);

	void pop_smallest(unsigned int* out);
};

}
