#if 1 // remove this file when fast flag RemoveInterpolationReciever is accepted and removed

#pragma once
#include "PhysicsReceiver.h"
#include "ReplicatorStats.h"
#include "BoostAppend.h"
#include "rbx/RunningAverage.h"
#include "rbx/Nil.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>


using boost::multi_index_container;
using namespace boost::multi_index;

namespace RBX { 

	class ModelInstance;

	namespace Network {

	/// Keeps a history of prior data and interpolates position data by introducing lag
	class InterpolatingPhysicsReceiver : public PhysicsReceiver
	{
		class Nugget
		{
		public:
			struct part_tag {};
			struct lastUpdate_tag {};

			// Index fields
			shared_ptr<PartInstance> part;
			RakNet::Time lastUpdate;

			// Non-indexed fields
			double lag;

			static const int bufferSize = 40;
			struct History
			{
				// TODO: Use boost::circular_buffer
				int last;
				int count;
				MechanismItem data[bufferSize];
				History():last(0),count(0) {}
			};
			shared_ptr<History> history;

			Nugget(const shared_ptr<PartInstance>& part):part(part),lag(0),history(new History()) {}

			void receive(RakNet::BitStream& bitsream, RakNet::Time timeStamp, const ModelInstance* noLagModel, InterpolatingPhysicsReceiver* receiver);
			bool step(RakNet::Time time, InterpolatingPhysicsReceiver* receiver) const;	// returns false if the item should be removed from the list
		};

		typedef multi_index_container<
		  Nugget,
		  indexed_by<
			hashed_unique< 
				tag<Nugget::part_tag>, 
				BOOST_MULTI_INDEX_MEMBER(Nugget,shared_ptr<PartInstance>,part) 
			>,
			ordered_non_unique< 
				tag<Nugget::lastUpdate_tag>, 
				BOOST_MULTI_INDEX_MEMBER(Nugget,RakNet::Time,lastUpdate)
			>
		  >
		> Nuggets;
		Nuggets nuggets;

		class Job;
		shared_ptr<Job> job;
		rbx::signals::scoped_connection serviceProviderConnection;

		class LagStats
		{
		private:
			typedef boost::accumulators::accumulator_set< double, boost::accumulators::stats< boost::accumulators::tag::mean > > AccumulatorSet;
			AccumulatorSet bufferSeekAccumulator;
			AccumulatorSet lagAccumulator;
			static void gccWorkaround()
			{
				// These statements eliminate "defined but not used" warnings
                /*
				boost::accumulators::extract::quantile;
				boost::accumulators::extract::sum;
				boost::accumulators::extract::sum_of_weights;
				boost::accumulators::extract::sum_of_variates;
				boost::accumulators::extract::mean;
				boost::accumulators::extract::mean_of_weights;
				boost::accumulators::extract::tail_mean;
                */
			}
		
			RunningAverage<double> averageBufferSeek;
			RunningAverage<double> averageLag;
		public:
			unsigned int maxBufferSeek;
			LagStats();

			void clearBufferAccumulator();
			void accumulateBufferSeek(unsigned int bufferSeek)
			{
				if (bufferSeek>maxBufferSeek)
					maxBufferSeek = bufferSeek;
				bufferSeekAccumulator(bufferSeek);
			}
			void sampleBufferAccumulator();

			void clearLagAccumulator();
			void accumulateLag(double lag)
			{
				lagAccumulator(lag);
			}
			void sampleLagAccumulator();

			unsigned int getMaxBufferSeek() const { return maxBufferSeek; }
			double getAverageBufferSeek() const { return averageBufferSeek.value(); }
			double getAverageLag() const { return averageLag.value(); }
		};
		LagStats lagStats;

		MechanismItem reusableMechanismItem;
	public:
		RunningAverage<double> outOfOrderMechanisms;
		InterpolatingPhysicsReceiver(Replicator* replicator, bool isServer);
		~InterpolatingPhysicsReceiver();
		/*override*/ void start(shared_ptr<PhysicsReceiver> physicsReceiver);
		void setLerpedPhysics(const MechanismItem& itemBefore, const MechanismItem& itemAfter, float lerpAlpha);
		virtual void receivePacket(RakNet::BitStream& bitsream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* stats);
		void sampleBufferSeek(unsigned int bufferSeek)
		{
			lagStats.accumulateBufferSeek(bufferSeek);
		}
		void sampleLag(double lag)
		{
			lagStats.accumulateLag(lag);
		}
		unsigned int getMaxBufferSeek() const { return lagStats.maxBufferSeek; }
		double getAverageBufferSeek() const { return lagStats.getAverageBufferSeek(); }
		double getAverageLag() const { return lagStats.getAverageLag(); }

		void step(RakNet::Time time);
	private:
		void onAncestryChanged(shared_ptr<InterpolatingPhysicsReceiver> interpolatingPhysicsReceiver);
		void tryToCreateJob(shared_ptr<InterpolatingPhysicsReceiver> interpolatingPhysicsReceiver);
	};

} }

#endif  // remove this file when fast flag RemoveInterpolationReciever is accepted and removed
