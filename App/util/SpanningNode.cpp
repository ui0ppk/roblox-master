/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SpanningNode.h"
#include "Util/SpanningEdge.h"


namespace RBX {


// Having a parent edge == in the spanning tree

void SpanningNode::setEdgeToParent(SpanningEdge* edge)
{
	edgeToParent = edge;
}

bool SpanningNode::lessThan(const IndexedTree* other) const
{
	RBXASSERT(edgeToParent);
	const SpanningNode* otherNode = rbx_static_cast<const SpanningNode*>(other);
	RBXASSERT(otherNode->getConstEdgeToParent());
	return edgeToParent->isLighterThan(otherNode->getConstEdgeToParent());
}


} // namespace
