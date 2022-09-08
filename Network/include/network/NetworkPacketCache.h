#pragma once


#include "V8Tree/Service.h"
#include "v8datamodel/partinstance.h"
#include "rbx/signal.h"
#include <boost/unordered/unordered_map.hpp>

namespace RakNet {
	class BitStream;
}

namespace RBX {
	namespace Network {

		extern const char* const sPhysicsPacketCache;
		class PhysicsPacketCache
			: public DescribedNonCreatable<PhysicsPacketCache, Instance, sPhysicsPacketCache>
			, public Service
		{
			typedef DescribedNonCreatable<PhysicsPacketCache, Instance, sPhysicsPacketCache> Super;
		
		public:
			class CachedBitStream
			{
			public:
				int lastStepId;
				int dataBitStartOffset;
				int totalNumBits;
				boost::shared_ptr<RakNet::BitStream> bitStream;

				CachedBitStream() : lastStepId(0), dataBitStartOffset(0), totalNumBits(0) {}
				~CachedBitStream() {}
			};

		private:
			typedef boost::unordered_map<unsigned char, boost::shared_ptr<CachedBitStream> > InnerMap;
			typedef boost::unordered_map<const Assembly*, InnerMap> StreamCacheMap;
			StreamCacheMap streamCache;
			boost::shared_mutex sharedMutex;

			rbx::signals::scoped_connection addingAssemblyConnection;
			rbx::signals::scoped_connection removedAssemblyConnection;

		public:
			PhysicsPacketCache();
			~PhysicsPacketCache();

			bool fetchIfUpToDate(const Assembly* key, unsigned char index, RakNet::BitStream& outBitStream);

			// copy data from bitStream starting at its read position to numBits
			bool update(const Assembly* key, unsigned char index, RakNet::BitStream& bitStream, unsigned int startReadBitPos, unsigned int numBits);

		protected:
			virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		private:
			void insert(const Assembly* key);
			void insertChildAssembly(const Assembly* assembly);
			void remove(const Assembly* key);
			void removeChildAssembly(const Assembly* assembly);
			void onAddingAssembly(shared_ptr<Instance> assembly);
			void onRemovedAssembly(shared_ptr<Instance> assembly);
			void addPart(PartInstance& part);

			boost::unique_lock<boost::shared_mutex> debugLock(boost::upgrade_lock<boost::shared_mutex>* upgradeLock = NULL);
		};


		extern const char* const sInstancePacketCache;
		class InstancePacketCache
			: public DescribedNonCreatable<InstancePacketCache, Instance, sInstancePacketCache>
			, public Service
		{
			typedef DescribedNonCreatable<InstancePacketCache, Instance, sInstancePacketCache> Super;
			typedef std::list<rbx::signals::connection> ConnectionList;

			class CachedBitStream
			{
			public:
				bool dirty;
				
				// regular new instance data containing non dictionary properties, and join time data containing all properties (except parent)
				boost::shared_ptr<RakNet::BitStream> bitStream[2]; 

				// debug
				const std::string guidString;

				rbx::signals::scoped_connection propChangedConnection;
				rbx::signals::scoped_connection ancestorChangedConnection;

				CachedBitStream(const std::string& guid) : dirty(true), guidString(guid) {}
				~CachedBitStream() {}

				void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc) { dirty = true; }
			};

			typedef boost::unordered_map<const Instance*, boost::shared_ptr<CachedBitStream> > StreamCacheMap;
			StreamCacheMap streamCache;
			boost::shared_mutex sharedMutex;

			ConnectionList connections;

			void onAncestorChanged(shared_ptr<Instance> instance, shared_ptr<Instance> newParent);

		public:
			InstancePacketCache();
			~InstancePacketCache();
			
			// not thread safe
			void insert(const Instance* key);

			// not thread safe
			void remove(const Instance* key);

			bool fetchIfUpToDate(const Instance* key, RakNet::BitStream& outBitStream, bool isJoinData);

			// copy data from bitStream starting at its read position to numBits
			bool update(const Instance* key, RakNet::BitStream& bitStream, unsigned int numBits, bool isJoinData);

		protected:
			virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);			
		};
	}
}

