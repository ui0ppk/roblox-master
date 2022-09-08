#include "Replicator.ChangePropertyItem.h"

#include "Item.h"
#include "Replicator.h"

#include "Reflection/property.h"

#include "BitStream.h"

namespace RBX {
namespace Network {

DeserializedChangePropertyItem::DeserializedChangePropertyItem()
{
	type = Item::ItemTypeChangeProperty;
}

void DeserializedChangePropertyItem::process(Replicator& replicator)
{
	replicator.readChangedPropertyItem(this);
}

Replicator::ChangePropertyItem::ChangePropertyItem(
	Replicator* replicator, const shared_ptr<const Instance>& instance,
	const Reflection::PropertyDescriptor& desc)
	: PooledItem(*replicator), desc(desc), instance(instance) 
{
}

bool Replicator::ChangePropertyItem::write(RakNet::BitStream& bitStream) {
	replicator.pendingChangedPropertyItems.erase(
		RBX::Reflection::ConstProperty(desc, instance.get()));

	// Check to see if we are still replicating this object
	if (!replicator.isReplicationContainer(instance.get()))
		return true;

	// Write value
	replicator.writeChangedProperty(instance.get(), desc, bitStream);
	return true;
}

shared_ptr<DeserializedItem> Replicator::ChangePropertyItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedChangePropertyItem> deserializedData(new DeserializedChangePropertyItem());

	int start = inBitstream.GetReadOffset();
	shared_ptr<Instance> deserializedInstance;
	replicator.deserializeInstanceRef(inBitstream, deserializedInstance, deserializedData->id);
	deserializedData->instance = deserializedInstance;

	// Read the property name
	unsigned int propId = replicator.propDictionary.receive(inBitstream, deserializedData->propertyDescriptor, true);

	if (replicator.ProcessOutdatedChangedProperty(inBitstream, deserializedData->id, deserializedInstance.get(), deserializedData->propertyDescriptor, propId))
	{
		return shared_ptr<DeserializedChangePropertyItem>();
	}

	if (!deserializedData->propertyDescriptor)
		throw RBX::runtime_error("Replicator readChangedProperty NULL descriptor");

	if (deserializedInstance && !deserializedInstance->getDescriptor().isA(deserializedData->propertyDescriptor->owner))
	{
		throw RBX::runtime_error("Replication: Bad re-binding prop %s-%s << %s",
			deserializedInstance->getClassName().c_str(),
			deserializedData->propertyDescriptor->name.c_str(), 
			RakNetAddressToString(replicator.remotePlayerId).c_str());
	}

	if (!replicator.isServerReplicator())
	{
		inBitstream >> deserializedData->versionReset;
	}

	Reflection::Property prop = Reflection::Property(*deserializedData->propertyDescriptor, deserializedInstance.get());
	replicator.deserializePropertyValue(inBitstream, prop, true /*useDictionary*/, false, &deserializedData->value);
    
    if (replicator.settings().trackDataTypes) {
        replicator.replicatorStats.incrementPacketsReceived(deserializedData->propertyDescriptor->category.str);
        replicator.replicatorStats.samplePacketsReceived(deserializedData->propertyDescriptor->category.str, (inBitstream.GetReadOffset()-start)/8);
    }

	return deserializedData;
}

}
}
