#include "Peer.h"

#include "Replicator.h"
#include "GetTime.h"
#include "RakPeer.h"
#include "ConcurrentRakPeer.h"
#include "rbx/Log.h"
#include "rbx/Debug.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/DataModelJob.h"
#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/DataModel.h"
#include "util/ObscureValue.h"
#include "util/standardout.h"

#include "DataBlockEncryptor.h"

const char* const RBX::Network::sPeer = "NetworkPeer";

namespace RBX {
	namespace Network {

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Peer, void(int)> func_SetOutgoingKBPSLimit(&Peer::setOutgoingKBPSLimit, "SetOutgoingKBPSLimit", "limit", Security::Plugin);
REFLECTION_END();

class ProfiledRakPeer : public RakNet::RakPeer
{
private:
	Peer& peer;	// time spent per step in the Raknet thread	
	typedef RakNet::RakPeer Super;
	
public:
	ProfiledRakPeer(Peer& peer):peer(peer) {}
	virtual bool RunUpdateCycle(RakNet::TimeUS timeNS, RakNet::Time timeMS)
	{
		Timer<Time::Benchmark> timer;
		RakNet::BitStream updateBitStream( MAXIMUM_MTU_SIZE);
		bool result = Super::RunUpdateCycle(timeNS, timeMS, updateBitStream);
		peer.rakDutyCycle.sample(timer.delta());
		return result;
	}

};

static double lerp = 0.05;
unsigned char Peer::aesKey[ 16 ];

Peer::Peer()
	:rakDutyCycle(lerp)
{
	for (int i = 0; i < 16; ++i)
		aesKey[i] = 0xFE ^ 7*i;

	protocolVersion = NETWORK_PROTOCOL_VERSION;
}

Peer::~Peer()
{
}

void Peer::onCreateRakPeer()
{
	RakNet::RakNetGUID guid = rakPeer->rawPeer()->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS);
	unsigned int seed = (unsigned int) ((guid.g >> 32) ^ guid.g);
	rnr.SeedMT(seed);

	rakPeer->rawPeer()->SetOccasionalPing(true);
}

void Peer::encryptDataPart(RakNet::BitStream& bitStream)
{
	DataBlockEncryptor encryptor;
	encryptor.SetKey(Peer::aesKey);

	unsigned int length = (unsigned int) bitStream.GetNumberOfBytesUsed() - 1;

	unsigned int bytesRequired = length + 6;
	bytesRequired = ((bytesRequired + 15) / 16) * 16;

	int deltaBits = 8 * bytesRequired - (bitStream.GetNumberOfBitsAllocated() - 8);
	if (deltaBits > 0)
		bitStream.AddBitsAndReallocate(deltaBits);

	encryptor.Encrypt( ( unsigned char* ) bitStream.GetData() + 1, length, ( unsigned char* ) bitStream.GetData() + 1, &length, &rnr );
	bitStream.SetWriteOffset((1 + length) * 8);
}


void Peer::decryptDataPart(RakNet::BitStream& inBitstream)
{
	DataBlockEncryptor encryptor;
	encryptor.SetKey(Peer::aesKey);

	unsigned int length = (unsigned int) inBitstream.GetNumberOfBytesUsed() - 1;

	bool success = encryptor.Decrypt( ( unsigned char* ) inBitstream.GetData() + 1, length, ( unsigned char* ) inBitstream.GetData() + 1, &length);
	if (!success)
		throw std::runtime_error("Data error");
}

bool Peer::askAddChild(const Instance* instance) const
{
	return Instance::fastDynamicCast<Replicator>(instance)!=NULL;
}

class PeerStatsItem : public Stats::Item
{
	Peer* peer;
	Stats::Item* rakRate;
	Stats::Item* rakActivity;
	Stats::Item* physicsSenders;
	Stats::Item* bufferHealth;

public:
	PeerStatsItem(Peer* peer):peer(peer)
	{
		Stats::Item* item = createChildItem("Packets Thread");	// TODO: Rename this when possible. Unfortunately, Game service requires this to be here
		rakRate = item->createChildItem("Rate");
		rakActivity = item->createChildItem("Activity");
		physicsSenders = item->createChildItem("Physics Senders");	// TODO: More this out of "Packets Thread" when we refactor Game Service
		bufferHealth = item->createChildItem("Send Buffer Health");
	}

	/* override */ void update()
	{
		rakActivity->formatPercent(peer->rakDutyCycle.dutyCycle());
		double rate = peer->rakDutyCycle.rate();
		rakRate->formatValue(rate, "%.2g/s", rate);

		PhysicsService* physicsService = ServiceProvider::find<PhysicsService>(this);
		physicsSenders->formatValue(physicsService ? physicsService->numSenders() : 0);

		double health = peer->rakPeer->GetBufferHealth();
		bufferHealth->formatValue(health, "%.4g", health);
	}
};


class PacketReceiveJob : public DataModelJob
{
	weak_ptr<DataModel> dataModel;
	ObscureValue<double> receiveRate;
public:
	weak_ptr<ConcurrentRakPeer> rakPeer;
	PacketReceiveJob(shared_ptr<ConcurrentRakPeer> rakPeer, DataModel* dataModel)
		:DataModelJob("Net PacketReceive", DataModelJob::DataIn, false, shared_from(dataModel), Time::Interval(0))
		,rakPeer(rakPeer)
		,dataModel(shared_from(dataModel))
		,receiveRate(NetworkSettings::singleton().getReceiveRate())
	{
		cyclicExecutive = true;
		cyclicPriority = CyclicExecutiveJobPriority_Network_ReceiveIncoming;
	}

private:
	Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, receiveRate);
	}

	virtual Job::Error error(const Stats& stats)
	{
		return computeStandardError(stats, receiveRate);
	}


	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if (shared_ptr<DataModel> safeDataModel = dataModel.lock())
		{
			FASTLOG1(FLog::DataModelJobs, "Packet receive start, data model: %p", safeDataModel.get());
			DataModel::scoped_write_request request(safeDataModel.get());

			if(shared_ptr<ConcurrentRakPeer> safeRakPeer = rakPeer.lock())
				while (RakNet::Packet* packet = safeRakPeer->rawPeer()->Receive())
					safeRakPeer->DeallocatePacket(packet);

			FASTLOG1(FLog::DataModelJobs, "Packet receive finish, data model: %p", safeDataModel.get());
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};


void Peer::setOutgoingKBPSLimit(int limit)
{
	if (limit<=0)
		rakPeer->rawPeer()->SetPerConnectionOutgoingBandwidthLimit(0);
	else
		rakPeer->rawPeer()->SetPerConnectionOutgoingBandwidthLimit(1000 * G3D::iClamp(limit, 10, 10000));
}

void Peer::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	RBX::Stats::StatsService* stats = ServiceProvider::find<RBX::Stats::StatsService>(oldProvider);
	if (stats)
	{ 
		shared_ptr<Stats::Item> network = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network"));
		if (network)
			network->setParent(NULL);
	}

	if (receiveJob)
	{
		receiveJob->rakPeer.reset();	// Make sure it doesn't try to run from now on. (The concurrency is such that it isn't running now)
		TaskScheduler::singleton().remove(receiveJob);
		receiveJob.reset();
	}

	// Ensure that all Replicators are removed, because they 
	// point to rakPeer, which is about to be deleted
	this->removeAllChildren();	

	if (rakPeer)
	{
		rakPeer->rawPeer()->DetachPlugin(this);
		rakPeer.reset();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
	if (newProvider)
	{
		rakPeer.reset(new ConcurrentRakPeer(
			new ProfiledRakPeer(*this), 
			boost::polymorphic_downcast<DataModel*>(newProvider)
		));
		rakPeer->rawPeer()->AttachPlugin(this);
		//rakPeer->rawPeer()->SetMTUSize(1400);
		onCreateRakPeer();

		receiveJob = shared_ptr<PacketReceiveJob>(new PacketReceiveJob(rakPeer, boost::polymorphic_downcast<DataModel*>(newProvider)));
		TaskScheduler::singleton().add(receiveJob);
	}

	stats = ServiceProvider::find<RBX::Stats::StatsService>(newProvider);
	if (stats)
	{
		RBXASSERT(!shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network")));
		shared_ptr<Stats::Item> network = Creatable<Instance>::create<PeerStatsItem>(this);
		network->setName("Network");
		network->setParent(stats);
	}
}

}}
