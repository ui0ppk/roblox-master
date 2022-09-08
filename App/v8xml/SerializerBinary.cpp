#include "stdafx.h"

#include "v8xml/SerializerBinary.h"
#include "v8xml/Serializer.h"

#include <vector>
#include <string>

#include <util/ProtectedString.h>
#include <util/UDim.h>
#include <util/Faces.h>
#include <util/Axes.h>
#include <util/BrickColor.h>
#include <util/Quaternion.h>
#include <util/BinaryString.h>

#include <v8tree/Instance.h>
#include <v8tree/Service.h>

#include <util/Vector3int32.h>
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/NumberRange.h"
#include "util/PhysicalProperties.h"

#include "../lz4/lz4.h"
#include "../lz4/lz4hc.h"

LOGVARIABLE(Serializer, 0)

namespace RBX
{
	static const char kHeaderSignature[] = "\x89\xff\x0d\x0a\x1a\x0a";
	
	static const char kChunkInstances[] = "INST";
	static const char kChunkProperty[] = "PROP";
	static const char kChunkParents[] = "PRNT";
	static const char kChunkEnd[] = "END\0";
	
	struct FileHeader
	{
		char magic[8];
		char signature[6];
		unsigned short version;
		
		unsigned int types;
		unsigned int objects;
		unsigned int reserved[2];
	};
	
    struct ChunkHeader
    {
		char name[4];
		unsigned int compressedSize; // if compressedSize is 0, chunk data is not compressed
		unsigned int size;
		unsigned int reserved;
    };
    
    enum BinaryObjectFormat
    {
        bofPlain,
        bofServiceType
    };

    enum BinaryPropertyFormat
    {
		bpfUnknown = 0,
		bpfString,
        bpfBool,
        bpfInt,
        bpfFloat,
        bpfDouble,
        bpfUDim,
        bpfUDim2,
        bpfRay,
        bpfFaces,
        bpfAxes,
        bpfBrickColor,
        bpfColor3,
        bpfVector2,
        bpfVector3,
        bpfVector2int16,
        bpfCFrameMatrix,
        bpfCFrameQuat,
        bpfEnum,
        bpfRef,
        bpfVector3int16,
        bpfNumberSequence,
        bpfColorSequenceV1,
        bpfNumberRange,
        bpfRect2D,
		bpfPhysicalProperties
    };

    enum BinaryParentLinkFormat
    {
        bplfPlain,
    };

    struct MemoryInputStream
    {
		boost::scoped_array<char> data;
        size_t offset;
        size_t datasize;
        
        MemoryInputStream(): offset(0), datasize(0)
        {
        }
        
        void read(void* value, size_t size)
        {
			if (offset + size > datasize)
				throw RBX::runtime_error("Read offset is out of bounds while reading %d bytes", (int)size);
			
			memcpy(value, &data[offset], size);
			offset += size;
        }
    };

    struct MemoryOutputStream
    {
		boost::scoped_array<char> data;
		size_t datasize;
		size_t capacity;
		
		std::string name;
        
		MemoryOutputStream(const std::string& name): name(name), datasize(0), capacity(0)
		{
		}
		
		void grow(size_t size)
		{
			RBXASSERT(size > capacity);
			
			size_t newcapacity = 16;
			
			while (newcapacity < size)
				newcapacity *= 2;
				
			boost::scoped_array<char> newdata(new char[newcapacity]);
			
			memcpy(newdata.get(), data.get(), datasize);
			
			data.swap(newdata);
			capacity = newcapacity;
		}
        
        void write(char value)
        {
			if (datasize + 1 > capacity) grow(datasize + 1);
			
			data[datasize] = value;
			datasize++;
        }

        void write(const void* value, size_t size)
        {
			if (datasize + size > capacity) grow(datasize + size);
			
			memcpy(data.get() + datasize, value, size);
			datasize += size;
        }
    };
    
	static void readData(std::istream& in, void* data, int size)
	{
		in.read(static_cast<char*>(data), size);

		if (in.gcount() != size)
			throw RBX::runtime_error("Unexpected end of file while reading %d bytes", size);
	}
    
	static ChunkHeader readChunk(std::istream& in, MemoryInputStream& result)
	{
		ChunkHeader header;
		readData(in, &header, sizeof(header));
		
		result.data.reset(new char[header.size]);
		result.datasize = header.size;
		result.offset = 0;
		
		if (header.size)
		{
			if (header.compressedSize == 0)
			{
				readData(in, result.data.get(), header.size);
			}
			else
			{
				std::vector<char> compressed(header.compressedSize);
				readData(in, &compressed[0], compressed.size());
				
				int count = LZ4_decompress_safe(&compressed[0], result.data.get(), compressed.size(), result.datasize);
				
				if (count != result.datasize)
					throw RBX::runtime_error("Malformed data (%d != %d)", count, (int)result.datasize);
			}
		}
		
		return header;
	}
	
	static void writeChunk(std::ostream& out, const MemoryOutputStream& stream, const char* name, unsigned int flags)
	{
		ChunkHeader header;
		strncpy(header.name, name, sizeof(header.name));
		header.compressedSize = 0;
		header.size = stream.datasize;
		header.reserved = 0;
		
		if (flags & SerializerBinary::sfNoCompression)
		{
			out.write(reinterpret_cast<char*>(&header), sizeof(header));
			out.write(stream.data.get(), stream.datasize);
		}
		else
		{
			int maxSize = LZ4_compressBound(stream.datasize);
			
			std::vector<char> compressed(maxSize);
			int compressedSize = (flags & SerializerBinary::sfHighCompression ? LZ4_compressHC : LZ4_compress)(stream.data.get(), &compressed[0], stream.datasize);
			
			header.compressedSize = compressedSize;
			
			out.write(reinterpret_cast<char*>(&header), sizeof(header));
			out.write(&compressed[0], compressedSize);
		}
		
		FASTLOGS(FLog::Serializer, "Stream: %s", stream.name);
		FASTLOG2(FLog::Serializer, "%d -> %d", header.size, header.compressedSize);
	}
	
    template <typename T> static void readRaw(MemoryInputStream& stream, T& value)
    {
        stream.read(&value, sizeof(value));
    }

    template <typename T> static void writeRaw(MemoryOutputStream& stream, const T& value)
    {
        stream.write(&value, sizeof(value));
    }

    static void readString(MemoryInputStream& stream, std::string& value)
    {
		uint32_t length;
		readRaw(stream, length);
		
		value.resize(length);
		
		if (length > 0)
			stream.read(&value[0], length);
    }

    static void writeString(MemoryOutputStream& stream, const std::string& value)
    {
		uint32_t length = value.length();
		
		writeRaw(stream, length);
        stream.write(value.c_str(), length);
    }

    static int encodeInt(int value)
    {
        // sign bit is LSB; abs(value) is in top 31 bits
        return (value << 1) ^ (value >> 31);
    }
    
    static int decodeInt(int value)
    {
        return (static_cast<unsigned int>(value) >> 1) ^ (-(value & 1));
    }

    static int getInstanceId(const std::map<const Instance*, int>& idMap, const Instance* instance)
    {
        std::map<const Instance*, int>::const_iterator it = idMap.find(instance);

        return it == idMap.end() ? -1 : it->second;
    }
    
    static void readIntVector(MemoryInputStream& stream, std::vector<int>& values, size_t count)
    {
		values.clear();
		values.reserve(count);

		if (stream.offset + count * 4 > stream.datasize)
			throw RBX::runtime_error("Read offset is out of bounds while reading %d bytes", (int)count * 4);
		
		for (size_t i = 0; i < count; ++i)
		{
			unsigned char v0 = stream.data[stream.offset + i];
			unsigned char v1 = stream.data[stream.offset + count + i];
			unsigned char v2 = stream.data[stream.offset + count * 2 + i];
			unsigned char v3 = stream.data[stream.offset + count * 3 + i];
			
			values.push_back(decodeInt((v0 << 24) | (v1 << 16) | (v2 << 8) | v3));
		}
		
		stream.offset += count * 4;
    }

    static void writeIntVector(MemoryOutputStream& stream, const std::vector<int>& values)
    {
        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeInt(values[i]) >> 24));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeInt(values[i]) >> 16));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeInt(values[i]) >> 8));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)encodeInt(values[i]));
        }
    }
    
    static void readUIntVector(MemoryInputStream& stream, std::vector<unsigned int>& values, size_t count)
    {
		values.clear();
		values.reserve(count);

		if (stream.offset + count * 4 > stream.datasize)
			throw RBX::runtime_error("Read offset is out of bounds while reading %d bytes", (int)count * 4);
		
		for (size_t i = 0; i < count; ++i)
		{
			unsigned char v0 = stream.data[stream.offset + i];
			unsigned char v1 = stream.data[stream.offset + count + i];
			unsigned char v2 = stream.data[stream.offset + count * 2 + i];
			unsigned char v3 = stream.data[stream.offset + count * 3 + i];
			
			values.push_back((v0 << 24) | (v1 << 16) | (v2 << 8) | v3);
		}
		
		stream.offset += count * 4;
    }

    static void writeUIntVector(MemoryOutputStream& stream, const std::vector<unsigned int>& values)
    {
        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(values[i] >> 24));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(values[i] >> 16));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(values[i] >> 8));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)values[i]);
        }
    }

    static void readIdVector(MemoryInputStream& stream, std::vector<int>& values, size_t count)
    {
		readIntVector(stream, values, count);
		
		int last = 0;
		
		for (size_t i = 0; i < count; ++i)
		{
			values[i] += last;
			last = values[i];
		}
    }

	static void writeIdVector(MemoryOutputStream& stream, const std::vector<int>& values)
	{
		std::vector<int> deltas;
		deltas.reserve(values.size());
		
		int last = 0;
        
        for (size_t i = 0; i < values.size(); ++i)
        {
			deltas.push_back(values[i] - last);
			last = values[i];
        }
        
        writeIntVector(stream, deltas);
	}
	
	union FloatBitcast
	{
		float f;
		unsigned int i;
	};
    
    static unsigned int encodeFloat(float value)
    {
		FloatBitcast bitcast;
		bitcast.f = value;
		
		// move sign bit to the end; that way exponent is in the first byte
        return (bitcast.i << 1) | (bitcast.i >> 31);
    }
    
    static float decodeFloat(unsigned int value)
    {
		FloatBitcast bitcast;
		bitcast.i = (value >> 1) | (value << 31);
		
        return bitcast.f;
    }

	static void readFloatVector(MemoryInputStream& stream, std::vector<float>& values, size_t count)
	{
		values.clear();
		values.reserve(count);
		
		if (stream.offset + count * 4 > stream.datasize)
			throw RBX::runtime_error("Read offset is out of bounds while reading %d bytes", (int)count * 4);
		
		for (size_t i = 0; i < count; ++i)
		{
			unsigned char v0 = stream.data[stream.offset + i];
			unsigned char v1 = stream.data[stream.offset + count + i];
			unsigned char v2 = stream.data[stream.offset + count * 2 + i];
			unsigned char v3 = stream.data[stream.offset + count * 3 + i];
			
			values.push_back(decodeFloat((v0 << 24) | (v1 << 16) | (v2 << 8) | v3));
		}
		
		stream.offset += count * 4;
	}

	static void writeFloatVector(MemoryOutputStream& stream, const std::vector<float>& values)
    {
        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeFloat(values[i]) >> 24));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeFloat(values[i]) >> 16));
        }

        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)(encodeFloat(values[i]) >> 8));
        }
        
        for (size_t i = 0; i < values.size(); ++i)
		{
			writeRaw(stream, (unsigned char)encodeFloat(values[i]));
        }
    }

	static void readCFrameRotation(MemoryInputStream& stream, G3D::Matrix3& transform)
    {
		char orientId;
		readRaw(stream, orientId);
		
		if (orientId)
		{
			Math::idToMatrix3(orientId - 1, transform);
		}
		else
		{
			Quaternion q;
			readRaw(stream, q);
			
			q.toRotationMatrix(transform);
		}
    }

	static void writeCFrameRotation(MemoryOutputStream& stream, const G3D::Matrix3& transform)
    {
		if (Math::isAxisAligned(transform))
		{
			char orientId = Math::getOrientId(transform) + 1;
			
			writeRaw(stream, orientId);
		}
		else
		{
			Quaternion q(transform);
			
			// Normalize output quaternion to avoid rotation drift over time
			q.normalize();
		
			writeRaw(stream, (char)0);
			writeRaw(stream, q);
		}
    }
    
	static void readCFrameRotationExact(MemoryInputStream& stream, G3D::Matrix3& transform)
    {
		char orientId;
		readRaw(stream, orientId);
		
		if (orientId)
		{
			Math::idToMatrix3(orientId - 1, transform);
		}
		else
		{
			readRaw(stream, transform);
		}
    }

	static void writeCFrameRotationExact(MemoryOutputStream& stream, const G3D::Matrix3& transform)
    {
		if (Math::isAxisAligned(transform))
		{
			char orientId = Math::getOrientId(transform) + 1;
			
			writeRaw(stream, orientId);
		}
		else
		{
			writeRaw(stream, (char)0);
			writeRaw(stream, transform);
		}
    }

	static void readPhysicalProperties(MemoryInputStream& stream, PhysicalProperties& physicalProp)
	{
		bool customizeProp;
		readRaw(stream, customizeProp);

		if (customizeProp)
		{
			float density;
			float friction;
			float elasticity;
			float frictionWeight;
			float elasticityWeight;

			readRaw(stream, density);
			readRaw(stream, friction);
			readRaw(stream, elasticity);
			readRaw(stream, frictionWeight);
			readRaw(stream, elasticityWeight);

			physicalProp = PhysicalProperties(	density,
												friction,
												elasticity,
												frictionWeight,
												elasticityWeight);
		}
		else
		{
			physicalProp = PhysicalProperties();
		}
	}

	static void writePhysicalProperties(MemoryOutputStream& stream, const PhysicalProperties& physicalProp)
	{
		bool customizeProp = physicalProp.getCustomEnabled();
		writeRaw(stream, customizeProp);

		if (customizeProp)
		{
			writeRaw(stream, physicalProp.getDensity());
			writeRaw(stream, physicalProp.getFriction());
			writeRaw(stream, physicalProp.getElasticity());
			writeRaw(stream, physicalProp.getFrictionWeight());
			writeRaw(stream, physicalProp.getElasticityWeight());
		}
	}

    static void readNumberSequence(MemoryInputStream& stream, NumberSequence& ns)
    {
        uint32_t size;
        readRaw(stream, size);

        std::vector<NumberSequence::Key> keys;
        keys.reserve(size);

        for (uint32_t i = 0; i < size; ++i)
        {
            NumberSequence::Key k;
            readRaw(stream, k);

            keys.push_back(k);
        }

        ns = keys;
    }

    static void writeNumberSequence(MemoryOutputStream& stream, const NumberSequence& ns)
    {
        const std::vector<NumberSequence::Key>& keys = ns.getPoints();

        uint32_t size = keys.size();
        writeRaw(stream, size);

        for (size_t i = 0; i < size; ++i)
        {
            writeRaw(stream, keys[i]);
        }
    }

    static void readColorSequence(MemoryInputStream& stream, ColorSequence& ns)
    {
        uint32_t size;
        readRaw(stream, size);

        std::vector<ColorSequence::Key> keys;
        keys.reserve(size);

        for (size_t i = 0; i < size; ++i)
        {
            ColorSequence::Key k;
            readRaw(stream, k);

            keys.push_back(k);
        }

        ns = keys;
    }

    static void writeColorSequence(MemoryOutputStream& stream, const ColorSequence& ns)
    {
        const std::vector<ColorSequence::Key>& keys = ns.getPoints();
        
        uint32_t size = keys.size();
        writeRaw(stream, size);

        for (size_t i = 0; i < size; ++i)
        {
            writeRaw(stream, keys[i]);
        }
    }

	template <typename T> static T getPropertyValue(const Reflection::PropertyDescriptor& desc, const Instance* instance)
	{
		return Reflection::ConstProperty(desc, instance).getValue<T>();
	}

	template <typename T> static void setPropertyValue(const Reflection::PropertyDescriptor& desc, Instance* instance, const T& value)
	{
		Reflection::Property(desc, instance).setValue<T>(value);
	}

    void readFormatExpected(MemoryInputStream& stream, BinaryPropertyFormat expected)
    {
        char format;
        readRaw(stream, format);

        if (format != expected)
            throw RBX::runtime_error("Unexpected format %d (expected %d)", format, expected);
    }

	void readPropertyValues(MemoryInputStream& stream, const Reflection::PropertyDescriptor& desc, const std::vector<Instance*>& instances, const std::vector<shared_ptr<Instance> >& idMap)
    {
        if (desc.type == Reflection::Type::singleton<ProtectedString>() || desc.type == Reflection::Type::singleton<std::string>() || desc.type == Reflection::Type::singleton<BinaryString>())
        {
            readFormatExpected(stream, bpfString);
			
			std::string value;

			for (size_t i = 0; i < instances.size(); ++i)
			{
				readString(stream, value);
				desc.setStringValue(instances[i], value);
			}
        }
        else if (desc.type == Reflection::Type::singleton<bool>())
        {
            readFormatExpected(stream, bpfBool);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				bool value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], value);
			}
        }
        else if (desc.type == Reflection::Type::singleton<int>())
        {
            readFormatExpected(stream, bpfInt);
			
			std::vector<int> values;
			readIntVector(stream, values, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], values[i]);
        }
        else if (desc.type == Reflection::Type::singleton<float>())
        {
            readFormatExpected(stream, bpfFloat);
			
			std::vector<float> values;
			readFloatVector(stream, values, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], values[i]);
        }
        else if (desc.type == Reflection::Type::singleton<double>())
        {
            readFormatExpected(stream, bpfDouble);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				double value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], value);
			}
        }
        else if (desc.type == Reflection::Type::singleton<UDim>())
        {
            readFormatExpected(stream, bpfUDim);
            
			std::vector<float> scales;
			readFloatVector(stream, scales, instances.size());
			
			std::vector<int> offsets;
			readIntVector(stream, offsets, instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], UDim(scales[i], offsets[i]));
        }
        else if (desc.type == Reflection::Type::singleton<UDim2>())
        {
            readFormatExpected(stream, bpfUDim2);
            
			std::vector<float> scalesx, scalesy;
			readFloatVector(stream, scalesx, instances.size());
			readFloatVector(stream, scalesy, instances.size());
			
			std::vector<int> offsetsx, offsetsy;
			readIntVector(stream, offsetsx, instances.size());
			readIntVector(stream, offsetsy, instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], UDim2(UDim(scalesx[i], offsetsx[i]), UDim(scalesy[i], offsetsy[i])));
        }
        else if (desc.type == Reflection::Type::singleton<RbxRay>())
        {
            readFormatExpected(stream, bpfRay);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				Vector3 origin, direction;
				readRaw(stream, origin);
				readRaw(stream, direction);
				
				setPropertyValue(desc, instances[i], RbxRay(origin, direction));
			}
        }
        else if (desc.type == Reflection::Type::singleton<Faces>())
        {
            readFormatExpected(stream, bpfFaces);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				char value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], Faces(value));
			}
        }
        else if (desc.type == Reflection::Type::singleton<Axes>())
        {
            readFormatExpected(stream, bpfAxes);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				char value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], Axes(value));
			}
        }
        else if (desc.type == Reflection::Type::singleton<BrickColor>())
        {
            readFormatExpected(stream, bpfBrickColor);
			
			std::vector<unsigned int> values;
			readUIntVector(stream, values, instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], BrickColor(values[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Color3>())
        {
            readFormatExpected(stream, bpfColor3);
            
			std::vector<float> r, g, b;
			readFloatVector(stream, r, instances.size());
			readFloatVector(stream, g, instances.size());
			readFloatVector(stream, b, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], G3D::Color3(r[i], g[i], b[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Rect2D>())
        {
            readFormatExpected(stream, bpfRect2D);
			
			std::vector<float> x0, y0, x1, y1;
			readFloatVector(stream, x0, instances.size());
			readFloatVector(stream, y0, instances.size());
            readFloatVector(stream, x1, instances.size());
			readFloatVector(stream, y1, instances.size());
            
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], G3D::Rect2D::xyxy(x0[i], y0[i], x1[i], y1[i]));
        }
		else if (desc.type == Reflection::Type::singleton<PhysicalProperties>())
		{
			readFormatExpected(stream, bpfPhysicalProperties);

			for (size_t i = 0; i < instances.size(); ++i)
			{
				PhysicalProperties instancePhysProp;
				readPhysicalProperties(stream, instancePhysProp);

				setPropertyValue(desc, instances[i], instancePhysProp);
			}
		}
        else if (desc.type == Reflection::Type::singleton<G3D::Vector2>())
        {
            readFormatExpected(stream, bpfVector2);
			
			std::vector<float> x, y;
			readFloatVector(stream, x, instances.size());
			readFloatVector(stream, y, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], G3D::Vector2(x[i], y[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector3>())
        {
            readFormatExpected(stream, bpfVector3);
			
			std::vector<float> x, y, z;
			readFloatVector(stream, x, instances.size());
			readFloatVector(stream, y, instances.size());
			readFloatVector(stream, z, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				setPropertyValue(desc, instances[i], G3D::Vector3(x[i], y[i], z[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector2int16>())
        {
            readFormatExpected(stream, bpfVector2int16);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Vector2int16 value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], value);
			}
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector3int16>())
        {
            readFormatExpected(stream, bpfVector3int16);
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Vector3int16 value;
				readRaw(stream, value);
				setPropertyValue(desc, instances[i], value);
			}
        }
        else if (desc.type == Reflection::Type::singleton<G3D::CoordinateFrame>())
        {
			std::vector<G3D::Matrix3> rot;
			std::vector<float> tx;
			std::vector<float> ty;
			std::vector<float> tz;
			
			rot.resize(instances.size());

            char format;
            readRaw(stream, format);

            if (format != bpfCFrameMatrix && format != bpfCFrameQuat)
                throw RBX::runtime_error("Unexpected cframe format %d", format);
			
			if (format == bpfCFrameMatrix)
			{
				for (size_t i = 0; i < instances.size(); ++i)
					readCFrameRotationExact(stream, rot[i]);
			}
			else
			{
				for (size_t i = 0; i < instances.size(); ++i)
					readCFrameRotation(stream, rot[i]);
			}
				
			readFloatVector(stream, tx, instances.size());
			readFloatVector(stream, ty, instances.size());
			readFloatVector(stream, tz, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::CoordinateFrame cframe(rot[i], Vector3(tx[i], ty[i], tz[i]));
				
				setPropertyValue(desc, instances[i], cframe);
			}
        }
        else if (desc.bIsEnum)
        {
			const Reflection::EnumPropertyDescriptor& enumDesc = static_cast<const Reflection::EnumPropertyDescriptor&>(desc);
			
            readFormatExpected(stream, bpfEnum);

			std::vector<unsigned int> values;
			readUIntVector(stream, values, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				enumDesc.setEnumValue(instances[i], values[i]);
			}
        }
        else if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(desc))
        {
            const Reflection::RefPropertyDescriptor& refDesc = static_cast<const Reflection::RefPropertyDescriptor&>(desc);

            readFormatExpected(stream, bpfRef);

			std::vector<int> values;
			readIdVector(stream, values, instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				if (values[i] != -1 && static_cast<unsigned int>(values[i]) >= idMap.size())
					throw RBX::runtime_error("Invalid id %d", values[i]);
					
				Instance* ref = values[i] != -1 ? idMap[values[i]].get() : NULL;
				
				refDesc.setRefValueUnsafe(instances[i], ref);
			}
        }
        else if (desc.type == Reflection::Type::singleton<ContentId>())
        {
            readFormatExpected(stream, bpfString);

			std::string value;
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				readString(stream, value);
				setPropertyValue(desc, instances[i], ContentId(value));
			}
		}
        else if (desc.type == Reflection::Type::singleton<NumberSequence>())
        {
            readFormatExpected(stream, bpfNumberSequence);

            NumberSequence ns;
            for (size_t i=0; i<instances.size(); ++i)
            {
                readNumberSequence(stream, ns);
                setPropertyValue(desc, instances[i], ns);
            }
        }
        else if (desc.type == Reflection::Type::singleton<ColorSequence>())
        {
            readFormatExpected(stream, bpfColorSequenceV1);

            ColorSequence cs;
            for (size_t i=0; i<instances.size(); ++i )
            {
                readColorSequence(stream, cs);
                setPropertyValue(desc, instances[i], cs);
            }
        }
        else if (desc.type == Reflection::Type::singleton<NumberRange>())
        {
            readFormatExpected(stream, bpfNumberRange );
            NumberRange r;

            for (size_t i=0; i<instances.size(); ++i )
            {
                readRaw(stream, r);
                setPropertyValue(desc, instances[i], r);
            }
        }
        else
        {
			throw RBX::runtime_error("Unknown property type for property %s", desc.name.c_str());
        }
    }

	void writePropertyValues(MemoryOutputStream& stream, const Reflection::PropertyDescriptor& desc, const std::vector<const Instance*>& instances, const std::map<const Instance*, int>& idMap, unsigned int flags)
    {
        if (desc.type == Reflection::Type::singleton<ProtectedString>() || desc.type == Reflection::Type::singleton<std::string>() || desc.type == Reflection::Type::singleton<BinaryString>())
        {
            writeRaw(stream, (char)bpfString);
            
			for (size_t i = 0; i < instances.size(); ++i)
				writeString(stream, desc.getStringValue(instances[i]));
        }
        else if (desc.type == Reflection::Type::singleton<bool>())
        {
            writeRaw(stream, (char)bpfBool);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, getPropertyValue<bool>(desc, instances[i]));
        }
        else if (desc.type == Reflection::Type::singleton<int>())
        {
			std::vector<int> values;
			values.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				values.push_back(getPropertyValue<int>(desc, instances[i]));
				
            writeRaw(stream, (char)bpfInt);
			writeIntVector(stream, values);
        }
        else if (desc.type == Reflection::Type::singleton<float>())
        {
			std::vector<float> values;
			values.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				values.push_back(getPropertyValue<float>(desc, instances[i]));
				
            writeRaw(stream, (char)bpfFloat);
			writeFloatVector(stream, values);
        }
        else if (desc.type == Reflection::Type::singleton<double>())
        {
            writeRaw(stream, (char)bpfDouble);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, getPropertyValue<double>(desc, instances[i]));
        }
        else if (desc.type == Reflection::Type::singleton<UDim>())
        {
			std::vector<float> scales;
			scales.reserve(instances.size());
			
			std::vector<int> offsets;
			offsets.reserve(instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
			{
				UDim value = getPropertyValue<UDim>(desc, instances[i]);
				
				scales.push_back(value.scale);
				offsets.push_back(value.offset);
			}

            writeRaw(stream, (char)bpfUDim);
            writeFloatVector(stream, scales);
            writeIntVector(stream, offsets);
        }
        else if (desc.type == Reflection::Type::singleton<UDim2>())
        {
			std::vector<float> scalesx, scalesy;
			scalesx.reserve(instances.size());
			scalesy.reserve(instances.size());
			
			std::vector<int> offsetsx, offsetsy;
			offsetsx.reserve(instances.size());
			offsetsy.reserve(instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
			{
				UDim2 value = getPropertyValue<UDim2>(desc, instances[i]);
				
				scalesx.push_back(value.x.scale);
				scalesy.push_back(value.y.scale);
				offsetsx.push_back(value.x.offset);
				offsetsy.push_back(value.y.offset);
			}

            writeRaw(stream, (char)bpfUDim2);
            writeFloatVector(stream, scalesx);
            writeFloatVector(stream, scalesy);
            writeIntVector(stream, offsetsx);
            writeIntVector(stream, offsetsy);
        }
        else if (desc.type == Reflection::Type::singleton<RbxRay>())
        {
            writeRaw(stream, (char)bpfRay);

			for (size_t i = 0; i < instances.size(); ++i)
			{
				RbxRay value = getPropertyValue<RbxRay>(desc, instances[i]);
				
				writeRaw(stream, value.origin());
				writeRaw(stream, value.direction());
			}
        }
        else if (desc.type == Reflection::Type::singleton<Faces>())
        {
            writeRaw(stream, (char)bpfFaces);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, (char)getPropertyValue<Faces>(desc, instances[i]).normalIdMask);
        }
        else if (desc.type == Reflection::Type::singleton<Axes>())
        {
            writeRaw(stream, (char)bpfAxes);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, (char)getPropertyValue<Axes>(desc, instances[i]).axisMask);
        }
        else if (desc.type == Reflection::Type::singleton<BrickColor>())
        {
			std::vector<unsigned int> values;
			values.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				values.push_back(getPropertyValue<BrickColor>(desc, instances[i]).asInt());

            writeRaw(stream, (char)bpfBrickColor);
            writeUIntVector(stream, values);
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Color3>())
        {
			std::vector<float> r;
			std::vector<float> g;
			std::vector<float> b;
			
			r.reserve(instances.size());
			g.reserve(instances.size());
			b.reserve(instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Color3 value = getPropertyValue<G3D::Color3>(desc, instances[i]);
				
				r.push_back(value.r);
				g.push_back(value.g);
				b.push_back(value.b);
			}
            
            writeRaw(stream, (char)bpfColor3);
			writeFloatVector(stream, r);
			writeFloatVector(stream, g);
			writeFloatVector(stream, b);
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Rect2D>())
        {
			std::vector<float> x0;
			std::vector<float> y0;
			std::vector<float> x1;
			std::vector<float> y1;
            
			x0.reserve(instances.size());
			y0.reserve(instances.size());
            x1.reserve(instances.size());
			y1.reserve(instances.size());
            
			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Rect2D value = getPropertyValue<G3D::Rect2D>(desc, instances[i]);
				
				x0.push_back(value.x0());
				y0.push_back(value.y0());
                x1.push_back(value.x1());
				y1.push_back(value.y1());
			}
            
            writeRaw(stream, (char)bpfRect2D);
			writeFloatVector(stream, x0);
			writeFloatVector(stream, y0);
            writeFloatVector(stream, x1);
			writeFloatVector(stream, y1);
        }
		else if (desc.type == Reflection::Type::singleton<PhysicalProperties>())
		{
			writeRaw(stream, (char)bpfPhysicalProperties);

			for (size_t i = 0; i < instances.size(); ++i)
			{
				PhysicalProperties value = getPropertyValue<PhysicalProperties>(desc, instances[i]);

				writePhysicalProperties(stream, value);
			}
		}
        else if (desc.type == Reflection::Type::singleton<G3D::Vector2>())
        {
			std::vector<float> x;
			std::vector<float> y;
			
			x.reserve(instances.size());
			y.reserve(instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Vector2 value = getPropertyValue<G3D::Vector2>(desc, instances[i]);
				
				x.push_back(value.x);
				y.push_back(value.y);
			}
            
            writeRaw(stream, (char)bpfVector2);
			writeFloatVector(stream, x);
			writeFloatVector(stream, y);
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector3>())
        {
			std::vector<float> x;
			std::vector<float> y;
			std::vector<float> z;
			
			x.reserve(instances.size());
			y.reserve(instances.size());
			z.reserve(instances.size());

			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::Vector3 value = getPropertyValue<G3D::Vector3>(desc, instances[i]);
				
				x.push_back(value.x);
				y.push_back(value.y);
				z.push_back(value.z);
			}
            
            writeRaw(stream, (char)bpfVector3);
			writeFloatVector(stream, x);
			writeFloatVector(stream, y);
			writeFloatVector(stream, z);
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector2int16>())
        {
            writeRaw(stream, (char)bpfVector2int16);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, getPropertyValue<G3D::Vector2int16>(desc, instances[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::Vector3int16>())
        {
            writeRaw(stream, (char)bpfVector3int16);

			for (size_t i = 0; i < instances.size(); ++i)
				writeRaw(stream, getPropertyValue<G3D::Vector3int16>(desc, instances[i]));
        }
        else if (desc.type == Reflection::Type::singleton<G3D::CoordinateFrame>())
        {
			std::vector<G3D::Matrix3> rot;
			std::vector<float> tx;
			std::vector<float> ty;
			std::vector<float> tz;
			
			rot.reserve(instances.size());
			tx.reserve(instances.size());
			ty.reserve(instances.size());
			tz.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				G3D::CoordinateFrame cframe = getPropertyValue<G3D::CoordinateFrame>(desc, instances[i]);
				
				rot.push_back(cframe.rotation);
				tx.push_back(cframe.translation.x);
				ty.push_back(cframe.translation.y);
				tz.push_back(cframe.translation.z);
			}
			
			bool exactCFrame = (flags & SerializerBinary::sfInexactCFrame) == 0;
			
			writeRaw(stream, (char)(exactCFrame ? bpfCFrameMatrix : bpfCFrameQuat));
			
			if (exactCFrame)
			{
				for (size_t i = 0; i < instances.size(); ++i)
					writeCFrameRotationExact(stream, rot[i]);
			}
			else
			{
				for (size_t i = 0; i < instances.size(); ++i)
					writeCFrameRotation(stream, rot[i]);
			}
				
			writeFloatVector(stream, tx);
			writeFloatVector(stream, ty);
			writeFloatVector(stream, tz);
        }
        else if (desc.bIsEnum)
        {
			const Reflection::EnumPropertyDescriptor& enumDesc = static_cast<const Reflection::EnumPropertyDescriptor&>(desc);
			
			std::vector<unsigned int> values;
			values.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				values.push_back(enumDesc.getEnumValue(instances[i]));
			}

            writeRaw(stream, (char)bpfEnum);

            writeUIntVector(stream, values);
        }
        else if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(desc))
        {
            const Reflection::RefPropertyDescriptor& refDesc = static_cast<const Reflection::RefPropertyDescriptor&>(desc);

			std::vector<int> values;
			values.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
			{
				values.push_back(getInstanceId(idMap, boost::polymorphic_downcast<const Instance*>(refDesc.getRefValue(instances[i]))));
			}

            writeRaw(stream, (char)bpfRef);

            writeIdVector(stream, values);
        }
        else if (desc.type == Reflection::Type::singleton<ContentId>())
        {
            writeRaw(stream, (char)bpfString);

			for (size_t i = 0; i < instances.size(); ++i)
				writeString(stream, getPropertyValue<ContentId>(desc, instances[i]).toString());
        }
        else if (desc.type == Reflection::Type::singleton<NumberSequence>())
        {
            writeRaw(stream, (char)bpfNumberSequence);

            for (size_t i = 0; i < instances.size(); ++i)
            {
                writeNumberSequence(stream, getPropertyValue<NumberSequence>(desc, instances[i]));
            }
        }
        else if (desc.type == Reflection::Type::singleton<ColorSequence>())
        {
            writeRaw(stream, (char)bpfColorSequenceV1);

            for (size_t i = 0; i < instances.size(); ++i)
            {
                writeColorSequence(stream, getPropertyValue<ColorSequence>(desc, instances[i]));
            }
        }
        else if (desc.type == Reflection::Type::singleton<NumberRange>())
        {
            writeRaw(stream, (char)bpfNumberRange );

            for (size_t i = 0; i < instances.size(); ++i)
            {
                writeRaw(stream, getPropertyValue<NumberRange>(desc, instances[i]));
            }
        }
        else
        {
			throw RBX::runtime_error("Unknown property type for property %s", desc.name.c_str());
        }
    }

    static void serializeGatherGraph(const Instances& instances, std::vector<const Instance*>& objects, std::vector<const Instance*>& objectspost, std::map<const Instance*, int>& idMap)
    {
		for (size_t i = 0; i < instances.size(); ++i)
		{
			const Instance* instance = instances[i].get();

			if (instance && instance->getIsArchivable() && instance->getDescriptor().isSerializable() && idMap.count(instance) == 0)
			{
                idMap[instance] = objects.size();

				objects.push_back(instance);

				const copy_on_write_ptr<Instances>& children = instance->getChildren();

				if (children)
					serializeGatherGraph(*children, objects, objectspost, idMap);

				objectspost.push_back(instance);
			}
		}
    }
        
	struct DescriptorNameComparator
	{
		bool operator()(const Reflection::Descriptor* lhs, const Reflection::Descriptor* rhs) const
		{
			return lhs->name < rhs->name;
		}
	};
	
	typedef std::map<const Reflection::ClassDescriptor*, std::vector<const Instance*>, DescriptorNameComparator> ObjectsByType;
	
	static void serializeGatherObjectsByType(const std::vector<const Instance*>& objects, ObjectsByType& types)
	{
        for (size_t i = 0; i < objects.size(); ++i)
        {
			const Instance* instance = objects[i];
			
			types[&instance->getDescriptor()].push_back(instance);
		}
	}
	
	static void serializeGatherProperties(const Reflection::ClassDescriptor* desc, std::vector<const Reflection::PropertyDescriptor*>& properties)
	{
		Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::Collection::const_iterator iter =
			desc->Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::descriptors_begin();
		Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::Collection::const_iterator end =
			desc->Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::descriptors_end();
				
		for (; iter != end; ++iter)
		{
			const Reflection::PropertyDescriptor& descriptor = **iter;

			if (!descriptor.isReadOnly() && descriptor.canXmlWrite())
				properties.push_back(&descriptor);
		}

		// For stable result, sort properties by name
		std::sort(properties.begin(), properties.end(), DescriptorNameComparator());
	}

	static void serializeImpl(std::ostream& out, const Instance* root, const Instances& instances, unsigned int flags)
	{
		std::vector<const Instance*> objects;
		std::vector<const Instance*> objectspost;
        std::map<const Instance*, int> idMap;

		serializeGatherGraph(instances, objects, objectspost, idMap);
        
		ObjectsByType types;
		serializeGatherObjectsByType(objects, types);
		
		FileHeader header;
		memcpy(header.magic, SerializerBinary::kMagicHeader, sizeof(header.magic));
		memcpy(header.signature, kHeaderSignature, sizeof(header.signature));
		header.version = 0;
		
		header.types = types.size();
		header.objects = objects.size();
		header.reserved[0] = header.reserved[1] = 0;
		
		out.write(reinterpret_cast<char*>(&header), sizeof(header));
		
		// Write out object ids for each type
		unsigned int typeIndex = 0;
		
		for (ObjectsByType::iterator it = types.begin(); it != types.end(); ++it, ++typeIndex)
		{
			const Reflection::ClassDescriptor* type = it->first;
			const std::vector<const Instance*>& instances = it->second;

			MemoryOutputStream stream(type->name.toString());
			
			writeRaw(stream, typeIndex);
			writeString(stream, type->name.toString());
			
			std::vector<int> ids;
			ids.reserve(instances.size());
			
			for (size_t i = 0; i < instances.size(); ++i)
				ids.push_back(getInstanceId(idMap, instances[i]));
				
			// for each id, write whether the object is a service parented to root; this affects deserialization
			// note that since all objects are of the same type it's enough to dynamic_cast once
			bool isServiceType = instances.size() > 0 && dynamic_cast<const Service*>(instances[0]) != NULL;
			
			writeRaw(stream, (char)(isServiceType ? bofServiceType : bofPlain));
			
			writeRaw(stream, (unsigned int)ids.size());
			writeIdVector(stream, ids);
			
			if (isServiceType)
			{
				for (size_t i = 0; i < instances.size(); ++i)
				{
					bool value = instances[i]->getParent() == root;
					writeRaw(stream, value);
				}
			}
			
			writeChunk(out, stream, kChunkInstances, flags);
		}
		
		// Write out properties for each type
		typeIndex = 0;
		
		for (ObjectsByType::iterator it = types.begin(); it != types.end(); ++it, ++typeIndex)
		{
			const Reflection::ClassDescriptor* type = it->first;
			const std::vector<const Instance*>& instances = it->second;

			std::vector<const Reflection::PropertyDescriptor*> properties;
			serializeGatherProperties(type, properties);
			
			for (size_t j = 0; j < properties.size(); ++j)
			{
				MemoryOutputStream stream(type->name.toString() + "-" + properties[j]->name.toString());
				
				writeRaw(stream, typeIndex);
				writeString(stream, properties[j]->name.toString());
				
				writePropertyValues(stream, *properties[j], instances, idMap, flags);
				
				writeChunk(out, stream, kChunkProperty, flags);
			}
		}
		
		// Write out parenting instructions
		{
			MemoryOutputStream stream("Instance-Parent");
			
			std::vector<int> oids;
			std::vector<int> pids;
			
			oids.reserve(objectspost.size());
			pids.reserve(objectspost.size());
			
			for (size_t i = 0; i < objectspost.size(); ++i)
			{
				oids.push_back(getInstanceId(idMap, objectspost[i]));
				pids.push_back(getInstanceId(idMap, objectspost[i]->getParent()));
			}

            writeRaw(stream, (char)bplfPlain);
            writeRaw(stream, (unsigned int)objectspost.size());
			
			writeIdVector(stream, oids);
			writeIdVector(stream, pids);
			
			writeChunk(out, stream, kChunkParents, flags);
		}

		// Write the end chunk
		{
			MemoryOutputStream stream("End");
			
			// This footer is required for the Web code to validate that the file is not truncated
			const char* footer = "</roblox>";
			
			stream.write(footer, strlen(footer));
			
			writeChunk(out, stream, kChunkEnd, SerializerBinary::sfNoCompression);
		}
	}

    static shared_ptr<Instance> createServiceInstance(Instance* root, const Name& name)
    {
        shared_ptr<Instance> result = root->createChild(name, SerializationCreator);

        // Usually all instances are created in bulk, and then parented during last deserialization stage.
        // However, some instances access DM services in onServiceProvider callback so they expect the services to already be there when parenting happens.
        // This means that we have to explicitly parent all services before the last stage.
        // Note that Service::createChild() either returns an existing service (that is already parented) or a new one (that has to be parented here).
        if (result)
            result->setParent(root);

        return result;
    }
	
	static const Reflection::ClassDescriptor* deserializeDecodeInstances(MemoryInputStream& stream, Instance* root, std::vector<shared_ptr<Instance> >& idMap, std::vector<Instance*>& objects)
	{
		const Reflection::ClassDescriptor* type = NULL;
		
		std::string typeName;
		readString(stream, typeName);

		char format;
		readRaw(stream, format);

		if (format != bofPlain && format != bofServiceType)
			throw RBX::runtime_error("Unrecognized object format %d", format);

		unsigned int idCount;
		readRaw(stream, idCount);
		
		std::vector<int> ids;
		readIdVector(stream, ids, idCount);
		
		bool isServiceType = (format == bofServiceType);
		
		std::vector<bool> isServiceRooted;
		
		if (isServiceType)
		{
			isServiceRooted.reserve(ids.size());
			
			for (size_t i = 0; i < ids.size(); ++i)
			{
				bool value;
				readRaw(stream, value);
				
				isServiceRooted.push_back(value);
			}
		}
		
		bool isRootServiceProvider = dynamic_cast<ServiceProvider*>(root) != NULL;

        const Name& typeNameName = Name::lookup(typeName);
		const ICreator* creator = Creatable<Instance>::getCreator(typeNameName);
		
		for (size_t i = 0; i < ids.size(); ++i)
		{
			if (static_cast<unsigned int>(ids[i]) >= idMap.size())
				throw RBX::runtime_error("Invalid id %d", ids[i]);
				
			shared_ptr<Instance> object =
				(isServiceType && isRootServiceProvider && isServiceRooted[i])
				? createServiceInstance(root, typeNameName)
				: creator
					? shared_polymorphic_downcast<Instance>(creator->create())
					: shared_ptr<Instance>();
				
			if (object)
			{
				// TODO: find a better way to do this
				type = &object->getDescriptor();

				if (idMap[ids[i]])
					throw RBX::runtime_error("Duplicate id %d", ids[i]);
					
				idMap[ids[i]] = object;
				objects.push_back(object.get());
			}
		}
		
		return type;
	}
	
	static void deserializeDecodeProperty(MemoryInputStream& stream, const Reflection::ClassDescriptor* typeDesc, const std::vector<shared_ptr<Instance> >& idMap, const std::vector<Instance*>& objects)
	{
		std::string propertyName;
		readString(stream, propertyName);
	
		const Reflection::PropertyDescriptor* propertyDesc = typeDesc->findPropertyDescriptor(propertyName.c_str());
	
		if (propertyDesc && propertyDesc->canXmlRead())
		{
			readPropertyValues(stream, *propertyDesc, objects, idMap);
		}
	}
	
	static void deserializeDecodeParents(MemoryInputStream& stream, Instance* root, Instances* result, const std::vector<shared_ptr<Instance> >& idMap)
	{
		std::vector<int> oids;
		std::vector<int> pids;

		char format;
		readRaw(stream, format);

		if (format != bplfPlain)
			throw RBX::runtime_error("Unrecognized parent link format %d", format);
		
		unsigned int linkCount;
		readRaw(stream, linkCount);

		readIdVector(stream, oids, linkCount);
		readIdVector(stream, pids, linkCount);
		
		for (size_t i = 0; i < oids.size(); ++i)
		{
			if (static_cast<unsigned int>(oids[i]) >= idMap.size())
				throw RBX::runtime_error("Invalid id %d", oids[i]);
				
			if (const shared_ptr<Instance>& object = idMap[oids[i]])
			{
				if (pids[i] != -1)
				{
					if (static_cast<unsigned int>(pids[i]) >= idMap.size())
						throw RBX::runtime_error("Invalid id %d", pids[i]);
					
					object->setParent(idMap[pids[i]].get());
				}
				else
				{
					if (root)
						object->setParent(root);
					
					if (result)
						result->push_back(object);
				}
			}
		}
	}
	
	static void deserializeImpl(std::istream& in, Instance* root, Instances* result)
	{
		FileHeader header;
		readData(in, &header, sizeof(header));
		
		if (memcmp(header.magic, SerializerBinary::kMagicHeader, sizeof(header.magic)) != 0)
			throw RBX::runtime_error("Unrecognized format");
		
		if (memcmp(header.signature, kHeaderSignature, sizeof(header.signature)) != 0)
			throw RBX::runtime_error("The file header is corrupted");
			
		if (header.version != 0)
			throw RBX::runtime_error("Unrecognized version %d", header.version);
		
		// Read types and object ids
		std::vector<const Reflection::ClassDescriptor*> types;
		types.resize(header.types);
		
		std::vector<shared_ptr<Instance> > objects;
		objects.resize(header.objects);
		
		// A list of objects for each type
		std::vector<std::vector<Instance*> > typedobjects;
		typedobjects.resize(header.types);
		
		while (in.good())
		{
			MemoryInputStream stream;
			
			ChunkHeader chunk = readChunk(in, stream);
			
			if (memcmp(chunk.name, kChunkInstances, sizeof(chunk.name)) == 0)
			{
				unsigned int typeIndex;
				readRaw(stream, typeIndex);
				
				if (typeIndex >= types.size())
					throw RBX::runtime_error("Type index out of bounds: %d", typeIndex);
				
				if (types[typeIndex])
					throw RBX::runtime_error("Duplicate type index: %d", typeIndex);

				types[typeIndex] = deserializeDecodeInstances(stream, root, objects, typedobjects[typeIndex]);
			}
			else if (memcmp(chunk.name, kChunkProperty, sizeof(chunk.name)) == 0)
			{
				unsigned int typeIndex;
				readRaw(stream, typeIndex);
				
				if (typeIndex >= types.size())
					throw RBX::runtime_error("Type index out of bounds: %d", typeIndex);

				if (types[typeIndex])
				{
					deserializeDecodeProperty(stream, types[typeIndex], objects, typedobjects[typeIndex]);
				}
			}
			else if (memcmp(chunk.name, kChunkParents, sizeof(chunk.name)) == 0)
			{
				deserializeDecodeParents(stream, root, result, objects);
			}
			else if (memcmp(chunk.name, kChunkEnd, sizeof(chunk.name)) == 0)
			{
				// We're done!
				return;
			}
			else
			{
				// Unknown chunk, skip
			}
		}
	
		// We should only finish reading the file when we see an END chunk
		throw RBX::runtime_error("Unexpected end of file");
	}

	namespace SerializerBinary
	{
		void serialize(std::ostream& out, const Instance* root, unsigned int flags, const Instance::SaveFilter saveFilter)
		{
			Instances emptyInstances;
			const Instances& instances = root->getChildren() ? *root->getChildren() : emptyInstances;
			
			Instances filteredInstances;

			for (Instances::const_iterator it = instances.begin(); it != instances.end(); ++it)
			{
				if (Serializer::canWriteChild(*it, saveFilter))
					filteredInstances.push_back(*it);
			}

			serializeImpl(out, root, filteredInstances, flags);
		}
		
		void serialize(std::ostream& out, const Instances& instances, unsigned int flags)
		{
			serializeImpl(out, NULL, instances, flags);
		}
	
		void deserialize(std::istream& in, Instance* root)
		{
			deserializeImpl(in, root, NULL);
		}
		
		void deserialize(std::istream& in, Instances& result)
		{
			deserializeImpl(in, NULL, &result);
		}
	}
}

/*
	Format description:
	
	File is structured as follows - header (FileHeader), followed by one or more chunks, followed by footer (kMagicSuffix)
	Each chunk has header (ChunkHeader) and a stream of optionally lz4-compressed data.
	The chunk has a name; currently, three chunk names are supported:
	
	INST chunk declares instance ids for a specific type.
	It contains the type index (an integer), the type name and a list of object ids of that type.
	Object ids are arbitrary unique integers; they are currently generated via depth-first pre-order traversal.
	There is some additional information for service types; read the source for details.
	
	PROP chunk specifies property values for all objects of a specific type.
	Each chunk has the type index, a property name, followed by the storage format (see BinaryPropertyFormat), followed by a stream of property data.
	Property data is encoded depending on the type, with interleaving/bit shuffling to improve LZ compression rates.
	
	PRNT chunk specifies parent-child relationship.
	It contains two lists of ids, where the first id is the id of the object, and the second if is the id of the parent.
	The parent-child list is currently stored in depth-first post-order traversal - this improves the time it takes to do setParent() over depth-first pre-order.
*/
