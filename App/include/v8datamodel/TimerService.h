#pragma once

#include "V8Tree/Service.h"
#include "util/runstateowner.h"
#include "util/RunningAverage.h"
#include "Util/HeartbeatInstance.h"
#include <queue>

namespace RBX {

	// A Utility class for scheduling operations in game time
	// TODO: Create signal for running repeated operations
	// TODO: Should this evolve into a full-blown, thread-aware task scheduler?
extern const char* const sTimerService;
class TimerService 
	: public DescribedCreatable<TimerService, Instance, sTimerService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
	, public Service
	, public HeartbeatInstance
{
private:
	typedef DescribedCreatable<TimerService, Instance, sTimerService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;

	class Item
	{			
	public:
		Time time;
		boost::function0<void> func;
	};
	std::list<Item> items;		// func list sorted in ascending time order

	// Instance - add hook for HeartbeatInstance
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);
	}

	// HeartbeatInstance
	/*override*/ void onHeartbeat(const Heartbeat& heartbeat);

public:
	TimerService();

	// Request that func be called once after specified time has elapsed
	void delay(boost::function0<void> func, double seconds);
};

}
