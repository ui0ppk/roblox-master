/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "boost/utility.hpp"

#include <list>
#include <vector>
#include "boost/intrusive/list.hpp"

namespace RBX {

	class Primitive;
	class Assembly;
	class SimJob;

	typedef boost::intrusive::list_base_hook< boost::intrusive::tag<SimJob> > SimJobHook;
	typedef boost::intrusive::list<SimJob, boost::intrusive::base_hook<SimJobHook> > SimJobList;

	class SimJobTracker
	{
	private:
		SimJob* simJob;

		bool containedBy(SimJob* s);
		void stopTracking();
	public:
		SimJobTracker() : simJob(NULL) {}

		~SimJobTracker() {
			stopTracking();
		}

		bool tracking();

		void setSimJob(SimJob* s);

		SimJob* getSimJob();

		static void transferTrackers(SimJob* from, SimJob* to);
	};

	class SimJob
		: public boost::noncopyable
		, public SimJobHook
	{
		friend class SimJobTracker;

	private:
		std::vector<SimJobTracker*> trackers;		// this will usually be empty, or have one tracker
		Assembly* assembly;

	public:
		int	useCount;

		SimJob(Assembly* _assembly);

		~SimJob();

		Assembly* getAssembly() {return assembly;}
		const Assembly* getConstAssembly() const {return assembly;}
	
		static SimJob* getSimJobFromPrimitive(Primitive* primitive);
		static const SimJob* getConstSimJobFromPrimitive(const Primitive* primitive);
	};

} // namespace
