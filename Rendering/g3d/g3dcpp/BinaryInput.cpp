/**
 @file BinaryInput.cpp
 
 @author Morgan McGuire, graphics3d.com
 Copyright 2001-2007, Morgan McGuire.  All rights reserved.
 
 @created 2001-08-09
 @edited  2010-03-05


  <PRE>
    {    
    BinaryOutput b("c:/tmp/test.b", BinaryOutput::LITTLE_ENDIAN);

    float f = 3.1415926;
    int i = 1027221;
    std::string s = "Hello World!";

    b.writeFloat32(f);
    b.writeInt32(i);
    b.writeString(s);
    b.commit();
    

    BinaryInput in("c:/tmp/test.b", BinaryInput::LITTLE_ENDIAN);

    debugAssert(f == in.readFloat32());
    int ii = in.readInt32();
    debugAssert(i == ii);
    debugAssert(s == in.readString());
    }
  </PRE>
 */

#include "G3D/platform.h"
#include "G3D/BinaryInput.h"
#include "G3D/Array.h"
#include "G3D/fileutils.h"
#include <zlib.h>
#include <stdexcept>
#include <cstring>

namespace G3D {

void BinaryInput::readBool8(std::vector<bool>& out, int64 n) {
    out.resize((int)n);
    // std::vector optimizes bool in a way that prevents fast reading
    for (int64 i = 0; i < n ; ++i) {
        out[i] = readBool8();
    }
}


void BinaryInput::readBool8(Array<bool>& out, int64 n) {
    out.resize(n);
    readBool8(out.begin(), n);
}


#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(std::vector<lcase>& out, int64 n) {\
    out.resize(n);\
    read##ucase(&out[0], n);\
}\
\
\
void BinaryInput::read##ucase(Array<lcase>& out, int64 n) {\
    out.resize(n);\
    read##ucase(out.begin(), n);\
}


IMPLEMENT_READER(UInt8,   uint8)
IMPLEMENT_READER(Int8,    int8)
IMPLEMENT_READER(UInt16,  uint16)
IMPLEMENT_READER(Int16,   int16)
IMPLEMENT_READER(UInt32,  uint32)
IMPLEMENT_READER(Int32,   int32)
IMPLEMENT_READER(UInt64,  uint64)
IMPLEMENT_READER(Int64,   int64)
IMPLEMENT_READER(Float32, float32)
IMPLEMENT_READER(Float64, float64)    

#undef IMPLEMENT_READER

// Data structures that are one byte per element can be 
// directly copied, regardles of endian-ness.
#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(lcase* out, int64 n) {\
    if (sizeof(lcase) == 1) {\
        readBytes(out, n);\
    } else {\
        for (int64 i = 0; i < n ; ++i) {\
            out[i] = read##ucase();\
        }\
    }\
}

IMPLEMENT_READER(Bool8,   bool)
IMPLEMENT_READER(UInt8,   uint8)
IMPLEMENT_READER(Int8,    int8)

#undef IMPLEMENT_READER


#define IMPLEMENT_READER(ucase, lcase)\
void BinaryInput::read##ucase(lcase* out, int64 n) {\
    if (m_swapBytes) {\
        for (int64 i = 0; i < n; ++i) {\
            out[i] = read##ucase();\
        }\
    } else {\
        readBytes(out, sizeof(lcase) * n);\
    }\
}


IMPLEMENT_READER(UInt16,  uint16)
IMPLEMENT_READER(Int16,   int16)
IMPLEMENT_READER(UInt32,  uint32)
IMPLEMENT_READER(Int32,   int32)
IMPLEMENT_READER(UInt64,  uint64)
IMPLEMENT_READER(Int64,   int64)
IMPLEMENT_READER(Float32, float32)
IMPLEMENT_READER(Float64, float64)    

#undef IMPLEMENT_READER

void BinaryInput::loadIntoMemory(int64 startPosition, int64 minLength) {
	assert(false); // should not be using this
}



const bool BinaryInput::NO_COPY = false;
    
static bool needSwapBytes(G3DEndian fileEndian) {
    return (fileEndian != System::machineEndian());
}


/** Helper used by the constructors for decompression */
static uint32 readUInt32(const uint8* data, bool swapBytes) {
    if (swapBytes) {
        uint8 out[4];
        out[0] = data[3];
        out[1] = data[2];
        out[2] = data[1];
        out[3] = data[0];
        return *((uint32*)out);
    } else {
        return *((uint32*)data);
    }
}


void BinaryInput::setEndian(G3DEndian e) {
    m_fileEndian = e;
    m_swapBytes = needSwapBytes(m_fileEndian);
}


BinaryInput::BinaryInput(
    const uint8*        data,
    int64               dataLen,
    G3DEndian           dataEndian,
    bool                compressed,
    bool                copyMemory) :
    m_filename("<memory>"),
    m_bitPos(0),
    m_bitString(0),
    m_beginEndBits(0),
    m_alreadyRead(0),
    m_bufferLength(0),
    m_pos(0) {

    setEndian(dataEndian);
    m_freeBuffer = copyMemory || compressed;

    if (compressed) {
        // Read the decompressed size from the first 4 bytes
        m_length = G3D::readUInt32(data, m_swapBytes);

        debugAssert(m_freeBuffer);
        m_buffer = (uint8*)malloc(m_length); // was: alignedMalloc()

        unsigned long L = m_length;
        // Decompress with zlib
        int64 result = uncompress(m_buffer, (unsigned long*)&L, data + 4, dataLen - 4);
        m_length = L;
        m_bufferLength = L;
        debugAssert(result == Z_OK); (void)result;

    } else {
    	m_length = dataLen;
        m_bufferLength = m_length;
        if (! copyMemory) {
 	        debugAssert(!m_freeBuffer);
            m_buffer = const_cast<uint8*>(data);
        } else {
	        debugAssert(m_freeBuffer);
            m_buffer = (uint8*)malloc(m_length); // was: alignedMalloc()
            System::memcpy(m_buffer, data, dataLen);
        }
    }
}


BinaryInput::BinaryInput(
    const std::string&  filename,
    G3DEndian           fileEndian,
    bool                compressed) :
    m_filename(filename),
    m_bitPos(0),
    m_bitString(0),
    m_beginEndBits(0),
    m_alreadyRead(0),
    m_length(0),
    m_bufferLength(0),
    m_buffer(NULL),
    m_pos(0),
    m_freeBuffer(true) {


		assert (false ); // we should not be using this
}

void BinaryInput::decompress() {
    // Decompress
    // Use the existing buffer as the source, allocate
    // a new buffer to use as the destination.
    assert(false); // we should not be using this
}


void BinaryInput::readBytes(void* bytes, int64 n) {
    prepareToRead(n);
    debugAssert(isValidPointer(bytes));

    memcpy(bytes, m_buffer + m_pos, n);
    m_pos += n;
}


BinaryInput::~BinaryInput() {
    if (m_freeBuffer) {
        free(m_buffer); // was: alignedFree()
    }
    m_buffer = NULL;
}


uint64 BinaryInput::readUInt64() {
    prepareToRead(8);
    uint8 out[8];

    if (m_swapBytes) {
        out[0] = m_buffer[m_pos + 7];
        out[1] = m_buffer[m_pos + 6];
        out[2] = m_buffer[m_pos + 5];
        out[3] = m_buffer[m_pos + 4];
        out[4] = m_buffer[m_pos + 3];
        out[5] = m_buffer[m_pos + 2];
        out[6] = m_buffer[m_pos + 1];
        out[7] = m_buffer[m_pos + 0];
    } else {
        *(uint64*)out = *(uint64*)(m_buffer + m_pos);
    }

    m_pos += 8;
    return *(uint64*)out;
}


std::string BinaryInput::readString(int64 n) {
	assert(false); // we should not be using this
	std::string dummystring;
	return dummystring;

}


std::string BinaryInput::readString() {
    int64 n = 0;

    if ((m_pos + m_alreadyRead + n) < (m_length - 1)) {
        prepareToRead(1);
    }

    if ( ((m_pos + m_alreadyRead + n) < (m_length - 1)) &&
         (m_buffer[m_pos + n] != '\0')) {

        ++n;
        while ( ((m_pos + m_alreadyRead + n) < (m_length - 1)) &&
                (m_buffer[m_pos + n] != '\0')) {

            prepareToRead(1);
            ++n;
        }
    }

    // Consume NULL
    ++n;

    return readString(n);
}

static bool isNewline(char c) {
    return c == '\n' || c == '\r';
}

std::string BinaryInput::readStringNewline() {
    int64 n = 0;

    if ((m_pos + m_alreadyRead + n) < (m_length - 1)) {
        prepareToRead(1);
    }

    if ( ((m_pos + m_alreadyRead + n) < (m_length - 1)) &&
         ! isNewline(m_buffer[m_pos + n])) {

        ++n;
        while ( ((m_pos + m_alreadyRead + n) < (m_length - 1)) &&
                ! isNewline(m_buffer[m_pos + n])) {

            prepareToRead(1);
            ++n;
        }
    }

    const std::string s = readString(n);

    // Consume the newline
    char firstNLChar = readUInt8();

    // Consume the 2nd newline
    if (isNewline(m_buffer[m_pos + 1]) && (m_buffer[m_pos + 1] != firstNLChar)) {
        readUInt8();
    }

    return s;
}


std::string BinaryInput::readStringEven() {
    std::string x = readString();
    if (hasMore() && (G3D::isOdd(x.length() + 1))) {
        skip(1);
    }
    return x;
}


std::string BinaryInput::readString32() {
    int len = readUInt32();
    return readString(len);
}


Vector4 BinaryInput::readVector4() {
    float x = readFloat32();
    float y = readFloat32();
    float z = readFloat32();
    float w = readFloat32();
    return Vector4(x, y, z, w);
}


Vector3 BinaryInput::readVector3() {
    float x = readFloat32();
    float y = readFloat32();
    float z = readFloat32();
    return Vector3(x, y, z);
}


Vector2 BinaryInput::readVector2() {
    float x = readFloat32();
    float y = readFloat32();
    return Vector2(x, y);
}


Color4 BinaryInput::readColor4() {
    float r = readFloat32();
    float g = readFloat32();
    float b = readFloat32();
    float a = readFloat32();
    return Color4(r, g, b, a);
}


Color3 BinaryInput::readColor3() {
    float r = readFloat32();
    float g = readFloat32();
    float b = readFloat32();
    return Color3(r, g, b);
}


void BinaryInput::beginBits() {
    debugAssert(m_beginEndBits == 0);
    m_beginEndBits = 1;
    m_bitPos = 0;

    debugAssertM(hasMore(), "Can't call beginBits when at the end of a file");
    m_bitString = readUInt8();
}


uint32 BinaryInput::readBits(int numBits) {
    debugAssert(m_beginEndBits == 1);

    uint32 out = 0;

    const int total = numBits;
    while (numBits > 0) {
        if (m_bitPos > 7) {
            // Consume a new byte for reading.  We do this at the beginning
            // of the loop so that we don't try to read past the end of the file.
            m_bitPos = 0;
            m_bitString = readUInt8();
        }

        // Slide the lowest bit of the bitString into
        // the correct position.
        out |= (m_bitString & 1) << (total - numBits);

        // Shift over to the next bit
        m_bitString = m_bitString >> 1;
        ++m_bitPos;
        --numBits;
    }

    return out;
}


void BinaryInput::endBits() {
    debugAssert(m_beginEndBits == 1);
    if (m_bitPos == 0) {
        // Put back the last byte we read
        --m_pos;
    }
    m_beginEndBits = 0;
    m_bitPos = 0;
}

}
