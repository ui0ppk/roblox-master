#include "stdafx.h"

#include "v8datamodel/debrisservice.h"
#include "v8datamodel/TimerService.h"
#include "util/standardout.h"

const char* const RBX::sTimerService = "TimerService";

using namespace RBX;


TimerService::TimerService(void)
{
}

void TimerService::delay(boost::function0<void> func, double seconds)
{
	Item item;
	item.time = Time::now<Time::Fast>() + Time::Interval(seconds);
	item.func = func;

	// Start searching for the right place from the end of the list
	// items going off later go in the back of the list
	
	bool inserted = false;
	for (std::list<Item>::iterator iter = items.begin(); iter != items.end(); iter++)
	{
		if (iter->time > item.time)
		{
			items.insert(iter, item);
			inserted = true;
			break;
		}
	}

	if (!inserted) 
		items.push_back(item);
}


void TimerService::onHeartbeat(const Heartbeat& heartbeat)
{
	// Fire off each delayed functor whose time has come
	Time time = Time::now<Time::Fast>();
	while (items.size()>0)
	{
		const Item& item = items.front();
		if (time>=item.time)
		{
			try
			{
				item.func();
			}
			catch (RBX::base_exception& e)
			{
				StandardOut::singleton()->print(MESSAGE_WARNING, e);
			}
			items.pop_front();
		}
		else
			break;
	}
}
