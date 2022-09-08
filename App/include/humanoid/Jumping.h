/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Flying.h"

namespace RBX {
	namespace HUMAN {

	extern const char* const sJumping;

	class Jumping : public Named<Flying, sJumping>
	{
	private:
		typedef Named<Flying, sJumping> Super;
		/*override*/ StateType getStateType() const {return JUMPING;}

		// Humanoid::State
		/*override*/ void onComputeForceImpl();

		/*override*/ bool armsShouldCollide()		const	{return false;}	
		/*override*/ bool legsShouldCollide()		const	{return false;}	
		/*override*/ bool torsoShouldCollide()		const	{return false;}	

		// Override hitTestFiler
		/*override*/ Result filterResult(const Primitive* testMe) const;

		bool findCeiling();
		shared_ptr<PartInstance> tryCeiling(const RbxRay& ray, float maxDistance, Assembly* humanoidAssembly);

		Vector3 jumpDir;

	public:
		Jumping(Humanoid* humanoid, StateType priorState);

		static float kJumpP()					{return 500.0f;}
		static float kJumpVelocityGrid()		{return 50.0f;}

	};

	} // namespace HUMAN
}	// namespace

