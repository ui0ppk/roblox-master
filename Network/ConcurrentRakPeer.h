#pragma once

#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "MTUSize.h"
#include "boost/noncopyable.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "V8DataModel/DataModelJob.h"
#include "rbx/threadsafe.h"
#include <map>


namespace RBX { 

	class DataModel;

	namespace Network
	{
		struct ConnectionStats
		{
			int mtuSize;
			int averagePing;
			int lastPing;
			int lowestPing;
			float maxPacketloss;

			RunningAverage<> bufferHealth;
			int prevBufferSize;

			RBX::RunningAverage<double> averageBandwidthExceeded;
			RBX::RunningAverage<double> averageCongestionControlExceeded;
			RBX::RunningAverage<double> kiloBytesSentPerSecond;
			RBX::RunningAverage<double> kiloBytesReceivedPerSecond;

			RakNet::RakNetStatistics rakStats;

			ConnectionStats() 
				: mtuSize(MAXIMUM_MTU_SIZE)
				, averagePing(0)
				, lastPing(0)
				, lowestPing(0) 
				, maxPacketloss(0.0f)
				, prevBufferSize(0)
				, averageBandwidthExceeded(0.1)
				, averageCongestionControlExceeded(0.1)
				, bufferHealth(0.1, 1.0)
			{}

			void updateBufferHealth(int bufferSize)
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
        
		/// Guards RakPeerInterface agains concurrent access
		/// Any Job running in DataModelJob::Write can access
		/// the peer interface directly using rawPeer().
		/// All other threads should use the functions provided.
		class ConcurrentRakPeer : boost::noncopyable
		{
			boost::shared_ptr<RakNet::RakPeerInterface> peer;

			class PacketJob;
			boost::shared_ptr<PacketJob> packetJob;
			class StatsUpdateJob;
			boost::shared_ptr<StatsUpdateJob> statsUpdateJob;
			RBX::DataModel* const dataModel;



		public:
			ConcurrentRakPeer(RakNet::RakPeerInterface* peer, RBX::DataModel* dataModel);
			~ConcurrentRakPeer();

			void addStats(RakNet::SystemAddress address, boost::function<void(const ConnectionStats&)> updateCallback);
			void removeStats(RakNet::SystemAddress address);

			// Call raw when you KNOW there are no concurrency issues
			RakNet::RakPeerInterface* rawPeer();
			const RakNet::RakPeerInterface* rawPeer() const;

			bool equals(RakNet::RakPeerInterface* peer) const { return this->peer.get() == peer; }

			void Send(boost::shared_ptr<const RakNet::BitStream> bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, RakNet::SystemAddress systemAddress, bool broadcast );
			
			void DeallocatePacket(RakNet::Packet *packet ) 
			{ 
				// Thread-safe
				peer->DeallocatePacket(packet); 
			}
	
			double GetBufferHealth();
			
			// 0..1
			double GetBandwidthExceeded( const RakNet::SystemAddress systemAddress );
			double GetCongestionControlExceeded( const RakNet::SystemAddress systemAddress );
			const RakNet::RakNetStatistics* GetStatistics( const RakNet::SystemAddress systemAddress ) const;

			
		};
	}
}

