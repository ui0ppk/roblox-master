/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Mechanism.h"
#include "V8World/Primitive.h"
#include "V8World/Clump.h"
#include "V8World/Assembly.h"
#include "V8World/Joint.h"

namespace RBX {

Mechanism::Mechanism()
{}

Mechanism::~Mechanism()
{}

const Primitive* Mechanism::getConstMechanismPrimitive() const
{
	return getConstTypedLower<Assembly>()->getConstTypedLower<Clump>()->getConstTypedLower<Primitive>();
}

Primitive* Mechanism::getMechanismPrimitive()
{
	return const_cast<Primitive*>(getConstMechanismPrimitive());
}




bool Mechanism::assemblyHasMovingParent(const Assembly* a)
{
	return (a->getConstTypedParent<Assembly>() && !a->getConstTypedParent<Assembly>()->computeIsGrounded());
}

bool Mechanism::isComplexMovingMechanism(const Assembly* a)
{
	RBXASSERT(isMovingAssemblyRoot(a));		// shouldn't be calling this otherwise

#ifdef _DEBUG
	// quick test to make sure all children are through spring joints.
	for (int i = 0; i < a->numChildren(); ++i)
	{
		const Assembly* child = a->getConstTypedChild<Assembly>(i);
		const Primitive* p = child->getConstAssemblyPrimitive();
		const SpanningEdge* spanningEdge = p->getConstEdgeToParent();
		const Joint* j = rbx_static_cast<const Joint*>(spanningEdge);
		RBXASSERT(Joint::isSpringJoint(j));
	}
#endif

	return (a->numChildren() > 0);
}

bool Mechanism::isMovingAssemblyRoot(const Assembly* a)
{
	if (a->computeIsGrounded()) {
		return false;
	}
	else {
		bool movingParent = assemblyHasMovingParent(a);
		return !movingParent;
	}
}

Assembly* Mechanism::getMovingAssemblyRoot(Assembly* a)
{
	while (assemblyHasMovingParent(a))
	{
		a = a->getTypedParent<Assembly>();
	}
	return a;
}

const Assembly* Mechanism::getConstMovingAssemblyRoot(const Assembly* a)
{
	while (assemblyHasMovingParent(a))
	{
		a = a->getConstTypedParent<Assembly>();
	}
	return a;
}


// static
const Primitive* Mechanism::getConstRootMovingPrimitive(const Primitive* p)
{
	const Assembly* a = p->getConstAssembly();
	RBXASSERT(a);
	if (!a) {
		return NULL;
	}

	if (a->computeIsGrounded()) {
		return NULL;
	}
	const Assembly* movingRoot = getConstMovingAssemblyRoot(a);
	return movingRoot->getConstAssemblyPrimitive();
}

Primitive* Mechanism::getRootMovingPrimitive(Primitive* p)
{
	return const_cast<Primitive*>(getConstRootMovingPrimitive(p));
}


Mechanism* Mechanism::getPrimitiveMechanism(Primitive* p)
{
	if (Assembly* assembly = Assembly::getPrimitiveAssembly(p)) {
		return rbx_static_cast<Mechanism*>(assembly->getComputedUpper());
	}
	return NULL;
}

const Mechanism* Mechanism::getConstPrimitiveMechanism(const Primitive* p)
{
	if (const Assembly* assembly = Assembly::getConstPrimitiveAssembly(p)) {
		return rbx_static_cast<const Mechanism*>(assembly->getConstComputedUpper());
	}
	return NULL;
}



Assembly* Mechanism::getRootAssembly()
{
	Assembly* answer = getTypedLower<Assembly>();
	RBXASSERT(answer);
	return answer;
}

const Assembly* Mechanism::getConstRootAssembly() const
{
	const Assembly* answer = getConstTypedLower<Assembly>();
	RBXASSERT(answer);
	return answer;
}

bool Mechanism::isMechanismRootPrimitive(const Primitive* p)
{
	bool answer = (		p 
					&&	p->getConstTypedUpper<Clump>()
					&&	p->getConstTypedUpper<Clump>()->getConstTypedUpper<Assembly>()
					&&	p->getConstTypedUpper<Clump>()->getConstTypedUpper<Assembly>()->getConstTypedUpper<Mechanism>()	);
	
	RBXASSERT(!answer || (getConstPrimitiveMechanism(p)->getConstTypedLower<Assembly>()->getConstTypedLower<Clump>()->getConstTypedLower<Primitive>() == p));
	return answer;
}


}	// namespace
