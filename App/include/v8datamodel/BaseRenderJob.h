#pragma once

#include <boost/weak_ptr.hpp>

#include "v8datamodel/DataModel.h"
#include "rbx/rbxTime.h"
#include "rbx/TaskScheduler.h"
#include "rbx/TaskScheduler.Job.h"
#include "util/IMetric.h"


namespace RBX {
	class View;

// This is the base class for all Rendering Jobs. All rendering jobs will inherit from this.

class BaseRenderJob : public DataModelJob
{
protected:
	Time							lastRenderTime;
	volatile bool					isAwake;
	double							minFrameRate;
	double							maxFrameRate;


public:
	BaseRenderJob(double minFrameRate, double maxFrameRate, boost::shared_ptr<DataModel> dataModel );

	virtual void wake();
	virtual bool tryJobAgain();
	virtual bool isCyclicExecutiveJob();

	virtual Time::Interval timeSinceLastRender() const;
	virtual Job::Error error(const Stats& stats);
	//virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats&);

    virtual TaskScheduler::StepResult step(const Stats& stats);
};

}