/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/TreeStage.h"
#include "V8World/MovingStage.h"
#include "V8World/SleepStage.h"
#include "V8World/Primitive.h"
#include "V8World/RigidJoint.h"
#include "V8World/MotorJoint.h"
#include "V8World/Clump.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8Kernel/Body.h"
#include "rbx/Debug.h"

#include <map>

namespace RBX {

///////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
TreeStage::TreeStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new MovingStage(this, world), 
					world)
	, maxTreeDepth(0)
{}
#pragma warning(pop)


TreeStage::~TreeStage()
{
	RBXASSERT(dirtyMechanisms.size() == 0);
	RBXASSERT(downstreamMechanisms.size() == 0);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

bool TreeStage::validateTree(SpanningNode* root)
{
	RBXASSERT_NOT_RELEASE();
#ifdef _DEBUG

	if (!Super::validateTree(root)) {
		return false;
	}
	Primitive* primitive = rbx_static_cast<Primitive*>(root);

	Joint* joint = rbx_static_cast<Joint*>(primitive->getEdgeToParent());
	Primitive* parent = primitive->getTypedParent<Primitive>();

	Clump* parentClump = parent ? parent->getClump() : NULL;
	Clump* childClump = primitive->getClump();
	Assembly* childAssembly = primitive->getAssembly();
	Mechanism* childMechanism = primitive->getMechanism();

	RBXASSERT(childClump && childAssembly && childMechanism);

	bool isRigid = RigidJoint::isRigidJoint(joint);
	bool isMotor = Joint::isMotorJoint(joint);
	bool isSpring = Joint::isSpringJoint(joint);
	bool isGround = Joint::isGroundJoint(joint);
	bool isKinematic = (isRigid || isMotor);
	RBXASSERT(isKinematic || isSpring || isGround);
	RBXASSERT(isKinematic == Joint::isKinematicJoint(joint));

	RBXASSERT(isRigid == (parentClump == childClump));
	RBXASSERT(isRigid == (!Clump::isClumpRootPrimitive(primitive)));
	RBXASSERT(isRigid == (primitive->getTypedUpper<Clump>() == NULL));
	RBXASSERT(isKinematic == (!Assembly::isAssemblyRootPrimitive(primitive)));
	RBXASSERT(isGround == (Mechanism::isMechanismRootPrimitive(primitive)));
	RBXASSERT(isKinematic || (primitive->getBody()->getParent() == NULL));
	RBXASSERT(!isKinematic  || (primitive->getBody()->getParent() == parent->getBody()));

	for (int i = 0; i < primitive->numChildren(); ++i)
	{
		Primitive* child = primitive->getTypedChild<Primitive>(i);
		RBXASSERT(child->getTypedParent<Primitive>() == primitive);
		validateTree(child);
	}
#endif
	return true;
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// walk both sides up the tree to a common node, finding the lightest joint along the way
// Parent side is the opposite side from where we found the lightest
// 
// existingActiveJoint will be the case where a freeJoint is added to an existing anchored primitive,
// or an anchorJoint is added to existing free primitive.  In the first case and possibly the second, there
// will be an active joint between the primitive and ground.  If active, it must be the lightest candidate



///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool chainToGround(Primitive* p)
{
	if (!p) {
		return true;
	}
	else {
		Primitive* root = p->getRoot<Primitive>();
		Joint* joint = rbx_static_cast<Joint*>(root->getEdgeToParent());
		return (joint && Joint::isGroundJoint(joint));
	}
}

void TreeStage::onSpanningEdgeAdding(SpanningEdge* edge, SpanningNode* child)
{
#ifdef _DEBUG
	Primitive* childPrim = rbx_static_cast<Primitive*>(child);
	RBXASSERT(!childPrim->getTypedUpper<Clump>());
	RBXASSERT(!childPrim->getClump());
	RBXASSERT(!childPrim->getAssembly());
	RBXASSERT(!childPrim->getMechanism());
	RBXASSERT(!chainToGround(childPrim));
#endif
	
	Primitive* parentPrim = rbx_static_cast<Primitive*>(edge->otherNode(child));
	RBXASSERT(!parentPrim || parentPrim->getClump());
	RBXASSERT(!parentPrim || parentPrim->getAssembly());
	RBXASSERT(!parentPrim || parentPrim->getMechanism());
	RBXASSERT(!parentPrim || chainToGround(parentPrim));

	if (parentPrim)
	{
		dirtyMechanism(parentPrim->getMechanism());
	}
}


/*
	Ground:		New Mechanism
	Spring:		New Assembly
	Motor:		New Clump
	Rigid:		.....
*/

void TreeStage::onSpanningEdgeAdded(SpanningEdge* edge)
{
	Joint* joint = rbx_static_cast<Joint*>(edge);
	bool isGroundJoint = Joint::isGroundJoint(joint);
	Primitive* parent = rbx_static_cast<Primitive*>(edge->getParentSpanningNode());
	Primitive* child = rbx_static_cast<Primitive*>(edge->getChildSpanningNode());

	RBXASSERT(chainToGround(child));
	RBXASSERT(child->getBody()->getParent() == NULL);
	RBXASSERT(!child->getTypedUpper<Clump>());
	RBXASSERT(isGroundJoint == (parent == NULL));	
	RBXASSERT(isGroundJoint == (child->getTypedParent<Primitive>() == NULL));

	if (parent)
	{
		dirtyMechanism(parent->getMechanism());
	}

	if (RigidJoint::isRigidJoint(joint)) {						// RIGID JOINT - same clump
		RigidJoint* r = rbx_static_cast<RigidJoint*>(joint);
		child->getBody()->setParent(parent ? parent->getBody() : NULL);
		child->getBody()->setMeInParent(r->getChildInParent(parent, child));
	}
	else {
#ifdef _DEBUG
		Clump* parentClump = parent ? parent->getClump() : NULL;
		RBXASSERT(isGroundJoint == (parentClump == NULL));
#endif
		Clump* childClump = new Clump();
		child->setUpper(childClump);
		if (Joint::isMotorJoint(joint)) {					// MOTOR Joint - same assembly (new clump)

#ifndef RBX_PLATFORM_IOS
			RBXASSERT(parent);
#endif

#pragma warning(push)
#pragma warning(disable:6011)	
			child->getBody()->setParent(parent->getBody());
#pragma warning(pop)
			child->getBody()->setMeInParent( joint->resetLink() );
		}
		else {
#ifdef _DEBUG
			Assembly* parentAssembly = parent ? parent->getAssembly() : NULL;
			RBXASSERT(isGroundJoint == (parentAssembly == NULL));
#endif
			Assembly* childAssembly = new Assembly();
			childClump->setUpper(childAssembly);
			if (!isGroundJoint) {								// Spring Joint - same mechanism (new assembly)
				RBXASSERT(Joint::isSpringJoint(joint));		
			}
			else {
				Mechanism* childMechanism = new Mechanism();	// Ground Joint - new mechanism
				childAssembly->setUpper(childMechanism);
				RBXASSERT(childMechanism->getIndexedMeshParent() == NULL);

				dirtyMechanism(childMechanism);
			}
#ifdef _DEBUG
			RBXASSERT(childAssembly->getIndexedMeshParent() == parentAssembly);
#endif
		}
#ifdef _DEBUG
		RBXASSERT(childClump->getIndexedMeshParent() == parentClump);
#endif
	}

	sendClumpChangedMessage(child);

	RBXASSERT(child->getClump());
	RBXASSERT(child->getAssembly());
	RBXASSERT(child->getMechanism());
}

void assertNotInPipeline(Assembly* a)
{
	RBXASSERT(!a->inPipeline());
}

bool noAssembliesInPipeline(Mechanism* m)
{
	Assembly* a = m->getTypedLower<Assembly>();
	a->visitAssemblies(boost::bind(&assertNotInPipeline, _1));
	return true;
}		

void TreeStage::onSpanningEdgeRemoving(SpanningEdge* edge)
{
	Primitive* child = rbx_static_cast<Primitive*>(edge->getChildSpanningNode());
#ifdef _DEBUG
	Primitive* parent = rbx_static_cast<Primitive*>(edge->getParentSpanningNode());
    RBX_UNUSED(parent);
    RBXASSERT_SLOW(chainToGround(parent));
	RBXASSERT_SLOW(chainToGround(child));
	RBXASSERT(child->getClump());
	RBXASSERT(child->getAssembly());
	RBXASSERT(child->getMechanism());
#endif
	dirtyMechanism(child->getMechanism());
}


void TreeStage::onSpanningEdgeRemoved(SpanningEdge* edge, SpanningNode* childNode)
{
	Joint* joint = rbx_static_cast<Joint*>(edge);
	Primitive* childPrim = rbx_static_cast<Primitive*>(childNode);
#ifdef _DEBUG
	Primitive* parentPrim = rbx_static_cast<Primitive*>(edge->otherNode(childNode));
	RBXASSERT_SLOW(chainToGround(parentPrim));
	RBXASSERT_SLOW(!chainToGround(childPrim));
	RBXASSERT(Joint::isKinematicJoint(joint) == (childPrim->getBody()->getParent() != NULL));
#endif

	if (RigidJoint::isRigidJoint(joint)) 
	{
#ifdef _DEBUG
		RBXASSERT(parentPrim->getClump());
#endif
	}
	else if (Joint::isMotorJoint(joint)) 
	{
		destroyClump(childPrim);
	}
	else if (Joint::isSpringJoint(joint)) 
	{
		destroyAssembly(childPrim);
	}
	else if (Joint::isGroundJoint(joint)) 	// anchor or free joint
	{
		RBXASSERT(childPrim->getAssembly() == childPrim->getClump()->getTypedUpper<Assembly>());
		RBXASSERT(!childPrim->getParent());
#ifdef _DEBUG
		RBXASSERT(!parentPrim);
#endif
		destroyMechanism(childPrim);
	}
	else 
	{
		RBXASSERT(0);
	}

	childPrim->getBody()->setParent(NULL);

	sendClumpChangedMessage(childPrim);

	RBXASSERT(!childPrim->getTypedUpper<Clump>());
	RBXASSERT(!childPrim->getClump());
	RBXASSERT(!childPrim->getAssembly());
	RBXASSERT(!childPrim->getMechanism());
}


void TreeStage::sendClumpChangedMessage(Primitive* childPrim)
{
	if (childPrim->getOwner()) {
		childPrim->getOwner()->onClumpChanged();
	}
	else {
		// Ground Primitive
	}

	// only do the clump
	for (int i = 0; i < childPrim->numChildren(); ++i) {
		Primitive* childChild = childPrim->getTypedChild<Primitive>(i);
		Joint* joint = rbx_static_cast<Joint*>(childChild->getEdgeToParent());
		if (RigidJoint::isRigidJoint(joint)) {
			sendClumpChangedMessage(childChild);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void TreeStage::removeFromPipeline(Mechanism* m)
{
	if (m->inPipeline()) {
		if (m->downstreamOfStage(this)) {
			rbx_static_cast<MovingStage*>(getDownstreamWS())->onMechanismRemoving(m);
			int num = downstreamMechanisms.erase(m);
			RBXASSERT(num == 1);
		}
		m->removeFromPipeline(this);
	}
	RBXASSERT(noAssembliesInPipeline(m));
}

void TreeStage::dirtyMechanism(Mechanism* m)
{
	RBXASSERT(m);
	removeFromPipeline(m);
	dirtyMechanisms.insert(m);
}

void TreeStage::destroyClump(Primitive* p)
{
	Clump* c = p->getClump();
	p->setUpper(NULL);
	delete c;
}

void TreeStage::destroyAssembly(Primitive* p)
{
	Assembly* a = p->getAssembly();
	Clump* c = p->getClump();

	c->setUpper(NULL);
	p->setUpper(NULL);

	delete c;
	delete a;
}

void TreeStage::destroyMechanism(Primitive* p)
{
	Mechanism* m = p->getMechanism();
	Assembly* a = p->getAssembly();
	Clump* c = p->getClump();

	removeFromPipeline(m);
	dirtyMechanisms.erase(m);

	a->setUpper(NULL);
	c->setUpper(NULL);
	p->setUpper(NULL);

	delete a;
	delete c;
	delete m;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void TreeStage::cleanMechanism(Mechanism* m)
{
	RBXASSERT(!m->getIndexedMeshParent());

	if (!m->inPipeline()) {
		m->putInPipeline(this);
	}

	RBXASSERT(m->inStage(this));
	rbx_static_cast<MovingStage*>(getDownstreamWS())->onMechanismAdded(m);
	bool ok = downstreamMechanisms.insert(m).second;
	RBXASSERT(ok);
}


void TreeStage::assemble()
{
	if (!isAssembled()) 
	{
		// 1. Assemblies
		std::set<Mechanism*>::iterator aIt;
		for (aIt = dirtyMechanisms.begin(); aIt != dirtyMechanisms.end(); ++aIt) {
			cleanMechanism(*aIt);				// clean upstream dirtyAssemblies
		}
		dirtyMechanisms.clear();
	}

	RBXASSERT(isAssembled());
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void TreeStage::onEdgeAdded(Edge* e)
{
	RBXASSERT(e->getPrimitive(0)->inOrDownstreamOfStage(this));
	RBXASSERT(!e->getPrimitive(1) || e->getPrimitive(1)->inOrDownstreamOfStage(this));

	e->putInStage(this);

	if (Joint::isSpanningTreeJoint(e)) {
		Joint* j = rbx_static_cast<Joint*>(e);
		insertSpanningTreeEdge(j);
		if (!(RigidJoint::isRigidJoint(j) || Joint::isGroundJoint(j))) {
			getDownstreamWS()->onEdgeAdded(e);
		}
	}
	else {
		getDownstreamWS()->onEdgeAdded(e);
	}
}


void TreeStage::onEdgeRemoving(Edge* e)
{
	RBXASSERT(e->getPrimitive(0)->inOrDownstreamOfStage(this));
	RBXASSERT(!e->getPrimitive(1) || e->getPrimitive(1)->inOrDownstreamOfStage(this));

	if (Joint::isSpanningTreeJoint(e)) {
		Joint* j = rbx_static_cast<Joint*>(e);
		if (!(RigidJoint::isRigidJoint(j) || Joint::isGroundJoint(j))) {
			getDownstreamWS()->onEdgeRemoving(e);
		}
		if (j->inSpanningTree()) {
			removeSpanningTreeEdge(j);
		}
	}
	else {
		getDownstreamWS()->onEdgeRemoving(e);
	}

	e->removeFromStage(this);
}


void TreeStage::onPrimitiveAdded(Primitive* p)
{
	RBXASSERT(p->getNumEdges() == 0);
	p->putInStage(this);
}


void TreeStage::onPrimitiveRemoving(Primitive* p)
{
	p->removeFromStage(this);
	RBXASSERT(p->getNumEdges() == 0);
}

int TreeStage::getMetric(IWorldStage::MetricType metricType)
{
	switch (metricType)
	{
	case MAX_TREE_DEPTH:
		{
			return maxTreeDepth;
		}
	default:
		{
			return IWorldStage::getMetric(metricType);
		}
	}
}

} // namespace
