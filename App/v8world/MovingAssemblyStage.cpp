/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MovingAssemblyStage.h"
#include "V8World/StepJointsStage.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8World/Contact.h"
#include "V8World/Joint.h"
#include "v8world/Mechanism.h"
#include "v8datamodel/JointInstance.h"

LOGVARIABLE(StepAnimatedJoints, 0)
DYNAMIC_FASTFLAGVARIABLE(StepAnimatedJointsInBufferZone, false)

namespace RBX {

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
MovingAssemblyStage::MovingAssemblyStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new StepJointsStage(this, world), 
					world)
{}
#pragma warning(pop)

MovingAssemblyStage::~MovingAssemblyStage()
{
	RBXASSERT(uiStepJoints.empty());
}

void MovingAssemblyStage::addMovingGroundedAssembly(Assembly* a)
{
	movingGroundedAssemblies.insert(a);
}

void MovingAssemblyStage::removeMovingGroundedAssembly(Assembly* a)
{
	movingGroundedAssemblies.erase(a);
}

void MovingAssemblyStage::addMovingAnimatedAssembly(Assembly* a)
{
	movingAnimatedAssemblies.insert(a);
}

void MovingAssemblyStage::removeMovingAnimatedAssembly(Assembly* a)
{
	movingAnimatedAssemblies.erase(a);
}

void MovingAssemblyStage::addAnimatedJoint(Joint* j)	
{
	if (j->canStepUi() && Joint::isMotorJoint(j))
	{
		const Motor* jointInstance = static_cast<const Motor*>(j->getJointOwner());
		if (jointInstance->getIsAnimatedJoint())
		{
			RBXASSERT(!j->MovingAssemblyStageHook::is_linked());
			animatedJoints.insert(j);
		}
	}
}

void MovingAssemblyStage::removeAnimatedJoint(Joint* j)
{
	if (j->canStepUi()) 
	{
		if (animatedJoints.erase(j) > 0)
		{
			if (Primitive *p = j->getPrimitive(0))
				removeMovingAnimatedAssembly(p->getAssembly());
		}
	}
	else {
		RBXASSERT(!j->MovingAssemblyStageHook::is_linked());
	}
}

void MovingAssemblyStage::addJoint(Joint* j)
{
	if (j->canStepUi()) {
		uiStepJoints.push_back(*j);

		RBXASSERT(animatedJoints.find(j) == animatedJoints.end());
	}
}

void MovingAssemblyStage::removeJoint(Joint* j)
{
	if (j->canStepUi()) {
		uiStepJoints.erase(uiStepJoints.iterator_to(*j));
		removeMovingGroundedAssembly(j->getPrimitive(0)->getAssembly());

		RBXASSERT(animatedJoints.find(j) == animatedJoints.end());
	}
	else {
		RBXASSERT(!j->MovingAssemblyStageHook::is_linked());
	}
}


void MovingAssemblyStage::onSimulateAssemblyAdded(Assembly* a)
{
	a->putInStage(this);							
	rbx_static_cast<StepJointsStage*>(getDownstreamWS())->onSimulateAssemblyAdded(a);
}


void MovingAssemblyStage::onSimulateAssemblyRemoving(Assembly* a) 
{
	rbx_static_cast<StepJointsStage*>(getDownstreamWS())->onSimulateAssemblyRemoving(a);
	a->removeFromStage(this);							// 2.  out of this stage

	removeMovingGroundedAssembly(a);
}


void MovingAssemblyStage::onEdgeAdded(Edge* e)
{
	e->putInStage(this);

	if (Joint::isJoint(e)) {
		Joint* j = rbx_static_cast<Joint*>(e);
		addJoint(j);
	}

	getDownstreamWS()->onEdgeAdded(e);
}

void MovingAssemblyStage::onEdgeRemoving(Edge* e)
{
	if (e->downstreamOfStage(this)) {
		getDownstreamWS()->onEdgeRemoving(e);
	}

	if (Joint::isJoint(e)) {
		Joint* j = rbx_static_cast<Joint*>(e);
		removeJoint(j);
	}

	e->removeFromStage(this);
}

void MovingAssemblyStage::jointsStepUiInternal(double distributedGameTime, Joint* j, bool fromAnimation)
{
	RBXASSERT(j->canStepUi());
	Assembly* a0 = NULL;
	bool a0IsGrounded = false;
	if (Joint::isKinematicJoint(j)) {
		a0 = j->getPrimitive(0)->getAssembly();

		const Assembly* root = Mechanism::getConstMovingAssemblyRoot(a0);
		if (root->getFilterPhase() == Assembly::Sim_BufferZone && (DFFlag::StepAnimatedJointsInBufferZone ? !root->isAnimationControlled() : true))
			// Only the owner steps kinematic joints to avoid simulation jitter
				return;

		if (a0->computeIsGrounded())
			a0IsGrounded = true;
	}

	bool moved = j->stepUi(distributedGameTime);
	if (moved)
	{
		if (fromAnimation)
			addMovingAnimatedAssembly(a0);
		else if (a0IsGrounded)
			addMovingGroundedAssembly(a0);
	}
}

void MovingAssemblyStage::jointsStepUi(double distributedGameTime)
{
	for (Joints::iterator it = uiStepJoints.begin(); it != uiStepJoints.end(); ++it) 
	{
		jointsStepUiInternal(distributedGameTime, &*it, false);
	}

	for (boost::unordered_set<Joint*>::iterator it = animatedJoints.begin(); it != animatedJoints.end(); ++it) 
	{
		FASTLOG3(FLog::StepAnimatedJoints, "jointsStepUi joint: %p, p0: %p, p1: %p", *it, (*it)->getPrimitive(0), (*it)->getPrimitive(1));
		jointsStepUiInternal(distributedGameTime, *it, true);
	}
}


} // namespace
