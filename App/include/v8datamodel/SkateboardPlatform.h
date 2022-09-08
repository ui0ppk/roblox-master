/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/Platform.h"
#include "reflection/reflection.h"

namespace RBX {

class Humanoid;
class SkateboardController;
class RotateJoint;

extern const char* const sSkateboardPlatform;


class SkateboardPlatform :	public DescribedCreatable<SkateboardPlatform, PlatformImpl<BasicPartInstance>, sSkateboardPlatform>
						  , public KernelJoint
{
public:
		enum MoveState {	// ground states:
				STOPPED = 0, 
				COASTING, 
				PUSHING,
				STOPPING,
				// air states:
				AIR_FREE } ;

		static bool isGroundState(MoveState state) { return state <= STOPPING; }
		static bool isAirState(MoveState state) { return state >= AIR_FREE; }

private:	
		typedef DescribedCreatable<SkateboardPlatform, PlatformImpl<BasicPartInstance>, sSkateboardPlatform> Super;

		// internal state variables
		float turnRate;			// between -1 and 1

		// only for controller/platform interface
		int throttle;			// -1, 0, 1 back, forward
		int steer;				// -1, 0, 1	left to right
		Vector3 gyroTarget;
		float gyroConvergeSpeed;
		MoveState moveState;
		Vector3 specificImpulseAccumulator;

		bool stickyWheels; // apply downforce to stabilize board.

		World* world;

		// cached on "stepUi", used in ApplyForces
		Motor6D* motor6D;
		Humanoid* humanoid;
		int numWheelsGrounded;

		// accumulated on ApplyForces, meant for use on StepUI
		float forwardVelocity;

		struct Wheel
		{
			Wheel() : joint(0), grounded(0) {};

			RotateJoint* joint;
			bool grounded;
		};
		shared_ptr<SkateboardController> myController;

		G3D::Array<Wheel> wheels;

		void doTurn(float yRotVelocity);
		void doPush(float forwardVelocity);
		void doStop(float forwardVelocity);
		void applyForwardForce(float force);

		float getGroundSpeed();

		void doLoadWheels(Primitive* primitive);
		void loadWheels();
		bool isFullyGrounded();
		void countGroundedWheels();


		//////////////////////////////////////////////////////////////////////////
		// Instance
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		//////////////////////////////////////////////////////////////////////////
		// Seat
		/*override*/ void onPlatformStandingChanged(bool platformStanding, Humanoid* humanoid);

		///////////////////////////////////////////////////////////////////////////
		// IAdornable
		/*override*/ bool shouldRender2d() const;
		/*override*/ void render2d(Adorn* adorn);

		///////////////////////////////////////////////////////////////////////////
		// KernelJoint / Connector
		/*override*/ void computeForce(bool throttling);
		/*override*/ Body* getEngineBody();
		/*override*/ bool canStepUi() const						{return true;}
		/*override*/ bool stepUi(double distributedGameTime);

		///////////////////////////////////////////////////////////////////////////
		// CameraSubject
		/*override*/ void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives);

		void onLocalPlatformStanding(Humanoid* humanoid);
		void onLocalNotPlatformStanding(Humanoid* humanoid);

		float maxPushVelocity;
		float maxPushForce;
		float stopForceMultiplier;

		float maxTurnVelocity;
		float maxTurnTorque;
		float turnTorqueGain;		// if deltaV == 1, apply maxTorque essentially


		static void delayedReparentToWorkspace(weak_ptr<ModelInstance> Board, weak_ptr<ModelInstance> Figure);
public:
		SkateboardPlatform();								
		virtual ~SkateboardPlatform();								

		void setThrottle(int value);
		int getThrottle() const {return throttle;}

		void setSteer(int value);
		int getSteer() const {return steer;}

		void setMoveState(const MoveState& value);
		MoveState getMoveState() const {return moveState;}

		void setStickyWheels(bool value);
		bool getStickyWheels() const { return stickyWheels; } 

		rbx::signal<void(MoveState,MoveState)> moveStateChangedSignal; // void(newState, oldState)

		// this is better:
		rbx::signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> equippedSignal; // void(humanoid, controller)
		rbx::signal<void(shared_ptr<Instance>)> unequippedSignal; //void(humanoid)

		rbx::remote_signal<void(shared_ptr<Instance>)> createPlatformMotor6DSignal;
		rbx::remote_signal<void()> destroyPlatformMotor6DSignal;

		SkateboardController* getController() const { return myController.get(); }		
		Humanoid* getControllingHumanoid() const { return humanoid; }
		
		void applySpecificImpulse(Vector3 impulseWorld);
		void applySpecificImpulse(Vector3 impulseWorld, Vector3 worldpos);

		static Reflection::BoundProp<float> prop_MaxPushVelocity;
		static Reflection::BoundProp<float> prop_MaxPushForce;
		static Reflection::BoundProp<float> prop_StopForceMultiplier;
		static Reflection::BoundProp<float> prop_MaxTurnVelocity;
		static Reflection::BoundProp<float> prop_MaxTurnTorque;
		static Reflection::BoundProp<float> prop_TurnTorqueGain;

		///////////////////////////////////////////////////////////////////////////
		// PlatformImpl
		/*override*/ void createPlatformMotor6D(Humanoid *h);
		/*override*/ void findAndDestroyPlatformMotor6D();
};


} // namespace
