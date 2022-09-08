/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/Constants.h"
#include "Util/Math.h"
#include "Util/Units.h"
#include "rbx/Debug.h"
#include <vector>
#include <algorithm>

namespace RBX {

//static int kernelStepsPerWorldStepTest = 80;
//static float originalHzFactorTest = 19.0f;

Constants::Constants() 
{}

int Constants::worldStepsPerLongUiStep()	{return 8;}

int Constants::worldStepsPerUiStep()		{return worldStepsPerLongUiStep() / 2;}

int Constants::kernelStepsPerWorldStep()	{return 19;}
int Constants::freeFallStepsPerWorldStep(){return 1;}
//const int Constants::kernelStepsPerWorldStep()	{return kernelStepsPerWorldStepTest;}
int Constants::worldStepsPerSec()			{return uiStepsPerSec() * worldStepsPerUiStep();}
int Constants::kernelStepsPerSec()		{return worldStepsPerSec() * kernelStepsPerWorldStep();}
int Constants::kernelStepsPerUiStep()		{return kernelStepsPerWorldStep() * worldStepsPerUiStep();}
int Constants::freeFallStepsPerSec()		{return worldStepsPerSec() * freeFallStepsPerWorldStep();}
int Constants::impulseSolverMaxIterations() {return 60;}
float Constants::impulseSolverAccuracy()	{return 0.1;}		// in unit of relative velocity
int Constants::impulseSolverAccuracyScalar() {return 400;}	// how many parts to dial down accuracy for one level
float Constants::impulseSolverSymStateTorqueBound() {return 0.05f;}
float Constants::impulseSolverSymStateForceBound() {return 0.001f;}
float Constants::uiDt()					{return 1.0f / uiStepsPerSec();}
float Constants::longUiStepDt()			{return 1.0f / longUiStepsPerSec();}
float Constants::worldDt()				{return 1.0f / worldStepsPerSec();}
float Constants::kernelDt()				{return 1.0f / kernelStepsPerSec();}
float Constants::freeFallDt()				{return 1.0f / freeFallStepsPerSec();}

// original joint K calcs were done and created the default jointK for 1x1x1 block of 960000 at 4560hz
float originalHz()						{return 30.0f * 8.0f * 19.0f;}
//const float originalHz()						{return 30.0f * 8.0f * originalHzFactorTest;}
float Constants::unitJointK()				{return 960000.0f / (originalHz() * originalHz());}

float Constants::getElasticMultiplier(float elasticity) 
{
	if (elasticity < 0.05f)			return 0.28f;
	else if (elasticity < 0.26f)	return 0.42f;
	else if (elasticity < 0.51f)	return 0.57f;
	else if (elasticity < 0.76f)	return 0.8f;
	else							return 1.0f;
}



// These are constants for LEGO, taken from actual measurements
// See:
//	Evolution of Complexity in Real-World Domains
//  Pablo Funes 
//  Brandeis University 
//
// The original TORQUE chart is here:
/*
Joint size () Approximate torque capacity ( ) 
knobs N-m  
1 12.7 
2 61.5 
3 109.8 
4 192.7 
5 345.0 
6 424.0 
*/
// This chart was converted from Torques to forces
// See "Joint Breakage.xlr"
// then factored by the length of the moment arm
// this if for a LEGO with 3 grid units high in the y direction
//
// This is the edge force for a one-unit wide beam.
// Point forces for a one-unit wide beam will be 1/2 of this value
// Point forces for a two-unit wide beam will equal this value
const float Constants::MAX_LEGO_JOINT_FORCES_THEORY[] = {	0.0f,		// kg*m/s^2
															1.5875f, 
															3.84375f, 
															4.5750f, 
															6.0210f,
															8.6250f,
															8.8330f	};

// These are the values I got from actual measurements
const float Constants::MAX_LEGO_JOINT_FORCES_MEASURED[] = {	0.0f,		// kg*m/s^2
															1.0980f,
															2.1340f, 
															2.4270f, 
															3.1910f,
															4.5710f,
															4.6810f	};

// To convert to other unit systems for same general behavior
//
// Consider a 1x3x4 brick in Lego World - overlapping by one grid
// Mass: 1.6 gram or 0.0016 kg
// Fg = m*Ag = 15.696 mm/s^2 - same force is applied on the opposing "up" direction
// so - only need to scale by the mass per grid unit....

float Constants::LEGO_GRID_MASS()
{
	// a 1x3x1 unit weighs about 0.4 grams;
	// convert to kg (*0.001)
	// divide by 3
	return (float) (0.4 * 0.001 * 0.33333333);		// kg,  divide by 3 for y layers
}


float Constants::LEGO_JOINT_K() {
	return 35000.0;							// kg/s^2
}


float Constants::getKmsMaxJointForce(float grid1, float grid2)		// kg*mm/s^2
{
	RBXASSERT(std::abs(grid1*10.0f - Math::iRound(grid1*10.0f)) < 0.01f);		// Can't enter with values too far off grid
	RBXASSERT(std::abs(grid2*10.0f - Math::iRound(grid2*10.0f)) < 0.01f);		// indicates a bad snap
																// for now - all snapes should be on units of 0.2
	int grid1int = std::max(1, Math::iRound(grid1));
	int grid2int = std::max(1, Math::iRound(grid2));

	int overlap = std::max(grid1int, grid2int);			// note - should be an integer, but handle non-int as well
	int width	= std::min(grid1int, grid2int);

	float oneWideEdgeForce;

	if (overlap < JOINT_FORCE_DATA) {
		oneWideEdgeForce = MAX_LEGO_JOINT_FORCES_MEASURED[overlap];
	}
	else {								// for now, just keep increasing force proportional to ratio...
		float ratio = static_cast<float>(overlap);
		ratio /= JOINT_FORCE_DATA;
		oneWideEdgeForce = MAX_LEGO_JOINT_FORCES_MEASURED[JOINT_FORCE_DATA - 1] * ratio;
	}

	float oneWidePointForce = oneWideEdgeForce * 0.5f;	// because force values are for the edge force, not a point force
														// of which there are two
	float maxPointForce = oneWidePointForce * width;	// scale for width;

	return maxPointForce * (1.0f / LEGO_GRID_MASS());		
}


/*

_____________________________________________________
JOINT Optimizer Results 

Part Type	x	y	z	Optimized	Recommended	RATIO
BALL		1	1	1	0.23	1.00	0.23	
BALL		2	2	2	1.49	2.64	0.56	
BALL		3	3	3	4.43	6.75	0.66	
BALL		4	4	4	11.50	16.00	0.72	
BALL		6	6	6	37.13	54.00	0.69	
BALL		10	10	10	171.88	250.00	0.69	
BALL		15	15	15	606.45	843.75	0.72	
BALL		20	20	20	1437.50	2000.00	0.72	
BALL		40	40	40	12000.00	16000.00	0.75	
BALL		80	80	80	104000.00	128000.00	0.81	
BLOCK		1	1	1	0.91	1.00	0.91	
BLOCK		1	1	2	1.61	1.66	0.97	
BLOCK		1	1	3	2.00	2.00	1.00	
BLOCK		1	1	4	2.13	3.33	0.64	
BLOCK		1	1	6	2.92	4.80	0.61	
BLOCK		1	1	10	4.63	8.00	0.58	
BLOCK		1	1	15	6.00	12.00	0.50	
BLOCK		1	1	20	8.50	16.00	0.53	
BLOCK		1	1	40	17.00	32.00	0.53	
BLOCK		1	1	80	32.50	64.00	0.51	
BLOCK		1	2	2	3.50	4.00	0.88	
BLOCK		1	2	3	4.16	5.33	0.78	
BLOCK		1	2	4	4.79	6.66	0.72	
BLOCK		1	2	6	6.98	7.98	0.88	
BLOCK		1	2	10	9.56	13.30	0.72	
BLOCK		1	2	15	14.34	19.95	0.72	
BLOCK		1	2	20	17.46	26.60	0.66	
BLOCK		1	2	40	31.59	53.20	0.59	
BLOCK		1	2	80	63.17	106.40	0.59	
BLOCK		1	3	3	6.07	4.98	1.22	
BLOCK		1	3	4	7.47	6.64	1.13	
BLOCK		1	3	6	10.27	9.96	1.03	
BLOCK		1	3	10	14.01	16.60	0.84	
BLOCK		1	3	15	21.01	24.90	0.84	
BLOCK		1	3	20	26.98	33.20	0.81	
BLOCK		1	3	40	49.80	66.40	0.75	
BLOCK		1	3	80	95.45	132.80	0.72	
BLOCK		1	4	4	9.65	7.92	1.22	
BLOCK		1	4	6	13.36	11.88	1.13	
BLOCK		1	4	10	19.18	19.80	0.97	
BLOCK		1	4	15	27.84	29.70	0.94	
BLOCK		1	4	20	35.89	39.60	0.91	
BLOCK		1	4	40	64.35	79.20	0.81	
BLOCK		1	4	80	123.75	158.40	0.78	
BLOCK		1	6	6	20.79	15.84	1.31	
BLOCK		1	6	10	33.83	26.40	1.28	
BLOCK		1	6	15	42.08	39.60	1.06	
BLOCK		1	6	20	51.15	52.80	0.97	
BLOCK		1	6	40	95.70	105.60	0.91	
BLOCK		1	6	80	198.00	211.20	0.94	
BLOCK		1	10	10	50.74	39.60	1.28	
BLOCK		1	10	15	70.54	59.40	1.19	
BLOCK		1	10	20	89.10	79.20	1.13	
BLOCK		1	10	40	163.35	158.40	1.03	
BLOCK		1	10	80	326.70	316.80	1.03	
BLOCK		1	15	15	110.45	84.15	1.31	
BLOCK		1	15	20	129.73	112.20	1.16	
BLOCK		1	15	40	280.50	224.40	1.25	
BLOCK		1	15	80	532.95	448.80	1.19	
BLOCK		1	20	20	186.04	145.20	1.28	
BLOCK		1	20	40	363.00	290.40	1.25	
BLOCK		1	20	80	635.25	580.80	1.09	
BLOCK		1	40	40	710.33	554.40	1.28	
BLOCK		1	40	80	1316.70	1108.80	1.19	
BLOCK		1	80	80	2706.00	2164.80	1.25	
BLOCK		2	2	2	7.34	2.64	2.78	
BLOCK		2	2	3	9.90	3.96	2.50	
BLOCK		2	2	4	11.22	5.28	2.13	
BLOCK		2	2	6	13.36	7.92	1.69	
BLOCK		2	2	10	21.45	13.20	1.63	
BLOCK		2	2	15	29.70	19.80	1.50	
BLOCK		2	2	20	37.95	26.40	1.44	
BLOCK		2	2	40	64.35	52.80	1.22	
BLOCK		2	2	80	122.10	105.60	1.16	
BLOCK		2	3	3	15.96	5.94	2.69	
BLOCK		2	3	4	19.06	7.92	2.41	
BLOCK		2	3	6	22.46	11.88	1.89	
BLOCK		2	3	10	32.17	19.80	1.63	
BLOCK		2	3	15	42.69	29.70	1.44	
BLOCK		2	3	20	51.98	39.60	1.31	
BLOCK		2	3	40	99.00	79.20	1.25	
BLOCK		2	3	80	183.15	158.40	1.16	
BLOCK		2	4	4	25.41	10.56	2.41	
BLOCK		2	4	6	32.17	15.84	2.03	
BLOCK		2	4	10	42.90	26.40	1.63	
BLOCK		2	4	15	56.92	39.60	1.44	
BLOCK		2	4	20	69.30	52.80	1.31	
BLOCK		2	4	40	128.70	105.60	1.22	
BLOCK		2	4	80	244.20	211.20	1.16	
BLOCK		2	6	6	44.92	23.76	1.89	
BLOCK		2	6	10	74.25	39.60	1.88	
BLOCK		2	6	15	100.24	59.40	1.69	
BLOCK		2	6	20	118.80	79.20	1.50	
BLOCK		2	6	40	198.00	158.40	1.25	
BLOCK		2	6	80	386.10	316.80	1.22	
BLOCK		2	10	10	115.50	66.00	1.75	
BLOCK		2	10	15	142.31	99.00	1.44	
BLOCK		2	10	20	214.50	132.00	1.63	
BLOCK		2	10	40	346.50	264.00	1.31	
BLOCK		2	10	80	693.00	528.00	1.31	
BLOCK		2	15	15	232.03	148.50	1.56	
BLOCK		2	15	20	284.63	198.00	1.44	
BLOCK		2	15	40	519.75	396.00	1.31	
BLOCK		2	15	80	990.00	792.00	1.25	
BLOCK		2	20	20	379.50	264.00	1.44	
BLOCK		2	20	40	726.00	528.00	1.38	
BLOCK		2	20	80	1353.00	1056.00	1.28	
BLOCK		2	40	40	1452.00	1056.00	1.38	
BLOCK		2	40	80	2904.00	2112.00	1.38	
BLOCK		2	80	80	5808.00	4224.00	1.38	
BLOCK		3	3	3	23.84	6.75	3.53	
BLOCK		3	3	4	30.09	9.00	3.34	
BLOCK		3	3	6	37.55	13.50	2.78	
BLOCK		3	3	10	49.92	22.50	2.22	
BLOCK		3	3	15	71.72	33.75	2.13	
BLOCK		3	3	20	85.08	45.00	1.89	
BLOCK		3	3	40	151.88	90.00	1.69	
BLOCK		3	3	80	303.75	180.00	1.69	
BLOCK		3	4	4	43.50	12.00	3.63	
BLOCK		3	4	6	55.13	18.00	3.06	
BLOCK		3	4	10	75.00	30.00	2.50	
BLOCK		3	4	15	95.63	45.00	2.13	
BLOCK		3	4	20	113.44	60.00	1.89	
BLOCK		3	4	40	210.00	120.00	1.75	
BLOCK		3	4	80	405.00	240.00	1.69	
BLOCK		3	6	6	85.22	27.00	3.16	
BLOCK		3	6	10	112.50	45.00	2.50	
BLOCK		3	6	15	156.09	67.50	2.31	
BLOCK		3	6	20	174.38	90.00	1.94	
BLOCK		3	6	40	337.50	180.00	1.88	
BLOCK		3	6	80	540.00	360.00	1.50	
BLOCK		3	10	10	187.50	75.00	2.50	
BLOCK		3	10	15	260.16	112.50	2.31	
BLOCK		3	10	20	290.63	150.00	1.94	
BLOCK		3	10	40	562.50	300.00	1.88	
BLOCK		3	10	80	1050.00	600.00	1.75	
BLOCK		3	15	15	358.59	168.75	2.13	
BLOCK		3	15	20	478.13	225.00	2.13	
BLOCK		3	15	40	871.88	450.00	1.94	
BLOCK		3	15	80	1518.75	900.00	1.69	
BLOCK		3	20	20	637.50	300.00	2.13	
BLOCK		3	20	40	1087.50	600.00	1.81	
BLOCK		3	20	80	2100.00	1200.00	1.75	
BLOCK		3	40	40	2175.00	1200.00	1.81	
BLOCK		3	40	80	3900.00	2400.00	1.63	
BLOCK		3	80	80	9000.00	4800.00	1.88	
BLOCK		4	4	4	59.75	16.00	3.73	
BLOCK		4	4	6	87.00	24.00	3.63	
BLOCK		4	4	10	107.50	40.00	2.69	
BLOCK		4	4	15	116.25	60.00	1.94	
BLOCK		4	4	20	145.00	80.00	1.81	
BLOCK		4	4	40	300.00	160.00	1.88	
BLOCK		4	4	80	560.00	320.00	1.75	
BLOCK		4	6	6	127.13	36.00	3.53	
BLOCK		4	6	10	155.63	60.00	2.59	
BLOCK		4	6	15	208.13	90.00	2.31	
BLOCK		4	6	20	243.75	120.00	2.03	
BLOCK		4	6	40	435.00	240.00	1.81	
BLOCK		4	6	80	750.00	480.00	1.56	
BLOCK		4	10	10	278.13	100.00	2.78	
BLOCK		4	10	15	360.94	150.00	2.41	
BLOCK		4	10	20	425.00	200.00	2.13	
BLOCK		4	10	40	750.00	400.00	1.88	
BLOCK		4	10	80	1450.00	800.00	1.81	
BLOCK		4	15	15	562.50	225.00	2.50	
BLOCK		4	15	20	637.50	300.00	2.13	
BLOCK		4	15	40	1134.38	600.00	1.89	
BLOCK		4	15	80	1950.00	1200.00	1.63	
BLOCK		4	20	20	925.00	400.00	2.31	
BLOCK		4	20	40	1625.00	800.00	2.03	
BLOCK		4	20	80	2700.00	1600.00	1.69	
BLOCK		4	40	40	3250.00	1600.00	2.03	
BLOCK		4	40	80	5800.00	3200.00	1.81	
BLOCK		4	80	80	12100.00	6400.00	1.89	
BLOCK		6	6	6	180.56	54.00	3.34	
BLOCK		6	6	10	233.44	90.00	2.59	
BLOCK		6	6	15	312.19	135.00	2.31	
BLOCK		6	6	20	365.63	180.00	2.03	
BLOCK		6	6	40	652.50	360.00	1.81	
BLOCK		6	6	80	1260.00	720.00	1.75	
BLOCK		6	10	10	487.50	150.00	3.25	
BLOCK		6	10	15	583.59	225.00	2.59	
BLOCK		6	10	20	750.00	300.00	2.50	
BLOCK		6	10	40	1275.00	600.00	2.13	
BLOCK		6	10	80	2175.00	1200.00	1.81	
BLOCK		6	15	15	875.39	337.50	2.59	
BLOCK		6	15	20	1082.81	450.00	2.41	
BLOCK		6	15	40	1743.75	900.00	1.94	
BLOCK		6	15	80	3150.00	1800.00	1.75	
BLOCK		6	20	20	1556.25	600.00	2.59	
BLOCK		6	20	40	2325.00	1200.00	1.94	
BLOCK		6	20	80	4200.00	2400.00	1.75	
BLOCK		6	40	40	4875.00	2400.00	2.03	
BLOCK		6	40	80	9000.00	4800.00	1.88	
BLOCK		6	80	80	18600.00	9600.00	1.94	
BLOCK		10	10	10	789.06	250.00	3.16	
BLOCK		10	10	15	1078.13	375.00	2.88	
BLOCK		10	10	20	1250.00	500.00	2.50	
BLOCK		10	10	40	2031.25	1000.00	2.03	
BLOCK		10	10	80	3625.00	2000.00	1.81	
BLOCK		10	15	15	1880.86	562.50	3.34	
BLOCK		10	15	20	2296.88	750.00	3.06	
BLOCK		10	15	40	3468.75	1500.00	2.31	
BLOCK		10	15	80	5625.00	3000.00	1.88	
BLOCK		10	20	20	2875.00	1000.00	2.88	
BLOCK		10	20	40	4625.00	2000.00	2.31	
BLOCK		10	20	80	7750.00	4000.00	1.94	
BLOCK		10	40	40	9250.00	4000.00	2.31	
BLOCK		10	40	80	15500.00	8000.00	1.94	
BLOCK		10	80	80	31000.00	16000.00	1.94	
BLOCK		15	15	15	2742.19	843.75	3.25	
BLOCK		15	15	20	3339.84	1125.00	2.97	
BLOCK		15	15	40	5414.06	2250.00	2.41	
BLOCK		15	15	80	8718.75	4500.00	1.94	
BLOCK		15	20	20	5015.62	1500.00	3.34	
BLOCK		15	20	40	7218.75	3000.00	2.41	
BLOCK		15	20	80	11625.00	6000.00	1.94	
BLOCK		15	40	40	17250.00	6000.00	2.88	
BLOCK		15	40	80	26625.00	12000.00	2.22	
BLOCK		15	80	80	53250.00	24000.00	2.22	
BLOCK		20	20	20	6500.00	2000.00	3.25	
BLOCK		20	20	40	9250.00	4000.00	2.31	
BLOCK		20	20	80	16250.00	8000.00	2.03	
BLOCK		20	40	40	23000.00	8000.00	2.88	
BLOCK		20	40	80	37000.00	16000.00	2.31	
BLOCK		20	80	80	80000.00	32000.00	2.50	
BLOCK		40	40	40	55000.00	16000.00	3.44	
BLOCK		40	40	80	74000.00	32000.00	2.31	
BLOCK		40	80	80	184000.00	64000.00	2.88	
BLOCK		80	80	80	440000.00	128000.00	3.44	
*/


// New version from actual data - June 30, 2005
float Constants::getJointKMultiplier(const Vector3& clippedSortedSize, bool ball)
{
	RBXASSERT(clippedSortedSize.y >= clippedSortedSize.x);
	RBXASSERT(clippedSortedSize.z >= clippedSortedSize.y);
	RBXASSERT(clippedSortedSize.max(Vector3(1,1,1)) == clippedSortedSize);

	Vector3int16 size(clippedSortedSize);

	if (ball) 
	{
		RBXASSERT(size.x >= 1);

		switch (size.x)
		{
		case 1:		return 0.23f;
		case 2:		return 1.49f;
		case 3:		return 4.43f;
		case 4:		return 11.50f;
		default:	return size.x * size.x * size.x * 0.175f;
		}
	}


	switch (size.x) 
	{
	case 1:			//////////// 1 thick table
		switch (size.y) 
		{
		case 1:				//	1*1*n
			switch (size.z) 
			{
			case 1:		return 0.91f;
			case 2:		return 1.61f;
			case 3:		return 2.0f;
			case 4:		return 2.13f;
			default:	return size.z * 0.4f;
			}

		case 2:				// 1*2*n
			switch (size.z) 
			{
			case 2:		return 3.5f;
			case 3:		return 4.16f;
			case 4:		return 4.79f;
			default:	return (size.z < 15.0f) ? size.z * 0.9f : size.z * 0.75f;
			}
				
		case 3:				// 1*3*n
			return (size.z < 7.0f) ? size.z * 1.66f : size.z * 1.18f;

		case 4:				// 1*4*n
			return (size.z < 7.0f) ? size.z * 2.26f : size.z * 1.53f;

		default:
			return size.z * (size.y * 0.3f + 0.66f);	
		}

	case 2:			//////////// 2 thick table
		switch (size.y) 
		{
		case 2:				//	2*2*n
			switch (size.z) 
			{
			case 2:		return 7.34f;
			case 3:		return 9.90f;
			case 4:		return 11.22f;
			default:	return (size.z < 15.0f) ? size.z * 1.9f : size.z * 1.5f;
			}

		case 3:				// 2*3*n
			switch (size.z) 
			{
			case 3:		return 15.0f;
			case 4:		return 19.0f;
			default:	return (size.z < 15.0f) ? size.z * 2.0f : size.z * 1.5f;
			}

		default:				
			return size.z * (size.y * 0.66f);	
		}

	default:		////////////// at least 3 thick
		return size.x * size.y * size.z * 0.25f;
	}
}

float Constants::getJointK(const Vector3& size, bool ball)
{
	Vector3 sortedSize = Math::sortVector3(size);

	Vector3 clippedSize = sortedSize.max(Vector3(1.0f,1.0f,1.0f));

	float sizeMultiplier = getJointKMultiplier(clippedSize, ball);

	if (sortedSize[0] < 1.0) {
		sizeMultiplier *= sortedSize[0];
	}

	return sizeMultiplier * kernelStepsPerSec() * kernelStepsPerSec() * unitJointK();
}


} // namespace
