
#include "rbx/Tasks/Coordinator.h"

using namespace RBX;
using namespace Tasks;

bool Barrier::isInhibited(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);
	return jobs[job] > counter;
}

void Barrier::onPostStep(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	unsigned int& count(jobs[job]);

	// Note: we could almost assert that count==counter, but
	//       there may be edge cases where a step slips through
	//       the cracks
	if (count == counter)
	{
		count++;
		if (--remainingTasks == 0)
			releaseBarrier();
	}
}

void Barrier::releaseBarrier()
{
	remainingTasks = int(jobs.size());
	counter++;
}

void Barrier::onAdded(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	RBXASSERT(jobs.find(job)==jobs.end());

	jobs[job] = counter;
	remainingTasks++;

	RBXASSERT(remainingTasks <= jobs.size());
}

void Barrier::onRemoved(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	RBXASSERT(jobs.find(job)!=jobs.end());

	if (jobs[job] <= counter)
		remainingTasks--;

	jobs.erase(job);

	RBXASSERT(remainingTasks <= jobs.size());
}




bool SequenceBase::isInhibited(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	if (nextJobIndex < jobs.size())
		return jobs[nextJobIndex] != job;
	else
		return true;
}

void SequenceBase::advance()
{
	RBX::mutex::scoped_lock lock(mutex);

	if (++nextJobIndex == jobs.size())
		nextJobIndex = 0;
}

void SequenceBase::onAdded(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	jobs.push_back(job);
}

void SequenceBase::onRemoved(TaskScheduler::Job* job)
{
	RBX::mutex::scoped_lock lock(mutex);

	for (size_t i=0; i<jobs.size(); ++i)
		if (jobs[i] == job)
		{
			jobs.erase(jobs.begin() + i);
			if (nextJobIndex > i)
				--nextJobIndex;
			break;
		}
}


bool Exclusive::isInhibited(TaskScheduler::Job* job)
{
	RBXASSERT(job != runningJob);
	return runningJob != NULL;
}

void Exclusive::onPreStep(TaskScheduler::Job* job)
{
	RBXASSERT(runningJob == NULL);
	runningJob = job;
}

void Exclusive::onPostStep(TaskScheduler::Job* job)
{
	RBXASSERT(runningJob == job);
	runningJob = NULL;
}





