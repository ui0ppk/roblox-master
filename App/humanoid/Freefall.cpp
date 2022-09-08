/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Freefall.h"
#include "Humanoid/RunningBase.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8World/Primitive.h"
#include "V8Kernel/Body.h"
#include "V8World/World.h"

namespace RBX {
namespace HUMAN {

const char* const sFreefall = "Freefall";

float Freefall::characterVelocityInfluence() {return 0.0f;}		// 0 to 1 - part of initial velocity carrying over
float Freefall::floorVelocityInfluence() {return 0.95f;}		// 0 to 1 - part of initial velocity carrying over
float Freefall::velocityDecay() {return 0.05f;}					// 0 to 1 - part of initial velocity lost every 1/30 sec



Freefall::Freefall(Humanoid* humanoid, StateType priorState)
	:Named<Balancing, sFreefall>(humanoid, priorState)
	, headFriction(0.0f)
	, torsoFriction(0.0f)
	, initialized(false)
{
	// Note - skip the properties stuff - don't replicate
	if (PartInstance* head = humanoid->getHeadSlow()) {
		headFriction = head->getPartPrimitive()->getFriction();
		head->getPartPrimitive()->setFriction(0.0f);
	}
	if (PartInstance* torso = humanoid->getTorsoSlow()) {
		torsoFriction = torso->getPartPrimitive()->getFriction();
		torso->getPartPrimitive()->setFriction(0.0f);
	}

	setBalanceP(5000.0f);
}

Freefall::~Freefall()
{
	// Note - skip the properties stuff - don't replicate
	if (PartInstance* head = getHumanoid()->getHeadSlow()) {
		if (headFriction > 0.0f) {
			RBXASSERT(head->getPartPrimitive()->getFriction() == 0.0f);
			head->getPartPrimitive()->setFriction(headFriction);
		}
	}
	if (PartInstance* torso = getHumanoid()->getTorsoSlow()) {
		if (torsoFriction > 0.0f) {
			RBXASSERT(torso->getPartPrimitive()->getFriction() == 0.0f);
			torso->getPartPrimitive()->setFriction(torsoFriction);
		}
	}
}

float Freefall::kTurnSpeed() {
	return  6.0f;
}	// note Humanoid autoTurnSpeed is 8.0f;

float Freefall::kTurnSpeedForPGS() {
	return  8.0f;
}


void Freefall::onSimulatorStepImpl(const float stepDt)
{
//	if (!initialized) {
//		if (PartInstance* torso = getHumanoid()->getTorsoSlow()) {
//			initialLinearVelocity =		characterVelocityInfluence() * torso->getLinearVelocity() 
//									+ 	floorVelocityInfluence() * getFloorPointVelocity().linear;
//		}
//		initialized = true;
//	}

	Velocity desiredWalkVelocity = 	getHumanoid()->calcDesiredWalkVelocity();

	desiredVelocity = desiredWalkVelocity;
	desiredVelocity.linear += initialLinearVelocity;

	if ((desiredVelocity.linear.magnitude() < 0.1)) {
		desiredVelocity = Velocity::zero();		
	}
	else {
		if (desiredWalkVelocity.linear.magnitude() > 0.1) {
			float angleError = static_cast<float>(Math::radWrap(Math::getHeading(desiredVelocity.linear) - getHumanoid()->getTorsoHeading()));
			if(!RBX::GameBasicSettings::singleton().mouseLockedInMouseLockMode())
			{
				if (getHumanoid()->getAutoRotate()) 
				{
					if (getHumanoid()->getWorld()->getUsingPGSSolver())
					{
						if (fabs(angleError) > 0.2f)
							desiredVelocity.rotational.y = kTurnSpeedForPGS() * Math::polarity(angleError);
						else
							desiredVelocity.rotational.y = 0.25f * kTurnSpeedForPGS() * Math::polarity(angleError) * fabs(angleError);
					}
					else
					{
						if (fabs(angleError) > 0.2f)
							desiredVelocity.rotational.y = kTurnSpeed() * Math::polarity(angleError);
						else
							desiredVelocity.rotational.y = 0.25f * kTurnSpeed() * Math::polarity(angleError) * fabs(angleError);
					}
				}
			}
		}
	}

	initialLinearVelocity *= (		1 - (velocityDecay() * stepDt / (1.0f/30.0f))		);	// decay away - tuned at 30 fps
}

void Freefall::onComputeForceImpl()
{
	Super::onComputeForceImpl();

	// Now move
	Body* root = getHumanoid()->getRootBodyFast();

	// Rotate around Y
	if (root)
	{
		float desiredTorqueY;
		if (!getHumanoid()->getWorld()->getUsingPGSSolver())
		{
			desiredTorqueY =  RunningBase::kTurnP() * root->getBranchIBodyV3().y * (desiredVelocity.rotational.y - root->getVelocity().rotational.y);
		}
		else
		{
			desiredTorqueY =  RunningBase::kTurnPForFreeFallPGS() * root->getBranchIBodyV3().y * (desiredVelocity.rotational.y - root->getVelocity().rotational.y);
		}

		// overcome existing torque:
		const float torqueMax = root->getBranchIBodyV3().y * kTurnAccelMax();
		desiredTorqueY -= root->getBranchTorque().y;
		desiredTorqueY = G3D::clamp(desiredTorqueY, -torqueMax, torqueMax);
		root->accumulateTorque(Vector3(0.0, desiredTorqueY, 0.0));

		// Move forward-backward (and up)
		Vector3 desiredAccel;
		if (getHumanoid()->getWorld()->getUsingPGSSolver())
			desiredAccel = runningKMovePForPGS() * (desiredVelocity.linear - root->getBranchVelocity().linear);
		else
			desiredAccel = runningKMoveP() * (desiredVelocity.linear - root->getBranchVelocity().linear);

		// clamp Y and XZ independantly
		desiredAccel.y = G3D::clamp(desiredAccel.y, minMoveForce().y, maxMoveForce().y);
		Vector3 horzontalAccel(desiredAccel.x, 0.0f, desiredAccel.z);
		float maxHorizontalForce = maxLinearMoveForce();
		if (horzontalAccel.squaredMagnitude() > maxHorizontalForce * maxHorizontalForce)
		{
			horzontalAccel = horzontalAccel.directionOrZero() * maxHorizontalForce;
			desiredAccel.x = horzontalAccel.x;
			desiredAccel.z = horzontalAccel.z;
		}

		Vector3 deltaForce = root->getBranchMass() * (desiredAccel);

		deltaForce.y = 0.0f;

		root->accumulateForceAtBranchCofm(deltaForce);
	}
}



	} // namespace HUMAN

} // namespace
