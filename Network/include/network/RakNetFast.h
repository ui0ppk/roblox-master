/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

//#include "Util/Velocity.h"
#include "bitstream.h"

//#include "Dictionary.h"
#include "Util.h"

namespace RBX
{
	namespace Network
	{
		// --------------------------------------------------------------------
		// reverseEndian
		//
		// Reuse the numeric constants as much as possible to reduce constant value loads.

		template <unsigned int Bytes> struct ReverseEndianBytes
		{
			template <class T>
			static inline void reverseEndian(T& result) {
				BOOST_STATIC_ASSERT( sizeof(T) == 1 );
			}
		};

		template<> struct ReverseEndianBytes<2>
		{
			template <class T>
			static inline void reverseEndian(T& x) {
				uint16_t& result = (uint16_t&)x;
				result = (result << 8) | (result >> 8);
			}
		};

		template<> struct ReverseEndianBytes<4>
		{
			template <class T>
			static inline void reverseEndian(T& x) {
				uint32_t& result = (uint32_t&)x;
				uint32_t val = ((result & 0x00FF00FF) << 8) | ((result >> 8) & 0x00FF00FF); 
				result = (val << 16) | (val >> 16);
			}
		};

		template<> struct ReverseEndianBytes<8>
		{
			template <class T>
			static inline void reverseEndian(T& x) {
				uint64_t& result = (uint64_t&)x;
				uint64_t val = (result << 32) | (result >> 32);
						 val = (val & 0x0000FFFF0000FFFF) << 16 | ((val >> 16) & 0x0000FFFF0000FFFF);
						 val = (val & 0x00FF00FF00FF00FF) << 8  | ((val >> 8 ) & 0x00FF00FF00FF00FF);
				result = val;
			}
		};

		template <class T>
		inline void reverseEndianBits(T& result, unsigned int bits ) {
			unsigned int msbPad = (-(signed int)bits)&7; // == (8-(bits&7))&7.  High bits that were truncated off the MSB.

			// Shift up by the amount clipped off the non-zero-MSB.
			T shifted = result << msbPad;

			// Right align the non-zero-MSB while it is still in the LSB and easy to locate.
			result = (shifted & ~(T)0xff) | (result & ((T)0xff >> msbPad));

			if( (sizeof(T)*8-7) <= bits )
			{
				ReverseEndianBytes<sizeof(T)>::reverseEndian( result );
			}
			else
			{
				// Warning:  Taking the address of a parameter forces a register spill.
				unsigned char* a = (unsigned char*)&result;
				unsigned char* b = a + ((bits-1) >> 3);
				while( a < b )
				{
					unsigned char t = *a;
					*a++ = *b;
					*b-- = t;
				}
			}
		}

		// --------------------------------------------------------------------

		template <unsigned int Bytes> struct ReadFastBytes
		{
			template <class T>
			static inline void readFast(RakNet::BitStream& bitStream, T& result, unsigned int bits)
			{
				unsigned int readOffset = bitStream.GetReadOffset();

				if (readOffset + bits > bitStream.GetNumberOfBitsUsed())
					throw RBX::network_stream_exception("readFast past end");

				const unsigned char* data = bitStream.GetData();
				const unsigned char* pos = data + (readOffset >> 3);

				unsigned int bitsOffset = readOffset & 7;
				unsigned int firstByteBits = 8 - bitsOffset;

				unsigned char firstByte = *pos & (0xff >> bitsOffset);

				if(firstByteBits >= bits)
				{
					result = (T)(firstByte >> (firstByteBits - bits));
					bitStream.SetReadOffset(readOffset + bits);
					return;
				}

				T tmp = (T)firstByte;
				++pos;

				unsigned int bitsRemaining = bits - firstByteBits; // 8 could be propagated out.
				for(; bitsRemaining > 8; bitsRemaining -= 8)
				{
					tmp = (tmp << 8) | *pos;
					++pos;
				}

				tmp = (tmp << bitsRemaining) | (*pos >> (8 - bitsRemaining));
				result = tmp;

				bitStream.SetReadOffset(readOffset + bits);
			}


			template <unsigned int Bits, class T>
			static inline void readFast(RakNet::BitStream& bitStream, T& result)
			{
				readFast(bitStream, result, Bits);
			}
		};

		template <> struct ReadFastBytes<1>
		{
			template <unsigned int Bits, class T>
			static inline void readFast(RakNet::BitStream& bitStream, T& result)
			{
				BOOST_STATIC_ASSERT(Bits >= 1 && Bits <= 8);

				unsigned int readOffset = bitStream.GetReadOffset();

				if (readOffset + (Bits+8) > bitStream.GetNumberOfBitsUsed())
				{
					// Avoid crashing due to reading an extra byte off the end of the buffer.
					ReadFastBytes<0>::readFast(bitStream, result, Bits);
				}
                else
                {
    				const unsigned char* data = bitStream.GetData();

    				unsigned int readOffsetBytes = readOffset >> 3;

    				unsigned char byte0 = data[readOffsetBytes + 0];
    				unsigned char byte1 = data[readOffsetBytes + 1];

    				result = (((((byte0 << 8) | byte1) << (readOffset & 7)) & 0xffff) >> (16 - Bits) );

    				bitStream.SetReadOffset(readOffset + Bits);
                }
			}
		};

		template <> struct ReadFastBytes<2>
		{
			template <unsigned int Bits, class T>
			static inline void readFast(RakNet::BitStream& bitStream, T& result)
			{
				BOOST_STATIC_ASSERT(Bits >= 9 && Bits <= 16);

				unsigned int readOffset = bitStream.GetReadOffset();

				if (readOffset + (Bits+8) > bitStream.GetNumberOfBitsUsed())
				{
					// Avoid crashing due to reading an extra byte off the end of the buffer.
					ReadFastBytes<0>::readFast(bitStream, result, Bits);
				}
                else
                {
    				const unsigned char* data = bitStream.GetData();

    				unsigned int readOffsetBytes = readOffset >> 3;

    				unsigned char byte0 = data[readOffsetBytes + 0];
    				unsigned char byte1 = data[readOffsetBytes + 1];
    				unsigned char byte2 = data[readOffsetBytes + 2];

    				result = ((((byte0 << 16) | (byte1 << 8) | byte2) << (readOffset & 7)) & 0xffffff) >> (24 - Bits);

    				bitStream.SetReadOffset(readOffset + Bits);
                }
			}
		};

		template <> struct ReadFastBytes<4>
		{
			template <unsigned int Bits, class T>
			static inline void readFast(RakNet::BitStream& bitStream, T& result)
			{
				BOOST_STATIC_ASSERT(Bits >= 17 && Bits <= 32);

				unsigned int readOffset = bitStream.GetReadOffset();

				if (readOffset + (Bits+8) > bitStream.GetNumberOfBitsUsed())
				{
					// Avoid crashing due to reading an extra byte off the end of the buffer.
					ReadFastBytes<0>::readFast(bitStream, result, Bits);
				}
                else
                {
    				const unsigned char* data = bitStream.GetData();

    				unsigned int readOffsetBytes = readOffset >> 3;

    				unsigned char byte0 = data[readOffsetBytes + 0];
    				unsigned char byte1 = data[readOffsetBytes + 1];
    				unsigned char byte2 = data[readOffsetBytes + 2];
    				unsigned char byte3 = data[readOffsetBytes + 3];
    				unsigned char byte4 = data[readOffsetBytes + 4];

    				T tmp0 = (((((byte0 << 16) | (byte1 << 8) | byte2) << (readOffset & 7)) & 0xffffff) >> (24 - 16));
    				T tmp1 = (((((byte2 << 16) | (byte3 << 8) | byte4) << (readOffset & 7)) & 0xffffff) >> (24 - (Bits - 16)));
    				result = (tmp0 << 16) | tmp1;

    				bitStream.SetReadOffset(readOffset + Bits);
                }
			}
		};

		// --------------------------------------------------------------------
		//
		// The non-"T" versions replace ReadBits versions which did not do
		// endian swapping

		template <class T>
		inline void readFastN(RakNet::BitStream& bitStream, T& result, unsigned int bits) {
			// Use default implementation for arbitrary bit-counts.
			ReadFastBytes<0>::readFast(bitStream, result, bits);

			reverseEndianBits( result, bits );
		}

		// The base template should only be needed for unusual numbers of bits.
		template <unsigned int Bits, class T>
		inline void readFastN(RakNet::BitStream& bitStream, T& result) {
			ReadFastBytes<0>::readFast<Bits>(bitStream, result);

			reverseEndianBits( result, Bits );
		}

		// --------------------------------------------------------------------
		// readFastT
		//
		// The "T" versions do not do endian swapping because the BitStream::Write
		// sends across network order/big-endian data.

		template <class T>
		inline void readFastT(RakNet::BitStream& bitStream, T& result)
		{
			BOOST_STATIC_ASSERT( sizeof(T) == 0 ); // This is undefined.
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, bool& result)
		{
			unsigned int readOffset = bitStream.GetReadOffset();

			if (readOffset + 1 > bitStream.GetNumberOfBitsUsed())
				throw RBX::network_stream_exception("readFastBool past end");

			const unsigned char* data = bitStream.GetData();

			bool tmp = (data[readOffset >> 3] & (0x80 >> (readOffset & 7))) != 0;
			result = tmp;

			bitStream.SetReadOffset(readOffset + 1);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, float& result) {
			union { float f; uint32_t i; } t;
			ReadFastBytes<4>::readFast<32, uint32_t>(bitStream, t.i);
			result = t.f;
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, double& result) {
			union { double d; uint64_t i; } t;
			ReadFastBytes<0>::readFast<64, uint64_t>(bitStream, t.i);
			result = t.d;
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, int8_t& result) {
			ReadFastBytes<1>::readFast<8, uint8_t>(bitStream, (uint8_t&)result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, uint8_t& result) {
			ReadFastBytes<1>::readFast<8, uint8_t>(bitStream, result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, char& result) {
			ReadFastBytes<1>::readFast<8, uint8_t>(bitStream, (uint8_t&)result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, int16_t& result) {
			ReadFastBytes<2>::readFast<16, uint16_t>(bitStream, (uint16_t&)result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, uint16_t& result) {
			ReadFastBytes<2>::readFast<16, uint16_t>(bitStream, result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, int32_t& result) {
			ReadFastBytes<4>::readFast<32, uint32_t>(bitStream, (uint32_t&)result);
		}
        
		template <>
		inline void readFastT(RakNet::BitStream& bitStream, long& result) {
#if defined(__LP64__)
            BOOST_STATIC_ASSERT(sizeof(long) == sizeof(uint64_t));
            ReadFastBytes<0>::readFast<64, uint64_t>(bitStream, (uint64_t&)result);
#else
            BOOST_STATIC_ASSERT(sizeof(long) == sizeof(uint32_t));
            ReadFastBytes<4>::readFast<32, uint32_t>(bitStream, (uint32_t&)result);
#endif

		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, unsigned long& result) {
#if defined(__LP64__)
            BOOST_STATIC_ASSERT(sizeof(unsigned long) == sizeof(uint64_t));
            ReadFastBytes<0>::readFast<64, uint64_t>(bitStream, (uint64_t&)result);
#else
            BOOST_STATIC_ASSERT(sizeof(unsigned long) == sizeof(uint32_t));
            ReadFastBytes<4>::readFast<32, uint32_t>(bitStream, (uint32_t&)result);
#endif

		}

        
		template <>
		inline void readFastT(RakNet::BitStream& bitStream, uint32_t& result) {
			ReadFastBytes<4>::readFast<32, uint32_t>(bitStream, result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, int64_t& result) {
			ReadFastBytes<0>::readFast<64, uint64_t>(bitStream, (uint64_t&)result);
		}

		template <>
		inline void readFastT(RakNet::BitStream& bitStream, uint64_t& result) {
			ReadFastBytes<0>::readFast<64, uint64_t>(bitStream, result);
		}

		// --------------------------------------------------------------------

		inline void readVectorFast(RakNet::BitStream& bitStream, float& x, float& y, float& z)
		{
			float magnitude;
			readFastT(bitStream, magnitude);
			if (magnitude>0.00001f)
			{
				unsigned short c;
				readFastT(bitStream, c);
				x = (float(c) * (1.0f / 32767.5f) - 1.0f) * magnitude;

				readFastT(bitStream, c);
				y = (float(c) * (1.0f / 32767.5f) - 1.0f) * magnitude;

				readFastT(bitStream, c);
				z = (float(c) * (1.0f / 32767.5f) - 1.0f) * magnitude;
			}
			else
			{
				x = y = z = 0;
			}
		}
	}
}
