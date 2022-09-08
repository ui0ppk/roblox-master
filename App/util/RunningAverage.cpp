/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/RunningAverage.h"
#include "Util/Math.h"



namespace RBX {

// i.e., .75*prev + .25*new
float RunningAverageState::weight() 
{
	return 0.75f;
}

void RunningAverageState::reset(const CoordinateFrame& cofm)
{
	position = cofm.translation;
	angles = Quaternion(cofm.rotation);
}

void RunningAverageState::update(const CoordinateFrame& cofm, float radius)
{
	float weightNew = 1.0f - weight();

	position = weight() * position + weightNew * cofm.translation;

	angles =	(angles * weight()) 
				+ (Quaternion(cofm.rotation) * (weightNew * radius));
}


bool RunningAverageState::withinTolerance(const CoordinateFrame& cofm, float radius, float tolerance)
{
	Vector3 deltaPos = Math::vector3Abs(position - cofm.translation);
	float maxPos = Math::maxAxisLength(deltaPos);
	if (maxPos > tolerance) {
		return false;
	}

	Quaternion deltaAngles = angles - Quaternion(cofm.rotation) * radius;
	float maxAngle = deltaAngles.maxComponent();
	if (maxAngle > tolerance) {
		return false;
	}

	return true;
}

}	// namespace
