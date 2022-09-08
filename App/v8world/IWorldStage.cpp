/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/IWorldStage.h"
#include "V8World/Contact.h"
#include "V8World/Edge.h"

namespace RBX {

void IWorldStage::onEdgeAdded(Edge* e) 
{
	RBXASSERT(getDownstreamWS());
	e->putInStage(this);
	getDownstreamWS()->onEdgeAdded(e);
}

void IWorldStage::onEdgeRemoving(Edge* e) 
{
	RBXASSERT(getDownstreamWS());
	getDownstreamWS()->onEdgeRemoving(e);
	e->removeFromStage(this);
}

} // namespace
