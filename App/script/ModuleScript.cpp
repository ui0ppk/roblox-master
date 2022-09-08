#include "stdafx.h"
#include "Script/ModuleScript.h"
#include "Util/MD5Hasher.h"

#include <lobject.h>
#include <lstate.h>
#include <lauxlib.h>

using namespace RBX;

const char* const RBX::sModuleScript = "ModuleScript";

const Reflection::PropDescriptor<ModuleScript, ProtectedString> ModuleScript::prop_Source(
	"Source", category_Data, &ModuleScript::getSource, &ModuleScript::setSource,
	Reflection::PropertyDescriptor::STANDARD, Security::Plugin);

const Reflection::PropDescriptor<ModuleScript, ContentId> prop_LinkedSource(
	"LinkedSource", category_Data, &ModuleScript::getScriptId, &ModuleScript::setScriptId);

ModuleScript::ModuleScript()
	: source()
    , reloadRequested(false)
{
	setName(sModuleScript);
}

ModuleScript::PerVMState::PerVMState()
	: scriptLoadingState(NotRunYet)
	, node(NULL)
	, globalStateContainingResult(NULL)
	, resultRegistryIndex(LUA_NOREF)
{
}

ModuleScript::PerVMState::~PerVMState()
{
	releaseReferenceIfCompletedSuccessfully();
	releaseScriptNodeIfPresent();
}

void ModuleScript::setSource(const ProtectedString& newSource)
{
	if (source != newSource)
	{
		source = newSource;
		raisePropertyChanged(prop_Source);
	}
}

ProtectedString ModuleScript::getSource() const
{
	return source;
}

std::string ModuleScript::requestHash() const
{
	boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
	hasher->addData(source.getSource());
	return hasher->toString();
}

ModuleScript::PerVMState& ModuleScript::vmState(lua_State* vm)
{
	return stateMap[vm];
}

int ModuleScript::PerVMState::getResultRegistryIndex() const
{
	return resultRegistryIndex;
}

void ModuleScript::PerVMState::setRunning(boost::intrusive_ptr<Lua::WeakThreadRef::Node> node)
{
	RBXASSERT(scriptLoadingState == NotRunYet);
	scriptLoadingState = Running;
	this->node = node;
}

void ModuleScript::PerVMState::setCompletedError()
{
	RBXASSERT(scriptLoadingState == Running);
	scriptLoadingState = CompletedError;
	// Clean up weak thread refs immediately on error (don't wait for ModuleScript
	// instance to be GCed).
	RBXASSERT(node);
	releaseScriptNodeIfPresent();
}

void ModuleScript::PerVMState::setCompletedSuccess(lua_State* globalStateContainingResult, int resultRegistryIndex)
{
	RBXASSERT(scriptLoadingState == Running);
	scriptLoadingState = CompletedSuccess;
    if (this->globalStateContainingResult == NULL)
        this->globalStateContainingResult = globalStateContainingResult;
	this->resultRegistryIndex = resultRegistryIndex;
}

ModuleScript::ScriptSetupState ModuleScript::PerVMState::getCurrentState() const
{
	return scriptLoadingState;
}

void ModuleScript::PerVMState::addYieldedImporter(Lua::WeakThreadRef thread)
{
	RBXASSERT(scriptLoadingState == Running);
	yieldedImporters.push_back(thread);
}

void ModuleScript::PerVMState::getAndClearYieldedImporters(std::vector<Lua::WeakThreadRef>* out)
{
	out->swap(yieldedImporters);
	yieldedImporters.clear();
}

void ModuleScript::cleanupAndResetState(const weak_ptr<ModuleScript> weakModule)
{
	if (shared_ptr<ModuleScript> module = weakModule.lock())
	{
		for (VMStateMap::iterator itr = module->stateMap.begin(); itr != module->stateMap.end(); ++itr)
		{
			itr->second.cleanupAndResetState();
		}
	}
}

void ModuleScript::PerVMState::cleanupAndResetState()
{
	releaseReferenceIfCompletedSuccessfully();
	releaseScriptNodeIfPresent();
	yieldedImporters.clear();
	scriptLoadingState = NotRunYet;
}

void ModuleScript::PerVMState::releaseReferenceIfCompletedSuccessfully()
{
	if (scriptLoadingState == CompletedSuccess)
	{
		RBXASSERT(resultRegistryIndex != LUA_NOREF);
		RBXASSERT(globalStateContainingResult);
		if (globalStateContainingResult != NULL && resultRegistryIndex != LUA_NOREF)
		{
			lua_unref(globalStateContainingResult, resultRegistryIndex);
		}
		globalStateContainingResult = NULL;
		resultRegistryIndex = LUA_NOREF;
	}
}

void ModuleScript::PerVMState::releaseScriptNodeIfPresent()
{
	// Destroy weak thread refs belonging to the thread that ran the module script's code.
	// This mechanism is how rbx::signal connections get cleaned up.
	if (node)
	{
		node->eraseAllRefs();
		node.reset();
	}
}

void ModuleScript::resetState()
{
	for (VMStateMap::iterator itr = stateMap.begin(); itr != stateMap.end(); ++itr)
	{
		itr->second.resetState();
	}
}

void ModuleScript::onScriptIdChanged()
{
	raisePropertyChanged(prop_LinkedSource);
}

void ModuleScript::fireSourceChanged()
{
	raisePropertyChanged(prop_Source);
}

void ModuleScript::PerVMState::resetState()
{
	RBXASSERT(scriptLoadingState != Running);
    resultRegistryIndex = LUA_NOREF;
    releaseScriptNodeIfPresent();
    scriptLoadingState = NotRunYet;
}

void ModuleScript::PerVMState::reassignResultRegistryIndex(int result)
{
    if (globalStateContainingResult != NULL && resultRegistryIndex != LUA_NOREF)
    {
        lua_unref(globalStateContainingResult, resultRegistryIndex);
    }
    
    resultRegistryIndex = result;
}

