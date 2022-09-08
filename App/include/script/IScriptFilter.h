#pragma once

#include "util/RunStateOwner.h"

namespace RBX
{
	class BaseScript;
	class ModuleScript;

	// Interface for Instances that turn in-game Scripts on and off
	// Implementations can add/remove the script from ScriptContext
	class RBXInterface IScriptFilter
	{
		friend class BaseScript;
	protected:
		// If script should run - pass back the IScriptOwner who should run it, otherwise NULL
		virtual bool scriptShouldRun(BaseScript* script) = 0;
	};

	extern const char *const sRuntimeScriptService;

	class RuntimeScriptService 
		: public DescribedNonCreatable<RuntimeScriptService, Instance, sRuntimeScriptService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<RuntimeScriptService, Instance, sRuntimeScriptService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

	public:
		RuntimeScriptService():isRunning(false)
		{
		}

		void runScript(BaseScript* script);
		void releaseScript(BaseScript* script);

	protected:
		virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	private:
		rbx::signals::scoped_connection runTransitionConnection;
		std::set<weak_ptr<BaseScript> > pendingScripts;		// holds Scripts that are waiting for "Run"
		std::set<weak_ptr<BaseScript> > runningScripts;

		bool isRunning;

		void onRunTransition(RunTransition event)
		{
			onRunState(event.newState);
		}
		void onRunState(RunState state);
	};
}
