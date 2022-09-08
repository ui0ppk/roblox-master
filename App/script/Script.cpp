#include "stdafx.h"
#include "script/script.h"
#include "v8datamodel/ContentProvider.h"
#include "Util/AsyncHttpQueue.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/ScriptInformationProvider.h"
#include "Util/MD5Hasher.h"
#include "v8datamodel/Tool.h"
#include "Network/api.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "script/ScriptContext.h"
#include "Reflection/EnumConverter.h"

#include "script/CoreScript.h"
#include "XStudioBuild.h"

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

namespace RBX
{
    const char* const sBaseScript	= "BaseScript";
	const char* const sScript		= "Script";
	const char* const sLocalScript	= "LocalScript";
	
	std::string BaseScript::adminScriptsPath;
}

using namespace RBX;

const Reflection::PropDescriptor<Script, ProtectedString> Script::prop_EmbeddedSourceCode("Source", category_Data, &Script::getEmbeddedCodeSafe, &Script::setEmbeddedCode, Reflection::PropertyDescriptor::STANDARD, Security::Plugin );
const Reflection::PropDescriptor<BaseScript, ScriptId> BaseScript::prop_SourceCodeId("LinkedSource", category_Data, &BaseScript::getScriptId, &BaseScript::setScriptId);
const Reflection::PropDescriptor<BaseScript, bool> BaseScript::prop_Disabled("Disabled", category_Behavior, &BaseScript::getDisabled, &BaseScript::setDisabled);
const Reflection::BoundFuncDesc<Script, std::string()> Script::func_GetHash(&Script::getHash, "GetHash", Security::RobloxPlace);

BaseScript::BaseScript()
	:workspace(NULL)
	,disabled(false)
	,badLinkedScript(false)
{
	setName(sBaseScript);
}

Script::Script()
	:DescribedCreatable<Script, BaseScript, sScript>()
{
	setName(sScript);
}

LocalScript::LocalScript()
	:DescribedCreatable<LocalScript, Script, sLocalScript>()
{
	setName(sLocalScript);
}

BaseScript::~BaseScript()
{
}

Script::~Script()
{
}

const boost::flyweight<ProtectedString>& Script::getEmbeddedCode() const 
{
    return embeddedSource;
}

const ProtectedString& Script::getEmbeddedCodeSafe() const 
{ 
    return embeddedSource;
}

void Script::setEmbeddedCode(const ProtectedString& value)
{
	if (embeddedSource.get() != value)
	{
        embeddedSource = value;
        embeddedSourceHash = ComputeMd5Hash(value.getSource());

		restartScript();

		raisePropertyChanged(prop_EmbeddedSourceCode);
	}
}

bool BaseScript::hasCoreScriptReplacements()
{
	if (adminScriptsPath.empty())
	{
		return false;
	}

	fs::path testPath(adminScriptsPath);
#if !ENABLE_XBOX_STUDIO_BUILD
	testPath /= "StarterScript.lua";
#else
    testPath /= "XStarterScript.lua";
#endif
	return fs::exists(testPath);
}

void BaseScript::setDisabled(bool value)
{
	if(disabled != value)
	{
		disabled = value;

		restartScript();

		raisePropertyChanged(prop_Disabled);
	}
}

void BaseScript::onScriptIdChanged()
{
	badLinkedScript = false;
	restartScript();
	raisePropertyChanged(prop_SourceCodeId);
}

RuntimeScriptService* BaseScript::computeNewWorkspace()
{
	if(isDisabled()){
		return NULL;
	}
	bool canRun = false;
	Instance* parent = getParent();		// Find the new script owner

	while (parent && !canRun) {
		if (IScriptFilter* controller = dynamic_cast<IScriptFilter*>(parent)) {
			canRun = controller->scriptShouldRun(this);
		}
		parent = parent->getParent();
	}

	if (canRun) {
		RuntimeScriptService* answer = ServiceProvider::create<RuntimeScriptService>(this);
		//answer might be NULL if we're shutting down and the script was not in the workspace
		return answer;
	}
	else {
		return NULL;
	}
}

void BaseScript::restartScript()
{
	RuntimeScriptService* newWorkspace = computeNewWorkspace();

	if(workspace != newWorkspace){
		if (workspace) {
			RuntimeScriptService* temp = workspace;
			workspace = NULL;
			temp->releaseScript(this);
		}

		workspace = newWorkspace;

		if(workspace){
			workspace->runScript(this);
		}
	}
}

void BaseScript::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	RuntimeScriptService* newWorkspace = computeNewWorkspace();

	if (workspace != newWorkspace)
	{
		if (workspace) {
			RuntimeScriptService* temp = workspace;
			workspace = NULL;
			temp->releaseScript(this);
		}

		workspace = newWorkspace;

		if (workspace) {
			workspace->runScript(this);
		}
	}
}

void BaseScript::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		if (workspace)
		{
			RuntimeScriptService* temp = workspace;
			workspace = NULL;
			temp->releaseScript(this);
		}
	}

	Super::onServiceProvider(oldProvider, newProvider);
}

BaseScript::Code BaseScript::requestCode(ScriptInformationProvider* scriptInfoProvider)
{
	RBXASSERT(getCachedRemoteSourceLoadState() != NotAttemptedToLoad);
	if (getCachedRemoteSourceLoadState() == Loaded)
	{
		return Code(boost::flyweight<ProtectedString>(getCachedRemoteSource()));
	}
	else
	{
		return Code();
	}
}

BaseScript::Code Script::requestCode(ScriptInformationProvider* scriptInfoProvider)
{
	if (isCodeEmbedded())
	{
		return Code(embeddedSource);
	}
	else
	{
		return Super::requestCode(scriptInfoProvider);
	}
}

const std::string BaseScript::emptyString;

const std::string& BaseScript::requestHash() const
{
	return emptyString;
}

const std::string& Script::requestHash() const
{
	if (isCodeEmbedded())
	{
        return embeddedSourceHash;
	}
	return Super::requestHash();
}

/*override*/ int Script::getPersistentDataCost() const 
{
	return Super::getPersistentDataCost() + (isCodeEmbedded() ? Instance::computeStringCost(getEmbeddedCode().get().getSource()) : 0);
}

void Script::fireSourceChanged()
{
	raisePropertyChanged(prop_EmbeddedSourceCode);
}
