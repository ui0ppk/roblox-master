#pragma once

#include "Replicator.h"
#include "V8DataModel/Stats.h"
#include "util/profiling.h"
#include "RakNetStatistics.h"
#include "ConcurrentRakPeer.h"
#include "InterpolatingPhysicsReceiver.h"
#include "GetTime.h"

using namespace RakNet;

FASTFLAG(RemoveInterpolationReciever)

#define STATS_ITEM_VERSION 2

namespace RBX { namespace Network {

class DeserializedStatsItem : public DeserializedItem
{
public:
	shared_ptr<Reflection::ValueTable> stats;

	DeserializedStatsItem() { type = Item::ItemTypeStats; }
	~DeserializedStatsItem() {}

	/*implement*/ void process(Replicator& replicator);
};

// For replication
class Replicator::StatsItem : public Item
{
	int version;

public:
	StatsItem(Replicator* replicator, int version) : version(version), Item(*replicator) {}
	/*implement*/virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);

};

class RakStatsItem : public Stats::Item
{
	const RakNet::RakNetStatistics* statistics;

	template<class T>
	void addBuffer(const char* name, T* data)
	{
		Item * item = createChildItem(name);
		item->createBoundChildItem("IMMEDIATE_PRIORITY", data[IMMEDIATE_PRIORITY]);
		item->createBoundChildItem("HIGH_PRIORITY", data[HIGH_PRIORITY]);
		item->createBoundChildItem("MEDIUM_PRIORITY", data[MEDIUM_PRIORITY]);
		item->createBoundChildItem("LOW_PRIORITY", data[LOW_PRIORITY]);
	}
public:
	RakStatsItem(const RakNet::RakNetStatistics* statistics)
		:statistics(statistics)
	{
		setName("Stats");
		
		createBoundChildItem("messageDataBytesSentPerSec", statistics->valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED]);
		createBoundChildItem("messageTotalBytesSentPerSec", statistics->valueOverLastSecond[USER_MESSAGE_BYTES_SENT]);
		createBoundChildItem("messageDataBytesResentPerSec", statistics->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT]);
		createBoundChildItem("messagesBytesReceivedPerSec", statistics->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_PROCESSED]);
		createBoundChildItem("messagesBytesReceivedAndIgnoredPerSec", statistics->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_IGNORED]);
		createBoundChildItem("bytesSentPerSec", statistics->valueOverLastSecond[ACTUAL_BYTES_SENT]);
		createBoundChildItem("bytesReceivedPerSec", statistics->valueOverLastSecond[ACTUAL_BYTES_RECEIVED]);

		createBoundChildItem("totalMessageBytesPushed", statistics->runningTotal[USER_MESSAGE_BYTES_PUSHED]);
		createBoundChildItem("totalMessageBytesSent", statistics->runningTotal[USER_MESSAGE_BYTES_SENT]);
		createBoundChildItem("totalMessageBytesResent", statistics->runningTotal[USER_MESSAGE_BYTES_RESENT]);
		createBoundChildItem("totalMessagesBytesReceived", statistics->runningTotal[USER_MESSAGE_BYTES_RECEIVED_PROCESSED]);
		createBoundChildItem("totalMessagesBytesReceivedAndIgnored", statistics->runningTotal[USER_MESSAGE_BYTES_RECEIVED_IGNORED]);
		createBoundChildItem("totalBytesSent", statistics->runningTotal[ACTUAL_BYTES_SENT]);
		createBoundChildItem("totalBytesReceived", statistics->runningTotal[ACTUAL_BYTES_RECEIVED]);

		createBoundChildItem("connectionStartTime", statistics->connectionStartTime);

		createBoundChildItem("outgoingBandwidthLimitBytesPerSecond", statistics->BPSLimitByOutgoingBandwidthLimit);
		createBoundChildItem("isLimitedByOutgoingBandwidthLimit", statistics->isLimitedByOutgoingBandwidthLimit);
		createBoundChildItem("congestionControlLimitBytesPerSecond", statistics->BPSLimitByCongestionControl);
		createBoundChildItem("isLimitedByCongestionControl", statistics->isLimitedByCongestionControl);

		addBuffer("messageSendBuffer", statistics->messageInSendBuffer);
		addBuffer("bytesInSendBuffer", statistics->bytesInSendBuffer);

		createBoundChildItem("messagesInResendQueue", statistics->messagesInResendBuffer);
		createBoundChildItem("bytesInResendQueue", statistics->bytesInResendBuffer);

		createBoundChildItem("packetlossLastSecond", statistics->packetlossLastSecond);
		createBoundChildItem("packetlossTotal", statistics->packetlossTotal);		
	}
};


class Replicator::Stats : public RBX::Stats::Item
{
	Stats::Item* incomingPacketQueue;
	Stats::Item* waitingRefs;
	Stats::Item* pendingItemsSize;
	Stats::Item* ping;
	Stats::Item* elapsedTime;
	Stats::Item* mtuSize;
	Stats::Item* bufferHealth;
	Stats::Item* bandwidthExceeded;
	Stats::Item* congestionControlExceeded;
	Stats::Item* instanceSize;
	Stats::Item* incomingPacketsSize;
	Stats::Item* characterPacketsSent;
	Stats::Item* characterPacketsReceived;
	Stats::Item* dataPacketsSent;
	Stats::Item* dataPacketsSentTypes;
	Stats::Item* dataPacketSendThrottle;
	Stats::Item* dataPacketsReceived;
    Stats::Item* dataPacketsReceivedSize;
	Stats::Item* dataPacketsReceivedTypes;
	Stats::Item* dataTimeInQueue;
	Stats::Item* dataNewItemsPerStep;
	Stats::Item* dataItemsSentPerStep;
	Stats::Item* clusterPacketsSent;
	Stats::Item* clusterPacketsReceived;
    Stats::Item* clusterPacketsReceivedSize;
	Stats::Item* touchPacketsSent;
    Stats::Item* touchPacketsReceived;
    Stats::Item* touchPacketsReceivedSize;
	Stats::Item* physicsPacketsSent;
	Stats::Item* physicsPacketsSentThrottle;
	Stats::Item* physicsPacketsSentSmooth;
	Stats::Item* physicsItemsPerPacket;
	Stats::Item* physicsPacketsReceived;
    Stats::Item* physicsPacketsReceivedSize;
	Stats::Item* physicsReceiverMaxBufferDepth;
	Stats::Item* physicsReceiverAverageBufferDepth;
	Stats::Item* physicsReceiverAverageLag;
	Stats::Item* physicsReceiverOutOfOrderMechanisms;
    Stats::Item* packetsReceived;
    Stats::Item* physicsPacketDetailedStats_sent;
    Stats::Item* physicsPacketDetailedStats_received;
	Stats::Item* maxPacketloss;
    
	void addStat(const ReplicatorStats& stats, const char* name,
			const ReplicatorStats::PacketType type) {
        
        Item* sentItem = dataPacketsSentTypes->createBoundChildItem(name, stats.dataPacketSentTypes[type]);
        sentItem->createBoundChildItem("Size", stats.dataPacketSizeSentTypes[type]);
        
        Item* receivedItem = dataPacketsReceivedTypes->createBoundChildItem(name, stats.dataPacketReceiveTypes[type]);
        receivedItem->createBoundChildItem("Size", stats.dataPacketSizeReceiveTypes[type]);

    }
    
protected:
	const weak_ptr<const Replicator> replicator;
public:
	size_t instanceCount;
	size_t instanceBits;

	Stats(const shared_ptr<const Replicator>& replicator)
		:replicator(replicator)
		,instanceCount(0)
		,instanceBits(0)
	{
		const ReplicatorStats& stats = replicator->stats();

		ping = createChildItem("Ping");
		createBoundChildItem("Data Ping", stats.dataPing);
		Creatable<Instance>::create<RakStatsItem>(&stats.peerStats.rakStats)->setParent(this);

		// TODO: kbps trait for formatting!
		Item* send = createBoundChildItem("Send kBps", stats.kiloBytesSentPerSecond);
		send->createBoundChildItem("Unsplit Messages", stats.numberOfUnsplitMessages);
		send->createBoundChildItem("Split Messages", stats.numberOfSplitMessages);

		bufferHealth = createBoundChildItem("Send Buffer Health", stats.bufferHealth);

		bandwidthExceeded = createChildItem("BandwidthExceeded");
		congestionControlExceeded = createChildItem("CongestionControlExceeded");

		createBoundChildItem("Receive kBps", stats.kiloBytesReceivedPerSecond);
		incomingPacketQueue = createChildItem("Packet Queue");

		dataPacketsSent = createChildItem("Sent Data Packets");
			dataPacketsSent->createBoundChildItem("Size", stats.dataPacketsSentSize);
			dataPacketSendThrottle = dataPacketsSent->createChildItem("Throttle");
			pendingItemsSize = dataPacketsSent->createChildItem("Queue Size");
			dataTimeInQueue = dataPacketsSent->createChildItem("Time In Queue");
			dataNewItemsPerStep = dataPacketsSent->createBoundChildItem("New Items Per Sec", stats.dataNewItemsPerSec);
			dataItemsSentPerStep = dataPacketsSent->createBoundChildItem("Items Sent Per Sec", stats.dataItemsSentPerSec);

        physicsPacketDetailedStats_sent = createChildItem("OutPhysicsDetails");
        stats.physicsSenderStats.details.CreateStatsItems(physicsPacketDetailedStats_sent);

        physicsPacketDetailedStats_received = createChildItem("InPhysicsDetails");
        stats.physicsReceiverStats.details.CreateStatsItems(physicsPacketDetailedStats_received);

		dataPacketsSentTypes = createChildItem("Send Data Types");

		dataPacketsReceivedTypes = createChildItem("Received Data Types");

		addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_InstanceNew],        ReplicatorStats::PACKET_TYPE_InstanceNew);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_InstanceDelete],     ReplicatorStats::PACKET_TYPE_InstanceDelete);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_Ping],               ReplicatorStats::PACKET_TYPE_Ping);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryData],       ReplicatorStats::PACKET_TYPE_CategoryData);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryBehavior],   ReplicatorStats::PACKET_TYPE_CategoryBehavior);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryState],      ReplicatorStats::PACKET_TYPE_CategoryState);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryAppearance], ReplicatorStats::PACKET_TYPE_CategoryAppearance);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryTeam],       ReplicatorStats::PACKET_TYPE_CategoryTeam);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryVideo],      ReplicatorStats::PACKET_TYPE_CategoryVideo);
        addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_CategoryControl],    ReplicatorStats::PACKET_TYPE_CategoryControl);
		addStat(stats, ReplicatorStats::kPacketTypeNames[ReplicatorStats::PACKET_TYPE_Event],			   ReplicatorStats::PACKET_TYPE_Event);

		physicsPacketsSent = createChildItem("Sent Physics Packets");
			physicsPacketsSent->createBoundChildItem("Size",
                stats.physicsSenderStats.physicsPacketsSentSize);
			physicsPacketsSentThrottle = physicsPacketsSent->createChildItem("Throttle");
			physicsPacketsSentSmooth = physicsPacketsSent->createChildItem("Smoothed");
			physicsItemsPerPacket = physicsPacketsSent->createBoundChildItem("Items Per Packet", stats.physicsSenderStats.physicsItemsPerPacket);

		touchPacketsSent = createChildItem("SentTouchPackets");
			touchPacketsSent->createBoundChildItem("Size",
                stats.physicsSenderStats.touchPacketsSentSize);
			touchPacketsSent->createBoundChildItem("WaitingTouches",
                stats.physicsSenderStats.waitingTouches);

        packetsReceived = createChildItem("Received Packets");

		dataPacketsReceived = createChildItem("Received Data Packets");
			incomingPacketsSize = dataPacketsReceived->createChildItem("Queue Size");
			instanceSize = dataPacketsReceived->createChildItem("Instance Size");
			waitingRefs = dataPacketsReceived->createChildItem("Waiting Refs");
            dataPacketsReceivedSize = dataPacketsReceived->createChildItem("Size");

		physicsPacketsReceived = createChildItem("Received Physics Packets");
			physicsReceiverAverageLag = physicsPacketsReceived->createChildItem("Average Lag");
			physicsReceiverAverageBufferDepth = physicsPacketsReceived->createChildItem("Average Buffer Seek");
			physicsReceiverMaxBufferDepth = physicsPacketsReceived->createChildItem("Max Buffer Seek");
			physicsReceiverOutOfOrderMechanisms = physicsPacketsReceived->createChildItem("Wrong Order");
            physicsPacketsReceivedSize = physicsPacketsReceived->createChildItem("Size");

		clusterPacketsSent = createChildItem("Sent Cluster Packets");
		clusterPacketsSent->createBoundChildItem("Size", stats.clusterPacketsSentSize);

		clusterPacketsReceived = createChildItem("Received Cluster Packets");
            clusterPacketsReceivedSize = clusterPacketsReceived->createChildItem("Size");

        touchPacketsReceived = createChildItem("Received Touch Packets");
            touchPacketsReceivedSize = touchPacketsReceived->createChildItem("Size");

		mtuSize = send->createChildItem("MtuSize");
		elapsedTime = createChildItem("ElapsedTime");
		maxPacketloss = createChildItem("MaxPacketLoss");
	}

	static void accumulateValue(shared_ptr<Instance> instance, double* value)
	{
		if (Stats::Item* item = instance->fastDynamicCast<Stats::Item>())
		{
			*value += item->getValue();
		}
	}
	virtual void update() {

		if(shared_ptr<const Replicator> locked = replicator.lock())
		{
			if(!FFlag::RemoveInterpolationReciever)
			{
				if (InterpolatingPhysicsReceiver* receiver = dynamic_cast<InterpolatingPhysicsReceiver*>(locked->physicsReceiver.get()))
				{
					physicsReceiverMaxBufferDepth->formatValue(receiver->getMaxBufferSeek());
					physicsReceiverAverageBufferDepth->formatValue(receiver->getAverageBufferSeek());
					physicsReceiverAverageLag->formatValue(receiver->getAverageLag());
					physicsReceiverOutOfOrderMechanisms->formatPercent(receiver->outOfOrderMechanisms.value());
				}
			}

			const ReplicatorStats& stats = locked->stats();
            
            packetsReceived->formatRate(stats.packetsReceived);

			dataPacketsSent->formatRate(stats.dataPacketsSent);
			dataPacketSendThrottle->formatPercent(stats.dataPacketSendThrottle.value());
			dataPacketsReceived->formatRate(stats.dataPacketsReceived);
            dataPacketsReceivedSize->formatValue(stats.dataPacketsReceivedSize.value());

			double total = 0;
			dataPacketsSentTypes->visitChildren(boost::bind(&accumulateValue, _1, &total));
			dataPacketsSentTypes->formatValue(total);
			
			total = 0;
			dataPacketsReceivedTypes->visitChildren(boost::bind(&accumulateValue, _1, &total));
			dataPacketsReceivedTypes->formatValue(total);

			clusterPacketsSent->formatRate(stats.clusterPacketsSent);
			clusterPacketsReceived->formatRate(stats.clusterPacketsReceived);
            clusterPacketsReceivedSize->formatValue(stats.clusterPacketsReceivedSize.value());

			physicsPacketsSent->formatRate(stats.physicsSenderStats.physicsPacketsSent);
			physicsPacketsSentThrottle->formatPercent(stats.physicsSenderStats.physicsSentThrottle.value());
			physicsPacketsSentSmooth->formatRate(stats.physicsSenderStats.physicsPacketsSentSmooth);
			physicsPacketsReceived->formatRate(stats.physicsPacketsReceived);
            physicsPacketsReceivedSize->formatValue(stats.physicsPacketsReceivedSize.value());

			touchPacketsSent->formatRate(stats.physicsSenderStats.touchPacketsSent);
            touchPacketsReceived->formatRate(stats.touchPacketsReceived);
            touchPacketsReceivedSize->formatValue(stats.touchPacketsReceivedSize.value());

            if (locked->rakPeer)
			{
				bandwidthExceeded->formatPercent(locked->rakPeer->GetBandwidthExceeded(locked->remotePlayerId));
				congestionControlExceeded->formatPercent(locked->rakPeer->GetCongestionControlExceeded(locked->remotePlayerId));
				ping->formatValue(stats.peerStats.lastPing, "%d avg:%d best:%d"
					, stats.peerStats.lastPing
					, stats.peerStats.averagePing
					, stats.peerStats.lowestPing
				);
				mtuSize->formatValue(stats.peerStats.mtuSize);
				elapsedTime->formatValue((RakNet::GetTimeUS() - stats.peerStats.rakStats.connectionStartTime) / 1e6f);
				incomingPacketQueue->formatValue(locked->incomingPacketsCount());
				waitingRefs->formatValue(locked->numWaitingRefs());
				pendingItemsSize->formatValue(locked->pendingItems.size());
				dataTimeInQueue->formatValue(stats.dataTimeInQueue.value());
				incomingPacketsSize->formatValue(locked->incomingPackets.size());
				instanceSize->formatMem(instanceCount ? (size_t)(instanceBits/(8*instanceCount)) : 0);
				maxPacketloss->formatValue(stats.peerStats.maxPacketloss);
			}
		}
	}
};

}}
