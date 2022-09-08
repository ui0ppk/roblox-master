#pragma once

#include "v8datamodel/BaseRenderJob.h"

class Marshaller;

namespace RBX 
{
    class View;
    class ViewBase;
}


// This job calls ViewBase::render(), which needs to be done exclusive to the
// DataModel. This is why it has the DataModelJob::Render enum, which
// prevents concurrent writes to DataModel. It also needs to run in the view's
// thread for OpenGL.
// TODO: Can Ogre be modified to not require the thread?
class RenderJob : public RBX::BaseRenderJob, public RBX::IMetric
{
    Marshaller*      marshaller;
    RBX::ViewBase*   view;
    volatile int     stopped;

public:
	RenderJob(RBX::ViewBase* robloxView, Marshaller* marshaller);

    virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats);
    virtual RBX::Time::Interval RenderJob::sleepTime(const Stats& stats);

    virtual std::string getMetric(const std::string& metric) const;
    virtual double getMetricValue(const std::string& metric) const;

    void stop();
};

