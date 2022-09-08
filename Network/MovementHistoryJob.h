#pragma once

#include "V8DataModel/DataModelJob.h"

namespace RBX { 

	class PartInstance;
	class DataModel;

namespace Network {

	class Server;
	class ServerReplicator;

	class MovementHistoryJob : public DataModelJob
	{
	private:
		boost::weak_ptr<DataModel> dataModel;
		float movementHistoryRate;

		// Job overrides
		/*override*/ virtual Error error(const Stats& stats);
		/*override*/ Time::Interval sleepTime(const Stats& stats);
		/*override*/ virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);

	public:
		MovementHistoryJob(shared_ptr<DataModel> dataModel);
	};
	
}
}

