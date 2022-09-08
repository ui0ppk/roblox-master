/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/ContactStage.h"
#include "V8World/TreeStage.h"
#include "V8World/Primitive.h"
#include "V8World/Contact.h"
#include "V8World/Joint.h"


namespace RBX {

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
ContactStage::ContactStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new TreeStage(this, world), 
					world)
{}
#pragma warning(pop)


TreeStage* ContactStage::getTreeStage()
{
	return rbx_static_cast<TreeStage*>(getDownstreamWS());
}

void ContactStage::onPrimitiveAdded(Primitive* p)
{
	p->putInStage(this);
	getTreeStage()->onPrimitiveAdded(p);			// this should now make some joints ok - their primitives are in the state
}

void ContactStage::onPrimitiveRemoving(Primitive* p)
{
	getTreeStage()->onPrimitiveRemoving(p);
	p->removeFromStage(this);
}


void ContactStage::onEdgeAdded(Edge* e)
{
	RBXASSERT(!e->inKernel());
	RBXASSERT(!e->inOrDownstreamOfStage(this));

	e->putInStage(this);

	if (Contact::isContact(e)) {
		Joint* j  = Primitive::getJoint(e->getPrimitive(0), e->getPrimitive(1));
		if (!j || !j->downstreamOfStage(this)) {
			getDownstreamWS()->onEdgeAdded(e);
		}
	}
	else {
		Joint* j = rbx_static_cast<Joint*>(e);
		if (!Joint::isGroundJoint(j)) {
			Contact* c = Primitive::getContact(e->getPrimitive(0), e->getPrimitive(1));
			if (c && c->downstreamOfStage(this)) {
				getDownstreamWS()->onEdgeRemoving(c);
			}
		}
		getDownstreamWS()->onEdgeAdded(e);
	}
}


void ContactStage::onEdgeRemoving(Edge* e)
{
	if (e->downstreamOfStage(this)) {
		getDownstreamWS()->onEdgeRemoving(e);
	}

	RBXASSERT(e->inStage(this));

	if (Contact::isContact(e)) {
		;
	}
	else {
		Joint* j = rbx_static_cast<Joint*>(e);
		if (!Joint::isGroundJoint(j)) {
			Contact* c = Primitive::getContact(e->getPrimitive(0), e->getPrimitive(1));
			if (c) {

 
//	ToDo - redo the whole duplicate joints issue - this should be checked in the Edge Stage, not in the world;:insertJoint 

//				RBXASSERT(!c->downstreamOfStage(this));
				if (c->inStage(this)) {
					getDownstreamWS()->onEdgeAdded(c);
				}
			}
		}
	}

	e->removeFromStage(this);
	RBXASSERT(!e->inOrDownstreamOfStage(this));
}

} // namespace





