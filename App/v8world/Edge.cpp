/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Edge.h"


namespace RBX {

Edge::Edge(Primitive* prim0, Primitive* prim1) :
	edgeState(Sim::UNDEFINED),
	throttleType(Sim::UNDEFINED_THROTTLE),
	prim0(prim0), 
	prim1(prim1), 
	index0(-1), 
	index1(-1)
{}


void Edge::setPrimitive(int i, Primitive* p)
{
//	RBXASSERT(!this->inPipeline());
	RBXASSERT((i == 0) || (i == 1));
	if (i == 0) {
		prim0 = p;
	}
	else {
		prim1 = p;
	}
}


} // namespace