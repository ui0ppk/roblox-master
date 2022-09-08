#pragma once

#include "Script/Script.h"
#include "Script/ModuleScript.h"
#include "Script/LuaSourceContainer.h"
#include "Util/ContentId.h"
#include "Util/HttpAsync.h"
#include "Util/ProtectedString.h"


class LuaSourceBuffer
{

public:
	static LuaSourceBuffer fromInstance(boost::shared_ptr<RBX::Instance> instance);
	static LuaSourceBuffer fromContentId(const RBX::ContentId& contentId);
	static RBX::HttpFuture getScriptAssetSource(const RBX::ContentId& contentId, int universeId);

	LuaSourceBuffer();

	boost::shared_ptr<RBX::LuaSourceContainer> toInstance() const;

	bool isNamedAsset() const;
	bool isModuleScript() const;
	void reloadLiveScript();

	bool isCodeEmbedded() const;
	bool sourceAvailable() const;
	std::string getScriptText() const;
	void setScriptText(const std::string& newText);
	std::string getScriptName() const;
	std::string getFullName() const;

	bool empty() const;

	bool operator==(const LuaSourceBuffer& other) const;
	bool operator!=(const LuaSourceBuffer& other) const;

	inline size_t hash_value() const
	{
		return boost::hash_value(script) ^
			boost::hash_value(moduleScript) ^
			boost::hash_value(contentId.toString());
	}

private:
	boost::shared_ptr<RBX::Script> script;
	boost::shared_ptr<RBX::ModuleScript> moduleScript;
	RBX::ContentId contentId;

	explicit LuaSourceBuffer(boost::shared_ptr<RBX::ModuleScript> moduleScript);
	explicit LuaSourceBuffer(boost::shared_ptr<RBX::Script> script);
	explicit LuaSourceBuffer(const RBX::ContentId& contentId);
};

inline size_t hash_value(const LuaSourceBuffer& lsb)
{
	return lsb.hash_value();
}
