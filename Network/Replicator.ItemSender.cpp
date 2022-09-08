#include "Replicator.ItemSender.h"

#include "ConcurrentRakPeer.h"
#include "Item.h"
#include "Replicator.h"

#include "BitStream.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace RBX {
namespace Network {

void Replicator::ItemSender::openPacket()
{
	if (!bitStream)
	{
		bitStream.reset(new RakNet::BitStream());
		*bitStream << (unsigned char) ID_DATA;
	}
}

void Replicator::ItemSender::closePacket()
{
	if (bitStream)
	{
		const int bytes = bitStream->GetNumberOfBytesUsed();
		if (bytes>0)
		{
			// Write the "end" id (non-compact)
			Item::writeItemType(*bitStream, Item::ItemTypeEnd);

			rakPeer->Send(bitStream, packetPriority, DATAMODEL_RELIABILITY, DATA_CHANNEL, replicator.remotePlayerId, false);
			replicator.replicatorStats.dataPacketsSent.sample();
			replicator.replicatorStats.dataPacketsSentSize.sample(bytes);
		}
		bitStream.reset();
	}
}

Replicator::ItemSender::ItemSender(Replicator& replicator, ConcurrentRakPeer *rakPeer)
	:replicator(replicator),rakPeer(rakPeer)
	,maxStreamSize(replicator.getAdjustedMtuSize())		// guesstimate
	,sentItems(false)
	,packetPriority(replicator.settings().getDataSendPriority())
{
}

Replicator::ItemSender::~ItemSender()
{
	closePacket();
}

Replicator::ItemSender::SendStatus Replicator::ItemSender::send(Item& item)
{

	if (bitStream && bitStream->GetNumberOfBytesUsed() >= static_cast<int>(maxStreamSize))
		return SEND_BITSTREAM_FULL;

	openPacket();

	if (!item.write(*bitStream))
		return SEND_BITSTREAM_FULL;

	sentItems = true;

	return SEND_OK;
}

int Replicator::ItemSender::getNumberOfBytesUsed() const
{
	if (bitStream)
		return bitStream->GetNumberOfBytesUsed();

	return 0;
}

}
}
