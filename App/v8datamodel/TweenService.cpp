#include "stdafx.h"

#include "V8DataModel/TweenService.h"
#include "V8DataModel/GuiObject.h"
#include "util/RunStateOwner.h"
#include "network/Players.h"

namespace RBX
{
const char* const sTweenService = "TweenService";

TweenService::TweenService() :
	IStepped(StepType_Render)
{
	setName(sTweenService);
}
void TweenService::addTweeningObject(boost::weak_ptr<GuiObject> guiObject)
{
	if(tweeningObjects.find(guiObject) == tweeningObjects.end())
	{
		tweeningObjects.insert(guiObject);
	}
}

void TweenService::addTweenCallback(boost::function<void(GuiObject::TweenStatus)> tweenCallbackFunc, GuiObject::TweenStatus tweenStatusForCallback)
{
	tweenCallbacks.push_back(std::pair<boost::function<void(GuiObject::TweenStatus)>, GuiObject::TweenStatus>(tweenCallbackFunc, tweenStatusForCallback) );
}

/*override*/ void TweenService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		tweenCallbacks.clear();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
    if(newProvider && RBX::Network::Players::serverIsPresent(newProvider))
	{
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);
	}
	else
	{
		onServiceProviderIStepped(oldProvider, newProvider);
	}
}

void TweenService::onStepped(const Stepped& event)
{
	update(event.gameStep);
}

void TweenService::onHeartbeat(const Heartbeat& heartbeatEvent)
{
	update(heartbeatEvent.wallStep);
}

void TweenService::update(const double step)
{	
	// call all the tween callbacks (for tweens that have finished/been cancelled)
	TweenCallbacks temp;
	tweenCallbacks.swap(temp);
	for(TweenCallbacks::iterator iter = temp.begin(); iter != temp.end(); iter++)
	{
		iter->first(iter->second);
	}

	// Step all the "tweening" objects
	for(TweeningObjectsList::iterator iter = tweeningObjects.begin(); iter != tweeningObjects.end();)
	{
		if(shared_ptr<GuiObject> guiObj = iter->lock())
		{
			if(guiObj->tweenStep(step))
			{
				//Return true when we are done with tweening
				tweeningObjects.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
		else
		{
			tweeningObjects.erase(iter++);
		}
	}
}

} // namespace RBX