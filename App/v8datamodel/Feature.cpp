/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Feature.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/MotorJoint.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "Reflection/EnumConverter.h"

#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"


namespace RBX {

using namespace Reflection;

const char *const sFeature = "Feature";
const char *const sHole = "Hole";
const char *const sMotorFeature = "MotorFeature";
const char *const sVelocityMotor = "VelocityMotor";

static const RefPropDescriptor<VelocityMotor, Hole> prop_Hole("Hole", category_Data, &VelocityMotor::getHole, &VelocityMotor::setHole);

static const PropDescriptor<VelocityMotor, float> prop_MaxVelocity("MaxVelocity", category_Data, &VelocityMotor::getMaxVelocity, &VelocityMotor::setMaxVelocity);
static const PropDescriptor<VelocityMotor, float> prop_DesiredAngle("DesiredAngle", category_Data, &VelocityMotor::getDesiredAngle, &VelocityMotor::setDesiredAngle);
static const PropDescriptor<VelocityMotor, float> prop_CurrentAngle("CurrentAngle", category_Data, &VelocityMotor::getCurrentAngle, &VelocityMotor::setCurrentAngle);

namespace Reflection {
template<>
EnumDesc<Feature::TopBottom>::EnumDesc()
	:EnumDescriptor("TopBottom")
{
	addPair(Feature::TOP, "Top");
	addPair(Feature::CENTER_TB, "Center");
	addPair(Feature::BOTTOM, "Bottom");
}

template<>
EnumDesc<Feature::LeftRight>::EnumDesc()
	:EnumDescriptor("LeftRight")
{
	addPair(Feature::LEFT, "Left");
	addPair(Feature::CENTER_LR, "Center");
	addPair(Feature::RIGHT, "Right");
}

template<>
EnumDesc<Feature::InOut>::EnumDesc()
	:EnumDescriptor("InOut")
{
	addPair(Feature::EDGE, "Edge");
	addPair(Feature::INSET, "Inset");
	addPair(Feature::CENTER_IO, "Center");
}
}//namespace Reflection

static const EnumPropDescriptor<Feature, Feature::FaceId> prop_FaceId("FaceId", category_Data, &Feature::getFaceId, &Feature::setFaceId);
static const EnumPropDescriptor<Feature, Feature::TopBottom> prop_TopBottom("TopBottom", category_Data, &Feature::getTopBottom, &Feature::setTopBottom);
static const EnumPropDescriptor<Feature, Feature::LeftRight> prop_LeftRight("LeftRight", category_Data, &Feature::getLeftRight, &Feature::setLeftRight);
static const EnumPropDescriptor<Feature, Feature::InOut> prop_InOut("InOut", category_Data, &Feature::getInOut, &Feature::setInOut);


Feature::Feature()
: faceId(NORM_X)
, topBottom(CENTER_TB)
, leftRight(CENTER_LR)
, inOut(CENTER_IO)
{
	setName("Feature");
}

Feature::~Feature()
{
}

bool Feature::getRenderCoord(CoordinateFrame& c) const
{
	if (const PartInstance* part = queryTypedParent<PartInstance>())
	{
		CoordinateFrame local = computeLocalCoordinateFrame();
		c = part->getCoordinateFrame() * local;
		return true;
	}
	else
	{
		return false;
	}
}

void Feature::render3dSelect(Adorn* adorn, SelectState selectState)
{
	CoordinateFrame worldCoord;
	if (getRenderCoord(worldCoord))
	{
		DrawAdorn::cylinder(adorn, worldCoord, 2, 0.3f, 0.6f, Draw::selectColor() );
	}
}


CoordinateFrame Feature::computeLocalCoordinateFrame() const
{
	const PartInstance* part = Instance::fastDynamicCast<PartInstance>(getParent());

	if (!part)
	{
		return CoordinateFrame();
	}
	else
	{
		Extents extents = part->getConstPartPrimitive()->getExtentsLocal();

		Vector3 corner = extents.max();

		Vector3 uv = Math::vector3Abs(objectToUvw(corner, faceId));
		uv.z = 0.0f;

		switch (leftRight)	// U
		{
		case LEFT:			uv.x = -uv.x;		break;
		case CENTER_LR:		uv.x = 0.0f;		break;
		case RIGHT:								break;
		}

		switch (topBottom)	// V
		{
		case TOP:							break;
		case CENTER_TB:		uv.y = 0.0f;	break;
		case BOTTOM:		uv.y = -uv.y;	break;
		}

		switch (inOut)
		{
		case EDGE:				
			break;

		case INSET:
			for (int i = 0; i < 2; ++i)
			{
				uv[i] = Math::sign(uv[i]) * std::max(0.0f, static_cast<float>(fabs(uv[i]) - 1.0f));
			}
			break;

		case CENTER_IO:	
			uv.x = 0.0f;
			uv.y = 0.0f;
			break;
		}

		Vector3 uvInObject = uvwToObject(uv, faceId);

		Vector3 ptInObject = extents.faceCenter(faceId) + uvInObject;

		NormalId rotateId = (getCoordOrientation() == Z_OUT)
								? faceId
								: intToNormalId((faceId + 3) % 6);

		return CoordinateFrame(	normalIdToMatrix3(rotateId), ptInObject);
	}
}


void Feature::setFaceId(FaceId value)
{
	if (faceId != value)
	{
		faceId = value;
		raiseChanged(prop_FaceId);
	}
}


void Feature::setTopBottom(TopBottom value)
{
	if (topBottom != value)
	{
		topBottom = value;
		raiseChanged(prop_TopBottom);
	}
}

void Feature::setLeftRight(LeftRight value)
{
	if (leftRight != value)
	{
		leftRight = value;
		raiseChanged(prop_LeftRight);
	}
}

void Feature::setInOut(InOut value)
{
	if (inOut != value)
	{
		inOut = value;
		raiseChanged(prop_InOut);
	}
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

Hole::Hole()
{
	setName("Hole");
}

void Hole::render3dAdorn(Adorn* adorn)
{
	CoordinateFrame worldCoord;
	if (getRenderCoord(worldCoord))
	{
		DrawAdorn::cylinder(adorn, worldCoord, 2, 0.2f, 0.3f, Color3::black());
	}
}

////////////////////////////////////////////////////////////

MotorFeature::MotorFeature()
{
	setName("MotorFeature");
}

void MotorFeature::render3dAdorn(Adorn* adorn)
{
	CoordinateFrame worldCoord;
	if (getRenderCoord(worldCoord))
	{
		DrawAdorn::cylinder(adorn, worldCoord, 2, 1.0f, 0.3f, Color3::yellow());
	}
}


bool MotorFeature::canJoin(Instance* i0, Instance* i1)
{
	return (	(Instance::fastDynamicCast<Hole>(i0) && Instance::fastDynamicCast<MotorFeature>(i1))
			||	(Instance::fastDynamicCast<Hole>(i1) && Instance::fastDynamicCast<MotorFeature>(i0))
			);
}


void MotorFeature::join(Instance* i0, Instance* i1)
{
	if (canJoin(i0, i1))
	{
		Instance* holeInstance = i0->fastDynamicCast<Hole>() ? i0 : i1;
		Instance* axleInstance = (holeInstance == i0) ? i1 : i0;

		Hole* hole = rbx_static_cast<Hole*>(holeInstance);
		MotorFeature* axle = rbx_static_cast<MotorFeature*>(axleInstance);

		shared_ptr<VelocityMotor> vm = Creatable<Instance>::create<VelocityMotor>();	

		vm->setHole(hole);
		vm->setParent(axle);
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

VelocityMotor::VelocityMotor()
	: DescribedCreatable<VelocityMotor, JointInstance, sVelocityMotor>(new MotorJoint())
{
	setName("VelocityMotor");
}

VelocityMotor::~VelocityMotor()
{
}

MotorJoint*	VelocityMotor::motorJoint()
{
	return rbx_static_cast<MotorJoint*>(joint);
}

const MotorJoint* VelocityMotor::motorJoint() const
{
	return rbx_static_cast<MotorJoint*>(joint);
}

void VelocityMotor::setPart(int i, Feature* feature)
{
	PartInstance* part = feature ? Instance::fastDynamicCast<PartInstance>(feature->getParent()) : NULL;
	Primitive* primitive = part ? part->getPartPrimitive() : NULL;
	CoordinateFrame c = feature ? feature->computeLocalCoordinateFrame() : CoordinateFrame();

	if (motorJoint()->getPrimitive(i) != primitive)
	{
		motorJoint()->setPrimitive(i, primitive);
		if (primitive)
		{
			motorJoint()->setJointCoord(i, c);
		}
	}
}

// This has nothing to do with whether parts are in world

void VelocityMotor::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	setPart(0, Instance::fastDynamicCast<Feature>(getParent()));

	World* oldWorld = motorJoint()->findWorld();
	World* newWorld = Workspace::getWorldIfInWorkspace(this);

	if (oldWorld != newWorld) {
		if (oldWorld) {
			oldWorld->removeJoint(motorJoint());		// is this dangerous?...
			setPart(0, NULL);
			setPart(1, NULL);
			RBXASSERT(!motorJoint()->findWorld());
		}
		if (newWorld) {
			newWorld->insertJoint(motorJoint());
			RBXASSERT(motorJoint()->findWorld());
		}
	}
}


void VelocityMotor::onEvent_HoleAncestorChanged()
{
	RBXASSERT(hole.get());

	setPart(1, hole.get());		// part is set/unset to hole's parent if its a part
}

Hole* VelocityMotor::getHole() const
{
	return hole.get();
}

void VelocityMotor::setHole(Hole* value)
{
	if (value != hole.get())
	{
		hole = shared_from<Hole>(value);
		raiseChanged(prop_Hole);

		setPart(1, hole.get());

		if (hole)
		{
			holeAncestorChanged = hole->ancestryChangedSignal.connect(boost::bind(&VelocityMotor::onEvent_HoleAncestorChanged, this));
		}
		else
		{
			holeAncestorChanged.disconnect();
		}
	}
}

float VelocityMotor::getMaxVelocity() const
{
	return motorJoint()->maxVelocity;
}

void VelocityMotor::setMaxVelocity(float value)
{
	if (value != motorJoint()->maxVelocity)
	{
		motorJoint()->maxVelocity = value;
		raiseChanged(prop_MaxVelocity);
	}
}

float VelocityMotor::getDesiredAngle() const
{
	return motorJoint()->desiredAngle;
}

void VelocityMotor::setDesiredAngle(float value)
{
	if (value != motorJoint()->desiredAngle)
	{
		motorJoint()->desiredAngle = value;
		raiseChanged(prop_DesiredAngle);
	}
}

float VelocityMotor::getCurrentAngle() const
{
	return motorJoint()->getCurrentAngle();
}

void VelocityMotor::setCurrentAngle(float value)
{
	if (value != motorJoint()->getCurrentAngle())
	{
		motorJoint()->setCurrentAngle(value);
		raiseChanged(prop_CurrentAngle);
	}
}


} // namespace