#pragma once

#include "Item.h"
#include "Replicator.h"

#include "RakNetTime.h"

namespace RBX {
namespace Network {

class DeserializedPingItem : public DeserializedItem
{
public:
	bool pingBack;
	RakNet::Time time;
	unsigned int sendStats;
	unsigned int extraStats;

	DeserializedPingItem();
	~DeserializedPingItem() {}

	/*implement*/ void process(Replicator& replicator);
};


class Replicator::PingItem : public PooledItem
{
	RakNet::Time time;
    unsigned int extraStats;
public:
	PingItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}
}
