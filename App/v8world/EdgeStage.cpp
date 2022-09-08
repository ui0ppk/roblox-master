/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/EdgeStage.h"
#include "V8World/ContactStage.h"
#include "V8World/Primitive.h"


namespace RBX {

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
EdgeStage::EdgeStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new ContactStage(this, world), 
					world)
{}
#pragma warning(pop)


ContactStage* EdgeStage::getContactStage()
{
	return rbx_static_cast<ContactStage*>(getDownstreamWS());
}

void EdgeStage::onPrimitiveAdded(Primitive* p)
{
	p->putInStage(this);
	getContactStage()->onPrimitiveAdded(p);			// this should now make some joints ok - their primitives are in the state
}

void EdgeStage::onPrimitiveRemoving(Primitive* p)
{
	getContactStage()->onPrimitiveRemoving(p);
	p->removeFromStage(this);
}


void EdgeStage::onEdgeAdded(Edge* e)
{
	Primitive::insertEdge(e);						// 3. Adds edge to the primitive's linked list

	Super::onEdgeAdded(e);
}


void EdgeStage::onEdgeRemoving(Edge* e)
{
	Super::onEdgeRemoving(e);

	Primitive::removeEdge(e);					// remove edge from the primitive's linked list
}

} // namespace





