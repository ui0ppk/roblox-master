#include "Replicator.TagItem.h"

#include "Item.h"
#include "Replicator.h"
#include "ClientReplicator.h"
#include "Util.h"
#include "Replicator.StreamJob.h"

#include "BitStream.h"
#include <string>

DYNAMIC_LOGGROUP(NetworkJoin)

namespace RBX { namespace Network {

DeserializedTagItem::DeserializedTagItem()
{
	type = Item::ItemTypeTag;
}

void DeserializedTagItem::process(Replicator& replicator)
{
	ClientReplicator* rep = rbx_static_cast<ClientReplicator*>(&replicator);
	rep->readTagItem(this);
}

Replicator::TagItem::TagItem(Replicator* replicator, int id, boost::function<bool()> readyCallback)
	: Item(*replicator), id(id), readyCallback(readyCallback)
{}

bool Replicator::TagItem::write(RakNet::BitStream& bitStream) 
{
	if (readyCallback && !readyCallback())
	{
		return false;
	}

	writeItemType(bitStream, ItemTypeTag);

	bitStream << id;
	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication: Sending tag %d to %s", 
			id,
			RakNetAddressToString(replicator.remotePlayerId).c_str()
			);
	}

	replicator.onSentTag(id);

	return true;
}

shared_ptr<DeserializedItem> Replicator::TagItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedTagItem> deserializedData(new DeserializedTagItem());

	inBitstream >> deserializedData->id;

	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Received tag %d from %s", deserializedData->id,
			RakNetAddressToString(replicator.remotePlayerId).c_str());
		}

	return deserializedData;
}

}}
