#pragma  once

#include "V8Tree/Service.h"
#include "rbx/signal.h"
#include "Util/StreamRegion.h"

#include "voxel/CellChangeListener.h"
#include "voxel2/GridListener.h"

namespace RakNet {
	class BitStream;
}

namespace RBX {
	
	class MegaClusterInstance;
	
	namespace Network {
		// This class stores the final raknet bit stream of each cluster chunk. 
		// Cache operations are guarded by a shared mutex that uses readers-writer lock,
		// this allows multiple send jobs to fetch from cache concurrently, but only one can update at any given time
		extern const char* const sClusterPacketCacheBase;

		template <class Key>
		class ClusterPacketCacheBase
			: public Voxel::CellChangeListener
            , public Voxel2::GridListener
		{
        protected:
			struct CachedBitStream
			{
				bool dirty;
				boost::shared_ptr<RakNet::BitStream> bitStream;

				CachedBitStream() : dirty(true) {}
			};

			typedef boost::unordered_map<Key, CachedBitStream> StreamCacheList;
			StreamCacheList streamCache;

			boost::shared_mutex sharedMutex;
			boost::shared_ptr<MegaClusterInstance> clusterInstance;

		public:
			ClusterPacketCacheBase();
			virtual ~ClusterPacketCacheBase() {};

			// Search for cached bitstream by index. This function uses a shared locked to allow multiple reads at same time.
			// Returns true if fetched successfully. 
			bool fetchIfUpToDate(const Key &regionId, RakNet::BitStream& outBitStream);

			// Copy all data from bitStream to cache. This function upgrades the shard mutex into an unique lock that blocks all other
			// read and write operations.
			bool update(const Key &regionId, RakNet::BitStream& bitStream, unsigned int numBits);

			unsigned int getCachedBitStreamBytesUsed(const Key &regionId);

			void setupListener(MegaClusterInstance* megaClusterInstance);

		protected:
			void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		};

		extern const char* const sClusterPacketCache;
		class ClusterPacketCache
			: public ClusterPacketCacheBase<SpatialRegion::Id>
			, public DescribedNonCreatable<ClusterPacketCache, Instance, sClusterPacketCache>
			, public Service
		{
		public:
			ClusterPacketCache() 
			{
				setName(sClusterPacketCache);
			};
			~ClusterPacketCache() {};

            void terrainCellChanged(const Voxel::CellChangeInfo& cell) override;
            void onTerrainRegionChanged(const Voxel2::Region& region) override;
		};

		extern const char* const sOneQuarterClusterPacketCache;
		class OneQuarterClusterPacketCache 
			: public ClusterPacketCacheBase<StreamRegion::Id>
			, public DescribedNonCreatable<OneQuarterClusterPacketCache, Instance, sOneQuarterClusterPacketCache>
			, public Service
		{
		public:
			OneQuarterClusterPacketCache() 
			{
				setName(sOneQuarterClusterPacketCache);
			};
			~OneQuarterClusterPacketCache() {};

            void terrainCellChanged(const Voxel::CellChangeInfo& cell) override;
            void onTerrainRegionChanged(const Voxel2::Region& region) override;
		};
}}

