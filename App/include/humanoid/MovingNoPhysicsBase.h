/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/HumanoidState.h"
#include "V8World/Mechanism.h"

namespace RBX {
	class Assembly;
	class PhysicsService;

namespace HUMAN {

	extern const char* const sMovingNoPhysicsBase;

	class MovingNoPhysicsBase 
		: public Named<HumanoidState, sMovingNoPhysicsBase>
	{
	private:
		typedef Named<HumanoidState, sMovingNoPhysicsBase> Super;
		/*override*/ StateType getStateType() const {return RUNNING_NO_PHYS;}
		/*override*/ void fireEvents();

		shared_ptr<PartInstance> torsoPart;
		weak_ptr<PhysicsService> physicsService;

		rbx::signals::scoped_connection torsoAncestryChanged;		
		void onEvent_TorsoAncestryChanged();
		void disconnectTorso();

		const Assembly* getAssemblyConst() const;

		void applyImpulseToFloor(float dt);

	protected:

		// Humanoid::State
		/*override*/ void onSimulatorStepImpl(float stepDt);
		/*override*/ void onComputeForceImpl(); 

		/*override*/ bool armsShouldCollide()		const	{return false;}	
		/*override*/ bool legsShouldCollide()		const	{return false;}	
		/*override*/ bool headTorsoShouldCollide()	const	{return false;}	

	public:
		MovingNoPhysicsBase(Humanoid* humanoid, StateType priorState);
		~MovingNoPhysicsBase();
	};

	} // namespace HUMAN
}	// namespace
