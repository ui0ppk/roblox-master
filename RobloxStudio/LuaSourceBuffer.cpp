#include "stdafx.h"
#include "LuaSourceBuffer.h"

#include "Script/ModuleScript.h"
#include "Script/Script.h"
#include "Script/ScriptContext.h"
#include "Util/ContentId.h"
#include "Util/ProtectedString.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/DataModel.h"
#include "V8Tree/Service.h"

#include "RobloxDocManager.h"
#include "RobloxGameExplorer.h"
#include "RobloxIDEDoc.h"
#include "RobloxSettings.h"
#include "UpdateUIManager.h"


using namespace boost;
using namespace RBX;

LuaSourceBuffer LuaSourceBuffer::fromInstance(shared_ptr<Instance> instance)
{
	if (shared_ptr<Script> script = Instance::fastSharedDynamicCast<Script>(instance))
	{
		return LuaSourceBuffer(script);
	}
	else if (shared_ptr<ModuleScript> moduleScript = Instance::fastSharedDynamicCast<ModuleScript>(instance))
	{
		return LuaSourceBuffer(moduleScript);
	}
	else
	{
		return LuaSourceBuffer();
	}
}

LuaSourceBuffer LuaSourceBuffer::fromContentId(const ContentId& contentId)
{
	if (!contentId.isNull())
	{
		return LuaSourceBuffer(contentId);
	}
	else
	{
		return LuaSourceBuffer();
	}
}

HttpFuture LuaSourceBuffer::getScriptAssetSource(const ContentId& contentId, int universeId)
{
	RobloxGameExplorer& rge = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);

	if (boost::optional<std::string> source = rge.findCachedAssetOrEmpty(contentId, universeId))
	{
		boost::promise<std::string> prom;
		prom.set_value(source.get());
		return HttpFuture(prom.get_future());
	}

	ContentId copy = contentId;
	const std::string baseUrl = RobloxSettings::getBaseURL().toStdString();
	copy.convertAssetId(baseUrl, universeId);
	copy.convertToLegacyContent(baseUrl);
	HttpOptions options;
	options.setDoNotUseCachedResponse();
	return HttpAsync::get(copy.toString(), options);
}

LuaSourceBuffer::LuaSourceBuffer()
{
	RBXASSERT(empty());
}

LuaSourceBuffer::LuaSourceBuffer(shared_ptr<Script> script)
	: script(script)
{
}

LuaSourceBuffer::LuaSourceBuffer(shared_ptr<ModuleScript> moduleScript)
	: moduleScript(moduleScript)
{
}

LuaSourceBuffer::LuaSourceBuffer(const ContentId& contentId)
	: contentId(contentId)
{
	RBXASSERT(!contentId.isNull());
}

shared_ptr<LuaSourceContainer> LuaSourceBuffer::toInstance() const
{
	return script ? shared_ptr<LuaSourceContainer>(script) : shared_ptr<LuaSourceContainer>(moduleScript);
}

bool LuaSourceBuffer::isNamedAsset() const
{
	return contentId.isNamedAsset();
}

bool LuaSourceBuffer::isModuleScript() const
{
	return moduleScript != NULL;
}

void LuaSourceBuffer::reloadLiveScript()
{
	if (moduleScript)
	{
		DataModel* dm = DataModel::get(moduleScript.get());
		dm->create<ScriptContext>()->reloadModuleScript(moduleScript);
	}
	else if (script)
	{
		DataModel* dm = DataModel::get(script.get());
		if (!script->getDisabled())
		{
			script->setDisabled(true);
			script->setDisabled(false);
		}
	}
}

bool LuaSourceBuffer::isCodeEmbedded() const
{
    if (script)
        return script->isCodeEmbedded();

    if (moduleScript)
        return true;

    return false;
}

bool LuaSourceBuffer::sourceAvailable() const
{
	if (empty())
		return false;

	if (script || moduleScript)
		return true;

	try
	{
		getScriptText();
		return true;
	}
	catch (const RBX::base_exception&)
	{
		return false;
	}
}

std::string LuaSourceBuffer::getScriptText() const
{
	RBXASSERT(!empty());

	if (script)
	{
		return script->getEmbeddedCodeSafe().getSource();
	}
	else if (moduleScript)
	{
		return moduleScript->getSource().getSource();
	}
	else if (!contentId.isNull())
	{
		return getScriptAssetSource(contentId, UpdateUIManager::Instance()
			.getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER).getCurrentGameId()).get();
	}
    else
    {
        RBXASSERT(false);
        return "";
    }
}

void LuaSourceBuffer::setScriptText(const std::string& newText)
{
	RBXASSERT(!empty());
	if (script)
	{
		script->setEmbeddedCode(ProtectedString::fromTrustedSource(newText));
	}
	else if (moduleScript)
	{
		moduleScript->setSource(ProtectedString::fromTrustedSource(newText));
	}
	else if (!contentId.isNull())
	{
		UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER)
			.updateAsset(contentId, newText);
	}
}

std::string LuaSourceBuffer::getScriptName() const
{
	RBXASSERT(!empty());
	if (shared_ptr<Instance> inst = toInstance())
	{
		return inst->getName();
	}
	else
	{
		if (contentId.isNamedAsset())
		{
			return contentId.getAssetName();
		}
		else
		{
			return contentId.toString();
		}
	}
}

std::string LuaSourceBuffer::getFullName() const
{
	RBXASSERT(!empty());
	if (shared_ptr<RBX::Instance> inst = toInstance())
	{
		return inst->getFullName();
	}
	else
	{
		return contentId.toString();
	}
}

bool LuaSourceBuffer::empty() const
{
	return !(script || moduleScript || !contentId.isNull());
}

bool LuaSourceBuffer::operator==(const LuaSourceBuffer& other) const
{
	return script == other.script && moduleScript == other.moduleScript && 
		contentId == other.contentId;
}

bool LuaSourceBuffer::operator!=(const LuaSourceBuffer& other) const
{
	return !(*this == other);
}
