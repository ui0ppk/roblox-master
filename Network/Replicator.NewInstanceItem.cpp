#include "Replicator.NewInstanceItem.h"

#include "Item.h"
#include "Replicator.h"
#include "ReplicatorStats.h"
#include "Security/FuzzyTokens.h"
#include "Players.h"
#include "FastLog.h"
#include "network/NetworkPacketCache.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/HackDefines.h"

#include "RakNetTime.h"
#include "BitStream.h"

#include "VMProtect/VMProtectSDK.h"

namespace RBX { 
namespace Network {

DeserializedNewInstanceItem::DeserializedNewInstanceItem() : classDescriptor(NULL)
{ 
	type = Item::ItemTypeNew;
}

void DeserializedNewInstanceItem::process(Replicator& replicator)
{
	replicator.readInstanceNewItem(this, false);
}

Replicator::NewInstanceItem::NewInstanceItem(Replicator* replicator, shared_ptr<const Instance> instance)
	: PooledItem(*replicator)
	, instance(instance)
	, useStoredParentGuid(false)
{
	// check if the flag is on, and that this isn't the special case for
	// initial player sync.
	// HACK -- Currently the client replicator creates a NewInstanceItem for
	// its local player before it has received the correct GUID for the
	// Players service from the server. So, bypass the parent desync bug
	// fix if the new instance is a Player under Players.
	useStoredParentGuid = 
		!(Instance::fastSharedDynamicCast<const Player>(instance) &&
		Instance::fastDynamicCast<const Players>(instance->getParent()));

	if (!replicator->isLegalSendInstance(instance.get()))
		return;

	RBX::mutex::scoped_lock lock(replicator->pendingInstancesMutex);
	replicator->pendingNewInstances.insert(instance.get());

	// create an entry in new instance bitstream cache
	if (replicator->instancePacketCache && !Replicator::isTopContainer(instance.get()))
		replicator->instancePacketCache->insert(instance.get());

	if (useStoredParentGuid)
	{
		instance->getParent()->getGuid().extract(parentIdAtItemCreation);
	}
}

bool Replicator::NewInstanceItem::write(RakNet::BitStream& bitStream) 
{
	// If the class is outdated, we still write the instance, because the property and event serializers will take care of the changes
	// But if the class is removed, we simply bypass it
	if (replicator.isClassRemoved(instance.get()))
	{
		return true;
	}
	DescriptorSender<RBX::Reflection::ClassDescriptor>::IdContainer idContainer = replicator.classDictionary.getId(&instance->getDescriptor());

	int byteStart = bitStream.GetNumberOfBytesUsed();
	if (!replicator.removeFromPendingNewInstances(instance.get()))
		return true;

	writeItemType(bitStream,ItemTypeNew);

	// Write the GUID
	replicator.serializeId(bitStream, instance.get());

	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,				// remote player always on right
			"Replication NewInstance::write: %s:%s:%s >> %s", 
			instance->getClassName().c_str(), 
			instance->getGuid().readableString().c_str(),
			instance->getName().c_str(),
			RakNetAddressToString(replicator.remotePlayerId).c_str()
			);
	}
	if (replicator.settings().trackDataTypes) {
		replicator.replicatorStats.incrementPacketsSent(ReplicatorStats::PACKET_TYPE_InstanceNew);
		replicator.replicatorStats.samplePacketsSent(ReplicatorStats::PACKET_TYPE_InstanceNew, bitStream.GetNumberOfBytesUsed()-byteStart);
	}

	// Write ClassName
	replicator.classDictionary.send(bitStream, idContainer.id);

	// Write ownership flag
	bool deleteOnDisconnect = replicator.remoteDeleteOnDisconnect(instance.get());
	bitStream << deleteOnDisconnect;

	// write non cacheable properties first
	replicator.writeProperties(instance.get(), bitStream, Replicator::PropertyCacheType_NonCacheable, true);

	if (replicator.instancePacketCache)	// use cache
	{
		unsigned int startBit = bitStream.GetWriteOffset();

		// try fetch from cache.
		if(!replicator.instancePacketCache->fetchIfUpToDate(instance.get(), bitStream, false))
		{
			// instance dirty or cached bitstream was not set

			replicator.writeProperties(instance.get(), bitStream, Replicator::PropertyCacheType_Cacheable, true);

			bitStream.SetReadOffset(startBit);

			// update the cache
			replicator.instancePacketCache->update(instance.get(), bitStream, bitStream.GetNumberOfBitsUsed() - startBit, false);
		}
		else
		{
			if (replicator.settings().printBits)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
					"   write from cache %d bits", 
					bitStream.GetWriteOffset() - startBit);
			}
		}
	}
	else // not using cache
		replicator.writeProperties(instance.get(), bitStream, Replicator::PropertyCacheType_Cacheable, true);

	// Write the Parent property
	if (useStoredParentGuid)
	{
		replicator.serializeId(bitStream, parentIdAtItemCreation);
	}
	else
	{
		Reflection::ConstProperty property(Instance::propParent, instance.get());
		replicator.serializePropertyValue(property, bitStream, true /*useDictionary*/);
	}

	return true;
}

bool Replicator::NewInstanceItem::read(Replicator& replicator, RakNet::BitStream& bitStream, bool isJoinData, DeserializedNewInstanceItem& deserializedItem)
{
	if (isJoinData)
		replicator.deserializeIdWithoutDictionary(bitStream, deserializedItem.id);
	else
		replicator.deserializeId(bitStream, deserializedItem.id);

	// Get the class and construct the object
	unsigned int classId = replicator.classDictionary.receive(bitStream, deserializedItem.classDescriptor, false /*we don't do version check for class here, if the properties or events of the class are changed, they will be handled later*/);

	if (replicator.ProcessOutdatedInstance(bitStream, isJoinData, deserializedItem.id, deserializedItem.classDescriptor, classId))
	{
		return false;
	}

	bool newInstance = false;
	shared_ptr<Instance> i;
	if (replicator.guidRegistry->lookupByGuid(deserializedItem.id, i))
	{
		// We got back an object we've already seen
		if (i==0)
			throw RBX::runtime_error("readInstanceNew got a null object (guid %s)", deserializedItem.id.readableString(32).c_str());

		deserializedItem.instance = i;

		if (deserializedItem.instance->getDescriptor()!=*deserializedItem.classDescriptor)
		{
			std::string message = RBX::format("Replication: Bad re-binding in deserialize new instance %s-%s << %s, %s-%s", 
				deserializedItem.classDescriptor->name.c_str(), 
				deserializedItem.id.readableString().c_str(), 
				RakNetAddressToString(replicator.remotePlayerId).c_str(), 
				deserializedItem.instance->getClassName().c_str(), 
				deserializedItem.id.readableString().c_str());

			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "%s", message.c_str());

			throw RBX::runtime_error("%s", message.c_str());
		}
	}
	else
	{
		deserializedItem.instance = Creatable<Instance>::createByName(deserializedItem.classDescriptor->name, RBX::ReplicationCreator);
		if (!deserializedItem.instance)
		{
			std::string message = format("Replication: Can't create object of type %s", deserializedItem.classDescriptor->name.c_str());
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, message);
			throw std::runtime_error(message);
		}

		replicator.guidRegistry->assignGuid(deserializedItem.instance.get(), deserializedItem.id);
		newInstance = true;
	}

	bitStream >> deserializedItem.deleteOnDisconnect;

	// Write properties directly into newly created instances since it's not in datamodel yet, otherwise write to variant array to be set later.
	if (isJoinData)
	{
		replicator.readProperties(bitStream, deserializedItem.instance.get(), PropertyCacheType_All, false, false, newInstance ? NULL : &deserializedItem.propValueList);
		replicator.deserializeIdWithoutDictionary(bitStream, deserializedItem.parentId);
	}
	else
	{
		replicator.readProperties(bitStream, deserializedItem.instance.get(), PropertyCacheType_NonCacheable, true, false, newInstance ? NULL : &deserializedItem.propValueList);
		replicator.readProperties(bitStream, deserializedItem.instance.get(), PropertyCacheType_Cacheable, true, false, newInstance ? NULL : &deserializedItem.propValueList);
		replicator.deserializeId(bitStream, deserializedItem.parentId);
	}
    
	replicator.guidRegistry->lookupByGuid(deserializedItem.parentId, deserializedItem.parent);

	return true;
}

shared_ptr<DeserializedItem> Replicator::NewInstanceItem::read(Replicator& replicator, RakNet::BitStream& bitStream, bool isJoinData)
{	
	shared_ptr<DeserializedNewInstanceItem> deserializedData(new DeserializedNewInstanceItem());

	if (!read(replicator, bitStream, isJoinData, *deserializedData))
		return shared_ptr<DeserializedNewInstanceItem>();

	return deserializedData;
}

}}
