/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/IndexedMesh.h"


namespace RBX {


IndexedMesh::IndexedMesh() 
: lower(NULL)
, upper(NULL)
, computedUpper(NULL)
{}



IndexedMesh::~IndexedMesh() 
{
	RBXASSERT(!lower);
	RBXASSERT(!upper);
	RBXASSERT(!computedUpper);
	RBXASSERT(!getTypedParent<IndexedMesh>());
	RBXASSERT(numChildren() == 0);
}


// definition, the upper goes out of the system if it has no lower;
//
void IndexedMesh::setComputedUpper(IndexedMesh* newComputedUpper)
{
	if (newComputedUpper != computedUpper)
	{
		computedUpper = newComputedUpper;
		for (int i = 0; i < this->numChildren(); ++i) {
			IndexedMesh* child = this->getTypedChild<IndexedMesh>(i);
			if (!child->getUpper()) {
				child->setComputedUpper(newComputedUpper);
			}
		}
	}
}


void IndexedMesh::setUpper(IndexedMesh* newUpper)
{
	if (newUpper != getUpper())
	{
		IndexedMesh* oldUpper = upper;
		upper = NULL;

		if (oldUpper) {
			oldUpper->setLower(NULL);
		}

		upper = newUpper;

		if (upper) {
			upper->setLower(this);
		}

		setComputedUpper(IndexedMesh::computeUpper(this));
	}

	getComputedUpper();
}

// only called by above - setUpper
void IndexedMesh::setLower(IndexedMesh* newLower)
{
	if (newLower != lower)
	{
		if (lower)			// pull out of the chain - I have no lower, so am out of commission!
		{
			IndexedMesh* oldParent = getIndexedMeshParent();		// 1.  Pull out of the chain - parent to null
			setIndexedTreeParent(NULL);								// 2.  Sew children to the old parent
			while (numChildren() > 0) {
				IndexedMesh* backChild = getTypedChild<IndexedMesh>(numChildren() - 1);
				RBXASSERT(backChild->getTypedParent<IndexedMesh>() == this);
				backChild->setIndexedTreeParent(oldParent);			// 3.  No need to use Severe Children - we konw them
			}
		}

		lower = newLower;

		if (newLower)
		{
			IndexedMesh* newLowerParent = newLower->getTypedParent<IndexedMesh>();
			if (newLowerParent) {
				IndexedMesh* newUpperParent = computeUpper(newLowerParent);	// 1. Put into the chain - find my new parent from the lowesr
				setIndexedTreeParent(newUpperParent);
			}
			for (int i = 0; i < newLower->numChildren(); ++i) {
				attachChildren(newLower->getTypedChild<IndexedMesh>(i));
			}
		}
	}
}



void IndexedMesh::onParentChanged(IndexedTree* oldParent)
{
	if (oldParent) {
		IndexedMesh* oldMeshParent = rbx_static_cast<IndexedMesh*>(oldParent);
		if (IndexedMesh* oldUpper = oldMeshParent->getComputedUpper()) {
			oldUpper->onLowerChildRemoved(this);
		}
	}

	if (IndexedMesh* newParent = getTypedParent<IndexedMesh>()) {
		if (IndexedMesh* newUpper = newParent->getComputedUpper()) {
			newUpper->onLowerChildAdded(this);
		}
	}

	setComputedUpper(IndexedMesh::computeUpper(this));
}


void IndexedMesh::onLowerChildAdded(IndexedMesh* lowerChild) 
{
	attachChildren(lowerChild);
	lowersChanged();
}

void IndexedMesh::onLowerChildRemoved(IndexedMesh* lowerChild) 
{
	severeChildren(lowerChild);
	lowersChanged();
}


void IndexedMesh::attachChildren(IndexedMesh* lowerChild)
{
	if (lowerChild->getUpper()) {
		lowerChild->getUpper()->setIndexedTreeParent(this);
	}
	else {
		for (int i = 0; i < lowerChild->numChildren(); ++i) {
			attachChildren(lowerChild->getTypedChild<IndexedMesh>(i));
		}
	}
}

void IndexedMesh::severeChildren(IndexedMesh* lowerChild)
{
	if (lowerChild->getUpper()) {
		lowerChild->getUpper()->setIndexedTreeParent(NULL);
	}
	else {
		for (int i = 0; i < lowerChild->numChildren(); ++i) {
			severeChildren(lowerChild->getTypedChild<IndexedMesh>(i));
		}
	}
}


const IndexedMesh* IndexedMesh::computeParentFromLower() const
{
	IndexedMesh* lowerParent = lower->getTypedParent<IndexedMesh>();
	while (lowerParent) {
		if (lowerParent->getUpper()) {
			return lowerParent->getUpper();
		}
		lowerParent = lowerParent->getTypedParent<IndexedMesh>();
	}
	return NULL;
}

const IndexedMesh* IndexedMesh::getConstIndexedMeshParent() const
{
	const IndexedMesh* answer = getConstTypedParent<IndexedMesh>();
	RBXASSERT(answer == computeParentFromLower());
	return answer;
}

IndexedMesh* IndexedMesh::getIndexedMeshParent()
{
	return const_cast<IndexedMesh*>(getConstIndexedMeshParent());
}

// Static - use of "upper" and "Lower" ok
const IndexedMesh* IndexedMesh::computeConstUpper(const IndexedMesh* lower)
{
	if (const IndexedMesh* upper = lower->getConstUpper()) {
		return upper;
	}
	else if (const IndexedMesh* lowerParent = lower->getConstTypedParent<IndexedMesh>()) {
		return computeConstUpper(lowerParent);
	}
	else {
		return NULL;
	}
}

IndexedMesh* IndexedMesh::computeUpper(IndexedMesh* lower)
{
	return const_cast<IndexedMesh*>(computeConstUpper(lower));
}

const IndexedMesh* IndexedMesh::getConstComputedUpper() const
{
	RBXASSERT_SLOW(computedUpper == IndexedMesh::computeConstUpper(this));
	return computedUpper;
}

IndexedMesh* IndexedMesh::getComputedUpper()
{
	return const_cast<IndexedMesh*>(getConstComputedUpper());
}



bool IndexedMesh::isUpperRoot(const IndexedMesh* lower) 
{
	bool answer = (		lower 
					&&	lower->getConstUpper()		);
	
	RBXASSERT(answer == (lower && (computeConstUpper(lower)->lower == lower)));

	return answer;
}

} // namespace
