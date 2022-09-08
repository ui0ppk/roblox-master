/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Flying.h"

#include "Humanoid/Humanoid.h"
//#include "V8World/Controller.h"
#include "V8Kernel/Body.h"

namespace RBX {
	
	namespace HUMAN {


const char* const sFlying = "Flying";

Flying::Flying(Humanoid* humanoid, StateType priorState)
	:Named<Balancing, sFlying>(humanoid, priorState)
{
	setBalanceP(5000.0f);
}


void Flying::onSimulatorStepImpl(float stepDt)
{
	// to implement
}

void Flying::onComputeForceImpl()
{
	// to implement
}

	} // HUMAN
} // namespace