/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Humanoid.h"
#include "Humanoid/Balancing.h"
#include "Util/Name.h"

namespace RBX {
	
	namespace HUMAN {

	extern const char* const sGettingUp;

	class GettingUp : public Named<Balancing, sGettingUp>
	{
	protected:
		/*override*/ StateType getStateType() const {return GETTING_UP;}

		/*override*/ bool armsShouldCollide()		const	{return false;}	
		/*override*/ bool legsShouldCollide()		const	{return false;}	
		/*override*/ bool enableAutoJump()	const	{ return false; }

	public:
		GettingUp(Humanoid* humanoid, StateType priorState);
	};

	} // HUMAN
}	// namespace

