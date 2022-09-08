/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/RunStateOwner.h"
#include "Util/G3DCore.h"

LOGGROUP(ISteppedLifetime)

namespace RBX {
	
	class IStepped
	{
    public:
        enum StepType
        {
            StepType_Default,
            StepType_HighPriority,
            StepType_Render,
        };

	private:
		StepType stepType;
		rbx::signals::scoped_connection steppedConnection;

	protected:
		// call this inside onServiceProvider
		void onServiceProviderIStepped(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		/*implement*/ virtual void onStepped(const Stepped& event) = 0;

		void stopStepping() {
			steppedConnection.disconnect();
		}

	public:
		IStepped(StepType stepType = StepType_Default): stepType(stepType) {}
		virtual ~IStepped()	{}
	};
}	// namespace RBX