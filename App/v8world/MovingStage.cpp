/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MovingStage.h"
#include "V8World/SpatialFilter.h"
#include "V8World/Mechanism.h"
#include "V8World/Assembly.h"
#include "V8World/Primitive.h"
#include "rbx/Debug.h"

namespace RBX {

///////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
MovingStage::MovingStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new SpatialFilter(this, world),
					world)
{}
#pragma warning(pop)


MovingStage::~MovingStage()
{
}

SpatialFilter* MovingStage::getSpatialFilter()
{
	return rbx_static_cast<SpatialFilter*>(getDownstreamWS());
}

void assertNotInPipeline2(Assembly* a)
{
	RBXASSERT(!a->inPipeline());
}

bool noAssembliesInPipeline2(Mechanism* m)
{
	Assembly* a = m->getTypedLower<Assembly>();
	a->visitAssemblies(boost::bind(&assertNotInPipeline2, _1));
	return true;
}	


void MovingStage::onMechanismAdded(Mechanism* m)
{
	RBXASSERT(noAssembliesInPipeline2(m));

	m->putInStage(this);									
	Assembly* root = m->getRootAssembly();

	Time now = Time::nowFast();
	if (root->computeIsGrounded()) {

		getSpatialFilter()->onFixedAssemblyRootAdded(root);

		for (int i = 0; i < root->numChildren(); ++i) {
			Assembly* movingChild = root->getTypedChild<Assembly>(i);
			getSpatialFilter()->onMovingAssemblyRootAdded(movingChild, now);
		}
	}
	else {
		getSpatialFilter()->onMovingAssemblyRootAdded(root, now);
	}
}
	

void MovingStage::onMechanismRemoving(Mechanism* m) 
{
	Assembly* root = m->getRootAssembly();

	getSpatialFilter()->onAssemblyRootRemoving(root);

	for (int i = 0; i < root->numChildren(); ++i) {
		Assembly* child = root->getTypedChild<Assembly>(i);
		getSpatialFilter()->onAssemblyRootRemoving(child);
	}

	m->removeFromStage(this);							

	RBXASSERT(noAssembliesInPipeline2(m));
}




} // namespace
