/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/HeartbeatInstance.h"
#include "util/SteppedInstance.h"
#include "v8datamodel/GuiObject.h"

namespace RBX
{
	extern const char* const sTweenService;
	class TweenService
		: public DescribedNonCreatable<TweenService, Instance, sTweenService>
		, public Service
		, public HeartbeatInstance
		, public IStepped
	{
	private:
		typedef DescribedNonCreatable<TweenService, Instance, sTweenService> Super;
	public:
		TweenService();

		void addTweeningObject(boost::weak_ptr<GuiObject> guiObject);

		void addTweenCallback(boost::function<void(GuiObject::TweenStatus)> tweenCallbackFunc, GuiObject::TweenStatus tweenStatusForCallback);

	protected:
		typedef std::set<boost::weak_ptr<GuiObject> > TweeningObjectsList;
		
		TweeningObjectsList tweeningObjects;

		/*override*/ void onHeartbeat(const Heartbeat& heartbeatEvent);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	private:
		typedef std::vector<std::pair<boost::function<void(GuiObject::TweenStatus)>, GuiObject::TweenStatus> > TweenCallbacks;

		TweenCallbacks tweenCallbacks;

		void update(const double step);

		// IStepped
		virtual void onStepped(const Stepped& event);
	};
}
