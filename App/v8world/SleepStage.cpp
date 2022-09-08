/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SleepStage.h"
#include "V8World/SimulateStage.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8World/Joint.h"
#include "V8World/Edge.h"
#include "V8World/Primitive.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "Util/Profiling.h"
#include "rbx/Debug.h"
#include "rbx/Profiler.h"
#include "V8Kernel/Kernel.h"

DYNAMIC_FASTFLAGVARIABLE(PGSWakeOtherAssemblyForJoints, false)

namespace RBX {

/*
Wake up events:

RECURSIVE
	STEPPING -> TOUCHING (touch event)	*High Velocity
	Explosion

NON-RECURSIVE
	STEPPING -> TOUCHING (touch event)	*Low Velocity
	TOUCHING -> STEPPING
	RotateJoint:				
	Humanoid on Floor:			
	Gyro applying different Force
*/

// Stock params:  size ==20, skip==8, tolerance = 0.1


float SleepStage::highVelocityContact()		{return 1.0;}		// studs/second

/////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
SleepStage::SleepStage(IStage* upstream, World* world)
	: IWorldStage(	upstream, 
					new SimulateStage(this, world), 
					world)
	, profilingWake(new Profiling::CodeProfiler("Wake"))
	, profilingCollision(new Profiling::CodeProfiler("Collision"))
	, profilingJointSleep(new Profiling::CodeProfiler("Joint Sleep"))
	, profilingSleep(new Profiling::CodeProfiler("Sleep"))
	, throttling(false)
	, longStepId(-1)
	, numContactsInStage(0)
	, numContactsInKernel(0)
	, recursivePassId(0)
	, externalRecursiveWake(false)
	, debugReentrant(false)
{}
#pragma warning(pop)


SleepStage::~SleepStage()
{
	RBXASSERT(numContactsInStage == 0);
	RBXASSERT(numContactsInKernel == 0);
	RBXASSERT(debugReentrant == 0);

	RBXASSERT(recursiveWakePending.empty());				// only on impact...
	RBXASSERT(wakePending.empty());
	RBXASSERT(awake.empty());
	RBXASSERT(sleepingChecking.empty());					// Edges that are awake
	RBXASSERT(sleepingDeeply.empty());						// no Edges that are awake
	RBXASSERT(removing.empty());
	RBXASSERT(steppingContacts[Sim::CAN_NOT_THROTTLE].size() == 0);
	RBXASSERT(touchingContacts[Sim::CAN_NOT_THROTTLE].size() == 0);
	RBXASSERT(steppingContacts[Sim::CAN_THROTTLE].size() == 0);
	RBXASSERT(touchingContacts[Sim::CAN_THROTTLE].size() == 0);
}

bool validateInKernel(Joint* j)
{
	bool oneInKernel = false;
	if (Joint::isKernelJoint(j)) {
//		Primitive* p = j->getPrimitive(0);
//		Primitive* assemblyRoot = p->getAssembly()->getAssemblyPrimitive();
//		oneInKernel = assemblyRoot->getBody()->inKernel();
		return true;			// todo Pull kernel joints if their Humanoid Moves
	}
	else {
		for (int i = 0; i < 2; ++i)	{
			if (Primitive* p = j->getPrimitive(i))
			{
				Body* b = p->getBody();
				if (b == b->getRoot() && b->getRootSimBody()->isInKernel()) {
					RBXASSERT(!b->isLeafBody());
					oneInKernel = true;
				}
				else if (b->isLeafBody()) {
					oneInKernel = true;
				}
			}
		}
	}
	return oneInKernel;
}


bool SleepStage::validateJoints()
{
	JointSet::const_iterator it = steppingJoints.begin();

	for (; it != steppingJoints.end(); ++it)
	{
		Joint* j = *it;
		bool ok = validateInKernel(j);
		if (!ok) {
			RBXASSERT(0);
			validateInKernel(j);
		}
	}
	return true;
}


bool SleepStage::validate()
{
	for (AssemblySetIt aIt = awake.begin(); aIt != awake.end(); ++aIt) 
	{
		Assembly* assembly = *aIt;
		RBXASSERT(assembly->getAssemblyState() == Sim::AWAKE);

		const G3D::Array<Edge*>& edges = assembly->getAssemblyEdges();
		for (int i = 0; i < edges.size(); ++i)
		{
			Edge* e = edges[i];
			if (e->inOrDownstreamOfStage(this)) {
				if (!edgeIsAwake(e)) {
					return false;
				}
				RBXASSERT(edgeIsAwake(e));				
			}
		}
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//
// 240 Hz
//
void SleepStage::stepSleepStage(int worldStepId, int _longStepId, bool _throttling)
{
	RBXPROFILER_SCOPE("Physics", "stepSleepStage");

	throttling = _throttling;
	longStepId = _longStepId;

	// 1. Clean up from old recursive wakes pending

	RBXASSERT_IF_VALIDATING(validate());

	stepAssembliesRecursiveWakePending();
	stepAssembliesWakePending();

	RBXASSERT(wakePending.empty());
	RBXASSERT(recursiveWakePending.empty());

	doContacts(steppingContacts);						// these are not in contact
														// new touch events - will make new recursive wakes
	RBXASSERT_IF_VALIDATING(validate());

	stepAssembliesRecursiveWakePending();
	stepAssembliesWakePending();

	RBXASSERT_IF_VALIDATING(validate());
	RBXASSERT(wakePending.empty());
	RBXASSERT(recursiveWakePending.empty());

	RBXASSERT_IF_VALIDATING(validate());

	stepAssembliesAwake();


	RBXASSERT_IF_VALIDATING(validate());

	if ((worldStepId % 4) == 0) 					// every 4th frame
	{		
		stepAssembliesSleepingChecking();
	}
	RBXASSERT_IF_VALIDATING(validate());
	RBXASSERT(recursiveWakePending.empty());

	stepAssembliesWakePending();

	RBXASSERT_IF_VALIDATING(validate());
	RBXASSERT(wakePending.empty());
	RBXASSERT(recursiveWakePending.empty());

	doContacts(touchingContacts);

	RBXASSERT_IF_VALIDATING(validate());

	stepAssembliesRecursiveWakePending();		// don't go to simulator with assemblies in pending states
	stepAssembliesWakePending();

	stepJoints();		// finally at the end

	RBXASSERT(wakePending.empty());
	RBXASSERT(recursiveWakePending.empty());
	RBXASSERT_IF_VALIDATING(validate());
	RBXASSERT_IF_VALIDATING(validateJoints());
}

void SleepStage::doContacts(ContactLists& contactLists)
{
	RBX::Profiling::Mark mark(*profilingCollision, false);
	stepContacts(contactLists[Sim::CAN_NOT_THROTTLE]);		
	if (!throttling) {
		stepContacts(contactLists[Sim::CAN_THROTTLE]);
	}
}




void SleepStage::stepAssembliesRecursiveWakePending()
{
    RBXPROFILER_SCOPE("Physics", "stepAssembliesRecursiveWakePending");

	wakeAssemblies(recursiveWakePending, 8, Sim::RECURSIVE_WAKE_PENDING);
}


void SleepStage::stepAssembliesWakePending()
{
    RBXPROFILER_SCOPE("Physics", "stepAssembliesWakePending");

	wakeAssemblies(wakePending, 0, Sim::WAKE_PENDING);
}

void SleepStage::wakeAssemblies(AssemblySet& wakeSet, int maxDepth, Sim::AssemblyState checkState)
{
	RBX::Profiling::Mark mark(*profilingWake, false);
	RBXASSERT_IF_VALIDATING(validate());

	std::deque<Assembly*> aDeque;

	recursivePassId++;

	for (AssemblySetIt it = wakeSet.begin(); it != wakeSet.end(); ++it) 
	{
		Assembly* a = *it;
		RBXASSERT(a->getAssemblyState() == checkState);
		RBXASSERT(a->getRecursiveDepth() == -1);
		a->setRecursivePassId(recursivePassId);
		a->setRecursiveDepth(0);
		aDeque.push_back(a);
	}

	while (!aDeque.empty())
	{
		Assembly* a = aDeque.front();
		aDeque.pop_front();
		RBXASSERT(a->getRecursivePassId() == recursivePassId);
		RBXASSERT(a->getRecursiveDepth() >= 0);
		RBXASSERT(a->getRecursiveDepth() <= maxDepth);
		traverse(a, aDeque, maxDepth);
		a->setRecursiveDepth(-1);	// done - should never visit again because recursivePassId is == recursivePassId;
	}

	externalRecursiveWake = false;	// reset this;

	RBXASSERT_IF_VALIDATING(validate());
}

bool canThrottle(Edge* e)
{
	RBXASSERT(e->getThrottleType() != Sim::UNDEFINED_THROTTLE);
	return (e->getThrottleType() == Sim::CAN_THROTTLE);
}


bool SleepStage::atLeastOneAssemblyMoving(Assembly* a0, Assembly* a1)
{
	return (a0->getAssemblyIsMovingState() || a1->getAssemblyIsMovingState());
}

void SleepStage::stepContacts(ContactList& contacts)
{
	RBXPROFILER_SCOPE("Physics", "stepContacts");

	RBXASSERT(toSleeping.empty());
	RBXASSERT(toStepping.empty());
	RBXASSERT(toContacting.empty());
	RBXASSERT(toContactingSleeping.empty());

	//int numContactsTouching = 0;

	int initialSize = contacts.size();
	int numConnectors = 0;

	for (int i = 0; i < contacts.size(); ++i) 
	{
		Contact* c = contacts[i];
		Primitive* p0 = c->getPrimitive(0);
		Primitive* p1 = c->getPrimitive(1);

		RBXASSERT(initialSize == contacts.size());
		RBXASSERT(!throttling || !canThrottle(c));	// if throttling, the contact should be one that "can't"
		RBXASSERT(c->inOrDownstreamOfStage(this));
		RBXASSERT(p0);
		RBXASSERT(p1);
		RBXASSERT(p0->getAssembly());
		RBXASSERT(p1->getAssembly());

#ifdef __RBX_CRASH_ASSERT_HP
		Joint* test = Primitive::getJoint(p0, p1);
		RBXASSERT_VERY_FAST(!test || !test->inOrDownstreamOfStage(this));
#endif

		bool assembliesMoving = atLeastOneAssemblyMoving(Assembly::getPrimitiveAssemblyFast(p0), Assembly::getPrimitiveAssembly(p1));
		bool inContact = c->step(longStepId);
		bool canCollide = (p0->getCanCollide() && p1->getCanCollide());
		bool wasTouching = (c->getEdgeState() == Sim::CONTACTING);

		numConnectors += c->numConnectors();

		Sim::EdgeState newState = computeContactState(assembliesMoving, inContact, canCollide, wasTouching);

		RBXASSERT(c->getEdgeState() != Sim::UNDEFINED);

		if (newState != c->getEdgeState()) {
			switch (newState)
			{
			case Sim::SLEEPING:				toSleeping.push_back(c);			break;
			case Sim::CONTACTING_SLEEPING:	toContactingSleeping.push_back(c);	break;
			case Sim::STEPPING:				toStepping.push_back(c);			break;
			case Sim::CONTACTING:			toContacting.push_back(c);			break;
            default:
                break;
			}
		}
	}
	changeContactState(toSleeping, Sim::SLEEPING);
	changeContactState(toStepping, Sim::STEPPING);
	changeContactState(toContacting, Sim::CONTACTING);
	changeContactState(toContactingSleeping, Sim::CONTACTING_SLEEPING);

	toSleeping.resize(0);					// prevent reallocation
	toStepping.resize(0);
	toContacting.resize(0);
	toContactingSleeping.resize(0);	

	RBXPROFILER_LABELF("Physics", "%d contacts, %d connectors", initialSize, numConnectors);
}

void SleepStage::stepJoints()
{
	RBX::Profiling::Mark mark(*profilingJointSleep, false);
	RBXASSERT(toSleepingJoint.empty());

	JointSet::iterator it;

	for (it = steppingJoints.begin(); it != steppingJoints.end(); ++it)
	{
		Joint* j = *it;
		RBXASSERT(j->inOrDownstreamOfStage(this));
		Primitive* p0 = j->getPrimitive(0);
		Primitive* p1 = j->getPrimitive(1);

		if (throttling && canThrottle(j)) {
			continue;
		}

		bool assembliesMoving = atLeastOneAssemblyMoving(p0->getAssembly(), p1->getAssembly());

		if (!assembliesMoving) {				
			toSleepingJoint.push_back(j);			// in contact, assemblies not moving (sleeping)
		}
	}

	changeJointState(toSleepingJoint, Sim::SLEEPING);

	toSleepingJoint.resize(0);					// prevent reallocation
}


void SleepStage::stepAssembliesAwake()
{
	RBX::Profiling::Mark mark(*profilingWake, false);
	RBXASSERT(toDeep.empty());
	RBXASSERT(toSleepingChecking.empty());

	for (AssemblySetIt it = awake.begin(); it != awake.end(); ++it) 
	{
		Assembly* assembly = *it;
		RBXASSERT(assembly->getAssemblyState() == Sim::AWAKE);

		bool notThrottlingThisAssembly = !(throttling && assembly->getCanThrottle());

		if (notThrottlingThisAssembly && assembly->sampleAndNotMoving())
		{
			switch (computeStateFromNeighbors(assembly))
			{
			case Sim::AWAKE:					break;
			case Sim::SLEEPING_CHECKING:		toSleepingChecking.push_back(assembly); break;
			case Sim::SLEEPING_DEEPLY:			toDeep.push_back(assembly);	break;
            default:
                break;
			}
		}
	}

	changeAssemblyState(toDeep, Sim::SLEEPING_DEEPLY);
	changeAssemblyState(toSleepingChecking, Sim::SLEEPING_CHECKING);

	toDeep.resize(0);
	toSleepingChecking.resize(0);
}


void SleepStage::stepAssembliesSleepingChecking()
{
	RBX::Profiling::Mark mark(*profilingSleep, false);
	RBXASSERT(toWake.empty());
	RBXASSERT(toDeep.empty());

	for (AssemblySetIt it = sleepingChecking.begin(); it != sleepingChecking.end(); ++it) 
	{
		Assembly* assembly = *it;
		RBXASSERT(assembly->getAssemblyState() == Sim::SLEEPING_CHECKING);

		switch (computeStateFromNeighbors(assembly))
		{
		case Sim::AWAKE:					toWake.push_back(assembly);	break;
		case Sim::SLEEPING_DEEPLY:			toDeep.push_back(assembly);	break;
		case Sim::SLEEPING_CHECKING:		break;
        default:
            break;
		}
	}

	changeAssemblyState(toWake, Sim::WAKE_PENDING);
	changeAssemblyState(toDeep, Sim::SLEEPING_DEEPLY);

	toWake.resize(0);
	toDeep.resize(0);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


void SleepStage::traverse(Assembly* assembly, std::deque<Assembly*>& aDeque, int maxDepth)
{
	Sim::AssemblyState state = assembly->getAssemblyState();
	int currentDepth = assembly->getRecursiveDepth();

	if (state != Sim::ANCHORED) 
	{
		if (state != Sim::AWAKE) {
			changeAssemblyState(assembly, Sim::AWAKE);
		}

		// edges that have both assemblies sleeping
		const G3D::Array<Edge*>& edges = assembly->getAssemblyEdges();
		for (int i = 0; i < edges.size(); ++i)
		{
			Edge* e = edges[i];
			RBXASSERT(e->inPipeline());
			if (e->inOrDownstreamOfStage(this)) 
			{
				wakeEdge(e);
				if (isAffecting(e))	
				{
					Assembly* other = assembly->otherAssembly(e);
					bool visited = (other->getRecursivePassId() == recursivePassId);
					if (currentDepth <  maxDepth) 
					{
						bool pushThrough = externalRecursiveWake ? true : (other->getAssemblyState() != Sim::AWAKE);
						
						if (pushThrough && !visited)
						{
							RBXASSERT(other->getRecursiveDepth() == -1);
							other->setRecursivePassId(recursivePassId);	// if not visited, visit at next depth level
							other->setRecursiveDepth(currentDepth + 1);
							aDeque.push_back(other);
						}
					}
					// If not doing a recursive wake, then set the other assembly if this edge is a Joint to be awakened.
					// This fixes PGS based models with joints that don't wake up due to the inherent stability of PGS.
					else if (DFFlag::PGSWakeOtherAssemblyForJoints && 
							 getKernel()->getUsingPGSSolver() &&
							 e->getEdgeType() == Edge::JOINT && !visited)
					{
						RBXASSERT(other->getRecursiveDepth() == -1);
						other->setRecursivePassId(recursivePassId);
						other->setRecursiveDepth(other->getRecursiveDepth() + 1);
						aDeque.push_back(other);
					}
					else 
					{
						if (other->getAssemblyState() == Sim::SLEEPING_DEEPLY) 
						{		// on the fringe, start checking...
							changeAssemblyState(other, Sim::SLEEPING_CHECKING);
						}
					}
				}
			}
		}
	}
}

bool SleepStage::isAffecting(Edge* e)
{
	RBXASSERT(e->inOrDownstreamOfStage(this));
	RBXASSERT(e->getEdgeState() != Sim::UNDEFINED);

	Sim::EdgeState state = e->getEdgeState();
	if (Contact::isContact(e)) {
		return ((state == Sim::CONTACTING) || (state == Sim::CONTACTING_SLEEPING));
	}
	else {
		return true;
	}
}

void SleepStage::wakeEdge(Edge* e)
{
	RBXASSERT(e->inOrDownstreamOfStage(this));
	RBXASSERT(e->getEdgeState() != Sim::UNDEFINED);
	Sim::EdgeState state = e->getEdgeState();
	if (Contact::isContact(e)) {
		if (state == Sim::CONTACTING_SLEEPING) {
			changeContactState(rbx_static_cast<Contact*>(e), Sim::CONTACTING);
		}
		else if (state == Sim::SLEEPING) {
			changeContactState(rbx_static_cast<Contact*>(e), Sim::STEPPING);
		}
	}
	else {
		RBXASSERT(Joint::isJoint(e));
		if (state == Sim::SLEEPING) {
			changeJointState(rbx_static_cast<Joint*>(e), Sim::STEPPING);
		}
	}
}


/*
						assembliesMoving						assembliesNotMoving
						canCollide			!canCollide			canCollide			!canCollide
wasTouching
	inContact			CONTACTING			STEPPING			CONTACTING_SLEEPING	SLEEPING

	outOfContact		STEPPING			STEPPING			SLEEPING			SLEEPING

wasStepping (!touching)
	inContact			CONTACTING			STEPPING			SLEEPING			SLEEPING
	
	outOfContact		STEPPING			STEPPING			SLEEPING			SLEEPING
*/


Sim::EdgeState SleepStage::computeContactState(bool assembliesMoving, bool inContact, bool canCollide, bool wasTouching)
{
	if (assembliesMoving) {
		if (inContact && canCollide) {
			return Sim::CONTACTING;
		}
		else {
			return Sim::STEPPING;
		}
	}
	else {
		if (wasTouching && inContact && canCollide) {
			return Sim::CONTACTING_SLEEPING;
		}
		else {
			return Sim::SLEEPING;
		}
	}
}

void SleepStage::wakeEvent(Edge* e)
{
	for (int i = 0; i < 2; ++i) {
		wakeEvent(e->getPrimitive(i)->getAssembly());
	}
}


// overtrumps "WAKE_PENDING" state
void SleepStage::recursiveWakeEvent(Contact* c)
{
	for (int i = 0; i < 2; ++i) {
		Assembly* a = c->getPrimitive(i)->getAssembly();
		recursiveWakeEvent(a);
	}
}


// wake assembly and all edges
// make all neighbors SLEEPING_CHECKING instead of SLEEPING_DEEPLY

void SleepStage::wakeEvent(Assembly* a)
{
	Sim::AssemblyState state = a->getAssemblyState();
	if (	(state != Sim::ANCHORED) 
		&&	(state != Sim::AWAKE)				// added this line 3/6/09 - am I missing anything? /DB
		&&	(state != Sim::WAKE_PENDING)
		&&	(state != Sim::RECURSIVE_WAKE_PENDING)) 
	{
		changeAssemblyState(a, Sim::WAKE_PENDING);
	}
}
	
void SleepStage::recursiveWakeEvent(Assembly* a)
{
	Sim::AssemblyState state = a->getAssemblyState();
	if (	(state != Sim::ANCHORED) 
		&&	(state != Sim::RECURSIVE_WAKE_PENDING)) 
	{
		changeAssemblyState(a, Sim::RECURSIVE_WAKE_PENDING);
	}
}
	



void SleepStage::changeContactState(const std::vector<Contact*>& contacts, Sim::EdgeState newState)
{
	for (size_t i = 0; i < contacts.size(); ++i) {
		changeContactState(contacts[i], newState);
	}
}

void SleepStage::changeJointState(const std::vector<Joint*>& joints, Sim::EdgeState newState)
{
	for (size_t i = 0; i < joints.size(); ++i) {
		changeJointState(joints[i], newState);
	}
}


void SleepStage::changeAssemblyState(const std::vector<Assembly*>& assemblies, Sim::AssemblyState newState)
{
	for (size_t i = 0; i < assemblies.size(); ++i) {
		changeAssemblyState(assemblies[i], newState);
	}
}


bool highVelocityVector(const Vector3& v, float max)
{
	for (int i = 0; i < 3; ++i) {
		if (fabs(v[i]) > max) {
			return true;
		}
	}
	return false;
}


bool SleepStage::highVelocityNewTouch(Contact* c)
{
	for (int i = 0; i < 2; ++i) {
		Assembly* a = c->getPrimitive(i)->getAssembly();
		const Velocity& v = a->getAssemblyPrimitive()->getPV().velocity;
		if (highVelocityVector(v.linear, highVelocityContact())) {
			return true;
		}
		else {
			float radius = a->computeMaxRadius();
			if (highVelocityVector(v.rotational * radius, highVelocityContact())) {
				return true;
			}
		}
	}
	return false;
}


void SleepStage::changeContactState(Contact* c, Sim::EdgeState newState)
{
	Sim::EdgeState currentState = c->getEdgeState();
	Sim::ThrottleType throttleType = c->getThrottleType();
	
	RBXASSERT(currentState != newState);

	// 1.  Stepping lists
	bool wasInStepping = (currentState == Sim::STEPPING);
	bool wasInTouching = (currentState == Sim::CONTACTING);
	bool desiredInStepping = (newState == Sim::STEPPING);
	bool desiredInTouching = (newState == Sim::CONTACTING);
	if (wasInStepping != desiredInStepping) {
		if (wasInStepping) {
			steppingContacts[throttleType].fastRemove(c);
		}
	}
	if (wasInTouching != desiredInTouching) {
		if (wasInTouching) {
			touchingContacts[throttleType].fastRemove(c);
		}
	}
	if (wasInStepping != desiredInStepping) {
		if (desiredInStepping) {
			steppingContacts[throttleType].fastAppend(c);
		}
	}
	if (wasInTouching != desiredInTouching) {
		if (desiredInTouching) {
			touchingContacts[throttleType].fastAppend(c);
		}
	}

	// 2. Kernel
	bool wasInKernel = (currentState == Sim::CONTACTING);
	bool desiredInKernel = (newState == Sim::CONTACTING);
	if (wasInKernel != desiredInKernel) {
		if (wasInKernel) {
			getDownstreamWS()->onEdgeRemoving(c);
			numContactsInKernel--;
		}
		else {
			getDownstreamWS()->onEdgeAdded(c);
			numContactsInKernel++;
		}
	}

	// 3. State Transitions - could reset the state
	if ((currentState == Sim::STEPPING) && (newState == Sim::CONTACTING)) {
		if (highVelocityNewTouch(c)) {
			recursiveWakeEvent(c);	
		}
		else {
			wakeEvent(c);
		}
	}
	else if ((currentState == Sim::CONTACTING) && (newState == Sim::STEPPING)) {
		wakeEvent(c);
	}

	// 4. Fire collision signal
	if ((newState == Sim::CONTACTING) && c->getNumTouchCycles() < 3)
		getWorld()->onPrimitiveCollided(c->getPrimitive(0), c->getPrimitive(1));

	c->setEdgeState(newState);
}

void SleepStage::changeJointState(Joint* j, Sim::EdgeState newState)
{
	Sim::EdgeState currentState = j->getEdgeState();
	RBXASSERT(currentState != newState);
	
	// 1.  In the list, in the kernel (the same)
	bool wasInList = (currentState == Sim::STEPPING);
	bool desiredInList = (newState == Sim::STEPPING);
	if (wasInList != desiredInList) {
		if (wasInList) {
			int num = steppingJoints.erase(j);
			RBXASSERT(num == 1);
			getDownstreamWS()->onEdgeRemoving(j);
		}
		else {
			bool ok = steppingJoints.insert(j).second;
			RBXASSERT(ok);
			getDownstreamWS()->onEdgeAdded(j);
		}
	}

	j->setEdgeState(newState);
}



void SleepStage::changeAssemblyState(Assembly* a, Sim::AssemblyState newState)
{
	RBXASSERT(!debugReentrant);
	debugReentrant = true;

	if (!a->inOrDownstreamOfStage(this)) {
		RBXASSERT(a->inStage(IStage::HUMANOID_STAGE));
	}
	else {
		Sim::AssemblyState currentState = a->getAssemblyState();
		RBXASSERT(currentState != newState);
		RBXASSERT(currentState != Sim::ANCHORED);

		AssemblySet& oldSet = stateToSet(currentState);
		AssemblySet& newSet = stateToSet(newState);

		// 1.  Put in right set
		int num = oldSet.erase(a);
		RBXASSERT(num == 1);
		bool ok = newSet.insert(a).second;
		RBXASSERT(ok);

		// 2.  All impulses gathered while sleeping are invalid and are discarded
		if (Sim::isSleepingAssemblyState(currentState) && !Sim::isSleepingAssemblyState(newState)) {
			a->getAssemblyPrimitive()->zeroVelocity();
			a->wakeUp();		// truly wake up
			a->getAssemblyPrimitive()->getBody()->resetImpulseAccumulators();		// move this to the kernel ; TODO
		}
		// 3.  All forces gathered while not in the kernel are discarded
		if (newState == Sim::AWAKE) {		// INTO KERNEL
			if (!a->downstreamOfStage(this)) {
				Body* b = a->getAssemblyPrimitive()->getBody();
				b->resetForceAccumulators();
				rbx_static_cast<SimulateStage*>(getDownstreamWS())->onAssemblyAdded(a);
			}
		}
		if (Sim::outOfKernelAssemblyState(newState)) {		// OUT OF KERNEL - note "wake pending" do not force into or out of kernel"
			if (a->downstreamOfStage(this)) {
				if (Sim::isSleepingAssemblyState(newState))
					a->getAssemblyPrimitive()->getBody()->getRootSimBody()->clearVelocity();
				rbx_static_cast<SimulateStage*>(getDownstreamWS())->onAssemblyRemoving(a);
			}
		}
		a->setAssemblyState(newState);
	}
	debugReentrant = false;
}

SleepStage::AssemblySet& SleepStage::stateToSet(Sim::AssemblyState state)
{
	switch (state)
	{
	case Sim::RECURSIVE_WAKE_PENDING:	return recursiveWakePending;
	case Sim::WAKE_PENDING:				return wakePending;
	case Sim::AWAKE:					return awake;
	case Sim::SLEEPING_CHECKING:		return sleepingChecking;
	case Sim::SLEEPING_DEEPLY:			return sleepingDeeply;
	case Sim::REMOVING:					return removing;
	default:							RBXASSERT(0);	return awake;
	}
}


bool SleepStage::edgeIsAwake(Edge* e)
{
	Sim::EdgeState state = e->getEdgeState();
	return ((state == Sim::STEPPING) || (state == Sim::CONTACTING));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Sim::AssemblyState SleepStage::computeStateFromNeighbors(Assembly* assembly)
{
	if (!Primitive::allowSleep) {			// global debug property
		return Sim::AWAKE;
	}

	bool allNeighborsSleeping = true;

	const G3D::Array<Edge*>& edges = assembly->getAssemblyEdges();
	for (int i = 0; i < edges.size(); ++i)
	{
		Edge* e = edges[i];
		if (e->inOrDownstreamOfStage(this)) {
			if (isAffecting(e)) {
				Assembly* other = assembly->otherAssembly(e);
				if (other->getAssemblyState() == Sim::AWAKE) {
					if (other->inOrDownstreamOfStage(this)) {
						allNeighborsSleeping = false;
						if (other->preventNeighborSleep()) {
							return Sim::AWAKE;
						}
					}
				}
			}
		}
	}
	return allNeighborsSleeping ? Sim::SLEEPING_DEEPLY : Sim::SLEEPING_CHECKING;
}


void SleepStage::onExternalTickleAssembly(Assembly* assembly, bool recursive)
{
	if (assembly->inPipeline()) {
		if (assembly->inOrDownstreamOfStage(this)) {
			assembly->wakeUp();		// reset timer - I will be awake for a while
			if (recursive) {
				externalRecursiveWake = true;
				recursiveWakeEvent(assembly);
			}
			else {
				wakeEvent(assembly);
			}
		}
	}
}



//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void SleepStage::onAssemblyAdded(Assembly* assembly)
{
	RBXASSERT(!assembly->computeIsGrounded());
	RBXASSERT(assembly->getAssemblyState() == Sim::ANCHORED);

	assembly->getAssemblyPrimitive()->getBody()->resetForceAccumulators();			// move this to the kernel ; TODO
	assembly->getAssemblyPrimitive()->getBody()->resetImpulseAccumulators();		// move this to the kernel ; TODO
	assembly->putInStage(this);

	if (assembly->getAssemblyPrimitive()->getNetworkIsSleeping()) 
	{
		assembly->reset(Sim::SLEEPING_CHECKING);
		bool ok = sleepingChecking.insert(assembly).second;
		RBXASSERT(ok);
		RBXASSERT(assembly->getAssemblyState() == Sim::SLEEPING_CHECKING);
	}
	else 
	{
		assembly->reset(Sim::WAKE_PENDING);
		assembly->wakeUp();		// the reset fills the average with the current value.  This wipes it.  Assembly can't sleep for 20 steps (buffer size)
		bool ok = wakePending.insert(assembly).second;
		RBXASSERT(ok);
		RBXASSERT(assembly->getAssemblyState() == Sim::WAKE_PENDING);
	}
}

void SleepStage::onAssemblyRemoving(Assembly* assembly)
{
	RBXASSERT(assembly->inOrDownstreamOfStage(this));
	RBXASSERT(assembly->getAssemblyState() != Sim::ANCHORED);
	RBXASSERT(assembly->getAssemblyState() != Sim::REMOVING);

	changeAssemblyState(assembly, Sim::REMOVING);		// pop from Kernel, handles the moving changed as well

	int num = removing.erase(assembly);
	RBXASSERT(num == 1);
	assembly->removeFromStage(this);
	assembly->reset(Sim::ANCHORED);
}


void SleepStage::onEdgeAdded(Edge* e)
{
	RBXASSERT(!e->inOrDownstreamOfStage(this));
	RBXASSERT(e->getEdgeState() == Sim::UNDEFINED);
	RBXASSERT(e->getThrottleType() == Sim::UNDEFINED_THROTTLE);

	e->setThrottleType( Assembly::computeCanThrottle(e) ? Sim::CAN_THROTTLE : Sim::CAN_NOT_THROTTLE );

	e->putInStage(this);

	if (Contact::isContact(e)) {
		Contact* c = rbx_static_cast<Contact*>(e);
		Sim::ThrottleType throttleType = c->getThrottleType();		// we just set this value above
		steppingContacts[throttleType].fastAppend(c);
		c->setEdgeState(Sim::STEPPING);
		numContactsInStage++;
	}
	else {
		Joint* j = rbx_static_cast<Joint*>(e);
		j->setEdgeState(Sim::SLEEPING);
		changeJointState(j, Sim::STEPPING);	// put in kernel
	}
}


void SleepStage::onEdgeRemoving(Edge* e)
{
	RBXASSERT(e->inOrDownstreamOfStage(this));

	if (Contact::isContact(e)) {
		if (	(e->getEdgeState() == Sim::CONTACTING_SLEEPING) 
			||	(e->getEdgeState() == Sim::CONTACTING)) 
		{
			wakeEvent(e);
		}
		if (e->getEdgeState() != Sim::SLEEPING) {
			changeContactState(rbx_static_cast<Contact*>(e), Sim::SLEEPING);
		}
		numContactsInStage--;
	}
	else {
		if (e->getEdgeState() == Sim::SLEEPING) {
			wakeEvent(e);
		}
		else {
			changeJointState(rbx_static_cast<Joint*>(e), Sim::SLEEPING);
		}
	}

	RBXASSERT(e->inStage(this));
	RBXASSERT(e->getEdgeState() == Sim::SLEEPING);
	RBXASSERT(e->getThrottleType() != Sim::UNDEFINED_THROTTLE);
	e->setThrottleType(Sim::UNDEFINED_THROTTLE);
	e->setEdgeState(Sim::UNDEFINED);
	e->removeFromStage(this);
}


int SleepStage::getMetric(IWorldStage::MetricType metricType)
{
	switch (metricType)
	{
	case NUM_CONTACTSTAGE_CONTACTS:
		{
			return numContactsInStage;
		}
	case NUM_STEPPING_CONTACTS:
		{
			return steppingContacts[Sim::CAN_NOT_THROTTLE].size() + steppingContacts[Sim::CAN_THROTTLE].size();
		}
	case NUM_TOUCHING_CONTACTS:
		{
			return numContactsInKernel;
		}
	default:
		{
			return Super::getMetric(metricType);
		}
	}
}


} // namespace
