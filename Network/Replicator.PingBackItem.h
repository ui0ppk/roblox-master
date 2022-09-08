#pragma once

#include "Item.h"
#include "Replicator.h"

#include "RakNetTime.h"

namespace RBX {
namespace Network {

class Replicator::PingBackItem : public PooledItem
{
	RakNet::Time time;
    unsigned int extraStats;
public:
	PingBackItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
};

}
}
