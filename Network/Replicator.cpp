
#include "Replicator.h"

#include "ConcurrentRakPeer.h"
#include "Replicator.ChangePropertyItem.h"
#include "Replicator.DeleteInstanceItem.h"
#include "Replicator.EventInvocationItem.h"
#include "Replicator.ItemSender.h"
#include "Replicator.JoinDataItem.h"
#include "Replicator.MarkerItem.h"
#include "Replicator.NewInstanceItem.h"
#include "Replicator.PingBackItem.h"
#include "Replicator.PingItem.h"
#include "Replicator.PingJob.h"
#include "Replicator.ProcessPacketsJob.h"
#include "Replicator.ReferencePropertyChangedItem.h"
#include "Replicator.RockyItem.h"
#include "Replicator.SendDataJob.h"
#include "Replicator.StreamJob.h"
#include "Replicator.StatsItem.h"
#include "Replicator.TagItem.h"
#include "Replicator.ItemSender.h"

#include "Marker.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/InsertService.h"
#include "V8DataModel/Hopper.h"
#include "V8Datamodel/Lighting.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/ChatService.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/PartOperation.h"
#include "V8DataModel/MarketplaceService.h"
#include "v8datamodel/BadgeService.h"
#include "v8datamodel/Test.h"
#include "V8DataModel/SpawnLocation.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/HttpService.h"
#include "Util/SoundService.h"
#include "Util/ProtectedString.h"
#include "Util/BrickColor.h"
#include "Util/ProgramMemoryChecker.h"
#include "Util/UDim.h"
#include "Util/Faces.h"
#include "Util/Axes.h"
#include "util/PhysicalProperties.h"
#include "V8DataModel/Teams.h"
#include "v8datamodel/MegaCluster.h"
#include "V8DataModel/TouchTransmitter.h"
#include "V8DataModel/Test.h"
#include "v8datamodel/ReplicatedStorage.h"
#include "v8datamodel/RobloxReplicatedStorage.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/LogService.h"
#include "v8datamodel/PointsService.h"
#include "v8datamodel/AdService.h"
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/NumberRange.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/ServerStorage.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "v8datamodel/NonReplicatedCSGDictionaryService.h"

#include "V8World/ContactManager.h"

#include "Security/SecurityContext.h"

#include "V8World/MotorJoint.h"
#include "V8World/Assembly.h"

#include "V8Tree/Service.h"
#include "Util/Quaternion.h"

#include "Streaming.h"
#include "GuidRegistryService.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "NetworkSettings.h"
#include "ErrorCompPhysicsSender.h"
#include "ErrorCompPhysicsSender2.h"
#include "RoundRobinPhysicsSender.h"
#include "TopNErrorsPhysicsSender.h"
#include "DirectPhysicsReceiver.h"
#include "InterpolatingPhysicsReceiver.h"
#include "Network/NetworkPacketCache.h"
#include "Network/NetworkClusterPacketCache.h"

// RakNet
#include "mtusize.h"
#include "BitStream.h"
#include "RakPeer.h"
#include "RakPeerInterface.h"
#include "GetTime.h"

#include "Util/ScopedAssign.h"
#include "Util/StandardOut.h"
#include "Util/Math.h"
#include "util/RobloxGoogleAnalytics.h"

#include "FastLog.h"
#include <boost/format.hpp> 

#include "NetworkProfiler.h"
#include "v8datamodel/TeleportService.h"

#include "voxel/Serializer.h"

#include "voxel2/Grid.h"
#include "voxel2/BitSerializer.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/copy.hpp>

#include "rbx/Profiler.h"

DYNAMIC_LOGGROUP(NetworkJoin)

LOGGROUP(MegaClusterNetwork)
LOGGROUP(MegaClusterNetworkInit)

LOGVARIABLE(NetworkStatsReport, 0)
LOGVARIABLE(NetworkStepsMultipliers, 0);
LOGGROUP(ReplicationDataLifetime)
LOGGROUP(NetworkInstances)
LOGGROUP(NetworkReadItem)
LOGGROUP(TerrainCellListener)
DYNAMIC_LOGVARIABLE(NetworkPacketsReceive, 0)

DYNAMIC_FASTFLAG(DebugDisableTimeoutDisconnect)
DYNAMIC_FASTFLAGVARIABLE(US25317p1, true)
DYNAMIC_FASTFLAGVARIABLE(US25317p2, true)
DYNAMIC_LOGGROUP(MaxJoinDataSizeKB)
FASTFLAGVARIABLE(DebugProtocolSynchronization, false)
FASTFLAGVARIABLE(RemoveUnusedPhysicsSenders, false)
FASTFLAGVARIABLE(RemoveInterpolationReciever, false)

DYNAMIC_FASTINTVARIABLE(MaxClusterKBPerSecond, 40)
DYNAMIC_FASTINT(RakNetMaxSplitPacketCount)

DYNAMIC_FASTINTVARIABLE(MaxDataPacketPerSend, 1)

DYNAMIC_FASTINTVARIABLE(PacketErrorInfluxHundredthsPercentage, 10000)

DYNAMIC_FASTFLAGVARIABLE(WhiteListChatFilter, false)

DYNAMIC_FASTFLAGVARIABLE(ReadDeSerializeProcessFlow, true)
DYNAMIC_FASTFLAGVARIABLE(ExplicitlyAssignDefaultPropVal, false)

FASTFLAGVARIABLE(FilterSinglePass, false)
FASTFLAGVARIABLE(FilterDoublePass, false)

namespace RBX {
	namespace Network {

const unsigned short CLUSTER_END_TOKEN = 0xffff;
const unsigned short CLUSTER_DATA_TOKEN = 0xfffe;


// average delta size:
// 1 bits for chunk changed
// rarely 1 bit for eom
// rarely 10 bits for new chunk address
// always 14 bits for position-within-chunk
// always 8 bits for content
// always 1 bit for material-present ?
// sometimes 8 bits for material
// --------------------------------------------
// approximately ~ 32 bits (4 bytes) on average per delta
const int kApproximateSizeOfVoxelDelta = 4;

// delta is ~100-200 bits on avg for 4^3 block
const int kApproximateSizeOfSmoothDelta = 16;

REFLECTION_BEGIN();
static Reflection::EventDesc<Replicator, void(std::string, bool)> event_Disconnection(&Replicator::disconnectionSignal, "Disconnection", "peer", "lostConnection", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, shared_ptr<Instance>()> func_SendMarker(&Replicator::sendMarker, "SendMarker", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, void()> func_requestCharacter(&Replicator::requestCharacter, "RequestCharacter", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, void()> func_closeConnection(&Replicator::closeConnection, "CloseConnection", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, shared_ptr<Instance>()> prop_RemotePlayer(&Replicator::getPlayer, "GetPlayer", Security::None);
static Reflection::BoundFuncDesc<Replicator, std::string(int)> func_getRakStatsString(&Replicator::getRakStatsString, "GetRakStatsString", "verbosityLevel", 0, Security::Plugin);
static Reflection::BoundFuncDesc<Replicator, void()> func_DisableProcessPackets(&Replicator::disableProcessPackets, "DisableProcessPackets", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, void()> func_EnableProcessPackets(&Replicator::enableProcessPackets, "EnableProcessPackets", Security::LocalUser);
static Reflection::BoundFuncDesc<Replicator, void(double)> func_SetPropSyncExpiration(&Replicator::setPropSyncExpiration, "SetPropSyncExpiration", "seconds", Security::LocalUser);

static Reflection::PropDescriptor<Replicator, int> prop_port("Port", category_Data, &Replicator::getPort, NULL, Reflection::PropertyDescriptor::UI, Security::LocalUser);
static Reflection::PropDescriptor<Replicator, std::string> prop_ip("MachineAddress", category_Data, &Replicator::getIpAddress, NULL, Reflection::PropertyDescriptor::UI, Security::LocalUser);
REFLECTION_END();

RBX::Time Replicator::remoteRaknetTimeToLocalRbxTime(const RemoteTime& time)
{
	return time - Time::Interval(rakTimeOffset);
}

RBX::Time Replicator::raknetTimeToRbxTime(const RakNet::Time& time)
{
	RBX::RemoteTime t((double)time / 1000.0f);
	return t - Time::Interval(rakTimeOffset);
}

RakNet::Time Replicator::rbxTimeToRakNetTime(const RBX::Time& time)
{
	return (RakNet::Time)(time + Time::Interval(rakTimeOffset)).timestampSeconds();
}

void Replicator::writeInstance(shared_ptr<Instance> instance, RakNet::BitStream* outBitstream)
{
	NewInstanceItem item(this, instance);
	item.write(*outBitstream);
}

bool Replicator::isPropertyCacheable(const Reflection::Type& type)
{
    bool isRef = Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(type);

	// ******** Enum type should always be cacheable! See ClientReplicator::ProcessOutdatedProperties ******** //
    if (type.isType<std::string>() || 
        type.isType<RBX::ProtectedString>() || 
        type.isType<RBX::BinaryString>() ||
        type.isType<RBX::SystemAddress>() ||
		type.isType<RBX::ContentId>() ||
        isRef)
    {
        return false;
    }

    return true;
}

bool Replicator::isPropertyCacheable(const Reflection::Type* type, bool isEnum)
{
    // Enum types have to be cacheable so that we can handle unknown enums (type == NULL)
    if (isEnum)
        return true;
    
    RBXASSERT(type);
    return isPropertyCacheable(*type);
}

void Replicator::writeProperties(const Instance* instance, RakNet::BitStream& outBitstream, PropertyCacheType cacheType, bool useDictionary)
{
    // if you make changes here, make sure ClientReplicator::writeProperties() also has it
 
    RBX::Reflection::ConstPropertyIterator iter = instance->properties_begin();
    RBX::Reflection::ConstPropertyIterator end = instance->properties_end();
    while (iter!=end)
    {
        Reflection::ConstProperty property = *iter;

		writePropertiesInternal(instance, property, outBitstream, cacheType ,useDictionary);
        ++iter;
    }
}

void Replicator::writePropertiesInternal(const Instance* instance, const Reflection::ConstProperty& property, RakNet::BitStream& outBitstream, PropertyCacheType cacheType, bool useDictionary)
{
	const Reflection::PropertyDescriptor& descriptor = property.getDescriptor();

	bool cacheable = (cacheType != PropertyCacheType_NonCacheable);

	if ((descriptor.canReplicate() || (isCloudEdit() && isCloudEditReplicateProperty(descriptor)))
		&& (cacheType == PropertyCacheType_All || isPropertyCacheable(descriptor.type) == cacheable)
		&& (descriptor != Instance::propParent)) // don't write the parent now
	{

		if (!isPropertyRemoved(instance, descriptor.name))
		{	
			const Instance* defaultInstance = getDefault(instance->getClassName());

			if (descriptor.type.isType<bool>())
			{
				// optimization for bool properties
				serializePropertyValue(property, outBitstream, useDictionary);
		
				if (settings().printBits) {
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
						"   write %s %s, 1 bit", 
						descriptor.type.name.c_str(),
						descriptor.name.c_str());
				}
			}
			else if (defaultInstance && descriptor.equalValues(instance, defaultInstance))
			{
				outBitstream << true;

				if (settings().printBits) {
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
						"   write %s %s, 1 bit", 
						descriptor.type.name.c_str(),
						descriptor.name.c_str());
				}
			}
			else
			{
				const int start = outBitstream.GetWriteOffset();

				outBitstream << false;
				serializePropertyValue(property, outBitstream, useDictionary);

				if (settings().printBits) {
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
						"   write %s %s, %d bit", 
						descriptor.type.name.c_str(),
						descriptor.name.c_str(),
						outBitstream.GetWriteOffset() - start);
				}
			}
		}
	}

}

ReplicatorJob::ReplicatorJob(const char* name, Replicator& replicator, TaskType taskType)
:DataModelJob(name, taskType, true, shared_from(DataModel::get(&replicator)), Time::Interval(0)) 
	,replicator(shared_from(&replicator))
{
}

bool ReplicatorJob::canSendPacket(shared_ptr<Replicator>& safeReplicator, PacketPriority packetPriority)
{
	if (safeReplicator) {
		return safeReplicator->getBufferCountAvailable(
			safeReplicator->settings().canSendPacketBufferLimit, packetPriority) > 0;
	} else {
		return false;
	}
}

int Replicator::getBufferCountAvailable(int bufferLimit, PacketPriority priority)
{
	const RakNetStatistics* statistics = getRakNetStats();
	if (!statistics) {
		return 0;
	}

	if (statistics->isLimitedByOutgoingBandwidthLimit &&
			settings().isThrottledByOutgoingBandwidthLimit) {
		return 0;
	}

	if (statistics->isLimitedByCongestionControl &&
			settings().isThrottledByCongestionControl) {
		return 0;
	}

	int remainingDataPackets = bufferLimit;
	remainingDataPackets -= statistics->messageInSendBuffer[priority];

	return remainingDataPackets >= 0 ? remainingDataPackets : 0;
}

const char* const sReplicator = "NetworkReplicator";


Replicator::Replicator(
	   RakNet::SystemAddress remotePlayerId, 
		boost::shared_ptr<ConcurrentRakPeer> rakPeer,
		NetworkSettings* networkSettings,
		bool clusterDebounceEnabled)
	:rakPeer(rakPeer)
	,remotePlayerId(remotePlayerId)
	,clusterDebounceEnabled(clusterDebounceEnabled)
	,removingInstance(NULL)
	,players(NULL)
	,deserializingProperty(NULL)
	,deserializingEventInvocation(NULL)
	,serializingInstance(NULL)
	,megaClusterInstance(NULL)
	,isOkToSendClusters(false)
	,isClusterSpotted(false)
	,approximateSizeOfPendingClusterDeltas(0)
	,clusterDebounce(false)
	,processAllPacketsPerStep(false)
	,networkSettings(networkSettings)
    ,streamingEnabled(false)
	,numItemsLeftFromPrevStep(0)
    ,protocolSyncEnabled(false)
    ,apiDictionaryCompression(false)
    ,connected(true)
    ,enableHashCheckBypass(true)
    ,enableMccCheckBypass(true)
	,canTimeout(false)
	,packetReceivedEvent(false)
	,deserializePacketsThreadEnabled(false)
{
	if (rakPeer)
	{
		rakPeer->addStats(remotePlayerId,
                          boost::bind(&Replicator::onStatisticsChanged, this, _1));
		rakPeer->rawPeer()->AttachPlugin(this);
	}

    RakNet::Time rakNow = RakNet::GetTime();
    rakTimeOffset = (double)rakNow/1000.0f - Time::nowFastSec();

	FASTLOGS(FLog::Network, "Replicator created for player %s", RakNetAddressToString(remotePlayerId).c_str());
	FASTLOG1(FLog::Network, "Replicator created: %p", this);
}

int Replicator::getPort() const
{
	return remotePlayerId.GetPort();
}
std::string Replicator::getIpAddress() const
{
	return RakNetAddressToString(remotePlayerId, false);
}

void Replicator::pushIncomingPacket(Packet* packet)
{
	if (deserializePacketsThread)
	{
		{
			boost::mutex::scoped_lock lock(receivedPacketsMutex);
			receivedPackets.push_back(packet);
		}
		packetReceivedEvent.Set();
	}
	else
	{
		bool wasEmpty = incomingPackets.empty();
		incomingPackets.push(packet);
		if (wasEmpty && processPacketsJob && !TaskScheduler::singleton().isCyclicExecutive())
			TaskScheduler::singleton().reschedule(processPacketsJob);
	}
}

void Replicator::createPhysicsReceiver(NetworkSettings::PhysicsReceiveMethod method, bool isServer)
{
	if(FFlag::RemoveInterpolationReciever)
	{
		RBXASSERT(false);
	}
	else
	{			

		RBXASSERT(!physicsReceiver);
		switch (method)
		{
		case NetworkSettings::Interpolation:
			{
	//			RBXASSERT(!settings().distributedPhysicsEnabled);
				physicsReceiver.reset(new InterpolatingPhysicsReceiver(this, isServer));
				break;
			}
		case NetworkSettings::Direct:
			{
				physicsReceiver.reset(new DirectPhysicsReceiver(this, isServer));
				break;
			}
		}
		physicsReceiver->start(physicsReceiver);
	}
}

void Replicator::clearIncomingPackets()
{
	Packet* packet;
	while (incomingPackets.pop_if_present(packet))
	{
		rakPeer->DeallocatePacket(packet);
	}
}

Replicator::~Replicator()
{
	RBXASSERT(replicationContainers.size()==0);

	RBXASSERT(!rakPeer);

	FASTLOG1(FLog::Network, "Replicator destroyed: %p", this);
}

bool Replicator::isTopContainer(const Instance* instance)
{
	const Instance* parent = instance->getParent();
	if(!parent)
		return false;

	return parent->getParent() == NULL;
}

void Replicator::addTopReplicationContainer(Instance* instance, bool replicateProperties, bool replicateChildren,
											boost::function<void (shared_ptr<Instance>)> replicationMethodFunc)
{
	RBXASSERT(instance);

	topReplicationContainersMap.insert(
		std::make_pair<const Reflection::ClassDescriptor*, TopReplConts::iterator>(&instance->getDescriptor(), topReplicationContainers.insert(topReplicationContainers.end(), instance)));

	if (replicateChildren || replicateProperties)
		addReplicationData(shared_from(instance), replicateProperties, replicateChildren);

	if (replicateChildren) {
		instance->visitChildren(boost::bind(&Replicator::onChildAdded, this, _1, replicationMethodFunc));
	}
}

void Replicator::addToPendingItemsList(shared_ptr<Instance> instance)
{
	pendingItems.push_back(new (newInstancePool.get()) NewInstanceItem(this, instance));
}

bool Replicator::disconnectReplicationData(shared_ptr<Instance> instance)
{
	if (!instance)
		return false;

	RepConts::iterator iter = replicationContainers.find(instance.get());
	if (iter==replicationContainers.end())
		return false;

	FASTLOG1(FLog::ReplicationDataLifetime, "Removing instance replication data: %p", instance.get());

	if (instance.get() == megaClusterInstance)
	{
		disconnectClusterReplicationData();
	}

	instance->visitChildren(boost::bind(&Replicator::disconnectReplicationData, this, _1));

	closeReplicationItem(iter->second);

	replicationContainers.erase(iter);

	return true;
}

void Replicator::disconnectClusterReplicationData()
{
	if (megaClusterInstance && megaClusterInstance->isInitialized())
	{
		if (megaClusterInstance->isSmooth())
			megaClusterInstance->getSmoothGrid()->disconnectListener(this);
		else
			megaClusterInstance->getVoxelGrid()->disconnectListener(this);
	}

	megaClusterInstance = NULL;
	approximateSizeOfPendingClusterDeltas = 0;

	clusterReplicationData.parentConnection.disconnect();

	clusterReplicationData = ClusterReplicationData();
}

void Replicator::closeReplicationItem(ReplicationData& item)
{
	item.connection.disconnect();
	if (item.deleteOnDisconnect)
		item.instance->setParent(NULL);
}

std::string Replicator::getRakStatsString(int verbosityLevel)
{
	char buffer[10000];
	buffer[0] = 0;
	StatisticsToString(getRakNetStats(), buffer, verbosityLevel);
	return buffer;
}

const RakNetStatistics* Replicator::getRakNetStats() const
{
	return rakPeer ? &(replicatorStats.peerStats.rakStats) : 0;
}

ReplicatorStats::PhysicsSenderStats& Replicator::physicsSenderStats() {
    return replicatorStats.physicsSenderStats;
}

TaskScheduler::Job::Error Replicator::SendDataJob::error(const Stats& stats)
{
	Error result;
	result.error = 0;
	if(replicator){
        if (!canSendPacket(replicator, packetPriority)) {
            return result;
        }
		if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
		{
			if (!replicator->highPriorityPendingItems.empty() || !replicator->pendingItems.empty())
			{
				result.error = 1.0f;
			}
			else
			{
				result.error = 0.0f;
			}
		}
		else if (replicator->settings().isQueueErrorComputed)
		{
			if (replicator->highPriorityPendingItems.size())
			{
				Time::Interval waitTime = replicator->highPriorityPendingItems.head_wait();
				result.error = waitTime.seconds() * dataSendRate;
				result.error *= std::min<int>(10, replicator->highPriorityPendingItems.size() + replicator->pendingItems.size() + 1) / 2.0;
			}
			else
			{
				// TODO: Good error metric!
				Time::Interval waitTime = replicator->pendingItems.head_wait();
				result.error = waitTime.seconds() * dataSendRate;
				result.error *= std::min<int>(10, replicator->pendingItems.size() + 1) / 2.0;
			}
		}
		else
		{
			if (!replicator->pendingItems.empty() || !replicator->highPriorityPendingItems.empty()) {
				result.error = stats.timespanSinceLastStep.seconds() * dataSendRate;
			}
		}
	}
	return result;
}

TaskScheduler::Job::Error Replicator::SendClusterJob::error(const Stats& stats)
{
	// Overview of cluster error strategy:
	// 1. If we can't send any packets yet then use 0 error to avoid being
	//    scheduled
	// 2. If the QueueErrorComputed flag is on:
	//    * estimate the number of deltas that can be sent per packet
	//    * count how many deltas we have to send
	//    * (standard error is timeSinceLastCompute * desired herz)
	//    * if we have more deltas than we can send in one packet, multiply
	//      the error (read: desired herz) by the ratio of
	//      (deltas to send) / (deltas per packet)
	//
	// Why this is a reasonable approximation:
	// If 2 packets worth of deltas have queued up in the time between sends,
	// then we want to send packets twice as frequently. Generalized, if we have N
	// packets worth of deltas queued up then we want to send packets N times
	// as frequently.

	Error result(computeStandardError(stats, dataSendRate));
	if(replicator){
        if (!canSendPacket(replicator, packetPriority)) {
            result.error = 0;
            return result;
        }
		if (replicator->settings().isQueueErrorComputed)
		{
			if (int maxBytesSend = replicator->getAdjustedMtuSize()) {
				// assuming there's just one cluster for now
				int deltas = replicator->getApproximateSizeOfPendingClusterDeltas();

				float multiplier = 1.0;
				if (deltas > maxBytesSend) {
					multiplier *= deltas;
					multiplier /= maxBytesSend;
				}
				// assert we aren't multiplying by more than if the entire cluster is queued
				RBXASSERT(multiplier <= std::max(1, (int)deltas));
				result.error *= multiplier;
			}
		}
	}
	return result;
}


void Replicator::clusterOutStep()
{
	try
	{
		sendClusterPacket();
	}
	catch (...)
	{
		FASTLOG(FLog::Network, "Exception during cluster out step, request disconnect");
		requestDisconnectWithSignal(DisconnectReason_SendPacketError);
		throw;
	}
}



void Replicator::dataOutStep()
{
	try
	{
		sendItemsPacket();
	}
	catch (...)
	{
		FASTLOG(FLog::Network, "Exception during data out step, request disconnect");	
		requestDisconnectWithSignal(DisconnectReason_SendPacketError);
		throw;
	}

	// If cluster creation command went out, set flag to allow sending cluster packets
	if(isClusterSpotted && !isOkToSendClusters)
	{
		clusterReplicationData.readyToSendChunks = true;
		isOkToSendClusters = true;
	}
};

bool Replicator::shouldStreamingHandleOnAddedForChild(shared_ptr<const Instance> child) {
	if (streamJob)
	{
        if (shared_ptr<const PartInstance> partInstance = Instance::fastSharedDynamicCast<const PartInstance>(child))
        {
			if (!child->isDescendantOf(ServiceProvider::find<Workspace>(this)))
			{
				// do not stream instances not under workspace
				return false;
			}
			else if (isInstanceAChildOfClientsCharacterModel(child.get()))
            {
                // do not stream character parts
                return false;
            }
            else if (Instance::fastSharedDynamicCast<const MegaClusterInstance>(child))
            {
                // do not stream mega cluster instance
                return false;
            }
            if (streamJob->getReady())
            {
                if (streamJob->isInStreamedRegions(partInstance->getConstPartPrimitive()->getExtentsWorld()))
                {
                    // do not stream it if stream job is ready (because we want to keep the order of instance creation and property change)
                    return false;
                }
                else
                {
                    // out of streamed regions, bypass by returning true (so it won't be handled at all)
                    return true;
                }
            }
            else
            {
                // stream job not ready and it is not part of character or mega cluster instance,
                // we want to queue this to the initial data of streaming, so we return true to let stream job handle it
                return true;
            }
        }
        else
        {
            // not a part instance, do not care
            return false;
        }
	}
	else
	{
        // not streaming, not interested
		return false;
	}
}

bool Replicator::isInStreamedRegions(const Extents& ext) const
{
	return streamJob ? streamJob->isInStreamedRegions(ext) : true;
}

bool Replicator::isAreaInStreamedRadius(const Vector3& center, float radius) const
{
	return streamJob ? streamJob->isAreaInStreamedRadius(center, radius) : true;
}

bool Replicator::isInstanceAChildOfClientsCharacterModel(const Instance* testInstance) const {
	const Player* player = findTargetPlayer();
	const ModelInstance* playerCharacterModel = player ? player->getConstCharacter() : 0;
	return playerCharacterModel && testInstance->isDescendantOf(playerCharacterModel);
}

void Replicator::addTopReplicationContainers(ServiceProvider* newProvider)
{
	FASTLOG1F(DFLog::NetworkJoin, "addTopReplicationContainers started at %f s", Time::nowFastSec());

	JoinDataItem *joinDataItem = new JoinDataItem(this);
	joinDataItem->setBytesPerStep(DFLog::MaxJoinDataSizeKB * 1000 /* convert to bytes */);
	boost::function<void (shared_ptr<Instance>)> replicationMethodFunc = boost::bind(&Replicator::JoinDataItem::addInstance, joinDataItem, _1);
	pendingItems.push_back(joinDataItem);

	// this should always be added first, as this is the first container we receive over the wire
	addTopReplicationContainer(ServiceProvider::create<ReplicatedFirst>(newProvider), false, isCloudEdit(), replicationMethodFunc);
	
	addTopReplicationContainer(ServiceProvider::create<RBX::Lighting>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::Soundscape::SoundService>(newProvider), true, false, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::StarterPackService>(newProvider), false, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::StarterGuiService>(newProvider), isCloudEdit(), true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::StarterPlayerService>(newProvider), isCloudEdit(), true, replicationMethodFunc);

	addTopReplicationContainer(ServiceProvider::create<RBX::CSGDictionaryService>(newProvider), false, true, replicationMethodFunc);

	FASTLOG1F(DFLog::NetworkJoin, "addTopReplicationContainer for Workspace called @ %f s", Time::nowFastSec());
	addTopReplicationContainer(ServiceProvider::find<RBX::Workspace>(newProvider), true, true, replicationMethodFunc);
	FASTLOG1F(DFLog::NetworkJoin, "addTopReplicationContainer for Workspace ended @ %f s", Time::nowFastSec());
	FASTLOG2(DFLog::NetworkJoin, "addTopReplicationContainers JoinDataItem(0x%p) size %d", joinDataItem, joinDataItem ? joinDataItem->size() : 0);

	addTopReplicationContainer(ServiceProvider::create<RBX::JointsService>(newProvider), false, true, replicationMethodFunc);
	addTopReplicationContainer(players = ServiceProvider::create<RBX::Network::Players>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::Teams>(newProvider), false, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::InsertService>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::ChatService>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::FriendService>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::MarketplaceService>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::BadgeService>(newProvider), true, false, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::ReplicatedStorage>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::RobloxReplicatedStorage>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<RBX::TestService>(newProvider), true, true, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<LogService>(newProvider), true, false, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<PointsService>(newProvider), true, false, replicationMethodFunc);
	addTopReplicationContainer(ServiceProvider::create<AdService>(newProvider), true, false, replicationMethodFunc);

	if (isCloudEdit())
	{
		addTopReplicationContainer(newProvider->create<ServerScriptService>(), true, true, replicationMethodFunc);
		addTopReplicationContainer(newProvider->create<ServerStorage>(), false, true, replicationMethodFunc);
		addTopReplicationContainer(newProvider->create<NonReplicatedCSGDictionaryService>(), false, true, replicationMethodFunc);
		addTopReplicationContainer(newProvider->create<HttpService>(), true, true, replicationMethodFunc);
	}

	FASTLOG1F(DFLog::NetworkJoin, "addTopReplicationContainers ended at %f s", Time::nowFastSec());
}

bool Replicator::canReplicateInstance(Instance* instance, int replicationProtocolVersion)
{
	FASTLOG2(FLog::Network, "Replicator:canReplicateInstance - start, instance is %s, replicationProtocolVersion = %i",instance->getName().c_str(),replicationProtocolVersion);

	if(Instance::fastDynamicCast<RBX::ReplicatedFirst>(instance))
		return (replicationProtocolVersion >= 25);
	if (Instance::fastDynamicCast<RBX::AdService>(instance))
		return (replicationProtocolVersion >= 26);

	return true;
}

void Replicator::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (deserializePacketsThread)
	{
		RBXASSERT(deserializePacketsThreadEnabled);
		deserializePacketsThreadEnabled = false;
		packetReceivedEvent.Set();
		deserializePacketsThread->join();
		deserializePacketsThread.reset();
	}

	while(!replicationContainers.empty())
	{
		ReplicationData item = replicationContainers.begin()->second;
		replicationContainers.erase(replicationContainers.begin());
		closeReplicationItem(item);
	}
	
	replicationContainers.clear();

	disconnectClusterReplicationData();

	if (instancePacketCache)
		instancePacketCache.reset();

	if (clusterPacketCache)
		clusterPacketCache.reset();

	if (oneQuarterClusterPacketCache)
		oneQuarterClusterPacketCache.reset();
	
	// must call this before freeing memory from pool
	pendingItems.deleteAll();
	highPriorityPendingItems.deleteAll();
	
	// free memory from pool
	newInstancePool.reset();
	deleteInstancePool.reset();
	changePropertyPool.reset();
	eventInvocationPool.reset();
	pingPool.reset();
	pingBackPool.reset();
	referencePropertyChangedPool.reset();

	topReplicationContainers.clear();
	topReplicationContainersMap.clear();

	physicsSender.reset();

	TaskScheduler::singleton().remove(sendDataJob);
	sendDataJob.reset();

	TaskScheduler::singleton().remove(sendClusterJob);
	sendClusterJob.reset();

	TaskScheduler::singleton().remove(processPacketsJob);
	processPacketsJob.reset();

	TaskScheduler::singleton().remove(pingJob);
	pingJob.reset();

	Super::onServiceProvider(oldProvider, newProvider);

	// Destroys the exiting stats items if created
	updateStatsItem(ServiceProvider::find<RBX::Stats::StatsService>(newProvider));

	if(oldProvider) {
		RBX::DataModel* oldDM = static_cast<RBX::DataModel*>(oldProvider);
		oldDM->setNetworkMetric(NULL);
	}

	if (newProvider)
	{
		sendDataJob = shared_ptr<SendDataJob>(new SendDataJob(*this));
		TaskScheduler::singleton().add(sendDataJob);

		sendClusterJob = shared_ptr<SendClusterJob>(new SendClusterJob(*this));
		TaskScheduler::singleton().add(sendClusterJob);

		processPacketsJob = shared_ptr<ProcessPacketsJob>(new ProcessPacketsJob(*this));
		TaskScheduler::singleton().add(processPacketsJob);

		pingJob = shared_ptr<PingJob>(new PingJob(*this));
		TaskScheduler::singleton().add(pingJob);

		instancePacketCache = shared_from(ServiceProvider::find<InstancePacketCache>(newProvider));
		oneQuarterClusterPacketCache = shared_from(ServiceProvider::find<OneQuarterClusterPacketCache>(newProvider));
		clusterPacketCache = shared_from(ServiceProvider::find<ClusterPacketCache>(newProvider));
		
		// initialize memory pool
		newInstancePool.reset(new AutoMemPool(sizeof(NewInstanceItem)));
		deleteInstancePool.reset(new AutoMemPool(sizeof(DeleteInstanceItem)));
		changePropertyPool.reset(new AutoMemPool(sizeof(ChangePropertyItem)));
		eventInvocationPool.reset(new AutoMemPool(sizeof(EventInvocationItem)));
		pingPool.reset(new AutoMemPool(sizeof(PingItem)));
		pingBackPool.reset(new AutoMemPool(sizeof(PingBackItem)));
		referencePropertyChangedPool.reset(new AutoMemPool(sizeof(ReferencePropertyChangedItem)));

		addTopReplicationContainers(newProvider);

		RBX::DataModel* newDM = static_cast<RBX::DataModel*>(newProvider);
		newDM->setNetworkMetric(this);

		Players *players = ServiceProvider::find<Players>(newProvider);
		if (players)
		{
			sendFilteredChatMessageConnection = players->sendFilteredChatMessageSignal.connect(
				boost::bind(&Replicator::sendFilteredChatMessage, shared_from(this), _1, _2, _3, _4, _5));
		}
	}

	if (!newProvider)
	{
		sendFilteredChatMessageConnection.disconnect();

		if (rakPeer)
		{
			clearIncomingPackets();

			boost::shared_ptr<ConcurrentRakPeer> temp(rakPeer);
			rakPeer.reset();
			temp->removeStats(remotePlayerId);
			temp->rawPeer()->CloseConnection(remotePlayerId, true);
			temp->rawPeer()->DetachPlugin(this);
		}
	}
}

void Replicator::sendFilteredChatMessage(const RakNet::SystemAddress &systemAddress, const shared_ptr<RakNet::BitStream> &data, const shared_ptr<Instance> sourceInstance,
										 const std::string &whitelist, const std::string &blacklist)
{
	if (remotePlayerId == systemAddress)
		return;
	Player *player = findTargetPlayer();
	if (!player || Player::CHAT_FILTER_WHITELIST == player->getChatFilterType()) { // assume player's whitelisted
		if (0 == whitelist.length()) {
			// If filtering failed, we may have nothing to send whitelist users.
			// This simulates the older filtering method of where "under 13" users could
			// not send or receive any messages.
			return;
		}
	}
	else if (0 == blacklist.length())
	{
		return;
	}

	shared_ptr<RakNet::BitStream> dataCopy(new RakNet::BitStream());
	dataCopy->WriteBits(data->GetData(), data->GetNumberOfBitsUsed(), false);

	Player *sourcePlayer = Instance::fastDynamicCast<Player>(sourceInstance.get());	
	if(DFFlag::WhiteListChatFilter && sourcePlayer && Player::CHAT_FILTER_WHITELIST == sourcePlayer->getChatFilterType())
	{		
		*dataCopy << whitelist;	
	}
	else
	{
		if (player && player->isChatInfoValid()) 
		{
				switch (player->getChatFilterType()) 
				{
				case Player::CHAT_FILTER_BLACKLIST:
					*dataCopy << blacklist;
					break;
				case Player::CHAT_FILTER_WHITELIST:
					*dataCopy << whitelist;
					break;
				}
		} 
		else 
		{
			*dataCopy << whitelist;
		}
	}
	
	if (rakPeer)
		rakPeer->Send(dataCopy, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, remotePlayerId, false);
}

void Replicator::createPhysicsSender(NetworkSettings::PhysicsSendMethod physicsSendMethod)
{
	if(FFlag::RemoveUnusedPhysicsSenders)
	{
		RBXASSERT(false);
		// this method should be removed when the flag (RemoveUnusedPhysicsSenders) is accepted and removed
	}
	else
	{
		switch (physicsSendMethod)
		{
		case NetworkSettings::ErrorComputation:
			physicsSender.reset(new ErrorCompPhysicsSender(*this));
			break;
		case NetworkSettings::ErrorComputation2:
			physicsSender.reset(new ErrorCompPhysicsSender2(*this));
			break;
		case NetworkSettings::RoundRobin:
			physicsSender.reset(new RoundRobinPhysicsSender(*this));
			break;
		case NetworkSettings::TopNErrors:
			physicsSender.reset(new TopNErrorsPhysicsSender(*this));
			break;
		}
	
	PhysicsSender::start(physicsSender);
	}
}

double Replicator::incomingPacketsCountHeadWaitTimeSec(const RBX::Time& timeNow)
{
	if (deserializePacketsThread)
		return deserializedPackets.head_waittime_sec(timeNow);

	return incomingPackets.head_waittime_sec(timeNow);
}

size_t Replicator::incomingPacketsCount() const
{
	if (deserializePacketsThread)
		return deserializedPackets.size();

	return incomingPackets.size();
};

void Replicator::updateStatsItem(RBX::Stats::StatsService* stats)
{
	if (statsItem!=NULL)
	{
		statsItem->setParent(NULL);
		statsItem.reset();
	}
	
	if (stats!=NULL)
	{
		shared_ptr<Stats::Item> network = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network"));
		if (network)
		{
			statsItem = createStatsItem();
			statsItem->setName(RakNetAddressToString(this->remotePlayerId));
			statsItem->setParent2(network);
		}
	}
}

void Replicator::learnDictionaries(RakNet::BitStream& rawBitStream, bool compressed, bool learnSchema)
{
    RakNet::BitStream bitStream;
    RakNet::BitStream* inBitstream;
    if (compressed)
    {
        inBitstream = &bitStream;
        decompressBitStream(rawBitStream, *inBitstream);
    }
    else
    {
        inBitstream = &rawBitStream;
    }
    NETPROFILE_START("classDictionary.learn", inBitstream);
    classDictionary.learn(*inBitstream, learnSchema, compressed);
    NETPROFILE_END("classDictionary.learn", inBitstream);
    NETPROFILE_START("propDictionary.learn", inBitstream);
    propDictionary.learn(*inBitstream, learnSchema, compressed);
    NETPROFILE_END("propDictionary.learn", inBitstream);
    NETPROFILE_START("eventDictionary.learn", inBitstream);
    eventDictionary.learn(*inBitstream, learnSchema, compressed);
    NETPROFILE_END("eventDictionary.learn", inBitstream);
    NETPROFILE_START("typeDictionary.learn", inBitstream);
    typeDictionary.learn(*inBitstream, learnSchema, compressed);
    NETPROFILE_END("typeDictionary.learn", inBitstream);

    NETPROFILE_START("deserializeSFFlags", inBitstream);
    deserializeSFFlags(*inBitstream);
    NETPROFILE_END("deserializeSFFlags", inBitstream);
}

SharedStringDictionary& Replicator::getSharedEventDictionary(const Reflection::EventDescriptor& descriptor)
{
	EventStrings::iterator iter = eventStrings.find(&descriptor);
	if (iter==eventStrings.end())
	{
		shared_ptr<SharedStringDictionary> result(new SharedStringDictionary());
		eventStrings[&descriptor] = result;
		return *result;
	}
	return *iter->second;
}

SharedStringProtectedDictionary& Replicator::getSharedPropertyProtectedDictionary(const Reflection::PropertyDescriptor& descriptor)
{
	// Find (or create) a SharedStringDictionary associated with the PropertyDescriptor
	PropertyProtectedStrings::iterator iter = protectedStrings.find(&descriptor);
	if (iter==protectedStrings.end())
	{
		shared_ptr<SharedStringProtectedDictionary> result(new SharedStringProtectedDictionary(isProtectedStringEnabled()));
		protectedStrings[&descriptor] = result;
		return *result;
	}
	return *iter->second;
}

SharedStringDictionary& Replicator::getSharedPropertyDictionary(const Reflection::PropertyDescriptor& descriptor)
{
	// Find (or create) a SharedStringDictionary associated with the PropertyDescriptor
	PropertyStrings::iterator iter = strings.find(&descriptor);
	if (iter==strings.end())
	{
		shared_ptr<SharedStringDictionary> result(new SharedStringDictionary());
		strings[&descriptor] = result;
		return *result;
	}
	return *iter->second;
}

SharedBinaryStringDictionary& Replicator::getSharedPropertyBinaryDictionary(const Reflection::PropertyDescriptor& descriptor)
{
	// Find (or create) a SharedStringDictionary associated with the PropertyDescriptor
	PropertyBinaryStrings::iterator iter = binaryStrings.find(&descriptor);
	if (iter==binaryStrings.end())
	{
		shared_ptr<SharedBinaryStringDictionary> result(new SharedBinaryStringDictionary());
		binaryStrings[&descriptor] = result;
		return *result;
	}
	return *iter->second;
}

template<>
bool SenderDictionary<RBX::SystemAddress>::isDefaultValue(const RBX::SystemAddress& value)
{
	return value.empty();
}

template<>
void ReceiverDictionary<RBX::SystemAddress>::setDefault(RBX::SystemAddress& value)
{
	value.clear();
}

template<>
bool SenderDictionary<RBX::ContentId>::isDefaultValue(const RBX::ContentId& value)
{
	return value.isNull();
}

template<>
void ReceiverDictionary<RBX::ContentId>::setDefault(RBX::ContentId& value)
{
	value.clear();
}

template<>
bool SenderDictionary<RBX::Guid::Scope>::isDefaultValue(const RBX::Guid::Scope& value)
{
	return value.isNull();
}

template<>
void ReceiverDictionary<RBX::Guid::Scope>::setDefault(RBX::Guid::Scope& value)
{
	value.setNull();
}

void Replicator::serializeEventInvocation(const Reflection::EventInvocation& eventInvocation, RakNet::BitStream& outBitStream)
{
	const Reflection::SignatureDescriptor& signatureDescriptor = eventInvocation.event.getDescriptor()->getSignature();
	//First write out the number of arguments
	outBitStream << (int)signatureDescriptor.arguments.size();

	Reflection::EventArguments::const_iterator valueIter = eventInvocation.args.begin(); 
	for(std::list<Reflection::SignatureDescriptor::Item>::const_iterator typeIter = signatureDescriptor.arguments.begin(); 
		typeIter != signatureDescriptor.arguments.end(); 
		++typeIter, ++valueIter)
	{
		RBXASSERT(valueIter != eventInvocation.args.end());

		if ((*typeIter->type) == Reflection::Type::singleton<std::string>())
		{
			getSharedEventDictionary(*eventInvocation.event.getDescriptor()).serializeString(valueIter->cast<std::string>(), outBitStream);
		}	
		else {
            if (!serializeValue((*typeIter->type), (*valueIter), outBitStream)) {
                RBXASSERT(false);
            }
		}
	}
}

bool Replicator::serializeValue(const Reflection::Type& type, const Reflection::Variant& value, RakNet::BitStream& outBitStream)
{
    if (ProcessOutdatedEnumSerialization(type, value, outBitStream))
    {
        return true;
    }
	if (type==Reflection::Type::singleton<bool>())
	{
		serializeGeneric<bool>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<int>())
	{
		serializeGeneric<int>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<long>())
	{
        int v = value.cast<long>();
		outBitStream << v;
	}
	else if (type==Reflection::Type::singleton<float>())
	{
		serializeGeneric<float>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<double>())
	{
		serializeGeneric<double>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<std::string>())
	{
		serializeGeneric<std::string>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<BinaryString>())
	{
		serializeGeneric<BinaryString>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<UDim>())
	{
		serializeGeneric<UDim>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<UDim2>())
	{
		serializeGeneric<UDim2>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<RBX::RbxRay>())
	{
		serializeGeneric<RBX::RbxRay>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<Faces>())
	{
		serializeGeneric<Faces>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<Axes>())
	{
		serializeGeneric<Axes>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<BrickColor>())
	{
		serializeGeneric<BrickColor>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::Color3>())
	{
		serializeGeneric<G3D::Color3>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::Vector2>())
	{
		serializeGeneric<G3D::Vector2>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::Vector3>())
	{
		serializeGeneric<G3D::Vector3>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::Vector3int16>())
	{
		serializeGeneric<G3D::Vector3int16>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::Vector2int16>())
	{
		serializeGeneric<G3D::Vector2int16>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<G3D::CoordinateFrame>())
	{
		serializeGeneric<G3D::CoordinateFrame>(value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<RBX::Region3>())
	{
        Region3 data = value.cast<Region3>();

		outBitStream << data.minPos();
        outBitStream << data.maxPos();
	}
	else if (type==Reflection::Type::singleton<RBX::Region3int16>())
	{
        Region3int16 data = value.cast<Region3int16>();

		outBitStream << data.getMinPos();
        outBitStream << data.getMaxPos();
	}
	else if (type==Reflection::Type::singleton<RBX::SystemAddress>())
	{
		systemAddressDictionary.send(outBitStream, value.cast<RBX::SystemAddress>());
	}
	else if (type==Reflection::Type::singleton<RBX::ContentId>())
	{
		contentIdDictionary.send(outBitStream, value.cast<RBX::ContentId>());
	}
	else if (type==Reflection::Type::singleton<shared_ptr<RBX::Instance> >())
	{
		serializeInstanceRef(value.cast<shared_ptr<RBX::Instance > >().get(), outBitStream);
	}
	else if (const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(type)){
		serializeEnum(enumDesc, value, outBitStream);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::Tuple> >())
	{
        const RBX::Reflection::Tuple* data = value.cast<shared_ptr<const RBX::Reflection::Tuple> >().get();

        int size = data ? data->values.size() : 0;
        outBitStream << size;

        for (int i = 0; i < size; ++i)
            serializeVariant(data->values[i], outBitStream);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueArray> >())
	{
        const RBX::Reflection::ValueArray* data = value.cast<shared_ptr<const RBX::Reflection::ValueArray> >().get();

        int size = data->size();
        outBitStream << size;

        for (int i = 0; i < size; ++i)
            serializeVariant((*data)[i], outBitStream);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueTable> >())
	{
        const RBX::Reflection::ValueTable* data = value.cast<shared_ptr<const RBX::Reflection::ValueTable> >().get();

        int size = data->size();
        outBitStream << size;

        for (RBX::Reflection::ValueTable::const_iterator it = data->begin(); it != data->end(); ++it)
        {
            outBitStream << it->first;
            serializeVariant(it->second, outBitStream);
        }
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueMap> >())
	{
        const RBX::Reflection::ValueMap* data = value.cast<shared_ptr<const RBX::Reflection::ValueMap> >().get();

        int size = data->size();
        outBitStream << size;

        for (RBX::Reflection::ValueMap::const_iterator it = data->begin(); it != data->end(); ++it)
        {
            outBitStream << it->first;
            serializeVariant(it->second, outBitStream);
        }
	}
	else if (type == Reflection::Type::singleton<NumberSequence>())
    {
        serializeGeneric<NumberSequence>(value, outBitStream);
    }
    else if (type == Reflection::Type::singleton<NumberRange>())
    {
        serializeGeneric<NumberRange>(value, outBitStream);
    }
    else if (type == Reflection::Type::singleton<ColorSequence>())
    {
        serializeGeneric<ColorSequence>(value, outBitStream);
    }
    else if (type == Reflection::Type::singleton<NumberSequenceKeypoint>())
    {
        serializeGeneric<NumberSequenceKeypoint>(value, outBitStream);
    }
    else if (type == Reflection::Type::singleton<ColorSequenceKeypoint>())
    {
        serializeGeneric<ColorSequenceKeypoint>(value, outBitStream);
    }
	else if (type == Reflection::Type::singleton<Rect2D>())
	{
		serializeGeneric<Rect2D>(value, outBitStream);
	}
	else if (type == Reflection::Type::singleton<PhysicalProperties>())
	{
		serializeGeneric<PhysicalProperties>(value, outBitStream);
	}
    else 
    {
		return false;
	}
	return true;
}

void Replicator::serializeVariant(const Reflection::Variant& value, RakNet::BitStream& outBitStream)
{
    typeDictionary.send(outBitStream, typeDictionary.getId(&value.type()).id);

    serializeValue(value.type(), value, outBitStream);
}

void Replicator::serializePropertyValue(const Reflection::ConstProperty& property, RakNet::BitStream& outBitStream, bool useDictionary)
{
	if (property.getDescriptor().type==Reflection::Type::singleton<RBX::ProtectedString>())
	{
        const ProtectedString& value = property.getValue<ProtectedString>();
        std::string valueString = encodeProtectedString(value, static_cast<const Instance*>(property.getInstance()), property.getDescriptor());

        if (canUseProtocolVersion(28))
        {
            if (useDictionary)	
                getSharedPropertyBinaryDictionary(property.getDescriptor()).serializeString(BinaryString(valueString), outBitStream);
            else
                outBitStream << BinaryString(valueString);
        }
        else
        {
            if (useDictionary)	
                getSharedPropertyProtectedDictionary(property.getDescriptor()).serializeString(valueString, outBitStream);
            else
                outBitStream << valueString;
        }
    }
	else if (property.getDescriptor().type==Reflection::Type::singleton<std::string>())
	{
		if (useDictionary)	
			getSharedPropertyDictionary(property.getDescriptor()).serializeString(property, outBitStream);
		else
			outBitStream << property.getDescriptor().getStringValue(property.getInstance());
	}
    else if (property.getDescriptor().type==Reflection::Type::singleton<BinaryString>())
	{
		serialize<BinaryString>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<bool>())
	{
		serialize<bool>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<int>())
	{
		serialize<int>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<float>())
	{
		serialize<float>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<double>())
	{
		serialize<double>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<UDim>())
	{
		serialize<UDim>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<UDim2>())
	{
		serialize<UDim2>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::RbxRay>())
	{
		serialize<RBX::RbxRay>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<Faces>())
	{
		serialize<Faces>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<Axes>())
	{
		serialize<Axes>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<BrickColor>())
	{
		serialize<BrickColor>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Color3>())
	{
		serialize<G3D::Color3>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector2>())
	{
		serialize<G3D::Vector2>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector3>())
	{
		if (property.getDescriptor()==PartInstance::prop_Size)
		{
			writeBrickVector(outBitStream, property.getValue<G3D::Vector3>());
		}
		else
		{
			serialize<G3D::Vector3>(property, outBitStream);
		}
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector2int16>())
	{
		serialize<G3D::Vector2int16>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector3int16>())
	{
		serialize<G3D::Vector3int16>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::CoordinateFrame>())
	{
		serialize<G3D::CoordinateFrame>(property, outBitStream);
	}
	else if (property.getDescriptor().bIsEnum)
	{
        if (!ProcessOutdatedPropertyEnumSerialization(property, outBitStream))
        {
		    serializeEnumProperty(property, outBitStream);
        }
	}
	else if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(property.getDescriptor()))
	{
		const Reflection::RefPropertyDescriptor* desc = boost::polymorphic_downcast<const Reflection::RefPropertyDescriptor*>(&property.getDescriptor());
		RBX::Instance* instance = boost::polymorphic_downcast<Instance*>(desc->getRefValue(property.getInstance()));

		if (useDictionary)
		{
			serializeId(outBitStream, instance);
		}
		else
		{
			serializeIdWithoutDictionary(outBitStream, instance);
		}
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::ContentId>())
	{
		if (useDictionary)
			contentIdDictionary.send(outBitStream, property.getValue<RBX::ContentId>());
		else
			serialize<RBX::ContentId>(property, outBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::SystemAddress>())
	{
		if (useDictionary)
			systemAddressDictionary.send(outBitStream, property.getValue<RBX::SystemAddress>());
		else
			outBitStream << property.getValue<RBX::SystemAddress>();
	}
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberSequence>())
    {
        serialize<NumberSequence>(property, outBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberRange>())
    {
        serialize<NumberRange>(property, outBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<ColorSequence>())
    {
        serialize<ColorSequence>(property, outBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberSequenceKeypoint>())
    {
        serialize<NumberSequenceKeypoint>(property, outBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<ColorSequenceKeypoint>())
    {
        serialize<ColorSequenceKeypoint>(property, outBitStream);
    }
	else if (property.getDescriptor().type == Reflection::Type::singleton<Rect2D>())
	{
		serialize<Rect2D>(property, outBitStream);
	}
	else if (property.getDescriptor().type == Reflection::Type::singleton<PhysicalProperties>())
	{
		serialize<PhysicalProperties>(property, outBitStream);
	}
	else {
		// TODO:  This used to throw an assert - did this for now so 
		// we don't forget
		RBXASSERT(false);
	}
}

void Replicator::deserializeEventInvocation(RakNet::BitStream& inBitStream, Reflection::EventInvocation& eventInvocation, bool resolveRefTypes)
{
	int numEntries;
	inBitStream >> numEntries;

	const Reflection::SignatureDescriptor& signature = eventInvocation.event.getDescriptor()->getSignature();
	RBXASSERT(signature.arguments.size() == numEntries);

	eventInvocation.args.clear();
	for (Reflection::SignatureDescriptor::Arguments::const_iterator iter = signature.arguments.begin(); iter != signature.arguments.end(); ++iter)
    {
		const Reflection::Type& type = (*(*iter).type);

		Reflection::Variant argument;
		if (type == Reflection::Type::singleton<std::string>())
		{
			std::string value;
			getSharedEventDictionary(*eventInvocation.event.getDescriptor()).deserializeString(value, inBitStream);
			argument = value;
		}
		else if (!resolveRefTypes && type==Reflection::Type::singleton<shared_ptr<RBX::Instance> >())
		{
			// only store the guid id if not resolving reference types, we'll resolve it later when it's used
			RBX::Guid::Data id;
			deserializeId(inBitStream, id);
			argument = id;
		}
		else
        {
			if (!deserializeValue(inBitStream, type, argument))
            {
                RBXASSERT(false);
            }
		}
		eventInvocation.args.push_back(argument);
	}
}

bool Replicator::deserializeValue(RakNet::BitStream& inBitStream, const Reflection::Type& type,	Reflection::Variant& value)
{
	if (type==Reflection::Type::singleton<bool>())
		deserializeGeneric<bool>(value, inBitStream);
	else if (type==Reflection::Type::singleton<int>())
		deserializeGeneric<int>(value, inBitStream);
	else if (type==Reflection::Type::singleton<long>())
    {
        int v;
        inBitStream >> v;
		value = long(v);
    }
	else if (type==Reflection::Type::singleton<float>())
		deserializeGeneric<float>(value, inBitStream);
	else if (type==Reflection::Type::singleton<double>())
		deserializeGeneric<double>(value, inBitStream);
	else if (type==Reflection::Type::singleton<std::string>())
		deserializeGeneric<std::string>(value, inBitStream);
	else if (type==Reflection::Type::singleton<BinaryString>())
	{
		deserializeGeneric<BinaryString>(value, inBitStream);
	}
	else if (type==Reflection::Type::singleton<UDim>())
		deserializeGeneric<UDim>(value, inBitStream);
	else if (type==Reflection::Type::singleton<UDim2>())
		deserializeGeneric<UDim2>(value, inBitStream);
	else if (type==Reflection::Type::singleton<RBX::RbxRay>())
		deserializeGeneric<RBX::RbxRay>(value, inBitStream);
	else if (type==Reflection::Type::singleton<Faces>())
		deserializeGeneric<Faces>(value, inBitStream);
	else if (type==Reflection::Type::singleton<Axes>())
		deserializeGeneric<Axes>(value, inBitStream);
	else if (type==Reflection::Type::singleton<BrickColor>())
		deserializeGeneric<BrickColor>(value, inBitStream);
	else if (type==Reflection::Type::singleton<G3D::Color3>())
		deserializeGeneric<G3D::Color3>(value, inBitStream);
	else if (type ==Reflection::Type::singleton<G3D::Vector2>())
		deserializeGeneric<G3D::Vector2>(value, inBitStream);
	else if (type ==Reflection::Type::singleton<G3D::Vector3>())
		deserializeGeneric<G3D::Vector3>(value, inBitStream);
	else if (type==Reflection::Type::singleton<G3D::Vector3int16>())
		deserializeGeneric<G3D::Vector3int16>(value, inBitStream);
	else if (type==Reflection::Type::singleton<G3D::Vector2int16>())
		deserializeGeneric<G3D::Vector2int16>(value, inBitStream);
	else if (type==Reflection::Type::singleton<G3D::CoordinateFrame>())
		deserializeGeneric<G3D::CoordinateFrame>(value, inBitStream);
	else if (type==Reflection::Type::singleton<Region3>())
    {
        Vector3 min, max;
        inBitStream >> min;
        inBitStream >> max;

		value = Region3(Extents(min, max));
    }
	else if (type==Reflection::Type::singleton<Region3int16>())
    {
        Vector3int16 min, max;
        inBitStream >> min;
        inBitStream >> max;

		value = Region3int16(min, max);
    }
	else if (const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(type))
    {
        if (!ProcessOutdatedEnumDeserialization(inBitStream, type, value))
        {
		    deserializeEnum(enumDesc, value, inBitStream);
        }
    }
	else if (type==Reflection::Type::singleton<RBX::ContentId>())
	{
		RBX::ContentId valueContentId;
		contentIdDictionary.receive(inBitStream, valueContentId);
		value = valueContentId;
	}
	else if (type==Reflection::Type::singleton<shared_ptr<RBX::Instance> >())
	{
        shared_ptr<Instance> instance;
		deserializeInstanceRef(inBitStream, instance);
		value = instance;
	}
	else if (type==Reflection::Type::singleton<RBX::SystemAddress>())
	{
		RBX::SystemAddress valueAddress;
		systemAddressDictionary.receive(inBitStream, valueAddress);
		value=valueAddress;
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::Tuple> >())
	{
        shared_ptr<RBX::Reflection::Tuple> data(new RBX::Reflection::Tuple());

        int size;
        inBitStream >> size;

        for (int i = 0; i < size; ++i)
        {
            Reflection::Variant v;
            deserializeVariant(inBitStream, v);

            data->values.push_back(v);
        }
        
        value = shared_ptr<const Reflection::Tuple>(data);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueArray> >())
	{
        shared_ptr<RBX::Reflection::ValueArray> data(new RBX::Reflection::ValueArray());

        int size;
        inBitStream >> size;

        for (int i = 0; i < size; ++i)
        {
            Reflection::Variant v;
            deserializeVariant(inBitStream, v);

            data->push_back(v);
        }
        
        value = shared_ptr<const Reflection::ValueArray>(data);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueTable> >())
	{
        shared_ptr<RBX::Reflection::ValueTable> data(new RBX::Reflection::ValueTable());

        int size;
        inBitStream >> size;

        for (int i = 0; i < size; ++i)
        {
            std::string k;
            inBitStream >> k;

            Reflection::Variant v;
            deserializeVariant(inBitStream, v);

            (*data)[k] = v;
        }
        
        value = shared_ptr<const Reflection::ValueTable>(data);
	}
	else if (type==Reflection::Type::singleton<shared_ptr<const RBX::Reflection::ValueMap> >())
	{
        shared_ptr<RBX::Reflection::ValueMap> data(new RBX::Reflection::ValueMap());

        int size;
        inBitStream >> size;

        for (int i = 0; i < size; ++i)
        {
            std::string k;
            inBitStream >> k;

            Reflection::Variant v;
            deserializeVariant(inBitStream, v);

            (*data)[k] = v;
        }
        
        value = shared_ptr<const Reflection::ValueMap>(data);
	}
    else if (type == Reflection::Type::singleton<NumberSequence>())
    {
        deserializeGeneric<NumberSequence>(value, inBitStream);
    }
    else if (type == Reflection::Type::singleton<NumberRange>())
    {
        deserializeGeneric<NumberRange>(value, inBitStream);
    }
    else if (type == Reflection::Type::singleton<ColorSequence>())
    {
        deserializeGeneric<ColorSequence>(value, inBitStream);
    }
    else if (type == Reflection::Type::singleton<NumberSequenceKeypoint>())
    {
        deserializeGeneric<NumberSequenceKeypoint>(value, inBitStream);
    }
    else if (type == Reflection::Type::singleton<ColorSequenceKeypoint>())
    {
        deserializeGeneric<ColorSequenceKeypoint>(value, inBitStream);
    }
	else if (type == Reflection::Type::singleton<Rect2D>())
	{
		deserializeGeneric<Rect2D>(value, inBitStream);
	}
	else if (type == Reflection::Type::singleton<PhysicalProperties>())
	{
		deserializeGeneric<PhysicalProperties>(value, inBitStream);
	}
    else
    {
		return false;
    }

    return true;
}

void Replicator::deserializeVariant(RakNet::BitStream& inBitStream, Reflection::Variant& value)
{
    const Reflection::Type* type;
    typeDictionary.receive(inBitStream, type, false); // type is primitive description, never get outdated

    RBXASSERT(type);
    deserializeValue(inBitStream, *type, value);
}

void Replicator::RemoteCheatHelper2(weak_ptr<RBX::DataModel> weakDataModel)
{
	if(shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock()){
		if(RBX::Network::Player* player = Network::Players::findLocalPlayer(dataModel.get())){
			player->reportStat("rocky");
		}
	}
}

void Replicator::deserializePropertyValue(RakNet::BitStream& inBitStream, Reflection::Property property, bool useDictionary, bool preventBounceBack, Reflection::Variant* outValue)
{
    // NOTE:
    // Please make corresponding change in ClientReplicator::skipPropertyValue
    // if you change this function


	// New 11/26/07 - all calls to deserialize value must block bounce back
	ScopedAssign<const Reflection::Property*> assign;
	if (preventBounceBack)
	{
		RBXASSERT(deserializingProperty==NULL);
		assign.assign(deserializingProperty, &property);
	}

	if (property.getDescriptor().type==Reflection::Type::singleton<RBX::ProtectedString>())
	{
        BinaryString valueString;

        if (canUseProtocolVersion(28))
        {
            if (useDictionary)
                getSharedPropertyBinaryDictionary(property.getDescriptor()).deserializeString(valueString, inBitStream);
            else
                inBitStream >> valueString;

        }
        else
        {
            std::string temp;

            if (useDictionary)
                getSharedPropertyProtectedDictionary(property.getDescriptor()).deserializeString(temp, inBitStream);
            else
                inBitStream >> temp;

            valueString = BinaryString(temp);
        }

        boost::optional<ProtectedString> value = decodeProtectedString(valueString.value(), static_cast<const Instance*>(property.getInstance()), property.getDescriptor());

        if (value)
        {
			if (outValue)
				*outValue = value.get();
            else if (property.getInstance())
                property.setValue<ProtectedString>(value.get());
        }
        else
        {
            //They are cheating with our strings. shut.it.down
            if (shared_ptr<DataModel> dataModel = shared_from(DataModel::get(this)))
                dataModel->submitTask(boost::bind(&Replicator::RemoteCheatHelper2, boost::weak_ptr<DataModel>(dataModel)), DataModelJob::Write);
        }
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<std::string>())
	{
		if (!outValue)
		{
			if (useDictionary)
				getSharedPropertyDictionary(property.getDescriptor()).deserializeString(property, inBitStream);
			else
				deserializeStringProperty(property, inBitStream);
		}
		else
		{
			std::string value;
			if (useDictionary)
				getSharedPropertyDictionary(property.getDescriptor()).deserializeString(value, inBitStream);
			else
				inBitStream >> value;

			*outValue = value;
		}

	}
    else if (property.getDescriptor().type==Reflection::Type::singleton<BinaryString>())
	{
		if (!outValue)
			deserialize<BinaryString>(property, inBitStream);
		else
			deserializeGeneric<BinaryString>(*outValue, inBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<bool>())
		outValue ? deserializeGeneric<bool>(*outValue, inBitStream) : deserialize<bool>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<int>())
		outValue ? deserializeGeneric<int>(*outValue, inBitStream) : deserialize<int>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<float>())
		outValue ? deserializeGeneric<float>(*outValue, inBitStream) : deserialize<float>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<double>())
		outValue ? deserializeGeneric<double>(*outValue, inBitStream) : deserialize<double>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<UDim>())
		outValue ? deserializeGeneric<UDim>(*outValue, inBitStream) : deserialize<UDim>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<UDim2>())
		outValue ? deserializeGeneric<UDim2>(*outValue, inBitStream) : deserialize<UDim2>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::RbxRay>())
		outValue ? deserializeGeneric<RBX::RbxRay>(*outValue, inBitStream) : deserialize<RBX::RbxRay>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<Faces>())
		outValue ? deserializeGeneric<Faces>(*outValue, inBitStream) : deserialize<Faces>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<Axes>())
		outValue ? deserializeGeneric<Axes>(*outValue, inBitStream) : deserialize<Axes>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<BrickColor>())
		outValue ? deserializeGeneric<BrickColor>(*outValue, inBitStream) : deserialize<BrickColor>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Color3>())
		outValue ? deserializeGeneric<G3D::Color3>(*outValue, inBitStream) : deserialize<G3D::Color3>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector2>())
		outValue ? deserializeGeneric<G3D::Vector2>(*outValue, inBitStream) : deserialize<G3D::Vector2>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector3>())
	{
		if (property.getDescriptor()==PartInstance::prop_Size)
		{
			G3D::Vector3 value;
			readBrickVector(inBitStream, value);

			if (!outValue)
			{
				if (property.getInstance())
					property.setValue(value);
			}
			else
				*outValue = value;

		}
		else
			outValue ? deserializeGeneric<G3D::Vector3>(*outValue, inBitStream) : deserialize<G3D::Vector3>(property, inBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector2int16>())
		outValue ? deserializeGeneric<G3D::Vector2int16>(*outValue, inBitStream) : deserialize<G3D::Vector2int16>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::Vector3int16>())
		outValue ? deserializeGeneric<G3D::Vector3int16>(*outValue, inBitStream) : deserialize<G3D::Vector3int16>(property, inBitStream);
	else if (property.getDescriptor().type==Reflection::Type::singleton<G3D::CoordinateFrame>())
		outValue ? deserializeGeneric<G3D::CoordinateFrame>(*outValue, inBitStream) : deserialize<G3D::CoordinateFrame>(property, inBitStream);
	else if (property.getDescriptor().bIsEnum)
    {
		if (!outValue)
		{
			if (!ProcessOutdatedPropertyEnumDeserialization(property, inBitStream))
			{
				deserializeEnumProperty(property, inBitStream);
			}
		}
		else
		{
			if (!ProcessOutdatedEnumDeserialization(inBitStream, property.getDescriptor().type, *outValue))
			{
				const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(property.getDescriptor().type);
				deserializeEnum(enumDesc, *outValue, inBitStream);
			}
		}
    }
	else if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(property.getDescriptor()))
	{
		RBX::Guid::Data id;

		if (useDictionary)
		{
			deserializeId(inBitStream, id);
		}
		else
		{
			deserializeIdWithoutDictionary(inBitStream, id);
		}

		if (!outValue)
			assignRef(property, id);
		else
			*outValue = id;
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::ContentId>())
	{
		if (useDictionary)
		{
			RBX::ContentId value;
			contentIdDictionary.receive(inBitStream, value);

			if (!outValue)
			{
				if (property.getInstance())
					property.setValue(value);
			}
			else
				*outValue = value;
		}
		else
			outValue ? deserializeGeneric<RBX::ContentId>(*outValue, inBitStream) : deserialize<RBX::ContentId>(property, inBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<RBX::SystemAddress>())
	{
		RBX::SystemAddress value;
		if (useDictionary)
			systemAddressDictionary.receive(inBitStream, value);
		else
			inBitStream >> value;

		if (!outValue)
		{
			if (property.getInstance())
				property.setValue(value);
		}
		else
			*outValue = value;

	}
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberSequence>())
    {
        outValue ? deserializeGeneric<NumberSequence>(*outValue, inBitStream) : deserialize<NumberSequence>(property, inBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberRange>())
    {
        outValue ? deserializeGeneric<NumberRange>(*outValue, inBitStream) : deserialize<NumberRange>(property, inBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<ColorSequence>())
    {
        outValue ? deserializeGeneric<ColorSequence>(*outValue, inBitStream) : deserialize<ColorSequence>(property, inBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<NumberSequenceKeypoint>())
    {
        outValue ? deserializeGeneric<NumberSequenceKeypoint>(*outValue, inBitStream) : deserialize<NumberSequenceKeypoint>(property, inBitStream);
    }
    else if (property.getDescriptor().type == Reflection::Type::singleton<ColorSequenceKeypoint>())
    {
        outValue ? deserializeGeneric<ColorSequenceKeypoint>(*outValue, inBitStream) : deserialize<ColorSequenceKeypoint>(property, inBitStream);
    }
	else if (property.getDescriptor().type==Reflection::Type::singleton<Rect2D>())
	{
		outValue ? deserializeGeneric<Rect2D>(*outValue, inBitStream) : deserialize<Rect2D>(property, inBitStream);
	}
	else if (property.getDescriptor().type==Reflection::Type::singleton<PhysicalProperties>())
	{
		outValue ? deserializeGeneric<PhysicalProperties>(*outValue, inBitStream) : deserialize<PhysicalProperties>(property, inBitStream);
	}
	else
		RBXASSERT(false);
}

void Replicator::setRefValue(WaitItem& wi, Instance* instance)
{
	// Prevent sending this property back to the remote peer
	Reflection::Property prop(*wi.desc, wi.instance.get());
	ScopedAssign<const Reflection::Property*> assign(deserializingProperty, &prop);
	Super::setRefValue(wi, instance);
}

void Replicator::writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream)
{
    DescriptorSender<RBX::Reflection::PropertyDescriptor>::IdContainer idContainer = propDictionary.getId(&desc);
    if (idContainer.outdated)
        return;

    if (isPropertyRemoved(instance, desc.name))
        return;

    int byteStart = outBitStream.GetNumberOfBytesUsed();

	Item::writeItemType(outBitStream, Item::ItemTypeChangeProperty);

	// Write the GUID
	serializeId(outBitStream, instance);

	// Write property name
	propDictionary.send(outBitStream, idContainer.id);

	serializePropertyValue(Reflection::ConstProperty(desc, instance), outBitStream, true/*useDictionary*/);

	if (settings().printProperties) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication prop: %s:%s.%s >> %s, bytes: %d", 
			instance->getClassName().c_str(), 
			instance->getGuid().readableString().c_str(), 
			desc.name.c_str(),
			RakNetAddressToString(remotePlayerId).c_str(),
			outBitStream.GetNumberOfBytesUsed()-byteStart
			);
	}

	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsSent(desc.category.str);
		replicatorStats.samplePacketsSent(desc.category.str, outBitStream.GetNumberOfBytesUsed()-byteStart);
	}
}

void Replicator::writeChangedRefProperty(const Instance* instance,
	const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid,
	RakNet::BitStream& outBitStream)
{
    DescriptorSender<RBX::Reflection::PropertyDescriptor>::IdContainer idContainer = propDictionary.getId(&desc);
    if (idContainer.outdated)
        return;

    if (isPropertyRemoved(instance, desc.name))
        return;

    int byteStart = outBitStream.GetNumberOfBytesUsed();

	Item::writeItemType(outBitStream, Item::ItemTypeChangeProperty);

	// Write the GUID
	serializeId(outBitStream, instance);

	// Write property name
	propDictionary.send(outBitStream, idContainer.id);

	if (newRefGuid.scope.isNull())
	{
		scopeNames.sendEmptyItem(outBitStream);
	}
	else
	{
		serializeId(outBitStream, newRefGuid);
	}

	if (settings().printProperties) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication ref prop: %s:%s.%s >> %s, bytes: %d", 
			instance->getClassName().c_str(), 
			instance->getGuid().readableString().c_str(), 
			desc.name.c_str(),
			RakNetAddressToString(remotePlayerId).c_str(),
			outBitStream.GetNumberOfBytesUsed()-byteStart
			);
	}

	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsSent(desc.category.str);
		replicatorStats.samplePacketsSent(desc.category.str, outBitStream.GetNumberOfBytesUsed()-byteStart);
	}
}

bool Replicator::wantReplicate(const Instance* source) const
{
	switch(source->getDescriptor().getReplicationLevel())
	{
		case Reflection::PLAYER_REPLICATE:
			if(const RBX::Network::Player* player = Instance::fastDynamicCast<RBX::Network::Player>(source->getParent())){
				if (const RBX::Network::Player* targetPlayer = findTargetPlayer()) {
					return player->getUserID() == targetPlayer->getUserID();
				}
			}
			return false;
		case Reflection::NEVER_REPLICATE:
			return false;
		case Reflection::STANDARD_REPLICATE:
			return true;
	}
	return true;
}
void Replicator::onChildAdded(shared_ptr<Instance> child,
							  boost::function<void (shared_ptr<Instance>)> replicationMethodFunc)
{
	RBXASSERT(child.get()!=removingInstance);

	if (!wantReplicate(child.get()))
		return;

	FASTLOG1(FLog::ReplicationDataLifetime, "Child instance added to replicatio: %p", child.get());

	// Check to see if the child is already being replicated
	if (replicationContainers.find(child.get())==replicationContainers.end() &&
		megaClusterInstance != child.get())
	{
        if ( shouldStreamingHandleOnAddedForChild(child) )
        {
            // if it is streaming mode, we check if the child should be handled by streaming (or discard it)
            return;
        }

		addReplicationData(child, true, true);

		// TODO: Assert that this item isn't in the delete or property queue
		// Submit it for replication
		replicationMethodFunc(child);

		// Now visit all current children (recursive)
		child->visitChildren(boost::bind(&Replicator::onChildAdded, this, _1, replicationMethodFunc));
	}
}

void Replicator::onTerrainParentChanged(shared_ptr<Instance> instance, shared_ptr<Instance> parent)
{
    if (instance.get() == megaClusterInstance && parent)
	{
        if (megaClusterInstance->isSmooth())
            megaClusterInstance->getSmoothGrid()->connectListener(this);
        else
            megaClusterInstance->getVoxelGrid()->connectListener(this);
	}
}

void Replicator::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	Vector3int16 cell = info.position;
	if(clusterDebounce)
		return;

	if (streamingEnabled && streamJob)
    {
        StreamRegion::Id regionId = StreamRegion::regionContainingVoxel(info.position);

        if (!streamJob->isRegionCollected(regionId))
        {
            return;
        }
        else if (streamJob->isRegionInPendingStreamItemQueue(regionId))
        {
            // collected but unsent, do nothing
            return;
        }
	}

	clusterReplicationData.updateBuffer.push(cell);
	approximateSizeOfPendingClusterDeltas = clusterReplicationData.updateBuffer.size() * kApproximateSizeOfVoxelDelta;
}

void Replicator::onTerrainRegionChanged(const Voxel2::Region& region)
{
	if (clusterDebounce)
        return;

	std::vector<Vector3int32> ids = region.getChunkIds(ClusterReplicationData::kUpdateChunkSizeLog2);

    for (size_t i = 0; i < ids.size(); ++i)
	{
        Vector3int32 chunkId = ids[i];

        if (streamingEnabled && streamJob)
        {
			StreamRegion::Id regionId = StreamRegion::regionContainingVoxel((chunkId << ClusterReplicationData::kUpdateChunkSizeLog2).toVector3int16());

            if (!streamJob->isRegionCollected(regionId))
                continue;

            if (streamJob->isRegionInPendingStreamItemQueue(regionId))
                continue;
        }

		clusterReplicationData.updateBufferSmooth.insert(chunkId);
	}

	approximateSizeOfPendingClusterDeltas = clusterReplicationData.updateBufferSmooth.size() * kApproximateSizeOfSmoothDelta;
}
        
void Replicator::onStatisticsChanged(const ConnectionStats& newStats) {
	FASTLOG(FLog::NetworkStatsReport, "Updating stats Replicator::onStatisticsChanged");
    replicatorStats.peerStats.mtuSize = newStats.mtuSize;
	replicatorStats.peerStats.lastPing = newStats.lastPing;
	replicatorStats.peerStats.lowestPing = newStats.lowestPing;
	replicatorStats.peerStats.averagePing = newStats.averagePing;
	replicatorStats.peerStats.rakStats = newStats.rakStats;
	replicatorStats.kiloBytesReceivedPerSecond = newStats.kiloBytesReceivedPerSecond.value();
	replicatorStats.kiloBytesSentPerSecond = newStats.kiloBytesSentPerSecond.value();
	replicatorStats.bufferHealth = newStats.bufferHealth.value();
}

Replicator::ReplicationData& Replicator::addReplicationData(shared_ptr<Instance> instance, bool listenToChanges, bool replicateChildren)
{
	MegaClusterInstance* cluster = Instance::fastDynamicCast<MegaClusterInstance>(instance.get());
	if (cluster)
    {
		RBXASSERT(megaClusterInstance == NULL);

		FASTLOG2(FLog::MegaClusterNetworkInit, "Adding MegaCluster replication data :%p, listenToChanges: %u", instance.get(), listenToChanges);

		clusterReplicationData = ClusterReplicationData();
        
		// No need to send data in two cases:
		// - received this instance from server (listen to changes)
		// - level doesn't use Terrain
		if (listenToChanges == false || !cluster->isAllocated())
		{
			FASTLOG(FLog::MegaClusterNetworkInit, "Disabling cluster initial send");
			clusterReplicationData.readyToSendChunks = true;
		}
		else
		{
			if (cluster->isSmooth())
			{
                RBXASSERT(oneQuarterClusterPacketCache);
                oneQuarterClusterPacketCache->setupListener(cluster);

                if (!streamingEnabled)
				{
					std::vector<Voxel2::Region> regions = cluster->getSmoothGrid()->getNonEmptyRegions();

                    for (size_t i = 0; i < regions.size(); ++i)
					{
						std::vector<Vector3int32> ids = regions[i].getChunkIds(StreamRegion::_PrivateConstants::kRegionSizeInVoxelsAsBitShift);

                        for (size_t j = 0; j < ids.size(); ++j)
							clusterReplicationData.initialSendSmooth.push_back(StreamRegion::Id(ids[j]));
					}
				}
			}
			else if (streamingEnabled)
			{
				// cache should be only be created in Server
				if (oneQuarterClusterPacketCache)
					oneQuarterClusterPacketCache->setupListener(cluster);
			}
			else
			{
				if (clusterPacketCache)
				{
					clusterPacketCache->setupListener(cluster);
					clusterReplicationData.initialChunkIterator = cluster->getVoxelGrid()->getNonEmptyChunks();
				}
				else
					clusterReplicationData.initialSendIterator = ClusterChunksIterator(cluster->getVoxelGrid()->getNonEmptyChunks());
			}
		}

		megaClusterInstance = cluster;

        if (megaClusterInstance->isInitialized())
		{
            // We have the grid so just connect now
            if (megaClusterInstance->isSmooth())
                megaClusterInstance->getSmoothGrid()->connectListener(this);
            else
                megaClusterInstance->getVoxelGrid()->connectListener(this);
        }
		else
		{
            // We'll get a smoothReplicated property later so we'll need to start listening later
            clusterReplicationData.parentConnection = instance->ancestryChangedSignal.connect(boost::bind(&Replicator::onTerrainParentChanged, this, _1, _2));
		}
	}

	ReplicationData& newOrOldData = replicationContainers[instance.get()];

	if (!newOrOldData.instance)
	{
		FASTLOG1(FLog::ReplicationDataLifetime, "Adding instance replication data: %p", instance.get());

		// new ReplicationData
		ReplicationData& newData = newOrOldData;
		newData.instance = instance;
		newData.deleteOnDisconnect = false;	// by default
		newData.replicateChildren = replicateChildren;
		newData.listenToChanges = listenToChanges;

		if (replicateChildren || listenToChanges)
			newData.connection = instance->combinedSignal.connect(boost::bind(&Replicator::onCombinedSignal, this, &newData, _1, _2));

		return newData;
	}
	else
	{
		// Old replication data
		ReplicationData& oldData = newOrOldData;
		RBXASSERT(oldData.instance == instance);

		oldData.replicateChildren = replicateChildren;
		oldData.listenToChanges = listenToChanges;

		if(!replicateChildren && !listenToChanges)
			oldData.connection.disconnect();
		else if(!oldData.connection.connected())
            oldData.connection = instance->combinedSignal.connect(boost::bind(&Replicator::onCombinedSignal, this, &oldData, _1, _2));

		return oldData;
	}	
}

bool Replicator::removeFromPendingNewInstances(const Instance* instance)
{
	RBX::mutex::scoped_lock lock(pendingInstancesMutex);
	
	if (megaClusterInstance && instance == megaClusterInstance
		&& !clusterReplicationData.readyToSendChunks)
	{
		// Ready to send cluster data!
		isClusterSpotted = true;
	}

	return pendingNewInstances.erase(instance) != 0;
}

bool Replicator::isSerializePending(const Instance* instance) const
{
	// We put a lock on pendingNewInstances because isSerializePending can be called by PhysicsOut code
	RBX::mutex::scoped_lock lock(pendingInstancesMutex);
	return pendingNewInstances.find(instance) != pendingNewInstances.end();
}

bool Replicator::isPropertyChangedPending(const RBX::Reflection::ConstProperty& property) const
{
	if (pendingChangedPropertyItems.find(property) != pendingChangedPropertyItems.end())
		return true;

	return false;
}

void Replicator::onParentChanged(shared_ptr<Instance> instance)
{
	Instance* parent = instance->getParent();
	if (isReplicationContainer(parent))
	{
		// skip this item if this instance is already being serialized via its parents' child_added signal,
		if (serializingInstance == instance.get())
		{
			// only do this once, because subsequent parent changes could come from other place, such as script
			serializingInstance = NULL;
		}
		else
		{
			pendingItems.push_back(new (referencePropertyChangedPool.get()) ReferencePropertyChangedItem(this, instance, Instance::propParent));
		}
	}
	else
	{
		FASTLOG1(FLog::ReplicationDataLifetime, "Parent changed to NULL on instance %p", instance.get());
		// Deletion is much more complicated...
		bool removedIt = disconnectReplicationData(instance);
		RBXASSERT(removedIt);

		pendingItems.push_back(new (deleteInstancePool.get()) DeleteInstanceItem(this, instance));
	}
}

void Replicator::onCombinedSignal(ReplicationData* replicationData, Instance::CombinedSignalType type, const ICombinedSignalData* genericData)
{
	switch(type)
	{
	case Instance::EVENT_INVOCATION:
	{
		if(replicationData->listenToChanges){
			const Instance::EventInvocationSignalData* data = boost::polymorphic_downcast<const Instance::EventInvocationSignalData*>(genericData);

			if(isLegalSendEvent(replicationData->instance.get(), *data->eventDescriptor))
				onEventInvocation(replicationData->instance.get(), data->eventDescriptor, data->eventArguments, data->target);
		}
		break;
	}
	case Instance::PROPERTY_CHANGED:
	{
		if(replicationData->listenToChanges){
			const Instance::PropertyChangedSignalData* data = boost::polymorphic_downcast<const Instance::PropertyChangedSignalData*>(genericData);
			if (filterChangedProperty(replicationData->instance.get(), *data->propertyDescriptor) == Accept)
				onPropertyChanged(replicationData->instance.get(), data->propertyDescriptor);
		}
		break;
	}
	case Instance::CHILD_ADDED:
	{
		if(replicationData->replicateChildren){
			const Instance::ChildAddedSignalData* data = boost::polymorphic_downcast<const Instance::ChildAddedSignalData*>(genericData);
			
			// Instance::SetParent function will signal parent's child_added before property_changed, 
			// so mark the child as serializing so we won't try to create a ParentPropChange item for it
			if (wantReplicate(data->child.get()) &&
				!isReplicationContainer(data->child.get()) &&
				megaClusterInstance != data->child.get())
			{
				serializingInstance = data->child.get();
			}

			onChildAdded(data->child, boost::bind(&Replicator::addToPendingItemsList, this, _1));
		}
		break;
	}
	case Instance::CHILD_REMOVED:
	{
		const Instance::ChildRemovedSignalData* data = boost::polymorphic_downcast<const Instance::ChildRemovedSignalData*>(genericData);
		if (strictFilter)
			strictFilter->onChildRemoved(data->child.get(), replicationData->instance.get());
		break;
	}
    default:
        break;
    }
}

void Replicator::onEventInvocation(Instance* instance, const Reflection::EventDescriptor* descriptor, const Reflection::EventArguments* args, const SystemAddress* target)
{
	if (instance == removingInstance)
		return;

    if (target && *target != RakNetToRbxAddress(remotePlayerId))
        return;

	if (deserializingEventInvocation!=NULL && Reflection::EventInvocation(Reflection::Event(*descriptor, shared_from(instance)), *args)==*deserializingEventInvocation)	
		return;

	pendingItems.push_back(new (eventInvocationPool.get()) EventInvocationItem(this, shared_from(instance), *descriptor, *args));
}

FilterResult Replicator::filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
	if (instance == removingInstance)
		return Reject;

	Reflection::Property prop(desc, instance);

	// avoid circular replication
	if (deserializingProperty!=NULL && prop==*deserializingProperty)
	{
		// Only debounce this once. Afterwards a property change might come from a Lua script or something else.
		// See the "PropertyBounceBack" test in App.UnitTest
		deserializingProperty = NULL;
		return Reject;
	}

	if (!isLegalSendProperty(instance, desc))
		return Reject;

	if (desc == Instance::propParent)
	{
		onParentChanged(shared_from(instance));
		return Reject;
	}

	// No need to send this change if we haven't even sent the object yet!
	if (isSerializePending(instance))
		return Reject;

	if (!desc.canReplicate() && !(isCloudEdit() && isCloudEditReplicateProperty(desc)))
		return Reject;

	if (isPropertyChangedPending(prop))
		return Reject;

	return Accept;
}

void Replicator::onPropertyChanged(Instance* instance, const Reflection::PropertyDescriptor* descriptor)
{
	if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(*descriptor))
	{
		pendingItems.push_back(new (referencePropertyChangedPool.get())
			ReferencePropertyChangedItem(this, shared_from(instance),
			static_cast<const Reflection::RefPropertyDescriptor&>(*descriptor)));
	}
	else
	{
		Player* player = findTargetPlayer();
		ModelInstance* m = player ? player->getCharacter() : 0;
		if ((m && instance->isDescendantOf(m)) || (player == instance))
		{
			// Give priority to my character changes
			Time headTime = pendingItems.head_time();
			pendingItems.push_front(new (changePropertyPool.get()) ChangePropertyItem(this, shared_from(instance), *descriptor));

			// preserve head time
			pendingItems.begin()->timestamp = headTime;

			pendingChangedPropertyItems.insert(RBX::Reflection::ConstProperty(*descriptor, instance));
		}
		else
		{
			pendingItems.push_back(new (changePropertyPool.get()) ChangePropertyItem(this, shared_from(instance), *descriptor));
			pendingChangedPropertyItems.insert(RBX::Reflection::ConstProperty(*descriptor, instance));
		}
	}
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
    // This should check pretty far back into the call stack.
    if (FFlag::FilterSinglePass)
    {
        // kick if fail
        if(uint32_t callChecks = detectDllByExceptionChainStack<4>(&instance, (RBX::Security::kAllowVmpAll | RBX::Security::kCheckReturnAddr)))
        {
            pendingItems.push_front(new (pingPool.get()) PingItem(this, 0, callChecks));

            // debug logging.
            if (FFlag::FilterDoublePass)
            {
                std::vector<CallChainInfo> info;
                generateCallInfo<4>(&instance, info);
	            pendingItems.push_front(new RockyDbgItem(this, info));
            }
        }
    }
#endif
}

bool Replicator::remoteDeleteOnDisconnect(const Instance* instance) const
{
	if (isCloudEdit())
	{
		if (Instance::fastDynamicCast<Player>(instance) && players == instance->getParent())
			return true;
	}
	else
	{
		if (instance->isDescendantOf(players))
			return true;
	}

	if (instance == Players::findConstLocalCharacter(instance))
	{
		return true;
	}

	if (fastDynamicCast<TouchTransmitter>(instance))
	{
		return true;
	}

	// TODO: What other objects should be auto-deleted?
	return false;
}

void Replicator::logPacketError(RakNet::Packet* packet, const std::string& type, const std::string& message)
{
	std::string errorMessage = RBX::format("Error while processing packet: %s (packet id: %d, packet length: %d)", message.c_str(), packet->data[0], packet->length);

	RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Error while processing packet.");
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_SENSITIVE, errorMessage.c_str());

	Analytics::InfluxDb::Points p;
	p.addPoint("Type", type.c_str());
	p.addPoint("Message", errorMessage.c_str());
	p.addPoint("PacketId", packet->data[0]);
	p.addPoint("Length", packet->length);
	p.report("PacketError", DFInt::PacketErrorInfluxHundredthsPercentage);
}

void Replicator::processDeserializedPacket(const DeserializedPacket& deserializedPacket)
{
	RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);

	if (!deserializedPacket.deserializedItems.empty())
	{
        RBXPROFILER_SCOPE("Network", "processDeserializedPacket");
        RBXPROFILER_LABELF("Network", "ID %d (%d bytes)", deserializedPacket.rawPacket->data[0], deserializedPacket.rawPacket->length);
        RBXPROFILER_LABELF("Network", "%d items", int(deserializedPacket.deserializedItems.size()));
        
		for (auto item: deserializedPacket.deserializedItems)
		{
			RBXASSERT(item);
			item->process(*this);
		}
	}
	else
    {
		processPacket(deserializedPacket.rawPacket);
    }
}

bool Replicator::processNextIncomingPacket()
{
	if (!rakPeer)
		return false;

	// Now process as many queued packets as we can
	Packet* packet;
	DeserializedPacket deserializedPacket;
	const double wait = settings().incommingReplicationLag;
	if (wait <= 0)
	{
		if (deserializePacketsThread)
		{
			if (!deserializedPackets.pop_if_present(deserializedPacket))
				return false;

			packet = deserializedPacket.rawPacket;
		}
		else
		{
			if (!incomingPackets.pop_if_present(packet))
				return false;
		}
	}
	else
	{
		if (deserializePacketsThread)
		{
			if (!deserializedPackets.pop_if_waited(Time::Interval(wait), deserializedPacket))
				return false;
			packet = deserializedPacket.rawPacket;
		}
		else
		{
			if (!incomingPackets.pop_if_waited(Time::Interval(wait), packet))
				return false;
		}
	}

	{
		try
		{
			if(physicsReceiver!=NULL)
			{
				physicsReceiver->setTime(Time::nowFast());
			}

			if (deserializePacketsThread)
				processDeserializedPacket(deserializedPacket);
			else
				processPacket(packet);
		}
        catch(std::out_of_range& e) // Workaround for iPad, where exceptions can't be caught by ancestor class
		{
			logPacketError(packet, "Stream", e.what());
			rakPeer->DeallocatePacket(packet);
			requestDisconnectWithSignal(DisconnectReason_ReceivePacketError);
			return false;
		}
		catch (RBX::physics_receiver_exception& e)
		{
			// ignore physics exceptions and just discard packet, since they are "unreliable" anyways
			FASTLOGS(FLog::Network,"Error while processing physics packet: %s", e.what());
			printf("(ignored) %s (Packet size: %d, packet type: %d)\n", e.what(), packet->bitSize, packet->data[0]); // for unit test
		}
		catch (RBX::network_stream_exception& e)
		{
			logPacketError(packet, "Stream", e.what());
			rakPeer->DeallocatePacket(packet);
			requestDisconnectWithSignal(DisconnectReason_ReceivePacketStreamError);
			return false;
		}
		catch (RBX::base_exception& e)
		{
			logPacketError(packet, "Other", e.what());
			rakPeer->DeallocatePacket(packet);
			requestDisconnectWithSignal(DisconnectReason_ReceivePacketError);
			return false;
		}
		rakPeer->DeallocatePacket(packet);
	}
	
	return true;
}

shared_ptr<Instance> Replicator::getPlayer()
{
	return shared_from(findTargetPlayer());
}

bool Replicator::isReplicationContainer(const Instance* instance) const
{
	return replicationContainers.find(instance)!=replicationContainers.end();
}

bool Replicator::sendItemsPacket()
{
	if (!canSendItems())
		return false;

	int count;
	const int limit = settings().sendPacketBufferLimit;
	if (limit == -1)
	{
		count = DFInt::MaxDataPacketPerSend;
        if (count < 1)
        {
            count = 1;
        }
		replicatorStats.dataPacketSendThrottle.sample(0.0f); // no throttle
	}
	else
	{
		count = getBufferCountAvailable(limit, settings().getDataSendPriority());
		replicatorStats.dataPacketSendThrottle.sample(1.0f - count / limit);
	}

	replicatorStats.dataNewItemsPerSec.increment(pendingItems.size() - numItemsLeftFromPrevStep);
	int itemcounter = 0;
	for (int i=0; i<count; ++i)
	{
		ItemSender sender(*this, rakPeer.get());

		itemcounter += sendItems(sender, highPriorityPendingItems);
		itemcounter += sendItems(sender, pendingItems);

		if (!sender.sentItems)
			return false;
	}

	replicatorStats.dataItemsSentPerSec.increment(itemcounter);
	numItemsLeftFromPrevStep = pendingItems.size();

	return true;
}

int Replicator::sendItems(ItemSender& sender, ItemQueue& itemQueue)
{
	Item* item;

	int numSent = 0;
	while (itemQueue.pop_if_present(item))
	{
		std::auto_ptr<Item> scope(item);	// auto-delete when done
		if (sender.send(*item) == ItemSender::SEND_BITSTREAM_FULL) 
		{
			scope.release();
			itemQueue.push_front_preserve_timestamp(item);
			break;
		}

		replicatorStats.dataTimeInQueue.sample((RBX::Time::now<RBX::Time::Fast>() - item->timestamp).msec());
		numSent++;
	}

	return numSent;
}

template <class Container> void sendClusterContentHelper(MegaClusterInstance* megaClusterInstance, shared_ptr<RakNet::BitStream>& bitStream, Container& container, int maxBytesSend)
{
	*bitStream << (int)(CLUSTER_DATA_TOKEN);

    Voxel::Serializer serializer;
    serializer.encodeCells(megaClusterInstance->getVoxelGrid(), container, bitStream.get(), maxBytesSend);
}

void Replicator::sendClusterContent(shared_ptr<RakNet::BitStream>& bitStream, ClusterUpdateBuffer& container, unsigned maxBytesSend)
{
    sendClusterContentHelper(megaClusterInstance, bitStream, container, maxBytesSend);
}

void Replicator::sendClusterContent(shared_ptr<RakNet::BitStream>& bitStream, ClusterChunksIterator& container, unsigned maxBytesSend)
{
    sendClusterContentHelper(megaClusterInstance, bitStream, container, maxBytesSend);
}

void Replicator::sendClusterContent(shared_ptr<RakNet::BitStream>& bitStream, OneQuarterClusterChunkCellIterator &container)
{
    sendClusterContentHelper(megaClusterInstance, bitStream, container, -1);
}

bool Replicator::isInitialDataSent()
{
    if (streamJob)
        return true;

	if (!megaClusterInstance)
        return true;

	if (megaClusterInstance->isSmooth())
		return clusterReplicationData.initialSendSmooth.empty();
	else if (clusterPacketCache)
        return clusterReplicationData.initialChunkIterator.empty();
	else
		return clusterReplicationData.initialSendIterator.size() == 0;
}

void Replicator::sendClusterChunk(const StreamRegion::Id &regionId)
{
	ClusterReplicationData& clusterData = clusterReplicationData;

	if (megaClusterInstance == NULL || !clusterData.readyToSendChunks)
		return;

	if (megaClusterInstance->isSmooth())
	{
        int chunkSizeLog2 = StreamRegion::_PrivateConstants::kRegionSizeInVoxelsAsBitShift;
        Voxel2::Region region = Voxel2::Region::fromChunk(regionId.value(), chunkSizeLog2);
		Voxel2::Box box = megaClusterInstance->getSmoothGrid()->read(region);

		if (box.isEmpty())
            return;

        shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
        *bitStream << (unsigned char) ID_CLUSTER;
        *bitStream << true; // using 1/4 iterator (streaming)

        serializeId(*bitStream, megaClusterInstance);

		Voxel2::BitSerializer<RakNet::BitStream> serializer;

		*bitStream << (char)chunkSizeLog2;

		serializer.encodeIndex(regionId.value(), *bitStream);

        // try fetch bitstream data from cache
        if (!oneQuarterClusterPacketCache->fetchIfUpToDate(regionId, *bitStream))
        {
            // cache miss, recreate and update cache
            unsigned int startData = bitStream->GetWriteOffset();

            serializer.encodeContent(box, *bitStream);

            bitStream->SetReadOffset(startData);

            oneQuarterClusterPacketCache->update(regionId, *bitStream, bitStream->GetNumberOfBitsUsed() - startData);
        }

        *bitStream << (char)0; // end token

        rakPeer->Send(bitStream, NetworkSettings::singleton().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
	}
	else
	{
        if (megaClusterInstance->getVoxelGrid()->getRegion(StreamRegion::getMinVoxelCoordinateInsideRegion(regionId), StreamRegion::getMaxVoxelCoordinateInsideRegion(regionId)).isGuaranteedAllEmpty())
            return;

        shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
        *bitStream << (unsigned char) ID_CLUSTER;
        *bitStream << true; // using 1/4 iterator (streaming)

        serializeId(*bitStream, megaClusterInstance);

        if (oneQuarterClusterPacketCache)
        {
            // try fetch bitstream data from cache
            if (!oneQuarterClusterPacketCache->fetchIfUpToDate(regionId, *bitStream))
            {
                // cache miss, recreate and update cache
                unsigned int startBitOffset = bitStream->GetWriteOffset();
                OneQuarterClusterChunkCellIterator chunkIterator;
                chunkIterator.setToStartOfStreamRegion(regionId);
                sendClusterContent(bitStream, chunkIterator);
                bitStream->SetReadOffset(startBitOffset);

                oneQuarterClusterPacketCache->update(regionId, *bitStream, bitStream->GetNumberOfBitsUsed() - startBitOffset);
            }
        }
        else
        {
            OneQuarterClusterChunkCellIterator chunkIterator;
            chunkIterator.setToStartOfStreamRegion(regionId);
            sendClusterContent(bitStream, chunkIterator);
        }

        // nothing to send
        if (bitStream->GetNumberOfBytesUsed() == 1+32/8)
            return;

        *bitStream << (int)(CLUSTER_END_TOKEN);
        rakPeer->Send(bitStream, NetworkSettings::singleton().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
	}
}

bool Replicator::sendClusterPacket()
{
	FASTLOG(FLog::MegaClusterNetwork, "sendClusterPacket job run");

	if (!canSendItems()) // Works for both data and cluster
		return false;

	ClusterReplicationData& clusterData = clusterReplicationData;
	if(megaClusterInstance == NULL || !clusterData.readyToSendChunks || !clusterData.hasDataToSend())
		return false;

	int maxBytesSend = getAdjustedMtuSize();
	RBXASSERT(maxBytesSend > 0);

	shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
	*bitStream << (unsigned char) ID_CLUSTER;
	*bitStream << false; // using regular iterator (non-streaming)

    if (megaClusterInstance->isSmooth())
    {
        serializeId(*bitStream, megaClusterInstance);

        Voxel2::BitSerializer<RakNet::BitStream> serializer;

        // Initial data send
        if (!streamJob)
		{
            float numKBytesSent = 0;
            float maxKBPerStep = (float)DFInt::MaxClusterKBPerSecond / settings().getDataSendRate();

            while (clusterData.initialSendSmooth.size() && (DFInt::MaxClusterKBPerSecond == -1 || (numKBytesSent < maxKBPerStep)))
            {
                StreamRegion::Id regionId = clusterData.initialSendSmooth.back();

				int chunkSizeLog2 = StreamRegion::_PrivateConstants::kRegionSizeInVoxelsAsBitShift;

                unsigned int startHeader = bitStream->GetWriteOffset();

				*bitStream << (char)chunkSizeLog2;

				serializer.encodeIndex(regionId.value(), *bitStream);

                // try fetch bitstream data from cache
                if (!oneQuarterClusterPacketCache->fetchIfUpToDate(regionId, *bitStream))
                {
                    // cache miss, recreate and update cache
                    unsigned int startData = bitStream->GetWriteOffset();

                    Voxel2::Region region = Voxel2::Region::fromChunk(regionId.value(), chunkSizeLog2);
                    Voxel2::Box box = megaClusterInstance->getSmoothGrid()->read(region);

					serializer.encodeContent(box, *bitStream);

					bitStream->SetReadOffset(startData);

					oneQuarterClusterPacketCache->update(regionId, *bitStream, bitStream->GetNumberOfBitsUsed() - startData);
                }

                unsigned int numBitsUsed = bitStream->GetNumberOfBitsUsed() - startHeader;

                clusterData.initialSendSmooth.pop_back();
                numKBytesSent += (numBitsUsed / 8.0f / 1000.0f);
            }
		}

        // Updates
        unsigned int maxBytesSend = getAdjustedMtuSize();
        RBXASSERT(maxBytesSend > 0);

		while (bitStream->GetNumberOfBytesUsed() < maxBytesSend && clusterData.updateBufferSmooth.size() > 0)
		{
            Vector3int32 chunkId = *clusterData.updateBufferSmooth.begin();

            Voxel2::Region region = Voxel2::Region::fromChunk(chunkId, ClusterReplicationData::kUpdateChunkSizeLog2);
            Voxel2::Box box = megaClusterInstance->getSmoothGrid()->read(region);

            *bitStream << (char)ClusterReplicationData::kUpdateChunkSizeLog2;

            serializer.encodeIndex(chunkId, *bitStream);
            serializer.encodeContent(box, *bitStream);

			clusterData.updateBufferSmooth.erase(clusterData.updateBufferSmooth.begin());
		}

        *bitStream << (char)0; // end token

		approximateSizeOfPendingClusterDeltas = clusterData.updateBufferSmooth.size() * kApproximateSizeOfSmoothDelta;
    }
	else
    {
        bool idWritten = false;
        if (!streamJob)
        {
            int maxBytesSend = getAdjustedMtuSize();
            RBXASSERT(maxBytesSend > 0);

            if (clusterPacketCache)
            {
                if (clusterData.initialChunkIterator.size())
                {
                    serializeId(*bitStream, megaClusterInstance);
                    idWritten = true;
                }

                float numKBytesSent = 0;
                float maxKBPerStep = (float)DFInt::MaxClusterKBPerSecond / settings().getDataSendRate();
                while (clusterData.initialChunkIterator.size() && (DFInt::MaxClusterKBPerSecond == -1 || (numKBytesSent < maxKBPerStep)))
                {
                    SpatialRegion::Id id = clusterData.initialChunkIterator.front();

                    unsigned int startBitOffset = bitStream->GetWriteOffset();
                    unsigned int numBitsUsed = 0;
                    // try fetch bitstream data from cache
                    if (!clusterPacketCache->fetchIfUpToDate(id, *bitStream))
                    {
                        // cache miss, recreate and update cache
                        ClusterChunksIterator chunkIterator(id);
                        sendClusterContent(bitStream, chunkIterator, -1);
                        bitStream->SetReadOffset(startBitOffset);

                        numBitsUsed = bitStream->GetNumberOfBitsUsed() - startBitOffset;
                        clusterPacketCache->update(id, *bitStream, numBitsUsed);
                    }
                    else
                        numBitsUsed = bitStream->GetNumberOfBitsUsed() - startBitOffset; 

                    clusterData.initialChunkIterator.erase(clusterData.initialChunkIterator.begin());
                    numKBytesSent += (numBitsUsed / 8.0f / 1000.0f);
                }
            }
            else // not using cache
            {
                if (clusterData.initialSendIterator.size() > 0)
                {
                    serializeId(*bitStream, megaClusterInstance);
                    idWritten = true;
                    sendClusterContent(bitStream, clusterData.initialSendIterator, (unsigned)maxBytesSend);
                }
            }
        }

        int numBytes = bitStream->GetNumberOfBytesUsed();
        if(numBytes < maxBytesSend && clusterData.updateBuffer.size() > 0)
        {
            if(!idWritten) {
                serializeId(*bitStream, megaClusterInstance);
                idWritten = true;
            }
            sendClusterContent(bitStream, clusterData.updateBuffer, maxBytesSend - numBytes);
        }

        *bitStream << (int)(CLUSTER_END_TOKEN);

        approximateSizeOfPendingClusterDeltas = clusterData.updateBuffer.size() * kApproximateSizeOfVoxelDelta;
    }

    FASTLOG(FLog::MegaClusterNetwork, "Cluster packed send");
    rakPeer->Send(bitStream, settings().getDataSendPriority(),
        DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);

    replicatorStats.clusterPacketsSent.sample();
    replicatorStats.clusterPacketsSentSize.sample(bitStream->GetNumberOfBytesUsed());

    return true;
}


const Instance* Replicator::getDefault(const RBX::Name& className)
{
	// We could make defaultObjects static if we made it safe for multi-threading
	// It is mildly faster (and easier to code) the way it is
	DefaultObjects::iterator iter = defaultObjects.find(&className);
	if (iter==defaultObjects.end())
	{
		RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);

		shared_ptr<Instance> instance = Creatable<Instance>::createByName(className, RBX::ReplicationCreator);
		if (!instance && strcmp(className.c_str(), "Workspace"))
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Replication: Can't create default object of type %s", className.c_str());
		defaultObjects[&className] = instance;
		return instance.get();
	}
	else
		return iter->second.get();
}

struct String_sink : public boost::iostreams::sink 
{
    std::string& s;
    String_sink(std::string& s):s(s){}
    std::streamsize write(const char* s, std::streamsize n) 
    {
        this->s.append(s, n);
        return n;
    }
};

void Replicator::compressBitStream(const RakNet::BitStream& inUncompressedBitStream, RakNet::BitStream& outCompressedBitStream, uint8_t compressRatio)
{
#ifdef NETWORK_DEBUG
    RBX::Timer<RBX::Time::Precise> timer;
#endif

    // compress the data
    std::string compressedData;
    boost::iostreams::stream< boost::iostreams::array_source > source ((char*)inUncompressedBitStream.GetData(), inUncompressedBitStream.GetNumberOfBytesUsed());
    boost::iostreams::filtering_streambuf<boost::iostreams::input> outStream; 
    outStream.push(boost::iostreams::gzip_compressor(compressRatio)); 
    outStream.push(source); 
    boost::iostreams::copy(outStream, String_sink(compressedData));

    // write compressed data to bitstream
    outCompressedBitStream << (unsigned int)compressedData.length();
    outCompressedBitStream.Write(compressedData.c_str(), compressedData.length());
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(MESSAGE_INFO, "Compressed bitStream from %d bytes to %d bytes with level %d compressRatio in %f seconds", inUncompressedBitStream.GetNumberOfBytesUsed(), outCompressedBitStream.GetNumberOfBytesUsed(), compressRatio, timer.delta().seconds());
#endif
}

void Replicator::decompressBitStream(RakNet::BitStream& inCompressedBitStream, RakNet::BitStream& outUncompressedBitStream)
{
    RBXPROFILER_SCOPE("Network", "decompressBitStream");
    
    unsigned int len;
    inCompressedBitStream >> len;

    // read compressed data from bitstream
    boost::scoped_ptr<char> buffer;
    buffer.reset(new char[len]);
    inCompressedBitStream.Read(buffer.get(), len);

    // read and decompress join data
    std::string decompressedData;

    // decompress the data
    boost::iostreams::stream< boost::iostreams::array_source > source (buffer.get(), len);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(source);
    boost::iostreams::copy(in, String_sink(decompressedData));

    outUncompressedBitStream.Write(decompressedData.c_str(), decompressedData.length());
    
    RBXPROFILER_LABELF("Network", "%d bytes -> %d bytes", int(inCompressedBitStream.GetNumberOfBytesUsed()), int(outUncompressedBitStream.GetNumberOfBytesUsed()));
}

void Replicator::readPropertiesFromValueArray(const std::vector<PropValuePair>& propValueArray, Instance* instance)
{
	for (std::vector<PropValuePair>::const_iterator iter = propValueArray.begin(); iter != propValueArray.end(); iter++)
	{
		RBXASSERT(!iter->value.isVoid());

		const Reflection::PropertyDescriptor& descriptor = *iter->descriptor;

		Reflection::Property property(descriptor, instance);
		ScopedAssign<const Reflection::Property*> assign(deserializingProperty, &property);

		if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(descriptor))
		{
			// reference types are stored as guid id
			Guid::Data id = iter->value.get<Guid::Data>();
			assignRef(property, id);
		}
		else if (descriptor.bIsEnum)
		{
			const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(descriptor.type);
			RBXASSERT(enumDesc);
			const Reflection::EnumDescriptor::Item* item = enumDesc->lookup(iter->value);
			RBXASSERT(item);

			const Reflection::EnumPropertyDescriptor& enumPropDesc = static_cast<const Reflection::EnumPropertyDescriptor&>(descriptor);
			enumPropDesc.setEnumItem(instance, *item);				
		}
		else if (descriptor.type == Reflection::Type::singleton<RBX::ContentId>())
		{
			RBX::ContentId value = iter->value.get<RBX::ContentId>();
			descriptor.setStringValue(instance, value.toString());
		}
		else
			descriptor.setVariant(instance, iter->value);
	}
}

void Replicator::readProperties(RakNet::BitStream& inBitstream, Instance* instance, PropertyCacheType cacheType, bool useDictionary, bool preventBounceBack, std::vector<PropValuePair>* valueArray)
{
    if (ProcessOutdatedProperties(inBitstream, instance, cacheType, useDictionary, preventBounceBack, valueArray))
    {
        return;
    }

    bool cacheable = (cacheType != PropertyCacheType_NonCacheable);

	RBX::Reflection::PropertyIterator iter = instance->properties_begin();
	RBX::Reflection::PropertyIterator end = instance->properties_end();
	while (iter!=end)
	{
		Reflection::Property property = *iter;
		const Reflection::PropertyDescriptor& descriptor = property.getDescriptor();
		if ((descriptor.canReplicate() || (isCloudEdit() && isCloudEditReplicateProperty(descriptor)))
            && (cacheType == PropertyCacheType_All || isPropertyCacheable(descriptor.type) == cacheable)
            && (!cacheable || descriptor != Instance::propParent)) // don't read parent if it's cacheable (PropertyCacheType_Cacheable) or potentially cacheable (PropertyCacheType_All)
		{
#ifdef NETWORK_DEBUG
            if (FFlag::DebugProtocolSynchronization && Workspace::serverIsPresent(this))
            {
                if (ServerReplicator* rep = dynamic_cast<ServerReplicator*>(this))
                {
                    // Cope with the property removal simulation
                    std::string className = instance->getClassNameStr();
                    std::string propertyName = descriptor.name.toString();
                    if (className == "WedgePart" && propertyName == "Material")
                    {
                        ++iter;
                        continue;
                    }
                }
            }
#endif

			if (valueArray)
			{
				Reflection::Variant value;
				readPropertiesInternal(property, inBitstream, useDictionary, preventBounceBack, &value);
				if (!value.isVoid())
					valueArray->push_back(PropValuePair(property.getDescriptor(), value));
			}
			else
				readPropertiesInternal(property, inBitstream, useDictionary, preventBounceBack, NULL);
		}
		++iter;
	}
}

void Replicator::readPropertiesInternal(Reflection::Property& property, RakNet::BitStream& inBitstream, bool useDictionary, bool preventBounceBack, Reflection::Variant* outValue)
{
    // in line with skipPropertiesInternal
	const Reflection::PropertyDescriptor& descriptor = property.getDescriptor();

	if (descriptor.type.isType<bool>())
	{
		if (!outValue)
			deserialize<bool>(property, inBitstream);		// short-circuit for bools
		else
			deserializeGeneric<bool>(*outValue, inBitstream);

		if (settings().printBits) {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"   read %s %s, 1 bit", 
			descriptor.type.name.c_str(),
			descriptor.name.c_str()
			);
		}
	}
	else
	{
		bool isDefault;
		inBitstream >> isDefault;
		if (isDefault)
		{
			if (outValue)
			{
				*outValue = Reflection::Variant(); // default value
				if (DFFlag::ExplicitlyAssignDefaultPropVal)
					assignDefaultPropertyValue(property, preventBounceBack, outValue);
			}

			if (settings().printBits) {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"   read %s %s, 1 bit (default)", 
				descriptor.type.name.c_str(),
				descriptor.name.c_str()
				);
			}
		}
		else
		{
			const int start = inBitstream.GetReadOffset();

			deserializePropertyValue(inBitstream, property, useDictionary, preventBounceBack, outValue);

			if (settings().printBits) {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"   read %s %s, %d bits", 
				descriptor.type.name.c_str(),
				descriptor.name.c_str(),
				inBitstream.GetReadOffset() - start + 1
				);
			}
		}
	}
}

// Reads the packet for a new Instance:
// 1) ID of object
// 2) Class
// 3) Non-cacheable properties (strings, refprop, systemAddress, stuff we use a dictionary for) 
// 3) Cacheable Properties  (except for Parent property)
// 4) Parent property
//
void Replicator::readInstanceNew(RakNet::BitStream& inBitstream, bool isJoinData)
{
	int start = inBitstream.GetReadOffset();

	RBX::Guid::Data id;
	if (!isJoinData)
		deserializeId(inBitstream, id);
	else
		deserializeIdWithoutDictionary(inBitstream, id);

	// Get the class and construct the object
	const Reflection::ClassDescriptor* classDescriptor;
	unsigned int classId = classDictionary.receive(inBitstream, classDescriptor, false /*we don't do version check for class here, if the properties or events of the class are changed, they will be handled later*/);

    if (ProcessOutdatedInstance(inBitstream, isJoinData, id, classDescriptor, classId))
    {
        return;
    }

	if (settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication: %s:%s << %s",								// note: remote player always on the left
			classDescriptor->name.c_str(), 
			id.readableString().c_str(), 
			RakNetAddressToString(remotePlayerId).c_str()
			);
	}

	shared_ptr<Instance> instance;
	ReplicationData* data;
	if (guidRegistry->lookupByGuid(id, instance))
	{
		// We got back an object we've already seen
		if (!instance)
			throw RBX::runtime_error("readInstanceNew got a null object (guid %s)", id.readableString(32).c_str());

		if (instance->getDescriptor()!=*classDescriptor)
		{
			std::string message = RBX::format("Replication: Bad re-binding %s-%s << %s, %s-%s", 
				classDescriptor->name.c_str(), 
				id.readableString().c_str(), 
				RakNetAddressToString(remotePlayerId).c_str(), 
				instance->getClassName().c_str(), 
				id.readableString().c_str());

			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "%s", message.c_str());

			throw RBX::runtime_error("%s", message.c_str());
		}
	}
	else
	{
		// TODO: Is this the most efficient way to create by ClassDescriptor???
		instance = Creatable<Instance>::createByName(classDescriptor->name, RBX::ReplicationCreator);
		if (!instance)
		{
			std::string message = format("Replication: Can't create object of type %s", classDescriptor->name.c_str());
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, message);
			throw std::runtime_error(message);
		}

		FASTLOG1(FLog::NetworkInstances, "Instance created from replication: %p", instance.get());
	
		guidRegistry->assignGuid(instance.get(), id);
	}

	if (!dynamic_cast<Service*>(instance.get()))
	{
		data = &addReplicationData(instance, false, true);
		// Read ownership flag
		inBitstream >> data->deleteOnDisconnect;
	}
	else
	{
		data = NULL;
		
		// Dummy read ownership flag
		bool deleteOnDisconnect;
		inBitstream >> deleteOnDisconnect;
		RBXASSERT(!deleteOnDisconnect);
	}

	RBX::Guid::Data parentId;
	if (!isJoinData)
	{
        readProperties(inBitstream, instance.get(), PropertyCacheType_NonCacheable, true);
        readProperties(inBitstream, instance.get(), PropertyCacheType_Cacheable, true);
		deserializeId(inBitstream, parentId);
	}
	else
	{
        readProperties(inBitstream, instance.get(), PropertyCacheType_All, false);
		deserializeIdWithoutDictionary(inBitstream, parentId);
	}

	// Look up the parent ahead of time so that we can pass it in to isLegalReceiveInstance
	shared_ptr<Instance> parent;
	guidRegistry->lookupByGuid(parentId, parent);

	const bool isService = dynamic_cast<Service*>(instance.get()) != NULL;

    if (!isService)
	{
		// We're counting on the parent having been replicated first. If not, then
		// our job is much more complicated. We'd have to defer the whole isLegalReceiveInstance
		// logic until the parent reference is resolved.
		LEGACY_ASSERT(parent);
		LEGACY_ASSERT(parent != instance);
		LEGACY_ASSERT(instance->getParent() == NULL);
	}

	bool reject = !isLegalReceiveInstance(instance.get(), parent.get());

	// Assign the Parent property
	if (isService)
	{
		// This is a hack to avoid trying to set the Parent property of a Service
	}
	else if (!reject)
	{
		RBXASSERT(data);
 		RBXASSERT(!data->listenToChanges);
		data->listenToChanges = true;
		RBXASSERT(Name::lookup("Terrain") == classDescriptor->name ||
			data->connection.connected());
		
		// no ReplicatedFirst members are allowed to replicate anything once received!
		if (!isCloudEdit())
		{
			ReplicatedFirst* replicatedFirst = RBX::ServiceProvider::find<ReplicatedFirst>(parent.get());
			if (parent.get() == replicatedFirst || parent->isDescendantOf(replicatedFirst))
			{
				closeReplicationItem(*data);
			}
		}

        // Player object is usually parented inside ServerReplicator::installRemotePlayer
		if (!prepareRemotePlayer(instance))
		{
			FASTLOG2(FLog::NetworkInstances, "Setting instance %p parent: %p", instance.get(), parent.get());

			assignParent(instance.get(), parent.get());
		}
	}
	else
	{
		// unregister rejected item
		guidRegistry->unregister(instance.get());
	}

	// TODO: set deserializingProperty for each resolving Property!!!!
	if (!reject)
		resolvePendingReferences(instance.get(), id);

	const int stop = inBitstream.GetReadOffset();

	if (statsItem)
	{
		statsItem->instanceCount++;
		statsItem->instanceBits += stop - start;
	}

	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceNew);
		replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceNew, (stop-start)/8);
	}
}

void Replicator::readInstanceNewItem(DeserializedNewInstanceItem* item, bool isJoinData)
{
	RBXASSERT(item->instance);
	if (!item->instance)
	{
		std::string message = "Replication: deserialized item contain null object " + item->id.readableString();
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, message.c_str());
		throw std::runtime_error(message);
	}
	
	const shared_ptr<Instance>& instance = item->instance;
	const bool isService = dynamic_cast<Service*>(instance.get()) != NULL;

	ReplicationData* data;
	if (!isService)
	{
		data = &addReplicationData(instance, false, true);
		// Set ownership flag
		data->deleteOnDisconnect = item->deleteOnDisconnect;
	}
	else
		data = NULL;

	// load all prop values into instance
	if (!item->propValueList.empty())
	{
		readPropertiesFromValueArray(item->propValueList, instance.get());
	}

	// Look up the parent ahead of time so that we can pass it in to isLegalReceiveInstance
	const shared_ptr<Instance>& parent = item->parent;

	if (!isService)
	{
		// We're counting on the parent having been replicated first. If not, then
		// our job is much more complicated. We'd have to defer the whole isLegalReceiveInstance
		// logic until the parent reference is resolved.
		LEGACY_ASSERT(parent);
		LEGACY_ASSERT(parent != instance);
		LEGACY_ASSERT(instance->getParent() == NULL);
	}

	bool reject = !isLegalReceiveInstance(instance.get(), parent.get());

	// Assign the Parent property
	if (isService)
	{
		// This is a hack to avoid trying to set the Parent property of a Service
	}
	else if (!reject)
	{
		RBXASSERT(data);
		RBXASSERT(!data->listenToChanges);
		data->listenToChanges = true;
		RBXASSERT(data->connection.connected());

		if (!isCloudEdit())
		{
			// no ReplicatedFirst members are allowed to replicate anything once received!
			ReplicatedFirst* replicatedFirst = RBX::ServiceProvider::find<ReplicatedFirst>(parent.get());
			if (parent.get() == replicatedFirst || parent->isDescendantOf(replicatedFirst))
			{
				closeReplicationItem(*data);
			}
		}

		// Player object is usually parented inside ServerReplicator::installRemotePlayer
		if (!prepareRemotePlayer(instance))
		{
			FASTLOG2(FLog::NetworkInstances, "Setting instance %p parent: %p", instance.get(), parent.get());

			assignParent(instance.get(), parent.get());
		}
	}
	else
	{
		// unregister rejected item
		guidRegistry->unregister(instance.get());
	}

	// TODO: set deserializingProperty for each resolving Property!!!!
	if (!reject)
		resolvePendingReferences(instance.get(), item->id);
}

struct CellUpdateFilter {
	ClusterUpdateBuffer* buffer;
	bool clusterDebounceEnabled;
	bool canSet(const Vector3int16& pos) {
		return !clusterDebounceEnabled || !buffer->chk(pos);
	}
};

void Replicator::receiveCluster(RakNet::BitStream& inBitstream, Instance* instance, bool usingOneQuarterIterator)
{
	if(instance == NULL)
		return;
	RBXASSERT(instance == megaClusterInstance);
	if (instance != megaClusterInstance) {
		return;
	}

    if (megaClusterInstance->isSmooth())
    {
        char chunkSizeLog2;
        inBitstream >> chunkSizeLog2;

        Voxel2::BitSerializer<RakNet::BitStream> serializer;

        while (chunkSizeLog2)
		{
            // This is true for client and false for server
            // Client does not need to echo updates it got from the server - it's pointless since server
            // sent the data.
            // Server has to echo *all* updates it got from the clients, including the client that sent
            // the update - otherwise there can be a ships-in-the-night scenario:
            // * server changes a chunk
            // * client changes same chunk
            // 1) server makes change locally
            // 2) client makes change locally
            // 3) server receives client change, makes chunk look like client
            // 4) client receives server change, makes chunk like (old) server
            // leading to a desync.
            clusterDebounce = clusterDebounceEnabled;

            unsigned int chunkSize = 1 << chunkSizeLog2;
            Voxel2::Box box(chunkSize, chunkSize, chunkSize);

            Vector3int32 chunkId;
            serializer.decodeIndex(chunkId, inBitstream);
            serializer.decodeContent(box, inBitstream);

            // If we're on the client and we're doing frequent updates, we'll get these updates back (since
            // debouncing is disabled on the server).
            // This is fine if we're just updating a chunk once; however, if we're updating it many times incrementally,
            // we can get a packet from the server that echoes our older update with stale data, which will overwrite
            // the more recent data.
            // So let's skip these updates.
            bool skipChunk =
                clusterDebounceEnabled &&
                chunkSizeLog2 == ClusterReplicationData::kUpdateChunkSizeLog2 &&
                clusterReplicationData.updateBufferSmooth.count(chunkId);

            if (!skipChunk)
                megaClusterInstance->getSmoothGrid()->write(Voxel2::Region::fromChunk(chunkId, chunkSizeLog2), box);

            clusterDebounce = false;

            inBitstream >> chunkSizeLog2;
		}
    }
	else
	{
        // Read chunks
        int chunkId;
        inBitstream >> chunkId;

        ClusterReplicationData& clusterData = clusterReplicationData;
        CellUpdateFilter filter;
        filter.buffer = &(clusterData.updateBuffer);
        filter.clusterDebounceEnabled = clusterDebounceEnabled;

        while (chunkId != CLUSTER_END_TOKEN)
        {
            if (chunkId == CLUSTER_DATA_TOKEN)
            {
                NETPROFILE_START("decodeCells", &inBitstream);
                // don't debounce server side -- this allows server to override client
                // changes if there is a ships-in-the-night scenario:
                // * server changes a cell
                // * client changes same cell cell
                // 1) server makes change locally
                // 2) client makes change locally
                // 3) server receives client change, makes cell look like client
                // 4) client receives server change, makes cell like (old) server
                //
                // if server debouncing is off, the server will always send an echo
                // update after it has merged incoming traffic, synchronizing state.
                clusterDebounce = clusterDebounceEnabled;

                Voxel::Serializer serializer;
                if (streamingEnabled && usingOneQuarterIterator)
                {
                    serializer.decodeCells<OneQuarterClusterChunkCellIterator>(megaClusterInstance->getVoxelGrid(), inBitstream, filter);
                }
                else
                {
                    serializer.decodeCells<ClusterChunksIterator>(megaClusterInstance->getVoxelGrid(), inBitstream, filter);
                }

                clusterDebounce = false;

                // read next chunk
                inBitstream >> chunkId;
                NETPROFILE_END("decodeCells", &inBitstream);
            }
            else
            {
                // chunkId != CLUSTER_DATA_TOKEN
                RBXASSERT(false);
                throw std::runtime_error("should only send updates in same format for chunk and delta");
            }
        }
	}
}

void Replicator::deserializeData(RakNet::BitStream& inBitstream, std::vector<shared_ptr<DeserializedItem> >& items)
{
	RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);

	while (true)
	{
		Item::ItemType itemType;
		Item::readItemType(inBitstream, itemType);
		if (itemType == Item::ItemTypeEnd)
			// Done!
				break;
		replicatorStats.lastItem = itemType;

		if (shared_ptr<DeserializedItem> item = deserializeItem(inBitstream, itemType))
			items.push_back(item);
	}
}

void Replicator::receiveData(RakNet::BitStream& inBitstream)
{
	RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
	
	FASTLOG2(FLog::NetworkReadItem, "receiveData bitstream %p, read offset %d", inBitstream.GetData(), inBitstream.GetReadOffset());

	while (true)
	{
		Item::ItemType itemType;
		Item::readItemType(inBitstream, itemType);
		if (itemType == Item::ItemTypeEnd)
			// Done!
			break;
		replicatorStats.lastItem = itemType;

		readItem(inBitstream, itemType);
	}
}

shared_ptr<DeserializedItem> Replicator::deserializeItem(RakNet::BitStream& inBitstream, Item::ItemType itemType)
{
	shared_ptr<DeserializedItem> item = shared_ptr<DeserializedItem>();

	switch (itemType)
	{
	case Item::ItemTypeDelete:
		NETPROFILE_START("readInstanceDelete", &inBitstream);
		item = DeleteInstanceItem::read(*this, inBitstream);
		NETPROFILE_END("readInstanceDelete", &inBitstream);
		break;
	case Item::ItemTypeNew:
		NETPROFILE_START("readInstanceNew", &inBitstream);
		item = NewInstanceItem::read(*this, inBitstream, false /*not join data*/);
		NETPROFILE_END("readInstanceNew", &inBitstream);
		break;
	case Item::ItemTypeChangeProperty:
		NETPROFILE_START("readChangedProperty", &inBitstream);	
		item = ChangePropertyItem::read(*this, inBitstream);
		NETPROFILE_END("readChangedProperty", &inBitstream);
		break;
	case Item::ItemTypeMarker:
		NETPROFILE_START("readMarker", &inBitstream);
		item = MarkerItem::read(*this, inBitstream);
		NETPROFILE_END("readMarker", &inBitstream);
		break;
	case Item::ItemTypePing:
	case Item::ItemTypePingBack:
		NETPROFILE_START("readDataPing", &inBitstream);
		item = PingItem::read(*this, inBitstream);
		NETPROFILE_END("readDataPing", &inBitstream);
		break;
	case Item::ItemTypeEventInvocation:
		NETPROFILE_START("readEventInvocation", &inBitstream);
		item = EventInvocationItem::read(*this, inBitstream);
		NETPROFILE_END("readEventInvocation", &inBitstream);
		break;
	case Item::ItemTypeJoinData:
		NETPROFILE_START("readJoinData", &inBitstream);
		item = JoinDataItem::read(*this, inBitstream);
		NETPROFILE_END("readJoinData", &inBitstream);
		break;
	default:
		RBXASSERT(false);
		throw std::runtime_error("");
	}

	return item;
}

void Replicator::readItem(RakNet::BitStream& inBitstream, Item::ItemType itemType)
{
	FASTLOG3(FLog::NetworkReadItem, "readItem %d, bitStream %p, offset %d", (int)itemType, inBitstream.GetData(), inBitstream.GetReadOffset());

	if (DFFlag::ReadDeSerializeProcessFlow)
	{	
		shared_ptr<DeserializedItem> item = deserializeItem(inBitstream, itemType);
		if (item) item->process(*this);  // make sure it is not NULL
	}
	else
	{
	switch (itemType)
	{
	case Item::ItemTypeDelete:
		NETPROFILE_START("readInstanceDelete", &inBitstream);
		readInstanceDelete(inBitstream);
		NETPROFILE_END("readInstanceDelete", &inBitstream);
		break;
	case Item::ItemTypeNew:
		NETPROFILE_START("readInstanceNew", &inBitstream);
		readInstanceNew(inBitstream, false /*isJoinData*/);
		NETPROFILE_END("readInstanceNew", &inBitstream);
		break;
	case Item::ItemTypeChangeProperty:
		NETPROFILE_START("readChangedProperty", &inBitstream);
		readChangedProperty(inBitstream);
		NETPROFILE_END("readChangedProperty", &inBitstream);
		break;
	case Item::ItemTypeMarker:
		NETPROFILE_START("readMarker", &inBitstream);
		readMarker(inBitstream);
		NETPROFILE_END("readMarker", &inBitstream);
		break;
	case Item::ItemTypePing:
	case Item::ItemTypePingBack:
		NETPROFILE_START("readDataPing", &inBitstream);
		readDataPing(inBitstream);
		NETPROFILE_END("readDataPing", &inBitstream);
		break;
	case Item::ItemTypeEventInvocation:
		NETPROFILE_START("readEventInvocation", &inBitstream);
		readEventInvocation(inBitstream);
		NETPROFILE_END("readEventInvocation", &inBitstream);
		break;
	case Item::ItemTypeJoinData:
		NETPROFILE_START("readJoinData", &inBitstream);
		readJoinData(inBitstream);
		NETPROFILE_END("readJoinData", &inBitstream);
		break;
	default:
		RBXASSERT(false);
		throw std::runtime_error("");
	}
}
}

void Replicator::sendDataPing()
{
	RakNet::Time timeStamp = RakNet::GetTimeMS();
#if defined(RBX_RCC_SECURITY)
    if (!enableHashCheckBypass && !DFFlag::US25317p1 && 
        (timeStamp - replicatorStats.lastReceivedHashTime) > 120000)
    {
        StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "User has stopped sending memory hashes:  %s\n", RakNetAddressToString(remotePlayerId).c_str());
        FASTLOGS(FLog::Network, "User has stopped sending memory hashes:  %s", RakNetAddressToString(remotePlayerId).c_str());
        sendDisconnectionSignal(RakNetAddressToString(remotePlayerId), true);
        requestDisconnect(DisconnectReason_HashTimeOut);
    }
    if (!enableMccCheckBypass && !DFFlag::US25317p2 && 
        (timeStamp - replicatorStats.lastReceivedMccTime) > 120000)
    {
        StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "User has stopped sending Mcc Reports:  %s\n", RakNetAddressToString(remotePlayerId).c_str());
        FASTLOGS(FLog::Network, "User has stopped sending Mcc Reports:  %s", RakNetAddressToString(remotePlayerId).c_str());
        sendDisconnectionSignal(RakNetAddressToString(remotePlayerId), true);
        requestDisconnect(DisconnectReason_HashTimeOut);
    }
#endif 
    if (canTimeout && ((timeStamp - replicatorStats.lastReceivedPingTime) > 120000) && !DFFlag::DebugDisableTimeoutDisconnect)
	{
		// disconnect player if we haven't received their ping in over 2 mins
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Lost connection to %s, timed out\n", RakNetAddressToString(remotePlayerId).c_str());
		FASTLOGS(FLog::Network, "Lost connection to %s due timeout", RakNetAddressToString(remotePlayerId).c_str());
		sendDisconnectionSignal(RakNetAddressToString(remotePlayerId), true);
		requestDisconnect(DisconnectReason_TimeOut);
	}
	else
		pendingItems.push_back(new (pingPool.get()) PingItem(this, timeStamp, kNoScornFlags));
}

void Replicator::sendStats(int version)
{
	pendingItems.push_back(new StatsItem(this, version));
}

void Replicator::readDataPing(RakNet::BitStream& inBitstream)
{
    int start = inBitstream.GetReadOffset();
	bool isPingBack;
	inBitstream >> isPingBack;
	RakNet::Time timeStamp;
	inBitstream >> timeStamp;

	unsigned int sendStats;
	unsigned int extraStats = 0;
	inBitstream >> sendStats;
    if (canUseProtocolVersion(34))
    {
	    inBitstream >> extraStats;
#if defined(RBX_RCC_SECURITY)
        if (timeStamp & 0x20) // change things up occaasionally
        {
            extraStats = ~extraStats;
        }
#endif
    }
	processSendStats(sendStats, extraStats);

	processDataPing(isPingBack, timeStamp);

	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_Ping);
        replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_Ping, (inBitstream.GetReadOffset()-start)/8);
    }
}

void Replicator::readDataPingItem(DeserializedPingItem* item)
{
	processSendStats(item->sendStats, item->extraStats);
	processDataPing(item->pingBack, item->time);
}

void Replicator::processDataPing(bool isPingBack, RakNet::Time timeStamp)
{
	if (isPingBack) {
		int elapsedTime = (int)(RakNet::GetTimeMS() - timeStamp);
		FASTLOG1(FLog::NetworkStepsMultipliers, "Ping Elapsed Time: %d", elapsedTime);
		replicatorStats.dataPing.sample(elapsedTime);
	} 
	else 
	{
		pendingItems.push_back(new (pingBackPool.get()) PingBackItem(this, timeStamp, kNoScornFlags));
	}

    checkPingItemTime();

	replicatorStats.lastReceivedPingTime = RakNet::GetTimeMS();
}

void Replicator::readMarkerItem(DeserializedMarkerItem* item)
{
	processMarker(item->id);
}

void Replicator::readMarker(RakNet::BitStream& inBitstream)
{
	int id;
	inBitstream >> id;
	processMarker(id);
}

void Replicator::processMarker(int id)
{
	if (settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Received marker %d from %s", id,
			RakNetAddressToString(remotePlayerId).c_str());
	}

	FASTLOG1(FLog::Network, "Replicator:ReadMarker id(%d)", id);

	if (incomingMarkers.size() > 0)
	{
		RBXASSERT(id==incomingMarkers.front()->id);
		incomingMarkers.front()->fireReturned();
		incomingMarkers.pop();
	}
	else
		RBXASSERT(0);
}

FilterResult Replicator::filterPhysics(PartInstance* instance)
{
	return Accept;
}

void Replicator::readChangedPropertyItem(DeserializedChangePropertyItem* item)
{
	Instance* instance = item->instance.get();
	if (instance)
	{
		if (filterReceivedChangedProperty(instance, *item->propertyDescriptor) == Reject)
		{
			instance = NULL;
		}
		else if (!isLegalReceiveProperty(instance, *item->propertyDescriptor))
		{
			instance = NULL;
		}
	}

	if (instance)
		readChangedPropertyItem(item, Reflection::Property(*item->propertyDescriptor, instance));
}

void Replicator::readChangedProperty(RakNet::BitStream& inBitstream)
{
    int start = inBitstream.GetReadOffset();
	shared_ptr<Instance> instance;
	RBX::Guid::Data id;
	deserializeInstanceRef(inBitstream, instance, id);

    // Read the property name
	const Reflection::PropertyDescriptor* propertyDescriptor;
	unsigned int propId = propDictionary.receive(inBitstream, propertyDescriptor, true);
    if (ProcessOutdatedChangedProperty(inBitstream, id, instance.get(), propertyDescriptor, propId))
    {
        return;
    }

	if (!propertyDescriptor)
		throw RBX::runtime_error("Replicator readChangedProperty NULL descriptor");

	if (instance)
    {
		if (filterReceivedChangedProperty(instance.get(), *propertyDescriptor) == Reject)
        {
			instance.reset();
        }
		else if (!isLegalReceiveProperty(instance.get(), *propertyDescriptor))
        {
			instance.reset();
        }
		else if (instance && !instance->getDescriptor().isA(propertyDescriptor->owner))
		{
			throw RBX::runtime_error("Replication: Bad re-binding prop %s-%s << %s",
				instance->getClassName().c_str(),
				propertyDescriptor->name.c_str(), 
				RakNetAddressToString(remotePlayerId).c_str());
		}
    }
#ifdef NETWORK_DEBUG
    //if (propertyDescriptor->name == "CFrame")
    //{
    //    StandardOut::singleton()->printf(MESSAGE_WARNING, "[ChangedProperty] Received property (%s) for %s", propertyDescriptor->name.c_str(), instance ? instance->getClassName().c_str() : "?");
    //}
#endif

	readChangedProperty(inBitstream, Reflection::Property(*propertyDescriptor, instance.get()));

	if (settings().printProperties) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication: %s-%s.%s << %s, bytes: %d",							// remote player always on right side 
			instance ? instance->getClassName().c_str() : "?", 
			id.readableString().c_str(), 
			propertyDescriptor->name.c_str(),
			RakNetAddressToString(remotePlayerId).c_str(),
			(inBitstream.GetReadOffset()-start)/8
			);
	}
	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsReceived(propertyDescriptor->category.str);
		replicatorStats.samplePacketsReceived(propertyDescriptor->category.str, (inBitstream.GetReadOffset()-start)/8);
	}
}

void Replicator::processChangedParentProperty(Guid::Data parentId, Reflection::Property prop)
{
	RBXASSERT(prop.getDescriptor() == Instance::propParent);

	Instance* instance = static_cast<Instance*>(prop.getInstance());

	// Look up the parent ahead of time so that we can pass it in to isLegalReceiveInstance
	shared_ptr<Instance> parent;
	bool recognizedId = guidRegistry->lookupByGuid(parentId, parent);

	if (!parent)
	{
		// parent could be an instance from an unknown class
		return;
	}

	// We're counting on the parent having been replicated first. If not, then 
	// our job is much more complicated. We'd have to defer the whole filterReceivedParent
	// logic until the parent reference is resolved.
	RBXASSERT(recognizedId);

	if (instance)
	{
		// Optimization
		if (recognizedId)
			if (parent.get() == instance->getParent())
				return;

		if (filterReceivedParent(instance, parent.get()) == Reject)
			return;

    	// Here is where the Parent property is finally set:
		assignParent(instance, parent.get());
	}
}

void Replicator::readChangedPropertyItem(DeserializedChangePropertyItem* item, Reflection::Property prop)
{
	const Reflection::PropertyDescriptor& descriptor = prop.getDescriptor();

	if (descriptor == Instance::propParent)
	{
		Guid::Data id = item->value.get<Guid::Data>();
		processChangedParentProperty(id, prop);
	}
	else
	{
		ScopedAssign<const Reflection::Property*> assign(deserializingProperty, &prop);
		if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(descriptor))
		{
			Guid::Data id = item->value.get<Guid::Data>();
			assignRef(prop, id);
		}
		else if (descriptor.bIsEnum)
		{
			const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(descriptor.type);
			RBXASSERT(enumDesc);
			const Reflection::EnumDescriptor::Item* enumDescItem = enumDesc->lookup(item->value);
			RBXASSERT(enumDescItem);

			const Reflection::EnumPropertyDescriptor& enumPropDesc = static_cast<const Reflection::EnumPropertyDescriptor&>(descriptor);
			enumPropDesc.setEnumItem(prop.getInstance(), *enumDescItem);				
		}
		else if (descriptor.type == Reflection::Type::singleton<RBX::ContentId>())
		{
			RBX::ContentId value = item->value.get<RBX::ContentId>();
			descriptor.setStringValue(prop.getInstance(), value.toString());
		}
		else
		{
			if (prop.getInstance())
				descriptor.setVariant(prop.getInstance(), item->value);
		}
	}
}

void Replicator::readChangedProperty(RakNet::BitStream& bitStream, Reflection::Property prop)
{
    // if you make changes, please also take care of Replicator::skipChangedProperty
	if (prop.getDescriptor() == Instance::propParent)
	{
		RBX::Guid::Data parentId;
		deserializeId(bitStream, parentId);
		processChangedParentProperty(parentId, prop);
	}
	else
		deserializePropertyValue(bitStream, prop, true/*useDictionary*/, true, NULL);
}

void Replicator::readEventInvocationItem(DeserializedEventInvocationItem* item)
{
	Instance* instance = item->instance.get();
	if (!isLegalReceiveEvent(instance, *item->eventDescriptor))
	{
		instance = NULL;
	}

	if (instance)
	{
		// resolve ref arguments
		for (Reflection::EventArguments::iterator iter = item->eventInvocation->args.begin(); iter != item->eventInvocation->args.end(); ++iter)
		{
			const Reflection::Type& type = (*iter).type();
			if (type == Reflection::Type::singleton<Guid::Data>())
			{
				Guid::Data id;
				id = iter->get<Guid::Data>();

				shared_ptr<Instance> instance;
				guidRegistry->lookupByGuid(id, instance);

				*iter = instance;
			}
		}

		instance->processRemoteEvent(*item->eventDescriptor, item->eventInvocation->args, RakNetToRbxAddress(remotePlayerId));

		if(item->eventDescriptor->isBroadcast()){
			deserializingEventInvocation = item->eventInvocation.get();

			//If we're a ServerReplicator, then this will handle the rebounce
			rebroadcastEvent(*item->eventInvocation);

			deserializingEventInvocation = NULL;
		}
	}
}

void Replicator::readEventInvocation(RakNet::BitStream& inBitstream)
{
	shared_ptr<Instance> instance;
	RBX::Guid::Data id;
	deserializeInstanceRef(inBitstream, instance, id);

	// Read the property name
	const Reflection::EventDescriptor* eventDescriptor;
	unsigned int eventId = eventDictionary.receive(inBitstream, eventDescriptor, true);

    if (ProcessOutdatedEventInvocation(inBitstream, id, instance.get(), eventDescriptor, eventId))
    {
        return;
    }

	if (settings().printEvents)
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
			"Replication: %s-%s.%s << %s",							// remote player always on right side 
			instance ? instance->getClassName().c_str() : "?", 
			id.readableString().c_str(), 
			eventDescriptor->name.c_str(),
			RakNetAddressToString(remotePlayerId).c_str() 
		);

	if (!isLegalReceiveEvent(instance.get(), *eventDescriptor))
	{
		instance.reset();
	}
	else if (instance && !instance->getDescriptor().isA(eventDescriptor->owner))
	{
		throw RBX::runtime_error("Replication: Bad re-binding event %s-%s << %s",
			instance->getClassName().c_str(),
			eventDescriptor->name.c_str(), 
			RakNetAddressToString(remotePlayerId).c_str());
	}

	Reflection::EventInvocation eventInvocation(Reflection::Event(*eventDescriptor, instance));
	deserializeEventInvocation(inBitstream, eventInvocation);

	if (instance)
	{
        instance->processRemoteEvent(*eventDescriptor, eventInvocation.args, RakNetToRbxAddress(remotePlayerId));

		if(eventDescriptor->isBroadcast()){
			deserializingEventInvocation = &eventInvocation;

			//If we're a ServerReplicator, then this will handle the rebounce
			rebroadcastEvent(eventInvocation);

			deserializingEventInvocation = NULL;
		}
	}
}

unsigned int Replicator::readJoinData(RakNet::BitStream& inBitstream)
{
	inBitstream.AlignReadToByteBoundary();

    unsigned int count;
	inBitstream >> count;
	
	if (count > 0)
	{
		// currently we only support reading data from BitStream, so need to copy decompressed data into a bitstream for reading
		RakNet::BitStream bitstream;

		decompressBitStream(inBitstream, bitstream);

		for (unsigned int i = 0; i < count; i++)
		{
			readInstanceNew(bitstream, true /*isJoinData*/);

			bitstream.AlignReadToByteBoundary();
		}
	}

    // Exploiters might not send a player, and we assume this creates the player.
    checkRemotePlayer();

	return count;
}

unsigned int Replicator::readJoinDataItem(DeserializedJoinDataItem* item)
{
	for (int i = 0; i < item->numInstances; i++)
	{
		readInstanceNewItem(&item->instanceInfos[i], true);
	}

	return item->numInstances;
}

void Replicator::closeConnection()
{
	if (rakPeer)
		rakPeer->rawPeer()->CloseConnection(remotePlayerId, true);
	// Remove myself from the chain
	unlockParent();
	setParent(NULL);
}

void Replicator::deleteInstanceById(Guid::Data id)
{
	shared_ptr<Instance> instance;
	if (guidRegistry->lookupByGuid(id, instance))
	{
		RBXASSERT(instance);

		bool rejected = !isLegalDeleteInstance(instance.get());

		if (settings().printInstances) {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
				"Replication: ~%s:%s << %s", 
				instance ? instance->getClassName().c_str() : "NULL", 
				id.readableString().c_str(),
				RakNetAddressToString(remotePlayerId).c_str() 
				);
		}

		if (!rejected)
			if (disconnectReplicationData(instance))
			{
				removeFromPendingNewInstances(instance.get());

				FASTLOG1(FLog::NetworkInstances, "Replicating unparenting instance %p", instance.get());
				RBXASSERT(removingInstance==NULL);
				RBX::ScopedAssign<Instance*> assign(removingInstance, instance.get());
				instance->setParent(NULL);

			}
	}
	else
	{
		if (settings().printInstances) {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
				"Replication ~??? << %s",
				RakNetAddressToString(remotePlayerId).c_str());
		}
	}

	// Resolve the binding to NULL, since the object is being deleted
	resolvePendingReferences(NULL, id);
}

void Replicator::readInstanceDeleteItem(DeserializedDeleteInstanceItem* item)
{
	deleteInstanceById(item->id);
}

void Replicator::readInstanceDelete(RakNet::BitStream& inBitstream)
{
    int start = inBitstream.GetReadOffset();
	RBX::Guid::Data id;

	deserializeId(inBitstream, id);
	deleteInstanceById(id);

	if (settings().trackDataTypes) {
		replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceDelete);
		replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_InstanceDelete, (inBitstream.GetReadOffset()-start)/8);
	}

}

void Replicator::processPacket(Packet *packet)
{
	RakNet::BitStream inBitstream(packet->data, packet->length, false);
    replicatorStats.packetsReceived.sample();

	replicatorStats.lastPacketType = packet->data[0];
    int bitStart = inBitstream.GetReadOffset();
	
	FASTLOG1(DFLog::NetworkPacketsReceive, "ProcessPacket %d start", (int)packet->data[0]);

	switch (packet->data[0])
	{
	case ID_DATA:
        NETPROFILE_START("ID_DATA", &inBitstream);
        inBitstream.IgnoreBits(8); // Ignore the packet id
		receiveData(inBitstream);
		replicatorStats.dataPacketsReceived.sample();
        replicatorStats.dataPacketsReceivedSize.sample((inBitstream.GetReadOffset() - bitStart)/8);
        NETPROFILE_END("ID_DATA", &inBitstream);
		break;

	case ID_CLUSTER:
        {
		    NETPROFILE_START("ID_CLUSTER", &inBitstream);
		    inBitstream.IgnoreBits(8); // Ignore the packet id
            bool usingOneQuarterIterator = streamingEnabled; // default value
            inBitstream >> usingOneQuarterIterator;

            shared_ptr<Instance> instance;
	        RBX::Guid::Data id;
	        deserializeInstanceRef(inBitstream, instance, id);

		    receiveCluster(inBitstream, instance.get(), usingOneQuarterIterator);
		    replicatorStats.clusterPacketsReceived.sample();
            replicatorStats.clusterPacketsReceivedSize.sample((inBitstream.GetReadOffset() - bitStart)/8);
		    NETPROFILE_END("ID_CLUSTER", &inBitstream);
        }
		break;

	case ID_TIMESTAMP:
		{
			if (physicsReceiver.get())
			{
				NETPROFILE_START("ID_TIMESTAMP", &inBitstream);

				inBitstream.IgnoreBits(8); // Ignore the packet id

				RakNet::Time timeStamp;
				NETPROFILE_START("timeStamp", &inBitstream);
				inBitstream >> timeStamp;
				NETPROFILE_END("timeStamp", &inBitstream);
				unsigned char id;
				inBitstream >> id;
				RBXASSERT (id==ID_PHYSICS);
				
				try
				{
					NETPROFILE_START("receivePacket", &inBitstream);
					ReplicatorStats::PhysicsReceiverStats* stats = NULL;
					if (settings().trackPhysicsDetails)
					{
						stats = &replicatorStats.physicsReceiverStats;
					}
					physicsReceiver->receivePacket(inBitstream, timeStamp, stats);
					NETPROFILE_END("receivePacket", &inBitstream);
				}
				catch (RBX::base_exception& e)
				{
					// convert to physics exception
					throw RBX::physics_receiver_exception(e.what());
				}

				replicatorStats.physicsPacketsReceived.sample();
                replicatorStats.physicsPacketsReceivedSize.sample((inBitstream.GetReadOffset() - bitStart)/8);
				NETPROFILE_END("ID_TIMESTAMP", &inBitstream);
			}
			else
			{
#ifdef NETWORK_DEBUG
                StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Received physics packet before PhysicsReceiver is created.");
#endif
				//RBXCRASH();			// we crash here because if not in distributed physics mode, there should be no physicsReceiver on the server side
			}
		}
		break;

	case ID_PHYSICS_TOUCHES:
		{
			if (physicsReceiver.get())
			{
				NETPROFILE_START("ID_PHYSICS_TOUCHES", &inBitstream);

				inBitstream.IgnoreBits(8); // Ignore the packet id
				
				physicsReceiver->readTouches(inBitstream, packet->systemAddress);

                replicatorStats.touchPacketsReceived.sample();
                replicatorStats.touchPacketsReceivedSize.sample((inBitstream.GetReadOffset() - bitStart)/8);
				NETPROFILE_END("ID_PHYSICS_TOUCHES", &inBitstream);
			}
			else
			{
				//RBXCRASH();			// we crash here because if not in distributed physics mode, there should be no physicsReceiver on the server side
			}
		}
		break;
	case ID_TEACH_DESCRIPTOR_DICTIONARIES:
		{
#ifdef NETWORK_DEBUG
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Syncing dictionaries..");
#endif
			NETPROFILE_START("ID_TEACH_DESCRIPTOR_DICTIONARIES", &inBitstream);

			inBitstream.IgnoreBits(8); // Ignore the packet id
            bool learnSchema = (!isServerReplicator()) && protocolSyncEnabled;
            bool compressedDictionary = learnSchema && apiDictionaryCompression;

            learnDictionaries(inBitstream, compressedDictionary, learnSchema);

			NETPROFILE_END("ID_TEACH_DESCRIPTOR_DICTIONARIES", &inBitstream);
#ifdef NETWORK_DEBUG
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Dictionaries and SFFlags synced.");
#endif
		}
		break;
	}

	FASTLOG2(DFLog::NetworkPacketsReceive, "ProcessPacket %d end, bytes read %d", (int)packet->data[0], (inBitstream.GetReadOffset() - bitStart)/8);
}

static void scheduledRemove(shared_ptr<Instance> instance)
{
	FASTLOG1(FLog::Network, "Removing replicator instance: %p", instance.get());

	instance->unlockParent();
	instance->remove();
}

void Replicator::teachDictionaries(const Replicator* rep, RakNet::BitStream& bitStream, bool teachSchema, bool toBeCompressed)
{
    rep->classDictionary.teach(bitStream, teachSchema, toBeCompressed);
    rep->propDictionary.teach(bitStream, teachSchema, toBeCompressed);
    rep->eventDictionary.teach(bitStream, teachSchema, toBeCompressed);
    rep->typeDictionary.teach(bitStream, teachSchema, toBeCompressed);
    rep->serializeSFFlags(bitStream);
}

void Replicator::sendDictionaries()
{
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "sendDictionaries");
#endif
    RakNet::BitStream bitStream;
    bitStream << (unsigned char) ID_TEACH_DESCRIPTOR_DICTIONARIES;

    bool teachSchema = isServerReplicator();

    teachDictionaries(this, bitStream, teachSchema, false);

    // Send ID_TEACH_DESCRIPTOR_DICTIONARIES
    rakPeer->rawPeer()->Send(&bitStream, settings().getDataSendPriority(),
        DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
}

void Replicator::sendDisconnectionSignal(std::string peer, bool lostConnection)
{
    if (connected)
    {
        connected = false;
        disconnectionSignal(peer, lostConnection);
    }
}

void Replicator::enableDeserializePacketThread()
{
	boost::mutex::scoped_lock lock(receivedPacketsMutex);

	deserializePacketsThreadEnabled = true;

	// start packet deserialize thread
	deserializePacketsThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&Replicator::deserializePacketsThreadImpl, shared_from(this)), "Deserialize Packets Thread")));

	// transfer packets to the list used in the deserialize thread
	Packet* packet;
	while (incomingPackets.pop_if_present(packet))
		receivedPackets.push_back(packet);
}

void Replicator::deserializePacketsThreadImpl()
{
    Profiler::onThreadCreate("DeserializePackets");
    
	bool addToDeserializedPacketsList = true;
	bool deserialized = false;
	while (deserializePacketsThreadEnabled)
	{
		if (packetsToDeserailze.empty() && !receivedPackets.empty())
		{
			boost::mutex::scoped_lock lock(receivedPacketsMutex);
			std::swap(receivedPackets, packetsToDeserailze);
		}

		if (!packetsToDeserailze.empty())
		{
			Packet* receivedPacket = packetsToDeserailze.front();
			packetsToDeserailze.pop_front();
            
            RBXPROFILER_SCOPE("Network", "deserializePacket");
            RBXPROFILER_LABELF("Network", "ID %d (%d bytes)", receivedPacket->data[0], receivedPacket->length);

			addToDeserializedPacketsList = true;
			deserialized = false;
			DeserializedPacket deserializedPacket(receivedPacket);
			
			RakNet::BitStream inBitstream(receivedPacket->data, receivedPacket->length, false);
			inBitstream.IgnoreBits(8); // Ignore the packet id

			try
			{
				if (receivedPacket->data[0] == ID_DATA)
				{
					deserializeData(inBitstream, deserializedPacket.deserializedItems);
					deserialized = true;
				}
				else if (receivedPacket->data[0] == ID_PHYSICS_TOUCHES)
				{
					if (physicsReceiver.get())
					{
						shared_ptr<DeserializedTouchItem> deserializedTouchItem(new DeserializedTouchItem());
						physicsReceiver->deserializeTouches(inBitstream, receivedPacket->systemAddress, deserializedTouchItem->touchPairs);
						deserializedPacket.deserializedItems.push_back(deserializedTouchItem);
					}
					deserialized = true;
				}
			}
			catch(std::out_of_range& e) // Workaround for iPad, where exceptions can't be caught by ancestor class
			{
				logPacketError(deserializedPacket.rawPacket, "Stream", e.what());
				rakPeer->DeallocatePacket(deserializedPacket.rawPacket);
				requestDisconnectWithSignal(DisconnectReason_ReceivePacketError);
				break;
			}
			catch (RBX::network_stream_exception& e)
			{
				logPacketError(deserializedPacket.rawPacket, "Stream", e.what());
				rakPeer->DeallocatePacket(deserializedPacket.rawPacket);
				requestDisconnectWithSignal(DisconnectReason_ReceivePacketStreamError);
				break;
			}
			catch (RBX::base_exception& e)
			{
				logPacketError(deserializedPacket.rawPacket, "Other", e.what());
				rakPeer->DeallocatePacket(deserializedPacket.rawPacket);
				requestDisconnectWithSignal(DisconnectReason_ReceivePacketError);
				break;
			}
				
			// if we did not deserialize any data items, possibly due to outdated client, skip adding this packet to the deserialized list
			// so the datamodel job (processDeserializedPacket) doesn't try to process the raw packet again
			if (deserialized && deserializedPacket.deserializedItems.size() == 0)
				addToDeserializedPacketsList = false;

			if (addToDeserializedPacketsList)
			{
				bool wasEmpty = deserializedPackets.empty();
				deserializedPackets.push(deserializedPacket);
                if (wasEmpty && processPacketsJob && !TaskScheduler::singleton().isCyclicExecutive())
					TaskScheduler::singleton().reschedule(processPacketsJob);
			}
		}
		else if (receivedPackets.empty())
		{
			// wait for signal from pushIncomingPacket
			packetReceivedEvent.Wait();
		}
	}
    
    Profiler::onThreadExit();
}

PluginReceiveResult Replicator::OnReceive(Packet *packet)
{
	if (packet->systemAddress==remotePlayerId)
	{
		try
		{
			switch (packet->data[0])
			{
			case ID_TIMESTAMP:
				{
                    NETPROFILE_LOG("ID_TIMESTAMP", packet);
					RakNet::BitStream inBitstream(packet->data, packet->length, false);
					inBitstream.IgnoreBits(8); // Ignore the packet id

					RakNet::Time timeStamp;
					inBitstream >> timeStamp;
					unsigned char id;
					inBitstream >> id;
					if (id!=ID_PHYSICS)
						return RR_CONTINUE_PROCESSING;	// Oops! this wasn't what we thought it was...
					
					pushIncomingPacket(packet);
				}
				return RR_STOP_PROCESSING;

			case ID_PHYSICS_TOUCHES:
				NETPROFILE_LOG("ID_PHYSICS_TOUCHES", packet);
				pushIncomingPacket(packet);
				return RR_STOP_PROCESSING;

            case ID_SCHEMA_SYNC:
                NETPROFILE_LOG("ID_SCHEMA_SYNC", packet);
                pushIncomingPacket(packet);
                return RR_STOP_PROCESSING;

			case ID_TEACH_DESCRIPTOR_DICTIONARIES:
				NETPROFILE_LOG("ID_TEACH_DESCRIPTOR_DICTIONARIES", packet);
				pushIncomingPacket(packet);
				return RR_STOP_PROCESSING;

			case ID_CONNECTION_LOST:
				NETPROFILE_LOG("ID_CONNECTION_LOST", packet);
				FASTLOGS(FLog::Network, "Lost connection to %s: ID_CONNECTION_LOST", RakNetAddressToString(packet->systemAddress).c_str());

				StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Lost connection to %s\n", RakNetAddressToString(packet->systemAddress).c_str());
				sendDisconnectionSignal(RakNetAddressToString(packet->systemAddress), true);

				{
				// We can't set parent to NULL in here because we're in the middle of a Raknet Update
				DataModel::get(this)->submitTask(boost::bind(&scheduledRemove, shared_from(this)), DataModelJob::Write);
				}
				return RR_CONTINUE_PROCESSING;

			case ID_DISCONNECTION_NOTIFICATION:
                {
				FASTLOGS(FLog::Network, "Disconnecto from %s: ID_DISCONNECTION_NOTIFICATION", RakNetAddressToString(packet->systemAddress).c_str());
				StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Disconnect from %s", RakNetAddressToString(packet->systemAddress).c_str());
                bool postDisconnect = true;
                if (TeleportService* teleportService = ServiceProvider::find<TeleportService>(this))
                {
                    if (teleportService->attemptingTeleport())
                    {
                        //do not send disconnect signal if we are in the middle of teleporting
                        postDisconnect = false;
                    }
                }
                if (postDisconnect)
                {
				    sendDisconnectionSignal(RakNetAddressToString(packet->systemAddress), false);
    				// We can't set parent to NULL in here because we're in the middle of a Raknet Update
	    			DataModel::get(this)->submitTask(boost::bind(&scheduledRemove, shared_from(this)), DataModelJob::Write);
                }
                }
				return RR_CONTINUE_PROCESSING;

			case ID_DATA:
				NETPROFILE_LOG("ID_DATA", packet);
				pushIncomingPacket(packet);
				return RR_STOP_PROCESSING;

			case ID_CLUSTER:
				NETPROFILE_LOG("ID_CLUSTER", packet);
				pushIncomingPacket(packet);
				return RR_STOP_PROCESSING;

			case ID_REQUEST_MARKER:
				{
					NETPROFILE_LOG("ID_REQUEST_MARKER", packet);
					RakNet::BitStream inBitstream(packet->data, packet->length, false);

					inBitstream.IgnoreBits(8); // Ignore the packet id

					unsigned id;
					inBitstream >> id;

					// Simply send the id back in the standard data stream
					pendingItems.push_back(new MarkerItem(this, id));
				}
				return RR_STOP_PROCESSING_AND_DEALLOCATE;

			case ID_SET_GLOBALS:
				NETPROFILE_LOG("ID_SET_GLOBALS", packet);
				pushIncomingPacket(packet);
				return RR_STOP_PROCESSING;
			case ID_CHAT_GAME:
			case ID_CHAT_TEAM:
			case ID_CHAT_ALL:
			case ID_CHAT_PLAYER:
				NETPROFILE_LOG("CHATS", packet);
				switch (players->OnReceiveChat(getRemotePlayer(), this->rakPeer->rawPeer(), packet, packet->data[0]))
				{
				case Players::PLAYERS_STOP_PROCESSING:
					return RR_STOP_PROCESSING;
				default:
					RBXASSERT(false);
				case Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE:
					return RR_STOP_PROCESSING_AND_DEALLOCATE;
				};

			case ID_REPORT_ABUSE:
				switch (players->OnReceiveReportAbuse(findTargetPlayer(), this->rakPeer->rawPeer(), packet))
				{
				case Players::PLAYERS_STOP_PROCESSING:
					return RR_STOP_PROCESSING;
				default:
					RBXASSERT(false);
				case Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE:
					return RR_STOP_PROCESSING_AND_DEALLOCATE;
				};
			}
		}
		catch (RBX::base_exception& e)
		{
			const char* what = e.what();
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Replicator::OnReceive packet %d: %s", (unsigned char) packet->data[0], what ? what : "empty error string");
			FASTLOGS(FLog::Network, "Error on Replicator::OnReceive: %s", what);

			requestDisconnectWithSignal(DisconnectReason_ReceivePacketError);

			return RR_STOP_PROCESSING_AND_DEALLOCATE;	// We handled the message. Nobody else needs to
		}
	}
	return RR_CONTINUE_PROCESSING;
}


void Replicator::OnInternalPacket(InternalPacket *internalPacket, unsigned frameNumber, RakNet::SystemAddress remoteSystemAddress, RakNet::TimeMS time, int isSend) 
{
	PluginInterface2::OnInternalPacket(internalPacket, frameNumber, remoteSystemAddress, time, isSend);
	if (isSend)
	{
		if (internalPacket->splitPacketCount == 0)
			replicatorStats.numberOfUnsplitMessages++;
		else
		{
			if (settings().printSplitMessages &&
					(internalPacket->splitPacketIndex == 0)) {// only print first split
				StandardOut::singleton()->printf(MESSAGE_INFO,
					"split message, id %u, size %d, split count %d",
					internalPacket->data[0],
					internalPacket->dataBitLength / 8,
					internalPacket->splitPacketCount);
			}
			replicatorStats.numberOfSplitMessages++;

			if (internalPacket->splitPacketCount > (uint32_t)DFInt::RakNetMaxSplitPacketCount)
			{
				RBXASSERT(internalPacket->splitPacketCount < (uint32_t)DFInt::RakNetMaxSplitPacketCount);
				std::stringstream label;
				label << (unsigned int)internalPacket->data[0];
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "NetworkPacketSplitCountOverThreshold", label.str().c_str());
			}
		}
	}
}

void Replicator::requestDisconnect(DisconnectReason reason)
{
	{
        // (security) all of these messages appear in the client and can give info
        // on what security feature are in use.
		std::string r = "none";
		switch (reason)
		{
		case DisconnectReason_BadHash:				r = "BadHash"; break;
		case DisconnectReason_SecurityKeyMismatch:	r = "SecurityKeyMismatch"; break;
		case DisconnectReason_ProtocolMismatch:		r = "ProtocolMismatch"; break;
		case DisconnectReason_ReceivePacketError:	r = "ReceivePacketError"; break;
		case DisconnectReason_ReceivePacketStreamError:	r = "ReceivePacketStreamError"; break;
		case DisconnectReason_SendPacketError:		r = "SendPacketError"; break;
		case DisconnectReason_IllegalTeleport:		r = "IllegalTeleport"; break;
		case DisconnectReason_DuplicatePlayer:		r = "DuplicatePlayer"; break;
		case DisconnectReason_DuplicateTicket:		r = "DuplicateTicket"; break;
		case DisconnectReason_TimeOut:				r = "TimeOut"; break;
		case DisconnectReason_LuaKick:				r = "LuaKick"; break;
		case DisconnectReason_OnRemoteSysStats:		r = "OnRemoteSysStats"; break;
        case DisconnectReason_HashTimeOut:          r = "MagicDisco"; break; // keep this a secret for now.
		case DisconnectReason_CloudEditKick:        r = "CloudEditKick"; break;
		default: break;
		}

		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, isServerReplicator() ? "ServerDisconnectReason" : "ClientDisconnectReason", r.c_str());
	}

	// We can't set parent to NULL in this context because we're in the middle of a Raknet Update
	DataModel::get(this)->submitTask(boost::bind(&scheduledRemove, shared_from(this)), DataModelJob::Write);
}

void Replicator::requestDisconnectWithSignal(DisconnectReason reason)
{
	sendDisconnectionSignal(RakNetAddressToString(remotePlayerId), true);
	requestDisconnect(reason);
}

shared_ptr<Instance> Replicator::sendMarker()
{
	shared_ptr<Marker> marker = Creatable<Instance>::create<Marker>();

	shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
	*bitStream << (unsigned char) ID_REQUEST_MARKER;

	int id = marker->id;

	*bitStream << id;

	FASTLOG1(FLog::Network, "Replicator:SendMarker id(%d)", id);

	if (settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Replicator: Requesting Marker %d of %s",
			id, RakNetAddressToString(remotePlayerId).c_str());
	}

	incomingMarkers.push(marker);

	// Send ID_REQUEST_MARKER
	rakPeer->Send(bitStream, networkSettings->getDataSendPriority(), RELIABLE, DATA_CHANNEL, remotePlayerId, false);

	return marker;
}

size_t Replicator::getAdjustedMtuSize() const
{
	RBXASSERT(settings().getReplicationMtuAdjust() <= 0);
	return std::max(0, replicatorStats.peerStats.mtuSize +
		settings().getReplicationMtuAdjust());
}

size_t Replicator::getPhysicsMtuSize() const {
	RBXASSERT(settings().getPhysicsMtuAdjust() <= 0);
	return std::max(0, replicatorStats.peerStats.mtuSize +
		settings().getPhysicsMtuAdjust());
}

void Replicator::disableProcessPackets()
{
	processPacketsJob->sleep();
}
void Replicator::enableProcessPackets()
{
	processPacketsJob->wake();
}

std::string Replicator::getMetric(const std::string& metric) const
{
	if(metric == "Network Send")
	{
		boost::format fmt("%.1f/s %.1f msec %d%%");
		fmt % sendDataJob->averageStepsPerSecond() % (1000.0 * sendDataJob->averageStepTime()) 
			% (int)(100.0 * sendDataJob->averageDutyCycle());
		return fmt.str();
	}

	if(metric == "Network Receive")
	{
		boost::format fmt("%.1f/s %.1f msec %d%%");
		fmt % processPacketsJob->averageStepsPerSecond() % (1000.0 * processPacketsJob->averageStepTime()) 
			% (int)(100.0 * processPacketsJob->averageDutyCycle());
		return fmt.str();
	}

	return "";
}

double Replicator::getMetricValue(const std::string& metric) const
{
    if(metric == "Network Receive CPU")
    {
        return 100.0 * processPacketsJob->averageDutyCycle();
    }
    if(metric == "Network Receive Time")
    {
        return 1000.0 * processPacketsJob->averageStepTime();
    }
	if (metric == "Total Bytes Received")
	{
		FASTLOG1(FLog::NetworkStatsReport, "Reporting ACTUAL_BYTES_RECEIVED: %d", getRakNetStats()->runningTotal[ACTUAL_BYTES_RECEIVED]);
		return getRakNetStats()->runningTotal[ACTUAL_BYTES_RECEIVED];	
	}
    
	return 0.0;
}

void Replicator::assignRef(Reflection::Property& property, RBX::Guid::Data id)
{
	Instance* baldReferencer = rbx_static_cast<Instance*>(property.getInstance());

	shared_ptr<Instance> referencer = shared_from<Instance>(baldReferencer);

	if (!referencer.get())
		return;

	const Reflection::RefPropertyDescriptor* desc = 
		static_cast<const Reflection::RefPropertyDescriptor*>(&property.getDescriptor());

	RBXASSERT(*desc != Instance::propParent);

	shared_ptr<Instance> instance;
	if (guidRegistry->lookupByGuid(id, instance))
	{
		desc->setRefValue(referencer.get(), instance.get());
	}
	else
	{
		addPendingRef(desc, referencer, id);
		if (streamingEnabled)
		{
			desc->setRefValue(referencer.get(), NULL);
		}
	}
}

void Replicator::assignParent(Instance* instance, Instance* parent)
{
	// since we decode items on a separate thread, the instance may already be destroyed by the time we reparent it
	if (instance->getIsParentLocked())
	{
		RBXASSERT(instance->getParent() == NULL);
		return;
	}

	Reflection::Property property(Instance::propParent, instance);
	ScopedAssign<const Reflection::Property*> assign(deserializingProperty, &property);
	instance->setParent(parent);
}

void Replicator::assignDefaultPropertyValue(Reflection::Property& property, bool preventBounceBack, Reflection::Variant* var)
{
	ScopedAssign<const Reflection::Property*> assign;
	if (preventBounceBack)
	{
		RBXASSERT(deserializingProperty == NULL);
		assign.assign(deserializingProperty, &property);
	}

	const Reflection::PropertyDescriptor& descriptor = property.getDescriptor();

	Instance* instance = rbx_static_cast<Instance*>(property.getInstance());
	const Instance* defaultInstance = instance ? getDefault(instance->getClassName()) : NULL;
	if (instance && defaultInstance)
	{
		try
		{
			if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(descriptor))
			{
				const Reflection::RefPropertyDescriptor* desc = boost::polymorphic_downcast<const Reflection::RefPropertyDescriptor*>(&descriptor);
				RBX::Instance* refInstance = boost::polymorphic_downcast<Instance*>(desc->getRefValue(defaultInstance));

				Guid::Data data;
				if (refInstance)
					refInstance->getGuid().extract(data);
				*var = data;
			}
			else if (descriptor.type.isEnum)
			{
				const Reflection::EnumPropertyDescriptor& enumPropDesc = static_cast<const Reflection::EnumPropertyDescriptor&>(descriptor);
				enumPropDesc.enumDescriptor.convertToValue(enumPropDesc.getIndexValue(defaultInstance), *var);
			}
			else
			{
				descriptor.getVariant(defaultInstance, *var);
			}
		}
		catch (...)
		{
		}
	}
}

bool Replicator::isCloudEditReplicateProperty(const Reflection::PropertyDescriptor& descriptor)
{
	return descriptor == PartOperation::desc_ChildData ||
		descriptor == Workspace::prop_FilteringEnabled ||
		descriptor == Workspace::prop_StreamingEnabled ||
		descriptor == Workspace::prop_allowThirdPartySales ||
		descriptor == ServerScriptService::desc_loadStringEnabled ||
		descriptor == Players::propCharacterAutoSpawn;
}

}}//namespace
