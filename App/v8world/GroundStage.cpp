/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/GroundStage.h"
#include "V8World/EdgeStage.h"
#include "V8World/Primitive.h"
#include "V8World/RigidJoint.h"
#include "V8World/KernelJoint.h"

namespace RBX {


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
GroundStage::GroundStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new EdgeStage(this, world), 
					world)
{}
#pragma warning(pop)


GroundStage::~GroundStage()
{
}

EdgeStage* GroundStage::getEdgeStage()
{
	return rbx_static_cast<EdgeStage*>(getDownstreamWS());
}


void GroundStage::onPrimitiveAdded(Primitive* p)
{
	RBXASSERT(p->getNumEdges() == 0);

	// Primitive
	p->putInStage(this);
	getEdgeStage()->onPrimitiveAdded(p);

	addGroundJoint(p, p->requestFixed());
}


void GroundStage::onPrimitiveRemoving(Primitive* p)
{
	RBXASSERT(p->getNumEdges() == 1);

	removeGroundJoint(p, p->requestFixed());

	getEdgeStage()->onPrimitiveRemoving(p);
	p->removeFromStage(this);
	RBXASSERT(p->getNumEdges() == 0);
}

void GroundStage::onPrimitiveFixedChanging(Primitive* p)
{
	if ((Joint::findConstJoint(p, Joint::ANCHOR_JOINT))) {
		removeGroundJoint(p, true);						// rebuild all primitives that were groundJointed to this primitive
	}
	else {
		removeGroundJoint(p, false);
	}
}

/*
	If primitive goes from Free -> Fixed
	* Need to check all primitives connected to it by rigid joints - they may become GroundJointed.

	If primitive goes from Fixed -> Free
	* Need to check the primitive - it may be ground jointed now
	* Need to check all primitives connected to it by rigid joints -they may lose groundJoints
*/

void GroundStage::onPrimitiveFixedChanged(Primitive* p)
{
	addGroundJoint(p, p->requestFixed());

	if (!p->requestFixed()) {
		rebuildFreeGround(p);		// check for any rigid joints between me and a grounded primitive
	}
	rebuildOthers(p);
}


void GroundStage::addGroundJoint(Primitive* p, bool grounded)
{
	RBXASSERT(!Joint::findConstJoint(p, Joint::ANCHOR_JOINT));
	RBXASSERT(!Joint::findConstJoint(p, Joint::FREE_JOINT));

	Joint* j;
	if (grounded) { 
		j = new AnchorJoint(p);
	}
	else { 
		 j = new FreeJoint(p);
	}

	// Edge
	j->putInPipeline(this);
	getEdgeStage()->onEdgeAdded(j);
}




void GroundStage::removeGroundJoint(Primitive* p, bool grounded)
{
	Joint::JointType jointType = grounded
									? Joint::ANCHOR_JOINT
									: Joint::FREE_JOINT;

	Joint* j = Joint::getJoint(p, jointType);
	RBXASSERT(j);

	// TODO:  saw a crash here.  Should always be a joint here, but for now play it safe.
	if (j) {
		getEdgeStage()->onEdgeRemoving(j);
		j->removeFromPipeline(this);
		j->setPrimitive(0, NULL);
		j->setPrimitive(1, NULL);
		delete j;
	}
	RBXASSERT(!Joint::findConstJoint(p, Joint::ANCHOR_JOINT));
	RBXASSERT(!Joint::findConstJoint(p, Joint::FREE_JOINT));
}




bool GroundStage::kernelJointHere(Primitive* p)
{
	for (int i = 0; i < p->getNumJoints(); ++i)
	{
		Joint* j = p->getJoint(i);
		if (Joint::isKernelJoint(j))
		{
			return true;
		}
	}
	return false;
}


void GroundStage::onEdgeAdded(Edge* e)
{
	if (Joint::isKernelJoint(e)) {
		onKernelJointAdded(rbx_static_cast<KernelJoint*>(e));
	}

	Super::onEdgeAdded(e);		// put the joint downstream, in the primitive list

	if (Joint::isRigidJoint(e)) {	// do this after - uses the primitive list
		checkForFreeGroundJoint(rbx_static_cast<RigidJoint*>(e));
	}
}


void GroundStage::onEdgeRemoving(Edge* e)
{
	Super::onEdgeRemoving(e);		// remove the joint from downstream

	if (Joint::isRigidJoint(e)) {
		checkForFreeGroundJoint(rbx_static_cast<RigidJoint*>(e));
	}

	if (Joint::isKernelJoint(e)) {
		onKernelJointRemoving(rbx_static_cast<KernelJoint*>(e));
	}
}


void GroundStage::onKernelJointAdded(KernelJoint* k)
{
	RBXASSERT(k->getPrimitive(0));
	RBXASSERT(k->getPrimitive(1));
	RBXASSERT(k->getPrimitive(0)->downstreamOfStage(this));
}


void GroundStage::onKernelJointRemoving(KernelJoint* k)
{
	RBXASSERT(k->getPrimitive(0));
	RBXASSERT(k->getPrimitive(1));
	RBXASSERT(k->getPrimitive(0)->downstreamOfStage(this));
}


void GroundStage::checkForFreeGroundJoint(RigidJoint* r)
{
	Primitive* p0 = r->getPrimitive(0);
	Primitive* p1 = r->getPrimitive(1);
	RBXASSERT(p0->downstreamOfStage(this));
	RBXASSERT(p1->downstreamOfStage(this));

	bool p0request = p0->requestFixed();
	bool p1request = p1->requestFixed();

	if (p0request != p1request)					// one grounded, one not.  This joint will not go downstream
	{
		if (p0request) {						
			rebuildFreeGround(p1);
		}
		else {
			rebuildFreeGround(p0);
		}
	}
}

void GroundStage::rebuildOthers(Primitive* changedP)
{
	RigidJoint* r = changedP->getFirstRigid();
	while (r)
	{
		Primitive* other = r->otherPrimitive(changedP);
		rebuildFreeGround(other);
		r = changedP->getNextRigid(r);
	}
}


void GroundStage::rebuildFreeGround(Primitive* p)
{
	if (!p->requestFixed()) {
		RigidJoint* r = heaviestRigidToGround(p);

		if (r) {
			if (!(Joint::findConstJoint(p, Joint::ANCHOR_JOINT))) {
				removeGroundJoint(p, false);						// set the anchor
				addGroundJoint(p, true);
			}
// For now assume in the correct place
//			Primitive* parent = r->otherPrimitive(p);				// align to heaviest rigid
//			p->setCoordinateFrame(parent->getCoordinateFrame() * r->getChildInParent(parent, p));
		}
		else {
			if (!(Joint::findConstJoint(p, Joint::FREE_JOINT))) {
				removeGroundJoint(p, true);							// undo and make free
				addGroundJoint(p, false);
			}			
		}
	}
	else {
		RBXASSERT(Joint::findConstJoint(p, Joint::ANCHOR_JOINT));
	}
}


RigidJoint* GroundStage::heaviestRigidToGround(Primitive* p)
{
	RBXASSERT(!p->requestFixed());
	RigidJoint* answer = NULL;

	RigidJoint* r = p->getFirstRigid();
	while (r) {
		if (r->otherPrimitive(p)->requestFixed()) {
			if (!answer || answer->isLighterThan(r)) {
				answer = r;
			}
		}
		r = p->getNextRigid(r);
	}
	return answer;
}

} // namespace
