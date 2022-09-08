/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Motor6DJoint.h"
#include "V8World/MotorJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"
#include "V8Kernel/Body.h"

#include "boost/functional/hash/hash.hpp"

namespace RBX {
		

Motor6DJoint::Motor6DJoint()
: maxZAngleVelocity(0.0f)
, desiredZAngle(0.0f)
, link(new D6Link())
, poseMaskWeight(1.0f)
, poseFreshness(0)
{}


Motor6DJoint::~Motor6DJoint()
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

int Motor6DJoint::getParentId() const
{
	const Body* b0 = getConstPrimitive(0)->getConstBody();
	const Body* b1 = getConstPrimitive(1)->getConstBody();

	int parentId = (b1->getConstParent() == b0) ? 0 : 1;
	RBXASSERT((parentId == 0) || (b0->getConstParent() == b1));

	return parentId;
}


void Motor6DJoint::setJointOffsetCFrame(const Vector3 offset, const Vector3 axisAngle)
{
	RBXASSERT(link);

	Vector3 axis = axisAngle;
	float angle = axis.unitize();
	link->setJointOffsetCFrame(CoordinateFrame(Matrix3::fromAxisAngleFast(axis, angle), offset));	// Parent id == 0: value
}


Link* Motor6DJoint::resetLink()
{
	int parentId = getParentId();
	int childId = (parentId == 1) ? 0 : 1;

	link->reset(	getJointCoord(parentId),
					getJointCoord(childId)	);

	setJointOffsetCFrame(currentOffset, currentAxisAngle);

	return link;
}

float Motor6DJoint::getCurrentZAngle() const
{
	return currentAxisAngle.z; // todo: should re renormalize?
}

bool Motor6DJoint::stepUi(double distributedGameTime)
{
	float maxVel = fabs(maxZAngleVelocity);

	float currentZAngle = getCurrentZAngle();
	float delta = (desiredZAngle - currentZAngle);

	float scriptedZAngle;

	if (fabs(delta) < maxVel) {
		scriptedZAngle = desiredZAngle;
	}
	else if (delta > 0.0f) {
		scriptedZAngle = currentZAngle + maxVel;
	}
	else {
		scriptedZAngle = currentZAngle - maxVel;
	}

	if(poseFreshness > 0) 
	{
		poseFreshness--;
	}
	else if (poseMaskWeight < 1.0f || !poseAxisAngleDelta.isZero() || !poseOffsetDelta.isZero()) // pose expired, fade back to normal.
	{
		poseMaskWeight = std::min(1.0f, poseMaskWeight+ (1.0f/MotorJoint::poseDuration));
		
		// shorten vectors.
		float sqangle = poseAxisAngleDelta.squaredMagnitude();
		if(sqangle != 0)
		{
			float newsqangle = std::max(0.0f, sqangle * (1.0f - (1.0f / MotorJoint::poseDuration)) - 0.01f);
			poseAxisAngleDelta *= (newsqangle/sqangle);
		}

		float sqmagnitude = poseOffsetDelta.squaredMagnitude();
		if(sqmagnitude != 0)
		{
			float newsqmagnitude = std::max(0.0f, sqmagnitude * (1.0f - (1.0f / MotorJoint::poseDuration)) - 0.01f);
			poseOffsetDelta *= (newsqmagnitude/sqmagnitude);
		}
	}

	return setCurrentOffsetAngle(poseOffsetDelta, Vector3(0, 0, scriptedZAngle* poseMaskWeight ) + poseAxisAngleDelta);
}

void Motor6DJoint::setCurrentZAngle(float value)
{
	setCurrentOffsetAngle(poseOffsetDelta, Vector3(0, 0, value* poseMaskWeight ) + poseAxisAngleDelta);
}


CoordinateFrame Motor6DJoint::getMeInOther(Primitive* me)
{
	CoordinateFrame p1InP0 = link->getChildInParent();

	if (me == getPrimitive(1)) {
		return p1InP0;
	}
	else {
		RBXASSERT(me == getPrimitive(0));
		return p1InP0.inverse();
	}
}

bool Motor6DJoint::setCurrentOffsetAngle(const Vector3 offset, const Vector3 axisAngle)
{
	if (currentOffset != offset || currentAxisAngle != axisAngle) {
		currentOffset = offset;
		currentAxisAngle = axisAngle;

//		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "angle: %f", currentAxisAngle.z);
		if (World* world = this->findWorld()) {
			
			setJointOffsetCFrame(currentOffset, axisAngle);
			world->ticklePrimitive(getPrimitive(0), true);
			world->ticklePrimitive(getPrimitive(1), true);
		}
		return true;
	} else
		return false;
}

void Motor6DJoint::applyPose(const Vector3& poseOffset, const Vector3& poseAxisAngle, float poseWeight, float maskWeight)
{
	// here we are essentially pre-lerping the pose inflence to the joint.
	poseOffsetDelta = poseOffset * poseWeight;
	poseAxisAngleDelta = poseAxisAngle * poseWeight;
	poseMaskWeight = maskWeight;
	poseFreshness = MotorJoint::poseDuration;
}

size_t Motor6DJoint::hashCode() const
{
	std::size_t seed = boost::hash<G3D::Vector3>()(jointCoord0.translation);
	boost::hash_combine(seed, jointCoord1.translation);
	return seed;
}

bool Motor6DJoint::isAligned()
{
	CoordinateFrame baseWorld = getJointWorldCoord(0);
	CoordinateFrame rotorWorld = getJointWorldCoord(1);

	return	(	Math::fuzzyEq(baseWorld.translation, rotorWorld.translation)
			&&	Math::fuzzyEq(baseWorld.rotation.column(2), rotorWorld.rotation.column(2))	);
}	


} // namespace