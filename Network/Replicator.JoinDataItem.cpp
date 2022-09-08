#include "Replicator.JoinDataItem.h"
#include "Replicator.NewInstanceItem.h"

SYNCHRONIZED_FASTFLAGVARIABLE(NetworkAlignJoinData, true) // 223

namespace RBX {
namespace Network {

DeserializedJoinDataItem::DeserializedJoinDataItem() : numInstances(0)
{
	type = Item::ItemTypeJoinData;
}

void DeserializedJoinDataItem::process(Replicator& replicator)
{
	replicator.readJoinDataItem(this);
}

bool Replicator::JoinDataItem::canUseCache(const Instance* instance)
{
	const PartInstance* part = instance->fastDynamicCast<PartInstance>();
	if (part)
	{
		if (NetworkOwner::isClient(part->getNetworkOwner()))
			return true;
		else if (part->getSleeping())
		{
			// when a part is simulated by the server, physics changes does not trigger prop change signal,
			// therefore causing cache to not mark itself dirty. This check prevent cache usage if part is simulated
			// by the server and awake.
			return true;
		}

		return false;
	}

	// Script instances have a ProtectedString property that switches the replication format based on protocol version
	// We can't cache these instances between different replicators until we can have older clients
	BOOST_STATIC_ASSERT(NETWORK_PROTOCOL_VERSION_MIN < 28); // Remove the if check when min protocol version is 28
	if (instance->isA<BaseScript>() || instance->isA<ModuleScript>())
		return false;

	// can use cache if not a part instance
	return true;
}


bool Replicator::JoinDataItem::writeInstance(const Instance* instance, RakNet::BitStream& bitStream)
{
	// If the class is outdated, we still write the instance, because the property and event serializers will take care of the changes
	// But if the class is removed, we simply bypass it
	if (replicator.isClassRemoved(instance))
	{
		return false;
	}

	DescriptorSender<RBX::Reflection::ClassDescriptor>::IdContainer idContainer = replicator.classDictionary.getId(&instance->getDescriptor());

	// Write GUID
	replicator.serializeIdWithoutDictionary(bitStream, instance);

	if (replicator.settings().printInstances)
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,				// remote player always on right
		"Replication NewInstance::write: %s:%s:%s >> %s", 
		instance->getClassName().c_str(), 
		instance->getGuid().readableString().c_str(),
		instance->getName().c_str(),
		RakNetAddressToString(replicator.remotePlayerId).c_str()
		);

	// Write ClassName, ok to use dictionary here because classDictionary entries are already sent to the client
	replicator.classDictionary.send(bitStream, idContainer.id);

	// Write ownership flag
	bool deleteOnDisconnect = replicator.remoteDeleteOnDisconnect(instance);
	bitStream << deleteOnDisconnect;

	// Write all properties
	replicator.writeProperties(instance, bitStream, PropertyCacheType_All, false/*useDictionary*/);

	// Write the Parent property
	Reflection::ConstProperty property(Instance::propParent, instance);
	replicator.serializePropertyValue(property, bitStream, false/*useDictionary*/);
    
	// This maximizes the compression ratio and improves instance packet cache update performance
	bitStream.AlignWriteToByteBoundary();

	return true;
}


size_t Replicator::JoinDataItem::writeInstances(RakNet::BitStream& bitStream)
{
	// preallocate memory for bitstream using estimated size
	RakNet::BitStream dataBitstream(instances.size() * 256);

	unsigned int numWritten = 0;
	while (!instances.empty() && (0 == maxInstancesToWrite || numWritten < maxInstancesToWrite))
	{
		const Instance* instance = instances.front().get();
		if (!replicator.removeFromPendingNewInstances(instance))
		{
			instances.pop_front();
			continue;
		}
        
		bool instanceWritten;
		if (replicator.instancePacketCache && canUseCache(instance))	// use cache
		{
			unsigned int startBit = dataBitstream.GetWriteOffset();

			// try fetch from cache.
			if(!replicator.instancePacketCache->fetchIfUpToDate(instance, dataBitstream, true))
			{
				// instance dirty or cached bitstream was not set
				instanceWritten = writeInstance(instance, dataBitstream);
				dataBitstream.SetReadOffset(startBit);

				// update the cache
				replicator.instancePacketCache->update(instance, dataBitstream, dataBitstream.GetNumberOfBitsUsed() - startBit, true /*isInitalJoinData*/);
			}
			else
			{
				// fetched from cache
				instanceWritten = true;

				if (replicator.settings().printInstances)
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,				// remote player always on right
					"Replication NewInstance::write from cache: %s:%s:%s >> %s, %d bits", 
					instance->getClassName().c_str(), 
					instance->getGuid().readableString().c_str(),
					instance->getName().c_str(),
					RakNetAddressToString(replicator.remotePlayerId).c_str(),
					dataBitstream.GetWriteOffset() - startBit
					);
			}
		}
		else
        {
			instanceWritten = writeInstance(instance, dataBitstream);
        }

		if (instanceWritten)
		{
			++numWritten;
		}
		instances.pop_front();

		if (sendBytesPerStep > 0)
		{
			// estimate data size after compression
			if (dataBitstream.GetNumberOfBytesUsed() >= (sendBytesPerStep * ESTIMATED_COMPRESSION_RATIO)) 
				break;
		}
		else
		{
			if (instanceWritten)
			{
				break; // 1 instance at a time
			}
		}
	}

	// This makes sure we're compressing to a byte-aligned stream to maximize writing performance
	bitStream.AlignWriteToByteBoundary();

	bitStream << numWritten;

	// compress the data and write to bitstream
	if (numWritten > 0)
		Replicator::compressBitStream(dataBitstream, bitStream, DFInt::JoinDataCompressionLevel);

	instancesWrittenOverLifetime += numWritten;
	FASTLOG2(DFLog::NetworkJoin, "JoinDataItem(0x%p) Finished writing %u instances", this, numWritten);

	return numWritten;
}

void Replicator::JoinDataItem::addInstance(shared_ptr<const Instance> instance)
{
	RBX::mutex::scoped_lock lock(replicator.pendingInstancesMutex);
	replicator.pendingNewInstances.insert(instance.get());

	instances.push_back(instance);

	// create an entry in new instance bitstream cache
	if (replicator.instancePacketCache && !Replicator::isTopContainer(instance.get()))
		replicator.instancePacketCache->insert(instance.get());
}


bool Replicator::JoinDataItem::write(RakNet::BitStream& bitStream) 
{
	RBXASSERT(sendBytesPerStep > 0);

	Timer<Time::Fast> writeTimer;
	++timesWriteCalled;

	writeItemType(bitStream, ItemTypeJoinData); 
	writeInstances(bitStream);

	if (DFInt::JoinDataBonus)
		writeBonus(bitStream, DFInt::JoinDataBonus);

	FASTLOG1F(DFLog::NetworkJoin, "\tTime: %f ms", writeTimer.delta().msec());

	return instances.empty();
}

shared_ptr<DeserializedItem> Replicator::JoinDataItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
	shared_ptr<DeserializedJoinDataItem> deserializedData(new DeserializedJoinDataItem());

	inBitstream.AlignReadToByteBoundary();

	unsigned int count;
	inBitstream >> count;
	
	if (count == 0)
		return shared_ptr<DeserializedJoinDataItem>();

	RakNet::BitStream bitstream;
	Replicator::decompressBitStream(inBitstream, bitstream);

	int numInstances = 0;
	deserializedData->instanceInfos.resize(count);
	for (unsigned int i = 0; i < count; i++)
	{
		if (!NewInstanceItem::read(replicator, bitstream, true, deserializedData->instanceInfos[numInstances]))
			deserializedData->instanceInfos[numInstances].reset();
		else
			numInstances++;

		bitstream.AlignReadToByteBoundary();
	}

	deserializedData->numInstances = numInstances;

	return deserializedData;
}

}}
