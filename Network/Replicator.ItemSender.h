#pragma once

#include "Replicator.h"

#include "BitStream.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace RBX {
namespace Network {

class ConcurrentRakPeer;
class Item;

class Replicator::ItemSender : boost::noncopyable
{
	Replicator& replicator;
	ConcurrentRakPeer* rakPeer;
	shared_ptr<RakNet::BitStream> bitStream;
	PacketPriority packetPriority;

	void openPacket();
	void closePacket();
	const unsigned int maxStreamSize;
public:
	bool sentItems;
	ItemSender(Replicator& replicator, ConcurrentRakPeer *rakPeer);
	~ItemSender();

	typedef enum {SEND_BITSTREAM_FULL = 0, SEND_OK} SendStatus;

	SendStatus send(Item& item);
	int getNumberOfBytesUsed() const;
};

}
}