/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Balancing.h"

#include "Humanoid/Humanoid.h"

#include "V8World/Primitive.h"
#include "V8Kernel/Body.h"
#include "V8World/World.h"

namespace RBX {
namespace HUMAN {

Balancing::Balancing(Humanoid* humanoid, StateType priorState)
	:HumanoidState(humanoid, priorState)
	,kP(2250.0f)
	,kD(50.0f)
	,tick(-1)
	,lastBalanceTorque(0.0f, 0.0f, 0.0f)
{
}


Balancing::Balancing(Humanoid* humanoid, StateType priorState, const float kP, const float kD)
	:HumanoidState(humanoid, priorState)
	,kP(kP)
	,kD(kD)
	,tick(-1)
	,lastBalanceTorque(0.0f, 0.0f, 0.0f)
{
}

int Balancing::balanceRate(double torqueMag)		
{ 
	return (int) (20.0f - (torqueMag / 12000.0f));
	// This formula calculates the number of iterations that the currently calculated torque will be applied without 
	//		recalculating the force.  The number of iterations that the force is applied decreases as the amount of 
	//		force increases, eventually capping at around 240k where it will recalculate the force every iteration.
	//		The constants in the formula were arrived at through tuning to maximize performance and minimize undesired
	//		behavoir
}  

int Balancing::balanceRateForPGS()		
{ 
	return 1;
}  


void Balancing::onComputeForceImpl()
{
	RBXASSERT(getHumanoid()->Connector::isInKernel());

	Body* root = getHumanoid()->getRootBodyFast();
	if (!root) {
		return;
	}

	if (tick > 0) {
		tick--;
		root->accumulateTorque(lastBalanceTorque);

		return;
	}

	Body* torsoBody = getHumanoid()->getTorsoBodyFast();
	if (!torsoBody) {
		return;
	}

	Vector3 yAxisWorld = torsoBody->getCoordinateFrame().rotation.column(1);		// from TORSO
	Vector3 tiltWorld = Vector3::unitY().cross(yAxisWorld);							
	Vector3 angVelWorld = torsoBody->getVelocity().rotational;						// from TORSO
	Vector3 externalTorqueWorld = root->getBranchTorque();							// from ROOT

	const CoordinateFrame& rootCoord = root->getCoordinateFrame();

	Vector3 tiltRoot = rootCoord.vectorToObjectSpace(tiltWorld);
	Vector3 angVelRoot = rootCoord.vectorToObjectSpace(angVelWorld);
	Vector3 externalTorqueRoot = rootCoord.vectorToObjectSpace(externalTorqueWorld);

	RBXASSERT(!Math::isNanInfVector3(angVelRoot));

	// P control system
	Vector3 controlTorqueRoot = -kP * (root->getBranchIBody() * tiltRoot);  // apply scalar to Vector3, not Matrix3
	Vector3 torqueRoot = externalTorqueRoot + controlTorqueRoot;

	Vector3 branchIBody = root->getBranchIBodyV3();
	for (int i = 0; i < 3; i+=2) {
		float max = maxTorqueComponent() * branchIBody[i];
		if (fabs(controlTorqueRoot[i] - externalTorqueRoot[i]) < max) {
			// for low external torque situations, ignore and use desired torque exactly
			torqueRoot[i] = controlTorqueRoot[i];	
		}
	}

	// D control system
	torqueRoot -= kD * (branchIBody * angVelRoot);  // apply scalar to Vector3, not Matrix3

	Vector3 torqueWorld = rootCoord.vectorToWorldSpace(torqueRoot);
	Vector3 addedTorque = torqueWorld - externalTorqueWorld;
	root->accumulateTorque(addedTorque);	
	lastBalanceTorque = addedTorque;
	if (getHumanoid()->getWorld()->getUsingPGSSolver())
		tick = balanceRateForPGS();
	else
		tick = balanceRate(addedTorque.magnitude());

}



} // namespace HUMAN
} // namespace