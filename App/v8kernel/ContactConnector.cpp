#include "stdafx.h"

#include "V8Kernel/ContactConnector.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Body.h"
#include "Util/Math.h"

FASTFLAGVARIABLE( BallBlockNarrowphaseFixEnabled, false )

namespace RBX {
	
int ContactConnector::inContactHit = 0;
int ContactConnector::outOfContactHit = 0;

float ContactConnector::percentActive()
{
	int denom = inContactHit + outOfContactHit;
	float answer = (denom == 0) 
		? -1
		: (float)(inContactHit) / (float)(denom);

	inContactHit = 0;
	outOfContactHit = 0;		// even looking at this function resets it :-)
	return answer;
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////



bool ContactConnector::canThrottle() const
{
	return (geoPair.body0->getCanThrottle() && geoPair.body1->getCanThrottle());
}

float ContactConnector::computeRelativeVelocity(const PairParams &params, Vector3* deltaVnormal, Vector3* perpVel)
{
	// deltaV is body1's linear velocity relative to body0
	Vector3 velocity0 = geoPair.body0->getPvUnsafe().linearVelocityAtPoint(params.position);
	Vector3 velocity1 = geoPair.body1->getPvUnsafe().linearVelocityAtPoint(params.position);
	Vector3 deltaV = velocity1 - velocity0;

	// params.normal points from p0 to p1
	float normalVel = params.normal.dot(deltaV);
	if (deltaVnormal)
		*deltaVnormal = params.normal * normalVel;
	if (perpVel)
		*perpVel =  deltaV - params.normal * normalVel;

	return normalVel;
}

float ContactConnector::computeRelativeVelocity()
{
	updateContactPoint();	// TBD: check if this is absolutely necessary
	// return positive velocity when body1 is approaching bpdy0
	return -computeRelativeVelocity(contactPoint, NULL, NULL);	
}

// Reorder the SimBody(s) so that simBody0 is always in kernel and adjust contact point data accordingly
bool ContactConnector::getReordedSimBody(SimBody*& simBody0, SimBody*& simBody1, Body*& bodyNotInKernel, PairParams& params)
{
	simBody0 = geoPair.body0->getRootSimBody();
	simBody1 = geoPair.body1->getRootSimBody();
	bodyNotInKernel = geoPair.body0;

	RBXASSERT(simBody0 && simBody1);

	if (!simBody0->isInKernel())
	{
		// Make sure simBody0 is always in kernel
		if (!simBody1->isInKernel())
			return false;
		simBody0 = simBody1;
		simBody1 = NULL;
		params.normal *= -1.0f;
	} else if (!simBody1->isInKernel())
	{
		simBody1 = NULL;
		bodyNotInKernel = geoPair.body1;
	}
	return true;
}

bool ContactConnector::getReordedSimBody(SimBody*& simBody0, SimBody*& simBody1, PairParams& params)
{
	Body* dummy = NULL;
	return getReordedSimBody(simBody0, simBody1, dummy, params);
}

// Compute the relative velocities between the two bodies
bool ContactConnector::getSimBodyAndContactVelocity(SimBody*& simBody0, SimBody*& simBody1, PairParams& params,
										 			float& normalVel, Vector3& perpVel)
{


	Body* bodyNotInKernel = NULL;
	if (!getReordedSimBody(simBody0, simBody1, bodyNotInKernel, params))
	{
		outOfContactHit++;	
		return false;
	}

	RBXASSERT(simBody0 || simBody1);
	inContactHit++;

	Vector3 deltaV = -simBody0->getPV().linearVelocityAtPoint(params.position);
	if (simBody1)
		deltaV += simBody1->getPV().linearVelocityAtPoint(params.position);
	else
		deltaV += bodyNotInKernel->getPvUnsafe().linearVelocityAtPoint(params.position);

	normalVel = params.normal.dot(deltaV);  // params.normal points from simBody0 to simBody1
	perpVel =  deltaV - params.normal * normalVel;

	return true;
}


void ContactConnector::computeForce(bool throttling) 
{
	RBXASSERT(!throttling || !canThrottle());

	updateContactPoint();
	const PairParams& params = getContactPoint();

	if (params.length < 0.0) 
	{
		inContactHit++;

		Vector3 deltaVnormal, perpVel;
		float normalVel = computeRelativeVelocity(params, &deltaVnormal, &perpVel);

		frictionOffset += perpVel * Constants::kernelDt();

		float frictionSpringK = contactParams.kSpring * 0.2f;
		float currentForce = frictionOffset.length() * frictionSpringK;
		float maxForce = forceMagLast * contactParams.kFriction;
		if ((currentForce > maxForce) && (currentForce > 1e-8)) {	// dynamic friction here
			frictionOffset *= (maxForce / currentForce);			// lose energy here!
		}
		// could do every 8th frame
		frictionOffset -= params.normal * (params.normal.dot(frictionOffset));	// remove no-planar component

		threshold = (threshold != 0.0f)
						?	(-overlapGoal() + ((threshold + overlapGoal()) * 0.999f))	
						:	(firstApproach == 0.0f) ? 0.0f : params.length;

		firstApproach = (firstApproach != 0.0f)
							? (-overlapGoal() + ((firstApproach + overlapGoal()) * 0.999f))
							: params.length;

			// normal force stuff
		float kApplied = (normalVel < 0) ? contactParams.kSpring  : contactParams.kNeg;			// different contact on rebound
		forceMagLast = kApplied * (firstApproach- params.length);	// should be zero first time through
		forceMagLast = (params.length <= threshold) ? forceMagLast : 0.0f;

		Vector3 force = (forceMagLast * params.normal) - (frictionOffset * frictionSpringK);

		// force to bodies
		geoPair.body0->accumulateForce(-force, params.position);
		geoPair.body1->accumulateForce(force, params.position);
		RBXASSERT(fabs(force.x) < Math::inf());
	}
	else {
		outOfContactHit++;
		reset();
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//	IMPULSE SOLVER
//
//	Invoked from the kernel step in the following sequence
//
//	stepVelocity()
//	for (i == 0; i < nIteration; ++i)
//	{
//		for each contact connector
//		{
//			computeImpulse()
//			stepImpulse()
//		}
//	}
//	stepPosition()
//
/////////////////////////////////////////////////////////////////////////////////////////
static float REST_THRESHOLD = 1.0f;
static float PENETRATION_BIAS_WEIGHT = 0.1f;
static float FRICTION_GAIN = 1.2f;
static float SYM_CONTACT_MAX_TANGENT_VEL = 1.0f;
static float SYM_CONTACT_MAX_NORMAL_VEL = 60.0f;

bool ContactConnector::computeImpulse(float& residualVelocity) 
{
	RBXASSERT(geoPair.body0->getRootSimBody()->isInKernel() || geoPair.body1->getRootSimBody()->isInKernel());

	// params.normal points from contact point of body0 to outside body0
	// params.length points from contact point of body0 to outside body0

	PairParams params = getContactPoint();

	if (params.length >= 0.0f)	// body0 and body1 are not inter-penetrating
	{
		outOfContactHit++;
		reset();
		return false;
	}

	SimBody *simBody0, *simBody1;
	float normalVel;
	Vector3 perpVel;

	if (!getSimBodyAndContactVelocity(simBody0, simBody1, params, normalVel, perpVel))
		return false;

	bool symetricContact = true;
	bool verticalSymetricContact = true;
	float normalVelSize = fabs(normalVel);
	// only update sym state in the first iteration
	bool velocityOutOfRange = (normalVelSize > SYM_CONTACT_MAX_NORMAL_VEL || perpVel.squaredMagnitude() > SYM_CONTACT_MAX_TANGENT_VEL);

	if (simBody0->isSymmetricContact())
	{
		if (velocityOutOfRange)
		{
			simBody0->clearSymmetricContact();
			symetricContact = verticalSymetricContact = false;
		} else if (!simBody0->isVerticalContact())
			verticalSymetricContact = false;
	} else
		symetricContact = verticalSymetricContact = false;

	if (simBody1)
	{
		if (simBody1->isSymmetricContact())
		{
			if (velocityOutOfRange)
			{
				simBody1->clearSymmetricContact();
				symetricContact = verticalSymetricContact = false;
			} else if (!simBody1->isVerticalContact())
				verticalSymetricContact = false;
		} else
			symetricContact = verticalSymetricContact = false;
	}

	if ( normalVel >= 0.0f && ( !symetricContact || age == 0 ) )		// body0 and body1 are separating
		return false;

	if (!impulseComputed)
	{
		age++;
		// In world space:
		// skew = toSkewSymmetic(relativeContactPosition)
		//
		// a cross b = skew * b
		// b cross a = -a cross b = -skew * b
		// deltaVel = (-skew  * inverseInertia * skew + diag(massRecip) ) * Impulse;

		inverseMass = simBody0->getMassRecip();
		Matrix3 contactPositionSkew = Math::toSkewSymmetric(params.position - simBody0->getPV().position.translation);
		deltaVelPerUnitImpulse = -contactPositionSkew * simBody0->getInverseInertiaInWorld() * contactPositionSkew;

		if (simBody1)
		{
			inverseMass += simBody1->getMassRecip();
			contactPositionSkew = Math::toSkewSymmetric(params.position - simBody1->getPV().position.translation);
			deltaVelPerUnitImpulse -= contactPositionSkew * simBody1->getInverseInertiaInWorld() * contactPositionSkew;
		}

		deltaVelPerUnitImpulse += Math::fromDiagonal(Vector3(inverseMass, inverseMass, inverseMass));

		bool revertable = deltaVelPerUnitImpulse.inverse(impulsePerUnitDeltaVel, 1e-20);
		RBXASSERT(revertable);
	}

	float desiredDeltaVelocity = -normalVel;	// Cancel the entrance relative velocity

	float velocityThisFrame = simBody0->getMassRecip() * simBody0->getImpulseLast().dot(params.normal);
	if (simBody1)
		velocityThisFrame -= simBody1->getMassRecip() * simBody1->getImpulseLast().dot(params.normal);

	if (!impulseComputed)
	{
		reboundVelocity = 0.0f;
		if (-normalVel >= REST_THRESHOLD && !isRestingContact() && !symetricContact &&
			velocityThisFrame >= 0.0f && velocityThisFrame <= -normalVel)
		{
			// Cancel the velocity entering this frame and the velocity just accumulated in this frame
			// Rebound only the velocity entering this frame
			float entranceVelocity = normalVel + velocityThisFrame;
			RBXASSERT(entranceVelocity <= 0.0f);
			// only rebounce the entrance velocity
			reboundVelocity = -contactParams.kNeg / contactParams.kSpring * entranceVelocity;
			RBXASSERT(reboundVelocity >= 0.0f);
		}

		float penetrationBias = std::max(0.0f, -params.length - overlapGoal() * 2.0f) / Constants::freeFallDt() * PENETRATION_BIAS_WEIGHT;
		if (reboundVelocity < penetrationBias)
			reboundVelocity = penetrationBias;	

		penetrationVelocity = std::max(1.0f, normalVelSize);
		impulseComputed = true;
	}
	desiredDeltaVelocity += reboundVelocity;

	residualVelocity += fabs(desiredDeltaVelocity / penetrationVelocity);

	Vector3 velocityToKill = desiredDeltaVelocity * params.normal - perpVel;
	Vector3 impulse = impulsePerUnitDeltaVel * velocityToKill;

	if (!verticalSymetricContact)
	{
	    // Check static friction
	    float impulseNormalSize = impulse.dot(params.normal);
        Vector3 impulseTangent = impulse - impulseNormalSize * params.normal;
	    velocityThisFrame = fabs(velocityThisFrame);
	    impulseNormalSize = fabs(impulseNormalSize);
        float impulseThisFrame = velocityThisFrame / inverseMass;
	    float impulseTangentMax = std::max(impulseNormalSize, impulseThisFrame) * contactParams.kFriction;
	    if (impulseTangent.squaredMagnitude() > impulseTangentMax * impulseTangentMax)
	    {
			// Switch to the dynamic friction
			// deltaVel dot Normal = (-skew  * inverseInertia * skew + diag(massRecip) ) * 
		    //						 ImpulseNormalSize * (Normal + kFriction * Tangent) dot Normal
		    Vector3 impulseDirection = params.normal + contactParams.kFriction * FRICTION_GAIN * impulseTangent.direction();
		    Vector3 VelDelta = deltaVelPerUnitImpulse * impulseDirection;
		    if (desiredDeltaVelocity > 0)
			    desiredDeltaVelocity = std::max(desiredDeltaVelocity, velocityThisFrame);
		    else
			    desiredDeltaVelocity = std::min(desiredDeltaVelocity, -velocityThisFrame);
		    impulse = desiredDeltaVelocity / VelDelta.dot(params.normal) * impulseDirection;
	    }	
	}

	simBody0->applyImpulse(-impulse, params.position);

	if (simBody1)
		simBody1->applyImpulse(impulse, params.position);

	return true;
}

void ContactConnector::applyContactPointForSymmetryDetection(SimBody* simBody0, SimBody* simBody1, const PairParams& params, float direction)
{
    float massRecipSum = simBody0->getMassRecip();
    if (simBody1)
	    massRecipSum += simBody1->getMassRecip();
    Vector3 penetrationForce = params.normal * params.length;   // force applied on body0 by body1
    
    float weight = simBody0->getMassRecip() / massRecipSum;
    Vector3 force = penetrationForce * weight;
    if (force.squaredMagnitude() > 1e-12)
	    simBody0->accumulatePenetrationForce(force * direction, params.position);
    
    if (simBody1)
    {
	    weight = simBody1->getMassRecip() / massRecipSum;
	    force = -penetrationForce * weight;
	    if (force.squaredMagnitude() > 1e-12)
		    simBody1->accumulatePenetrationForce(force * direction, params.position);
    }
}

void ContactConnector::updateContactPoint()
{
	if (!isContact())
		return;

    if (contactPoint == oldContactPoint)
	    return;
    
    SimBody* simBody0 = NULL;
    SimBody* simBody1 = NULL;
    PairParams params = oldContactPoint;
    
    if (getReordedSimBody(simBody0, simBody1, params))
	    applyContactPointForSymmetryDetection(simBody0, simBody1, params, -1.0f);
    
    params = contactPoint;
    if (getReordedSimBody(simBody0, simBody1, params))
	    applyContactPointForSymmetryDetection(simBody0, simBody1, params, 1.0f);
 	oldContactPoint = contactPoint;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void BallBallConnector::updateContactPoint()
{
	const Vector3& p0 = geoPair.body0->getPosFast();
	contactPoint.normal = geoPair.body1->getPosFast() - p0;

	contactPoint.length = contactPoint.normal.unitize() - radiusSum;
	contactPoint.position = p0 + radius0 * contactPoint.normal;
	ContactConnector::updateContactPoint();
}		

//////////////////////////////////////////////////////////////////////////////////////////

void BallBlockConnector::updateContactPoint()
{
	if (geoPairType == BALL_PLANE_PAIR) {
		computeBallPlane(contactPoint);
	}
	else if (geoPairType == BALL_EDGE_PAIR) {
		computeBallEdge(contactPoint);
	}
	else {
		computeBallPoint(contactPoint);
	}
	ContactConnector::updateContactPoint();
}

void BallBlockConnector::computeBallPlane(PairParams& params) 
{
	const Vector3& p0 = geoPair.body0->getPosFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();
	Vector3 p1 = c1.pointToWorldSpace(offset1);

	params.normal = -Math::getWorldNormal(normalId1, c1);

	Vector3 delta = p1 - p0;			// vector from p1 to p0, edge point to center of ball
	Vector3 centerToPlanePoint = params.normal * params.normal.dot(delta);
    if( FFlag::BallBlockNarrowphaseFixEnabled )
    {
        params.position = p0 + radius0 * params.normal;
    }
    else
    {
        params.position = p0 + centerToPlanePoint;
    }

	params.length = centerToPlanePoint.length() - radius0;
}

void BallBlockConnector::computeBallEdge(PairParams& params) 
{
	const Vector3& p0 = geoPair.body0->getPosFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Vector3 p1 = c1.pointToWorldSpace(offset1);

	Vector3 edgeNormal = Math::getWorldNormal(normalId1, c1);
	Vector3 delta = p1 - p0;	// vector from p0 to p1, center of ball to edge point
	Vector3 projection = edgeNormal * edgeNormal.dot(delta);
    if( FFlag::BallBlockNarrowphaseFixEnabled )
    {
        params.normal = delta - projection;			// right now normal is not unitized
        params.length = params.normal.unitize() - radius0;
        params.position = p0 + radius0 * params.normal;
    }
    else
    {
        params.normal = delta - projection;			// right now normal is not unitized
        params.position = p0 + params.normal;
        params.length = params.normal.unitize() - radius0;
    }
}

void BallBlockConnector::computeBallPoint(PairParams& params) 
{
	const Vector3& pCenter = geoPair.body0->getPosFast();
	params.position = geoPair.body1->getCoordinateFrameFast().pointToWorldSpace(offset1);
	params.normal = params.position - pCenter;
	params.length = params.normal.unitize() - radius0;
    if( FFlag::BallBlockNarrowphaseFixEnabled )
    {
        params.position = params.position - params.length * params.normal;
    }
}


} // namespace
