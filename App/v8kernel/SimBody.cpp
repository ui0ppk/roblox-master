/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/SimBody.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "Util/Units.h"

namespace RBX {

float SimBody::maxTorqueXX = 1e15f;
float SimBody::maxForceXX = 1e15f;
float SimBody::maxLinearImpulseXX = 1e15f;
float SimBody::maxRotationalImpulseXX = 1e15f;

float SimBody::maxDebugTorque() {return maxTorqueXX;}
float SimBody::maxDebugForce() {return maxForceXX;}
float SimBody::maxDebugLinearImpulse() {return maxLinearImpulseXX;}
float SimBody::maxDebugRotationalImpulse() {return maxRotationalImpulseXX;}

SimBody::SimBody(Body* body) 
	: body(body)
	, dt(0.0f)
	, dirty(true)
	, symmetricContact(false)
	, angMomentum(Vector3::zero())
	, moment(Vector3::zero())
	, momentRecip(Vector3::zero())
	, massRecip(0.0f)
	, constantForceY(0.0f)
	, force(Vector3::zero())
	, torque(Vector3::zero())
	, impulse(Vector3::zero())
	, impulseLast(Vector3::zero())
	, rotationalImpulse(Vector3::zero())
	, penetrationTorque(Vector3::zero())
	, freeFallBodyIndex(-1)
	, realTimeBodyIndex(-1)	
	, jointBodyIndex(-1)
	, buoyancyBodyIndex(-1)
	, contactBodyIndex(-1)	
	, numOfConnectors(0)
	, numOfHumanoidConnectors(0)
	, numOfSecondPassConnectors(0)
	, numOfRealTimeConnectors(0)
	, numOfJointConnectors(0)
	, numOfBuoyancyConnectors(0)
	, numOfContactConnectors(0)
{
	momentRecipWorld = Matrix3::zero();
    uid = body->getUID();
}

SimBody::~SimBody()
{
	RBXASSERT(dt == 0.0f);
	RBXASSERT(freeFallBodyIndex == -1);
	RBXASSERT(realTimeBodyIndex == -1);
	RBXASSERT(jointBodyIndex == -1);
	RBXASSERT(contactBodyIndex == -1);
}

PV SimBody::getOwnerPV()
{
	RBXASSERT(!dirty);
	Vector3 cofmInBody = body->getBranchCofmOffset();
	return pv.pvAtLocalOffset(-cofmInBody);
}

void SimBody::update()
{
	RBXASSERT(dirty);

	Vector3 cofmInBody = body->getBranchCofmOffset();
	pv = body->getPvUnsafe().pvAtLocalOffset(cofmInBody);
	qOrientation = Quaternion(pv.position.rotation);	
	qOrientation.normalize();

	massRecip = 1.0f / body->getBranchMass();
	moment = body->getBranchIBodyV3();
	momentRecip = Vector3(1,1,1) / moment;
	updateAngMomentum();
	updateMomentRecipWorld();
	constantForceY = body->getBranchMass() * Units::kmsAccelerationToRbx( Constants::getKmsGravity() );

	RBXASSERT_VERY_FAST(Math::longestVector3Component(force) < 1e20f);
	RBXASSERT_VERY_FAST(constantForceY < 1e20f);
	RBXASSERT_VERY_FAST(body->cofmIsClean());
	RBXASSERT_FISHING(!Math::isNanInfDenormVector3(qOrientation.imag()));

	RBXASSERT(dirty);		// concurrency check

	dirty = false;
}

// iWorldInverse = rot * momentRecip * rot.transpose();

inline void SimBody::updateMomentRecipWorld()
{
	Matrix3 temp;
	Math::mulMatrixDiagVector(pv.position.rotation, momentRecip, temp);
	Math::mulMatrixMatrixTranspose(temp, pv.position.rotation, momentRecipWorld);
}


void SimBody::clearVelocity()
{
	pv.velocity.rotational = Vector3::zero();
	pv.velocity.linear = Vector3::zero();
	body->pv.velocity = pv.velocity;
	body->advanceStateIndex();
}

void SimBody::updateAngMomentum()
{
	Matrix3 temp;
	Matrix3 iWorld;
	Math::mulMatrixDiagVector(pv.position.rotation, moment, temp);
	Math::mulMatrixMatrixTranspose(temp, pv.position.rotation, iWorld);
	angMomentum = iWorld * pv.velocity.rotational;
}

// rotationalVelocity = iWorldInverse * angMomentum;
inline Vector3 SimBody::computeRotationVelocityFromMomentumFast()
{
	return momentRecipWorld * angMomentum;
}

// rotationalVelocity = iWorldInverse * angMomentum;
inline Vector3 SimBody::computeRotationVelocityFromMomentum()
{
	updateMomentRecipWorld();
	return computeRotationVelocityFromMomentumFast();
}

static const Vector3 denormalSmall(1e-20f, 1e-20f, 1e-20f);

void SimBody::step() 
{
	RBXASSERT(!dirty);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	angMomentum *= 0.9998f;			// damping
	angMomentum += (rotationalImpulse + torque * dt);
	pv.velocity.rotational = computeRotationVelocityFromMomentum() + denormalSmall;
	Quaternion qDot(Quaternion(pv.velocity.rotational) * qOrientation * 0.5f);
	qOrientation += qDot * dt;
	qOrientation.normalize();
	qOrientation.toRotationMatrix(pv.position.rotation);				

	pv.velocity.linear += massRecip * (impulse + force * dt);
	pv.position.translation += pv.velocity.linear * dt;

	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	clearForceAccumulators();	// as opposed to reset - this is the fast version
	clearImpulseAccumulators();	// as opposed to reset - this is the fast version

	angMomentum += denormalSmall;
	pv.velocity.linear += denormalSmall;

	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.position.translation));
	RBXASSERT_FISHING(!Math::isNanInfDenormVector3(qOrientation.imag()));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormMatrix3(pv.position.rotation));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.linear));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.rotational));

	body->pv = (body->cofm == NULL) ? pv : getOwnerPV();			// NUKE THIS WHEN ALL SET
	body->advanceStateIndex();
}

void SimBody::stepVelocity() 
{
	RBXASSERT(!dirty);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	angMomentum *= 0.99621f;		// damping  0.9998 ^ 19
	angMomentum += rotationalImpulse + torque * dt;
	pv.velocity.rotational = computeRotationVelocityFromMomentum() + denormalSmall;	
	impulseLast = impulse + force * dt;
	pv.velocity.linear += massRecip * impulseLast;
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	clearForceAccumulators();	// as opposed to reset - this is the fast version
	clearImpulseAccumulators();	// as opposed to reset - this is the fast version

	angMomentum += denormalSmall;
	pv.velocity.linear += denormalSmall;

	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.linear));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.rotational));
	body->pv.velocity = (body->cofm == NULL) ? pv.velocity : getOwnerPV().velocity; // NUKE THIS WHEN ALL SET
	body->advanceStateIndex();
}

void SimBody::applyImpulse(const Vector3& _impulse, const Vector3& worldPos)
{
	RBXASSERT(!dirty);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(force) < 1e19);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	Vector3 localPosWorld = worldPos - pv.position.translation;
	Vector3 _rotationalImpulse = localPosWorld.cross(_impulse);

	angMomentum *= 0.9998f;				// damping
	angMomentum += _rotationalImpulse;
	pv.velocity.rotational = computeRotationVelocityFromMomentumFast() + denormalSmall;	
	pv.velocity.linear += massRecip * _impulse + denormalSmall;

	angMomentum += denormalSmall;

	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.linear));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.velocity.rotational));
	
	body->pv.velocity = (body->cofm == NULL) ? pv.velocity : getOwnerPV().velocity; // NUKE THIS WHEN ALL SET
	body->advanceStateIndex();
}

void SimBody::stepPosition()
{
	RBXASSERT(!dirty);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT_SLOW(Math::longestVector3Component(pv.velocity.linear) < 1e6);

	Quaternion qDot(Quaternion(pv.velocity.rotational) * qOrientation * 0.5f);
	qOrientation += qDot * dt;
	qOrientation.normalize();
	qOrientation.toRotationMatrix(pv.position.rotation);				

	pv.position.translation += pv.velocity.linear * dt;

	RBXASSERT_SLOW(Math::longestVector3Component(pv.position.translation) < 1e6);
	RBXASSERT(!hasExternalForceOrImpulse());

	RBXASSERT_VERY_FAST(!Math::isNanInfDenormVector3(pv.position.translation));
	RBXASSERT_FISHING(!Math::isNanInfDenormVector3(qOrientation.imag()));
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormMatrix3(pv.position.rotation));
	body->pv = (body->cofm == NULL) ? pv : getOwnerPV();			// NUKE THIS WHEN ALL SET
	body->advanceStateIndex();
}


// Optimized free fall integrator that doesn't do rotational velocity update
void SimBody::stepFreeFall() 
{
	RBXASSERT(!hasExternalForceOrImpulse());
	pv.velocity.rotational *= 0.99621f;  /* 0.9998 ^ 19 */
    Quaternion qDot(Quaternion(pv.velocity.rotational) * qOrientation * 0.5f);
    qOrientation += qDot * dt;
    qOrientation.normalize();
    qOrientation.toRotationMatrix(pv.position.rotation);
	RBXASSERT_VERY_FAST(!Math::isNanInfDenormMatrix3(pv.position.rotation));
    
    // Assumption: Gravity acceleration remains constant in the current engine. 
    // In the future if gravity acceleration can change we need to use the actual gravity force here
    // 
    float dyHalf = Units::kmsAccelerationToRbx( Constants::getKmsGravity() ) * dt / 2.0f;
    pv.velocity.linear.y += dyHalf;
    pv.position.translation += pv.velocity.linear * dt;
    pv.velocity.linear.y += dyHalf;

	pv.velocity.rotational += denormalSmall;
	pv.velocity.linear += denormalSmall;

	body->pv = (body->cofm == NULL) ? pv : getOwnerPV();			// NUKE THIS WHEN ALL SET
	body->advanceStateIndex();
}

void SimBody::updateFromSolver( const Vector3& newPosition, const Matrix3& newOrientation, 
                                const Vector3& newLinearVelocity, const Vector3& newAngularVelocity )
{
    pv.position.translation = newPosition;
    pv.position.rotation = newOrientation;
    pv.velocity.linear = newLinearVelocity;
    pv.velocity.rotational = newAngularVelocity;
    qOrientation = Quaternion(pv.position.rotation);

    clearForceAccumulators();
    clearImpulseAccumulators();

    body->pv = (body->cofm == NULL) ? pv : getOwnerPV();
    body->advanceStateIndex();

    dirty = true;
    update();
}

} // namespace
