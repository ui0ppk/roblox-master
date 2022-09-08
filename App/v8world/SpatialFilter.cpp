/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SpatialFilter.h"
#include "V8World/World.h"
#include "V8World/SendPhysics.h"
#include "V8World/MechToAssemblyStage.h"
#include "V8World/Primitive.h"
#include "V8World/Mechanism.h"
#include "V8World/Assembly.h"
#include "V8World/DistributedPhysics.h"
#include "Network/NetworkOwner.h"
#include "humanoid/Humanoid.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/PartCookie.h"

FASTFLAG(HumanoidFloorPVUpdateSignal)
DYNAMIC_FASTFLAG(NetworkOwnershipRuleReplicates)

namespace RBX {

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
SpatialFilter::SpatialFilter(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new MechToAssemblyStage(this, world), 
					world)
{}
#pragma warning(pop)


SpatialFilter::~SpatialFilter()
{
	for (int i = 0; i < Assembly::NUM_PHASES; ++i) {
		RBXASSERT(assemblies[i].empty());
	}
}

MechToAssemblyStage* SpatialFilter::getMechToAssemblyStage()
{
	return rbx_static_cast<MechToAssemblyStage*>(this->getDownstreamWS());
}



void SpatialFilter::changePhase(MoveInstructions& mi)
{
	RBXASSERT(mi.to != mi.from);
	RBXASSERT(mi.a->getFilterPhase() == mi.from);

	if (mi.from < Assembly::NUM_PHASES)
	{
		int num = assemblies[mi.from].erase(mi.a);				
		RBXASSERT(num == 1);
	}

	mi.a->setFilterPhase(mi.to);
	moveInto(mi);

	if (mi.to < Assembly::NUM_PHASES)
	{
		bool ok = assemblies[mi.to].insert(mi.a).second;		// 1. Into local dynamic assemblies list
		RBXASSERT(ok);
	}
}




// Sim_SendIfSim, Sim_SendAlwasy, NoSim_SendAlways, NoSim_NoSend, NUM_PHASES, NOT_IN_LISTS} Phase;


void SpatialFilter::moveInto(MoveInstructions& mi)
{
	bool wasSending = sendingPhase(mi.from);
	bool wasSimulating = simulatingPhase(mi.from);
	bool wasNoSim = noSimPhase(mi.from);
	bool wasFixed = (mi.from == Assembly::Fixed);
	bool wasAnimated = animationPhase(mi.from);

	bool willSend = sendingPhase(mi.to);
	bool willSimulate = simulatingPhase(mi.to);
	bool willNoSim = noSimPhase(mi.to);
	bool willFixed = (mi.to == Assembly::Fixed);
	bool willAnimate = animationPhase(mi.to);

	{
		if (wasSending) {
			getWorld()->getSendPhysics()->onMovingAssemblyRootRemoving(mi.a);
		}

		if (wasSimulating) {
			RBXASSERT(mi.a->downstreamOfStage(this));
			getMechToAssemblyStage()->onSimulateAssemblyRootRemoving(mi.a);
		}
		else
		{
			if (wasNoSim) {
				RBXASSERT(mi.a->downstreamOfStage(this));
				getMechToAssemblyStage()->onNoSimulateAssemblyRootRemoving(mi.a);
			}
			else if (wasFixed) {
				RBXASSERT(mi.a->downstreamOfStage(this));
				getMechToAssemblyStage()->onFixedAssemblyRemoving(mi.a);
			}

			if (wasAnimated)
				mi.a->visitPrimitives(boost::bind(&SpatialFilter::removePrimitiveJoints, this, _1));
		}

	}

	{
		if (willSend) {
			getWorld()->getSendPhysics()->onMovingAssemblyRootAdded(mi.a);
		}

		if (willSimulate) {
			getMechToAssemblyStage()->onSimulateAssemblyRootAdded(mi.a);
			RBXASSERT(mi.a->downstreamOfStage(this));
		}
		else
		{
			if (willNoSim) {
				getMechToAssemblyStage()->onNoSimulateAssemblyRootAdded(mi.a);
				RBXASSERT(mi.a->downstreamOfStage(this));
			}
			else if (willFixed) {
				getMechToAssemblyStage()->onFixedAssemblyAdded(mi.a);
				RBXASSERT(mi.a->downstreamOfStage(this));
			}

			if (willAnimate)
				mi.a->visitPrimitives(boost::bind(&SpatialFilter::insertPrimitiveJoints, this, _1));
			
		}

	}
}

void SpatialFilter::insertPrimitiveJoints(Primitive* p)
{
	for (int i = 0; i < p->getNumJoints(); i++)
	{
		getWorld()->addAnimatedJointToMovingAssemblyStage(p->getJoint(i));
	}
}

void SpatialFilter::removePrimitiveJoints(Primitive* p)
{
	for (int i = 0; i < p->getNumJoints(); i++)
	{
		getWorld()->removeAnimatedJointFromMovingAssemblyStage(p->getJoint(i));
	}
}


/*
	If server, then all the NO_SIMULATE assemblies (which are by definition moving)
	must be placed in send physics.

	If not a server, then all the NO_SIMULATE assemblies should not be in the send physics area.
*/


bool SpatialFilter::addressMatch(Assembly* a)
{
	return (a->getAssemblyPrimitive()->getNetworkOwner() == filter.networkAddress);
}

bool SpatialFilter::isNotClientAddress(Assembly* a)
{
	RBXASSERT(filter.networkAddress == Network::NetworkOwner::Server());
	const RBX::SystemAddress owner = a->getAssemblyPrimitive()->getNetworkOwner();
	return !Network::NetworkOwner::isClient(owner);
}

bool SpatialFilter::inClientSimRegion(Assembly* a)
{
	Primitive* p = a->getAssemblyPrimitive();
	if (Humanoid::humanoidFromBodyPart(PartInstance::fromPrimitive(p)))
		return false;
	Vector2 pos2d = a->get2dPosition();

	return filter.region.contains(pos2d, RBX::Network::DistributedPhysics::CLIENT_SLOP());
}

void updateNetworkIsSleeping(Assembly* a, Time wakeupNow)
{
	bool isSleeping = Sim::isSleepingAssemblyState(a->getAssemblyState());
	a->getAssemblyPrimitive()->setNetworkIsSleeping(isSleeping, wakeupNow);
}

Assembly::FilterPhase SpatialFilter::filterAssembly(Assembly* a, bool simulating, Time wakeupNow)
{
	switch (filter.mode)
	{
	case SimSendFilter::Client:		return Assembly::NoSim_SendIfSim;

	case SimSendFilter::Server:		
	case SimSendFilter::EditVisit:	return Assembly::Sim_SendIfSim;

	case SimSendFilter::dPhysClient:
		{
			if (addressMatch(a)) {			// we are the network owner - so we can fiddle with the network sleeping bit
				if (simulating) {
					updateNetworkIsSleeping(a, wakeupNow);
				}
				return Assembly::Sim_SendIfSim;
			}
			else
            {
				if (inClientSimRegion(a) 
				    // check for unassigned part because additional properties can still being streamed in, we don't want to
				    // start simulation until all those properties are here. i.e. in script: model:clone() model:makeJoint(), newly cloned parts
				    // have unassigned owner until it goes through network owner job, we start to receive these parts as soon as they 
				    // are created, we need to wait for all the joints are here before simulating
				    && (a->getAssemblyPrimitive()->getNetworkOwner() != RBX::Network::NetworkOwner::ServerUnassigned())
					&& !(DFFlag::NetworkOwnershipRuleReplicates && a->getAssemblyPrimitive()->getNetworkOwnershipRuleInternal() == NetworkOwnership_Manual))
			    {
				    return Assembly::Sim_BufferZone;
			    }
			    else {
				    return a->isAnimationControlled() ? Assembly::NoSim_SendIfSim_Anim : Assembly::NoSim_SendIfSim;
                }
			}
		}
	case SimSendFilter::dPhysServer:
		{
            if (isNotClientAddress(a)) {	// I'm the owner - I can twiddle this bit
				if (simulating) {
					updateNetworkIsSleeping(a, wakeupNow);
				}
				return Assembly::Sim_SendIfSim;
			}
			else {
				bool sleepingClientSide = a->getAssemblyPrimitive()->getNetworkIsSleeping();
				if (sleepingClientSide) {
					return a->isAnimationControlled() ? Assembly::NoSim_SendIfSim_Anim : Assembly::NoSim_SendIfSim;	// i.e. NoSim NoSend
				}
				else {
					return a->isAnimationControlled() ? Assembly::NoSim_Send_Anim : Assembly::NoSim_Send;		
				}
			}
		}
	default:
		{
			RBXASSERT(0);
			return Assembly::NoSim_SendIfSim;
		}
	}
}


void SpatialFilter::filterAssemblies()
{
	RBXASSERT((filter.mode == SimSendFilter::dPhysClient) || (filter.mode == SimSendFilter::dPhysServer));

	toMove.fastClear();					// buffer of where to move

	Time now = Time::nowFast();

	for (int i = 0; i < Assembly::NUM_PHASES; ++i) {
		AssemblySet& workingSet = assemblies[i];
		for (AssemblySet::iterator it = workingSet.begin(); it != workingSet.end(); ++it) 
		{
			Assembly* a = *it;
			RBXASSERT(a->getFilterPhase() == i);

			Assembly::FilterPhase desiredPhase = filterAssembly(a, true, now);		// i.e. we are simulating
			
			if (desiredPhase != i) {
				toMove.append(MoveInstructions(a, static_cast<Assembly::FilterPhase>(i), desiredPhase));
			}
		}
	}

	for (int i = 0; i < toMove.size(); ++i) {
		changePhase(toMove[i]);
	}
}

void SpatialFilter::moveAll(Assembly::FilterPhase destination)
{
	toMove.fastClear();					// buffer of where to move

	for (int i = 0; i < Assembly::NUM_PHASES; ++i) {
		if (i != destination) {
			AssemblySet& workingSet = assemblies[i];
			for (AssemblySet::iterator it = workingSet.begin(); it != workingSet.end(); ++it) 
			{
				Assembly* a = *it;
				toMove.append(MoveInstructions(a, static_cast<Assembly::FilterPhase>(i), destination));
			}
		}
	}

	for (int i = 0; i < toMove.size(); ++i) {
		changePhase(toMove[i]);
	}
}


void SpatialFilter::filterStep()
{
	switch (filter.mode)
	{
		case SimSendFilter::Client:			moveAll(Assembly::NoSim_SendIfSim);		return;

		case SimSendFilter::Server:
		case SimSendFilter::EditVisit:		moveAll(Assembly::Sim_SendIfSim);		return;

		case SimSendFilter::dPhysClient:
		case SimSendFilter::dPhysServer:	
		default:
            {
				filterAssemblies();
                return;
            }
	}
}


void SpatialFilter::onMovingAssemblyRootAdded(Assembly* a, Time now)
{
	RBXASSERT(Mechanism::isMovingAssemblyRoot(a));
	RBXASSERT(!a->computeIsGrounded());
	a->putInPipeline(this);

	Assembly::FilterPhase desiredPhase = filterAssembly(a, false, now);			// shouldn't matter either way - it will get re-done on the filter call
#ifdef _WIN32
	changePhase(MoveInstructions(a, Assembly::NOT_ASSIGNED, desiredPhase));
#else
	MoveInstructions mi = MoveInstructions(a, Assembly::NOT_ASSIGNED, desiredPhase);
	changePhase(mi);
#endif
	
}

void SpatialFilter::onFixedAssemblyRootAdded(Assembly* a)
{
	RBXASSERT(!Mechanism::isMovingAssemblyRoot(a));
	RBXASSERT(a->computeIsGrounded());
	a->putInPipeline(this);

	// gcc crap on this EL
	//changePhase(MoveInstructions(a, Assembly::NOT_ASSIGNED, Assembly::Fixed));
#ifdef _WIN32
	changePhase(MoveInstructions(a, Assembly::NOT_ASSIGNED, Assembly::Fixed));
#else 
	MoveInstructions mi = MoveInstructions(a, Assembly::NOT_ASSIGNED, Assembly::Fixed);
	changePhase(mi);
#endif
	
}

/*
	This can be getting three kinds of assembly "roots"
	Fixed
	Moving, child of fixed
	Moving, root
*/

void SpatialFilter::onAssemblyRootRemoving(Assembly* a) 
{
	if (a->inPipeline()) {
		MoveInstructions mi(a, a->getFilterPhase(), Assembly::NOT_ASSIGNED);
		changePhase(mi);
		a->removeFromPipeline(this);
	}
	else {
		RBXASSERT(a->getFilterPhase() == Assembly::NOT_ASSIGNED);
	}
}


} // namespace
