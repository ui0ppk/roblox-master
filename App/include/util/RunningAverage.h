/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "Util/Quaternion.h"


namespace RBX {

	class RunningAverageState
	{
	private:
		Vector3		position;
		Quaternion	angles;

		static float weight();				// % of prior average to use

	public:
		static int stepsToSleep();			// number of good steps to sleep

		RunningAverageState() {}
	
		void reset(const CoordinateFrame& cofm);

		void update(const CoordinateFrame& cofm, float radius);

		bool withinTolerance(const CoordinateFrame& cofm, float radius, float tolerance);
	};



}// namespace
