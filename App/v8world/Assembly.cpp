/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Assembly.h"
#include "V8World/AssemblyHistory.h"
#include "V8World/Clump.h"
#include "V8World/Primitive.h"
#include "V8World/Contact.h"
#include "V8World/MotorJoint.h"
#include "V8World/Motor6DJoint.h"
#include "V8World/RigidJoint.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Kernel.h"
#include "util/standardout.h"
#include "V8DataModel/JointInstance.h"

namespace RBX {


Assembly::Assembly()
: state(Sim::ANCHORED)
, filterPhase(NOT_ASSIGNED)
, inCode(false)
, networkHumanoidState(0)
, simJob(NULL)
, recursivePassId(-1)
, recursiveDepth(-1)
, history(NULL)
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
, maxRadius(this, &Assembly::computeAssemblyMaxRadius)
, animationControlled(false)
#pragma warning(pop)
{}


Assembly::~Assembly()
{
	RBXASSERT(history == NULL);
	RBXASSERT(state == Sim::ANCHORED);
	RBXASSERT(simJob == NULL);
	RBXASSERT(recursiveDepth == -1);
}
/*
		void reset(Sim::AssemblyState newState);		// resets state, sleep count, running average
		bool sampleAndNotMoving();
		bool preventNeighborSleep();
		void wakeUp();									// moving from sleeping to non-Sleeping state
*/

void Assembly::reset(Sim::AssemblyState newState)
{
	RBXASSERT(state != newState);

	Sim::AssemblyState oldState = state;

	state = newState;

	if (oldState != Sim::ANCHORED) {
		RBXASSERT(history);

		if( newState != Sim::ANCHORED)
		{
			history->clear(*this);
			return;
		}

		delete history;
		history = NULL;
		return;
	}

	if (newState != Sim::ANCHORED) {
		RBXASSERT(!history);
		history = new AssemblyHistory(*this);		// 30 samples for the averageStateBuffer
	}
}

bool Assembly::sampleAndNotMoving()
{
	return history->sampleAndNotMoving(*this);
}

bool Assembly::preventNeighborSleep()
{
	return history->preventNeighborSleep();
}

void Assembly::wakeUp()
{
    getAssemblyPrimitive()->setNetworkIsSleeping(false, Time::nowFast());    // TL (3/25/11) - Without this the assembly parent
                                                            // might still be network sleeping even though the 
                                                            // whole assembly is undergoing wake-up.  Caused floating parts in network play.

	if (history) {
		history->wakeUp();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


Sim::AssemblyState Assembly::getAssemblyState() const
{
	RBXASSERT((state == Sim::ANCHORED) || (getCurrentStage()->getStageType() >= IStage::HUMANOID_STAGE));

	return state;
}

Primitive* Assembly::getAssemblyPrimitive()	
{
	Primitive* answer = getTypedLower<Clump>()->getTypedLower<Primitive>();
	RBXASSERT(answer);
	return answer;
}

const Primitive* Assembly::getConstAssemblyPrimitive() const
{
	const Primitive* answer = getConstTypedLower<Clump>()->getConstTypedLower<Primitive>();
	RBXASSERT(answer);
	return answer;
}

Assembly* Assembly::getPrimitiveAssemblyFast(Primitive* p)
{
	IndexedMesh* clump = p->getComputedUpper();
	RBXASSERT(clump);
	Assembly* answer = static_cast<Assembly*>(clump->getComputedUpper());
	RBXASSERT_IF_VALIDATING(!answer || dynamic_cast<Assembly*>(answer));
	return answer;
}


Assembly* Assembly::getPrimitiveAssembly(Primitive* p)
{
	if (IndexedMesh* clump = p->getComputedUpper()) {
		Assembly* answer = static_cast<Assembly*>(clump->getComputedUpper());
		RBXASSERT_IF_VALIDATING(!answer || dynamic_cast<Assembly*>(answer));
		return answer;
	}
	return NULL;
}

const Assembly* Assembly::getConstPrimitiveAssembly(const Primitive* p)
{
	if (const IndexedMesh* clump = p->getConstComputedUpper()) {
		const Assembly* answer = static_cast<const Assembly*>(clump->getConstComputedUpper());
		RBXASSERT_IF_VALIDATING(!answer || dynamic_cast<const Assembly*>(answer));
		return answer;
	}
	return NULL;
}

void Assembly::onLowersChanged()
{
	maxRadius.setDirty();
}

Clump* Assembly::getAssemblyClump() 
{
	Clump* answer = getTypedLower<Clump>();
	RBXASSERT(answer);
	return answer;
}

const Clump* Assembly::getConstAssemblyClump() const
{
	const Clump* answer = getConstTypedLower<Clump>();
	RBXASSERT(answer);
	return answer;
}



bool lessMotor(const Joint* a0, const Joint* a1)
{
	return a0->isLighterThan(a1);
}



void Assembly::getPhysics(G3D::Array<CompactCFrame>& motorAngles) const
{
	G3D::Array<const Joint*> motors;

    getConstAssemblyMotors(motors, true);

	motorAngles.resize(motors.size());

	for (int i = 0; i < motorAngles.size(); ++i) 
	{
		if(MotorJoint::isMotorJoint(motors[i]))
		{
			const MotorJoint* m1d = rbx_static_cast<const MotorJoint*>(motors[i]);
			motorAngles[i] = CompactCFrame(Vector3::zero(), Vector3::unitZ(), m1d->getCurrentAngle());
		}
		else
		{
            const Motor6DJoint* m6d = rbx_static_cast<const Motor6DJoint*>(motors[i]);
			motorAngles[i] = CompactCFrame(m6d->getCurrentOffset(), m6d->getCurrentAngle());
		}

	}
}




void Assembly::setPhysics(const G3D::Array<CompactCFrame>& motorAngles, const PV& pv)
{
	getAssemblyMotors(assemblyMotors, true);

	bool anglesUpdated = false;

	if (assemblyMotors.size() == motorAngles.size()) 
	{
		for (int i = 0; i < assemblyMotors.size(); ++i) 
		{
			if(MotorJoint::isMotorJoint(assemblyMotors[i]))
			{
				MotorJoint* m1d = rbx_static_cast<MotorJoint*>(assemblyMotors[i]);
				// TODO: Put this assertion back in and debug http://roblox.onjira.com/browse/CLIENT-250
				//RBXASSERT(G3D::fuzzyEq(fabs(motorAngles[i].getAxis().z), 1.0f));

				RBXASSERT(motorAngles[i].translation.isZero());
				float newAngle = motorAngles[i].getAngle() * motorAngles[i].getAxis().z;
				anglesUpdated = anglesUpdated || (newAngle != m1d->getCurrentAngle());
				m1d->setCurrentAngle( newAngle );
			}
			else
			{
				Motor6DJoint* m6d = rbx_static_cast<Motor6DJoint*>(assemblyMotors[i]);
				Vector3 newAngle = motorAngles[i].getAxisAngle();
				anglesUpdated = anglesUpdated || (newAngle != m6d->getCurrentAngle());
				m6d->setCurrentOffsetAngle(motorAngles[i].translation, newAngle);
			}
		}
		//if(assemblyMotors.size() >= 4)
		//{
		//	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "motors: %f,%f,%f,%f", motorAngles[0].rotationangle,  motorAngles[1].rotationangle,  motorAngles[2].rotationangle,  motorAngles[3].rotationangle);
		//}

		// prevent duplicate traversal of the assembly when the angles change and the pv changes
		if (anglesUpdated) {
			if (pv == getAssemblyPrimitive()->getPV()) {
				notifyMovedFromExternal();
			}
		}
	}
}



bool Assembly::isAssemblyRootPrimitive(const Primitive* p)
{
	bool answer = false;
	if (p) {
		if (const Clump* c = p->getConstTypedUpper<Clump>()) {
			if (c->getConstTypedUpper<Assembly>()) {
				answer = true;
			}
		}
	}

	RBXASSERT(!answer || (getConstPrimitiveAssembly(p)->getConstTypedLower<Clump>()->getConstTypedLower<Primitive>() == p));
	return answer;
}

Assembly* Assembly::otherAssembly(Edge* edge)
{
	Assembly* a0 = edge->getPrimitive(0)->getAssembly();
	Assembly* a1 = edge->getPrimitive(1)->getAssembly();
	RBXASSERT(a0 != a1);
	return (a0 == this) ? a1 : a0;
}


const Assembly* Assembly::otherConstAssembly(const Edge* edge) const
{
	const Assembly* a0 = edge->getConstPrimitive(0)->getConstAssembly();
	const Assembly* a1 = edge->getConstPrimitive(1)->getConstAssembly();
	RBXASSERT(a0 != a1);
	return (a0 == this) ? a1 : a0;
}


bool Assembly::getCanThrottle() const
{
	return getConstAssemblyPrimitive()->getCanThrottle();
}

bool Assembly::computeCanThrottle(Edge* edge)
{
	const Assembly* a0 = getConstPrimitiveAssembly(edge->getConstPrimitive(0));
	const Assembly* a1 = getConstPrimitiveAssembly(edge->getConstPrimitive(1));

	return (a0->getCanThrottle() && a1->getCanThrottle());
}


Vector2 Assembly::get2dPosition() const
{
	return getConstAssemblyPrimitive()->getCoordinateFrame().translation.xz();
}

void Assembly::getAssemblyMotors(G3D::Array<Joint*>& motors, bool nonAnimatedOnly)
{
	motors.fastClear();
	getAssemblyClump()->loadMotors(motors, nonAnimatedOnly);
	std::sort(motors.begin(), motors.end(), lessMotor);
}

void Assembly::getConstAssemblyMotors(G3D::Array<const Joint*>& motors, bool nonAnimatedOnly) const
{
	motors.fastClear();
	getConstAssemblyClump()->loadConstMotors(motors, nonAnimatedOnly);
	std::sort(motors.begin(), motors.end(), lessMotor);
}

void Assembly::gatherPrimitiveExternalEdges(Primitive* p)
{
	RBXASSERT(p->getAssembly() == this);

	Edge* e = p->getFirstEdge();

	size_t count = 0;

	while (e)
	{
		Primitive* other = e->otherPrimitive(p);
		if (other && (other->getAssembly()!= this)) 		// could be edge from P to NULL (ground)
		{
			assemblyExternalEdges.append(e);
		}
		e = p->getNextEdge(e);
		count++;
	}
}

const G3D::Array<Edge*>& Assembly::getAssemblyEdges()
{
	RBXASSERT(this->getCurrentStage()->getStageType() >= IStage::TREE_STAGE);

	assemblyExternalEdges.fastClear();

	this->visitPrimitives(boost::bind(&Assembly::gatherPrimitiveExternalEdges, this, _1));

	return assemblyExternalEdges;
}

bool Assembly::computeIsGroundingPrimitive(const Primitive* p)
{
	if (Joint::findConstJoint(p, Joint::ANCHOR_JOINT) != NULL) {
		return true;
	}
	else {
		return false;
	}
}


bool Assembly::computeIsGrounded() const
{
	return Assembly::computeIsGroundingPrimitive(getConstAssemblyPrimitive());
}



void computeAssemblyPrimitiveMaxRadius(Primitive* p, const Vector3& cofm, float& answer)
{
	Vector3 offsetCenter = Math::vector3Abs(p->getBody()->getPos() - cofm);
	Vector3 offsetCorner = p->getSize() * 0.5;
	float offset = (offsetCenter + offsetCorner).length();
	answer = std::max(answer, offset);
};

float Assembly::computeAssemblyMaxRadius()
{
	Primitive* p = getAssemblyPrimitive();

	Vector3 cofm(p->getBody()->getBranchCofmPos());
	float answer = 0.0f;

	this->visitPrimitives(boost::bind(&computeAssemblyPrimitiveMaxRadius, _1, boost::cref(cofm), boost::ref(answer)));

	return answer;
}

//void notifyAssemblyPrimitiveMoved(Primitive* p)
//{
//	p->getOwner()->notifyMoved();
//}


void notifyAssemblyPrimitiveMoved(Primitive* p, bool resetContacts)
{
	p->getOwner()->notifyMoved();

	if (resetContacts)
	{
#ifdef RBXASSERTENABLED
		int temp = p->getNumContacts();
#endif
		
		for (int i = 0; i < p->getNumContacts(); ++i)
		{
			RBXASSERT(p->getContact(i));
			p->getContact(i)->primitiveMovedExternally();
		}
		RBXASSERT(temp == p->getNumContacts());
	}
}

void Assembly::notifyMovedFromInternalPhysics()
{
	this->visitPrimitives(boost::bind(&notifyAssemblyPrimitiveMoved, _1, false));
}

void Assembly::notifyMovedFromExternal()
{
	this->visitPrimitives(boost::bind(&notifyAssemblyPrimitiveMoved, _1, true));
}





}// namespace
