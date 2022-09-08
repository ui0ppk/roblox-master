/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SpanningTree.h"
#include "Util/SpanningNode.h"
#include "Util/SpanningEdge.h"
#include "rbx/Debug.h"


/*
	Solves the Primitive/Joint graph as a Minimum Spanning Tree Problem

	Weight of a Joint is a function of Joint Type, Biggest Primitive, and Primitive Guid

	Weight of joint types in order (heaviest first):	Anchor, Rigid, Kinematic, Dynamic, Free

	Free Joint ensures that everything is a complete graph

	Add Joint:	
		Walk both sides to a common root // optimization:  each node stores "Lightest above"
		Common root: at the same depth
		Along the way, the active joint should be included
		Find lightest
		If lightest < new joint, lightest becomes unactive, new becomes active

	Remove Joint: 
		If not active - nothing
		If active, then walk tree downstream and find heaviest unactive joint // optimization: each node stores "Heaviest Below"

		Note - removing a joint essentially severes the tree and everything below the cut joint.  When finding
		the "heaviest" downstream, all joints that connect back to the severed tree must be ignored.

		Activate the heaviest joint
*/

namespace RBX {


SpanningTree::SpanningTree() : size(0)
{}


SpanningTree::~SpanningTree()
{
	RBXASSERT(size == 0);
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// walk both sides up the tree to a common node, finding the lightest joint along the way
// Parent side is the opposite side from where we found the lightest
// 
// existingActiveJoint will be the case where a freeJoint is added to an existing anchored primitive,
// or an anchorJoint is added to existing free primitive.  In the first case and possibly the second, there
// will be an active joint between the primitive and ground.  If active, it must be the lightest candidate

void SpanningTree::insertSpanningTreeEdge(SpanningEdge* insertEdge)
{
	RBXASSERT(!insertEdge->inSpanningTree());

	int lightSide = 0;	
	SpanningEdge* deActivate = NULL;
	SpanningTree::findLightestUpstream(insertEdge, deActivate, lightSide);

	if (!deActivate || SpanningEdge::heavierEdge(insertEdge, deActivate))	
	{
		SpanningNode* insertParent = insertEdge->otherNode(lightSide);
		swapTree(deActivate, insertEdge, insertParent);
	}
}

void SpanningTree::removeSpanningTreeEdge(SpanningEdge* removeEdge)
{
	RBXASSERT(removeEdge->inSpanningTree());

	SpanningNode* newParentNode = NULL;
	SpanningEdge* heaviest = findHeaviestDownstream(removeEdge->getChildSpanningNode(), newParentNode);

	swapTree(removeEdge, heaviest, newParentNode);
}


void SpanningTree::swapTree(SpanningEdge* deactivate, SpanningEdge* activate, SpanningNode* newParent)
{
	if (!activate) {
		RBXASSERT(deactivate);
	}

	if (deactivate) {
		RBXASSERT_IF_VALIDATING(validateTree(deactivate->getChildSpanningNode()->getRoot<SpanningNode>()));
	}

	swap(deactivate, activate, newParent);

	if (activate) {
		RBXASSERT_IF_VALIDATING(validateTree(activate->getChildSpanningNode()->getRoot<SpanningNode>()));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SpanningTree::removeEdge(SpanningEdge* edge)
{
	RBXASSERT(edge->inSpanningTree());

	SpanningNode* child = edge->getChildSpanningNode();
	onSpanningEdgeRemoving(edge);
	edge->removeFromSpanningTree();
	onSpanningEdgeRemoved(edge, child);

	RBXASSERT(!edge->inSpanningTree());
}


void SpanningTree::addEdge(SpanningEdge* edge, SpanningNode* newParent)
{
	RBXASSERT(!edge->inSpanningTree());

	onSpanningEdgeAdding(edge, edge->otherNode(newParent));
	edge->addToSpanningTree(newParent);
	onSpanningEdgeAdded(edge);

	RBXASSERT(edge->inSpanningTree());
}


/* need to climb from the old child, switching polarity along the way*/

void SpanningTree::findAndDeactivateEdges(SpanningNode* child, SpanningEdge* deactivate, G3D::Array<SpanningEdge*>& toActivate)
{
	if (SpanningNode* oldParent = child->getParent()) 
	{
		SpanningEdge* edge = child->getEdgeToParent();
		RBXASSERT(edge);

		RBXASSERT(edge->inSpanningTree());

		if (edge != deactivate)
		{
			toActivate.append(edge);
			removeEdge(edge);

			RBXASSERT(!edge->inSpanningTree());

			findAndDeactivateEdges(oldParent, deactivate, toActivate);
		}
	}
}

// These are being done top down
//
void SpanningTree::activateEdges(SpanningNode* child, const G3D::Array<SpanningEdge*>& toActivate)
{
	SpanningNode* newParent = child;
	for (int i = 0; i < toActivate.size(); ++i)
	{
		SpanningEdge* e = toActivate[i];

		RBXASSERT(!e->inSpanningTree());

		addEdge(e, newParent);

		RBXASSERT(e->inSpanningTree());

		newParent = e->otherNode(newParent);
	}
}


void SpanningTree::swap(SpanningEdge* deactivate, SpanningEdge* activate, SpanningNode* newParent)
{
	if (activate) {
		tempEdges.fastClear();
		SpanningNode* child = activate->otherNode(newParent);
		findAndDeactivateEdges(child, deactivate, tempEdges);		// 1.  Bottom up remove edges that will swap
	}

	if (deactivate) {
		removeEdge(deactivate);										// 2.  Remove the deactivated edge
	}

	if (activate) {
		addEdge(activate, newParent);								// 3.  Add the activated edge
		SpanningNode* child = activate->otherNode(newParent);
		activateEdges(child, tempEdges);							// 4.  Top down add the activated edges
	}
}




///////////////////////////////////////////////////////////////////////////////////////////////////////


SpanningNode* SpanningTree::testEdgeToParent(int testSide, SpanningNode* child, SpanningEdge*& answer, int& lightSide)
{
	if (SpanningEdge* edge = child->getEdgeToParent()) {
		if (!answer || edge->isLighterThan(answer)) {		// i.e. edge is lighter than current answer
			answer = edge;
			lightSide = testSide;
		}
	}
	return child->getParent();
}


void SpanningTree::findLightestUpstream(SpanningNode* n0, SpanningNode* n1, int d0, int d1, SpanningEdge*& answer, int& lightSide)
{
	if (d0 != d1) {
		if (d0 > d1) {
			SpanningNode* n0Parent = testEdgeToParent(0, n0, answer, lightSide);
			findLightestUpstream(n0Parent, n1, d0-1, d1, answer, lightSide);
		}
		else {
			SpanningNode* n1Parent = testEdgeToParent(1, n1, answer, lightSide);
			findLightestUpstream(n0, n1Parent, d0, d1-1, answer, lightSide);
		}
	}
	else {
		if (n0 != n1) {
			SpanningNode* n0Parent = n0 ? testEdgeToParent(0, n0, answer, lightSide) : NULL;
			SpanningNode* n1Parent = n1 ? testEdgeToParent(1, n1, answer, lightSide) : NULL;
			findLightestUpstream(n0Parent, n1Parent, d0-1, d1-1, answer, lightSide);
		}
	}
}

void SpanningTree::findLightestUpstream(SpanningEdge* e, SpanningEdge*& answer, int& lightSide)
{
	SpanningNode* n0 = e->getNode(0);
	SpanningNode* n1 = e->getNode(1);
	int d0 = SpanningNode::getDepth(n0);
	int d1 = SpanningNode::getDepth(n1);

	findLightestUpstream(n0, n1, d0, d1, answer, lightSide);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void SpanningTree::buildDownstreamTree(SpanningNode* root, std::set<SpanningNode*>& tree)
{
	tree.insert(root);
	for (int i = 0; i < root->numChildren(); ++i) {
		buildDownstreamTree(root->getChild(i), tree);
	}
}

// Find heaviest inactive joint downstream from p;
// This joint cannot connect to the severed part of the tree.


class FindHeaviest
{
public:
	std::set<SpanningNode*>& tree;
	SpanningEdge*& heaviest;
	SpanningNode*& newParent;

	FindHeaviest(
		std::set<SpanningNode*>& _tree, 
		SpanningEdge*& _heaviest,
		SpanningNode*& _newParent) : tree(_tree), heaviest(_heaviest), newParent(_newParent)
	{}

	void operator()(SpanningNode* node, SpanningEdge* edge) 
	{
		if (!edge->inSpanningTree()) {
			SpanningNode* other = edge->otherNode(node);
			if (!other || (tree.find(other) == tree.end())) {		// not in the tree
				if (!heaviest || edge->isHeavierThan(heaviest)) {
					heaviest = edge;
					newParent = other;
				}
			}
		}
	}
};



SpanningEdge* SpanningTree::findHeaviestDownstream(SpanningNode* node, SpanningNode*& newParent)
{
	std::set<SpanningNode*> downstreamTree;
	buildDownstreamTree(node, downstreamTree);

	SpanningEdge* heaviest = NULL;
	newParent = NULL;

	std::set<SpanningNode*>::const_iterator it;
	for (it = downstreamTree.begin(); it != downstreamTree.end(); ++it)
	{
		SpanningNode* n = *it;
		n->visitEdges<FindHeaviest>(FindHeaviest(downstreamTree, heaviest, newParent));
	}

	return heaviest;
}



} // namespace
