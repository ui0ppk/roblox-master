/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Platform.h"

namespace RBX {

const char* const sPlatform = "Platform";

REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<Platform, void(shared_ptr<Instance>)> event_createPlatformMotor6D(&Platform::createPlatformMotor6DSignal, "RemoteCreateMotor6D", "humanoid", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY,	Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Platform, void()> event_destroyPlatformMotor6D(&Platform::destroyPlatformMotor6DSignal, "RemoteDestroyMotor6D", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

void Platform::createPlatformMotor6D(Humanoid *h)
{
	event_createPlatformMotor6D.fireAndReplicateEvent(this, shared_from(h));

}

void Platform::findAndDestroyPlatformMotor6D()
{
	event_destroyPlatformMotor6D.fireAndReplicateEvent(this);
}

} // namespace