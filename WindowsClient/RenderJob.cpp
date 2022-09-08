#include "stdafx.h"
#include "RenderJob.h"

#include "FunctionMarshaller.h"
#include "GfxBase/ViewBase.h"
#include "GfxBase/FrameRateManager.h"
#include "network/api.h"
#include "network/players.h"
#include "rbx/Log.h"
#include "rbx/SystemUtil.h"
#include "RenderSettingsItem.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Workspace.h"
#include "View.h"

#include "VMProtect/VMProtectSDK.h"

FASTFLAG(RenderLowLatencyLoop)

namespace RBX {

RenderJob::RenderJob(View* robloxView, 
					 FunctionMarshaller* marshaller,
					 boost::shared_ptr<DataModel> dataModel)
	: BaseRenderJob(CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), dataModel)
	, robloxView(robloxView)
	, marshaller(marshaller)
	, stopped(0)
	, prepareBeginEvent(false)
	, prepareEndEvent(false)
{
}

void RenderJob::stop()
{
	stopped = 1;
}


Time::Interval RenderJob::timeSinceLastRender() const
{
	return Time::now<Time::Fast>() - lastRenderTime;
}

Time::Interval RenderJob::sleepTime(const Stats& stats)
{
    if (isAwake)
        return computeStandardSleepTime(stats, maxFrameRate);
    else
        return RBX::Time::Interval::max();
}


static void remoteCheatHelper(boost::weak_ptr<DataModel> weakDataModel)
{
	boost::shared_ptr<DataModel> dataModel = weakDataModel.lock();

	if (dataModel)
	{
		// this will send special item to server and server will kick user off
		Network::getSystemUrlLocal(dataModel.get());
	}
}

static void reportHacker(boost::weak_ptr<DataModel> weakDataModel, const char* stat)
{
	if (boost::shared_ptr<DataModel> dataModel = weakDataModel.lock())
	{
		if (Network::Players* players = dataModel->find<Network::Players>())
		{
			if (Network::Player* player = players->getLocalPlayer())
			{
				player->reportStat(stat);
			}
		}
	}
}

void RenderJob::scheduleRender(weak_ptr<RenderJob> selfWeak, ViewBase* view, double timeJobStart)
{
	shared_ptr<RenderJob> self = selfWeak.lock();
	if (!self) return;

	self->prepareBeginEvent.Wait();

	view->renderPrepare(self.get());

	self->prepareEndEvent.Set();

	view->renderPerform(timeJobStart);

	self->wake();
}

static void scheduleRenderPerform(const weak_ptr<RenderJob>& selfWeak, ViewBase* view, double timeJobStart)
{
    if (shared_ptr<RenderJob> self = selfWeak.lock())
    {
        view->renderPerform(timeJobStart);
        self->wake();
    }
}

TaskScheduler::StepResult RenderJob::stepDataModelJob(const Stats& stats)
{
	shared_ptr<DataModel> dm = robloxView->getDataModel();
	if (!dm || stopped)
		return TaskScheduler::Done;

	// Enable security checks for speedhack and attached debugger in release mode
#if !defined(LOVE_ALL_ACCESS) && !defined(RBX_STUDIO_BUILD) && !defined(_NOOPT) && !defined(DEBUG)
	VMProtectBeginMutation("34");
	if (Time::isSpeedCheater())
	{
		dm->submitTask(boost::bind(&reportHacker, boost::weak_ptr<DataModel>(dm),
			"richard"), DataModelJob::Write);
	}
	if (Time::isDebugged())
	{
		dm->submitTask(boost::bind(&reportHacker, boost::weak_ptr<DataModel>(dm),
			"suzanne"), DataModelJob::Write);
	}
	VMProtectEnd();
#endif

	double timeJobStart = Time::nowFastSec();

	ViewBase* view = robloxView->GetGfxView();

	if (FFlag::RenderLowLatencyLoop)
	{
		RBX::DataModel::scoped_write_request request(dm.get());

		const double renderDelta = timeSinceLastRender().seconds();

		lastRenderTime = RBX::Time::now<RBX::Time::Fast>();
		isAwake = false;

		marshaller->Submit(boost::bind(&scheduleRender, weak_from(this), view, timeJobStart));

		view->updateVR();

		dm->renderStep(renderDelta);

		prepareBeginEvent.Set();
		prepareEndEvent.Wait();
	}
	else
	{
		{
			DataModel::scoped_write_request request(robloxView->getDataModel().get());

			view->updateVR();

			float secondsElapsed = robloxView->GetGfxView()->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;

			dm->renderStep(secondsElapsed);	

			isAwake = false;
			marshaller->Execute(boost::bind(&ViewBase::renderPrepare, view, this));

			lastRenderTime = Time::now<Time::Fast>();
		}

		{
			marshaller->Submit(boost::bind(&scheduleRenderPerform, weak_from(this), view, timeJobStart));
		}
	}

	return TaskScheduler::Stepped;
}

// Return information (via IMetric interface, probable asker was DataModel)
std::string RenderJob::getMetric(const std::string& metric) const
{
	if (metric == "Graphics Mode") 
		return RBX::Reflection::EnumDesc<CRenderSettings::GraphicsMode>::singleton().convertToString(robloxView->GetLatchedGraphicsMode());

	if  (metric == "Render") {
		boost::format fmt("%.1f/s %d%%");
		fmt % averageStepsPerSecond() % (int)(100.0 * averageDutyCycle());
		return fmt.str();
	}

	ViewBase* view = robloxView->GetGfxView();
	RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;
	if(frm) {
		if (metric == "FRM")						return (frm && frm->IsBlockCullingEnabled()) ? "On" : "Off";
		if (metric == "Anti-Aliasing")				return (frm && frm->getAntialiasingMode() == CRenderSettings::AntialiasingOn) ? "On" : "Off";
	}

	// If we got here it means we were explicitly asked for information we do 
	// not have.
	RBXASSERT(0);
	return "?";
}

// Return information (via IMetric interface, probable asker was DataModel)
double RenderJob::getMetricValue(const std::string& metric) const
{
	ViewBase* view = robloxView->GetGfxView();

	if (metric == "Render Duty")				return averageDutyCycle();
	if (metric == "Render FPS")					return averageStepsPerSecond();
	if (metric == "Render Job Time")			return averageStepTime();
	if (metric == "Render Nominal FPS")			return 1000.0 / view->getFrameRateManager()->GetRenderTimeAverage();
	if (metric == "Delta Between Renders")		return view->getMetricValue(metric);
	if (metric == "Total Render")				return view->getMetricValue(metric);
	if (metric == "Present Time")				return view->getMetricValue(metric);
	if (metric == "GPU Delay")				    return view->getMetricValue(metric);
	if (metric == "Video Memory")			    return SystemUtil::getVideoMemory();

	// If we got here it means we were explicitly asked for information we do 
	// not have.
	RBXASSERT(0);
	return 0.0;
}

}  // namespace RBX
