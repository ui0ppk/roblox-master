/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <string>
#include "V8Tree/Instance.h"
#include "Util/Velocity.h"
#include "bitstream.h"
#include <boost/any.hpp>

#include "StreamingUtil.h"

#include "Dictionary.h"
#include "Util.h"
#include "Network/RakNetFast.h"

DYNAMIC_FASTINT(PhysicsCompressionSizeFilter)

namespace RBX
{
	namespace Network
	{
		void serializeStringCompressed(const std::string& value, RakNet::BitStream &bitStream);
		void deserializeStringCompressed(std::string& value, RakNet::BitStream &bitStream);

	
		template<class T>
		class DescriptorSender
		{
        public:
            struct IdContainer
            {
                uint32_t id;
                bool outdated;
            };
        protected:
			std::map<const T*, IdContainer> descToId;
            int idBits;
			void visit(const T* desc);
			std::string teachName(const T* t) const;
            void send(RakNet::BitStream& stream, const T* value) const
            {
                unsigned int id = getId(value).id;
                send(stream, id);
            }
		public:
			DescriptorSender();
            std::map<const T*, IdContainer> DescToId() const { return descToId; }
            IdContainer getId(const T* value) const
            {
                typename std::map<const T*, IdContainer>::const_iterator iter = descToId.find(value);
                if (iter != descToId.end())
                {
                    // found the desc in dictionary
                    return iter->second;
                }
                else
                {
                    IdContainer result;
                    //Failure to send is all 1s, which will be guaranteed to be too big
                    result.id = 0xFFFFFFFF >> (32-idBits);
                    result.outdated = true;
                    return result;
                }
            }
			void teach(RakNet::BitStream& stream, bool exchangeChecksum, bool useRakString) const
			{
				unsigned int count = descToId.size();
				stream << count;
				for (typename std::map<const T*, IdContainer>::const_iterator iter = descToId.begin(); iter != descToId.end(); ++iter)
				{
					int i = iter->second.id;
					stream << i;
					std::string s = teachName(iter->first);
                    if (useRakString)
                    {
                        RakNet::RakString rakStr = s.c_str();
                        stream.Write(rakStr);
                    }
                    else
                    {
                        serializeStringCompressed(s, stream);
                    }
                    if (exchangeChecksum)
                    {
                        // checksum for this item
                        boost::crc_32_type result;
                        uint32_t checksum = Reflection::ClassDescriptor::checksum(iter->first, result); 
#ifdef NETWORK_DEBUG
                        //StandardOut::singleton()->printf(MESSAGE_INFO, "Checksum of %s: %d", s.c_str(), checksum);
#endif
                        stream << checksum;
                    }
				}
			}
            void send(RakNet::BitStream& stream, uint32_t id)
            {
                stream.WriteBits((unsigned char*) &id, idBits);
            }
		};

		template<class T>
		class DescriptorReceiver
		{
            struct DescContainer
            {
                const T* desc;
                bool outdated;
            };

			std::vector<DescContainer> idToDesc;
			int idBits;
			void learnName(std::string s, int i, uint32_t checksum);
		public:
            void getValue(unsigned int id, const T*& value) const
            {
                value = idToDesc.at(id).desc;
            }
			void learn(RakNet::BitStream& stream, bool exchangeChecksum, bool useRakString)
			{
				uint32_t count;
				stream >> count;
				idToDesc.resize(count);
				for (size_t n=0; n<count; ++n)
				{
					int i;
					stream >> i;
					std::string s;
                    if (useRakString)
                    {
                        RakNet::RakString rakStr;
                        stream.Read(rakStr);
                        s = rakStr.C_String();
                    }
                    else
                    {
                        deserializeStringCompressed(s, stream);
                    }
                    uint32_t checksum = 0;
                    if (exchangeChecksum)
                    {
                        stream >> checksum;
                    }
                    learnName(s, i, checksum);
				}
				idBits = Math::computeMSB(idToDesc.size())+1;
			}
			unsigned int receive(RakNet::BitStream& stream, const T*& value, bool versionCheck) const
			{
				unsigned int id = 0;
				readFastN( stream, id, idBits );

				value = idToDesc.at(id).desc;
                if (value && versionCheck)
                {
                    if (idToDesc.at(id).outdated)
                    {
                        // outdated API
                        value = NULL;
                    }
                }
                return id;
			}

            bool verifyChecksum(const T* value, uint32_t remoteChecksum)
            {
                if (remoteChecksum == 0)
                {
                    // server will not verify the checksum because client does not send checksum to server
                    return true;
                }
                else
                {
                    // only client because only server sends client the checksum
                    boost::crc_32_type result;
                    uint32_t localChecksum = Reflection::ClassDescriptor::checksum(value, result);
                    return (localChecksum == remoteChecksum);
                }
            }
		};

		template<class T>
		class DescriptorDictionary
			: public DescriptorSender<T>
			, public DescriptorReceiver<T>
			, boost::noncopyable
		{
		};

		class IdSerializer : public Instance
		{
		private:
			typedef Instance Super;
            void serializeEnumIndex(const Reflection::EnumDescriptor* desc, const size_t& index, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);
            void deserializeEnumIndex(const Reflection::EnumDescriptor* desc, size_t& index, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);

		protected:
			typedef SharedDictionary<RBX::Guid::Scope> SharedGuidDictionary;
			SharedGuidDictionary scopeNames;		// Used for RBX::Guid
            RBX::Guid::Scope serverScope;
			boost::intrusive_ptr<GuidItem<Instance>::Registry> guidRegistry;
			// A RefProperty that is waiting for an object to be streamed in
			struct WaitItem
			{
				const Reflection::RefPropertyDescriptor* desc;
				boost::shared_ptr<Instance> instance;
			};
			// Map of unknown ID to WaitItem
			typedef std::map<RBX::Guid::Data, std::vector<WaitItem> > WaitItemMap;
			WaitItemMap waitItems;
			boost::mutex waitItemsMutex;

			virtual void setRefValue(WaitItem& wi, Instance* instance);
		public:
			struct Id
			{
				bool valid;
				RBX::Guid::Data id;
			};
			IdSerializer();
			Id extractId(const Instance* instance);
			void sendId(RakNet::BitStream& stream, const Id& id);
			void serializeId(RakNet::BitStream& stream, const Instance* instance);
			void serializeId(RakNet::BitStream& stream, const RBX::Guid::Data& id);
			void serializeIdWithoutDictionary(RakNet::BitStream& stream, const Instance* instance);
			void serializeIdWithoutDictionary(RakNet::BitStream& stream, const RBX::Guid::Data& id);
			bool trySerializeId(RakNet::BitStream& stream, const Instance* instance);
			bool canSerializeId(const Instance* instance);
			void deserializeId(RakNet::BitStream& stream, RBX::Guid::Data& id);
			void deserializeIdWithoutDictionary(RakNet::BitStream& stream, RBX::Guid::Data& id);
			void resolvePendingReferences(Instance* instance, RBX::Guid::Data id);

			void serializeInstanceRef(const Instance* instance, RakNet::BitStream& bitStream);

			bool deserializeInstanceRef(RakNet::BitStream& stream, shared_ptr<Instance>& instance, RBX::Guid::Data& id);		// returns false if it couldn't find the Instance
			bool deserializeInstanceRef(RakNet::BitStream& stream, shared_ptr<Instance>& instance) {
				RBX::Guid::Data dummy;
				return deserializeInstanceRef(stream, instance, dummy);
			}
			size_t numWaitingRefs() const { return waitItems.size(); }
			void addPendingRef(const Reflection::RefPropertyDescriptor* desc,
				boost::shared_ptr<Instance> instance, RBX::Guid::Data id);
		protected:

			void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		};
		void writeBrickVector(RakNet::BitStream& stream, const G3D::Vector3& value);
		void readBrickVector(RakNet::BitStream& stream, G3D::Vector3& value);

		template<class T>
		void serializeGeneric(const Reflection::Variant& value, RakNet::BitStream &bitStream)
		{
			bitStream << value.cast<T>();
		}

		template<class T>
		void deserializeGeneric(Reflection::Variant& value, RakNet::BitStream &bitStream)
		{
			T inputValue;
			bitStream >> inputValue;
			value = inputValue;
		}

		template<class T>
		void serialize(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream)
		{
			bitStream << property.getValue<T>();
		}

		template<class T>
		void deserialize(Reflection::Property& property, RakNet::BitStream &bitStream)
		{
			T value;
			bitStream >> value;
			if (property.getInstance())
				property.setValue(value);
		}

		template<>
		void serialize<ContentId>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<ContentId>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<BrickColor>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<BrickColor>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<UDim>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<UDim>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<UDim2>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<UDim2>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<RBX::RbxRay>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<RBX::RbxRay>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<Faces>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<Faces>(Reflection::Property& property, RakNet::BitStream &bitStream);

		template<>
		void serialize<Axes>(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		template<>
		void deserialize<Axes>(Reflection::Property& property, RakNet::BitStream &bitStream);


		void serializeEnum(const Reflection::EnumDescriptor* desc, const Reflection::Variant& value, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);
		void deserializeEnum(const Reflection::EnumDescriptor* desc, Reflection::Variant& result, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);

		void serializeStringProperty(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream);
		void deserializeStringProperty(Reflection::Property& property, RakNet::BitStream &bitStream);

		void serializeGuidScope(RakNet::BitStream& stream, const RBX::Guid::Scope& value, bool canDisableCompression);
		void deserializeGuidScope(RakNet::BitStream& stream, RBX::Guid::Scope& value, bool canDisableCompression);

		void serializeEnumProperty(const Reflection::ConstProperty& property, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);
		void deserializeEnumProperty(Reflection::Property& property, RakNet::BitStream &bitStream, size_t enumSizeMSB = 0);


        namespace CustomSerializer
        {
			static const float kMinDeltaShort = 1.f/(65535.f*2);
			static const float kMinDeltaByte = 1.f/(255.f*2);

            inline void writeCompressedFloat(bool heavyCompression, const float &inVar, RakNet::BitStream &bitStream)
            {
                //| -1| 0 |+1 | // after compression
                //|---|---|---|
                //--|---|---|--
                //  |  0.f  |   // before compression (minDelta for each segment)

                float minDelta;
                if (heavyCompression)
                {
                    // to byte
                    minDelta = kMinDeltaByte;
                }
                else
                {
                    // to short
                    minDelta = kMinDeltaShort;
                }

                RakAssert(inVar > -1.f-minDelta/2 && inVar < 1.f+minDelta/2);

                bool isNegative = inVar < 0;
                bitStream.Write(isNegative);

                float absValue = fabs(inVar);
                if (absValue > 1.0f)
                    absValue=1.0f;

                if (heavyCompression)
                {
                    unsigned char compressedValue = (unsigned char)((absValue+minDelta)*255.f);
                    bitStream.Write(compressedValue);
                }
                else
                {
                    unsigned short compressedValue = (unsigned short)((absValue+minDelta)*32767.f);
                    bitStream.Write(compressedValue);
                }
            }

            inline bool readCompressedFloat(bool heavyCompression, float &outVar, RakNet::BitStream &bitStream)
            {
                bool isNegative;
                bitStream.Read(isNegative);

                float absValue;
                if (heavyCompression)
                {
                    unsigned char compressedFloat;
                    if (bitStream.Read(compressedFloat))
                    {
                        absValue = ((float)compressedFloat / 255.f - kMinDeltaByte);
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    unsigned short compressedFloat;
                    if (bitStream.Read(compressedFloat))
                    {
                        absValue = ((float)compressedFloat / 32767.f - kMinDeltaShort);
                    }
                    else
                    {
                        return false;
                    }
                }
                if (isNegative)
                {
                    outVar = -absValue;
                }
                else
                {
                    outVar = absValue;
                }
                return true;
            }

            inline void writeVector(bool heavilyCompressed, const float &x, const float &y, const float &z, RakNet::BitStream &bitStream)
            {
                float magnitude = sqrt(x * x + y * y + z * z);

                // Let's check if we really want to compress the vector heavily
                if (heavilyCompressed && magnitude > (float)(DFInt::PhysicsCompressionSizeFilter))
                {
                    // when magnitude is too large, heavy lossy compression could cause noticeable desyncs
                    heavilyCompressed = false;
                }

                bitStream.Write(heavilyCompressed);
                bitStream.Write(magnitude);
                float minDelta;
                if (heavilyCompressed)
                {
                    minDelta = kMinDeltaByte;
                }
                else
                {
                    minDelta = kMinDeltaShort;
                }
                if (magnitude > minDelta)
                {
                    writeCompressedFloat(heavilyCompressed, x/magnitude, bitStream);
                    writeCompressedFloat(heavilyCompressed, y/magnitude, bitStream);
                    // we will re-construct z from x and y
                    bitStream.Write((bool)(z>0.f)); // remember the sign of z
                }
            }

            inline bool readVector( float &x, float &y, float &z, RakNet::BitStream &bitStream )
            {
                bool heavilyCompressed;
                bitStream.Read(heavilyCompressed);
                float magnitude;
                if (!bitStream.Read(magnitude))
                    return false;
                bool hasValues;
                if (heavilyCompressed)
                {
                    hasValues = magnitude>kMinDeltaByte;
                }
                else
                {
                    hasValues = magnitude>kMinDeltaShort;
                }
                if (hasValues)
                {
                    readCompressedFloat(heavilyCompressed, x, bitStream);
                    readCompressedFloat(heavilyCompressed, y, bitStream);

                    // calculate z
                    bool zSign;
                    bitStream.Read(zSign);
                    float difference = 1.0f - x*x - y*y;
                    if (difference < 0.0f)
                        difference=0.0f;
                    z = sqrt(difference);
                    if (zSign == false)
                    {
                        z=-z;
                    }

                    x*=magnitude;
                    y*=magnitude;
                    z*=magnitude;
                }
                else
                {
                    x=0.0;
                    y=0.0;
                    z=0.0;
                }
                return true;
            }

            inline void writeNormQuat(bool heavilyCompressed, const float &w, const float &x, const float &y, const float &z, RakNet::BitStream &bitStream)
            {
                bitStream.Write(heavilyCompressed);
                bitStream.Write((bool)(w<0.0));
                writeCompressedFloat(heavilyCompressed, x, bitStream);
                writeCompressedFloat(heavilyCompressed, y, bitStream);
                writeCompressedFloat(heavilyCompressed, z, bitStream);
                // we will re-construct w from x,y,z
            }

            inline bool readNormQuat(float &w, float &x, float &y, float &z, RakNet::BitStream &bitStream)
            {
                bool heavilyCompressed;
                bool cwNeg=false;
                bitStream.Read(heavilyCompressed);
                bitStream.Read(cwNeg);
                readCompressedFloat(heavilyCompressed, x, bitStream);
                readCompressedFloat(heavilyCompressed, y, bitStream);
                readCompressedFloat(heavilyCompressed, z, bitStream);

                // Calculate w from x,y,z
                float difference = 1.0f - x*x - y*y - z*z;
                if (difference < 0.0f)
                    difference=0.0f;
                w = sqrt(difference);
                if (cwNeg)
                    w=-w;

                return true;
            }
        }
	}
}
