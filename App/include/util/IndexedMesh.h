/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/IndexedTree.h"

/*
	used ad the basis for the Primitive / Clump / Assembly / Mechanism / SimJob data structures

	Up:		Parent
	Down:	Child
	Right:	Upper
	Left:	Lower

	Primitive	<->		Clump		<->		Assembly	<->		Mechanism
		A
		|
	Primitive	
		A				A
		|				|
	Primitive	
		A
		|
	Primitive	<->		Clump					A
		A										|
		|
	Primitive	
		A				A
		|				|
	Primitive	
		A
		|
	Primitive	<->		Clump		<->		Assembly

*/


namespace RBX {

	class IndexedMesh : public IndexedTree
	{
	private:
		IndexedMesh* upper;						// could be null - only exists if direct connection to upper
		IndexedMesh* lower;						// could be null - only exists if direct connection to upper
		IndexedMesh* computedUpper;				// should == computeUpper;

	protected:
		// TODO: Make these protected
		IndexedMesh* getUpper() 							{return upper;}
		IndexedMesh* getLower()								{return lower;}

		const IndexedMesh* getConstUpper() const			{return upper;}
		const IndexedMesh* getConstLower() const			{return lower;}

	private:
		const IndexedMesh* computeParentFromLower() const;

		void setComputedUpper(IndexedMesh* newComputedUpper);
		void setLower(IndexedMesh* newLower);

		void severeChildren(IndexedMesh* lowerChild);
		void attachChildren(IndexedMesh* lowerChild);

		void lowersChanged() {		// cycles up to the parent, calling onLowersChanged();
			onLowersChanged();
			if (getTypedParent<IndexedMesh>()) {
				getTypedParent<IndexedMesh>()->lowersChanged();
			}
			if (getUpper()) {
				getUpper()->lowersChanged();
			}
		}
		static IndexedMesh* computeUpper(IndexedMesh* lower);
		static const IndexedMesh* computeConstUpper(const IndexedMesh* lower);

		void onLowerChildRemoved(IndexedMesh* lowerChild);
		void onLowerChildAdded(IndexedMesh* lowerChild);

		//////////////////////////////////////////////////
		//
		// Indexed Tree
		/*override*/ void onParentChanged(IndexedTree* oldParent);

	protected:
		/*implement*/ virtual void onLowersChanged() {}

	public:
		IndexedMesh();
		IndexedMesh(IndexedMesh* lower, IndexedMesh* parent);

		~IndexedMesh();

		IndexedMesh* getIndexedMeshParent();							// same as getTypedParent, but with bug checking
		const IndexedMesh* getConstIndexedMeshParent() const;			// same as getTypedParent, but with bug checking

		void setUpper(IndexedMesh* newUpper);

		template<class Type>
		Type* getTypedLower() {
			return rbx_static_cast<Type*>(getLower());
		}

		template<class Type>
		const Type* getConstTypedLower() const {
			return rbx_static_cast<const Type*>(getConstLower());
		}

		template<class Type>
		Type* getTypedUpper() {
			return rbx_static_cast<Type*>(getUpper());
		}

		template<class Type>
		const Type* getConstTypedUpper() const {
			return rbx_static_cast<const Type*>(getConstUpper());
		}

		IndexedMesh* getComputedUpper();
		const IndexedMesh* getConstComputedUpper() const;

		static bool isUpperRoot(const IndexedMesh* lower);

		template<class Type, class Func>
		inline void visitMeAndChildrenWhileNoUpper(Func func)				// hack - for iterating all assemblies in a mechanism
		{
			Type* t = rbx_static_cast<Type*>(this);
			func(t);
			for (int i = 0; i < numChildren(); ++i) {
				Type* child = getTypedChild<Type>(i);
				IndexedMesh* childUpper = child->getUpper();
				if (!childUpper)
				{
					child->visitMeAndChildrenWhileNoUpperUppers(func);
				}
			}
		}
	};
} // namespace
