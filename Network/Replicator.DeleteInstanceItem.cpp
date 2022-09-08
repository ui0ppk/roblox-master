#include "Replicator.DeleteInstanceItem.h"

#include "Item.h"
#include "NetworkPacketCache.h"
#include "Replicator.h"
#include "ReplicatorStats.h"
#include "Streaming.h"
#include "Util.h"

#include "V8Tree/Instance.h"

#include "BitStream.h"

#include <string>

namespace RBX {
namespace Network {

DeserializedDeleteInstanceItem::DeserializedDeleteInstanceItem()
{
	type = Item::ItemTypeDelete;
}

void DeserializedDeleteInstanceItem::process(Replicator& replicator)
{
	replicator.readInstanceDeleteItem(this);
}

Replicator::DeleteInstanceItem::DeleteInstanceItem(Replicator* replicator, const shared_ptr<const Instance>& instance)
		:PooledItem(*replicator)
		,id(replicator->extractId(instance.get())) {
	if (replicator->settings().printInstances) {
		details.reset(new InstanceDetails());
		details->className = instance->getClassName().c_str();
		details->guid = instance->getGuid().readableString();
	}

	// remove entry from instance cache
	if (replicator->instancePacketCache)
		replicator->instancePacketCache->remove(instance.get());
}

bool Replicator::DeleteInstanceItem::write(RakNet::BitStream& bitStream) {
	if (!id.valid)
		RBX::StandardOut::singleton()->printf(
		RBX::MESSAGE_SENSITIVE, 
		"Replication: ~NULL >> %s",						// remove player always on right 
		RakNetAddressToString(replicator.remotePlayerId).c_str());
	else
	{
		try
		{
            int byteStart = bitStream.GetNumberOfBytesUsed();
			writeItemType(bitStream, ItemTypeDelete);

			// Write the GUID
			replicator.sendId(bitStream, id);

			if (details)
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"Replication: ~%s:%s >> %s, bytes: %d", 
				details->className.c_str(), 
				details->guid.c_str(),
				RakNetAddressToString(replicator.remotePlayerId).c_str(),
				bitStream.GetNumberOfBytesUsed()-byteStart
				);

			if (replicator.settings().trackDataTypes) {
				replicator.replicatorStats.incrementPacketsSent(ReplicatorStats::PACKET_TYPE_InstanceDelete);
                replicator.replicatorStats.samplePacketsSent(ReplicatorStats::PACKET_TYPE_InstanceDelete, bitStream.GetNumberOfBytesUsed()-byteStart);
            }
		}
		catch (std::runtime_error& e)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"Replication: ~%s >> %s, %s", 
				"UnknownInstance", //instance->getClassName().c_str(), 
				RakNetAddressToString(replicator.remotePlayerId).c_str(), 
				e.what()
				);
		}
	}
	return true;
}

shared_ptr<DeserializedItem> Replicator::DeleteInstanceItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedDeleteInstanceItem> deserializedData(new DeserializedDeleteInstanceItem());

	int start = inBitstream.GetReadOffset();

	replicator.deserializeId(inBitstream, deserializedData->id);

	if (replicator.settings().trackDataTypes) {
		replicator.replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceDelete);
		replicator.replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceDelete, (inBitstream.GetReadOffset()-start)/8);
	}

	return deserializedData;
}


}}
