#pragma once

#include "Util/AsyncHttpQueue.h"
#include "v8datamodel/DataModelJob.h"
#include "rbx/threadsafe.h"

namespace RBX
{
	class DataModel;

class ContentProviderJob : public DataModelJob
{	
public:
	enum ExecutionMode
	{
		JobMode,
		ImmediateMode
	};

private:
	boost::function<TaskScheduler::StepResult(std::string,shared_ptr<const std::string>)> processFunc;
	boost::function<void(std::string)> errorFunc;

	struct ContentProviderTask
	{
		std::string id;
		shared_ptr<const std::string> data;
	};
	bool aborted;
	rbx::safe_queue<ContentProviderTask> tasks;
	ExecutionMode execMode;

	TaskScheduler::StepResult processTask(const ContentProviderTask& task);
public:

	ContentProviderJob(shared_ptr<DataModel> dataModel, const char* name,
					   boost::function<TaskScheduler::StepResult(std::string,shared_ptr<const std::string>)> processFunc,
					   boost::function<void(std::string)> errorFunc);

	/*override*/ Time::Interval sleepTime(const Stats& stats);
	/*override*/ Job::Error error(const Stats& stats);
	/*override*/ TaskScheduler::StepResult stepDataModelJob(const Stats& stats);


	void abort();
	void addTask(const std::string& id, AsyncHttpQueue::RequestResult result, std::istream* filestream, shared_ptr<const std::string> data);
	void setExecutionMode(ExecutionMode execMode);
};


}