/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */
#if 1 // remove this file when fast flag RemoveInterpolationReciever is accepted and removed

#include "InterpolatingPhysicsReceiver.h"
#include "Compressor.h"
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "NetworkSettings.h"

#include "Replicator.h"

#include "Network/Player.h"

#include "Util/ObscureValue.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/DataModel.h"

#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"

using namespace RBX;
using namespace RBX::Network;

FASTFLAG(RemoveInterpolationReciever)

class InterpolatingPhysicsReceiver::Job : public ReplicatorJob
{
	weak_ptr<InterpolatingPhysicsReceiver> receiver;
	ObscureValue<double> receiveRate;
public:
	Job(shared_ptr<InterpolatingPhysicsReceiver> receiver)
		:ReplicatorJob("Net InterpolatePhysics", *(receiver->replicator), DataModelJob::PhysicsIn) 
		,receiver(receiver)
		,receiveRate(replicator->settings().getReceiveRate())
	{
		cyclicExecutive = true;
	}

private:
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, receiveRate);
	}
	virtual Error error(const Stats& stats)
	{
		return computeStandardError(stats, receiveRate);
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if(shared_ptr<InterpolatingPhysicsReceiver> safeReceiver = receiver.lock()){
			safeReceiver->step(RakNet::GetTimeMS());
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};

static double lerpValue = 0.05;

InterpolatingPhysicsReceiver::InterpolatingPhysicsReceiver(Replicator* replicator, bool isServer)
: PhysicsReceiver(replicator, isServer) 
, outOfOrderMechanisms(lerpValue)
{
	if(FFlag::RemoveInterpolationReciever)
	{
		RBXASSERT(false); // nobody should be creating this
	}
}

void InterpolatingPhysicsReceiver::start(shared_ptr<PhysicsReceiver> physicsReceiver)
{
	shared_ptr<InterpolatingPhysicsReceiver> interpolatingPhysicsReceiver = boost::dynamic_pointer_cast<InterpolatingPhysicsReceiver>(physicsReceiver);
	if(interpolatingPhysicsReceiver.get() != this) 
		RBXCRASH();

	tryToCreateJob(interpolatingPhysicsReceiver);
	if(!job)
		serviceProviderConnection = replicator->ancestryChangedSignal.connect(boost::bind(&InterpolatingPhysicsReceiver::onAncestryChanged, this, interpolatingPhysicsReceiver));
}

void InterpolatingPhysicsReceiver::tryToCreateJob(shared_ptr<InterpolatingPhysicsReceiver> interpolatingPhysicsReceiver)
{
	RBXASSERT(interpolatingPhysicsReceiver.get() == this);
	RBXASSERT(!job);

	// We can't create the job until we're in the DataModel
	if (!DataModel::get(replicator))
		return;

	job.reset(new Job(interpolatingPhysicsReceiver));
	TaskScheduler::singleton().add(job);
}

void InterpolatingPhysicsReceiver::onAncestryChanged(shared_ptr<InterpolatingPhysicsReceiver> interpolatingPhysicsReceiver)
{
	tryToCreateJob(interpolatingPhysicsReceiver);
	if (job)
		// We've succeeded in creating the job
		serviceProviderConnection.disconnect();
}

InterpolatingPhysicsReceiver::~InterpolatingPhysicsReceiver()
{
	TaskScheduler::singleton().remove(job);
}


void InterpolatingPhysicsReceiver::Nugget::receive(RakNet::BitStream& bitstream, RakNet::Time timeStamp, const ModelInstance* noLagModel, InterpolatingPhysicsReceiver* receiver)
{
	if (history->count>0)
	{
		// Check for out-of-order data
		if (timeStamp < history->data[history->last].networkTime)
		{
			MechanismItem dummy;
            int numNodesInHistory;
			receiver->receiveMechanism(bitstream, part.get(), dummy, timeStamp, numNodesInHistory);
			receiver->outOfOrderMechanisms.sample(1);
			return;
		}
		else
			receiver->outOfOrderMechanisms.sample(0);
	}

	history->last = (history->last + 1 + bufferSize) % bufferSize;
	if (history->count<bufferSize)
		history->count++;	 

	MechanismItem& item = history->data[history->last];
	
    int numNodesInHistory;
	receiver->receiveMechanism(bitstream, part.get(), item, timeStamp, numNodesInHistory);
	item.networkTime = timeStamp;

	// Now compute the lag
	if (noLagModel && part && part->isDescendantOf(noLagModel))
	{
		lag = 0;
	}
	else if (history->count>=2)
	{
		const int index2 = (history->last - 1 + bufferSize) % bufferSize;
		double newLag = (double)(item.networkTime - history->data[index2].networkTime);

		// Add a lag buffer
		const double physicsReceiveDelayFactor = 1.2;
		newLag *= physicsReceiveDelayFactor;

		// Use an asymetric log average that favors large values.
		// Small values will be large in number, so they will win out
		// over time.
#if 1
		lag = (lag*lag + newLag*newLag) / (lag + newLag);
#else
		if (newLag>lag)
			lag = 0.8 * newLag + 0.2 * lag;
		else
			lag = 0.2 * newLag + 0.8 * lag;
#endif
	}
	receiver->sampleLag(lag);
}


void InterpolatingPhysicsReceiver::setLerpedPhysics(const MechanismItem& itemBefore, const MechanismItem& itemAfter, float lerpAlpha)
{
	// For efficiency we don't bother lerping near maxima
	if (lerpAlpha<=0.001)
		setPhysics(itemBefore);
	else if (lerpAlpha>=0.99)
		setPhysics(itemAfter);
	else if (!MechanismItem::consistent(&itemBefore, &itemAfter))
		setPhysics(itemAfter);
	else
	{
		MechanismItem::lerp(&itemBefore, &itemAfter, &reusableMechanismItem, lerpAlpha);
		setPhysics(reusableMechanismItem);
	}
}


bool InterpolatingPhysicsReceiver::Nugget::step(RakNet::Time time, InterpolatingPhysicsReceiver* receiver) const
{
	if (part->getDragging()) {
		return false;
	}

	if (!part->getPartPrimitive()->getAssembly()) {
		return false;
	}

	if (!receiver->okDistributedReceivePart(part)) {
		return false;
	}

	if (history->count==0)
		return true;
	
	if (history->count==1)
	{
		receiver->setPhysics(history->data[history->last]);
		return true;
	}
	
	const double laggedTime = time - lag;

	const MechanismItem* itemAfter = 0;
	int i;
	for (i = 0; i<history->count; ++i)
	{
		const int index = (history->last - i + bufferSize) % bufferSize;
		
		const MechanismItem& item(history->data[index]);

		if (item.networkTime > laggedTime)
		{
			itemAfter = &item;
		}
		else if (itemAfter==0)
		{
			itemAfter = &item;
			break;
		}
		else
		{
			float lerpAlpha = (float)(laggedTime - item.networkTime) / (float)(itemAfter->networkTime - item.networkTime);
			// TODO: Allow for forward-looking lerpAlpha up to a point
			receiver->setLerpedPhysics(item, *itemAfter, lerpAlpha);
			receiver->sampleBufferSeek(i+1);
			return true;
		}
	}

	receiver->sampleBufferSeek(i);
	receiver->setPhysics(*itemAfter);

	return true;
}

void InterpolatingPhysicsReceiver::step(RakNet::Time time)
{
	lagStats.clearBufferAccumulator();
	Nuggets::index<Nugget::part_tag>::type& index = nuggets.get<Nugget::part_tag>();
	Nuggets::index_iterator<Nugget::part_tag>::type iter = index.begin();
	Nuggets::index_iterator<Nugget::part_tag>::type end = index.end();
	while (iter!=end)
	{
		// TODO: Erase if no longer relevant
		if (!(*iter).step(time, this))
			iter = index.erase(iter);
		else
			++iter;
	}
	lagStats.sampleBufferAccumulator();
}

void InterpolatingPhysicsReceiver::receivePacket(RakNet::BitStream& inBitstream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* stats)
{
	lagStats.clearLagAccumulator();
	Nuggets::index<Nugget::part_tag>::type& index = nuggets.get<Nugget::part_tag>();

	const ModelInstance* noLagModel = 0;

	while (true)
	{
		shared_ptr<PartInstance> part;
		
		if (!receiveRootPart(part, inBitstream)) {
			break;						// no instance - done
		}

		Nuggets::index_iterator<Nugget::part_tag>::type it = index.find(part);
		
		if (it!=index.end())
		{
			index.modify(it, boost::bind(&Nugget::receive, _1, boost::ref(inBitstream), timeStamp, noLagModel, this));
		}
		else
		{
			Nugget nugget(part);
			nugget.receive(inBitstream, timeStamp, noLagModel, this);
			if (part)
				index.insert(nugget);
		}
	}
	lagStats.sampleLagAccumulator();
}


InterpolatingPhysicsReceiver::LagStats::LagStats()
:averageBufferSeek(lerpValue)
,averageLag(lerpValue)
,maxBufferSeek(0)
{
}

void InterpolatingPhysicsReceiver::LagStats::clearBufferAccumulator()
{
	bufferSeekAccumulator = AccumulatorSet();
}

void InterpolatingPhysicsReceiver::LagStats::sampleBufferAccumulator()
{
	if (boost::accumulators::count(bufferSeekAccumulator)>0)
		averageBufferSeek.sample(boost::accumulators::mean(bufferSeekAccumulator));
}
void InterpolatingPhysicsReceiver::LagStats::clearLagAccumulator()
{
	lagAccumulator = AccumulatorSet();
}

void InterpolatingPhysicsReceiver::LagStats::sampleLagAccumulator()
{
	if (boost::accumulators::count(lagAccumulator)>0)
		averageLag.sample(boost::accumulators::mean(lagAccumulator));
}

#endif // remove this file when fast flag RemoveInterpolationReciever is accepted and removed