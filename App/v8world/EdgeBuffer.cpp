/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/EdgeBuffer.h"
#include "V8World/MovingAssemblyStage.h"
#include "V8World/Edge.h"
#include "V8World/Joint.h"
#include "V8World/Contact.h"
#include "V8World/Primitive.h"
#include "V8world/World.h"
#include "V8world/ContactManager.h"
#include "V8world/ContactManagerSpatialHash.h"
#include "V8Kernel/Debug.h"

namespace RBX {

EdgeBuffer::~EdgeBuffer()
{
	RBXASSERT(assemblyEdges.empty());
}



void EdgeBuffer::afterAssemblyAdded(Assembly* a)
{
	RBXASSERT_SLOW(debugAddAssembly(a));

	a->visitPrimitives(boost::bind(&EdgeBuffer::assemblyPrimitiveAdded, this, _1));
}

void EdgeBuffer::beforeAssemblyRemoving(Assembly* a)
{
	a->visitPrimitives(boost::bind(&EdgeBuffer::assemblyPrimitiveRemoved, this, _1));

	RBXASSERT_SLOW(debugRemoveAssembly(a));
}


void EdgeBuffer::assemblyPrimitiveAdded(Primitive* p)
{
	if (p->getWorld() && p != p->getWorld()->getGroundPrimitive()) {
		p->getWorld()->getContactManager()->onPrimitiveAssembled(p);
	}
	for (int i = 0; i < p->getNumContacts(); ++i) {			// this is faster than p->forEachEdge
		pushEdgeIfOk(p->getContact(i));
	}
	for (int i = 0; i < p->getNumJoints(); ++i) {
		pushEdgeIfOk(p->getJoint(i));
	}
}


void EdgeBuffer::assemblyPrimitiveRemoved(Primitive* p)
{
	for (int i = 0; i < p->getNumContacts(); ++i) {			// this is faster than p->forEachEdge
		removeEdgeIfDownstream(p->getContact(i));
	}
	for (int i = 0; i < p->getNumJoints(); ++i) {
		removeEdgeIfDownstream(p->getJoint(i));
	}
}


bool EdgeBuffer::assemblyIsHere(Assembly* a) 
{
	return	(	a
			&&	a->inPipeline()
			&&	a->inOrDownstreamOfStage(this)	);
}



//
//
//	Note - for now, everyone does kinematic joints, both clients and server

void EdgeBuffer::pushEdgeIfOk(Edge* e)
{
	if (e->inStage(this)) {
		RBXASSERT(Joint::isKinematicJoint(e) == Joint::isMotorJoint(e));

		bool ok = Joint::isKinematicJoint(e) ? pushKinematicOk(e) : pushSpringOk(e);		// this is asserting in some places, but needed to allow animation of leg joints after sitting

		if (ok) {
            e->generateDataForMovingAssemblyStage();
			getDownstreamWS()->onEdgeAdded(e);
			RBXASSERT_SLOW(debugPushEdgeToDownstream(e));
		}
	}
}

bool EdgeBuffer::pushSpringOk(Edge* e)
{
	RBXASSERT(e->inStage(this));

	Assembly* a0 = e->getPrimitive(0)->getAssembly();
	Assembly* a1 = e->getPrimitive(1)->getAssembly();

	bool ok = (		(assemblyIsHere(a0) && assemblyIsHere(a1))								// 1.  Both here
			&&		(a0->downstreamOfStage(this) || a1->downstreamOfStage(this))			// 2.  One assembly is downstream
			&&		(a0 != a1)		);														// 3.  Either a kinematic joint (motor), or different assemblies
	return ok;
}


/*
	Kinematic joints should go for two reasons:

	Both Assemblies Grounded?
		* this implies no simulation - everyone does it
		* otherwise, use
	1.	pushSpringOK - i.e. - both assemblies here, at least one downstream (simulating).  Will only happen on the dPhysics owner.
	2.	
*/

bool EdgeBuffer::pushKinematicOk(Edge* e)
{
	RBXASSERT(e->inStage(this));
	RBXASSERT(Joint::isMotorJoint(e));

	Assembly* a0 = e->getPrimitive(0)->getAssembly();

	RBX_ENGINE_ASSERT(a0 == e->getPrimitive(1)->getAssembly());

	bool allClientsStepUi = a0->computeIsGrounded();
	bool thisClientDoDPhysics = (assemblyIsHere(a0) && a0->downstreamOfStage(this));

	return (allClientsStepUi || thisClientDoDPhysics);
}


void EdgeBuffer::removeEdgeIfDownstream(Edge* e)
{
	if (e->downstreamOfStage(this)) {
		getDownstreamWS()->onEdgeRemoving(e);
		RBXASSERT_SLOW(debugRemoveEdgeFromDownstream(e));
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////


void EdgeBuffer::onEdgeAdded(Edge* e)
{
	RBXASSERT(!Joint::isGroundJoint(e));
	RBXASSERT(!Joint::isRigidJoint(e));
	e->putInStage(this);
	pushEdgeIfOk(e);
}



void EdgeBuffer::onEdgeRemoving(Edge* e)
{
	removeEdgeIfDownstream(e);
	e->removeFromStage(this);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EdgeBuffer::debugPushEdgeToDownstream(Edge* e)
{
	RBXASSERT_NOT_RELEASE();

	RBXASSERT(e->downstreamOfStage(this));

	Assembly* a0 = e->getPrimitive(0)->getAssembly();
	Assembly* a1 = e->getPrimitive(1)->getAssembly();
	RBXASSERT(a0 && a1);

	if (a0 != a1) {
		assemblyEdges.insertPair(a0, e);
		assemblyEdges.insertPair(a1, e);
	}
	else {
		RBXASSERT(Joint::isMotorJoint(e));
	}
	return true;
}


bool EdgeBuffer::debugRemoveEdgeFromDownstream(Edge* e)
{
	RBXASSERT_NOT_RELEASE();

	Assembly* a0 = e->getPrimitive(0)->getAssembly();
	Assembly* a1 = e->getPrimitive(1)->getAssembly();
	RBXASSERT(a0 && a1);

	if (a0 != a1) {
		assemblyEdges.removePair(a0, e);
		assemblyEdges.removePair(a1, e);
	}
	else {
		RBXASSERT(Joint::isMotorJoint(e));
	}

	return true;
}


bool EdgeBuffer::debugAddAssembly(Assembly* a)
{
	RBXASSERT_NOT_RELEASE();

	//RBXASSERT(assemblyEdges.emptyLeft(a));
	return true;
}



bool EdgeBuffer::debugRemoveAssembly(Assembly* a)
{
	RBXASSERT_NOT_RELEASE();

	RBXASSERT(assemblyEdges.emptyLeft(a));
	return true;
}



} // namespace
