/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/PhysicsCoord.h"
#include "Util/Average.h"
#include "rbx/Debug.h"

namespace RBX {

	class Assembly;
	
	class AssemblyHistory
	{
	private:
		Average<PhysicsCoord> average;
		int stepsSinceSample;
		int awakeSteps;
        float maxDeviationSquared;

		static size_t sampleSkip();
		static size_t bufferSize();
		static float sleepTolerance(); 
		static float sleepToleranceSquared(); 

		bool notMoving();
		void updateMaxDeviationSquared();
		PhysicsCoord getAssemblyPhysicsCoord(Assembly& a);

	public:
		AssemblyHistory(Assembly& a);

		~AssemblyHistory();

		void clear(Assembly& a);
		
		bool sampleAndNotMoving(Assembly& a);

		bool preventNeighborSleep();

		void wakeUp();
	};

}// namespace
