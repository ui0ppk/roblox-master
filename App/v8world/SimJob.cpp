/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SimJob.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "Util/StlExtra.h"

namespace RBX {

const SimJob* SimJob::getConstSimJobFromPrimitive(const Primitive* primitive)
{
	if (primitive)
	{
		if (const Assembly* assembly = primitive->getConstAssembly())
		{
			if (const SimJob* answer = assembly->getConstSimJob())
			{
				return answer;
			}
		}
	}
	return NULL;
}

SimJob* SimJob::getSimJobFromPrimitive(Primitive* primitive)
{
	return const_cast<SimJob*>(getConstSimJobFromPrimitive(primitive));
}

bool SimJobTracker::containedBy(SimJob* s)
{
	return (std::find(	s->trackers.begin(), s->trackers.end(), this) != s->trackers.end());
}


void SimJobTracker::stopTracking()
{
	if (simJob) {

		RBXASSERT(this->containedBy(simJob));
	
		fastRemoveShort<SimJobTracker*>(simJob->trackers, this);

		RBXASSERT(!this->containedBy(simJob));

		simJob = NULL;
	}
}

bool SimJobTracker::tracking()
{
	if (simJob) {
		RBXASSERT(this->containedBy(simJob));
		return true;
	}
	else {
		return false;
	}
}

void SimJobTracker::setSimJob(SimJob* m)
{
	stopTracking();

	if (m) {
		RBXASSERT(!this->containedBy(m));

		m->trackers.push_back(this);

		RBXASSERT(this->containedBy(m));

		simJob = m;
	}
}

SimJob* SimJobTracker::getSimJob()
{
	RBXASSERT(simJob);
	RBXASSERT(this->containedBy(simJob));
	return simJob;
}

void SimJobTracker::transferTrackers(SimJob* from, SimJob* to)
{
	RBXASSERT(from);
	RBXASSERT(from != to);

	while (!from->trackers.empty())
	{
		SimJobTracker* transfer = from->trackers.back();

		transfer->setSimJob(to);
	}
}

///////////////////////////////////////////////////////////////////////////////

SimJob::SimJob(Assembly* _assembly) : assembly(_assembly), useCount(0)
{
	RBXASSERT(assembly->getConstSimJob() == NULL);
}


SimJob::~SimJob()
{
	RBXASSERT(useCount == 0);
	RBXASSERT(assembly->getConstSimJob() == NULL);
}

}	// namespace
