#pragma once

#include "Util/SteppedInstance.h"
#include "V8World/KernelJoint.h"
#include "Util/RunStateOwner.h"
#include "solver/Constraint.h"

namespace RBX
{
	class PartInstance;
	class World;

	void registerBodyMovers();

	extern const char* const sBodyMover;
	// Base class for Instances that move a body using Controller::computeForce()
	class BodyMover : public DescribedNonCreatable<BodyMover, Instance, sBodyMover>
					, public KernelJoint					// Implements "computeForce"
	{
	private:
		typedef Instance Super;
		Vector3 lastWakeForce;	// last values latched to wake
		Vector3 lastWakeTorque;

		void computeForce(bool throttling, Body* &root, Vector3& force, Vector3& torque);	// internal version

		// Connector
		/*override*/ void computeForce(bool throttling);

		// Joint
		/*override*/ bool canStepWorld() const {return true;}

	protected:
		// KernelJoint
		/*override*/ Body* getEngineBody();

        /*override*/ void stepWorld();

	private:
		// Instance
		/*override*/ void onAncestorChanged(const AncestorChanged& event);

		virtual bool duplicateBodyMoverExists(Primitive* p0, Primitive* p1);

	protected:
		World* world;
		weak_ptr<PartInstance> part;				// The Body this controls

		/*implement*/ virtual void computeForceImpl(bool throttling, 
													Body* body, 
													Body* root, 
													Vector3& force, 
													Vector3& torque) = 0;

		BodyMover(const char* name);

		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void removeFromKernel();

	public:
		~BodyMover();
		bool askSetParent(const Instance* instance) const;	
	};

	// A "gyroscope" used to control the motion of a PartInstance
	extern const char* const sBodyGyro;
	class BodyGyro 
		: public DescribedCreatable<BodyGyro, BodyMover, sBodyGyro>
	{
		float kP;                  // units: 1/sec^2      torque = kP * momentOfInertia * rotation
		float kD;                  // units: 1/sec        torque = kD * momentOfInertia * rotVelocity
		Vector3 maxTorque;    // units: 1/sec^2??    torque <= maxTorqueComponent * momentOfInertia
		CoordinateFrame cframe; // The desired orientation (translation is ignored)
        MovingRegression instabilityDetectorX;
        MovingRegression instabilityDetectorY;
        MovingRegression instabilityDetectorZ;

	private:
		ConstraintBodyAngularVelocity* angularVelocityConstraint;

		Vector3 computeBalanceTorque(Body* body, Body* root);
		Vector3 computeOrientationTorque(Body* body, Body* root);

		/*override*/ void computeForceImpl(	
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);

		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void removeFromKernel();
		void update(void);

		void onPDChanged(const Reflection::PropertyDescriptor&);

        /*override*/ void stepWorld();

	public:
		BodyGyro(void);
		~BodyGyro(void);
		
		static Reflection::BoundProp<float> prop_kP;
		static Reflection::BoundProp<float> prop_kD;
		static Reflection::PropDescriptor<BodyGyro, G3D::Vector3> prop_maxTorque;
		static Reflection::PropDescriptor<BodyGyro, G3D::Vector3> prop_maxTorqueDeprecated;
		static Reflection::PropDescriptor<BodyGyro, CoordinateFrame> prop_cframe;
		static Reflection::PropDescriptor<BodyGyro, CoordinateFrame> prop_cframeDeprecated;

		Vector3 getMaxTorque() const { return maxTorque; }
		void setMaxTorque(Vector3 value);
		CoordinateFrame getCFrame() const { return cframe; }
		void setCFrame(CoordinateFrame value);
	};


	// A constant force at COM (expressed in world coordinates)
	extern const char* const sBodyForce;
	class BodyForce 
		: public DescribedCreatable<BodyForce, BodyMover, sBodyForce>
	{
	private:
		Vector3 bodyForceValue;
	
		/*override */ bool duplicateBodyMoverExists(Primitive*, Primitive*) { return false; }
		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);
	public:
		BodyForce(void);
		
		static Reflection::PropDescriptor<BodyForce, Vector3> prop_Force;
		static Reflection::PropDescriptor<BodyForce, Vector3> prop_ForceDeprecated;

		Vector3 getBodyForce() const { return bodyForceValue; }
		void setBodyForce(Vector3 value);
	};


	// A constant force expressed in body coordinates
	extern const char* const sBodyThrust;
	class BodyThrust 
		: public DescribedCreatable<BodyThrust, BodyMover, sBodyThrust>
	{
	private:
		Vector3 bodyThrustValue;
		Vector3 location;

	protected:
		/*override */ bool duplicateBodyMoverExists(Primitive*, Primitive*) { return false; }
		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);
	public:
		BodyThrust(void);
		
		static Reflection::PropDescriptor<BodyThrust, Vector3> prop_force;
		static Reflection::PropDescriptor<BodyThrust, Vector3> prop_forceDeprecated;
		static Reflection::PropDescriptor<BodyThrust, Vector3> prop_location;
		static Reflection::PropDescriptor<BodyThrust, Vector3> prop_locationDeprecated;

		Vector3 getForce() const { return bodyThrustValue; }
		void setForce(Vector3 value);
		Vector3 getLocation() const { return location; }
		void setLocation(Vector3 value);
	};

	// Attempts to keep a body at a fixed position
	extern const char* const sBodyPosition;
	class BodyPosition 
		: public DescribedCreatable<BodyPosition, BodyMover, sBodyPosition>
		, public IStepped
	{
	private:
        ConstraintLinearSpring* spring;
		typedef DescribedCreatable<BodyPosition, BodyMover, sBodyPosition> Super;

		float kP;                  // units: 1/sec^2      force = kP * mass * position
		float kD;                  // units: 1/sec        force = kD * mass * velocity
		Vector3 maxForce;     //                     force <= maxForce
		Vector3 position;
		Vector3 lastForce;
		bool firedEvent;

		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void removeFromKernel();
		void update(void);

		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			Super::onServiceProvider(oldProvider, newProvider);
			onServiceProviderIStepped(oldProvider, newProvider);
		}

		// IStepped
		/*override*/ void onStepped(const Stepped& event);
		void onPDChanged(const Reflection::PropertyDescriptor&);

        /*override*/ void stepWorld();

	public:
		BodyPosition(void);		
		~BodyPosition(void);
		
		static Reflection::BoundProp<float> prop_kP;
		static Reflection::BoundProp<float> prop_kD;
		static Reflection::PropDescriptor<BodyPosition, Vector3> prop_maxForce;
		static Reflection::PropDescriptor<BodyPosition, Vector3> prop_maxForceDeprecated;
		static Reflection::PropDescriptor<BodyPosition, G3D::Vector3> prop_position;
		static Reflection::PropDescriptor<BodyPosition, G3D::Vector3> prop_positionDeprecated;

		rbx::remote_signal<void()> reachedTargetSignal;

		Vector3 getLastForce() { return lastForce; }
		Vector3 getMaxForce() const { return maxForce; }
		void setMaxForce(Vector3 value);
		Vector3 getPosition() const { return position; }
		void setPosition(Vector3 value);
	};

	// Attempts to keep a body at a fixed velocity
	extern const char* const sBodyVelocity;
	class BodyVelocity 
		: public DescribedCreatable<BodyVelocity, BodyMover, sBodyVelocity>
	{
	private:
		float kP;                  // units: 1/sec        force = kMoveP * mass * velocity
		// TODO: should this be maxAccel?
		Vector3 maxForce;     // units: 1/sec^2      force <= maxForce
		Vector3 velocity;
		Vector3 lastForce;
        ConstraintLinearVelocity* linearVelocity;

		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);

        /*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();
        void update();
		void onPChanged(const Reflection::PropertyDescriptor&);

	public:
		BodyVelocity(void);
		
		static Reflection::BoundProp<float> prop_kP;
		static Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> prop_maxForce;
		static Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> prop_maxForceDeprecated;
		static Reflection::PropDescriptor<BodyVelocity, Vector3> prop_velocity;
		static Reflection::PropDescriptor<BodyVelocity, Vector3> prop_velocityDeprecated;

		Vector3 getLastForce() { return lastForce; }
		Vector3 getMaxForce() const { return maxForce; }
		void setMaxForce(Vector3 value);
		Vector3 getVelocity() const { return velocity; }
		void setVelocity(Vector3 value);
	};

	// Attempts to keep a body at a fixed angular velocity
	extern const char* const sBodyAngularVelocity;
	class BodyAngularVelocity 
		: public DescribedCreatable<BodyAngularVelocity, BodyMover, sBodyAngularVelocity>
	{
	private:
		float kP;              // units: 1/sec        force = kMoveP * mass * velocity
		Vector3 maxTorque;     // units: 1/sec^2      force <= maxForce
		Vector3 angularvelocity;
		Vector3 lastTorque;
        ConstraintLegacyAngularVelocity* angularVelocityConstraint;

        /*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();
        void update(void);
		void onPChanged(const Reflection::PropertyDescriptor&);

	protected:
		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);

	public:
		BodyAngularVelocity(void);
		
		static Reflection::BoundProp<float> prop_kP;
		static Reflection::PropDescriptor<BodyAngularVelocity, Vector3> prop_maxTorque;
		static Reflection::PropDescriptor<BodyAngularVelocity, Vector3> prop_maxTorqueDeprecated;
		static Reflection::PropDescriptor<BodyAngularVelocity, Vector3> prop_angularvelocity;
		static Reflection::PropDescriptor<BodyAngularVelocity, Vector3> prop_angularvelocityDeprecated;

		Vector3 getMaxTorque() const { return maxTorque; }
		void setMaxTorque(Vector3 value);
		Vector3 getAngularVelocity() const { return angularvelocity; }
		void setAngularVelocity(Vector3 value);
	};

	extern const char* const sRocket;
	// Steers a part by rotating it and applying thrust along the Part's -z axis
	class Rocket 
		: public DescribedCreatable<Rocket, BodyMover, sRocket>
		, public IStepped
	{
	private:
		typedef DescribedCreatable<Rocket, BodyMover, sRocket> Super;
		bool active;

		shared_ptr<PartInstance> target;
		Vector3 targetOffset;			// a point in target coordinates
		float targetRadius;
		bool firedEvent;

		// Propulsion
		float maxThrust;
		float kThrustP;                  // units: 1/sec^2      force = kP * mass * position
		float kThrustD;                  // units: 1/sec        force = kD * mass * velocity
		float maxSpeed;

		// Orientation & Roll
		float kTurnP;                  // units: 1/sec^2      torque = kP * momentOfInertia * rotation
		float kTurnD;                  // units: 1/sec        torque = kD * momentOfInertia * rotVelocity
		Vector3 maxTorque;            // units: 1/sec^2??    torque <= maxTorqueComponent * momentOfInertia (Torque is in Part-frame)
		float cartoonFactor;			// 0 - realistic.  1 - cartoony

		Vector3 computeTorque(Body* body, Body* root, const G3D::Vector3& targetDir);

		PartInstance* getTargetDangerous() const { return target.get(); }		// for reflection only
		void setTarget(PartInstance* value);

		void onGoalChanged(const Reflection::PropertyDescriptor&) {
			firedEvent = false;
		}

		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			Super::onServiceProvider(oldProvider, newProvider);
			onServiceProviderIStepped(oldProvider, newProvider);
		}

		// IStepped
		/*override*/ void onStepped(const Stepped& event);

		// Gyro
		/*override*/ void computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque);
	public:
		Rocket(void);
		virtual ~Rocket();
		
		static Reflection::RefPropDescriptor<Rocket, PartInstance> prop_Target;
		static Reflection::BoundProp<Vector3> prop_targetOffset;
		static Reflection::BoundProp<float> prop_targetRadius;

		static Reflection::BoundProp<float> prop_MaxThrust;
		static Reflection::BoundProp<float> prop_ThrustP;
		static Reflection::BoundProp<float> prop_ThrustD;
		static Reflection::BoundProp<float> prop_MaxSpeed;

		//static Reflection::BoundProp<float> prop_RollVelocity;
		static Reflection::BoundProp<Vector3> prop_MaxTorque;
		static Reflection::BoundProp<float> prop_TurnP;
		static Reflection::BoundProp<float> prop_TurnD;
		static Reflection::BoundProp<float> prop_CartoonFactor;

		static Reflection::BoundFuncDesc<Rocket, void()> func_Fire;
		static Reflection::BoundFuncDesc<Rocket, void()> func_Abort;

		rbx::remote_signal<void()> reachedTargetSignal;

		void fire();
		void abort();
	private:
		static Reflection::BoundProp<bool> prop_Active;
	};
}