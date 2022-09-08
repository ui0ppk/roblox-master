#include "stdafx.h"

#include "V8Kernel/Connector.h"
#include "V8Kernel/Point.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Body.h"
#include "Util/Math.h"

namespace RBX {


bool Connector::computeCanThrottle()
{
	return (getBody(body0)->getCanThrottle() && getBody(body1)->getCanThrottle());
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


void PointToPointBreakConnector::forceToPoints(const G3D::Vector3& force)
{
	point0->accumulateForce(-force);
	point1->accumulateForce(force);
	RBXASSERT_SLOW(force.magnitude() < Math::inf());
}	

Body* PointToPointBreakConnector::getBody(BodyIndex id)
{
	return (id == body0) ? point0->getBody() : point1->getBody();
}


/////////////////////////////////////////////////

RotateConnector::RotateConnector(
	Body*	_b0,
	Body*	_b1,
	const CoordinateFrame& _j0,
	const CoordinateFrame& _j1,
	float	_baseAngle,
	float	kValue,
	float	armLength)
: b0(_b0)
, b1(_b1)
, j0(_j0)
, j1(_j1)
, baseRotation(_baseAngle)
, k(kValue * armLength * armLength)
, currentAngle(0.0f)
, desiredAngle(0.0f)
, increment(0.0f)
, zeroVelocity(false)
{
	reset();
}

void RotateConnector::reset()			// occurs after networking read;
{
	Vector3 tempNormal;
	desiredAngle = computeNormalRotationFromBase(tempNormal);	// reset to match current rotation
}

Body* RotateConnector::getBody(BodyIndex id)
{
	return (id == body0) ? b0 : b1;
}

float RotateConnector::computeNormalRotationFromBase(Vector3& normal)
{
	float angle = computeJointAngle(	b0->getCoordinateFrame(),
										b1->getCoordinateFrame(),
										j0,
										j1,
										normal);

	float answer = angle - baseRotation;
	RBXASSERT_FISHING(fabs(answer) <= Math::twoPif());
	return answer;
}


float RotateConnector::computeNormalRotationFromBaseFast(Vector3& normal)
{
	float angle = computeJointAngle(	b0->getCoordinateFrameFast(),
										b1->getCoordinateFrameFast(),
										j0,
										j1,
										normal);

	float answer = angle - baseRotation;
	RBXASSERT(fabs(answer) <= Math::twoPif());
	return answer;
}

float RotateConnector::computeJointAngle(const CoordinateFrame& b0,
										 const CoordinateFrame& b1,
										 const CoordinateFrame& j0,
										 const CoordinateFrame& j1,
										 Vector3& normal)
{
/*
	CoordinateFrame j0World = b0 * j0;
	CoordinateFrame j1World = b1 * j1;
	CoordinateFrame j1Inj0 = j0World.toObjectSpace(j1World);

	normal =  j0World.rotation.getColumn(2);
	float rot = Math::zAxisAngle(j1Inj0);
*/

	Matrix3 j0World = b0.rotation * j0.rotation;
	Matrix3 j1World = b1.rotation * j1.rotation;
	Matrix3 j1Inj0 = j0World.transpose() * j1World;
	normal =  j0World.column(2);
	float rot = Math::zAxisAngle(j1Inj0);
	return rot;
}


void RotateConnector::setRotationalGoal(float newGoal) 
{
	float normalizedGoal = static_cast<float>(Math::radWrap(newGoal));

	float deltaRotation = Math::deltaRotationClose(normalizedGoal, desiredAngle);

	increment = deltaRotation / Constants::kernelStepsPerWorldStep();
}


void RotateConnector::setVelocityGoal(float velocity) 
{
	// velocity unit:  radian per long ui step
	increment = velocity * Constants::longUiStepsPerSec() * Constants::kernelDt();
	if (velocity == 0.0)
	{
		zeroVelocity = true;
	}
}


void RotateConnector::stepGoals()
{
	if (zeroVelocity)
	{
		zeroVelocity = false;
		desiredAngle = Math::averageRotationClose(currentAngle, desiredAngle);
	}
	desiredAngle += increment;
}	


void RotateConnector::computeForce(bool throttling) 
{
	Vector3 normal;
	currentAngle = computeNormalRotationFromBaseFast(normal);		// between -2pi and 2pi

	stepGoals();		// between -pi and pi

	float deltaRotation = Math::deltaRotationClose(desiredAngle, currentAngle);		// between -pi and pi;

	RBXASSERT(fabs(deltaRotation) <= Math::pif());
	
	float torqueVal = k * deltaRotation;
	Vector3 torque = normal * torqueVal;

	b0->accumulateTorque(-torque);
	b1->accumulateTorque(torque);
}


/////////////////////////////////////////////////

float PointToPointBreakConnector::potentialEnergy() 
{
	Vector3 delta =  point1->getWorldPos() - point0->getWorldPos();
	float length = delta.length();
	return length * length * k * 0.5f;
}



void PointToPointBreakConnector::computeForce(bool throttling) 
{	
	Vector3 force = -k * (point1->getWorldPos() - point0->getWorldPos());
	float mag = Math::taxiCabMagnitude(force);
	forceToPoints(force);

	broken = (mag > breakForce);
}


/////////////////////////////////////////////////

// normal direction is "out" from the body surface
// delta = P1 - P0, where P0 is the body with the normal direction
// 

void NormalBreakConnector::computeForce(bool throttling) 
{	
	Vector3 normal = Math::getWorldNormal(	normalIdBody0, 
											point0->getBody()->getCoordinateFrameFast().rotation);

	Vector3 force = -k * (point1->getWorldPos() - point0->getWorldPos());
	float magApart = -normal.dot(force);
	forceToPoints(force);

	broken = (magApart > breakForce);
}


} // namespace