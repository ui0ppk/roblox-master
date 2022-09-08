/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IPipelined.h"
#include "Util/IndexedMesh.h"
#include "boost/utility.hpp"
#include "Assembly.h"

namespace RBX {
	class Primitive;

	class Mechanism 
		: public IPipelined
		, public boost::noncopyable
		, public IndexedMesh
	{
	private:
		static bool assemblyHasMovingParent(const Assembly* a);

	public:
		Mechanism();

		~Mechanism();

		Primitive* getMechanismPrimitive();
		const Primitive* getConstMechanismPrimitive() const;

		Assembly* getRootAssembly();
		const Assembly* getConstRootAssembly() const;

		///////////////////////////////////////////////////////////////
		// Primitive Stuff

		static bool isMechanismRootPrimitive(const Primitive* p);

		static Mechanism* getPrimitiveMechanism(Primitive* p);
		static const Mechanism* getConstPrimitiveMechanism(const Primitive* p);

		static Primitive* getRootMovingPrimitive(Primitive* p);
		static const Primitive* getConstRootMovingPrimitive(const Primitive* p);

		///////////////////////////////////////////////////////////////
		//  Assembly stuff

		static bool isMovingAssemblyRoot(const Assembly* a);

		static bool isComplexMovingMechanism(const Assembly* a);		// i.e. - the assembly has children, connected by spring joints - complex networking issues

		static Assembly* getMovingAssemblyRoot(Assembly* a);
		static const Assembly* getConstMovingAssemblyRoot(const Assembly* a);

    private:
        template<class Func>
        inline void visitPrimitivesImpl(Func func, Assembly* a) {
            a->visitPrimitives(func);
            for (int i = 0; i < a->numChildren(); ++i) {
                Assembly* child = a->getTypedChild<Assembly>(i);
                visitPrimitivesImpl(func, child);
            }
        }

    public:
        // Primitive Visiting Functions
        template<class Func>
        inline void visitPrimitives(Func func) {
            Assembly *root = getRootAssembly();
            RBXASSERT(root);
            visitPrimitivesImpl(func, root);
        }
	};

} // namespace
