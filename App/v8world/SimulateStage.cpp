/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SimulateStage.h"
#include "V8Kernel/Kernel.h"
#include "V8World/SendPhysics.h"
#include "V8World/Mechanism.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"

namespace RBX {


#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
SimulateStage::SimulateStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new Kernel(this), 
					world)
{}
#pragma warning(pop)


SimulateStage::~SimulateStage()
{
	RBXASSERT(movingDynamicAssemblies.size() == 0);
	RBXASSERT(realTimeAssemblies.size() == 0);
	RBXASSERT(movingAssemblyRoots.size() == 0);
}


// Receives assemblies from the sleep stage
void SimulateStage::onAssemblyAdded(Assembly* a)
{
	RBXASSERT(!a->getAssemblyPrimitive()->requestFixed());
	RBXASSERT(!a->computeIsGrounded());

	if (a->getCanThrottle())
		movingDynamicAssemblies.push_back(*a);
	else
		realTimeAssemblies.push_back(*a);

	a->putInStage(this);
	a->putInKernel(getKernel());
	getKernel()->insertBody(a->getAssemblyPrimitive()->getBody());

	putFirstMovingRootInSendPhysics(a);

	// remove assembly from moving assembly list, because it'll be taken care of by the kernel
	getWorld()->onAssemblyInSimluationStage(a);
}

void SimulateStage::onAssemblyRemoving(Assembly* a)
{
	removeLastMovingRootFromSendPhysics(a);

	getKernel()->removeBody(a->getAssemblyPrimitive()->getBody());
	a->removeFromKernel();
	a->removeFromStage(this);

	if (a->getCanThrottle())
		movingDynamicAssemblies.erase(movingDynamicAssemblies.iterator_to(*a));
	else
		realTimeAssemblies.erase(realTimeAssemblies.iterator_to(*a));
}


void SimulateStage::putFirstMovingRootInSendPhysics(Assembly* a)
{
	Assembly* movingRoot = Mechanism::getMovingAssemblyRoot(a);
	RBXASSERT(Mechanism::isMovingAssemblyRoot(movingRoot));
	
	AssemblyMap::iterator it = movingAssemblyRoots.find(movingRoot);
	if (it == movingAssemblyRoots.end())
	{
		// put first one in
		movingAssemblyRoots.insert(AssemblyMap::value_type(movingRoot, 1));
		getWorld()->getSendPhysics()->onMovingAssemblyRootAdded(movingRoot);
	}
	else
	{
		// otherwise count
		it->second++;
	}
}




// This can be called after one of the assemblies in the chain has been grounded
// Thus - need to walk the assembly chain to the top and test the root as well as one below the root.
//
void SimulateStage::removeLastMovingRootFromSendPhysics(Assembly* a)
{
	Assembly* root = a->getRoot<Assembly>();
	if (!removeFromSendPhysics(root)) {
		Assembly* oneBelowRoot = a->getOneBelowRoot<Assembly>();
		RBXASSERT(oneBelowRoot);
		if (oneBelowRoot) {
			bool ok = removeFromSendPhysics(oneBelowRoot);
			RBXASSERT(ok);
		}
	}
}

bool SimulateStage::removeFromSendPhysics(Assembly* a)
{
	AssemblyMap::iterator it = movingAssemblyRoots.find(a);
	if (it != movingAssemblyRoots.end()) {
		if (it->second == 1) {
			getWorld()->getSendPhysics()->onMovingAssemblyRootRemoving(a);
			movingAssemblyRoots.erase(it);
		}
		else {
			it->second--;
		}
		return true;
	}
	else {
		return false;
	}
}



bool SimulateStage::validateEdge(Edge* e)
{
#ifdef _DEBUG
	Assembly* a0 = e->getPrimitive(0)->getAssembly();
	Assembly* a1 = e->getPrimitive(1)->getAssembly();
	RBXASSERT(a0 && a1);
#endif
	return true;
}

void SimulateStage::onEdgeAdded(Edge* e)
{
	RBXASSERT(validateEdge(e));
	e->putInStage(this);
	e->putInKernel(getKernel());
}


void SimulateStage::onEdgeRemoving(Edge* e)
{
	RBXASSERT(validateEdge(e));

	e->removeFromKernel();
	e->removeFromStage(this);
}

} // namespace
