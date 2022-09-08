
#include "NetworkPacketCache.h"
#include "FastLog.h"
#include "v8datamodel/PhysicsService.h"
#include "v8world/World.h"
#include "v8world/Assembly.h"

#include "util/RobloxGoogleAnalytics.h"

// raknet
#include "BitStream.h"
#include "NetworkOwner.h"

LOGGROUP(NetworkCache)
DYNAMIC_FASTFLAGVARIABLE(DebugLogStaleInstanceCacheEntry, false)


const char* const RBX::Network::sPhysicsPacketCache = "PhysicsPacketCache";
const char* const RBX::Network::sInstancePacketCache = "InstancePacketCache";

using namespace RBX;
using namespace RBX::Network;

PhysicsPacketCache::PhysicsPacketCache()
{
	setName(sPhysicsPacketCache);

	FASTLOG(FLog::NetworkCache, "PhysicsPacketCache Start");
}

PhysicsPacketCache::~PhysicsPacketCache()
{

}

void PhysicsPacketCache::insert(const Assembly* key)
{
	boost::unique_lock<boost::shared_mutex> lock(sharedMutex);
	
	std::pair<StreamCacheMap::iterator, bool> res = streamCache.insert(std::make_pair(key, InnerMap()));
	RBXASSERT(res.second);

	// create cache entry for child assemblies
	key->visitConstDescendentAssemblies(boost::bind(&PhysicsPacketCache::insertChildAssembly, this, _1));
}

void PhysicsPacketCache::insertChildAssembly(const Assembly* assembly)
{
	RBXASSERT(assembly);

	std::pair<StreamCacheMap::iterator, bool> res = streamCache.insert(std::make_pair(assembly, InnerMap()));
	RBXASSERT(res.second);
}

void PhysicsPacketCache::remove(const Assembly* key)
{
	RBXASSERT(key);

	boost::unique_lock<boost::shared_mutex> lock(sharedMutex);

	// remove child assemblies
	key->visitConstDescendentAssemblies(boost::bind(&PhysicsPacketCache::removeChildAssembly, this, _1));

	streamCache.erase(key);
}

void PhysicsPacketCache::removeChildAssembly(const Assembly* assembly)
{
	RBXASSERT(assembly);

	streamCache.erase(assembly);
}

bool PhysicsPacketCache::fetchIfUpToDate(const Assembly* key, unsigned char index, RakNet::BitStream& outBitStream)
{
	boost::shared_lock<boost::shared_mutex> lock(sharedMutex);
	
	StreamCacheMap::iterator iter = streamCache.find(key);

	if (iter != streamCache.end())
	{	
		InnerMap& innerMap = iter->second;

		// entry has not been created
		InnerMap::iterator innerIter = innerMap.find(index);
		if (innerIter == innerMap.end())
			return false;

		CachedBitStream* cachedItem = innerIter->second.get();

		RBXASSERT(key->getConstAssemblyPrimitive()->getWorld());

		int id = cachedItem->lastStepId;
		if ((id > 0) && (id == key->getConstAssemblyPrimitive()->getWorld()->getWorldStepId()))
		{
			RBXASSERT(cachedItem->bitStream);

			outBitStream.WriteBits(cachedItem->bitStream->GetData(), cachedItem->dataBitStartOffset, (RakNet::BitSize_t)cachedItem->totalNumBits);

			return true;
		}
	}

	return false;
}

bool PhysicsPacketCache::update(const Assembly* key, unsigned char index, RakNet::BitStream& bitStream, unsigned int startReadBitPos, unsigned int numBits)
{
	boost::upgrade_lock<boost::shared_mutex> lock(sharedMutex);

	StreamCacheMap::iterator iter = streamCache.find(key);

	if (iter != streamCache.end())
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

		InnerMap& innerMap = iter->second;
		std::pair<InnerMap::iterator, bool> i = innerMap.insert(std::make_pair(index, boost::shared_ptr<CachedBitStream>()));
		if (i.second)
		{
			// create new entry if index is not found
			i.first->second.reset(new CachedBitStream());
		}
		
		InnerMap::iterator innerIter = i.first;
		RBXASSERT(innerIter != innerMap.end());

		CachedBitStream* cachedItem = innerIter->second.get();

		// calculate how many bytes to write
		int numBytes = ((numBits + 7) >> 3);
		if ((startReadBitPos & 7) != 0)
			numBytes++;

		if (!cachedItem->bitStream)
			cachedItem->bitStream.reset(new RakNet::BitStream(numBytes));

		cachedItem->bitStream->Reset();
		
		cachedItem->totalNumBits = numBits;
		int startByte = ((startReadBitPos + 7) >> 3);

		// start at beginning of the byte if read position is between 2 bytes
		if ((startReadBitPos & 7) != 0)
			startByte--;

		// write byte aligned for speed and record starting bit so later we know where to start reading
		cachedItem->bitStream->Write((const char*) bitStream.GetData()+startByte, numBytes);
		cachedItem->dataBitStartOffset = startReadBitPos - (startByte << 3);

		RBXASSERT(key->getConstAssemblyPrimitive()->getWorld());
		cachedItem->lastStepId = key->getConstAssemblyPrimitive()->getWorld()->getWorldStepId();

		return true;
	}

	return false;
}

void PhysicsPacketCache::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		addingAssemblyConnection.disconnect();
		removedAssemblyConnection.disconnect();
		
		streamCache.clear();
	}

	if (newProvider)
	{
		boost::shared_ptr<PhysicsService> physicsService = shared_from(ServiceProvider::find<PhysicsService>(newProvider));
		RBXASSERT(physicsService);

		// Get all current assemblies
		std::for_each(physicsService->begin(), physicsService->end(), boost::bind(&PhysicsPacketCache::addPart, this, _1));

		// Listen for assembly changes
		addingAssemblyConnection = physicsService->assemblyAddingSignal.connect(boost::bind(&PhysicsPacketCache::onAddingAssembly, this, _1));
		removedAssemblyConnection = physicsService->assemblyRemovedSignal.connect(boost::bind(&PhysicsPacketCache::onRemovedAssembly, this, _1));
	}

	Super::onServiceProvider(oldProvider, newProvider);
}

void PhysicsPacketCache::onAddingAssembly(shared_ptr<Instance> assembly)
{
	shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);

	const Assembly* partAssembly = part->getConstPartPrimitive()->getConstAssembly();

	insert(partAssembly);
}

void PhysicsPacketCache::onRemovedAssembly(shared_ptr<Instance> assembly)
{	
	shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(assembly);
	RBXASSERT(part);
	
	const Assembly* partAssembly = part->getConstPartPrimitive()->getConstAssembly();

	remove(partAssembly);
}

void PhysicsPacketCache::addPart(PartInstance& part)
{
	const Assembly* partAssembly = part.getConstPartPrimitive()->getConstAssembly();
	insert(partAssembly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


InstancePacketCache::InstancePacketCache()
{
	setName(sInstancePacketCache);
}

InstancePacketCache::~InstancePacketCache()
{
}

void InstancePacketCache::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		for (StreamCacheMap::iterator i = streamCache.begin(); i != streamCache.end(); i++)
		{
			i->second->ancestorChangedConnection.disconnect();
			i->second->propChangedConnection.disconnect();
		}

		streamCache.clear();
	}

	Super::onServiceProvider(oldProvider, newProvider);
}

void InstancePacketCache::onAncestorChanged(shared_ptr<Instance> instance, shared_ptr<Instance> newParent)
{
	if (!newParent)
	{
		// remove from cache
		remove(instance.get());

		//StandardOut::singleton()->printf(MESSAGE_WARNING, "Cache Removing %s", instance->getName().c_str());
	}
}

void InstancePacketCache::insert(const Instance* key)
{
	boost::shared_ptr<CachedBitStream> newCacheItem(new CachedBitStream(key->getGuid().readableString()));

	std::pair<StreamCacheMap::iterator, bool> result = streamCache.insert(std::make_pair(key, newCacheItem));

	// new item, add listener for prop change
	if (result.second)
	{
		Instance* instance = const_cast<Instance*>(key);
		result.first->second->propChangedConnection = instance->propertyChangedSignal.connect(boost::bind(&InstancePacketCache::CachedBitStream::onPropertyChanged, result.first->second, _1));
		result.first->second->ancestorChangedConnection = instance->ancestryChangedSignal.connect(boost::bind(&InstancePacketCache::onAncestorChanged, this, _1, _2));
	}
	else
	{
		if (!(key->getGuid().readableString() == result.first->second->guidString))
		{
			if (DFFlag::DebugLogStaleInstanceCacheEntry)
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "InstanceCache stale entry", "insert");

			result.first->second->dirty = true;
		}
	}
}

void InstancePacketCache::remove(const Instance* key)
{
	StreamCacheMap::iterator iter = streamCache.find(key);
	
	if (iter != streamCache.end())
	{
		iter->second->ancestorChangedConnection.disconnect();
		iter->second->propChangedConnection.disconnect();
		streamCache.erase(iter);
	}
}

bool InstancePacketCache::fetchIfUpToDate(const Instance* key, RakNet::BitStream& outBitStream, bool isJoinData)
{
	boost::shared_lock<boost::shared_mutex> lock(sharedMutex);

	StreamCacheMap::iterator iter = streamCache.find(key);

	if (iter != streamCache.end())
	{	
		CachedBitStream* cachedItem = iter->second.get();

		if (!(key->getGuid().readableString() == cachedItem->guidString))
		{
			if (DFFlag::DebugLogStaleInstanceCacheEntry)
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "InstanceCache stale entry", "fetch");

			cachedItem->dirty = true;
		}

		if (!cachedItem->bitStream[(int)isJoinData] || cachedItem->dirty)
			return false;

		outBitStream.WriteBits(cachedItem->bitStream[(int)isJoinData]->GetData(), cachedItem->bitStream[(int)isJoinData]->GetNumberOfBitsUsed(), false);
		return true;
	}

	//StandardOut::singleton()->printf(MESSAGE_INFO, "Instance cache miss, part: %s, parent: %s", 
	//								key->getName().c_str(),
	//								key->getParent() ? key->getParent()->getName().c_str() : ""
	//								);

	return false;
}

bool InstancePacketCache::update(const Instance* key, RakNet::BitStream& bitStream, unsigned int numBits, bool isJoinData)
{
	boost::upgrade_lock<boost::shared_mutex> lock(sharedMutex);

	StreamCacheMap::iterator iter = streamCache.find(key);

	if (iter != streamCache.end())
	{
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

		boost::shared_ptr<CachedBitStream> cachedItem = iter->second;

		if (!cachedItem->bitStream[(int)isJoinData])
			cachedItem->bitStream[(int)isJoinData].reset(new RakNet::BitStream());

		cachedItem->bitStream[(int)isJoinData]->Reset();
		cachedItem->bitStream[(int)isJoinData]->Write(bitStream, numBits);
		cachedItem->dirty = false;

		return true;
	}

	return false;
}

