#include "stdafx.h"

/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved  */
#include "V8DataModel/PlayerScripts.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/StarterPlayerService.h"
#include "V8DataModel/Folder.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Script/Script.h"
#include "Script/ModuleScript.h"
#include "Reflection/Event.h"


namespace RBX {

const char* const sPlayerScripts = "PlayerScripts";
PlayerScripts::PlayerScripts()
: Super()
{
	setName(sPlayerScripts);
}

bool PlayerScripts::askSetParent(const Instance* parent) const
{
	return Instance::fastDynamicCast<RBX::Network::Player>(parent) != NULL;
}

bool PlayerScripts::askForbidParent(const Instance* parent) const
{
	return !askSetParent(parent);
}

bool PlayerScripts::askAddChild(const Instance* instance) const
{
	if (Instance::fastDynamicCast<Folder>(instance) != NULL)
		return true;

	if (Instance::fastDynamicCast<ModuleScript>(instance) != NULL)
		return true;

	return Instance::fastDynamicCast<RBX::LocalScript>(instance) != NULL;
}

bool PlayerScripts::askForbidChild(const Instance* instance) const
{
	return !askAddChild(instance);
}


bool PlayerScripts::scriptShouldRun(BaseScript* script)
{
	RBXASSERT(isAncestorOf(script));

	bool answer = false;

	if (script->fastDynamicCast<LocalScript>())
	{
		{
			RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);
			answer = (localPlayer && (getParent() == localPlayer));
			if(answer){
				script->setLocalPlayer(shared_from(localPlayer));
			}
		}
	}

	return answer;
}


static void copyChildrenToPlayerScripts(Instance* scripts, Instance* playerScripts)
{
	for(size_t n = 0; n < scripts->numChildren(); n++) {
		Instance* c = scripts->getChild(n);

		bool previousArchivableValue = c->getIsArchivable();
		c->setIsArchivable(true); // We need to set this to archivable so it can be cloned, non-archivable objects can't be cloned
		if(shared_ptr<Instance> copy = c->clone(EngineCreator))
			copy.get()->setParent(playerScripts);

		c->setIsArchivable(previousArchivableValue); // Set archivable back to the previous value, default players scripts shouldn't be archivable but developer made scripts should
	}
}

void PlayerScripts::CopyStarterPlayerScripts(StarterPlayerScripts* scripts)
{
	if (scripts)
		copyChildrenToPlayerScripts(scripts, this);
}

void PlayerScripts::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider && RBX::Network::Players::frontendProcessing(newProvider))
	{
		// Check to see if we need to load the default scripts into this object - Client side from StarterPlayerService
		RBX::Network::Player* player = findFirstAncestorOfType<RBX::Network::Player>();
		RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(newProvider);
		if (player && player == localPlayer)
		{
			if (StarterPlayerService* starterPlayer = ServiceProvider::find<StarterPlayerService>(newProvider)) 
			{
				StarterPlayerScripts* scripts = starterPlayer->findFirstChildOfType<StarterPlayerScripts>();
				if (scripts)
					scripts->requestDefaultScripts();
			} else {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlayerScripts %s didn't find StarterPlayerService", getName().c_str());
			}
		}
	}
}


REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<StarterPlayerScripts, void(shared_ptr<Instance>)> event_requestDefaultScripts(&StarterPlayerScripts::requestDefaultScriptsSignal, 
																								 "RequestDefaultScripts", "root", Security::None, 
																								 Reflection::RemoteEventCommon::REPLICATE_ONLY, 
																								 Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<StarterPlayerScripts, void(int)> event_confirmDefaultScripts(&StarterPlayerScripts::confirmDefaultScriptsSignal, 
																								 "ConfirmDefaultScripts", "root", Security::None, 
																								 Reflection::RemoteEventCommon::REPLICATE_ONLY, 
																								 Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

const char* const sStarterPlayerScripts = "StarterPlayerScripts";
StarterPlayerScripts::StarterPlayerScripts()
: Super()
, defaultScriptsLoadRequested(false)
, defaultScriptsLoaded(false)
, defaultScriptsRequested(false)
{
	setName(sStarterPlayerScripts);	
}

bool StarterPlayerScripts::askSetParent(const Instance* parent) const
{
	return Instance::fastDynamicCast<RBX::StarterPlayerService>(parent) != NULL;
}

bool StarterPlayerScripts::askForbidParent(const Instance* parent) const
{
	return !askSetParent(parent);
}

bool StarterPlayerScripts::askAddChild(const Instance* instance) const
{
	if (Instance::fastDynamicCast<Folder>(instance) != NULL)
		return true;

	if (Instance::fastDynamicCast<ModuleScript>(instance) != NULL)
		return true;

	return Instance::fastDynamicCast<RBX::LocalScript>(instance) != NULL;
}

bool StarterPlayerScripts::askForbidChild(const Instance* instance) const
{
	return !askAddChild(instance);
}


static void addChildToInstance(const weak_ptr<Instance>& parent, const shared_ptr<Instance>& child)
{
	shared_ptr<Instance> p(parent.lock());
	if (!p)
		return;

	child->setIsArchivable(false);
	child->setParent(p.get());
}

static void PlayerScriptsLoadHelper(weak_ptr<StarterPlayerScripts> playerScripts, AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances)
{
	if(result == AsyncHttpQueue::Succeeded){
		std::for_each(instances->begin(), instances->end(), boost::bind(&addChildToInstance, playerScripts, _1));
		shared_ptr<StarterPlayerScripts> p(playerScripts.lock());
		StarterPlayerScripts *pPlayerScripts = p.get();
		if (pPlayerScripts)
			pPlayerScripts->checkDefaultScriptsLoaded();
	}
}

void StarterPlayerScripts::InitializeDefaultScripts()
{
	if (checkDefaultScriptsLoaded() ||  defaultScriptsLoadRequested)
		return;

	Instance *controlScript = findFirstChildByName("ControlScript");
	if (!controlScript) {
		ServiceProvider::create<ContentProvider>(this)->loadContent(ContentId::fromAssets("fonts/characterControlScript.rbxmx"), 
			ContentProvider::PRIORITY_SCRIPT, boost::bind(&PlayerScriptsLoadHelper, weak_from(this), _1, _2), AsyncHttpQueue::AsyncWrite);
	}

	Instance *cameraScript = findFirstChildByName("CameraScript");
	if (!cameraScript) {
		ServiceProvider::create<ContentProvider>(this)->loadContent(ContentId::fromAssets("fonts/characterCameraScript.rbxmx"), 
			ContentProvider::PRIORITY_SCRIPT, boost::bind(&PlayerScriptsLoadHelper, weak_from(this), _1, _2), AsyncHttpQueue::AsyncWrite);
	}

	defaultScriptsLoadRequested = true;
}

void StarterPlayerScripts::InitializeDefaultScriptsRunService(RunTransition transition)
{
	if (transition.newState == RS_RUNNING && transition.oldState == RS_STOPPED)
	{
		InitializeDefaultScripts();
		if (initializeDefaultScriptsConnection != NULL)
			initializeDefaultScriptsConnection.disconnect();
	}
}

bool StarterPlayerScripts::checkDefaultScriptsLoaded()
{
	if (defaultScriptsLoaded)
		return true;

	Instance *controlScript = findFirstChildByName("ControlScript");
	if (!controlScript) 
		return false;

	Instance *cameraScript = findFirstChildByName("CameraScript");
	if (!cameraScript) 
		return false;

	defaultScriptsLoaded = true;
	defaultScriptsLoadedSignal();
	defaultScriptsLoadedSignal.disconnectAll();

	return true;
}

// CLIENT - Request confirmation from server that control scripts are loaded
void StarterPlayerScripts::requestDefaultScripts()
{
	RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);
	if (localPlayer && !defaultScriptsRequested) 
	{
		event_requestDefaultScripts.fireAndReplicateEvent(this, shared_from(localPlayer));
		defaultScriptsRequested = true;
	}
}

// SERVER - Client has requested confirmation that scripts are loaded
void StarterPlayerScripts::requestDefaultScriptsServer(shared_ptr<Instance> player)
{
	RBX::Network::Player* pPlayer = Instance::fastDynamicCast<RBX::Network::Player>(player.get());
	if (pPlayer)
	{		    
		if (checkDefaultScriptsLoaded()) 
		{
			defaultScriptsSend(weak_from(pPlayer));
		} 
		else
		{
			InitializeDefaultScripts();
			defaultScriptsLoadedSignal.connect(boost::bind(&StarterPlayerScripts::defaultScriptsSend, this, weak_from(pPlayer)));
		}
	}
}

// SERVER - Send to client confirmation that scripts are loaded
void StarterPlayerScripts::defaultScriptsSend(weak_ptr<RBX::Network::Player> p)
{

	shared_ptr<RBX::Network::Player> player = p.lock();
	if (player) 
	{
		RBX::Network::Player* pPlayer = player.get();
		if (pPlayer) {

			if (Network::Players::serverIsPresent(this)) // check for play solo
			{
				const RBX::SystemAddress& target = pPlayer->getRemoteAddressAsRbxAddress();

				// respond to client
				Reflection::EventArguments args(1);
				args[0] = (int) 1;

				raiseEventInvocation(event_confirmDefaultScripts, args, &target);
			} 
			else 
			{
				defaultScriptsReceived(1);
			}
		}
	}
}

// CLIENT - Receive confirmation from server that scripts are replicated
void StarterPlayerScripts::defaultScriptsReceived(int confirm)
{
	RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);	
	if (localPlayer && confirm == 1) 
	{
		PlayerScripts* scripts = localPlayer->findFirstChildOfType<PlayerScripts>();
		if (scripts)
			scripts->CopyStarterPlayerScripts(this);
	}
}


void StarterPlayerScripts::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	
	Super::onServiceProvider(oldProvider, newProvider);

	// Check to see if we need to load the default scripts into this object - Server side from files
	if (newProvider && RBX::Network::Players::backendProcessing(newProvider) )
	{
		if (RunService* runService = ServiceProvider::create<RunService>(newProvider))
		{
			if (runService->isRunning())
			{
				InitializeDefaultScripts();
			} else {
				initializeDefaultScriptsConnection = runService->runTransitionSignal.connect(boost::bind(&StarterPlayerScripts::InitializeDefaultScriptsRunService, this, _1));
			}
		}

		requestDefaultScriptsSignal.connect(boost::bind(&StarterPlayerScripts::requestDefaultScriptsServer, this, _1));
	}

	if (newProvider && RBX::Network::Players::frontendProcessing(newProvider))
	{
		confirmDefaultScriptsSignal.connect(boost::bind(&StarterPlayerScripts::defaultScriptsReceived, this, _1)); 
		requestDefaultScripts();
	}
}


const char* const sStarterCharacterScripts = "StarterCharacterScripts";
StarterCharacterScripts::StarterCharacterScripts()
	: Super()
{
	setName(sStarterCharacterScripts);	
}

bool StarterCharacterScripts::askAddChild(const Instance* instance) const
{
	if (Instance::fastDynamicCast<Script>(instance) != NULL)
		return true;

	return Super::askAddChild(instance);
}

void StarterCharacterScripts::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	DescribedCreatable<StarterPlayerScripts, Instance, sStarterPlayerScripts, Reflection::ClassDescriptor::PERSISTENT>::onServiceProvider(oldProvider, newProvider);
}

} // Namespace
