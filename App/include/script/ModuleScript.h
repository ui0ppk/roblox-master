#pragma once

#include "Reflection/reflection.h"
#include "Script/ThreadRef.h"
#include "Script/LuaSourceContainer.h"
#include "Util/ProtectedString.h"
#include "V8Tree/Instance.h"

#include <boost/intrusive_ptr.hpp>

#include <vector>

namespace RBX
{

extern const char* const sModuleScript;
class ModuleScript
	: public DescribedCreatable<ModuleScript, LuaSourceContainer, sModuleScript>
{
public:
	static const Reflection::PropDescriptor<ModuleScript, ProtectedString> prop_Source;

	enum ScriptSetupState
	{
		NotRunYet = 0,
		Running = 1,
		CompletedError = 2,
		CompletedSuccess = 3
	};

	class PerVMState
	{
	public:
		PerVMState();
		virtual ~PerVMState();

		int getResultRegistryIndex() const;

		// Destroy the current result index and replace it with index.
		void reassignResultRegistryIndex(int newIndex);

		void setRunning(boost::intrusive_ptr<Lua::WeakThreadRef::Node> node);
		void setCompletedError();
		void setCompletedSuccess(lua_State* globalStateContainingResult, int resultRegistryIndex);
		ScriptSetupState getCurrentState() const;

		void addYieldedImporter(Lua::WeakThreadRef L);
		void getAndClearYieldedImporters(std::vector<Lua::WeakThreadRef>* out);

		void cleanupAndResetState();
		void resetState();
	private:
		ScriptSetupState scriptLoadingState;
		boost::intrusive_ptr<Lua::WeakThreadRef::Node> node;
		lua_State* globalStateContainingResult;
		int resultRegistryIndex;
		std::vector<Lua::WeakThreadRef> yieldedImporters;

		void releaseReferenceIfCompletedSuccessfully();
		void releaseScriptNodeIfPresent();
	};

	ModuleScript();

	// Instance
	bool askSetParent(const Instance* instance) const override { return true; }

	ProtectedString getSource() const;
	void setSource(const ProtectedString& newText);

	std::string requestHash() const;

	PerVMState& vmState(lua_State* vm);

	// Try to get rid of this method once new play button is launched.
	static void cleanupAndResetState(const weak_ptr<ModuleScript> module);

    // Reset the state of the module script without destroying its result index.
    void resetState();

    void setReloadRequested(bool reload) { reloadRequested = reload; }
    bool getReloadRequested() const { return reloadRequested; }

	void fireSourceChanged() override;

	rbx::signal<void(lua_State*)> starting;
    
protected:
	void onScriptIdChanged() override;

private:
	ProtectedString source;
	bool reloadRequested;
	typedef boost::unordered_map<lua_State*, PerVMState> VMStateMap;
	VMStateMap stateMap;
};

} // namespace
