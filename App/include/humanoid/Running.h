/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/RunningBase.h"

namespace RBX {

	class Body;

	namespace HUMAN {

	extern const char* const  sRunning;
	extern const char* const sRunningSlave;
	extern const char* const sLanded;
	extern const char* const sClimbing;


	class Running : public Named<RunningBase, sRunning>
	{
	private:
		typedef Named<RunningBase, sRunning> Super;
		/*override*/ StateType getStateType() const {return RUNNING;}
		/*override*/ void fireEvents();

	protected:
		/*override*/ void onComputeForceImpl();

	public:
		Running(Humanoid* humanoid, StateType priorState);
	};

	// Slave side only - stays running until some other change to respond to touch events correctly
	class RunningSlave : public Named<Running, sRunningSlave>
	{
	public:
		RunningSlave(Humanoid* humanoid, StateType priorState);
	};

	class Landed : public Named<RunningBase, sLanded>
	{
	private:
		/*override*/ StateType getStateType() const {return LANDED;}

	public:
		Landed(Humanoid* humanoid, StateType priorState);
	};


	class Climbing : public Named<RunningBase, sClimbing>
	{
	private:
		typedef Named<RunningBase, sClimbing> Super;
		/*override*/ StateType getStateType() const {return CLIMBING;}
		/*override*/ void fireEvents();
		/*override*/  int ladderCheckRate() { return 0; }
		/*override*/ bool enableAutoJump()	const	{ return false; }
	public:
		Climbing(Humanoid* humanoid, StateType priorState) : Named<RunningBase, sClimbing>(humanoid, priorState)
		{}

	};

	} // namespace HUMAN
}	// namespace RBX

