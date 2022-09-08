
#include "ConcurrentRakPeer.h"
#include "v8datamodel/datamodel.h"
#include "RakNetStatistics.h"
#include "NetworkSettings.h"
#include "util/SystemAddress.h"

LOGGROUP(NetworkStatsReport)

using namespace RBX;
using namespace RBX::Network;

// Used by ConcurrentRakPeer to synchronize the sending of packets in a thread-safe way
class ConcurrentRakPeer::PacketJob : public RBX::DataModelJob
{
public:
	struct SendData
	{
		boost::shared_ptr<const RakNet::BitStream> bitStream;
		PacketPriority priority;
		PacketReliability reliability;
		char orderingChannel;
		RakNet::SystemAddress systemAddress;
		bool broadcast;
	};
	rbx::timestamped_safe_queue<SendData> sendQueue;
	bool isQueueErrorComputed;

	weak_ptr<RakNet::RakPeerInterface> peer;
	PacketJob(shared_ptr<RakNet::RakPeerInterface> peer, DataModel* dataModel)
		:DataModelJob("Net Peer Send", DataModelJob::RaknetPeer, false, shared_from(dataModel), Time::Interval(0))
		,peer(peer)
		,isQueueErrorComputed(NetworkSettings::singleton().isQueueErrorComputed)
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ReceiveIncoming;
	}

private:
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		return sendQueue.empty() ? Time::Interval::max() : Time::Interval::zero();
	}

	virtual Error error(const Stats& stats)
	{
		Error result;
		if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
		{
			if (sendQueue.size())
			{
				result.error = 1.0f;
			}
			else 
			{
				result.error = 0.0f;
			}
		}
		else if (isQueueErrorComputed)
		{
			double waitTime = sendQueue.head_waittime_sec(stats.timeNow);
			result.error = waitTime * 100.0;
			result.error *= std::min<int>(10, sendQueue.size() + 1) / 2.0;
		}
		else
		{
			double size = sendQueue.size();

			if (size>0)
				result.error = stats.timespanSinceLastStep.seconds() * 100.0 * (double)size;
			else
				result.error = 0;
		}

		return result;
	}

	TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		if(shared_ptr<RakNet::RakPeerInterface> safePeer = peer.lock()){
			SendData data;
			while (sendQueue.pop_if_present(data))
			{
				bool result = safePeer->Send(data.bitStream.get(), data.priority, data.reliability, data.orderingChannel, data.systemAddress, data.broadcast) != 0;
				RBXASSERT(result);
			}
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};


class ConcurrentRakPeer::StatsUpdateJob : public RBX::DataModelJob
{
private:
    struct SystemAddressHasher {
        size_t operator()(const RakNet::SystemAddress& key) const {
            return RakNet::SystemAddress::ToInteger(key);
        }
    };
public:

	boost::mutex mapMutex;
	typedef boost::unordered_map< RakNet::SystemAddress, ConnectionStats, SystemAddressHasher > StatsMap;
	StatsMap statsMap;
    
    typedef boost::unordered_map< RakNet::SystemAddress, boost::function<void(const ConnectionStats&)> > UpdateCallbackMap;
    UpdateCallbackMap updateCallbackMap;

	// 0 to 1
	// 1: good, nothing in buffer, room to grow. 0: bad, buffer is increasing, should probably send less data
	RunningAverage<> bufferHealth;
	int prevBufferSize;

	weak_ptr<RakNet::RakPeerInterface> peer;
	StatsUpdateJob(shared_ptr<RakNet::RakPeerInterface> peer, DataModel* dataModel)
		:DataModelJob("Net Peer Stats", DataModelJob::RaknetPeer, false, shared_from(dataModel), Time::Interval(0))
		,peer(peer)
		,prevBufferSize(0)
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessIncoming;
	}

	void updateStats(StatsMap::value_type& x)
	{
		if(shared_ptr<RakNet::RakPeerInterface> safePeer = peer.lock()){
			updateStats(x, safePeer.get());
		}
	}

private:
	void updateStats(StatsMap::value_type& x, RakNet::RakPeerInterface* peer)
	{
		FASTLOG(FLog::NetworkStatsReport, "updateStatsJob::updateStats running");
		x.second.mtuSize = peer->GetMTUSize(x.first);
		x.second.averagePing = peer->GetAveragePing(x.first);
		x.second.lastPing = peer->GetLastPing(x.first);
		x.second.lowestPing = peer->GetLowestPing(x.first);
		RakNet::RakNetStatistics* stats = peer->GetStatistics(x.first);
		if (stats)
		{
			int ourstats = stats->runningTotal[RakNet::ACTUAL_BYTES_RECEIVED];
			FASTLOG1(FLog::NetworkStatsReport, "updateStatsJob rakStats ACTUAL_BYTES_RECEIVE: %d", ourstats);
			x.second.rakStats = *stats;
			x.second.maxPacketloss = std::max(x.second.maxPacketloss, stats->packetlossLastSecond);	
			x.second.averageBandwidthExceeded.sample(stats->isLimitedByOutgoingBandwidthLimit ? 1.0 : 0.0);
			x.second.averageCongestionControlExceeded.sample(stats->isLimitedByCongestionControl ? 1.0 : 0.0);
			x.second.kiloBytesSentPerSecond.sample(stats->valueOverLastSecond[RakNet::USER_MESSAGE_BYTES_PUSHED] / 1000.0f);
			x.second.kiloBytesReceivedPerSecond.sample(stats->valueOverLastSecond[RakNet::USER_MESSAGE_BYTES_RECEIVED_PROCESSED] / 1000.0f);

			x.second.updateBufferHealth(stats->messageInSendBuffer[IMMEDIATE_PRIORITY]+stats->messageInSendBuffer[HIGH_PRIORITY]+stats->messageInSendBuffer[MEDIUM_PRIORITY]+stats->messageInSendBuffer[LOW_PRIORITY]);
		}
	}
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, 30);
	}

	virtual Error error(const Stats& stats)
	{
		return computeStandardErrorCyclicExecutiveSleeping(stats, 30);
	}

	TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		if(shared_ptr<RakNet::RakPeerInterface> safePeer = peer.lock())
		{
			boost::mutex::scoped_lock lock(mapMutex);

			int bufferSize = 0;
			for (StatsMap::iterator iter = statsMap.begin(); iter != statsMap.end(); iter++)
			{
				updateStats(*iter, safePeer.get());
				RakNet::RakNetStatistics& stats = iter->second.rakStats;
				bufferSize +=  stats.messageInSendBuffer[IMMEDIATE_PRIORITY]+stats.messageInSendBuffer[HIGH_PRIORITY]+stats.messageInSendBuffer[MEDIUM_PRIORITY]+stats.messageInSendBuffer[LOW_PRIORITY];
                
                UpdateCallbackMap::iterator callback = updateCallbackMap.find(iter->first);
                if (callback != updateCallbackMap.end()) {
                    callback->second(iter->second);
                }
			}

			calcBufferHealth(bufferSize);

			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}

	void calcBufferHealth(int bufferSize)
	{
		if (bufferSize > prevBufferSize)
			bufferHealth.sample(0.0);
		else if (bufferSize == 0)
			bufferHealth.sample(1);
		else
			bufferHealth.sample(0.5);

		prevBufferSize = bufferSize;
	}
};

void ConcurrentRakPeer::addStats(RakNet::SystemAddress address, boost::function<void(const ConnectionStats&)> callback)
{
	RBXASSERT(dataModel->write_requested);
	StatsUpdateJob::StatsMap::value_type p(address, ConnectionStats());
	statsUpdateJob->updateStats(p);

	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	statsUpdateJob->statsMap.insert(p);
    statsUpdateJob->updateCallbackMap[address] = callback;

	// We need to call this callback
	// Otherwise we send uninitialized data to Diag sometimes.
	callback(p.second);
}

void ConcurrentRakPeer::removeStats(RakNet::SystemAddress address)
{
	RBXASSERT(dataModel->write_requested);
	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	statsUpdateJob->statsMap.erase(address);
    statsUpdateJob->updateCallbackMap.erase(address);
}

ConcurrentRakPeer::ConcurrentRakPeer(RakNet::RakPeerInterface* peer, RBX::DataModel* dataModel)
	:peer(shared_ptr<RakNet::RakPeerInterface>(peer))
	,dataModel(dataModel)
{
	packetJob.reset(new PacketJob(this->peer, dataModel));
	statsUpdateJob.reset(new StatsUpdateJob(this->peer, dataModel));
	RBX::TaskScheduler::singleton().add(packetJob);
	RBX::TaskScheduler::singleton().add(statsUpdateJob);
}


ConcurrentRakPeer::~ConcurrentRakPeer()
{
	RBX::TaskScheduler::singleton().remove(packetJob);
	RBX::TaskScheduler::singleton().remove(statsUpdateJob);
}

void ConcurrentRakPeer::Send(boost::shared_ptr<const RakNet::BitStream> bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, RakNet::SystemAddress systemAddress, bool broadcast )
{
	PacketJob::SendData data = { bitStream, priority, reliability, orderingChannel, systemAddress, broadcast };
	packetJob->sendQueue.push(data);
	TaskScheduler::singleton().reschedule(packetJob);
}

double ConcurrentRakPeer::GetBufferHealth()
{
	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	return statsUpdateJob->bufferHealth.value();
}

const RakNet::RakNetStatistics *  ConcurrentRakPeer::GetStatistics( const RakNet::SystemAddress systemAddress ) const
{ 
	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	return &statsUpdateJob->statsMap[systemAddress].rakStats;
}
double ConcurrentRakPeer::GetBandwidthExceeded( const RakNet::SystemAddress systemAddress )
{ 
	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	return statsUpdateJob->statsMap[systemAddress].averageBandwidthExceeded.value(); 
}
double ConcurrentRakPeer::GetCongestionControlExceeded( const RakNet::SystemAddress systemAddress )
{ 
	boost::mutex::scoped_lock lock(statsUpdateJob->mapMutex);
	return statsUpdateJob->statsMap[systemAddress].averageCongestionControlExceeded.value(); 
}


RakNet::RakPeerInterface* ConcurrentRakPeer::rawPeer() 
{ 
	RBXASSERT(dataModel->write_requested);
	return peer.get(); 
}
const RakNet::RakPeerInterface* ConcurrentRakPeer::rawPeer() const 
{ 
	RBXASSERT(dataModel->write_requested);
	return peer.get(); 
}

