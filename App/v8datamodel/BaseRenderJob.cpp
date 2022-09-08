#include "stdafx.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/Workspace.h"

#include "rbx/Profiler.h"

LOGGROUP(TaskSchedulerTiming)

namespace RBX 
{

BaseRenderJob::BaseRenderJob(double minFps, double maxFps, boost::shared_ptr<DataModel> dataModel)
	: RBX::DataModelJob("Render", RBX::DataModelJob::Render, false, dataModel, RBX::Time::Interval(.02))
	, isAwake(true)
	, minFrameRate(minFps)
	, maxFrameRate(maxFps)
	{
		cyclicExecutive = true;
		// We originally intended RenderJob to run after Network, Physics, and so on
		// to reduce latency, however this introduced a weird variability into the dt between
		// each RenderJob::step. Since makes sure that the first job is always 16 - 17ms apart from
		// it's previous run, making RenderJob run first will grant more visual stability at the cost
		// of potentially some latency in input.
		cyclicPriority = CyclicExecutiveJobPriority_EarlyRendering;
	}


void BaseRenderJob::wake()
{
    isAwake = true;
	if (!isCyclicExecutiveJob())
		TaskScheduler::singleton().reschedule(shared_from_this());
}

bool BaseRenderJob::tryJobAgain()
{
	if (isCyclicExecutiveJob() && !isAwake)
	{
		return true;
	}
	return false;
}

bool BaseRenderJob::isCyclicExecutiveJob()
{
	return RBX::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive;
}

TaskScheduler::Job::Error BaseRenderJob::error(const Stats& stats)
{
	if (!isAwake && (!isCyclicExecutiveJob()))
	{
		return Job::Error();
	}

	Job::Error result;
	if ( isCyclicExecutiveJob() )
	{
		result = computeStandardError(stats, maxFrameRate);
		FASTLOG1F(FLog::TaskSchedulerTiming, "Error recalculated, time since last call: %f", (float)stats.timespanSinceLastStep.seconds());
	}
	else
	{
		result = computeStandardError(stats, minFrameRate);
	}
	return result;
}

Time::Interval BaseRenderJob::timeSinceLastRender() const
{
	return Time::now<RBX::Time::Fast>() - lastRenderTime;
}

TaskScheduler::StepResult BaseRenderJob::step(const Stats& stats)
{
	Profiler::onFrame();

	return DataModelJob::step(stats);
}

}
