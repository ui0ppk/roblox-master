/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/MovingNoPhysicsBase.h"
#include "Humanoid/Humanoid.h"

#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/PartInstance.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/World.h"
#include "V8World/SimulateStage.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Kernel.h"

DYNAMIC_FASTFLAG(HumanoidFloorPVUpdateSignal)
LOGGROUP(HumanoidFloorProcess)
DYNAMIC_FASTFLAG(HumanoidFeetIsPlastic)
DYNAMIC_FASTFLAG(UseTerrainCustomPhysicalProperties)

namespace RBX {
namespace HUMAN {

const char* const sMovingNoPhysicsBase = "MovingNoPhysicsBase";

// Note - this state object only exists if the humanoid is in the Workspace chain

MovingNoPhysicsBase::MovingNoPhysicsBase(Humanoid* humanoid, StateType priorState)
	:Named<HumanoidState, sMovingNoPhysicsBase>(humanoid, priorState)
{
	torsoPart = shared_from<PartInstance>(humanoid->getTorsoSlow());
	RBXASSERT(!torsoPart.get() || torsoPart->getPartPrimitive()->getEngineType() == Primitive::DYNAMICS_ENGINE);

	if (torsoPart.get())
	{
		torsoPart->getPartPrimitive()->setEngineType(Primitive::HUMANOID_ENGINE);

		// watch for torso being destroyed
		torsoAncestryChanged = torsoPart->ancestryChangedSignal.connect(boost::bind(&MovingNoPhysicsBase::onEvent_TorsoAncestryChanged, this));
	}
}


MovingNoPhysicsBase::~MovingNoPhysicsBase()
{
	disconnectTorso();

	RBXASSERT(!torsoPart.get());
}

// should never be called!
void MovingNoPhysicsBase::onComputeForceImpl()
{
#ifdef __RBX_NOT_RELEASE
	//	Note humanoid is always in kernel!
	if (torsoPart.get()) {
		RBXASSERT(!torsoPart->getPartPrimitive()->inKernel());
	}
#endif
}


void MovingNoPhysicsBase::onEvent_TorsoAncestryChanged()
{
	disconnectTorso();

	RBXASSERT(!torsoPart.get());
}

void MovingNoPhysicsBase::disconnectTorso()
{
	torsoAncestryChanged.disconnect();											// disconnect

	if (torsoPart.get())
	{
		RBXASSERT(torsoPart->getPartPrimitive()->getEngineType() == Primitive::HUMANOID_ENGINE);

		torsoPart->getPartPrimitive()->setEngineType(Primitive::DYNAMICS_ENGINE);

		torsoPart.reset();
	}
}

const Assembly* MovingNoPhysicsBase::getAssemblyConst() const 
{
	if (const Primitive* torso = getHumanoidConst()->getTorsoSlow()->getConstPartPrimitive()) {
		return torso->getConstAssembly();
	}
	return NULL;
}

void MovingNoPhysicsBase::applyImpulseToFloor(float dt)
{
	if (Body* root = getHumanoid()->getRootBodyFast())
	{
		if (Primitive* floor = getFloorPrimitive())
		{
			float massCharacter = root->getBranchMass();
			float massFloor = floor->getBody()->getBranchMass();	// mass of floor rigid clump
			float mass = std::min(massCharacter, massFloor);
			float force = mass * Units::kmsAccelerationToRbx( Constants::getKmsGravity() );		// note kmsGravity.y is negative.
			float impulse = force * dt * 0.5f;		// for now, reduce this
	    	floor->getBody()->accumulateLinearImpulse( Vector3(0.0, impulse, 0.0), getFloorTouchInWorld() );

			PartInstance *p = PartInstance::fromPrimitive(floor);
			PartInstance *torso = getHumanoid()->getTorsoSlow();
			if (p && torso)
				p->reportTouch(shared_from(torso));
		}
	}
}

void MovingNoPhysicsBase::onSimulatorStepImpl(float stepDt)
{
	const float sink = 0.0f;

	if (Primitive* torso = getHumanoid()->getTorsoSlow()->getPartPrimitive()) 
	{
		if (torso->getAssembly()->getAssemblyPrimitive() == torso) 
		{
			RBXASSERT(!(torso->getBody()->getRootSimBody()->isInKernel()));		// make sure engine not simulating with physics
			CoordinateFrame currentPosition = torso->getCoordinateFrame();
			Velocity desiredVelocity = getHumanoid()->calcDesiredWalkVelocity() + getFloorPointVelocity(); 

			Primitive* floor = getFloorPrimitive();  
			if (DFFlag::HumanoidFloorPVUpdateSignal)
			{
				// Helps transition when going from BufferZone to NotSimulating
				getHumanoid()->updateFloorSimPhaseCharVelocity(floor);
			}

			if (floor)
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


			if (floor) {
				float desiredAltitude = getDesiredAltitude() - sink;			// small sink factor to prevent bouncing out of no_phys state
				float altitudeError = (desiredAltitude - currentPosition.translation.y);		// velocity proportional to error
				if (fabs(altitudeError) > (sink / 4)) {
					desiredVelocity.linear.y += altitudeError / stepDt; // * 20.0f;
				}
			}

			PV currentPV = torso->getPV();
			Vector3 XZForce = Vector3(desiredVelocity.linear.x - currentPV.velocity.linear.x, 0.0f,  desiredVelocity.linear.z - currentPV.velocity.linear.z);
			if (getHumanoid()->getWorld()->getUsingPGSSolver())
				XZForce *= runningKMovePForPGS(); // units: playerMass * m / s^2
			else
				XZForce *= runningKMoveP(); // units: playerMass * m / s^2
			float maxHorizontalForce = maxLinearMoveForce(); // units: playerMass * m / s^2
			if (getFloorPrimitive() != NULL) {
				maxHorizontalForce = maxLinearGroundMoveForce();
			}

			if (getHumanoid()->getWorld()->getUsingPGSSolver())	
			{
				if (floor) 
				{
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
						kFric = floor->getFriction();
					}

					Vector3 XZDeltaV = Vector3(desiredVelocity.linear.x - currentPV.velocity.linear.x, 0.0f,  desiredVelocity.linear.z - currentPV.velocity.linear.z) * kFric;
					desiredVelocity.linear.x = currentPV.velocity.linear.x + XZDeltaV.x;
					desiredVelocity.linear.z = currentPV.velocity.linear.z + XZDeltaV.z;
				}
			}

			if (XZForce.squaredMagnitude() > maxHorizontalForce * maxHorizontalForce)
			{
				Vector3 XZDeltaV = XZForce.directionOrZero() * maxHorizontalForce * stepDt; // units: m/s
				if ((XZDeltaV.x >= 0 && XZDeltaV.x  <= desiredVelocity.linear.x - currentPV.velocity.linear.x) ||
					(XZDeltaV.x < 0 && XZDeltaV.x >= desiredVelocity.linear.x - currentPV.velocity.linear.x))
				{
					desiredVelocity.linear.x = currentPV.velocity.linear.x + XZDeltaV.x;
				}
				if ((XZDeltaV.z >= 0 && XZDeltaV.z  <= desiredVelocity.linear.z - currentPV.velocity.linear.z) ||
					(XZDeltaV.z < 0 && XZDeltaV.z >= desiredVelocity.linear.z - currentPV.velocity.linear.z))
				{
					desiredVelocity.linear.z = currentPV.velocity.linear.z + XZDeltaV.z;
				}
			}

			// Calc new position
			currentPosition.translation += (desiredVelocity.linear * stepDt);
			if (desiredVelocity.rotational.y != 0.0)
			{
				Math::rotateAboutYGlobal(currentPosition, desiredVelocity.rotational.y * stepDt);
			}

			torso->setCoordinateFrame(currentPosition);
			torso->setVelocity(Velocity(desiredVelocity));		// big change! - setting rotational velocity

			if (desiredVelocity.linear.magnitude() > 2.0f && floor)
			{
				if(getHumanoid()->getWorld()->getUsingPGSSolver())
					getHumanoid()->getWorld()->ticklePrimitive(floor, true);
				else
					getHumanoid()->getWorld()->ticklePrimitive(floor, false);
			}
				
			applyImpulseToFloor(stepDt);
		}
	}
}

void MovingNoPhysicsBase::fireEvents()
{
	Super::fireEvents();

	fireMovementSignal(getHumanoid()->runningSignal, getRelativeMovementVelocity().xz().length() );
}




} // namespace HUMAN
} // namespace RBX
