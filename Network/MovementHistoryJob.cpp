
#include "MovementHistoryJob.h"
#include "NetworkSettings.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "Util.h"


using namespace RBX;
using namespace RBX::Network;

MovementHistoryJob::MovementHistoryJob(shared_ptr<DataModel> dataModel)
	: DataModelJob("Movement History Job", DataModelJob::Write, false, dataModel, Time::Interval(0))
	, dataModel(dataModel)
	, movementHistoryRate(20)
{
	cyclicExecutive = true;
	cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessIncoming;
}

Time::Interval MovementHistoryJob::sleepTime(const Stats& stats)
{
	return computeStandardSleepTime(stats, movementHistoryRate);
}

TaskScheduler::Job::Error MovementHistoryJob::error(const Stats& stats)
{
	return TaskScheduler::Job::computeStandardErrorCyclicExecutiveSleeping(stats, movementHistoryRate);
}


TaskScheduler::StepResult MovementHistoryJob::stepDataModelJob(const Stats& stats) 
{
    bool deprecated = true;
    RBXASSERT(!deprecated);

    if(shared_ptr<DataModel> safeDataModel = dataModel.lock()){
        DataModel::scoped_write_request request(safeDataModel.get());

        safeDataModel->getWorkspace()->updateHistory();

        return TaskScheduler::Stepped;
    }
	return TaskScheduler::Done;
}
