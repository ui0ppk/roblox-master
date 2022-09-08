/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/SimJob.h"
#include "rbx/signal.h"
#include "Util/ConcurrencyValidator.h"
#include "rbx/threadsafe.h"


namespace RBX {

	class Edge;
	class Kernel;
	class Assembly;

	class SendPhysics
	{
	private:
		SimJobList simJobs;

		ConcurrencyValidator concurrencyValidator;

		void buildSimJob(SimJob* job);
		void destroySimJob(SimJob* job);
		mutable rbx::spin_mutex changeTrackerMutex;

		void setTrackerSimJob(SimJobTracker& tracker, SimJob* simJob) const
		{
			rbx::spin_mutex::scoped_lock lock(changeTrackerMutex);
			tracker.setSimJob(simJob);
		}

	public:
		rbx::signal<void(Primitive*)> assemblyPhysicsOnSignal;
		rbx::signal<void(Primitive*)> assemblyPhysicsOffSignal;

		SimJob* nextSimJob(SimJob* current)
		{
			RBXASSERT(!simJobs.empty());
			SimJobList::iterator iter = simJobs.iterator_to(*current);
			++iter;
			return (iter == simJobs.end()) ? &simJobs.front() : &*iter;
		}

		template<class Callback>
		int reportSimJobs(Callback& callback, SimJobTracker& tracker, const SimJob* ignore, int numToReport = -1)
		{
			int reported = 0;
			
			ReadOnlyValidator readOnlyValidator(concurrencyValidator);
			{
				if (simJobs.empty()) {
					return 0;
				}

				if (!tracker.tracking()) {
					setTrackerSimJob(tracker, &simJobs.front());
				}

				SimJob* simJob = tracker.getSimJob();
				int num = numToReport;
				if (num == -1)
					num = simJobs.size();

				while (reported < num)
				{
					SimJob* current = simJob;
					++reported;
					simJob = nextSimJob(simJob);

					if (current != ignore)
					{
						if (!callback(*current))					// returns true if wants another sample (m is always used)
						{
							break;
						}
					}
				}
				setTrackerSimJob(tracker, simJob);
			}

			return reported;
		}

		SendPhysics();

		~SendPhysics();

		int getNumSimJobs() { return simJobs.size(); };

		void onMovingAssemblyRootAdded(Assembly* assembly);
		void onMovingAssemblyRootRemoving(Assembly* assembly);
	};

} // namespace
