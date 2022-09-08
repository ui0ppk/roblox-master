#include "stdafx.h"

#include "Util/UintSet.h"

#include "rbx/Debug.h"

#ifdef _WIN32
#include <intrin.h>
#else
#include <strings.h>
#endif

namespace RBX {

const unsigned int UintSet::kShiftForGroup = 5;
const UintSet::BitGroup UintSet::kBitInGroupMask = 0x1F;

UintSet::UpdateBitInfo::UpdateBitInfo(unsigned int intVal) 
	: bitGroup(intVal >> kShiftForGroup), bitPosition(intVal & kBitInGroupMask), mask(1 << bitPosition)
{}

UintSet::UintSet()
	: internalSize(0), isEmpty(true), minBitGroup(-1), maxBitGroup(-1), bitSet() {}

size_t UintSet::size() const {
	return internalSize;
}

bool UintSet::insert(const unsigned int intVal) {
	UpdateBitInfo info(intVal);
	if (isEmpty) {
		minBitGroup = info.bitGroup;
		maxBitGroup = info.bitGroup;
		bitSet.push_back(info.mask);
		isEmpty = false;
		internalSize++;

		return true;
	}

	// maintain bitSet so that we can set the right bit
	if (info.bitGroup < minBitGroup) {
		BitGroup extraGroups = minBitGroup - info.bitGroup;
		while (extraGroups) {
			extraGroups--;
			bitSet.push_front(0);
		}
		minBitGroup = info.bitGroup;
	} else if (info.bitGroup > maxBitGroup) {
		BitGroup extraGroups = info.bitGroup - maxBitGroup;
		while (extraGroups) {
			extraGroups--;
			bitSet.push_back(0);
		}
		maxBitGroup = info.bitGroup;
	}

	// set the update bit
	BitGroup& data = bitSet[info.bitGroup - minBitGroup];
	bool notAlreadySet = (~data >> info.bitPosition) & 0x1;
	internalSize += notAlreadySet;
	data |= info.mask;
	return notAlreadySet;
}

bool UintSet::contains(const unsigned int intVal) {
	UpdateBitInfo info(intVal);
	if (isEmpty ||
		info.bitGroup < minBitGroup ||
		info.bitGroup > maxBitGroup) {

		return false;
	}

	return (bitSet[info.bitGroup - minBitGroup] & info.mask) != 0;
}

void UintSet::pop_smallest(unsigned int* out) {
	RBXASSERT(internalSize);
	RBXASSERT(bitSet.size());
	BitGroup poppedGroup = minBitGroup;
	BitGroup& lastData = bitSet[0];
	RBXASSERT(lastData);

	// Step-by-step for this bit magic:
	// first (lastData & (lastData - 1)):
	// logically this turns all least-significant zeros into 1s and turns the
	// least significant 1 into a 0 (by carry):
	// 01100100 - 00000001 =
	// 01100011
	// when we & the original and n-1, everything above the least significant
	// 1 is preserved, and everything from the least significant 1 and lower
	// becomes zero:
	// 01100100 &
	// 01100011 =
	// 01100000
	// finally, when we xor this & with the original, the xor will cancel
	// everything except the least significant 1 (the only value changed via
	// the previous ops is the least significant 1):
	// 01100100 ^
	// 01100000 =
	// 00000100
	BitGroup flag = lastData ^ (lastData & (lastData - 1));
	RBXASSERT(flag != 0);
	RBXASSERT((flag & (flag - 1)) == 0); // assert exactly 1 bit is set

	lastData &= ~flag;
	internalSize--;
	isEmpty = internalSize == 0;
	if (isEmpty) {
		minBitGroup = -1;
		maxBitGroup = -1;
	}

	BitGroup unread;
	while (bitSet.size() != 0 && bitSet[0] == 0) {
		bitSet.pop_front(&unread);
		minBitGroup++;
	}

	// this section mutates flag, so don't read it after
	(*out) = poppedGroup << kShiftForGroup;

#ifdef _WIN32
	unsigned long bitPosition;
	_BitScanForward(&bitPosition, flag);
	(*out) += bitPosition;
#else
	(*out) += ffs(flag) - 1;
#endif
}

}
