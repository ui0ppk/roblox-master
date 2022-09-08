/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/AssemblyHistory.h"
#include "V8World/Assembly.h"
#include "V8World/Primitive.h"
#include "V8Kernel/Body.h"

namespace RBX {

size_t AssemblyHistory::bufferSize()			{return 15;}	
size_t AssemblyHistory::sampleSkip()			{return 8;}			// third of a second
float AssemblyHistory::sleepTolerance()			{return 0.1f;}
float AssemblyHistory::sleepToleranceSquared()	{return sleepTolerance() * sleepTolerance();}


AssemblyHistory::AssemblyHistory(Assembly& a)
: average(bufferSize(), getAssemblyPhysicsCoord(a))
, stepsSinceSample(0)
, awakeSteps(bufferSize())
, maxDeviationSquared(0)
{}

AssemblyHistory::~AssemblyHistory()
{}

void AssemblyHistory::clear(Assembly& a)
{
	average.resetValues( bufferSize(), getAssemblyPhysicsCoord(a) );
	stepsSinceSample = 0;
	awakeSteps = bufferSize();
	maxDeviationSquared = 0;
}

bool AssemblyHistory::sampleAndNotMoving(Assembly& a)
{
	stepsSinceSample++;
	if (stepsSinceSample == sampleSkip())
	{
		stepsSinceSample = 0;
		awakeSteps = std::max(0, --awakeSteps);
		average.sample(getAssemblyPhysicsCoord(a));

        updateMaxDeviationSquared();
	}

	return notMoving();
}

bool AssemblyHistory::notMoving()
{
	return (	(awakeSteps == 0) 
			&&	(maxDeviationSquared < sleepToleranceSquared())	);
}

bool AssemblyHistory::preventNeighborSleep()
{
	return (maxDeviationSquared > (2.0f * sleepToleranceSquared()));
}

void AssemblyHistory::wakeUp()
{
	awakeSteps = bufferSize();
}

void AssemblyHistory::updateMaxDeviationSquared()
{
	float answer = 0.0f;

	PhysicsCoord averageLocation = average.getAverage();

	size_t size = average.size();
	for (size_t i = 0; i < size; ++i) {
		PhysicsCoord delta = average.getValue(i) - averageLocation;
		float distSquared = delta.squaredMagnitude();
		answer = std::max(answer, distSquared);
	}

	maxDeviationSquared = answer;
}

PhysicsCoord AssemblyHistory::getAssemblyPhysicsCoord(Assembly& a)
{
	const CoordinateFrame& cofm = a.getAssemblyPrimitive()->getBody()->getBranchCofmCoordinateFrame();
	return PhysicsCoord(	cofm.translation, 
							Quaternion(cofm.rotation) * a.computeMaxRadius()	);
}

}// namespace
