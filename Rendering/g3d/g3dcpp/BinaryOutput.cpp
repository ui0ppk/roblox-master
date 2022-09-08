/**
 @file BinaryOutput.cpp
 
 @author Morgan McGuire, http://graphics.cs.williams.edu
 Copyright 2002-2010, Morgan McGuire, All rights reserved.
 
 @created 2002-02-20
 @edited  2010-03-17
 */

#include "G3D/platform.h"
#include "G3D/BinaryOutput.h"
#include "G3D/fileutils.h"
#include "G3D/stringutils.h"
#include "G3D/Array.h"
#include <zlib.h>
#include <stdexcept>
#include <cstring>

#if defined(G3D_LINUX) || defined(G3D_IOS) || defined(G3D_ANDROID) // ROBLOX
#    include <errno.h>
#endif

// Largest memory buffer that the system will use for writing to
// disk.  After this (or if the system runs out of memory)
// chunks of the file will be dumped to disk.
//
// Currently 400 MB
#define MAX_BINARYOUTPUT_BUFFER_SIZE 400000000

namespace G3D {

void BinaryOutput::writeBool8(const std::vector<bool>& out, int n) {
    for (int i = 0; i < n; ++i) {
        writeBool8(out[i]);
    }
}


void BinaryOutput::writeBool8(const Array<bool>& out, int n) {
    writeBool8(out.getCArray(), n);
}

#define IMPLEMENT_WRITER(ucase, lcase)\
void BinaryOutput::write##ucase(const std::vector<lcase>& out, int n) {\
    write##ucase(&out[0], n);\
}\
\
\
void BinaryOutput::write##ucase(const Array<lcase>& out, int n) {\
    write##ucase(out.getCArray(), n);\
}


IMPLEMENT_WRITER(UInt8,   uint8)
IMPLEMENT_WRITER(Int8,    int8)
IMPLEMENT_WRITER(UInt16,  uint16)
IMPLEMENT_WRITER(Int16,   int16)
IMPLEMENT_WRITER(UInt32,  uint32)
IMPLEMENT_WRITER(Int32,   int32)
IMPLEMENT_WRITER(UInt64,  uint64)
IMPLEMENT_WRITER(Int64,   int64)
IMPLEMENT_WRITER(Float32, float32)
IMPLEMENT_WRITER(Float64, float64)    

#undef IMPLEMENT_WRITER

// Data structures that are one byte per element can be 
// directly copied, regardles of endian-ness.
#define IMPLEMENT_WRITER(ucase, lcase)\
void BinaryOutput::write##ucase(const lcase* out, int n) {\
    if (sizeof(lcase) == 1) {\
        writeBytes((void*)out, n);\
    } else {\
        for (int i = 0; i < n ; ++i) {\
            write##ucase(out[i]);\
        }\
    }\
}

IMPLEMENT_WRITER(Bool8,   bool)
IMPLEMENT_WRITER(UInt8,   uint8)
IMPLEMENT_WRITER(Int8,    int8)

#undef IMPLEMENT_WRITER


#define IMPLEMENT_WRITER(ucase, lcase)\
void BinaryOutput::write##ucase(const lcase* out, int n) {\
    if (m_swapBytes) {\
        for (int i = 0; i < n; ++i) {\
            write##ucase(out[i]);\
        }\
    } else {\
        writeBytes((const void*)out, sizeof(lcase) * n);\
    }\
}


IMPLEMENT_WRITER(UInt16,  uint16)
IMPLEMENT_WRITER(Int16,   int16)
IMPLEMENT_WRITER(UInt32,  uint32)
IMPLEMENT_WRITER(Int32,   int32)
IMPLEMENT_WRITER(UInt64,  uint64)
IMPLEMENT_WRITER(Int64,   int64)
IMPLEMENT_WRITER(Float32, float32)
IMPLEMENT_WRITER(Float64, float64)    

#undef IMPLEMENT_WRITER


void BinaryOutput::reallocBuffer(size_t bytes, size_t oldBufferLen) {
    //debugPrintf("reallocBuffer(%d, %d)\n", bytes, oldBufferLen);

    size_t newBufferLen = (int)(m_bufferLen * 1.5) + 100;
    uint8* newBuffer = NULL;

    if ((m_filename == "<memory>") || (newBufferLen < MAX_BINARYOUTPUT_BUFFER_SIZE)) {
        // We're either writing to memory (in which case we *have* to try and allocate)
        // or we've been asked to allocate a reasonable size buffer.

        //debugPrintf("  realloc(%d)\n", newBufferLen); 
        newBuffer = (uint8*)System::realloc(m_buffer, newBufferLen);
        if (newBuffer != NULL) {
            m_maxBufferLen = newBufferLen;
        }
    }

    if ((newBuffer == NULL) && (bytes > 0)) {
        // Realloc failed; we're probably out of memory.  Back out
        // the entire call and try to dump some data to disk.
        m_bufferLen = oldBufferLen;
        reserveBytesWhenOutOfMemory(bytes);
    } else {
        m_buffer = newBuffer;
        debugAssert(isValidHeapPointer(m_buffer));
    }
}


void BinaryOutput::reserveBytesWhenOutOfMemory(size_t bytes) {
	assert(false); // we should not be using this
}


BinaryOutput::BinaryOutput() {
    m_alreadyWritten = 0;
    m_swapBytes = false;
    m_pos       = 0;
    m_filename  = "<memory>";
    m_buffer = NULL;
    m_bufferLen = 0;
    m_maxBufferLen = 0;
    m_beginEndBits = 0;
    m_bitString = 0;
    m_bitPos = 0;
    m_ok = true;
    m_committed = false;
}


BinaryOutput::BinaryOutput(
    const std::string&  filename,
    G3DEndian           fileEndian) {

    m_pos = 0;
    m_alreadyWritten = 0;
    setEndian(fileEndian);
    m_filename = filename;
    m_buffer = NULL;
    m_bufferLen = 0;
    m_maxBufferLen = 0;
    m_beginEndBits = 0;
    m_bitString = 0;
    m_bitPos = 0;
    m_committed = false;

    m_ok = true;    
    /** Verify ability to write to disk */
    commit(false);
    m_committed = false;
}


void BinaryOutput::reset() {
    debugAssert(m_beginEndBits == 0);
    alwaysAssertM(m_filename == "<memory>", 
        "Can only reset a BinaryOutput that writes to memory.");

    // Do not reallocate, just clear the size of the buffer.
    m_pos = 0;
    m_alreadyWritten = 0;
    m_bufferLen = 0;
    m_beginEndBits = 0;
    m_bitString = 0;
    m_bitPos = 0;
    m_committed = false;
}


BinaryOutput::~BinaryOutput() {
    debugAssert((m_buffer == NULL) || isValidHeapPointer(m_buffer));
    System::free(m_buffer);
    m_buffer = NULL;
    m_bufferLen = 0;
    m_maxBufferLen = 0;
}


void BinaryOutput::setEndian(G3DEndian fileEndian) {
    m_fileEndian = fileEndian;
    m_swapBytes = (fileEndian != System::machineEndian());
}


bool BinaryOutput::ok() const {
    return m_ok;
}


void BinaryOutput::compress() {
    if (m_alreadyWritten > 0) {
        throw "Cannot compress huge files (part of this file has already been written to disk).";
    }

    // Old buffer size
    int L = m_bufferLen;
    uint8* convert = (uint8*)&L;

    // Zlib requires the output buffer to be this big
    unsigned long newSize = iCeil(m_bufferLen * 1.01) + 12;
    uint8* temp = (uint8*)System::malloc(newSize);
    int result = compress2(temp, &newSize, m_buffer, m_bufferLen, 9); 

    debugAssert(result == Z_OK); (void)result;

    // Write the header
    if (m_swapBytes) {
        m_buffer[0] = convert[3];
        m_buffer[1] = convert[2];
        m_buffer[2] = convert[1];
        m_buffer[3] = convert[0];
    } else {
        m_buffer[0] = convert[0];
        m_buffer[1] = convert[1];
        m_buffer[2] = convert[2];
        m_buffer[3] = convert[3];
    }

    // Write the data
    if ((int64)newSize + 4 > (int64)m_maxBufferLen) {
        m_maxBufferLen = newSize + 4;
        m_buffer = (uint8*)System::realloc(m_buffer, m_maxBufferLen);
    }
    m_bufferLen = newSize + 4;
    System::memcpy(m_buffer + 4, temp, newSize);
    m_pos = m_bufferLen;

    System::free(temp);
}


void BinaryOutput::commit(bool flush) {
	assert(false); // we should not be using this
}


void BinaryOutput::commit(
    uint8*                  out) {
    debugAssertM(! m_committed, "Cannot commit twice");
    m_committed = true;

    System::memcpy(out, m_buffer, m_bufferLen);
}


void BinaryOutput::writeUInt16(uint16 u) {
    reserveBytes(2);

    uint8* convert = (uint8*)&u;

    if (m_swapBytes) {
        m_buffer[m_pos]     = convert[1];
        m_buffer[m_pos + 1] = convert[0];
    } else {
        *(uint16*)(m_buffer + m_pos) = u;
    }

    m_pos += 2;
}


void BinaryOutput::writeUInt32(uint32 u) {
    reserveBytes(4);

    uint8* convert = (uint8*)&u;

    debugAssert(m_beginEndBits == 0);

    if (m_swapBytes) {
        m_buffer[m_pos]     = convert[3];
        m_buffer[m_pos + 1] = convert[2];
        m_buffer[m_pos + 2] = convert[1];
        m_buffer[m_pos + 3] = convert[0];
    } else {
        *(uint32*)(m_buffer + m_pos) = u;
    }

    m_pos += 4;
}


void BinaryOutput::writeUInt64(uint64 u) {
    reserveBytes(8);

    uint8* convert = (uint8*)&u;

    if (m_swapBytes) {
        m_buffer[m_pos]     = convert[7];
        m_buffer[m_pos + 1] = convert[6];
        m_buffer[m_pos + 2] = convert[5];
        m_buffer[m_pos + 3] = convert[4];
        m_buffer[m_pos + 4] = convert[3];
        m_buffer[m_pos + 5] = convert[2];
        m_buffer[m_pos + 6] = convert[1];
        m_buffer[m_pos + 7] = convert[0];
    } else {
        *(uint64*)(m_buffer + m_pos) = u;
    }

    m_pos += 8;
}


void BinaryOutput::writeString(const char* s) {
    // +1 is because strlen doesn't count the null
    int len = strlen(s) + 1;

    debugAssert(m_beginEndBits == 0);
    reserveBytes(len);
    System::memcpy(m_buffer + m_pos, s, len);
    m_pos += len;
}


void BinaryOutput::writeStringEven(const char* s) {
    // +1 is because strlen doesn't count the null
    int len = strlen(s) + 1;

    reserveBytes(len);
    System::memcpy(m_buffer + m_pos, s, len);
    m_pos += len;

    // Pad with another NULL
    if ((len % 2) == 1) {
        writeUInt8(0);
    }
}


void BinaryOutput::writeString32(const char* s) {
    writeUInt32(strlen(s) + 1);
    writeString(s);
}


void BinaryOutput::writeVector4(const Vector4& v) {
    writeFloat32(v.x);
    writeFloat32(v.y);
    writeFloat32(v.z);
    writeFloat32(v.w);
}


void BinaryOutput::writeVector3(const Vector3& v) {
    writeFloat32(v.x);
    writeFloat32(v.y);
    writeFloat32(v.z);
}


void BinaryOutput::writeVector2(const Vector2& v) {
    writeFloat32(v.x);
    writeFloat32(v.y);
}


void BinaryOutput::writeColor4(const Color4& v) {
    writeFloat32(v.r);
    writeFloat32(v.g);
    writeFloat32(v.b);
    writeFloat32(v.a);
}


void BinaryOutput::writeColor3(const Color3& v) {
    writeFloat32(v.r);
    writeFloat32(v.g);
    writeFloat32(v.b);
}


void BinaryOutput::beginBits() {
    debugAssertM(m_beginEndBits == 0, "Already in beginBits...endBits");
    m_bitString = 0x00;
    m_bitPos = 0;
    m_beginEndBits = 1;
}


void BinaryOutput::writeBits(uint32 value, int numBits) {

    while (numBits > 0) {
        // Extract the current bit of value and
        // insert it into the current byte
        m_bitString |= (value & 1) << m_bitPos;
        ++m_bitPos;
        value = value >> 1;
        --numBits;

        if (m_bitPos > 7) {
            // We've reached the end of this byte
            writeUInt8(m_bitString);
            m_bitString = 0x00;
            m_bitPos = 0;
        }
    }
}


void BinaryOutput::endBits() {
    debugAssertM(m_beginEndBits == 1, "Not in beginBits...endBits");
    if (m_bitPos > 0) {
        writeUInt8(m_bitString);
    }
    m_bitString = 0;
    m_bitPos = 0;
    m_beginEndBits = 0;
}

}
