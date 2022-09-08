#pragma once

#include "ConcurrentRakPeer.h"
#include "Item.h"
#include "v8datamodel/Stats.h"

namespace RBX {
namespace Network {

struct ReplicatorStats {

    struct PhysicsPacketDetailedStats {
        // Packet break down

        // Level 1: Mechanism or MechanismCFrame
        TotalCountTimeInterval<> mechanismCFrame;
        RunningAverage<int> mechanismCFrameSize;
        TotalCountTimeInterval<> mechanism;
        RunningAverage<int> mechanismSize;

        // Level 1: Character
        TotalCountTimeInterval<> characterAnim;
        RunningAverage<int> characterAnimSize;

        // Level 2
        TotalCountTimeInterval<> translation;
        RunningAverage<int> translationSize;
        TotalCountTimeInterval<> rotation;
        RunningAverage<int> rotationSize;
        TotalCountTimeInterval<> velocity;
        RunningAverage<int> velocitySize;

        PhysicsPacketDetailedStats();

        void CreateStatsItems(Stats::Item* parent) const;
    };

	struct PhysicsSenderStats {
		RunningAverageTimeInterval<> physicsPacketsSent;
		RunningAverageTimeInterval<> physicsPacketsSentSmooth;
		RunningAverage<double> physicsSentThrottle;
		RunningAverage<int> physicsPacketsSentSize;
		RunningAverage<int> physicsItemsPerPacket;
        RunningAverageTimeInterval<> touchPacketsSent;
        RunningAverage<int> touchPacketsSentSize;
        RunningAverage<int> waitingTouches;

        PhysicsPacketDetailedStats details;
        
        PhysicsSenderStats();
	};
	struct PhysicsReceiverStats {

        PhysicsPacketDetailedStats details;

		PhysicsReceiverStats();
	};

	enum PacketType {
		PACKET_TYPE_InstanceNew = 0,
		PACKET_TYPE_InstanceDelete,
		PACKET_TYPE_Ping,
		PACKET_TYPE_CategoryData,
		PACKET_TYPE_CategoryBehavior,
		PACKET_TYPE_CategoryState,
		PACKET_TYPE_CategoryAppearance,
		PACKET_TYPE_CategoryTeam,
		PACKET_TYPE_CategoryVideo,
		PACKET_TYPE_CategoryControl,
		PACKET_TYPE_Event,
		PACKET_TYPE_MAX,
	};
	static const char* kPacketTypeNames[PACKET_TYPE_MAX];

    typedef TotalCountTimeInterval<> PacketByTypeMap[PACKET_TYPE_MAX];
    typedef RunningAverage<int> PacketSizeByTypeMap[PACKET_TYPE_MAX];

	PacketByTypeMap dataPacketSentTypes;
	PacketByTypeMap dataPacketReceiveTypes;
    PacketSizeByTypeMap dataPacketSizeSentTypes;
    PacketSizeByTypeMap dataPacketSizeReceiveTypes;

    // Incoming packets
	double kiloBytesReceivedPerSecond;

    RunningAverageTimeInterval<> packetsReceived;

	RunningAverageTimeInterval<> dataPacketsReceived;
    RunningAverage<double> dataPacketsReceivedSize;

	RunningAverageTimeInterval<> physicsPacketsReceived;
    RunningAverage<double> physicsPacketsReceivedSize;

	RunningAverageTimeInterval<> clusterPacketsReceived;
    RunningAverage<double> clusterPacketsReceivedSize;

    RunningAverageTimeInterval<> touchPacketsReceived;
    RunningAverage<double> touchPacketsReceivedSize;

   double kiloBytesSentPerSecond;

	// 0 to 1
	// 1: good, nothing in buffer, room to grow. 0: bad, buffer is increasing, should probably send less data
	double bufferHealth;

	RunningAverageTimeInterval<> dataPacketsSent;
	RunningAverage<double> dataPacketSendThrottle;
	RunningAverage<int> dataPacketsSentSize;
	RunningAverage<double> dataTimeInQueue;
	TotalCountTimeInterval<int> dataNewItemsPerSec;
	TotalCountTimeInterval<int> dataItemsSentPerSec;
	RunningAverageTimeInterval<> clusterPacketsSent;
	RunningAverage<int> clusterPacketsSentSize;

	int numberOfUnsplitMessages;
	int numberOfSplitMessages;
	RunningAverage<int> dataPing;
	PhysicsSenderStats physicsSenderStats;
	PhysicsReceiverStats physicsReceiverStats;

	// WARNING!
    // THIS VALUE IS UPDATED ASYNCHRONOUSLY FROM ANOTHER THREAD.
    // INDIVIDUAL INT VALUES WILL LIKELY BE INTERNALLY CONSISTENT, BUT
    // PAIRS OF FIELDS MAY NOT BE VALID IN CONJUNCTION
	ConnectionStats peerStats;

	RakNet::Time lastReceivedPingTime;
    RakNet::Time lastReceivedHashTime;
    RakNet::Time lastReceivedMccTime;
	Time lastStatsSampleTime;
	unsigned long lastBitsSent;
	unsigned long lastBitsReceived;
	Item::ItemType lastItem;
	int lastPacketType;

    ReplicatorStats();

	void incrementPacketsSent(PacketType type);
	void incrementPacketsSent(const std::string& categoryName);
    void samplePacketsSent(PacketType type, int size);
    void samplePacketsSent(const std::string& categoryName, int size);

	void incrementPacketsReceived(PacketType type);
	void incrementPacketsReceived(const std::string& categoryName);
    void samplePacketsReceived(PacketType type, int size);
    void samplePacketsReceived(const std::string& categoryName, int size);

    void resetSecurityTimes();

};

}
}