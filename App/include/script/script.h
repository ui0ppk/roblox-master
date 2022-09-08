#pragma once

#include "V8Tree/Instance.h"
#include "Script/ThreadRef.h"
#include "Util/ScriptInformationProvider.h"
#include "Util/ProtectedString.h"
#include "script/LuaSourceContainer.h"
#include "rbx/atomic.h"
#include <boost/function.hpp>
#include <boost/flyweight.hpp>

struct lua_State;

namespace RBX
{
	class IScriptOwner;
	class ScriptContext;
	class RuntimeScriptService;
	class ScriptInformationProvider;
	class ContentProvider;

	namespace Network
	{
		class Player;
	}

	typedef ContentId ScriptId;
	extern const char* const sBaseScript;
	class BaseScript 
		: public DescribedNonCreatable<BaseScript, LuaSourceContainer, sBaseScript>
	{
	private:
		typedef DescribedNonCreatable<BaseScript, LuaSourceContainer, sBaseScript> Super;

	public:
		class Slot;

		// Used for development only. It allows you to load CoreScripts from your local disk
		static std::string adminScriptsPath;
		static bool hasCoreScriptReplacements();

		void restartScript();
	protected:
		RuntimeScriptService* workspace;

		///////////////////////////////////
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void onScriptIdChanged();

	private:
		static const std::string emptyString;

		weak_ptr<RBX::Network::Player> localPlayer;

		bool disabled;
		bool badLinkedScript;
		
		RuntimeScriptService* computeNewWorkspace();

	public:
		struct Code
		{
			bool loaded;
			boost::flyweight<ProtectedString> script;

			Code()
				:loaded(false)
			{}
			Code(const boost::flyweight<ProtectedString>& script)
				:loaded(true)
				,script(script)
			{}
		};
		BaseScript();
		~BaseScript();


		static const Reflection::PropDescriptor<BaseScript, ScriptId> prop_SourceCodeId;

		weak_ptr<RBX::Network::Player> getLocalPlayer() { return localPlayer; }
		void setLocalPlayer(const shared_ptr<RBX::Network::Player>& localPlayer) { this->localPlayer = localPlayer; }

		// Thread management
		boost::intrusive_ptr<Lua::WeakThreadRef::Node> threadNode;
		rbx::signal<void(lua_State*)> starting;
		rbx::signal<void()> stopped;

		bool isDisabled() const { return disabled; }
		static const Reflection::PropDescriptor<BaseScript, bool> prop_Disabled;

		virtual Code requestCode(ScriptInformationProvider* scriptInfoProvider=NULL);

		virtual void extraErrorReporting(lua_State *thread) {}

		//Properties
		bool getDisabled() const { return disabled; }
		void setDisabled(bool value);

		virtual const std::string& requestHash() const;
	};
	// A BaseScript is started when a containing IScriptOwner sends it to the ScriptContext service
	extern const char* const sScript;
	class Script 
		: public DescribedCreatable<Script, BaseScript, sScript>
	{
	private:
		typedef DescribedCreatable<Script, BaseScript, sScript> Super;

	private:
		boost::flyweight<ProtectedString> embeddedSource;
        std::string embeddedSourceHash;

	public:
		Script();
		~Script();

		static const Reflection::PropDescriptor<Script, ProtectedString> prop_EmbeddedSourceCode;
	
		/*override*/ XmlElement* writeXml(const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole)
		{
			return Super::writeXml(isInScope, creatorRole);
		}

		/*override*/ bool askSetParent(const Instance* instance) const
		{
			// Scripts can be anywhere
			return true;
		}

		bool isCodeEmbedded() const { return getScriptId().isNull(); }

		/*override*/ Code requestCode(ScriptInformationProvider* scriptInfoProvider=NULL);

		/*override*/ const std::string& requestHash() const;

		void setEmbeddedCode(const ProtectedString& value);
		const boost::flyweight<ProtectedString>& getEmbeddedCode() const;
		const ProtectedString& getEmbeddedCodeSafe() const;
		/*override*/ int getPersistentDataCost() const;
		/*override*/ void fireSourceChanged();

	private:
		std::string getHash() { return requestHash(); }

		static const Reflection::BoundFuncDesc<Script, std::string()> func_GetHash;
	};

	// Only runs on a local machine if either
	// a) Inside a tool which is inside a local character
	// b) Inside the local backpack

	// Local scripts have the full power of a normal script, but can also interact with the Mouse.
	// While they are currently run client side, this is a large security hole that will have to be addressed.
	// The plan is to have them execute server side, but with an adapter taking the place of the "Mouse" object and abstracting it per-user
	//
	// A better name would be GuiScript or UserScript, if we could redo this work.
	extern const char* const sLocalScript;
	class LocalScript
		: public DescribedCreatable<LocalScript, Script, sLocalScript>
	{
	public:

		LocalScript();
		~LocalScript() {}
    };

	class BaseScript::Slot
	{
		rbx::signals::connection connection;
	public:
		// A Slot must keep a reference to its connection, because it
		// must be capable of disconnecting itself. 
		void assignConnection(const rbx::signals::connection& connection)
		{
			this->connection = connection;
		}
	protected:
		Slot()
		{
		}
		~Slot()
		{
			// TODO: Disconnect here???
		}
		void disconnect()
		{
			connection.disconnect();
		}
	};
}
