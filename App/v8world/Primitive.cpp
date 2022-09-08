/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MaterialProperties.h"
#include "V8World/Primitive.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Kernel.h"
#include "V8World/Block.h"
#include "V8World/Ball.h"
#include "V8World/WedgePoly.h"
#include "V8World/PrismPoly.h"
#include "V8World/PyramidPoly.h"
#include "V8World/ParallelRampPoly.h"
#include "V8World/RightAngleRampPoly.h"
#include "V8World/CornerWedgePoly.h"
#include "V8World/MegaClusterPoly.h"
#include "V8World/SmoothClusterGeometry.h"
#include "V8World/TriangleMesh.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8World/Clump.h"
#include "V8World/RigidJoint.h"
#include "V8World/Contact.h"
#include "V8World/Tolerance.h"
#include "V8World/Cylinder.h"
#include "Util/Units.h"
#include "Util/Math.h"
#include "Network/NetworkOwner.h"

#include "btBulletCollisionCommon.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"


LOGGROUP(PrimitiveLifetime)
DYNAMIC_FASTFLAG(FixTouchEndedReporting)
FASTFLAG(PGSSolverFileDump)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

namespace RBX {

bool Primitive::allowSleep = true;	// globalSwitch

Primitive::Primitive(Geometry::GeometryType geometryType) 
: sizeMultiplier(DEFAULT_SIZE)
, sortSize(0)
, dragging(false)
, anchoredProperty(false)
, engineType(DYNAMICS_ENGINE)
, preventCollide(false)
, world(NULL)
, worldIndex(-1)
, elasticity(Primitive::defaultElasticity())
, friction(Primitive::defaultFriction())
, material(PLASTIC_MATERIAL)
, specificGravity(0.0)
, body(new Body())
, geometry(newGeometry(geometryType))
, myOwner(NULL)
, fuzzyExtentsStateId(fuzzyExtentsReset())
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
, joints(this)
, contacts(this)
#pragma warning(pop)
, jointK(0)
, jointKDirty(true)
, networkOwner(Network::NetworkOwner::Unassigned())
, networkOwnershipRule(NetworkOwnership_Auto)
, networkIsSleeping(false)
, surfaceData(NULL)
, customPhysicalProperties(PhysicalProperties())
{
	FASTLOG1(FLog::PrimitiveLifetime, "Primitive %p created", this);
	for (int i = 0; i < 6; i++) {
		surfaceType[i] = NO_SURFACE;
	}
}


Primitive::~Primitive() 
{
	RBXASSERT(!world);
	RBXASSERT(geometry);
	RBXASSERT(body);

	delete geometry;
	delete body;
    delete[] surfaceData;

	FASTLOG1(FLog::PrimitiveLifetime, "Primitive %p destroyed", this);
}

void Primitive::setNetworkIsSleeping(bool value, Time wakeupNow)
{
    if (value != networkIsSleeping) {
        networkIsSleeping = value;
        myOwner->onNetworkIsSleepingChanged(wakeupNow);
    }
}

void Primitive::onBuoyancyChanged(bool value)
{
	myOwner->onBuoyancyChanged( value );
}

unsigned int Primitive::getSizeMultiplier() const
{
	switch (sizeMultiplier)
	{
		case Primitive::DEFAULT_SIZE:	return 1;
		case Primitive::TORSO_SIZE:		return 5;
		case Primitive::ROOT_SIZE:		return 10;
		case Primitive::SEAT_SIZE:		return 20;

		default:					RBXASSERT(0); return 1;
	}
}

void Primitive::setSizeMultiplier(SizeMultiplier value)
{
	if (value != sizeMultiplier) 
	{
		if (!world) {
			sizeMultiplier = value;
			sortSize = 0;
		}
		else {
			RBXASSERT(0);		// show to dave - changing primitive name to/from "Torso" while in world
		}
	}
}


const Guid& Primitive::getGuid() const
{
	return guid;
}

void Primitive::setGuid(const Guid& value)
{
	RBXASSERT(!world);
	guid.copyDataFrom(value);

    if( FFlag::PGSSolverFileDump )
    {
        Guid::Data data;
        value.extract(data);
        body->setGuidIndex(data.index);
    }
}


//////////////////////////////////////////////////////////////
// Fuzzy Extents
static const int fuzzyExtentsReset() 
{
	return -2;
}



Extents Primitive::computeFuzzyExtents()
{
	RBXASSERT(!Math::isNanInfVector3(getBody()->getPos()));
	Extents answer = Extents::fromCenterCorner(
									getBody()->getPos(), 
									geometry->getCenterToCorner(getBody()->getCoordinateFrame().rotation));
	answer.expand(Tolerance::maxOverlapOrGap());
	return answer;
}


const Extents& Primitive::getFastFuzzyExtents()  
{
	if (fuzzyExtentsStateId != getBody()->getStateIndex()) {
		fuzzyExtents = computeFuzzyExtents();
		fuzzyExtentsStateId = getBody()->getStateIndex();
	}

	RBXASSERT_VERY_FAST(computeFuzzyExtents() == fuzzyExtents);	
	RBXASSERT_VERY_FAST(fuzzyExtentsStateId == getBody()->getStateIndex());

	return fuzzyExtents;
}



bool Primitive::hasAutoJoints() const
{
	const Joint* j = this->getConstFirstJoint();
	while (j)
    {
		if (Joint::isAutoJoint(j))
        {
            return true;
		}
		j = this->getConstNextJoint(j);
	}
	return false;
}


Primitive* Primitive::downstreamPrimitive(Joint* j)
{
	Primitive* p0 = j->getPrimitive(0);
	Primitive* p1 = j->getPrimitive(1);
	if (!p1) {
		return p0;
	}
	else {
		Body* b0 = p0->getBody();
		Body* b1 = p1->getBody();
		RBXASSERT((b0->getParent() == b1) || (b1->getParent() == b0));
		return (b0->getParent() == b1) ? p0 : p1;
	}
}

Edge* EdgeList::getNext(const Primitive* p, Edge* e) const
{
	RBXASSERT(e);
	RBXASSERT(e->links(p));
	unsigned int nextIndex = e->getIndex(p) + 1;
	return (nextIndex < list.size())
		? list[nextIndex].edge
		: NULL;
}

void EdgeList::insertEdge(Edge* e)
{
	RBXASSERT(e->getIndex(owner) == -1);

	e->setIndex(owner, list.size());
    
    Entry entry = { e, e->otherPrimitive(owner) };
	list.push_back(entry);
}

void EdgeList::removeEdge(Edge* e)
{
	int removeIndex = e->getIndex(owner);

	RBXASSERT(removeIndex >= 0);
	RBXASSERT(list[removeIndex].edge == e);

	// Move last item to removal index
	Entry oldLast = list.back();		// if array size == 1, this is redundant

	list[removeIndex] = oldLast;

	list.pop_back();

	// Update indices
	oldLast.edge->setIndex(owner, removeIndex);
	e->setIndex(owner, -1);
}


Joint* Primitive::getJoint(int id)
{
	return rbx_static_cast<Joint*>(joints.getEdge(id));
}

const Joint* Primitive::getConstJoint(int id) const
{
	return rbx_static_cast<Joint*>(joints.getEdge(id));
}


Contact* Primitive::getContact(int id)
{
	return rbx_static_cast<Contact*>(contacts.getEdge(id));
}

void Primitive::insertEdge(Edge* e)
{
	Primitive* p0 = e->getPrimitive(0);
	Primitive* p1 = e->getPrimitive(1);

	if (Joint::isJoint(e)) {
		p0->joints.insertEdge(e);
		if (p1) {
			p1->joints.insertEdge(e);
		}
		else {
			RBXASSERT(AnchorJoint::isAnchorJoint(rbx_static_cast<Joint*>(e)) || FreeJoint::isFreeJoint(rbx_static_cast<Joint*>(e)));
		}
	}
	else {
		RBXASSERT_VERY_FAST(Contact::isContact(e));
		p0->contacts.insertEdge(e);
		p1->contacts.insertEdge(e);
	}
}
	
void Primitive::removeEdge(Edge* e)
{
	Primitive* p0 = e->getPrimitive(0);
	Primitive* p1 = e->getPrimitive(1);

	if (Joint::isJoint(e)) {
		p0->joints.removeEdge(e);
		if (p1) {
			p1->joints.removeEdge(e);
		}
		else {
			RBXASSERT(AnchorJoint::isAnchorJoint(rbx_static_cast<Joint*>(e)) || FreeJoint::isFreeJoint(rbx_static_cast<Joint*>(e)));
		}
	}
	else {
		RBXASSERT_VERY_FAST(Contact::isContact(e));
		p0->contacts.removeEdge(e);
		p1->contacts.removeEdge(e);
	}
}



Edge* Primitive::getFirstEdge() const 
{
	return (joints.size() > 0) 
		? joints.getEdge(0) 
		: contacts.getFirst();
}

Edge* Primitive::getNextEdge(Edge* e) const 
{
	RBXASSERT_VERY_FAST(e);
	
	if (e->getEdgeType() == Edge::JOINT) {
		if (Edge* answer = joints.getNext(this, e)) {
			return answer;
		}
		else {
			return contacts.getFirst();
		}
	}
	else {
		return contacts.getNext(this, e);
	}
}

// Joint
const Joint* Primitive::getConstFirstJoint() const 
{
	return rbx_static_cast<Joint*>(joints.getFirst());
}

Joint* Primitive::getFirstJoint()
{
	return const_cast<Joint*>(getConstFirstJoint());
}


const Joint* Primitive::getConstNextJoint(const Joint* prev) const 
{
	RBXASSERT_VERY_FAST(prev);

	Edge* e = joints.getNext(this, const_cast<Joint*>(prev));

	return rbx_static_cast<const Joint*>(e);
}

Joint* Primitive::getNextJoint(Joint* prev)
{
	return const_cast<Joint*>(getConstNextJoint(prev));
}


// Contact
Contact* Primitive::getFirstContact() 
{
	return rbx_static_cast<Contact*>(contacts.getFirst());
}

Contact* Primitive::getNextContact(Contact* prev) 
{
	RBXASSERT_VERY_FAST(prev);

	return rbx_static_cast<Contact*>(contacts.getNext(this, prev));
}


RigidJoint* Primitive::getFirstRigidAt(Joint* start)
{
	while (start) {
		if (RigidJoint::isRigidJoint(start)) {
			return rbx_static_cast<RigidJoint*>(start);
		}
		start = this->getNextJoint(start);
	}
	return NULL;
}



RigidJoint* Primitive::getFirstRigid()
{
	return this->getFirstRigidAt(this->getFirstJoint());
}

RigidJoint* Primitive::getNextRigid(RigidJoint* prev)
{
	return this->getFirstRigidAt(this->getNextJoint(prev));
}

Joint* Primitive::getJoint(Primitive* p0, Primitive* p1, int index)
{
	RBXASSERT_VERY_FAST(p0 != p1);
	Primitive* leastJoints = (p0->getNumJoints() < p1->getNumJoints()) ? p0 : p1;
	Primitive* mostJoints = (leastJoints == p0) ? p1 : p0;

	int foundId = 0;
	for (int i = 0; i < leastJoints->getNumJoints(); ++i)
	{
		Primitive* other = leastJoints->getJointOther(i);
		if (other == mostJoints)
		{
			RBXASSERT(leastJoints->getJoint(i)->links(p0, p1));
			if (foundId == index)
			{
				return leastJoints->getJoint(i);
			}
			foundId++;
		}
		else
		{
			RBXASSERT(!leastJoints->getJoint(i)->links(p0, p1));
		}
	}
	return NULL;
}


Contact* Primitive::getContact(Primitive* p0, Primitive* p1)
{
	RBXASSERT_VERY_FAST(p0 != p1);
	Primitive* leastContacts = (p0->getNumContacts() < p1->getNumContacts()) ? p0 : p1;
	Primitive* mostContacts = (leastContacts == p0) ? p1 : p0;

	for (int i = 0; i < leastContacts->getNumContacts(); ++i)
	{
		Primitive* other = leastContacts->getContactOther(i);
		if (other == mostContacts)
		{
			RBXASSERT(leastContacts->getContact(i)->links(p0, p1));
			return leastContacts->getContact(i);
		}
		else
		{
			RBXASSERT(!leastContacts->getContact(i)->links(p0, p1));
		}
	}
	return NULL;
}

template<World::TouchInfo::Type T>
static void reportOverlap(Primitive* touchReporting, Primitive* touchOther)
{
	Assembly* touchReportingAssembly = touchReporting->getAssembly();
	Assembly* touchOtherAssembly = touchOther->getAssembly();
	RBXASSERT(touchReportingAssembly);
	RBXASSERT(touchOtherAssembly);

	if (	touchReporting->getOwner()->reportTouches()
		&& 	(touchReportingAssembly->getAssemblyIsMovingState() || touchOtherAssembly->getAssemblyIsMovingState() || 
				(DFFlag::FixTouchEndedReporting && (touchReporting->getDragging() || touchOther->getDragging()))))
	{
		if (DFFlag::FixTouchEndedReporting) 
		{
			touchReporting->getWorld()->reportTouchInfo(touchReporting, touchOther, T);
		}
		else
		{
            shared_ptr<PartInstance> nullPartInstance;
			const World::TouchInfo info = { touchReporting, touchOther, nullPartInstance, nullPartInstance, T };
			touchReporting->getWorld()->reportTouchInfo(info);
		}
	}
}

void Primitive::onNewOverlap(Primitive* p0, Primitive* p1)
{
	reportOverlap<World::TouchInfo::Touch>(p0, p1);
	reportOverlap<World::TouchInfo::Touch>(p1, p0);
}

void Primitive::onStopOverlap(Primitive* p0, Primitive* p1)
{
	reportOverlap<World::TouchInfo::Untouch>(p0, p1);
	reportOverlap<World::TouchInfo::Untouch>(p1, p0);
}

Clump* Primitive::getClump()
{
	return Clump::getPrimitiveClump(this);
}


const Clump* Primitive::getConstClump() const
{
	return Clump::getConstPrimitiveClump(this);
}

Assembly* Primitive::getAssembly()
{
	return Assembly::getPrimitiveAssembly(this);
}

const Assembly* Primitive::getConstAssembly() const
{
	return Assembly::getConstPrimitiveAssembly(this);
}

Mechanism* Primitive::getMechanism()
{
	return Mechanism::getPrimitiveMechanism(this);
}

const Mechanism* Primitive::getConstMechanism() const
{
	return Mechanism::getConstPrimitiveMechanism(this);
}


Geometry* Primitive::newGeometry(Geometry::GeometryType geometryType)
{
	switch (geometryType)
	{
	case Geometry::GEOMETRY_BALL:	return new Ball();		

	case Geometry::GEOMETRY_CYLINDER:	return new Cylinder();

	case Geometry::GEOMETRY_BLOCK:	
	default:						return new Block();	

	case Geometry::GEOMETRY_WEDGE:	return new WedgePoly();

	case Geometry::GEOMETRY_PRISM:	return new PrismPoly();

	case Geometry::GEOMETRY_PYRAMID:	return new PyramidPoly();

	case Geometry::GEOMETRY_PARALLELRAMP:	return new ParallelRampPoly();

	case Geometry::GEOMETRY_RIGHTANGLERAMP:	return new RightAngleRampPoly();

	case Geometry::GEOMETRY_CORNERWEDGE:	return new CornerWedgePoly();

	case Geometry::GEOMETRY_MEGACLUSTER:	return new MegaClusterPoly(this);

	case Geometry::GEOMETRY_SMOOTHCLUSTER:	return new SmoothClusterGeometry(this);

	case Geometry::GEOMETRY_TRI_MESH:	return new TriangleMesh();	
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
//  Geometry - size, type, parameters

Geometry::GeometryType Primitive::getGeometryType() const
{
	RBXASSERT(geometry);
    return geometry ? geometry->getGeometryType() : Geometry::GEOMETRY_UNDEFINED;
}


Geometry::CollideType Primitive::getCollideType() const
{
	RBXASSERT(geometry);
	return geometry->getCollideType();
}

Primitive* Primitive::getMechRoot()
{
	if (Mechanism* mechanism = getMechanism())
	{
		return mechanism->getMechanismPrimitive();
	}
	return NULL;
}

Primitive* Primitive::getRootMovingPrimitive()
{
	// This if statement is because we have RBX Assert that we don't want to trigger
	// inside of the getRootMovingPrimitive call.
	if (this->getConstAssembly())
	{
		return Mechanism::getRootMovingPrimitive(this);
	}
	return NULL;
}

bool Primitive::isAncestorOf(Primitive* prim)
{
	if (Primitive* parentPrim = prim->getTypedParent<Primitive>())
	{
		if (parentPrim == this)
		{
			return true;
		}
		return isAncestorOf(parentPrim);
	}
	return false;
}

int Primitive::getGeometryParameter(const std::string& parameter) const
{
	return geometry->getGeometryParameter(parameter);
}

void Primitive::resetGeometryType(Geometry::GeometryType geometryType)
{
	RBXASSERT_VERY_FAST(!hasAutoJoints());
	Vector3 oldSize = geometry->getSize();
	delete geometry;
	geometry = newGeometry(geometryType);
	if (world) {
		world->onPrimitiveGeometryChanged(this);
	}
	this->setSize(oldSize);
	jointKDirty = true;
}

void Primitive::setGeometryType(Geometry::GeometryType geometryType)
{
	RBXASSERT_VERY_FAST(!hasAutoJoints());

	if (geometry->getGeometryType() != geometryType) 
	{
		resetGeometryType(geometryType);
	}
}

void Primitive::setSize(const G3D::Vector3& size)
{
	RBXASSERT_VERY_FAST(!hasAutoJoints());

	Vector3 protectedSize = clipToSafeSize(size);

	if (protectedSize != geometry->getSize())
	{
		fuzzyExtentsStateId = fuzzyExtentsReset();		// extents are dirty
		geometry->setSize(protectedSize);

		if (!world && !DFFlag::MaterialPropertiesEnabled)
			setMassInertia(geometry->getVolume() * 1.0f);

		if (world) {
			updateMassValues(world->getUsingNewPhysicalProperties());
			world->onPrimitiveExtentsChanged(this);
			world->onPrimitiveGeometryChanged(this);
		}
		jointKDirty = true;

		// set this to 0 so it'll be recalculated
		sortSize = 0;
	}
}

void Primitive::setGeometryParameter(const std::string& parameter, int value)
{
	if (value != geometry->getGeometryParameter(parameter)) 
	{
		geometry->setGeometryParameter(parameter, value);
		if (!world && !DFFlag::MaterialPropertiesEnabled)
			setMassInertia(geometry->getVolume() * 1.0f);

		if (world) {
			updateMassValues(world->getUsingNewPhysicalProperties());
			world->onPrimitiveGeometryChanged(this);		// This is the big call the redoes the contacts
		}
		jointKDirty = true;
	}
}



float Primitive::computeJointK()	
{
    Vector3 size = getGeometry()->isTerrain() ? Vector3(4, 4, 4) : getSize();
	return Constants::getJointK(size, getGeometryType() == Geometry::GEOMETRY_BALL);
}

float Primitive::getJointK()
{
    if (jointKDirty)
    {
        jointK = computeJointK();
        jointKDirty = false;
    }
    
    return jointK;
}

bool Primitive::hitTest(const RbxRay& worldRay, Vector3& worldHitPoint, Vector3& surfaceNormal)
{
    const CoordinateFrame& cframe = getBody()->getCoordinateFrame();

	RbxRay localRay = cframe.toObjectSpace(worldRay);
	Vector3 localHitPoint;
    Vector3 localSurfaceNormal;

	bool hit = geometry->hitTest(localRay, localHitPoint, localSurfaceNormal);
    
	if (hit) {
		worldHitPoint = cframe.pointToWorldSpace(localHitPoint);
        surfaceNormal = cframe.vectorToWorldSpace(localSurfaceNormal);
		return true;
	}
	else {
		return false;
	}
}


void Primitive::setMassInertia(float mass) 
{
	getBody()->setMass(mass);
	getBody()->setMoment(geometry->getMoment(mass));
	getBody()->setCofmOffset(geometry->getCofmOffset());
	jointKDirty = true;
}



Vector3 Primitive::clipToSafeSize(const Vector3& newSize)
{
	// Temporary extend for cluster
	static const float maxVolume = 64.0e6f * 64.0e6f;

	Vector3 safeSize = newSize.min(Vector3(2048.0f, 2048.0f, 2048.0f)).max(Vector3::zero());	// up from 512

	if ((safeSize.x * safeSize.y * safeSize.z) > maxVolume)			// up from 1 million
	{
		safeSize.y = floorf(1.0e6f / (safeSize.x * safeSize.z));
		RBXASSERT_VERY_FAST(safeSize.x * safeSize.y * safeSize.z <= (maxVolume * 1.01f));
	}
	return safeSize;
}


bool Primitive::getCanThrottle() const
{
	return body->getCanThrottle();
}


void Primitive::setCanThrottle(bool value)
{
	if (body->getCanThrottle() != value)
	{
		Assembly* changing = NULL;
		if (world) {
			changing = world->onPrimitiveEngineChanging(this);
		}

		body->setCanThrottle(value, *this);
		RBXASSERT(!inPipeline() || !inKernel());
		if (changing) {
			world->onPrimitiveEngineChanged(changing);
		}
	}
}

void Primitive::setEngineType(EngineType value)
{
	if (engineType != value) 
	{
		Assembly* changing = NULL;
		if (world) {
			changing = world->onPrimitiveEngineChanging(this);
		}

		engineType = value;

		if (changing) {
			world->onPrimitiveEngineChanged(changing);
		}
	}
}

void Primitive::setOwner(IMoving* set)
{
	myOwner = set;
	FASTLOG2(FLog::PrimitiveLifetime, "Owner %p set on primitive %p", set, myOwner);
}

void Primitive::setDragging(bool value)
{
	setFixed(anchoredProperty, value);
}


void Primitive::setAnchoredProperty(bool value)
{
	setFixed(value, dragging);
}


void Primitive::setFixed(bool newAnchoredProperty, bool newDragging)
{
	bool wasFixed = (anchoredProperty || dragging);
	bool willBeFixed = (newAnchoredProperty || newDragging);

	bool update = (wasFixed != willBeFixed);

	if (update && world) 
	{
		world->onPrimitiveFixedChanging(this);
	}

	anchoredProperty = newAnchoredProperty;
	dragging = newDragging;

	if (update && world) {
		world->onPrimitiveFixedChanged(this);
	}
}



void Primitive::setPreventCollide(bool _preventCollide)
{
	if (_preventCollide != preventCollide) {
		preventCollide = _preventCollide;
		if (world) {
			world->onPrimitivePreventCollideChanged(this);
		}
	}
}

void Primitive::setPartMaterial(PartMaterial _material)
{
	if (material != _material)
	{
		material = _material;
		if (world)
		{
			updateMassValues(world->getUsingNewPhysicalProperties());
			world->onPrimitiveContactParametersChanged(this);
		}
	}
}

void Primitive::setFriction(float _friction)
{
	if (_friction != friction) {
		friction = _friction;
		if (world) {
			world->onPrimitiveContactParametersChanged(this);
		}
	}
}

void Primitive::setPhysicalProperties(const PhysicalProperties& _prop)
{
	if (_prop != customPhysicalProperties)
	{
		customPhysicalProperties = _prop;
		if (world)
		{
			updateMassValues(world->getUsingNewPhysicalProperties());
			world->onPrimitiveContactParametersChanged(this);
		}
	}
}


void Primitive::setElasticity(float _elasticity)
{
	if (_elasticity != elasticity) {
		elasticity = _elasticity;
		if (world) {
			world->onPrimitiveContactParametersChanged(this);
		}
	}
}

CoordinateFrame Primitive::getFaceCoordInObject(NormalId objectFace) const
{
	return CoordinateFrame(	normalIdToMatrix3(objectFace),
							0.5f * normalIdToVector3(objectFace) * geometry->getSize()	);
}

Face Primitive::getFaceInObject(NormalId objectFace) const
{
	return Face::fromExtentsSide(getExtentsLocal(), objectFace);
}

Face Primitive::getFaceInWorld(NormalId objectFace) 
{
	return getFaceInObject(objectFace).toWorldSpace(getBody()->getCoordinateFrame());
}

void Primitive::updateBulletCollisionObject(void)
{
	if (btCollisionObject* object = getGeometry()->getBulletCollisionObject())
		getBody()->updateBulletCollisionObject(object);
}

void Primitive::setPV(const PV& newPv)
{
	const PV& bodyPv = getBody()->getPvUnsafe();

	if (newPv != bodyPv) 
	{
		// when the primitive moves, update it's bullet collision object as well
		updateBulletCollisionObject();

		Assembly* assembly = getAssembly();
		bool assemblyRoot = (assembly && (assembly->getAssemblyPrimitive() == this));
		bool moved = (newPv.position != bodyPv.position);
		
		RBXASSERT((world == NULL) == (assembly == NULL));

		if (assemblyRoot || !assembly) {
			getBody()->setPv(newPv, *this);
		}

		if (!assembly) {
			if (moved) {
				getOwner()->notifyMoved();
			}
		}

		if (assemblyRoot) {
			if (moved) {
                assembly->visitPrimitives(World::OnPrimitiveMovingVisitor(world->getContactManager()));
			}
			if (!requestFixed()) {
				world->ticklePrimitive(this, false);
			}
		}
	}
}

void Primitive::zeroVelocity()
{
	Assembly* assembly = getAssembly();
	bool assemblyRoot = (assembly && (assembly->getAssemblyPrimitive() == this));

	if (assemblyRoot || !assembly) {
		getBody()->setVelocity(Velocity::zero(), *this);
	}
}

void Primitive::setVelocity(const Velocity& vel)
{
	setPV(	PV(getCoordinateFrame(), vel)	);
}

void Primitive::setCoordinateFrame(const CoordinateFrame& cFrame)
{
	setPV(	PV(cFrame, getPV().velocity)	);

	// When Primitive CFrame changes abruptly (e.g. during dragging), Bullet contact cache can become stale
	// We need to invalidate it but keep it in simulation updates. For now we only do it if primitive is being
	// dragged to avoid complicated heuristics.
	if (getDragging())
	{
		for (Contact* c = getFirstContact(); c; c = getNextContact(c))
			c->invalidateContactCache();
	}
}

const PV& Primitive::getPV() const
{
	return getConstBody()->getPvSafe();
}

const CoordinateFrame& Primitive::getCoordinateFrame() const 
{
	return getConstBody()->getPvSafe().position;
}

const CoordinateFrame& Primitive::getCoordinateFrameUnsafe() 
{
	return getBody()->getPvUnsafe().position;
}

void Primitive::setSurfaceData(NormalId id, const SurfaceData& newSurfaceData)
{
	if (!surfaceData)
    {
        if (newSurfaceData.isEmpty())
    		return;
        
        surfaceData = new SurfaceData[6];
	}

	surfaceData[id] = newSurfaceData;
}

void Primitive::setSurfaceType(	NormalId			id, 
								SurfaceType			newSurfaceType)
{
	// Number of joints must be zero for now, because 
	// changing the surface type with a joint in place is not supported
	// In the future allow changing a specific surface by 
	// removing joints from that specific surface, then rejoining
	//

	RBXASSERT_IF_VALIDATING(!hasAutoJoints());

	surfaceType[id] = newSurfaceType;
}

/////////////////////////////////////////////////////
//

SpanningEdge* Primitive::nextSpanningEdgeFromJoint(Joint* j) 
{
	while (j) 
	{
		if (Joint::isSpanningTreeJoint(j)) {
			return j;
		}
		j = getNextJoint(j);
	}
	return NULL;
}


SpanningEdge* Primitive::getFirstSpanningEdge() 
{
	Joint* j = getFirstJoint();
	return nextSpanningEdgeFromJoint(j);
}

SpanningEdge* Primitive::getNextSpanningEdge(SpanningEdge* edge) 
{
	Joint* j = rbx_static_cast<Joint*>(edge);
	return nextSpanningEdgeFromJoint(getNextJoint(j));
}

bool Primitive::isGeometryOrthogonal( void ) const
{
	// Orthogonal shapes have surfaces defined by legacy 6-sides (NORM_X, NORM_Y, etc)
	// Nonorthogonal shapes are generalized polyhedra such as prisms, pyramids, wedges and ramps.
	return geometry->isGeometryOrthogonal();
} 

unsigned int Primitive::getSortSize()
{
	if (sortSize == 0)
		calculateSortSize();

	return sortSize;
}

void Primitive::calculateSortSize()
{
	float planar = getPlanarSize();
	RBXASSERT(planar * 1000.0f + 1 < (float)std::numeric_limits<unsigned int>::max());
	RBXASSERT(planar > 0.0f);

	//unsigned long long int planarInt = Math::iFloor(planar);
	unsigned int planarInt = Math::iFloor(planar * 50.0f);
	unsigned int multiplier = getSizeMultiplier();

	sortSize = planarInt * multiplier + 1; // we reserve 0 for uninitialized parts
}

void Primitive::setSpecificGravity( float value )
{ 
	specificGravity = value; 
	if (world)
		world->onPrimitiveExtentsChanged(this);

	// TODO: If/when specificGravity is associated with mass, then call setMassInertia
}

void Primitive::updateMassValues(bool physicalPropertiesEnabled)
{
	setMassInertia(getCalculateMass(physicalPropertiesEnabled));
}
  
float Primitive::getCalculateMass(bool physicalPropertiesEnabled)
{
	if (physicalPropertiesEnabled)
	{
		return geometry->getVolume() * MaterialProperties::getDensity(this);
	}
	else
	{
		return geometry->getVolume() * 1.0f;
	}
}

bool Primitive::computeIsGrounded() const
{
	if(getDragging())
		return false;

    const Assembly* assembly = Assembly::getConstPrimitiveAssembly(this);
    
    return assembly ? assembly->computeIsGrounded() : false;
}

}// namespace
