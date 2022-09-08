#include "stdafx.h"

#include "Util/ContentProviderJob.h"
#include "v8datamodel/DataModel.h"

#include "rbx/Profiler.h"

DYNAMIC_FASTINTVARIABLE(MaxContentProviderRunsPerStep, 10)
DYNAMIC_FASTINTVARIABLE(MaxContentProviderRunsAccumulated, 20)

LOGGROUP(NetworkStepsMultipliers)

namespace RBX
{
ContentProviderJob::ContentProviderJob(shared_ptr<DataModel> dataModel, const char* name,
									   boost::function<TaskScheduler::StepResult(std::string,shared_ptr<const std::string>)> processFunc,
									   boost::function<void(std::string)> errorFunc)
	:DataModelJob(name, DataModelJob::None, false, dataModel, Time::Interval(0.01))
	,processFunc(processFunc)
	,errorFunc(errorFunc)
	,execMode(JobMode)
	,aborted(false)
{
	cyclicExecutive = true;
}

void ContentProviderJob::setExecutionMode(ExecutionMode execMode)
{
	this->execMode = execMode;
}

void ContentProviderJob::abort()
{
	aborted = true;
}
void ContentProviderJob::addTask(const std::string& id, 
								 AsyncHttpQueue::RequestResult result, std::istream* filestream, shared_ptr<const std::string> data)
{
	try
	{
		if(result == AsyncHttpQueue::Succeeded){
			if(!data){
				std::ostringstream strbuffer;
				strbuffer << filestream->rdbuf();
				data.reset(new std::string(strbuffer.str()));
			}
		}
		else{
			data.reset();
		}

		ContentProviderTask task;
		task.id = id;
		task.data = data;

		switch(execMode)
		{
		case JobMode:
			tasks.push(task);
			break;
		case ImmediateMode:
			processTask(task);
			break;
		}
	}
	catch(RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s failed to load %s because %s", name.c_str(), id.c_str(), e.what());
		errorFunc(id);
	}
}

Time::Interval ContentProviderJob::sleepTime(const Stats& stats)
{
	return computeStandardSleepTime(stats, 30);
}

TaskScheduler::Job::Error ContentProviderJob::error(const Stats& stats)
{
	Job::Error result;
	result.error = tasks.size() * stats.timespanSinceLastStep.seconds();
	return result;
}
TaskScheduler::StepResult ContentProviderJob::processTask(const ContentProviderTask& task)
{
	try
	{
		if(processFunc(task.id, task.data) == TaskScheduler::Done)
			return TaskScheduler::Done;
	}
	catch(RBX::base_exception& e)
	{
		errorFunc(task.id);
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s failed to process %s because %s", name.c_str(), task.id.c_str(), e.what());
	}
	return TaskScheduler::Stepped;
}
TaskScheduler::StepResult ContentProviderJob::stepDataModelJob(const Stats& stats)
{
	ContentProviderTask task;
	// If Job is not CyclicExecutive, we still need logic to Catch-up
	if (TaskScheduler::singleton().isCyclicExecutive())
	{
		float tasksToRun = updateStepsRequiredForCyclicExecutive(stats.timespanSinceLastStep.seconds(), 
																	tasks.size(), 
																	(float) DFInt::MaxContentProviderRunsPerStep, 
																	(float) DFInt::MaxContentProviderRunsAccumulated);

		FASTLOG1(FLog::NetworkStepsMultipliers, "Content Provider: Running %d tasks", (int)tasksToRun);
		for (int i = 0; i < (int)tasksToRun; i++)
		{
			if (!aborted && tasks.pop_if_present(task)) 
			{
                RBXPROFILER_SCOPE("Content", "processTask");
                
				processTask(task);
			}
		}

		return TaskScheduler::Stepped;
	}
	else
	{
		if (!aborted && tasks.pop_if_present(task)) {
			if(processTask(task) == TaskScheduler::Done)
				return TaskScheduler::Done;
			}
	}

	return TaskScheduler::Stepped;
}

}


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag1 = 0;
    };
};
