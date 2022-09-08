#include "Replicator.EventInvocationItem.h"

#include "Item.h"
#include "NetworkSettings.h"
#include "Replicator.h"
#include "Util.h"

#include "Reflection/Event.h"
#include "Util/StandardOut.h"

#include "BitStream.h"

namespace RBX {
namespace Network {

DeserializedEventInvocationItem::DeserializedEventInvocationItem()
{
	type = Item::ItemTypeEventInvocation;
}

void DeserializedEventInvocationItem::process(Replicator& replicator)
{
	replicator.readEventInvocationItem(this);
}

Replicator::EventInvocationItem::EventInvocationItem(Replicator* replicator,
		const shared_ptr<Instance>& instance,
		const Reflection::EventDescriptor& desc,
		const Reflection::EventArguments& arguments)
	: PooledItem(*replicator)
	, desc(desc)
	, instance(instance) 
	, arguments(arguments)
	{}

bool Replicator::EventInvocationItem::write(RakNet::BitStream& bitStream)
{
	// Check to see if we are still replicating this object
	if (!replicator.isReplicationContainer(instance.get()))
		return true;

    DescriptorSender<RBX::Reflection::EventDescriptor>::IdContainer idContainer = replicator.eventDictionary.getId(&desc);
    if (idContainer.outdated)
    {
        return true;
    }

    if (replicator.isEventRemoved(instance.get(), desc.name))
    {
        return true;
    }

	int byteStart = bitStream.GetNumberOfBytesUsed();
	writeItemType(bitStream, ItemTypeEventInvocation);

	// Write the GUID
	replicator.serializeId(bitStream, instance.get());

	// Write property name
	replicator.eventDictionary.send(bitStream, idContainer.id);

	// Write value
	replicator.serializeEventInvocation(
		Reflection::EventInvocation(Reflection::Event(desc, instance),arguments), bitStream);

	if (replicator.settings().printEvents) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication: %s-%s.%s >> %s, bytes: %d", 
			instance->getClassName().c_str(), 
			instance->getGuid().readableString().c_str(), 
			desc.name.c_str(),
			RakNetAddressToString(replicator.remotePlayerId).c_str(),
			bitStream.GetNumberOfBytesUsed()-byteStart
			);
	}

	if (replicator.settings().trackDataTypes) {
		replicator.replicatorStats.incrementPacketsSent(ReplicatorStats::PACKET_TYPE_Event);
		replicator.replicatorStats.samplePacketsSent(ReplicatorStats::PACKET_TYPE_Event, bitStream.GetNumberOfBytesUsed()-byteStart);
	}

	return true;
}

shared_ptr<DeserializedItem> Replicator::EventInvocationItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedEventInvocationItem> deserializedData(new DeserializedEventInvocationItem());

	shared_ptr<Instance> deseiralizedInstance;
	RBX::Guid::Data id;
	replicator.deserializeInstanceRef(inBitstream, deseiralizedInstance, id);

	// Read the event name
	unsigned int eventId = replicator.eventDictionary.receive(inBitstream, deserializedData->eventDescriptor, true);

	if (replicator.ProcessOutdatedEventInvocation(inBitstream, id, deseiralizedInstance.get(), deserializedData->eventDescriptor, eventId))
	{
		return shared_ptr<DeserializedEventInvocationItem>();
	}

	if (deseiralizedInstance && !deseiralizedInstance->getDescriptor().isA(deserializedData->eventDescriptor->owner))
	{
		throw RBX::runtime_error("Replication: Bad re-binding event %s-%s << %s",
			deseiralizedInstance->getClassName().c_str(),
			deserializedData->eventDescriptor->name.c_str(), 
			RakNetAddressToString(replicator.remotePlayerId).c_str());
	}

	if (replicator.settings().printEvents)
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
		"Replication: %s-%s.%s << %s",							// remote player always on right side 
		deseiralizedInstance ? deseiralizedInstance->getClassName().c_str() : "?", 
		id.readableString().c_str(), 
		deserializedData->eventDescriptor->name.c_str(),
		RakNetAddressToString(replicator.remotePlayerId).c_str() 
		);

	deserializedData->instance = deseiralizedInstance;

	deserializedData->eventInvocation.reset(new Reflection::EventInvocation(Reflection::Event(*deserializedData->eventDescriptor, deserializedData->instance)));
	replicator.deserializeEventInvocation(inBitstream, *deserializedData->eventInvocation, false);

	return deserializedData;
}

}
}
