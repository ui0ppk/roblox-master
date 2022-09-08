/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"
#include "Util/IndexedMesh.h"
#include "Primitive.h"
#include <set>

namespace RBX {

	class Edge;
	class Joint;
	class Clump;
	class PrimIterator;

	class Clump 
		: public boost::noncopyable
		, public IndexedMesh
	{
	private:
		template<class Func>
		inline void visitPrimitivesImpl(Func func, Primitive* p) {
			func(p);
			for (int i = 0; i < p->numChildren(); ++i) {
				Primitive* child = p->getTypedChild<Primitive>(i);
				if (!Clump::isClumpRootPrimitive(child)) {
					visitPrimitivesImpl(func, child);
				}
			}
		}

	public:
		Clump();

		~Clump();

		Clump* getRootClump()								{return getRoot<Clump>();}
		const Clump* getRootClump() const				{return getRoot<Clump>();}

		Primitive* getClumpPrimitive()						{return rbx_static_cast<Primitive*>(getLower());}
		const Primitive* getConstClumpPrimitive() const		{return rbx_static_cast<const Primitive*>(getConstLower());}

		static Clump* getPrimitiveClump(Primitive* p);
		static const Clump* getConstPrimitiveClump(const Primitive* p);
		static bool isClumpRootPrimitive(const Primitive* p);

		void loadMotors(G3D::Array<Joint*>& load, bool nonAnimatedOnly);
		void loadConstMotors(G3D::Array<const Joint*>& load, bool nonAnimatedOnly) const;

		template<class Func>
		inline void visitPrimitives(Func func) {
			Primitive* p = getClumpPrimitive();
			RBXASSERT(p);
			visitPrimitivesImpl(func, p);
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}	// namespace
