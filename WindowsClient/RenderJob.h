#pragma once

#include <boost/weak_ptr.hpp>

#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/DataModel.h"
#include "rbx/rbxTime.h"
#include "rbx/TaskScheduler.h"
#include "rbx/TaskScheduler.Job.h"
#include "util/IMetric.h"

namespace RBX {

class FunctionMarshaller;
class View;
class ViewBase;

// This job calls ViewBase::render(), which needs to be done exclusive to the
// DataModel. This is why it has the DataModelJob::Render enum, which
// prevents concurrent writes to DataModel. It also needs to run in the view's
// thread for OpenGL.
// TODO: Can Ogre be modified to not require the thread?
class RenderJob : public BaseRenderJob, public IMetric
{
	FunctionMarshaller* marshaller;
	View* robloxView;
	volatile int stopped;

	CEvent prepareBeginEvent;
	CEvent prepareEndEvent;

	static void scheduleRender(weak_ptr<RenderJob> selfWeak, ViewBase* view, double timeJobStart);

public:
	RenderJob(View* robloxView, FunctionMarshaller* marshaller,
		boost::shared_ptr<DataModel> dataModel);

	Time::Interval timeSinceLastRender() const;
	Time::Interval sleepTime(const Stats& stats);

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);

	virtual std::string getMetric(const std::string& metric) const;
	virtual double getMetricValue(const std::string& metric) const;

	void stop();
};

}  // namespace RBX
