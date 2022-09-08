/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	class Constants {
	private:
		static const int JOINT_FORCE_DATA = 7;
		static const float MAX_LEGO_JOINT_FORCES_THEORY[JOINT_FORCE_DATA];
		static const float MAX_LEGO_JOINT_FORCES_MEASURED[JOINT_FORCE_DATA];

		static float LEGO_GRID_MASS();								// kg
		static float LEGO_JOINT_K();								// kg/s^2
		static float LEGO_DEFAULT_ELASTIC_K();

		static float unitJointK();

		static float getJointKMultiplier(const Vector3& clippedSortedSize, bool ball);

		Constants();

	public:
		///////////////////////////////////////////////////////////////////
		//
		// Timestep related stuff
		//
		static int uiStepsPerSec() {return longUiStepsPerSec() * 2;}
		static int worldStepsPerUiStep();
		static int longUiStepsPerSec() {return 30;}
		static int worldStepsPerLongUiStep();
		static int kernelStepsPerWorldStep();
		static int freeFallStepsPerWorldStep();
		static int worldStepsPerSec();
		static int kernelStepsPerSec();
		static int kernelStepsPerUiStep();
		static int freeFallStepsPerSec();
		static int impulseSolverMaxIterations();
		static float impulseSolverAccuracy();
		static int impulseSolverAccuracyScalar();
		static float impulseSolverSymStateTorqueBound();
		static float impulseSolverSymStateForceBound();
		static float uiDt();
		static float longUiStepDt();
		static float worldDt();
		static float kernelDt();
		static float freeFallDt();
		static const Vector3& denormalSmall();

		//////////////////////////////////////////////////////////////////
		//
		// Dimenensions and K related stuff
		//
		static inline float getKmsGravity()		{return -9.81f;}
		static float getKmsMaxJointForce(float grid1, float grid2);
		static float getElasticMultiplier(float elasticity);
		static float getJointK(const Vector3& size, bool ball);							// kg/s^2
};

} // namespace