/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/HeartbeatInstance.h"
#include "Util/RunStateOwner.h"

namespace RBX {

void HeartbeatInstance::onServiceProviderHeartbeatInstance(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	RBXASSERT((oldProvider == NULL) != (newProvider == NULL));

	heartbeatConnection.disconnect();

	if (newProvider) {
		RunService* runService = ServiceProvider::create<RunService>(newProvider);
		RBXASSERT(runService);
		heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&HeartbeatInstance::onHeartbeat, this, _1));
	}
}

} // namespace