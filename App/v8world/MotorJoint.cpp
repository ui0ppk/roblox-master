 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/MotorJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"
#include "V8Kernel/Body.h"

#include "boost/functional/hash/hash.hpp"

namespace RBX {
		

MotorJoint::MotorJoint()
: maxVelocity(0.0f)
, desiredAngle(0.0f)
, currentAngle(0.0f)
, link(new RevoluteLink())
, poseAngleDelta(0.0f)
, poseMaskWeight(1.0f)
, poseFreshness(0)
{}


MotorJoint::~MotorJoint()
{
	delete link;
}

// TODO: Big hack
// In a reverse polarity situation, where the joint polarity (set by P0, P1)
// is different than the assembly polarity, for now use the JOINT polarity
// to make kinematic adjustments.
// In the case where polarity == -1, we must move the ROOT body to make this adjustment.
// Only do this if the root is not anchored.
//

int MotorJoint::getParentId() const
{
	const Body* b0 = getConstPrimitive(0)->getConstBody();
	const Body* b1 = getConstPrimitive(1)->getConstBody();

	int parentId = (b1->getConstParent() == b0) ? 0 : 1;
	RBXASSERT((parentId == 0) || (b0->getConstParent() == b1));

	return parentId;
}


void MotorJoint::setJointAngle(float value)
{
	RBXASSERT(link);
	
	link->setJointAngle(value);	// Parent id == 0: value
}


Link* MotorJoint::resetLink()
{
	int parentId = getParentId();
	int childId = (parentId == 1) ? 0 : 1;

	link->reset(	getJointCoord(parentId),
					getJointCoord(childId)	);

	// 	link->setJointAngle(currentAngle);
	setJointAngle(currentAngle);

	return link;
}

bool MotorJoint::stepUi(double distributedGameTime)
{
	float maxVel = fabs(maxVelocity);

	float delta = (desiredAngle - currentAngle);

	float scriptedAngle;

	if (fabs(delta) < maxVel) {
		scriptedAngle = desiredAngle;
	}
	else if (delta > 0.0f) {
		scriptedAngle = currentAngle + maxVel;
	}
	else {
		scriptedAngle = currentAngle - maxVel;
	}

	if(poseFreshness > 0) 
	{
		poseFreshness--;
	}
	else if (poseMaskWeight < 1.0f || poseAngleDelta > 0) // pose expired, fade back to normal.
	{
		poseMaskWeight = std::min(1.0f, poseMaskWeight+ (1.0f/poseDuration));
		poseAngleDelta = std::max(0.0f, poseAngleDelta * (1.0f - (1.0f / poseDuration)) - 0.01f);
	}

	return setCurrentAngle(scriptedAngle * poseMaskWeight + poseAngleDelta);
}

CoordinateFrame MotorJoint::getMeInOther(Primitive* me)
{
	CoordinateFrame rotatedCoord0(	Math::rotateAboutZ(jointCoord0.rotation, currentAngle),
									jointCoord0.translation);
	
	CoordinateFrame p1InP0 = rotatedCoord0 * jointCoord1.inverse();

	if (me == getPrimitive(1)) {
		return p1InP0;
	}
	else {
		RBXASSERT(me == getPrimitive(0));
		return p1InP0.inverse();
	}
}


bool MotorJoint::setCurrentAngle(float value)
{
	if (currentAngle != value) {
		currentAngle = value;

		if (World* world = this->findWorld()) {
			setJointAngle(value);

			world->ticklePrimitive(getPrimitive(0), true);
			world->ticklePrimitive(getPrimitive(1), true);
		}
		return true;
	}
	return false;
}

void MotorJoint::applyPose(float poseAngle, float poseWeight, float maskWeight)
{
	// here we are essentially pre-lerping the pose inflence to the joint.
	poseAngleDelta = poseAngle * poseWeight;
	poseMaskWeight = maskWeight;
	poseFreshness = MotorJoint::poseDuration;
}

size_t MotorJoint::hashCode() const
{
	std::size_t seed = boost::hash<G3D::Vector3>()(jointCoord0.translation);
	boost::hash_combine(seed, jointCoord1.translation);
	return seed;
}

bool MotorJoint::isAligned()
{
	CoordinateFrame baseWorld = getJointWorldCoord(0);
	CoordinateFrame rotorWorld = getJointWorldCoord(1);

	return	(	Math::fuzzyEq(baseWorld.translation, rotorWorld.translation)
			&&	Math::fuzzyEq(baseWorld.rotation.column(2), rotorWorld.rotation.column(2))	);
}	


} // namespace
