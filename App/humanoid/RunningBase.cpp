/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/RunningBase.h"
#include "Humanoid/Humanoid.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Filters.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"
#include "v8World/MaterialProperties.h"
#include "v8World/Geometry.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Kernel.h"


LOGVARIABLE(HumanoidFloorProcess, 0)

DYNAMIC_FASTFLAG(HumanoidFloorPVUpdateSignal)
DYNAMIC_FASTFLAGVARIABLE(CheckForHeadHit, false)
DYNAMIC_FASTFLAGVARIABLE(PGSFixGroundSinking, false)
DYNAMIC_FASTFLAGVARIABLE(HumanoidFeetIsPlastic, false);
DYNAMIC_FASTFLAGVARIABLE(FixSlowLadderClimb, false);
DYNAMIC_FASTFLAG(UseTerrainCustomPhysicalProperties)

namespace RBX {
namespace HUMAN {

const float kAltitudeP = 30000.0f;					// units: 1/sec^2      force = kAltitudeP * mass * position
const float kAltitudeD = 1100.0f;				// units: 1/sec        force = kAltitudeD * mass * velocity

RunningBase::RunningBase(Humanoid* humanoid, StateType priorState)
	:Balancing(humanoid, priorState)
	,desiredAltitude(0.0f)
{
	desiredAltitude = std::numeric_limits<float>::infinity();

	fireMovementSignal(getHumanoid()->runningSignal, getRelativeMovementVelocity().xz().length() );

	// see if there's any residual impulse that needs applying
	if (humanoid)
	{
		Vector3 impulse = humanoid->getActivatePhysicsImpulse();
		if (!impulse.isZero())
		{
			if (PartInstance *instance = humanoid->getTorsoFast())
			{
				if (Primitive *prim = instance->getPartPrimitive())
				{
					if (Body *body = prim->getBody())
					{
						body->accumulateImpulseAtBranchCofm(impulse);
						humanoid->setActivatePhysics(false, Vector3::zero());
					}
				}
			}
		}
	}

	RBXASSERT(!humanoid->getTorsoSlow() || humanoid->getTorsoSlow()->getPartPrimitive()->getEngineType() == Primitive::DYNAMICS_ENGINE);
}

RunningBase::RunningBase(Humanoid* humanoid, StateType priorState, const float kP, const float kD)
	:Balancing(humanoid, priorState, kP, kD)
	,desiredAltitude(0.0f)
{
	desiredAltitude = std::numeric_limits<float>::infinity();

	fireMovementSignal(getHumanoid()->runningSignal, getRelativeMovementVelocity().xz().length() );

	RBXASSERT(!humanoid->getTorsoSlow() || humanoid->getTorsoSlow()->getPartPrimitive()->getEngineType() == Primitive::DYNAMICS_ENGINE);
}

void RunningBase::onComputeForceImpl()
{
	Super::onComputeForceImpl();

	// Now move
	Humanoid *humanoid = getHumanoid();
	if (!humanoid)
		return;

	Body* torsoBody = humanoid->getTorsoBodyFast();
	if (!torsoBody)
		return;

	Body* root = humanoid->getRootBodyFast();
	if (!root)
		return;

#if CHARACTER_FORCE_DEBUG
		if (getFloorPrimitive())
		{
			Vector3 force = root->getBranchForce();
			debugRayList.push_back(DebugRay(RbxRay(getFloorTouchInWorld(), (force) / 500.0f ), Color3::green()));
		}
#endif
	Primitive* floor = getFloorPrimitive();  
	if (DFFlag::HumanoidFloorPVUpdateSignal)
	{
		// Helps transition when going from BufferZone to NotSimulating
		getHumanoid()->updateFloorSimPhaseCharVelocity(floor);
	}

	// Rotate with the ground
	{
		float kP;
		if (getHumanoid()->getWorld()->getUsingPGSSolver())
			kP = RunningBase::kTurnPForRotatePGS();
		else
			kP = RunningBase::kTurnP();

		float desiredTorqueY = kP * root->getBranchIBodyV3().y * (floorVelocity.rotational.y + desiredVelocity.rotational.y - root->getVelocity().rotational.y);
		const float torqueMax = 1e5f;			// tunable value - this is how strongly a humanoid can twist around
		desiredTorqueY = G3D::clamp(desiredTorqueY, -torqueMax, torqueMax);
		root->accumulateTorque(Vector3(0.0, desiredTorqueY, 0.0));
	}

	// Pick the smaller of the 2 masses to ensure we don't apply too high a force.
	// Perhaps we could apply a different cap to each body.
	// Perhaps the engine could do something clever like cap total accels and dribble excess accel to the next frame???
	bool solidFloor = true;
	float mass;
	
	if ((getHumanoid()->getWorld()->getUsingPGSSolver()) || 
		floor == NULL || 
		floor->getAnchoredProperty() || 
		(floor->getAssembly() && floor->getAssembly()->getAssemblyState() == Sim::ANCHORED)) {
		mass = root->getBranchMass();
	} else {
		float humanMass = root->getBranchMass();
		float floorMass = floor->getBody()->getRoot()->getBranchMass();
		if (floorMass < humanMass) {
			mass = floorMass;
			solidFloor = false;
		} else {
			mass = humanMass;
		}
	}

	// Maintain height above the floor
	if (floor && desiredAltitude < std::numeric_limits<float>::infinity())
	{
		RBXASSERT(floor->getAssembly()->getAssemblyPrimitive()->getBody() != root);
		float yAccelDesired = (kAltitudeP * (desiredAltitude - torsoBody->getPos().y)) - (kAltitudeD * (root->getVelocity().linear.y - floorVelocity.linear.y));
		if (yAccelDesired > 0.0)	{		// If yAccelDesired <= 0.0 then just free-fall
			const float currentAccelY = root->getBranchForce().y / root->getBranchMass();
			if (yAccelDesired > currentAccelY) {
				if (getHeadClear()) {
					float deltaForce;
					if (getHumanoid()->getWorld()->getUsingPGSSolver())
					{
						float scaleFactor = 0.1f;
						float accelerationDelta = 0.0f;
						if (DFFlag::PGSFixGroundSinking)
						{
							scaleFactor = 1.0f;
							static float desiredScaleFactor = 0.2f;
							accelerationDelta = (yAccelDesired * desiredScaleFactor) - currentAccelY;
						} else {
							accelerationDelta = yAccelDesired - currentAccelY;
						}

						if (solidFloor) {
							deltaForce = mass * accelerationDelta;
							deltaForce = G3D::clamp(deltaForce, -1e7f, 1e7f);
						} else {
							deltaForce = mass * std::min(accelerationDelta, maxMoveForce().y);
							deltaForce = G3D::clamp(deltaForce, -1e5f, 1e5f);
						}
						root->accumulateForceAtBranchCofm(Vector3(0.0, deltaForce * scaleFactor, 0.0));
						floor->getBody()->accumulateForce(Vector3(0.0, -deltaForce * scaleFactor, 0.0), getFloorTouchInWorld());
					} else {
						if (solidFloor) {
							deltaForce = mass * (yAccelDesired - currentAccelY);
							deltaForce = G3D::clamp(deltaForce, -1e7f, 1e7f);
						} else {
							deltaForce = mass * std::min(yAccelDesired - currentAccelY, maxMoveForce().y);
							deltaForce = G3D::clamp(deltaForce, -1e5f, 1e5f);
						}					
						root->accumulateForceAtBranchCofm(Vector3(0.0, deltaForce, 0.0));
   						floor->getBody()->accumulateForce(Vector3(0.0, -deltaForce * 0.5f, 0.0), getFloorTouchInWorld());
					}						
				}
#if CHARACTER_FORCE_DEBUG
				debugRayList.push_back(DebugRay(RbxRay(getFloorTouchInWorld(), Vector3(0.0, deltaForce/100.0f, 0.0)), Color3::red()));
				Vector3 force = root->getBranchForce();
				debugRayList.push_back(DebugRay(RbxRay(getFloorTouchInWorld() + Vector3(3.0f, 0.0f, 0.0f) , (force) / 500.0f ), Color3::yellow()));
#endif
			}
		}
	}

	// Move forward-backward (and up)
	{
		Vector3 desiredVelocityInWorld = floorVelocity.linear + desiredVelocity.linear;
		// Humanoid Network Update requires to ignore floor velocity of part is not simulated.

		const Vector3& currentVelocityInWorld = root->getBranchVelocity().linear;	// velocity at COFM of the assembly
		Vector3 currentAccel;
		Vector3 desiredAccel;
		if (getHumanoid()->getWorld()->getUsingPGSSolver())
		{
			desiredAccel = runningKMovePForPGS() * (desiredVelocityInWorld - currentVelocityInWorld);
			if (DFFlag::FixSlowLadderClimb && getStateType() == CLIMBING)
			{
				// recalculate Accel
				currentAccel = (root->getBranchForce() - root->getRootSimBody()->getWorldGravityForce()) / root->getBranchMass();
			} else {
				currentAccel = root->getBranchForce() / root->getBranchMass();
			}
		}
		else
		{
			currentAccel = root->getBranchForce() / root->getBranchMass();
			desiredAccel = runningKMoveP() * (desiredVelocityInWorld - currentVelocityInWorld);
		}

		if (desiredVelocity.linear.y <= 0.0f) {
			if (!getFacingLadder()) {
				desiredAccel.y = currentAccel.y;
			}
		}

		Vector3 deltaAccel = desiredAccel - currentAccel;
		// clamp Y and XZ independantly
		if (!getFacingLadder()) {
			deltaAccel.y = G3D::clamp(deltaAccel.y, minMoveForce().y, maxMoveForce().y);
		}

		Vector3 horzontalAccel(deltaAccel.x, 0.0f, deltaAccel.z);
		float maxHorizontalForce;
		if (floor != NULL) {
			maxHorizontalForce = maxLinearGroundMoveForce();
		} else {
			maxHorizontalForce = maxLinearMoveForce();
		}
		if (horzontalAccel.squaredMagnitude() > maxHorizontalForce * maxHorizontalForce)
		{
			horzontalAccel = horzontalAccel.directionOrZero() * maxHorizontalForce;
			deltaAccel.x = horzontalAccel.x;
			deltaAccel.z = horzontalAccel.z;
		}
		Vector3 deltaForce = mass * (deltaAccel);

		if (getStateType() == CLIMBING && getHumanoid()->getWorld()->getUsingPGSSolver())
		{
			deltaForce -= root->getRootSimBody()->getWorldGravityForce();
		}


		if (floor) {
			float kFric;
			if (getHumanoid()->getWorld()->getUsingNewPhysicalProperties())
			{
				if (DFFlag::HumanoidFeetIsPlastic)
				{
					if (!DFFlag::UseTerrainCustomPhysicalProperties || !floor->getPhysicalProperties().getCustomEnabled())
						kFric = MaterialProperties::frictionBetweenMaterials(getFloorMaterial(), PLASTIC_MATERIAL);
					else
						kFric = MaterialProperties::frictionBetweenPrimAndMaterial(floor, PLASTIC_MATERIAL);
				}
				else
				{
					kFric = getFloorFrictionProperty(floor);
				}
			}
			else
			{
				kFric =  floor->getFriction();
			}
			if (getHumanoid()->getWorld()->getUsingPGSSolver() || getHumanoid()->getWorld()->getUsingNewPhysicalProperties())	
			{
				deltaForce.x *= kFric;
				deltaForce.z *= kFric;
			} else {
				if ( kFric < .3f) { // omg hax
					kFric *= kFric;
					deltaForce.x *= kFric;
					deltaForce.z *= kFric;
				}
			}
		}

		// To conserve momentum, we apply an equal and opposite force on the "floor"
		root->accumulateForceAtBranchCofm(deltaForce);
		if (getHumanoid()->getWorld()->getUsingPGSSolver()) // For PGS kernel bodies we apply 1/10th the force
		{
			if (floor) 
			{
				if (deltaForce.y > 0.0f) 						
					floor->getBody()->accumulateForce(-deltaForce * 0.1, getFloorTouchInWorld());
				else 
					floor->getBody()->accumulateForce(Vector3::zero(), getFloorTouchInWorld());
			}			
		}
		else
		{
			if (floor) 
			{
				if (deltaForce.y > 0.0f)
					floor->getBody()->accumulateForce(-deltaForce, getFloorTouchInWorld());
				else 
					floor->getBody()->accumulateForce(Vector3::zero(), getFloorTouchInWorld());
			}
		}

		PartInstance *p = PartInstance::fromPrimitive(floor);
		PartInstance *torso = getHumanoid()->getTorsoSlow();
		if (p && torso)
		{
			p->reportTouch(shared_from(torso));
			root->getRootSimBody()->updateIfDirty();
		}
	}
}

void RunningBase::onSimulatorStepImpl(float stepDt)
{
	// TODO: Determine length of legs and height of torso!
	desiredAltitude = getFloorPrimitive() ? getDesiredAltitude() : std::numeric_limits<float>::infinity();
	floorVelocity = getFloorPointVelocity();
	if (getFloorPrimitive() || getStateType() == CLIMBING || getHumanoid()->getPreviousStateType() == CLIMBING)
	{
		desiredVelocity = getHumanoid()->calcDesiredWalkVelocity();
	}

	if (getFacingLadder() && (!DFFlag::FixSlowLadderClimb || desiredVelocity.linear.length() > 0.0f))
	{
		Vector3 forwardDir = getHumanoid()->getTorsoSlow()->getCoordinateFrame().lookVector();
		forwardDir.y = 0;
		forwardDir = forwardDir.unit();
		float moveDir = 1.0f;
		float dot = forwardDir.dot(desiredVelocity.linear.unit());
		if (dot < -0.2f) 
		{
			moveDir = -1.0f;
		}

		if (moveDir > 0.0f || !getFloorPrimitive()) {
			const float speed = desiredVelocity.linear.length();

			if (speed < 0.1f) {
				desiredVelocity.linear.y = 0.01f * moveDir;
			} else {
				desiredVelocity.linear.y = 0.7f * speed * moveDir;
			}
			desiredVelocity.linear.x = 0;
			desiredVelocity.linear.z = 0;
			desiredVelocity.rotational = Vector3::zero();
		}
	}

	if (getFloorPrimitive())
	{
		Vector3 normal = getFloorTouchNormal();
		float dot = normal.dot(Vector3::unitY());
		if (dot < steepSlopeAngle())
		{
			// too steep - move down hill
			Vector3 downhill = -(Vector3::unitY() - (dot * normal));
			if (downhill.unitize() > Math::epsilonf())
			{
				Vector3 inputOntoSurface = desiredVelocity.linear - (desiredVelocity.linear.dot(normal) * normal);
				desiredVelocity.linear = inputOntoSurface + (downhill * getHumanoid()->getWalkSpeed()) - (inputOntoSurface.dot(downhill) * downhill);
			}
		}
	} 
}

void RunningBase::onCFrameChangedFromReflection()
{
	Super::onCFrameChangedFromReflection();

	// TODO: Determine length of legs and height of torso!
	desiredAltitude = getFloorPrimitive() ? getDesiredAltitude() : std::numeric_limits<float>::infinity();
	floorVelocity = getFloorPointVelocity();
	desiredVelocity = getHumanoid()->calcDesiredWalkVelocity();

	if (!desiredVelocity.linear.isZero() && getFacingLadder()) {
		const float speed = desiredVelocity.linear.length();
		desiredVelocity.linear.x = 0;
		desiredVelocity.linear.y = 0.7f * speed;
		desiredVelocity.linear.z = 0;
	}
}


} // namespace HUMAN
} // namespace RBX
