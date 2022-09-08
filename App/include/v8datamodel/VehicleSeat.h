/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Seat.h"

namespace RBX {

class RotateJoint;
class RotateVJoint;
class Humanoid;
class VehicleController;

extern const char* const sVehicleSeat;

class VehicleSeat : public DescribedCreatable<VehicleSeat, SeatImpl<PartInstance>, sVehicleSeat>
				  , public KernelJoint
{
private:
		typedef DescribedCreatable<VehicleSeat, SeatImpl<PartInstance>, sVehicleSeat> Super;

		// saves
		float maxSpeed;
		float turnSpeed;
		float torque;			// dominates low-end startup performance
		bool  enableHud;

		// only for controller/seat interface
		int throttle;			// -1, 0, 1 back, forward
		int steer;				// -1, 0, 1	left to right

		World* world;
	
		G3D::Array<RotateJoint*> hinges;
		G3D::Array<bool> onRights;
		G3D::Array<bool> axlePointingIns;
		G3D::Array<float> axleVelocities;

		weak_ptr<VehicleController> myController;

		int computeNumHinges();
		void doLoadHinges(Primitive* primitive);
		void loadMotorsAndHinges();
		void getJointInfo(RotateJoint* rotateJoint, bool& aligned, bool& onRight, bool& axlePointingIn);
		void stepHinges();
		int lookupFunction(int throttle, int steer, bool onRight, bool overMaxSpeed, bool overTurnSpeed, float jointVelocity);
		

		//////////////////////////////////////////////////////////////////////////
		// Instance
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		//////////////////////////////////////////////////////////////////////////
		// Seat
		/*override*/ void onSeatedChanged(bool seated, Humanoid* humanoid);
		/*override*/ void createSeatWeld(Humanoid *h);
		/*override*/ void findAndDestroySeatWeld();
		/*override*/ void setOccupant(Humanoid* value);

		///////////////////////////////////////////////////////////////////////////
		// KernelJoint / Connector
		/*override*/ void computeForce(bool throttling);
		/*override*/ Body* getEngineBody();
		/*override*/ bool canStepUi() const						{return true;}
		/*override*/ bool stepUi(double distributedGameTime);

		///////////////////////////////////////////////////////////////////////////
		// IAdornable
		/*override*/ bool shouldRender2d() const;
		/*override*/ void render2d(Adorn* adorn);

		///////////////////////////////////////////////////////////////////////////
		// CameraSubject
		/*override*/ void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives);

		void onLocalSeated(Humanoid* humanoid);
		void onLocalUnseated(Humanoid* humanoid);

public:
		rbx::remote_signal<void(shared_ptr<Instance>)> createSeatWeldSignal;
		rbx::remote_signal<void()> destroySeatWeldSignal;

		VehicleSeat();								
		virtual ~VehicleSeat();								

		int getNumHinges() const;

		void setThrottle(int value);
		int getThrottle() const {return throttle;}

		void setEnableHud(bool value);
		bool getEnableHud() const { return enableHud; }

		void setSteer(int value);
		int getSteer() const {return steer;}

		void setMaxSpeed(float value);
		float getMaxSpeed() const {return maxSpeed;}

		void setTurnSpeed(float value);
		float getTurnSpeed() const {return turnSpeed;}

		void setTorque(float value);
		float getTorque() const {return torque;}

		Humanoid* getLocalHumanoid();
};



} // namespace
