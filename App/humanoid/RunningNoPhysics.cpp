/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/RunningNoPhysics.h"
#include "Humanoid/Humanoid.h"

DYNAMIC_FASTFLAG(ClampRunSignalMinSpeed)

namespace RBX {
namespace HUMAN {

const char* const sRunningNoPhysics = "RunningNoPhysics";


RunningNoPhysics::RunningNoPhysics(Humanoid* humanoid, StateType priorState)
	:Named<MovingNoPhysicsBase, sRunningNoPhysics>(humanoid, priorState)
{
	if (DFFlag::ClampRunSignalMinSpeed)
	{
		fireMovementSignal(humanoid->runningSignal, getRelativeMovementVelocity().xz().length() );
	
	}
	else
	{
		humanoid->runningSignal(getRelativeMovementVelocity().xz().length());
	}
}



} // namespace HUMAN
} // namespace RBX