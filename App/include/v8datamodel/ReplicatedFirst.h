#pragma once

#include "V8Tree/Service.h"
#include "script/IScriptFilter.h"

namespace RBX {

	extern const char* const sReplicatedFirst;

	class ReplicatedFirst
		: public DescribedCreatable<ReplicatedFirst, Instance, sReplicatedFirst, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		, public Service
		, public IScriptFilter
	{
	public:
		ReplicatedFirst();

        rbx::signal<void()> finishedReplicatingSignal;
		rbx::signal<void()> removeDefaultLoadingGuiSignal;

		void doRemoveDefaultLoadingGui();
		bool getIsDefaultLoadingGuiRemoved() { return isDefaultLoadingGuiRemoved; }
        
        bool getIsFinishedReplicating() { return allInstancesHaveReplicated; }

		void setAllInstancesHaveReplicated();
		bool getAllInstancesHaveReplicated() { return allInstancesHaveReplicated; }

		void startLocalScript(shared_ptr<Instance> instance);

		void gameIsLoaded();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IScriptFilter
		/*override*/ bool scriptShouldRun(BaseScript* script);
	private:
		typedef DescribedCreatable<ReplicatedFirst, Instance, sReplicatedFirst, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;

		bool isDefaultLoadingGuiRemoved;
		bool allInstancesHaveReplicated;
		bool removeDefaultLoadingGuiOnGameLoaded;

		void startLocalScripts();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const { return false; }
		/*override*/ bool askForbidParent(const Instance* instance) const { return !askSetParent(instance); }
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const { return !askAddChild(instance); }
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};

}
