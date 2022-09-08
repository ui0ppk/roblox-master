/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/IndexedTree.h"


namespace RBX {


IndexedTree::IndexedTree()
: parent(NULL)
, index(-1)
{
}

IndexedTree::~IndexedTree() 
{
	RBXASSERT(children.size() == 0);
	RBXASSERT(!parent);
	RBXASSERT(index == -1);
}



// Make sure newAncestor and all parents do not have child as their parent
bool IndexedTree::circularReference(IndexedTree* newAncestor, IndexedTree* child)
{
	RBXASSERT(child);
	if (!newAncestor) {
		return false;
	}
	else if (newAncestor->parent == child) {
		return true;
	}
	else {
		return circularReference(newAncestor->parent, child);
	}
}


void IndexedTree::setIndexedTreeParent(IndexedTree* newParent)
{
	RBXASSERT(!newParent || (newParent->parent != this));
	RBXASSERT(newParent != this);
	RBXASSERT_VERY_FAST(!circularReference(newParent, this));

	if (parent != newParent) 
	{
		IndexedTree* oldParent = parent;

		onParentChanging();

		if (parent) {
			parent->onChildRemoving(this);
			parent->children.fastRemove(this);
			parent->onChildRemoved(this);				// parent Cofm will be set dirty here - mine shouldn't change!
		}

		parent = newParent;

		if (parent) {
			parent->onChildAdding(this);
			parent->children.fastAppend(this);
			parent->onChildAdded(this);
		}

		onParentChanged(oldParent);
	}
}


} // namespace
