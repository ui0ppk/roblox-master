/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/HumanoidState.h"

namespace RBX {

	namespace HUMAN {

	extern const char *const  sSeated;

	class Seated : public Named<HumanoidState, sSeated>
	{
	private:
		/*override*/ StateType getStateType() const {return SEATED;}

		/*override*/ bool armsShouldCollide() const	{return false;}	
		/*override*/ bool legsShouldCollide() const	{return false;}	
		/*override*/ void onComputeForceImpl() {}
		/*override*/ bool enableAutoJump()	const	{ return false; }

	public:
		Seated(Humanoid* humanoid, StateType priorState);
		~Seated();
	};


	extern const char* const sPlatformStanding;

	class PlatformStanding : public Named<HumanoidState, sPlatformStanding>
	{
	private:
		/*override*/ StateType getStateType() const {return PLATFORM_STANDING;}

		/*override*/ bool armsShouldCollide() const	{return false;}	
		/*override*/ bool legsShouldCollide() const	{return false;}	
		/*override*/ void onComputeForceImpl() {}
		/*override*/ bool enableAutoJump()	const	{ return false; }

	public:
		PlatformStanding(Humanoid* humanoid, StateType priorState);
		~PlatformStanding();
	};

	} // namespace
}	// namespace

