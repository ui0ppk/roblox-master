/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/DataModelJob.h"
#include "rbx/Debug.h"
#include "boost/cast.hpp"
#include "reflection/enumconverter.h"
#include "util/Object.h"

#include "rbx/Profiler.h"

using namespace RBX;

namespace RBX
{
	namespace Reflection
	{
		template<>
		Reflection::EnumDesc<DataModelArbiter::ConcurrencyModel>::EnumDesc()
			:Reflection::EnumDescriptor("ConcurrencyModel")
		{
			addPair(DataModelArbiter::Serial, "Serial");
			addPair(DataModelArbiter::Safe, "Safe");
			addPair(DataModelArbiter::Logical, "Logical");
			addPair(DataModelArbiter::Empirical, "Empirical");
		}
	}
}

DataModelJob::DataModelJob(const char* name, TaskType taskType, bool isPerPlayer, shared_ptr<DataModelArbiter> arbiter, Time::Interval stepBudget)
:Job(name, arbiter, stepBudget)
,taskType(taskType)
,stepsAccumulated(0)
,isPerPlayer(isPerPlayer)
,profilingToken(0)
{
	RBXASSERT(arbiter);

#ifdef RBXPROFILER
	profilingToken = Profiler::getToken("Jobs", name);
#endif
}

TaskScheduler::StepResult DataModelJob::step(const Stats& stats)
{
#ifdef RBXPROFILER
	Profiler::Scope profilingScope(profilingToken);
#endif

	return stepDataModelJob(stats);
}

double DataModelJob::updateStepsRequiredForCyclicExecutive(float stepDt, float desiredHz, float maxStepsPerCycle, float maxStepsAccumulated)
{
	float desiredStepsFloat = stepDt * desiredHz + stepsAccumulated;

	float desiredStepsFloor = ::floorf( desiredStepsFloat );
	desiredStepsFloor = std::min(desiredStepsFloor, maxStepsPerCycle);				
	stepsAccumulated = desiredStepsFloat - desiredStepsFloor;
	stepsAccumulated = std::min( (float)stepsAccumulated, maxStepsAccumulated);

	if( desiredStepsFloor <= 0.0f )
	{
		return 0.0f;
	}
	return desiredStepsFloor;
}

double DataModelJob::getPriorityFactor()
{
	//shared_ptr<DataModelArbiter> arbiter(shared_polymorphic_downcast<DataModelArbiter>(getArbiter()));
	//if (!arbiter)
	//	return 1;

	return isPerPlayer ? 1 : std::max(1, getArbiter()->getNumPlayers());
}

// The default is "Logical"
DataModelArbiter::ConcurrencyModel DataModelArbiter::concurrencyModel = Logical;

bool DataModelArbiter::areExclusive(TaskScheduler::Job * job1, TaskScheduler::Job * job2)
{
	DataModelJob* j1 = boost::polymorphic_downcast<DataModelJob*>(job1);
	DataModelJob* j2 = boost::polymorphic_downcast<DataModelJob*>(job2);
	return areExclusive(j1->taskType, j2->taskType);
}


bool DataModelArbiter::areExclusive(DataModelJob::TaskType function1, DataModelJob::TaskType function2)
{
	return lookup[concurrencyModel][function1][function2];
}

void static copy(bool src[DataModelJob::TaskTypeCount][DataModelJob::TaskTypeCount], bool dst[DataModelJob::TaskTypeCount][DataModelJob::TaskTypeCount])
{
	for (int j=0; j<DataModelJob::TaskTypeCount; ++j)
		for (int k=0; k<DataModelJob::TaskTypeCount; ++k)
			dst[j][k] = src[j][k];
}

static void setConcurrent(bool table[DataModelJob::TaskTypeCount][DataModelJob::TaskTypeCount], DataModelJob::TaskType f1, DataModelJob::TaskType f2)
{
	table[f1][f2] = false;
	table[f2][f1] = false;
}

DataModelArbiter::DataModelArbiter(void)
{
	for (int j=0; j<DataModelJob::TaskTypeCount; ++j)
		for (int k=0; k<DataModelJob::TaskTypeCount; ++k)
			lookup[Serial][j][k] = true;	// default all to exclusive

	{
		copy(lookup[Serial], lookup[Safe]);
		// Open up concurrency for Safe:

		setConcurrent(lookup[Safe], DataModelJob::PhysicsOut, DataModelJob::PhysicsOut);
		setConcurrent(lookup[Safe], DataModelJob::DataOut, DataModelJob::DataOut);
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::None);

		// RaknetPeer can be concurrent with everything except for Write,Physics,RaknetPeer:
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::PhysicsOut);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::DataOut);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::PhysicsIn);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::DataIn);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::Read);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::Render);	
		setConcurrent(lookup[Safe], DataModelJob::RaknetPeer, DataModelJob::None);

		// PhysicsOutSort can be concurrent with everything except PhysicsOut
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::DataOut);	
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::PhysicsIn);	
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::DataIn);	
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::Read);	
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::Render);	
		setConcurrent(lookup[Safe], DataModelJob::PhysicsOutSort, DataModelJob::None);

		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::PhysicsOut);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::DataOut);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::PhysicsIn);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::DataIn);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::Read);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::Write);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::Render);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::Physics);	
		setConcurrent(lookup[Safe], DataModelJob::None, DataModelJob::RaknetPeer);	
	}

	{
		copy(lookup[Safe], lookup[Logical]);

		// Open up concurrency for Logical:
		setConcurrent(lookup[Logical], DataModelJob::DataOut, DataModelJob::PhysicsOut);

		// Can't do rendering in parallel because it currently makes changes to the DataModel
		//setConcurrent(lookup[Logical], DataModelJob::Render, DataModelJob::PhysicsOut);	
		
		setConcurrent(lookup[Logical], DataModelJob::Read, DataModelJob::Read);
		setConcurrent(lookup[Logical], DataModelJob::Read, DataModelJob::DataOut);
		setConcurrent(lookup[Logical], DataModelJob::Read, DataModelJob::PhysicsOut);

		// Note: Render and Read are not allowed, because Render can modify the Camera
		// Note: Render and DataOut are not allowed, because Render can modify the Camera
	}

	{
		copy(lookup[Logical], lookup[Empirical]);

		// Open up concurrency for Imperical:
		//setConcurrent(lookup[Empirical], DataModelJob::Render, DataModelJob::PhysicsIn);
		//setConcurrent(lookup[Empirical], DataModelJob::Render, DataModelJob::Physics);
	}
}

// This does not get called closing a place in Studio.
DataModelArbiter::~DataModelArbiter() {
} 

void DataModelArbiter::preStep(TaskScheduler::Job* job) { 
	activityMeter.increment(); 
}

void DataModelArbiter::postStep(TaskScheduler::Job* job) { 
	activityMeter.decrement(); 
}

