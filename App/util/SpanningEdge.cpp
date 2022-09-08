/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SpanningEdge.h"
#include "Util/SpanningNode.h"

namespace RBX {

	
const SpanningNode* SpanningEdge::getConstParentSpanningNode() const
{
	const SpanningNode* child = getConstChildSpanningNode();
	return otherConstNode(child);
}

const SpanningNode* SpanningEdge::getConstChildSpanningNode() const
{
	for (int i = 0; i < 2; ++i) {
		const SpanningNode* n = getConstNode(i);
		if (n && (n->getConstEdgeToParent() == this)) {
			return n;
		}
	}
	RBXASSERT(0);
	return NULL;
}

SpanningNode* SpanningEdge::getChildSpanningNode()
{
	return const_cast<SpanningNode*>(getConstChildSpanningNode());
}

SpanningNode* SpanningEdge::getParentSpanningNode()
{
	return const_cast<SpanningNode*>(getConstParentSpanningNode());
}


void SpanningEdge::removeFromSpanningTree()
{
	SpanningNode* child = getChildSpanningNode();
	RBXASSERT(child);

	child->setIndexedTreeParent(NULL);
	child->setEdgeToParent(NULL);
}

void SpanningEdge::addToSpanningTree(SpanningNode* newParent)
{
	SpanningNode* child = this->otherNode(newParent);
	RBXASSERT(this->otherNode(child) == newParent);
	RBXASSERT(child);
	RBXASSERT(!child->getParent());
	RBXASSERT(!child->getEdgeToParent());

	child->setEdgeToParent(this);			// do first so sort will work
	child->setIndexedTreeParent(newParent);
}

bool SpanningEdge::inSpanningTree() const
{
	for (int i = 0; i < 2; ++i) {
		const SpanningNode* n = getConstNode(i);
		if (n && (n->getConstEdgeToParent() == this)) {
			return true;
		}
	}
	return false;
}


} // namespace
