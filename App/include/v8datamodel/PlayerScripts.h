/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved  */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"

#include "V8DataModel/InputObject.h"
#include "V8DataModel/UserInputService.h"
#include "Script/IScriptFilter.h"
#include "Gui/GuiEvent.h"

namespace RBX {
	class StarterPlayerScripts;

	extern const char *const sPlayerScripts;
	class PlayerScripts
		: public DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public IScriptFilter
	{
	private:
		typedef DescribedCreatable<PlayerScripts, Instance, sPlayerScripts, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

	public:
		PlayerScripts();

		void CopyStarterPlayerScripts(StarterPlayerScripts* scripts);

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IScriptFilter
		/*override*/ bool scriptShouldRun(BaseScript* script);

	};


	extern const char *const sStarterPlayerScripts;
	class StarterPlayerScripts
		: public DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT>
	{
	private:
		typedef DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT> Super;

		void InitializeDefaultScripts();
		void InitializeDefaultScriptsRunService(RunTransition transition);
		rbx::signals::scoped_connection initializeDefaultScriptsConnection;

		rbx::signal<void()> defaultScriptsLoadedSignal;	

		bool defaultScriptsLoadRequested;
		bool defaultScriptsLoaded;
		bool defaultScriptsRequested;

	public:
		StarterPlayerScripts();
		bool areDefaultScriptsLoaded() 	{ return defaultScriptsLoaded; } 
		bool checkDefaultScriptsLoaded();

		void requestDefaultScripts();
		void requestDefaultScriptsServer(shared_ptr<Instance> player);
		void defaultScriptsSend(weak_ptr<RBX::Network::Player> p);
		void defaultScriptsReceived(int confirm);

		rbx::remote_signal<void(shared_ptr<Instance>)> requestDefaultScriptsSignal;
		rbx::remote_signal<void(int)> confirmDefaultScriptsSignal;

	protected:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const;
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};

	extern const char *const sStarterCharacterScripts;
	class StarterCharacterScripts
		: public DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT>
	{
	private:
		typedef DescribedCreatable<StarterCharacterScripts, StarterPlayerScripts, sStarterCharacterScripts, Reflection::ClassDescriptor::PERSISTENT> Super;
	public:
		StarterCharacterScripts();


		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};



}

