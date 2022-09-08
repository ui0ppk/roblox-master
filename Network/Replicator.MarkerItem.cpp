#include "Replicator.MarkerItem.h"

#include "Item.h"
#include "Replicator.h"
#include "Util.h"
#include "Replicator.StreamJob.h"

#include "BitStream.h"
#include <string>

DYNAMIC_LOGGROUP(NetworkJoin)

namespace RBX {
namespace Network {

Replicator::MarkerItem::MarkerItem(Replicator* replicator, int id)
		: Item(*replicator), id(id) 
	{}

bool Replicator::MarkerItem::write(RakNet::BitStream& bitStream) {
	if(!replicator.isInitialDataSent())
		return false;

	writeItemType(bitStream, ItemTypeMarker);

	bitStream << id;
	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
		"Replication: Sending marker %d to %s", 
		id, 
		RakNetAddressToString(replicator.remotePlayerId).c_str()
		);
	}

	replicator.onSentMarker(id);
	FASTLOG1F(DFLog::NetworkJoin, "MarkerItem %ld sent", id);

	return true;
}

shared_ptr<DeserializedItem> Replicator::MarkerItem::read(Replicator& replicator, RakNet::BitStream& bitStream)
{
	shared_ptr<DeserializedMarkerItem> deserializedData(new DeserializedMarkerItem());

	bitStream >> deserializedData->id;

	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Received marker %d from %s", deserializedData->id,
			RakNetAddressToString(replicator.remotePlayerId).c_str());
	}

	return deserializedData;
}

}
}
