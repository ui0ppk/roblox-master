#include "Replicator.PingItem.h"

#include "Item.h"
#include "Replicator.h"
#include "ReplicatorStats.h"
#include "Security/FuzzyTokens.h"
#include "Security/ApiSecurity.h"
#include "V8DataModel/HackDefines.h"

#include "FastLog.h"
#include "V8DataModel/DataModel.h"

#include "RakNetTime.h"
#include "BitStream.h"

#include "VMProtect/VMProtectSDK.h"

namespace RBX {
namespace Network {

DeserializedPingItem::DeserializedPingItem() : extraStats(0)
{
	type = Item::ItemTypePing;
}

void DeserializedPingItem::process(Replicator& replicator)
{
	replicator.readDataPingItem(this);
}

Replicator::PingItem::PingItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats)
		: PooledItem(*replicator), time(time), extraStats(extraStats)
	{}

bool Replicator::PingItem::write(RakNet::BitStream& bitStream) {
#if !defined(RBX_STUDIO_BUILD)
	VMProtectBeginVirtualization(NULL);
#endif

    boost::scoped_ptr<unsigned int> moreStatsCopy(new unsigned int);

    int byteStart = bitStream.GetNumberOfBytesUsed();

    writeItemType(bitStream, ItemTypePing);

    unsigned int moreStats = 0;
#if !defined(LOVE_ALL_ACCESS) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
	unsigned int sendStats = Tokens::simpleToken | DataModel::perfStats | DataModel::sendStats |
			DataModel::get(&replicator)->allHackFlagsOredTogether();
    // I'm sorry for this.  The values need to be spread out in memory and thus can't be an array.
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag0);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag1);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag2);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag3);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag4);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag5);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag6);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag7);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag8);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag9);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag10);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag11);
    moreStats |= RBX::Security::getHackFlag<LINE_RAND4>(RBX::Security::hackFlag12);
    moreStats |= sendStats;
    *moreStatsCopy = moreStats;
#endif
    bitStream << false;
    bitStream << time;
    bitStream.Write(static_cast<uint32_t>(moreStats));
    if (replicator.canUseProtocolVersion(34))
    {
#if !defined(RBX_RCC_SECURITY) && !defined(RBX_STUDIO_BUILD)
        if (time & 0x20)
        {
            extraStats = ~extraStats;
        }
#endif
        bitStream.Write(static_cast<uint32_t>(extraStats));
    }

    if (replicator.settings().trackDataTypes) {
        replicator.replicatorStats.incrementPacketsSent(ReplicatorStats::PACKET_TYPE_Ping);
        replicator.replicatorStats.samplePacketsSent(ReplicatorStats::PACKET_TYPE_Ping, bitStream.GetNumberOfBytesUsed()-byteStart);
    }

#if !defined(RBX_STUDIO_BUILD)
    if (*moreStatsCopy != moreStats)
    {
        RBX::Tokens::apiToken.addFlagSafe(RBX::kPingItem); // can be changed to addFlagFast later.
    }
	VMProtectEnd();
#endif
	return true;
}

shared_ptr<DeserializedItem> Replicator::PingItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedPingItem> deserializedData(new DeserializedPingItem());

	int start = inBitstream.GetReadOffset();

	inBitstream >> deserializedData->pingBack;
	inBitstream >> deserializedData->time;
	inBitstream >> deserializedData->sendStats;
    if (replicator.canUseProtocolVersion(34))
    {
        inBitstream >> deserializedData->extraStats;
#ifdef RBX_RCC_SECURITY
        if (deserializedData->time & 0x20 && !replicator.isCloudEdit()) // change things up
        {
            deserializedData->extraStats = ~deserializedData->extraStats;
        }
#endif
    }

	if (replicator.settings().trackDataTypes) {
		replicator.replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_Ping);
		replicator.replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_Ping, (inBitstream.GetReadOffset()-start)/8);
	}

	return deserializedData;
}

}
}
