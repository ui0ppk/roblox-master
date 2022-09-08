/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/JointInstance.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/GlueJoint.h"
#include "V8World/WeldJoint.h"
#include "V8World/SnapJoint.h"
#include "V8World/RotateJoint.h"
#include "V8World/MotorJoint.h"
#include "V8World/Motor6DJoint.h"
#include "V8World/Clump.h"
#include "V8Tree/Property.h"
#include "AppDraw/DrawPrimitives.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "v8world/Assembly.h"

LOGGROUP(JointInstanceLifetime)

using namespace RBX;
using namespace Reflection;

REFLECTION_BEGIN();
const RefPropDescriptor<JointInstance, PartInstance> JointInstance::prop_Part0("Part0", category_Data, &JointInstance::getPart0Dangerous, &JointInstance::setPart0);
const RefPropDescriptor<JointInstance, PartInstance> JointInstance::prop_Part1("Part1", category_Data, &JointInstance::getPart1Dangerous, &JointInstance::setPart1);
static const RefPropDescriptor<JointInstance, PartInstance> dep_Part1("part1", category_Data, &JointInstance::getPart1Dangerous, &JointInstance::setPart1, Reflection::PropertyDescriptor::Attributes::deprecated(JointInstance::prop_Part1, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));

static const PropDescriptor<ManualSurfaceJointInstance, int> prop_Surface0("Surface0", category_Data, &ManualSurfaceJointInstance::getSurface0, &ManualSurfaceJointInstance::setSurface0, Reflection::PropertyDescriptor::STREAMING);
static const PropDescriptor<ManualSurfaceJointInstance, int> prop_Surface1("Surface1", category_Data, &ManualSurfaceJointInstance::getSurface1, &ManualSurfaceJointInstance::setSurface1, Reflection::PropertyDescriptor::STREAMING);

static const PropDescriptor<JointInstance, CoordinateFrame> prop_C0("C0", category_Data, &JointInstance::getC0, &JointInstance::setC0);
static const PropDescriptor<JointInstance, CoordinateFrame> prop_C1("C1", category_Data, &JointInstance::getC1, &JointInstance::setC1);

static const PropDescriptor<Glue, Vector3> prop_F0("F0", category_Data, &Glue::getF0, &Glue::setF0);
static const PropDescriptor<Glue, Vector3> prop_F1("F1", category_Data, &Glue::getF1, &Glue::setF1);
static const PropDescriptor<Glue, Vector3> prop_F2("F2", category_Data, &Glue::getF2, &Glue::setF2);
static const PropDescriptor<Glue, Vector3> prop_F3("F3", category_Data, &Glue::getF3, &Glue::setF3);

static const PropDescriptor<DynamicRotate, float/**/> prop_BaseAngle("BaseAngle", category_Data, &DynamicRotate::getBaseAngle, &DynamicRotate::setBaseAngle);

static const PropDescriptor<Motor, float/**/> prop_MaxVelocity("MaxVelocity", category_Data, &Motor::getMaxVelocity, &Motor::setMaxVelocity);
static const PropDescriptor<Motor, float/**/> prop_DesiredAngle("DesiredAngle", category_Data, &Motor::getDesiredAngle, &Motor::setDesiredAngle);
static BoundFuncDesc<Motor, void(float)> func_SetDesiredAngle(&Motor::setDesiredAngleUi, "SetDesiredAngle", "value", Security::None);	// Non replicating local function
static const PropDescriptor<Motor, float/**/> prop_CurrentAngle("CurrentAngle", category_Data, &Motor::getCurrentAngle, &Motor::setCurrentAngleUi, PropertyDescriptor::UI);
REFLECTION_END();


const char *const RBX::sJointInstance = "JointInstance";
const char *const RBX::sSnap = "Snap";
const char *const RBX::sWeld = "Weld";
const char *const RBX::sManualSurfaceJointInstance = "ManualSurfaceJointInstance";
const char *const RBX::sManualWeld = "ManualWeld";
const char *const RBX::sManualGlue = "ManualGlue";
const char *const RBX::sGlue = "Glue";
const char *const RBX::sRotate = "Rotate";
const char *const RBX::sDynamicRotate = "DynamicRotate";
const char *const RBX::sRotateP = "RotateP";
const char *const RBX::sRotateV = "RotateV";
const char *const RBX::sMotor = "Motor";
const char *const RBX::sMotor6D = "Motor6D";


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

JointInstance::JointInstance(Joint* joint) 
	: joint(joint)
{
	FASTLOG2(FLog::JointInstanceLifetime, "Joint Instance created: %p, joint: %p", this, joint);
	joint->setJointOwner(this);

	PartInstance* p0 = PartInstance::fromPrimitive(joint->getPrimitive(0));
	PartInstance* p1 = PartInstance::fromPrimitive(joint->getPrimitive(1));
	part[0] = shared_from<PartInstance>(p0);
	part[1] = shared_from<PartInstance>(p1);
}

// Destruction - always occurs here
JointInstance::~JointInstance()
{
	FASTLOG2(FLog::JointInstanceLifetime, "Joint Instance destroyed: %p, joint: %p", this, joint);
	RBXASSERT(getParent() == NULL);
	RBXASSERT(joint->findWorld() == NULL);

	joint->setJointOwner(NULL);
	joint->setPrimitive(0, NULL);
	joint->setPrimitive(1, NULL);
	delete joint;
	joint = NULL;
}

XmlElement* JointInstance::writeXml(const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole)
{
	PartInstance* const p0 = getPart0();
	PartInstance* const p1 = getPart1();

	bool const s0 = p0 != NULL && isInScope(p0);
	bool const s1 = p1 != NULL && isInScope(p1);

	if (s0 != s1)
	{
		// Don't write if one of the parts isn't being written.
		// If both aren't being written, then write this, because
		// we are presumably writing out just this joint explicitly
		return NULL;	
	}

	return Super::writeXml(isInScope, creatorRole);
}

bool JointInstance::isAligned() const
{
	return joint->isAligned();
}


Joint::JointType JointInstance::getJointType() const
{
	return joint->getJointType();
}

bool JointInstance::shouldRender3dAdorn() const
{
	// We'd like to be able to toggle showSpanningTree in runtime; however, on iPad this costs us ~2% of the frame,
	// since we call render3dAdorn for many joints even when we don't have to.
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
	return PartInstance::showSpanningTree;
#else
	return true;
#endif
}

void JointInstance::render3dAdorn(Adorn* adorn)
{
	RBXASSERT(shouldRender3dAdorn());

    if (PartInstance::showSpanningTree && Joint::isSpanningTreeJoint(joint) &&
		(joint->inSpanningTree() || Joint::isSpringJoint(joint)))			// remove "inSpanningTree" to show all spanning joints
	{
		Primitive* p0 = getPart0() ? getPart0()->getPartPrimitive() : NULL;
		Primitive* p1 = getPart1() ? getPart1()->getPartPrimitive() : NULL;

		Vector3 pt0 = p0 ? p0->getCoordinateFrame().translation : Vector3::zero();
		Vector3 pt1 = p1 ? p1->getCoordinateFrame().translation : Vector3::zero();

        Color3 color = Joint::isSpringJoint(joint) ? ( joint->inSpanningTree() ? Color3::green() : Color3::blue() ) : Color3::cyan();

        CoordinateFrame world;
        DrawAdorn::lineSegmentRelativeToCoord(adorn, world, pt0, pt1, color, 0.08);
	}
}

PartInstance* JointInstance::getPart0Dangerous() const 
{
	return part[0].lock().get();
}

PartInstance* JointInstance::getPart1Dangerous() const 
{
	return part[1].lock().get();
}

PartInstance* JointInstance::getPart0() 
{
	return part[0].lock().get();
}

PartInstance* JointInstance::getPart1() 
{
	return part[1].lock().get();
}

void JointInstance::setPart0(PartInstance* value)
{
	setPart(0, value);
	raiseChanged(prop_Part0);
}

void JointInstance::setPart1(PartInstance* value)
{
	setPart(1, value);
	raiseChanged(prop_Part1);
}


bool JointInstance::askSetParent(const Instance* instance) const 
{
	return (	instance->getDescriptor()==JointsService::classDescriptor()
			||	instance->getDescriptor()==PartInstance::classDescriptor()	);
}

World* JointInstance::computeWorld()
{
	// Case #1 - Joint is in the JointService - parallel to world
	if (getParent() && getParent()->getDescriptor() == JointsService::classDescriptor()) {
		if (Workspace* workspace = ServiceProvider::find<Workspace>(this)) {
			return workspace->getWorld();
		}
		else {
			return NULL;
		}
	}

	// Case #2 - Joint is in Workspace
	return Workspace::getWorldIfInWorkspace(this);
}

bool JointInstance::inEngine()
{
	bool answer = joint->inPipeline();
	RBXASSERT(answer == (computeWorld() != NULL));
	return answer;
}

void JointInstance::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);
	handleWorldChanged();
}
void JointInstance::onCoParentChanged()
{
	handleWorldChanged();
}

void JointInstance::handleWorldChanged()
{
	World* oldWorld = joint->findWorld();
	World* newWorld = computeWorld();

	if (oldWorld != newWorld) {
		// Make sure both PartInstances are alive before we insert Joint into the world
		// If PartInstance died outside of the world, JointInstance won't get notification to destroy the Joint, so need to check it explicitly
		// TODO: In future, we should consider Joints automatically registering inside Primitives even outside of the World
		shared_ptr<PartInstance> lock1(part[0].lock());
		shared_ptr<PartInstance> lock2(part[1].lock());
		if(lock1.get() == NULL)
		{
			FASTLOG1(FLog::JointInstanceLifetime, "JointInstance %p has empty primitive0 on world add", this);
			joint->setPrimitive(0, NULL);
		}
		if(lock2.get() == NULL)
		{
			FASTLOG1(FLog::JointInstanceLifetime, "JointInstance %p has empty primitive1 on world add", this);
			joint->setPrimitive(1, NULL);
		}

		if (oldWorld) {
			oldWorld->removeJoint(joint);		// is this dangerous?...
			RBXASSERT_SLOW(!joint->findWorld());
		}
		if (newWorld) {
			newWorld->insertJoint(joint);
			RBXASSERT_SLOW(joint->findWorld());
		}
	}
}

void JointInstance::setName(const std::string& value)
{
	FASTLOG1(FLog::JointInstanceLifetime, "JointInstance %p named", this);
	FASTLOGS(FLog::JointInstanceLifetime, "Name: %s", value);
	Super::setName(value);
}

void JointInstance::setPart(int i, PartInstance* value)
{
	shared_ptr<PartInstance> lock(part[i].lock());
	if (lock.get() != value)
	{
		FASTLOG3(FLog::JointInstanceLifetime, "Joint instance %p, setting part %u to %p", this, i, value);
		part[i] = shared_from<PartInstance>(value);
		joint->setPrimitive(i, value ? value->getPartPrimitive() : NULL);
	}
}


const CoordinateFrame& JointInstance::getC0() const
{
	return joint->getJointCoord(0);
}

void JointInstance::setC0(const CoordinateFrame& value)
{
	joint->setJointCoord(0, value);
	raiseChanged(prop_C0);
}

const CoordinateFrame& JointInstance::getC1() const
{
	return joint->getJointCoord(1);
}

void JointInstance::setC1(const CoordinateFrame& value)
{
	joint->setJointCoord(1, value);
	raiseChanged(prop_C1);
}


#ifdef RBX_FLYWEIGHT_INSTANCES
static const boost::flyweight<std::string> snapName("Snap");
#else
static const char* snapName("Snap");
#endif


Snap::Snap(Joint* joint)
	:DescribedCreatable<Snap, JointInstance, sSnap>(joint)
{
	this->setName(snapName);
	RBXASSERT(joint->getJointType() == Joint::SNAP_JOINT);
	//RBXASSERT(joint->getJointCoord(0) == CoordinateFrame());		
	RBXASSERT(Math::isAxisAligned(joint->getJointCoord(0).rotation));		
}

Snap::Snap()
	:DescribedCreatable<Snap, JointInstance, sSnap>(new SnapJoint())
{
	this->setName(snapName);
}

#ifdef RBX_FLYWEIGHT_INSTANCES
static const boost::flyweight<std::string> weldName("Weld");
#else
static const char* weldName("Weld");
#endif


Weld::Weld(Joint* joint)
	:DescribedCreatable<Weld, JointInstance, sWeld>(joint)
{
	this->setName(weldName);
	RBXASSERT(joint->getJointType() == Joint::WELD_JOINT);
//	RBXASSERT(joint->getJointCoord(0) == CoordinateFrame());		
	RBXASSERT(Math::isAxisAligned(joint->getJointCoord(0).rotation));
}

Weld::Weld()
	:DescribedCreatable<Weld, JointInstance, sWeld>(new WeldJoint())
{
	this->setName(weldName);
}


const WeldJoint* Weld::weldJointConst() const
{
	return rbx_static_cast<const WeldJoint*>(joint);
}

void Weld::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);
}

ManualSurfaceJointInstance::ManualSurfaceJointInstance(Joint* joint)
	:DescribedNonCreatable<ManualSurfaceJointInstance, JointInstance, sManualSurfaceJointInstance>(joint)
{
}

void ManualSurfaceJointInstance::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

}

void ManualSurfaceJointInstance::setSurface0(int surfId)
{
	Joint::JointType jt = joint ? joint->getJointType() : Joint::NO_JOINT;
	
	if(jt == Joint::MANUAL_WELD_JOINT)
	{
		ManualWeldJoint* mWJ = static_cast<ManualWeldJoint*>(joint);
		
		if(surfId != mWJ->getSurface0())
		{
			mWJ->setSurface0((size_t)surfId);
			raiseChanged(prop_Surface0);
		}
	}
	else if(jt == Joint::MANUAL_GLUE_JOINT)
	{
		ManualGlueJoint* mGJ = static_cast<ManualGlueJoint*>(joint);
		
		if(surfId != mGJ->getSurface0())
		{
			mGJ->setSurface0((size_t)surfId);
			raiseChanged(prop_Surface0);
		}
	}
}

void ManualSurfaceJointInstance::setSurface1(int surfId)
{
	Joint::JointType jt = joint ? joint->getJointType() : Joint::NO_JOINT;
	
	if(jt == Joint::MANUAL_WELD_JOINT)
	{
		ManualWeldJoint* mWJ = static_cast<ManualWeldJoint*>(joint);

		if(surfId != mWJ->getSurface1())
		{
			mWJ->setSurface1((size_t)surfId);
			raiseChanged(prop_Surface1);
		}
	}
	else if(jt == Joint::MANUAL_GLUE_JOINT)
	{
		ManualGlueJoint* mGJ = static_cast<ManualGlueJoint*>(joint);
		
		if(surfId != mGJ->getSurface1())
		{
			mGJ->setSurface1((size_t)surfId);
			raiseChanged(prop_Surface1);
		}
	}
}

int ManualSurfaceJointInstance::getSurface0(void) const
{
	Joint::JointType jt = joint ? joint->getJointType() : Joint::NO_JOINT;
	
	int surfId = -1;
	if(jt == Joint::MANUAL_WELD_JOINT)
	{
		ManualWeldJoint* mWJ = static_cast<ManualWeldJoint*>(joint);
		surfId = (int)mWJ->getSurface0();
	}
	else if(jt == Joint::MANUAL_GLUE_JOINT)
	{
		ManualGlueJoint* mGJ = static_cast<ManualGlueJoint*>(joint);
		surfId = (int)mGJ->getSurface0();
	}

	return surfId;
}

int ManualSurfaceJointInstance::getSurface1(void) const
{
	Joint::JointType jt = joint ? joint->getJointType() : Joint::NO_JOINT;
	
	int surfId = -1;
	if(jt == Joint::MANUAL_WELD_JOINT)
	{
		ManualWeldJoint* mWJ = static_cast<ManualWeldJoint*>(joint);
		surfId = (int)mWJ->getSurface1();
	}
	else if(jt == Joint::MANUAL_GLUE_JOINT)
	{
		ManualGlueJoint* mGJ = static_cast<ManualGlueJoint*>(joint);
		surfId = (int)mGJ->getSurface1();
	}

	return surfId;
}

ManualWeld::ManualWeld(Joint* joint)
	:DescribedCreatable<ManualWeld, ManualSurfaceJointInstance, sManualWeld>(joint)
{
	this->setName("ManualWeld");
	RBXASSERT(joint->getJointType() == Joint::MANUAL_WELD_JOINT);
//	RBXASSERT(joint->getJointCoord(0) == CoordinateFrame());		
	RBXASSERT(Math::isAxisAligned(joint->getJointCoord(0).rotation));
}

ManualWeld::ManualWeld()
	:DescribedCreatable<ManualWeld, ManualSurfaceJointInstance, sManualWeld>(new ManualWeldJoint())
{
	this->setName("ManualWeld");
}

const ManualWeldJoint* ManualWeld::manualWeldJointConst() const
{
	return rbx_static_cast<const ManualWeldJoint*>(joint);
}

void ManualWeld::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	const Primitive* p0 = getPart0() ? getPart0()->getConstPartPrimitive() : NULL;
	const Primitive* p1 = getPart1() ? getPart1()->getConstPartPrimitive() : NULL;

	if (p0 && p1)
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		if(!workspace)
			return;
		ServiceClient< Selection > selection(workspace);
		// Only render the weld joints if one of its parents is selected
		if( selection && (selection->isSelected(getPart0()) || selection->isSelected(getPart1()) || selection->isSelected(this)) )
		{
			const ManualWeldJoint* manWeldJoint = manualWeldJointConst();
			size_t s0 = manWeldJoint->getSurface0();
			size_t s1 = manWeldJoint->getSurface1();

			if(s0 != (size_t)-1 && s1 != (size_t)-1)
			{
				std::vector<Vector3> polygon1;
				for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
				{
					Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
					polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
				}
				
				std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

				DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::white(), 0.1);
			}
		}
	}
}

ManualGlue::ManualGlue(Joint* joint)
	:DescribedCreatable<ManualGlue, ManualSurfaceJointInstance, sManualGlue>(joint)
{
	this->setName("ManualGlue");
	RBXASSERT(joint->getJointType() == Joint::MANUAL_GLUE_JOINT);
}

ManualGlue::ManualGlue()
	:DescribedCreatable<ManualGlue, ManualSurfaceJointInstance, sManualGlue>(new ManualGlueJoint())
{
	this->setName("ManualGlue");
}


const ManualGlueJoint* ManualGlue::manualGlueJointConst() const
{
	return rbx_static_cast<const ManualGlueJoint*>(joint);
}

void ManualGlue::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	const Primitive* p0 = getPart0() ? getPart0()->getConstPartPrimitive() : NULL;
	const Primitive* p1 = getPart1() ? getPart1()->getConstPartPrimitive() : NULL;

	if (p0 && p1)
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		if(!workspace)
			return;
		ServiceClient< Selection > selection(workspace);
		// Only render the Glue joints if one of its parents is selected
		if(selection && (selection->isSelected(getPart0()) || selection->isSelected(getPart1()) || selection->isSelected(this)))
		{
			const ManualGlueJoint* manGlueJoint = manualGlueJointConst();
			size_t s0 = manGlueJoint->getSurface0();
			size_t s1 = manGlueJoint->getSurface1();

			if(s0 != (size_t)-1 && s1 != (size_t)-1)
			{
				std::vector<Vector3> polygon1;
				for( int i = 0; i < p1->getConstGeometry()->getNumVertsInSurface(s1); i++ )
				{
					Vector3 p1VertInWorld = p1->getCoordinateFrame().pointToWorldSpace(p1->getConstGeometry()->getSurfaceVertInBody(s1, i));
					polygon1.push_back(p0->getCoordinateFrame().pointToObjectSpace(p1VertInWorld));
				}
				
				std::vector<Vector3> intersectPolyInP0 = p0->getConstGeometry()->polygonIntersectionWithFace(polygon1, s0);

				DrawAdorn::polygonRelativeToCoord(adorn, p0->getCoordinateFrame(), intersectPolyInP0, Color3::brown(), 0.1);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////

Glue::Glue(Joint* joint)
	:DescribedCreatable<Glue, JointInstance, sGlue>(joint)
{
	RBXASSERT(joint->getJointType() == Joint::GLUE_JOINT);
}

Glue::Glue()
	:DescribedCreatable<Glue, JointInstance, sGlue>(new GlueJoint())
{}


GlueJoint* Glue::glueJoint()
{
	return rbx_static_cast<GlueJoint*>(joint);
}

const GlueJoint* Glue::glueJointConst() const
{
	return rbx_static_cast<const GlueJoint*>(joint);
}


#define CREATE_GLUE_FACE(n) \
const Vector3& Glue::getF##n() const \
{return glueJointConst()->getFacePoint(n);}\
\
void Glue::setF##n(const Vector3& value)\
{glueJoint()->setFacePoint(n, value);	raiseChanged(prop_F##n);}

CREATE_GLUE_FACE(0);
CREATE_GLUE_FACE(1);
CREATE_GLUE_FACE(2);
CREATE_GLUE_FACE(3);

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////



Rotate::Rotate(Joint* joint)
	:DescribedCreatable<Rotate, JointInstance, sRotate>(joint)
{
	RBXASSERT(joint->getJointType() == Joint::ROTATE_JOINT);
}


Rotate::Rotate()
:DescribedCreatable<Rotate, JointInstance, sRotate>(new RotateJoint())
{}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


float DynamicRotate::getBaseAngle() const
{
	const DynamicRotateJoint* dynamicRotateJoint = rbx_static_cast<const DynamicRotateJoint*>(joint);
	return dynamicRotateJoint->getBaseAngle();

}

void DynamicRotate::setBaseAngle(float value)
{
	DynamicRotateJoint* dynamicRotateJoint = rbx_static_cast<DynamicRotateJoint*>(joint);
	dynamicRotateJoint->setBaseAngle(value);
}


DynamicRotate::DynamicRotate()
:DescribedNonCreatable<DynamicRotate, JointInstance, sDynamicRotate>()
{}


DynamicRotate::DynamicRotate(Joint* joint)
	:DescribedNonCreatable<DynamicRotate, JointInstance, sDynamicRotate>(joint)
{}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

RotateP::RotateP(Joint* joint)
	:DescribedCreatable<RotateP, DynamicRotate, sRotateP>(joint)
{
	RBXASSERT(joint->getJointType() == Joint::ROTATE_P_JOINT);
}

RotateP::RotateP()
:DescribedCreatable<RotateP, DynamicRotate, sRotateP>(new RotatePJoint())
{}



RotateV::RotateV(Joint* joint)
	:DescribedCreatable<RotateV, DynamicRotate, sRotateV>(joint)
{
	RBXASSERT(joint->getJointType() == Joint::ROTATE_V_JOINT);
}

RotateV::RotateV()
:DescribedCreatable<RotateV, DynamicRotate, sRotateV>(new RotateVJoint())
{}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

Motor::Motor(Joint* joint)
	:DescribedCreatable<Motor, JointInstance, sMotor>(joint)
{
	RBXASSERT(joint->getJointType() == Joint::MOTOR_1D_JOINT);
	RBXASSERT(0);	// creating a motor by AutoJoin
	this->setName("Motor");
}

Motor::Motor()
	:DescribedCreatable<Motor, JointInstance, sMotor>(new MotorJoint())
{
	this->setName("Motor");
}

// spectial protected constructor for Motor6D
Motor::Motor(Joint* joint, int)
 	:DescribedCreatable<Motor, JointInstance, sMotor>(joint)
{
}


MotorJoint* Motor::getMotorJoint()
{
	return rbx_static_cast<MotorJoint*>(joint);
}

const MotorJoint* Motor::getMotorJoint() const
{
	return rbx_static_cast<MotorJoint*>(joint);
}

float Motor::getMaxVelocity() const
{
	return getMotorJoint()->maxVelocity;
}

void Motor::setMaxVelocity(float value)
{
	if (value != getMotorJoint()->maxVelocity)
	{
		getMotorJoint()->maxVelocity = value;
		raiseChanged(prop_MaxVelocity);
	}
}


float Motor::getDesiredAngle() const
{
	return getMotorJoint()->desiredAngle;
}


void Motor::setDesiredAngle(float value)
{
	if (!Math::fuzzyEq(value, getMotorJoint()->desiredAngle, Math::degreesToRadians(2.0f)))	// debounce to within 2 degrees
	{
		getMotorJoint()->desiredAngle = value;
		raiseChanged(prop_DesiredAngle);
		raiseChanged(prop_CurrentAngle);			// trigger updates on the current angle - only local
	}
}

void Motor::setDesiredAngleUi(float value)
{
	if (!Math::fuzzyEq(value, getMotorJoint()->desiredAngle, Math::degreesToRadians(2.0f)))	// debounce to within 2 degrees
	{
		getMotorJoint()->desiredAngle = value;
		raiseChanged(prop_CurrentAngle);			// trigger updates on the current angle - only local
	}
}

float Motor::getCurrentAngle() const
{
	return getMotorJoint()->getCurrentAngle();
}

void Motor::setCurrentAngleUi(float value)		// note - setting this is a "command" - the property is not replicated or streamed.
{
	if (value != getMotorJoint()->getCurrentAngle())
	{
		getMotorJoint()->setCurrentAngle(value);
		raiseChanged(prop_CurrentAngle);
	}
}

/*implement*/ const std::string& Motor::getParentName()
{
	PartInstance* p = getPart0();
	if(p)
	{
		return p->getName();
	}
	else
	{
		return IAnimatableJoint::sNULL;
	}
}
/*implement*/ const std::string& Motor::getPartName()
{
	PartInstance* p = getPart1();
	if(p)
	{
		return p->getName();
	}
	else
	{
		return IAnimatableJoint::sNULL;
	}
}
/*implement*/ void Motor::applyPose(const CachedPose& pose)
{
	getMotorJoint()->applyPose(pose.rotaxisangle.z, pose.weight, pose.maskWeight);
}

/*implement*/ void Motor::setIsAnimatedJoint(bool val)
{
    isAnimatedJoint = val;
    PartInstance* part = getPart1();
    if(part)
    {
        Primitive* prim = part->getPartPrimitive();
        if (Assembly* a = prim->getAssembly())
        {
            a->setAnimationControlled(val);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

Motor6D::Motor6D(Joint* joint)
	:DescribedCreatable<Motor6D, Motor, sMotor6D>(joint, 1)
{
	RBXASSERT(joint->getJointType() == Joint::MOTOR_6D_JOINT);
	RBXASSERT(0);	// creating a motor by AutoJoin
	this->setName("Motor6D");
}

Motor6D::Motor6D()
	:DescribedCreatable<Motor6D, Motor, sMotor6D>(new Motor6DJoint(), 1)
{
	this->setName("Motor6D");
}

Motor6DJoint* Motor6D::getMotorJoint()
{
	return rbx_static_cast<Motor6DJoint*>(joint);
}

const Motor6DJoint* Motor6D::getMotorJoint() const
{
	return rbx_static_cast<Motor6DJoint*>(joint);
}

float Motor6D::getMaxVelocity() const
{
	return getMotorJoint()->maxZAngleVelocity;
}

void Motor6D::setMaxVelocity(float value)
{
	if (value != getMotorJoint()->maxZAngleVelocity)
	{
		getMotorJoint()->maxZAngleVelocity = value;
		raiseChanged(prop_MaxVelocity);
	}
}


float Motor6D::getDesiredAngle() const
{
	return getMotorJoint()->desiredZAngle;
}


void Motor6D::setDesiredAngle(float value)
{
	if (!Math::fuzzyEq(value, getMotorJoint()->desiredZAngle, Math::degreesToRadians(2.0f)))	// debounce to within 2 degrees
	{
		getMotorJoint()->desiredZAngle = value;
		raiseChanged(prop_DesiredAngle);
		raiseChanged(prop_CurrentAngle);			// trigger updates on the current angle - only local
	}
}

void Motor6D::setDesiredAngleUi(float value)
{
	if (!Math::fuzzyEq(value, getMotorJoint()->desiredZAngle, Math::degreesToRadians(2.0f)))	// debounce to within 2 degrees
	{
		getMotorJoint()->desiredZAngle = value;
		raiseChanged(prop_CurrentAngle);			// trigger updates on the current angle - only local
	}
}

float Motor6D::getCurrentAngle() const
{
	return getMotorJoint()->getCurrentZAngle();
}

void Motor6D::setCurrentAngleUi(float value)		// note - setting this is a "command" - the property is not replicated or streamed.
{
	if (value != getMotorJoint()->getCurrentZAngle())
	{
		getMotorJoint()->setCurrentZAngle(value);
		raiseChanged(prop_CurrentAngle);
	}
}

/*implement*/ void Motor6D::applyPose(const CachedPose& pose)
{
	getMotorJoint()->applyPose(pose.translation, pose.rotaxisangle, pose.weight, pose.maskWeight);
}

