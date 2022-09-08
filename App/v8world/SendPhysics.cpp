/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SendPhysics.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8World/Primitive.h"


namespace RBX {


SendPhysics::SendPhysics()
{}


SendPhysics::~SendPhysics()
{
	RBXASSERT(simJobs.empty());
}

void SendPhysics::buildSimJob(SimJob* job)
{
	if (job->getAssembly()) {
		RBXASSERT(!job->getConstAssembly()->getConstSimJob());
		job->getAssembly()->setSimJob(job);
	}

	simJobs.push_back(*job);
}


void SendPhysics::destroySimJob(SimJob* job)
{
	RBXASSERT_SLOW(job->is_linked());

	SimJob* transferTo = (simJobs.size() > 1) 
										? nextSimJob(job) 
										: NULL;

	SimJobTracker::transferTrackers(job, transferTo);
	simJobs.erase(simJobs.iterator_to(*job));

	if (job->getAssembly()) {
		RBXASSERT(job->getAssembly()->getSimJob() == job);
		job->getAssembly()->setSimJob(NULL);
	}
}

void SendPhysics::onMovingAssemblyRootAdded(Assembly* a)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT(Mechanism::isMovingAssemblyRoot(a));
	if (!a->getSimJob())
	{
		SimJob* job = new SimJob(a);
		buildSimJob(job);

		assemblyPhysicsOnSignal(a->getAssemblyPrimitive());
	}
	a->getSimJob()->useCount++;
}


void SendPhysics::onMovingAssemblyRootRemoving(Assembly* a)
{
	WriteValidator writeValidator(concurrencyValidator);

//	RBXASSERT_VERY_FAST(Mechanism::isMovingAssemblyRoot(a));					// todo - get rid of this assert by refactoring primitive anchoring
	SimJob* job = a->getSimJob();
	job->useCount--;

	if (job->useCount == 0)
	{
		assemblyPhysicsOffSignal(a->getAssemblyPrimitive());

		destroySimJob(job);
		delete job;
	}
}


} // namespace