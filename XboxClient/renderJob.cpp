
#include "renderJob.h"


#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/DataModel.h"
#include "rbx/rbxTime.h"

#include "GfxBase/ViewBase.h"
#include "GfxBase/FrameRateManager.h"
#include "RenderSettingsItem.h"

#include "marshaller.h"
#include "rbx/Profiler.h"

RenderJob::RenderJob(RBX::ViewBase* v, Marshaller* m)
: RBX::BaseRenderJob(CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), shared_from(v->getDataModel()) )
{
    view = v;
    marshaller = m;
    stopped = 0;
}

void RenderJob::stop()
{
    stopped = 1;
}

static void scheduleRenderPerform(const weak_ptr<RenderJob>& selfWeak, RBX::ViewBase* view, double timeJobStart)
{
    if (shared_ptr<RenderJob> self = selfWeak.lock())
    {
        view->renderPerform(timeJobStart);
        self->wake();
    }
}

RBX::TaskScheduler::StepResult RenderJob::stepDataModelJob(const Stats& stats)
{
    RBXPROFILER_SCOPE("Jobs", __FUNCTION__);
    if (stopped)
	{
        return RBX::TaskScheduler::Done;
	}

    RBX::DataModel* dm = view->getDataModel();
    RBX::FrameRateManager* frm = view->getFrameRateManager();
    double seconds = 0.016;

    if(1)
    {
        RBX::DataModel::scoped_write_request request(dm);

        if(frm) seconds = frm->GetFrameTimeStats().getLatest() / 1000.f;
        dm->renderStep(seconds);

        if( frm )
            frm->configureFrameRateManager(RBX::CRenderSettings::FrameRateManagerAuto, true);

        isAwake = false;

        seconds = RBX::Time::nowFastSec();
        marshaller->execute( boost::bind(&RBX::ViewBase::renderPrepare, view, this) );
    }
    marshaller->submit(boost::bind(&scheduleRenderPerform, weak_from(this), view, seconds));
    return RBX::TaskScheduler::Stepped;
}


RBX::Time::Interval RenderJob::sleepTime(const Stats& stats)
{
    if (isAwake)
        return computeStandardSleepTime(stats, maxFrameRate);
    else
        return RBX::Time::Interval::max();
}

std::string RenderJob::getMetric(const std::string& metric) const
{
    return "";
}

double RenderJob::getMetricValue(const std::string& metric) const
{
    return 0;
}
