/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/HumanoidStage.h"
#include "V8World/SleepStage.h"
#include "V8World/SendPhysics.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8World/Joint.h"

namespace RBX {


#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
HumanoidStage::HumanoidStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new SleepStage(this, world), 
					world)
{}
#pragma warning(pop)


HumanoidStage::~HumanoidStage()
{
	RBXASSERT(movingHumanoidAssemblies.size() == 0);
}


void HumanoidStage::toDynamics(Assembly* a)
{
	rbx_static_cast<SleepStage*>(getDownstreamWS())->onAssemblyAdded(a);
}

void HumanoidStage::fromDynamics(Assembly* a)
{
	rbx_static_cast<SleepStage*>(getDownstreamWS())->onAssemblyRemoving(a);
}

void HumanoidStage::toHumanoid(Assembly* a)
{
	RBXASSERT(a->getAssemblyState() == Sim::ANCHORED);
	a->setAssemblyState(Sim::AWAKE);

	getWorld()->getSendPhysics()->onMovingAssemblyRootAdded(a);
	bool ok = movingHumanoidAssemblies.insert(a).second;
	RBXASSERT(ok);
}

void HumanoidStage::fromHumanoid(Assembly* a)
{
	int num = movingHumanoidAssemblies.erase(a);
	RBXASSERT(num);
	getWorld()->getSendPhysics()->onMovingAssemblyRootRemoving(a);

	RBXASSERT(a->getAssemblyState() == Sim::AWAKE);
	a->setAssemblyState(Sim::ANCHORED);
}


void HumanoidStage::onAssemblyAdded(Assembly* a)
{
	a->putInStage(this);	
//	a->computeMaxRadius();		// force a compute so Physics Service can use "getLastComputedValue" without an assert because not computed

	if (a->getAssemblyPrimitive()->getEngineType() == Primitive::DYNAMICS_ENGINE)
	{
		toDynamics(a);
	}
	else
	{
		toHumanoid(a);
	}
}


void HumanoidStage::onAssemblyRemoving(Assembly* a)
{
	if (a->inStage(this))
	{
		fromHumanoid(a);
	}
	else
	{
		fromDynamics(a);
	}
	a->removeFromStage(this);
}

} // namespace