#pragma once

#include "Replicator.h"
#include "util/standardout.h"
#include "network/NetworkPacketCache.h"
#include "network/NetworkOwner.h"
#include "Util.h"
#include "Compressor.h"
#include "script/Script.h"
#include "script/ModuleScript.h"
#include "Replicator.NewInstanceItem.h"

#include "rbx/rbxTime.h"
#include "util/VarInt.h"

#define ESTIMATED_COMPRESSION_RATIO 5.0f

DYNAMIC_LOGGROUP(NetworkJoin)
DYNAMIC_FASTINT(JoinDataCompressionLevel)

DYNAMIC_FASTINT(JoinDataBonus)

namespace RBX { namespace Network {

class DeserializedJoinDataItem : public DeserializedItem
{
public:
	int numInstances;
	std::vector<DeserializedNewInstanceItem> instanceInfos;

	DeserializedJoinDataItem();
	~DeserializedJoinDataItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::JoinDataItem : public Item
{
	std::list<shared_ptr<const Instance> > instances;
	int sendBytesPerStep;
	unsigned timesWriteCalled;
	size_t instancesWrittenOverLifetime;
	size_t maxInstancesToWrite;

	bool canUseCache(const Instance* instance);

protected:
	bool writeInstance(const Instance* instance, RakNet::BitStream& bitStream);

	size_t writeInstances(RakNet::BitStream& bitStream);

    void writeBonus(RakNet::BitStream& bitStream, unsigned int bytes)
	{
		unsigned char buf[64] = {};

		while (bytes >= sizeof(buf))
		{
			bitStream.WriteAlignedBytes(buf, sizeof(buf));
			bytes -= sizeof(buf);
		}
	}



public:
	JoinDataItem(Replicator* replicator):Item(*replicator), sendBytesPerStep(-1), timesWriteCalled(0), instancesWrittenOverLifetime(0), maxInstancesToWrite(0)
	{
		FASTLOG1(DFLog::NetworkJoin, "Created JoinDataItem(0x%p)", this);
	}
	virtual ~JoinDataItem() {
		FASTLOG3(DFLog::NetworkJoin, "~JoinDataItem(0x%p) handled %u instances over %u invocations",
			this, instancesWrittenOverLifetime, timesWriteCalled);
	}
	void setMaxInstancesToWrite(size_t num) {
		maxInstancesToWrite = num;
	}
	bool empty() const
	{
		return instances.empty();
	}
	size_t size() const {
		return instances.size();
	}
	void setBytesPerStep(int numBytes)
	{
		sendBytesPerStep = numBytes;
	}
	void addInstance(shared_ptr<const Instance> instance);

	/*implement*/ virtual bool write(RakNet::BitStream& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

}}
