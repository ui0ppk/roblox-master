#include "ReplicatorStats.h"

#include "V8Tree/Property.h"

//RakNet
#include "GetTime.h"

namespace RBX {
namespace Network {

static double lerp = 0.05;
static double lerp_smooth = 0.01;

ReplicatorStats::PhysicsSenderStats::PhysicsSenderStats() :
    physicsPacketsSent(lerp)
	,physicsPacketsSentSmooth(lerp_smooth)
	,physicsPacketsSentSize(lerp)
	,touchPacketsSent(lerp)
	,touchPacketsSentSize(lerp)
	,waitingTouches(lerp)
    {}

ReplicatorStats::PhysicsReceiverStats::PhysicsReceiverStats()
	{}

ReplicatorStats::PhysicsPacketDetailedStats::PhysicsPacketDetailedStats() :
    mechanismCFrame()
    ,mechanismCFrameSize(lerp)
    ,mechanism()
    ,mechanismSize(lerp)
    ,translation()
    ,translationSize(lerp)
    ,rotation()
    ,rotationSize(lerp)
    ,velocity()
    ,velocitySize(lerp)
    ,characterAnim()
    ,characterAnimSize(lerp)
    {}

void ReplicatorStats::PhysicsPacketDetailedStats::CreateStatsItems(Stats::Item* parent) const
{
    Stats::Item* tempChild;
    tempChild = parent->createBoundChildItem("1) character", characterAnim);
        tempChild->createBoundChildItem("Size", characterAnimSize);
    tempChild = parent->createBoundChildItem("2) cframeOnly", mechanismCFrame);
        tempChild->createBoundChildItem("Size", mechanismCFrameSize);
    tempChild = parent->createBoundChildItem("2) mechanism", mechanism);
        tempChild->createBoundChildItem("Size", mechanismSize);
    tempChild = parent->createBoundChildItem("3) translation", translation);
        tempChild->createBoundChildItem("Size", translationSize);
    tempChild = parent->createBoundChildItem("3) rotation", rotation);
        tempChild->createBoundChildItem("Size", rotationSize);
    tempChild = parent->createBoundChildItem("3) velocity", velocity);
        tempChild->createBoundChildItem("Size", velocitySize);
}

const char* ReplicatorStats::kPacketTypeNames[PACKET_TYPE_MAX] = {
	"InstanceNew",
	"InstanceDelete",
	"Ping",
	category_Data,
	category_Behavior,
	category_State,
	category_Appearance,
	category_Team,
	category_Video,
	category_Control,
	"Events"
};
    
ReplicatorStats::ReplicatorStats() :
	dataPacketSentTypes()
	,dataPacketReceiveTypes()
    ,kiloBytesReceivedPerSecond(lerp)
	,kiloBytesSentPerSecond(lerp)
	,bufferHealth(1.0)
    ,packetsReceived(lerp)
	,dataPacketsReceived(lerp)
	,physicsPacketsReceived(lerp)
	,clusterPacketsReceived(lerp)
	,dataPacketsSent(lerp)
	,dataPacketsSentSize(lerp)
	,clusterPacketsSent(lerp)
	,clusterPacketsSentSize(lerp)
    ,touchPacketsReceived(lerp)
    ,touchPacketsReceivedSize(lerp)
    ,numberOfUnsplitMessages(0)
    ,numberOfSplitMessages(0)
    ,dataPing(0.25)
    ,physicsSenderStats()
    ,peerStats()
    ,lastReceivedPingTime(RakNet::GetTimeMS())
    ,lastReceivedHashTime(RakNet::GetTimeMS())
    ,lastReceivedMccTime(RakNet::GetTimeMS())
    ,lastStatsSampleTime()
    ,lastBitsSent(0)
    ,lastBitsReceived(0)
    ,lastItem(Item::ItemTypeEnd)
	,lastPacketType(-1)
	{}

void ReplicatorStats::incrementPacketsSent(PacketType type) {
	dataPacketSentTypes[type].increment();
}

void ReplicatorStats::incrementPacketsSent(const std::string& categoryName) {
	for (unsigned int i = 0; i < PACKET_TYPE_MAX; ++i) {
		if (categoryName == kPacketTypeNames[i]) {
			incrementPacketsSent((PacketType)i);
			return;
		}
	}
}

void ReplicatorStats::samplePacketsSent(PacketType type, int size) {
    dataPacketSizeSentTypes[type].sample(size);
}

void ReplicatorStats::samplePacketsSent(const std::string& categoryName, int size) {
    for (unsigned int i = 0; i < PACKET_TYPE_MAX; ++i) {
        if (categoryName == kPacketTypeNames[i]) {
            samplePacketsSent((PacketType)i, size);
            return;
        }
    }
}

void ReplicatorStats::incrementPacketsReceived(PacketType type) {
	dataPacketReceiveTypes[type].increment();
}

void ReplicatorStats::incrementPacketsReceived(const std::string& categoryName) {
	for (unsigned int i = 0; i < PACKET_TYPE_MAX; ++i) {
		if (categoryName == kPacketTypeNames[i]) {
			incrementPacketsReceived((PacketType)i);
			return;
		}
	}
}

void ReplicatorStats::samplePacketsReceived(PacketType type, int size) {
    dataPacketSizeReceiveTypes[type].sample(size);
}

void ReplicatorStats::samplePacketsReceived(const std::string& categoryName, int size) {
    for (unsigned int i = 0; i < PACKET_TYPE_MAX; ++i) {
        if (categoryName == kPacketTypeNames[i]) {
            samplePacketsReceived((PacketType)i, size);
            return;
        }
    }
}

    void ReplicatorStats::resetSecurityTimes()
    {
        const RakNet::Time now = RakNet::GetTimeMS();
        lastReceivedPingTime = now;
        lastReceivedHashTime = now;
        lastReceivedMccTime = now;
    }

}
}
