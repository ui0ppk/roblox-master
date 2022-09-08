/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#include "Dictionary.h"
#include "stringcompressor.h"
#include "streaming.h"
#include "util/RobloxGoogleAnalytics.h"
#include <boost/functional/hash/hash.hpp>

namespace RBX { 
	
using namespace Reflection;

namespace Network {

template<>
bool SenderDictionary<std::string>::isDefaultValue(const std::string& value)
{
	return value.empty();
}
    
template<>
bool SenderDictionary<BinaryString>::isDefaultValue(const BinaryString& value)
{
	return value.value().empty();
}

void SenderDictionary<const RBX::Name*>::send(RakNet::BitStream& stream, const RBX::Name* value)
{
	if (isDefaultValue(value->toString()))
	{
		// id==0 is reserved for the empty item
		unsigned char id = 0;
		stream.Write(id);
		return;
	}

	std::pair<Dictionary::iterator, bool> pair = dictionary.insert(Dictionary::value_type(value, nextIndex));
	if (!pair.second)
	{
		// The item already exists in the dictionary. Simply send the current ID
		unsigned char id = pair.first->second;
		stream.Write(id);
	}
	else
	{
		// Remove the pre-existing entry from dictionary
		dictionary.erase(items[nextIndex]);

		// Put the new entry into the item table
		items[nextIndex] = const_cast<RBX::Name*>(value);

		// Send the id with 0x80 bit set to indicate this is a new item
		unsigned char id = nextIndex | 0x80;
		stream.Write(id);	// TODO: We could get away with just sending a single bit. The receiver knows what the new index should be
		stream << value->toString();

		// Advance nextIndex == [1..127]
		nextIndex %= (DICTIONARY_SIZE-1);
		++nextIndex;
	}
}

template<>
void ReceiverDictionary<std::string>::setDefault(std::string& value)
{
	value.clear();
}

template<>
void ReceiverDictionary<BinaryString>::setDefault(BinaryString& value)
{
	value = BinaryString();
}
    
void ReceiverStringDictionary::setDefault(std::string& value)
{
	value.clear();
}

void ReceiverStringDictionary::learn(unsigned char id, const std::string& value)
{
	dictionary[id] = value;
	if(protection){
		if(!hashTable.get()){
			hashTable.reset(new std::size_t[DICTIONARY_SIZE]);
		}
		boost::hash<std::string> stringHash;
		hashTable.get()[id] = stringHash(std::string("a") + value + std::string("s"));
	}
}
bool ReceiverStringDictionary::get(unsigned char id, std::string& value)
{
	value = dictionary[id];

	if(protection){
		RBXASSERT(hashTable.get());
		boost::hash<std::string> stringHash;
		if(stringHash(std::string("a") + value + std::string("s")) != hashTable.get()[id]){
			//Someone has been messing with our content... death to the infidels
			value.clear();
			return false;
		}
	}

	//It's all good
	return true;
}

void SharedStringDictionary::serializeString(const std::string& stringValue, RakNet::BitStream &bitStream)
{
	SenderDictionary<std::string>::send(bitStream, stringValue);
}

void SharedStringDictionary::serializeString(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	const PropertyDescriptor& desc = property.getDescriptor();
	std::string value = desc.getStringValue(property.getInstance());
	
	serializeString(value,bitStream);
}

void SharedStringDictionary::deserializeString(std::string& valueString, RakNet::BitStream &bitStream)
{
	ReceiverDictionary<std::string>::receive(bitStream, valueString);
}

void SharedStringDictionary::deserializeString(Property& property, RakNet::BitStream &bitStream)
{
	std::string value;
	deserializeString(value,bitStream);

	if (property.getInstance())
	{
		const PropertyDescriptor& desc = property.getDescriptor();
		desc.setStringValue(property.getInstance(), value);
	}
}

SharedStringProtectedDictionary::SharedStringProtectedDictionary(bool protection)
	:ReceiverStringDictionary(protection)
{}

void SharedStringProtectedDictionary::serializeString(const std::string& stringValue, RakNet::BitStream &bitStream)
{
	SenderDictionary<std::string>::send(bitStream, stringValue);
}

void SharedStringProtectedDictionary::serializeString(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	const PropertyDescriptor& desc = property.getDescriptor();
	std::string value = desc.getStringValue(property.getInstance());
	
	serializeString(value,bitStream);
}

bool SharedStringProtectedDictionary::deserializeString(std::string& valueString, RakNet::BitStream &bitStream)
{
	return ReceiverStringDictionary::receive(bitStream, valueString);
}

bool SharedStringProtectedDictionary::deserializeString(Property& property, RakNet::BitStream &bitStream)
{
	std::string value;
	if(!deserializeString(value,bitStream)){
		//They were cheating
		return false;
	}

	if (property.getInstance())
	{
		const PropertyDescriptor& desc = property.getDescriptor();
		desc.setStringValue(property.getInstance(), value);
	}

	return true;
}

void SharedBinaryStringDictionary::serializeString(const BinaryString& stringValue, RakNet::BitStream &bitStream)
{
	SenderDictionary<BinaryString>::send(bitStream, stringValue);
}

void SharedBinaryStringDictionary::serializeString(const ConstProperty& property, RakNet::BitStream &bitStream)
{
	BinaryString value = property.getValue<BinaryString>();
	
	serializeString(value, bitStream);
}

void SharedBinaryStringDictionary::deserializeString(BinaryString& valueString, RakNet::BitStream &bitStream)
{
	ReceiverDictionary<BinaryString>::receive(bitStream, valueString);
}

void SharedBinaryStringDictionary::deserializeString(Property& property, RakNet::BitStream &bitStream)
{
	BinaryString value;
	deserializeString(value,bitStream);

	if (property.getInstance())
		property.setValue<BinaryString>(value);
}

} }
