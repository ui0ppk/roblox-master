#pragma once

#include "Network/api.h"

namespace RBX {

template<int WindowSize=4>
struct VarInt {
	static const unsigned char kDataMask = (1 << WindowSize) - 1;
	static const unsigned char kFlagMask = (1 << WindowSize);

	template<class OutputStream>
	static void encode(OutputStream& out, unsigned int count) {
		RBXASSERT(WindowSize < 8);

		do {
			unsigned char data = count & kDataMask;
			count >>= WindowSize;
			if (count) {
				data |= kFlagMask;
			}
			out.WriteBits(&data, WindowSize + 1);
		} while (count);
	}

	template<class InputStream>
	static void decode(InputStream& in, unsigned int* out) {
		RBXASSERT(WindowSize < 8);

		unsigned int count = 0;
		(*out) = 0;
		unsigned char data;
		do {
			data = 0;
			in.ReadBits(&data, WindowSize + 1);

// TODO: The streaming.h include needs to be moved for it to be used here.
//
//			Network::readFastN<WindowSize + 1>( in, data );

			(*out) |= (data & kDataMask) << (WindowSize * count);
			count++;
		} while (data & kFlagMask);
	}

};

}
