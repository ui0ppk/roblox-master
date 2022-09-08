/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#include "streaming.h"
#include "stringcompressor.h"
#include "Util/BinaryString.h"
#include "Util/BrickColor.h"
#include "Util/UDim.h"
#include "Util/Faces.h"
#include "Util/Axes.h"
#include "Util/Quaternion.h"
#include "Util/SystemAddress.h"
#include "GuidRegistryService.h"
#include "Util/Math.h"
#include "Util/NormalId.h"
#include "Reflection/Event.h"
#include "Reflection/EnumConverter.h"
#include "util/StreamRegion.h"
#include <boost/algorithm/string.hpp>
#include "Replicator.h"
#include "util/VarInt.h"
#include "util/PhysicalProperties.h"

#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/NumberRange.h"
#include "v8datamodel/ColorSequence.h"

//#define LOSSY_QUAT
// with this defined we lose precision when compressing quaternions for streaming.
// This makes places load incorrectly in multiplayer.

DYNAMIC_FASTINTVARIABLE(PhysicsCompressionSizeFilter, 50)

SYNCHRONIZED_FASTFLAGVARIABLE(NetworkAlignBinaryString, true) // 223

SYNCHRONIZED_FASTFLAGVARIABLE(NetworkDisableStringCompression, false)

#define MAX_STRING_SIZE 200000

namespace RBX {
	
using namespace Reflection;

namespace Network {

void serializeEnumIndex(const Reflection::EnumDescriptor* enumDesc, const size_t& index, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
    RBXASSERT(index < enumDesc->getEnumCount());
    if (enumSizeMSB == 0)
    {
        enumSizeMSB = enumDesc->getEnumCountMSB();
    }
    bitStream.WriteBits((const unsigned char*) &index, enumSizeMSB+1);
}

void deserializeEnumIndex(const Reflection::EnumDescriptor* enumDesc, size_t& index, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
    if (enumSizeMSB == 0)
    {
        enumSizeMSB = enumDesc->getEnumCountMSB();
    }

	readFastN( bitStream, index, enumSizeMSB+1 );
	
	if (index >= enumDesc->getEnumCount())
    {
        // overflowed value, set to default
        // This could happen on an outdated client connecting to the latest server where some new values are added to an enum
        StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Enum value overflow on %s, size %d, index %d. Set to 0. (Are you using an outdated client?)", enumDesc->name.c_str(), (int)enumDesc->getEnumCount(), (int)index);
        index = 0;
    }
}

void serializeEnum(const Reflection::EnumDescriptor* enumDesc , const Reflection::Variant& value, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
	const EnumDescriptor::Item* item = enumDesc->lookup(value);
	RBXASSERT(item);
	const size_t valueIndex = item->index;
    serializeEnumIndex(enumDesc, valueIndex, bitStream, enumSizeMSB);
}

void deserializeEnum(const Reflection::EnumDescriptor* enumDesc, Reflection::Variant& result, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
	size_t index = 0;
    deserializeEnumIndex(enumDesc, index, bitStream, enumSizeMSB);
    if(!enumDesc->convertToValue(index,result))
        throw RBX::network_stream_exception("deserializeEnum conversion failed");
}

void serializeEnumProperty(const ConstProperty& property, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
	const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(property.getDescriptor());
	const size_t value = enumDesc.getIndexValue(property.getInstance());
    serializeEnumIndex(&enumDesc.enumDescriptor, value, bitStream, enumSizeMSB);
}

void deserializeEnumProperty(Property& property, RakNet::BitStream &bitStream, size_t enumSizeMSB/*default to 0*/)
{
	const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(property.getDescriptor());
    size_t index = 0;
    deserializeEnumIndex(&enumDesc.enumDescriptor, index, bitStream, enumSizeMSB);
	if (property.getInstance())
		enumDesc.setIndexValue(property.getInstance(), index);
}

void serializeStringCompressed(const std::string& value, RakNet::BitStream& stream)
{
	uint32_t size = static_cast<uint32_t>(value.size());
	if (size > MAX_STRING_SIZE)
		throw RBX::network_stream_exception(RBX::format("BitStream string write: String too long: %u", size));
        
	stream.Write(size);
    RakNet::StringCompressor::Instance()->EncodeString(value.c_str(), static_cast<int>(size+1), &stream);
}

void deserializeStringCompressed(std::string& value, RakNet::BitStream& stream)
{
	uint32_t size;
	Network::readFastT( stream, size );

	if (size>MAX_STRING_SIZE)
		throw RBX::network_stream_exception(RBX::format("BitStream >> std::string: Bad string length: %d, bit pos: %d", (int)size, stream.GetReadOffset()));

	char* buffer = (char*)alloca(size+1);
	RakNet::StringCompressor::Instance()->DecodeString(buffer, static_cast<int>(size+1), &stream);
	value = buffer;
}

} // namespace Network

RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::Guid::Scope& value)
{
	Network::serializeGuidScope( stream, value, false );
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, RBX::Guid::Scope& value)
{
	Network::deserializeGuidScope( stream, value, false );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, int value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, int& value)
{
	Network::readFastT( stream, value );
    return stream;
}


RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned int value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, unsigned int& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned long long value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, unsigned long long& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, char value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, char& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, signed char value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, signed char& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned char value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, unsigned char& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, short value)
{
	stream.Write(value);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned short value)
{
    stream.Write(value);
    return stream;
}


template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, short& value)
{
	Network::readFastT( stream, value );
	return stream;
}


template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, unsigned short& value)
{
	Network::readFastT( stream, value );
	return stream;
}


RakNet::BitStream& operator << (RakNet::BitStream& stream, bool value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, bool& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, float value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, float& value)
{
	Network::readFastT( stream, value );
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, double value)
{
	stream.Write(value);
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, double& value)
{
	Network::readFastT( stream, value );
	return stream;
}

// This one is expensive in terms of CPU
// It uses huffman coding based on empirical alphabet frequency
// So please avoid using it unless the string worth the compression
// TODO: refactor to explicit function to avoid abusing
RakNet::BitStream& operator << (RakNet::BitStream& stream, const std::string& value)
{
	uint32_t size = static_cast<uint32_t>(value.size());
	if (size > MAX_STRING_SIZE)
		throw RBX::network_stream_exception(RBX::format("BitStream string write: String too long: %u", size));
        
	stream.Write(size);

    if (SFFlag::getNetworkDisableStringCompression())
    {
        stream.Write(value.c_str(), value.size());
    }
    else
    {
    	RakNet::StringCompressor::Instance()->EncodeString(value.c_str(), static_cast<int>(size+1), &stream);
    }

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, std::string& value)
{
	uint32_t size;
	Network::readFastT( stream, size );

	if (size>MAX_STRING_SIZE)
		throw RBX::network_stream_exception(RBX::format("BitStream >> std::string: Bad string length: %d, bit pos: %d", (int)size, stream.GetReadOffset()));

    if (SFFlag::getNetworkDisableStringCompression())
    {
        value.resize(size);
        
        if (size)
            stream.Read(&value[0], size);
    }
    else
    {
    	char* buffer = (char*)alloca(size+1);
    	RakNet::StringCompressor::Instance()->DecodeString(buffer, static_cast<int>(size+1), &stream);
    	value = buffer;
    }

	return stream;
}

#define MAX_BINARY_STRING_SIZE 512000

RakNet::BitStream& operator << (RakNet::BitStream& stream, const BinaryString& value)
{
	uint32_t size = static_cast<uint32_t>(value.value().size());
	if (size > MAX_BINARY_STRING_SIZE)
		throw RBX::network_stream_exception(RBX::format("BitStream string write: BinaryString too long: %u", size));

	stream.AlignWriteToByteBoundary();

	stream.Write(size);
    stream.Write(value.value().c_str(), static_cast<int>(size));

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, BinaryString& value)
{
	stream.AlignReadToByteBoundary();

	uint32_t size;
	if (!stream.Read(size))
		throw RBX::network_stream_exception("BitStream >> BinaryString: failed to read length");

	if (size > MAX_BINARY_STRING_SIZE)
		throw RBX::network_stream_exception("BitStream >> BinaryString: Bad string length");

	char* buffer = (char*)alloca(size+1);

    stream.Read(buffer, size);
    value.set(buffer, size);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::ContentId& value)
{
	stream << value.toString();
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, RBX::ContentId& value)
{
	std::string text;
	stream >> text;
	value = ContentId(text);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const BrickColor& value)
{
	// NOTE: This is technically a "lossy" write
	size_t i = value.getClosestPaletteIndex();
	stream.WriteBits((const unsigned char*) &i, BrickColor::paletteSizeMSB);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const UDim& value)
{
	stream.Write(value.scale);
	stream.Write((int)value.offset);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const UDim2& value)
{
	stream << value.x;
	stream << value.y;
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::RbxRay& value)
{
	stream << value.origin();
	stream << value.direction();
	return stream;
}
RakNet::BitStream& operator << (RakNet::BitStream& stream, const Faces& value)
{
	stream.Write(value.normalIdMask);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const Axes& value)
{
	stream.Write(value.axisMask);
	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Color3& value)
{
	RBXASSERT_VERY_FAST(G3D::isFinite(value.r));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.g));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.b));

	stream.Write(value.r);
	stream.Write(value.g);
	stream.Write(value.b);
	return stream;
}

namespace Network {

const float brickEpsilon = 0.0005f;

inline bool brickEq(float a, float b) {
    return (a == b) || (fabs(a - b) <= brickEpsilon);
}

// Bricks tend to snap to 0.5, 0.1, 0.5 increments. Special-case these values
// by sending them as 11-bit integers
static bool isBrickLocation(const G3D::Vector3& v, short& x, unsigned short& y, short& z)
{
	// Limit range to a 11x11x11 bit box around the origin, with y>=0
	if (v.x>=512.0f)
		return false;
	if (v.x<=-512.0f)
		return false;
	if (v.z>=512.0f)
		return false;
	if (v.z<=-512.0f)
		return false;
	if (v.y>=204.8f)
		return false;
	if (v.y<0)
		return false;

	// Now convert the components to integers, checking each time to confirm it conforms
	float dx(2*v.x);
	x = short(dx);
	if (float(x)!=dx)	// exact compare is OK
		return false;

	float dz(2*v.z);
	z = short(dz);
	if (float(z)!=dz)	// exact compare is OK
		return false;

	float dy(10*v.y);
	y = (unsigned short)dy;
	if (!brickEq(y, dy))	// fuzzy compare because of round-off error
		return false;

	return true;
}

void writeBrickVector(RakNet::BitStream& stream, const G3D::Vector3& value)
{
	RBXASSERT_VERY_FAST(G3D::isFinite(value.x));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.y));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.z));

	short x;
	unsigned short y;
	short z;
	if (isBrickLocation(value, x, y, z))
	{
		stream << true;
		stream.WriteBits((const unsigned char*)&x, 11);
		stream.WriteBits((const unsigned char*)&y, 11);
		stream.WriteBits((const unsigned char*)&z, 11);
	}
	else
	{
		stream << false;
		stream << value.x;
		stream << value.y;
		stream << value.z;
	}
}

void readBrickVector(RakNet::BitStream& stream, G3D::Vector3& value)
{
	bool isBrickLocation;
	stream >> isBrickLocation;
	if (isBrickLocation)
	{
		short x = 0;
		unsigned short y = 0;
		short z = 0;

		Network::readFastN<11>(stream, x);
		Network::readFastN<11>(stream, y);
		Network::readFastN<11>(stream, z);

		// Fill in the sign bits:
		if (x & 0x0400)	x |= 0xFC00;
		if (z & 0x0400) z |= 0xFC00;

		value.x = float(x) / 2;
		value.y = float(y) / 10;
		value.z = float(z) / 2;
	}
	else
	{
		stream >> value.x;
		stream >> value.y;
		stream >> value.z;
	}

	RBXASSERT_VERY_FAST(G3D::isFinite(value.x));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.y));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.z));
}
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector2& value)
{
	RBXASSERT_FISHING(G3D::isFinite(value.x));
	RBXASSERT_FISHING(G3D::isFinite(value.y));

	stream << value.x;
	stream << value.y;

	return stream;
}


template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::Vector2& value)
{
	stream >> value.x;
	stream >> value.y;

	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const StreamRegion::Id& value)
{
	const Vector3int32 &vv = value.value();

    if (vv.x <= 127 && vv.y <= 127 && vv.z <= 127 &&
        vv.x >= -128 && vv.y >= -128 && vv.z >= -128)
    {
        stream << false; // small int, use 3 bytes
        stream << (char)vv.x;
        stream << (char)vv.y;
        stream << (char)vv.z;
    }
    else
    {
        stream << true; // large int, use 12 bytes
        stream << vv.x;
        stream << vv.y;
        stream << vv.z;
    }

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, StreamRegion::Id& value)
{
	Vector3int32 vv;

    bool largeInt;
    stream >> largeInt;
    if (largeInt)
    {
	    stream >> vv.x;
	    stream >> vv.y;
	    stream >> vv.z;
    }
    else
    {
        char x, y, z;
        stream >> x;
        stream >> y;
        stream >> z;
        vv.x = x;
        vv.y = y;
        vv.z = z;
    }

	value = StreamRegion::Id(vv);

	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector3& value)
{
	RBXASSERT_FISHING(G3D::isFinite(value.x));
	RBXASSERT_FISHING(G3D::isFinite(value.y));
	RBXASSERT_FISHING(G3D::isFinite(value.z));

	stream << value.x;
	stream << value.y;
	stream << value.z;

	return stream;
}


template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::Vector3& value)
{
	stream >> value.x;
	stream >> value.y;
	stream >> value.z;

	return stream;
}


RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector3int16& value)
{
	stream << value.x;
	stream << value.y;
	stream << value.z;

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::Vector3int16& value)
{
	stream >> value.x;
	stream >> value.y;
	stream >> value.z;

	return stream;
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector2int16& value)
{
	stream << value.x;
	stream << value.y;

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::Vector2int16& value)
{
	stream >> value.x;
	stream >> value.y;

	return stream;
}



RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::Velocity& value)
{
	stream << value.linear;
	stream << value.rotational;
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, RBX::Velocity& value)
{
	stream >> value.linear;
	stream >> value.rotational;
	return stream;
}


namespace Network {
void rationalize(G3D::CoordinateFrame& value)
{
	if (!value.translation.isFinite())
		value.translation = G3D::Vector3(0, -1e6, 0);
	else
		value.translation = G3D::clamp(G3D::Vector3(-1e6, -1e6, -1e6), value.translation, G3D::Vector3(1e6, 1e6, 1e6));
}

const int orientationBits = 6;
BOOST_STATIC_ASSERT((2 << (orientationBits-1)) > Math::maxOrientationId);
BOOST_STATIC_ASSERT(0 <= Math::minOrientationId);
}

RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::CoordinateFrame& cf)
{
	// TODO: Get rid of this hack when we figure out why the values are bad:
	G3D::CoordinateFrame value = cf;
	Network::rationalize(value);

	Network::writeBrickVector(stream, value.translation);

	const bool isAxisAligned = Math::isAxisAligned(value.rotation);
	stream << isAxisAligned;

	if (isAxisAligned) {
		const int orientId = Math::getOrientId(value.rotation);
		stream.WriteBits((const unsigned char*)&orientId, Network::orientationBits);
	}
	else 
	{
		Quaternion q(value.rotation);

		RBXASSERT_VERY_FAST(G3D::isFinite(q.w));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.x));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.y));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.z));

#ifdef LOSSY_QUAT
		stream.WriteNormQuat(q.w, q.x, q.y, q.z);
#else
		// Orientation quaternions are unit quaternions, so max and min are 1 and -1.
		// WriteNormQuat (if using LOSSY_QUAT) uses 6 bytes + 4 bits
		// Straight streaming of 4 floats uses 16 bytes
		// Handled here with 4 WriteFloat16 calls which uses 8 bytes
		stream.WriteFloat16(q.w, -1.0f, 1.0f);
		stream.WriteFloat16(q.x, -1.0f, 1.0f);
		stream.WriteFloat16(q.y, -1.0f, 1.0f);
		stream.WriteFloat16(q.z, -1.0f, 1.0f);
#endif
	}

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::CoordinateFrame& value)
{
	Network::readBrickVector(stream, value.translation);

	bool isAxisAligned;
	stream >> isAxisAligned;
	if (isAxisAligned)
	{
		int orientId = 0;
		Network::readFastN<Network::orientationBits>( stream, orientId );

		Math::idToMatrix3(orientId, value.rotation);
	}
	else 
	{
		Quaternion q;
#ifdef LOSSY_QUAT
		if (!stream.ReadNormQuat(q.w, q.x, q.y, q.z))
			throw RBX::network_stream_exception("BitStream >> CoordinateFrame ReadNormQuat failed");
#else
		// Orientation quaternions are unit quaternions, so max and min are 1 and -1.
		// WriteNormQuat (if using LOSSY_QUAT) uses 6 bytes + 4 bits
		// Straight streaming of 4 floats uses 16 bytes
		// Handled here with 4 WriteFloat16 calls which uses 8 bytes
		stream.ReadFloat16(q.w, -1.0f, 1.0f);
		stream.ReadFloat16(q.x, -1.0f, 1.0f);
		stream.ReadFloat16(q.y, -1.0f, 1.0f);
		stream.ReadFloat16(q.z, -1.0f, 1.0f);
#endif
		RBXASSERT_VERY_FAST(G3D::isFinite(q.w));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.x));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.y));
		RBXASSERT_VERY_FAST(G3D::isFinite(q.z));

		q.toRotationMatrix(value.rotation);
		Math::orthonormalizeIfNecessary(value.rotation);
	}
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, UDim& value)
{
	int offset;
	stream >> value.scale;
	stream >> offset;
	value.offset = offset;
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, UDim2& value)
{
	stream >> value.x;
	stream >> value.y;
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, RbxRay& value)
{
	stream >> value.origin();
	stream >> value.direction();
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, Faces& value)
{
	stream >> value.normalIdMask;
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, Axes& value)
{
	stream >> value.axisMask;
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, BrickColor& value)
{
	size_t i = 0;
	Network::readFastN<BrickColor::paletteSizeMSB>( stream, i );

	value = BrickColor::colorPalette()[i];
	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, G3D::Color3& value)
{
	stream >> value.r;
	stream >> value.g;
	stream >> value.b;

	RBXASSERT_VERY_FAST(G3D::isFinite(value.r));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.g));
	RBXASSERT_VERY_FAST(G3D::isFinite(value.b));

	return stream;
}


RakNet::BitStream& operator << (RakNet::BitStream& stream, RBX::SystemAddress value)
{
	stream << value.binaryAddress;
	stream.Write(value.port);

	return stream;
}

template<>
RakNet::BitStream& operator >> (RakNet::BitStream& stream, RBX::SystemAddress& value)
{
	stream >> value.binaryAddress;
	stream >> value.port;

	return stream;
}

//////////////////////////////////////////////////////////////////////////


RakNet::BitStream& operator<<( RakNet::BitStream& stream, const NumberSequenceKeypoint& p )
{
    return stream << p.time << p.value << p.envelope;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, NumberSequenceKeypoint& p )
{
    return stream >> p.time >> p.value >> p.envelope;
}

RakNet::BitStream& operator<<( RakNet::BitStream& stream, const ColorSequenceKeypoint& p )
{
    return stream << p.time << p.value << p.envelope;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, ColorSequenceKeypoint& p )
{
    return stream >> p.time >> p.value >> p.envelope;
}


RakNet::BitStream& operator<<( RakNet::BitStream& stream, const NumberSequence& ns )
{
    const std::vector<NumberSequence::Key>& keys = ns.getPoints();
    stream<<uint32_t(keys.size());
    for (int j=0, e=keys.size(); j<e; ++j )
    {
        stream<<keys[j];
    }
    return stream;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, NumberSequence& ns )
{
    uint32_t size;
    stream>>size;
    if( size > NumberSequence::kMaxSize )
        throw network_stream_exception("Number sequence is too big");
    
    std::vector<NumberSequence::Key> keys(size);
    for (unsigned j=0; j<size; ++j)
    {
        stream>>keys[j];
    }
    ns = keys;
    return stream;
}

RakNet::BitStream& operator<<( RakNet::BitStream& stream, const ColorSequence& ns )
{
    const std::vector<ColorSequence::Key>& keys = ns.getPoints();
    stream<<uint32_t(keys.size());
    for (int j=0, e=keys.size(); j<e; ++j )
    {
        stream<<keys[j];
    }
    return stream;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, ColorSequence& ns )
{
    uint32_t size;
    stream>>size;
    if( size > ColorSequence::kMaxSize )
        throw network_stream_exception("Number sequence is too big");

    std::vector<ColorSequence::Key> keys(size);
    for (unsigned j=0; j<size; ++j)
    {
        stream>>keys[j];
    }
    ns = keys;
    return stream;
}

RakNet::BitStream& operator<<( RakNet::BitStream& stream, const NumberRange& r )
{
    return stream << r.min << r.max;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, NumberRange& r )
{
    return stream >> r.min >> r.max;
}

RakNet::BitStream& operator<<( RakNet::BitStream& stream, const Rect2D& r )
{
	return stream << r.x0y0() << r.x1y1();
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, Rect2D& r )
{
	Vector2 x0y0;
	Vector2 x1y1;
	stream >> x0y0;
	stream >> x1y1;
	r = Rect2D::xyxy(x0y0,x1y1);

	return stream;
}

RakNet::BitStream& operator<<( RakNet::BitStream& stream, const PhysicalProperties& p)
{
	bool customEnabled = p.getCustomEnabled();
	stream << customEnabled;

	if (customEnabled)
		stream << p.getDensity() << p.getFriction() << p.getElasticity() << p.getFrictionWeight() << p.getElasticityWeight();
		
	return stream;
}

template<>
RakNet::BitStream& operator>>( RakNet::BitStream& stream, PhysicalProperties& p)
{
	bool customEnabled;
	float density;
	float friction;
	float elasticity;
	float frictionWeight;
	float elasticityWeight;

	stream >> customEnabled;
	if (customEnabled)
	{
		stream >> density;
		stream >> friction;
		stream >> elasticity;
		stream >> frictionWeight;
		stream >> elasticityWeight;
		p = PhysicalProperties(density, friction, elasticity, frictionWeight, elasticityWeight);
	}
	else
	{
		p = PhysicalProperties();
	}

	return stream;
}

//////////////////////////////////////////////////////////////////////////


namespace Network {

template<>
void serialize<RBX::ContentId>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getStringValue();
}


template<>
void serialize<UDim>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<UDim>();
}


template<>
void deserialize<UDim>(Property& property, RakNet::BitStream &bitStream)
{
	UDim c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void serialize<UDim2>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<UDim2>();
}

template<>
void deserialize<UDim2>(Property& property, RakNet::BitStream &bitStream)
{
	UDim2 c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void serialize<RBX::RbxRay>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<RBX::RbxRay>();
}

template<>
void deserialize<RBX::RbxRay>(Property& property, RakNet::BitStream &bitStream)
{
	RbxRay c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void serialize<Faces>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<Faces>();
}


template<>
void deserialize<Faces>(Property& property, RakNet::BitStream &bitStream)
{
	Faces c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void serialize<Axes>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<Axes>();
}


template<>
void deserialize<Axes>(Property& property, RakNet::BitStream &bitStream)
{
	Axes c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void serialize<BrickColor>(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getValue<BrickColor>();
}

template<>
void deserialize<BrickColor>(Property& property, RakNet::BitStream &bitStream)
{
	BrickColor c;
	bitStream >> c;
	if (property.getInstance())
		property.setValue(c);
}

template<>
void deserialize<RBX::ContentId>(Property& property, RakNet::BitStream &bitStream)
{
	std::string value;
	bitStream >> value;
	if (property.getInstance())
		property.setStringValue(value);
}

void serializeStringProperty(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream)
{
	bitStream << property.getDescriptor().getStringValue(property.getInstance());
}

void deserializeStringProperty(Reflection::Property& property, RakNet::BitStream &bitStream)
{
	std::string value;
	bitStream >> value;

	if (property.getInstance())
	{
		const Reflection::PropertyDescriptor& desc = property.getDescriptor();
		desc.setStringValue(property.getInstance(), value);
	}
}

void serializeGuidScope(RakNet::BitStream& stream, const RBX::Guid::Scope& value, bool canDisableCompression)
{
	if (canDisableCompression) {
		RakNet::RakString scope = value.getName()->c_str();
		stream.Write(scope);
	} else {
		stream << value.getName()->toString();
	}
}

void deserializeGuidScope(RakNet::BitStream& stream, RBX::Guid::Scope& value, bool canDisableCompression)
{
	std::string str;
	if (canDisableCompression) {
		RakNet::RakString scope;
		stream.Read(scope);
		str = scope.C_String();
	} else {
		stream >> str;
	}
	value.set(str);
}

IdSerializer::IdSerializer()
{
}

bool IdSerializer::trySerializeId(RakNet::BitStream& stream, const Instance* instance)
{
	if (instance)
	{
		guidRegistry->registerGuid(instance);
		RBX::Guid::Data id;
		instance->getGuid().extract(id);
		if (!scopeNames.trySend(stream, id.scope))
			return false;
		stream.WriteBits((const unsigned char*) &id.index, 32);
		return true;
	}
	else
	{
		serializeId(stream, NULL);
		return true;
	}
}

bool IdSerializer::canSerializeId(const Instance* instance)
{
	if (instance)
	{
		// check if value is in dictionary
		guidRegistry->registerGuid(instance);
		RBX::Guid::Data id;
		instance->getGuid().extract(id);
		return scopeNames.canSend(id.scope);
	}

	return false;
}

void IdSerializer::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	guidRegistry.reset();
	Super::onServiceProvider(oldProvider, newProvider);
	if (newProvider)
		guidRegistry = ServiceProvider::create<GuidRegistryService>(newProvider)->registry;
}


IdSerializer::Id IdSerializer::extractId(const Instance* instance)
{
	IdSerializer::Id result;
	if (instance)
	{
		guidRegistry->registerGuid(instance);
		instance->getGuid().extract(result.id);
		result.valid = true;
	}
	else{
		result.valid = false;
	}
	return result;
}

void IdSerializer::sendId(RakNet::BitStream& stream, const Id& id)
{
	if(id.valid){
		scopeNames.send(stream, id.id.scope);
		stream.WriteBits((const unsigned char*) &id.id.index, 32);
	}
	else{
		scopeNames.sendEmptyItem(stream);
	}
}

void IdSerializer::serializeId(RakNet::BitStream& stream, const Instance* instance)
{
	if (instance)
	{
		guidRegistry->registerGuid(instance);
		RBX::Guid::Data id;
		instance->getGuid().extract(id);
		serializeId(stream, id);
	}
	else
	{
		scopeNames.sendEmptyItem(stream);
	}
}

void IdSerializer::serializeId(RakNet::BitStream& stream, const RBX::Guid::Data& id) {
	scopeNames.send(stream, id.scope);
	stream.WriteBits((const unsigned char*) &id.index, 32);
}

void IdSerializer::serializeIdWithoutDictionary(RakNet::BitStream& stream, const Instance* instance)
{
	RBX::Guid::Data id;

	if (instance)
	{
		guidRegistry->registerGuid(instance);
		instance->getGuid().extract(id);
	}
	
	serializeIdWithoutDictionary(stream, id);
}

void IdSerializer::serializeIdWithoutDictionary(RakNet::BitStream& stream, const RBX::Guid::Data& id)
{
	if (id.scope.isNull())
	{
		unsigned char code = 0;
		stream << code;
	}
	else
	{
		if (id.scope == serverScope)
		{
			unsigned char code = 255;
			stream << code;
		}
		else
		{
			const std::string& scope = id.scope.getName()->toString();
			RBXASSERT(scope.size() < 255);

			unsigned char code = scope.size();
			stream << code;
			
			stream.WriteBits(reinterpret_cast<const unsigned char*>(scope.c_str()), code * 8);
		}

		stream.WriteBits((const unsigned char*) &id.index, 32);
	}
}

void IdSerializer::deserializeId(RakNet::BitStream& stream, RBX::Guid::Data& id)
{
	scopeNames.receive(stream, id.scope);
	if (!id.scope.isNull())
	{
		id.index = 0;
		// This version does endian swapping.
		Network::readFastN<32>( stream, id.index );
	}
	else
		id.index = 0;
}

void IdSerializer::deserializeIdWithoutDictionary(RakNet::BitStream& stream, RBX::Guid::Data& id)
{
	unsigned char code = 0;
	Network::readFastT(stream, code);

	if (code == 0)
	{
		id.scope.setNull();
		id.index = 0;
	}
	else
	{
		if (code == 255)
		{
			RBXASSERT(!serverScope.isNull());
			id.scope = serverScope;
		}
		else
		{
			char buffer[256];
			stream.ReadBits(reinterpret_cast<unsigned char*>(buffer), code * 8);
			buffer[code] = 0;
			id.scope.set(buffer);
		}

		Network::readFastN<32>(stream, id.index);
	}
}

void IdSerializer::setRefValue(WaitItem& wi, Instance* instance)
{
	wi.desc->setRefValue(wi.instance.get(), instance);
}


void IdSerializer::resolvePendingReferences(Instance* instance, RBX::Guid::Data id)
{
	boost::mutex::scoped_lock lock(waitItemsMutex);
	WaitItemMap::iterator iter = waitItems.find(id);
	if (iter!=waitItems.end())
	{
		std::for_each(
			iter->second.begin(), 
			iter->second.end(), 
			boost::bind(&IdSerializer::setRefValue, this, _1, instance)
		);
		waitItems.erase(iter);
	}
}


void IdSerializer::serializeInstanceRef(const Instance* instance, RakNet::BitStream& bitStream)
{
	serializeId(bitStream, instance);
}

//Debuggable - 
// Parent == NULL, or Parent::Debugable

bool IdSerializer::deserializeInstanceRef(RakNet::BitStream& stream, shared_ptr<Instance>& instance, RBX::Guid::Data& id)
{
	deserializeId(stream, id);
	bool answer = guidRegistry->lookupByGuid(id, instance);
	RBXASSERT(		!instance 
				||	!ServiceProvider::findServiceProvider(instance.get())
				|| (ServiceProvider::findServiceProvider(instance.get()) == ServiceProvider::findServiceProvider(this))
				);

	return answer;
}

void IdSerializer::addPendingRef(const Reflection::RefPropertyDescriptor* desc,
		boost::shared_ptr<Instance> instance, RBX::Guid::Data id) {

	boost::mutex::scoped_lock lock(waitItemsMutex);
	WaitItem item = { desc, instance };
	waitItems[id].push_back(item);
}


template<class T>
void DescriptorSender<T>::visit(const T* desc)
{
	const unsigned int id = descToId.size();
    IdContainer idContainer;
    idContainer.id = id;
    idContainer.outdated = false;
	descToId[desc] = idContainer;
	idBits = Math::computeMSB(descToId.size())+1;
}

template<>
std::string DescriptorSender<ClassDescriptor>::teachName(const ClassDescriptor* t) const
{
	return t->name.toString();
}

template<>
void DescriptorReceiver<ClassDescriptor>::learnName(std::string s, int i, uint32_t checksum)
{
	const RBX::Name& n = RBX::Name::lookup(s);

	ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
	while (iter!=ClassDescriptor::all_end())
	{
		if ((*iter)->name == n)
		{
			idToDesc[i].desc = *iter;
            idToDesc[i].outdated = !verifyChecksum((*iter), checksum);
            *((*iter)->isOutdated) = idToDesc[i].outdated;
            *((*iter)->isReplicable) = true;
			return;
		}
		++iter;
	}
	StandardOut::singleton()->printf(MESSAGE_WARNING, "ClassDescriptor failed to learn %s", s.c_str());
	idToDesc[i].desc = NULL;
    idToDesc[i].outdated = false;
}

template<>
std::string DescriptorSender<EventDescriptor>::teachName(const EventDescriptor* t) const
{
	return t->owner.name.toString() + ":" + t->name.toString();
}

template<>
void DescriptorReceiver<EventDescriptor>::learnName(std::string s, int i, uint32_t checksum)
{
	std::vector<std::string> words;

	boost::split(words, s, boost::is_any_of(":"));

	// First get the class name
	const RBX::Name& n = RBX::Name::lookup(words[0]);

	ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
	while (iter!=ClassDescriptor::all_end())
	{
		const ClassDescriptor* c = *iter;
		if (c->name == n)
		{
			if (EventDescriptor* desc = c->findEventDescriptor(words[1].c_str()))
			{
				idToDesc[i].desc = desc;
                idToDesc[i].outdated = !verifyChecksum(desc, checksum);
                
                if (idToDesc[i].outdated)
                {
                    StandardOut::singleton()->printf(MESSAGE_WARNING, "EventDescriptor %s is out of date, replication will be ignored", s.c_str());
                }

                *(desc->isOutdated) = idToDesc[i].outdated;
                *(desc->isReplicable) = true;
				return;
			}
			else
				break;
		}
		++iter;
	}
	StandardOut::singleton()->printf(MESSAGE_WARNING, "EventDescriptor failed to learn %s", s.c_str());
	idToDesc[i].desc = NULL;
    idToDesc[i].outdated = false;
}

template<>
std::string DescriptorSender<PropertyDescriptor>::teachName(const PropertyDescriptor* t) const
{
	return t->owner.name.toString() + ":" + t->name.toString();
}


template<>
void DescriptorReceiver<PropertyDescriptor>::learnName(std::string s, int i, uint32_t checksum)
{
	std::vector<std::string> words;

	boost::split(words, s, boost::is_any_of(":"));

	// First get the class name
	const RBX::Name& n = RBX::Name::lookup(words[0]);

	ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
	while (iter!=ClassDescriptor::all_end())
	{
		const ClassDescriptor* c = *iter;
		if (c->name == n)
		{
			if (PropertyDescriptor* desc = c->findPropertyDescriptor(words[1].c_str()))
			{
				idToDesc[i].desc = desc;
                idToDesc[i].outdated = !verifyChecksum(desc, checksum);
                *(desc->isOutdated) = idToDesc[i].outdated;
                *(desc->isReplicable) = true;
				return;
			}
			else
				break;
		}
		++iter;
	}

	idToDesc[i].desc = NULL;
    idToDesc[i].outdated = false;
}

template<>
std::string DescriptorSender<Type>::teachName(const Type* t) const
{
	return t->name.toString();
}

template<>
void DescriptorReceiver<Type>::learnName(std::string s, int i, uint32_t checksum)
{
	const RBX::Name& n = RBX::Name::lookup(s);

    const std::vector<const Type*>& types = Type::getAllTypes();

	for (size_t ti = 0; ti < types.size(); ++ti)
	{
		if (types[ti]->name == n)
		{
			idToDesc[i].desc = types[ti];
            idToDesc[i].outdated = !verifyChecksum(types[ti], checksum);
			return;
		}
	}

	StandardOut::singleton()->printf(MESSAGE_WARNING, "Type failed to learn %s", s.c_str());
	idToDesc[i].desc = NULL;
    idToDesc[i].outdated = false;
}

template<>
DescriptorSender<ClassDescriptor>::DescriptorSender()
{
	ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
	ClassDescriptor::ClassDescriptors::const_iterator end = ClassDescriptor::all_end();
	while (iter!=end)
	{
		// TODO: Skip classes that can't be constructed???  (Abstract classes)
		visit(*iter);
		++iter;
	}
}

template<>
DescriptorSender<PropertyDescriptor>::DescriptorSender()
{
	MemberDescriptorContainer<PropertyDescriptor>::Collection::const_iterator iter = MemberDescriptorContainer<PropertyDescriptor>::all_begin();
	MemberDescriptorContainer<PropertyDescriptor>::Collection::const_iterator end = MemberDescriptorContainer<PropertyDescriptor>::all_end();
	while (iter!=end)
	{
		visit(*iter);
		++iter;
	}
}


template<>
DescriptorSender<EventDescriptor>::DescriptorSender()
{
	MemberDescriptorContainer<EventDescriptor>::Collection::const_iterator iter = MemberDescriptorContainer<EventDescriptor>::all_begin();
	MemberDescriptorContainer<EventDescriptor>::Collection::const_iterator end = MemberDescriptorContainer<EventDescriptor>::all_end();
	while (iter!=end)
	{
		visit(*iter);
		++iter;
	}
}

template<>
DescriptorSender<Type>::DescriptorSender()
{
    const std::vector<const Type*>& types = Type::getAllTypes();

	for (size_t i = 0; i < types.size(); ++i)
	{
        visit(types[i]);
    }
}

} 


}
