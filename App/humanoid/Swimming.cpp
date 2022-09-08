/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Swimming.h"
#include "Humanoid/Humanoid.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "V8DataModel/GameBasicSettings.h"

namespace RBX {

	namespace HUMAN {

		float Swimming::velocityDecay() {return 0.05f;}

		const char* const sSwimming = "Swimming";

		Swimming::Swimming(Humanoid* humanoid, StateType priorState)
			:Named<HumanoidState, sSwimming>(humanoid, priorState)
		{
			humanoid->swimmingSignal( 0.0f );
		}

		void Swimming::onComputeForceImpl()
		{
			float pPitch = 7500.0f;
			float pRoll = 1000.0f;
			float kD = 50.0f;

			// Now move
			Body* torsoBody = getHumanoid()->getTorsoBodyFast();
			if (!torsoBody)
				return;

			Body* root = getHumanoid()->getRootBodyFast();
			if (!root)
				return;

			Vector3 desiredVelocityInWorld = desiredVelocity.linear;
			float desiredVelocityMagnitute = desiredVelocityInWorld.magnitude();
			const Vector3 horizontalVelocity(desiredVelocityInWorld.x, 0.0f, desiredVelocityInWorld.z);
			if ( desiredVelocityInWorld.y * 2.0f < horizontalVelocity.magnitude() &&
				 desiredVelocityInWorld.y * 2.0f > -horizontalVelocity.magnitude() )	// Within +/- 26.5 degree
			{
				desiredVelocityInWorld = horizontalVelocity.direction() * desiredVelocityMagnitute;
			}

			const Vector3& currentVelocityInWorld = root->getBranchVelocity().linear;	// velocity at COFM of the assembly

			// Move forward-backward (and up)
			{
				if( desiredVelocityMagnitute > 10.0f )
				{
					Vector3 desiredAccel = 35.0f * root->getBranchMass() * (desiredVelocityInWorld - currentVelocityInWorld);				
					Vector3 deltaForce = G3D::clamp( desiredAccel, minSwimmingMoveForce(), maxSwimmingMoveForce()	);
					root->accumulateForceAtBranchCofm(deltaForce);
				}
			}

			// control torque for facing direction
			{
				// axis to balance for character pitching
				Vector3 pitchBalanceAxis = torsoBody->getCoordinateFrame().rotation.column(1);
				//Vector3 rollBalanceAxis = torsoBody->getCoordinateFrame().rotation.column(0);
				Vector3 pitchTilt;
				//Vector3 rollTilt = Vector3::unitX().cross( rollBalanceAxis );
				Vector3 angVelWorld = torsoBody->getVelocity().rotational;
				Vector3 externalTorqueWorld = root->getBranchTorque();

				float speedSquared = desiredVelocityInWorld.squaredMagnitude();

				if( speedSquared < 5.0f )
				{
					// if desired velocity is small, balance towards the y-axis
					pPitch = 500.0f;
					pitchTilt = Vector3::unitY().cross( pitchBalanceAxis );	
					pRoll = 0.0f;
				}
				else
				{
					pPitch = 2000.0f;
					// if there's a desired velocity, balance towards the direction of the velocity
					pitchTilt = desiredVelocityInWorld.unit().cross( pitchBalanceAxis );					
				}
				const CoordinateFrame& rootCoord = root->getCoordinateFrame();

				Vector3 angVelRoot = rootCoord.vectorToObjectSpace(angVelWorld);
				RBXASSERT(!Math::isNanInfVector3(angVelRoot));

				// P control component
				Vector3 controlTorqueRoot = -pPitch * (root->getBranchIBody() *  rootCoord.vectorToObjectSpace( pitchTilt ) );  // apply scalar to Vector3, not Matrix3				
				
				// D control component
				controlTorqueRoot -= kD * ( root->getBranchIBodyV3() * angVelRoot );

				// existing torque component
				controlTorqueRoot += rootCoord.vectorToObjectSpace( externalTorqueWorld );
								
				Vector3 torqueWorld = rootCoord.vectorToWorldSpace( controlTorqueRoot );
				root->accumulateTorque( torqueWorld - externalTorqueWorld );

				//RBX::StandardOut::singleton()->printf( RBX::MESSAGE_WARNING, "Swimming facing vector (%f, %f, %f)\n", controlTorqueRoot.x, controlTorqueRoot.y, controlTorqueRoot.z );
			}
		}

		void Swimming::onSimulatorStepImpl(float stepDt)
		{
			Velocity desiredWalkVelocity = 	getHumanoid()->calcDesiredWalkVelocity();

			desiredVelocity = desiredWalkVelocity;
			desiredVelocity.linear += initialLinearVelocity;

			if ((desiredVelocity.linear.magnitude() < 0.1)) {
				desiredVelocity = Velocity::zero();		
			}
#if 0		// Swimming direction is now decoupled from torso facing direction
			else {
				if (desiredWalkVelocity.linear.magnitude() > 0.1) {
					float yawAngle = static_cast<float>(Math::radWrap(Math::getHeading(desiredVelocity.linear) - getHumanoid()->getTorsoHeading()));
					float pitchAngle = static_cast<float>(Math::radWrap(Math::getElevation(desiredVelocity.linear.direction()) - getHumanoid()->getTorsoElevation()));
					if(!RBX::GameBasicSettings::singleton().mouseLockedInMouseLockMode())
					{
						if (fabs(yawAngle) > 0.2f)
							desiredVelocity.rotational.y = kTurnSpeed() * Math::polarity(yawAngle);
						else
							desiredVelocity.rotational.y = 0.25f * kTurnSpeed() * Math::polarity(yawAngle) * fabs(yawAngle);
						if (fabs(pitchAngle) > 0.2f)
							desiredVelocity.rotational.x = kTurnSpeed() * Math::polarity(pitchAngle);
						else
							desiredVelocity.rotational.x = 0.25f * kTurnSpeed() * Math::polarity(pitchAngle) * fabs(pitchAngle);
					}
				}
			}
#endif
			initialLinearVelocity *= ( 1 - (velocityDecay() * stepDt / (1.0f/30.0f) ) );
		}

		void Swimming::fireEvents()
		{
			Super::fireEvents();
			
			fireMovementSignal( getHumanoid()->swimmingSignal, getRelativeMovementVelocity().length() );
		}

	} // HUMAN
} // namespace
