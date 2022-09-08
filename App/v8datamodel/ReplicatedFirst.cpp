#include "stdafx.h"

#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/GuiObject.h"

#include "network/Players.h"

#include "script/script.h"


using namespace RBX;

const char* const RBX::sReplicatedFirst = "ReplicatedFirst";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<ReplicatedFirst, void()> func_setLoadingFinished(&ReplicatedFirst::doRemoveDefaultLoadingGui, "RemoveDefaultLoadingScreen", Security::None);

static Reflection::EventDesc<ReplicatedFirst, void()> event_finishedReplicating(&ReplicatedFirst::finishedReplicatingSignal, "FinishedReplicating", Security::RobloxScript);
static Reflection::BoundFuncDesc<ReplicatedFirst, bool()> func_getFinishedReplicating(&ReplicatedFirst::getIsFinishedReplicating, "IsFinishedReplicating", Security::RobloxScript);

static Reflection::EventDesc<ReplicatedFirst, void()> event_loadingFinished(&ReplicatedFirst::removeDefaultLoadingGuiSignal, "RemoveDefaultLoadingGuiSignal", Security::RobloxScript);
static Reflection::BoundFuncDesc<ReplicatedFirst, bool()> func_getLoadingFinished(&ReplicatedFirst::getIsDefaultLoadingGuiRemoved, "IsDefaultLoadingGuiRemoved", Security::RobloxScript);
REFLECTION_END();

ReplicatedFirst::ReplicatedFirst(void) :
	isDefaultLoadingGuiRemoved(false),
	allInstancesHaveReplicated(false),
	removeDefaultLoadingGuiOnGameLoaded(false)
{
	setName(sReplicatedFirst);
}


void ReplicatedFirst::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider,newProvider);

	if (newProvider)
	{
		if(DataModel* dm = DataModel::get(newProvider))
		{
			dm->gameLoadedSignal.connect(boost::bind(&ReplicatedFirst::gameIsLoaded,this));
			if (dm->getIsGameLoaded())
			{
				gameIsLoaded();
			}
		}
	}
}

void ReplicatedFirst::gameIsLoaded()
{
	// Play Solo should always just start replicated first
	if (RBX::Network::Players::frontendProcessing(this) && RBX::Network::Players::backendProcessing(this))
	{
		setAllInstancesHaveReplicated();
	}

	if (removeDefaultLoadingGuiOnGameLoaded)
	{
		doRemoveDefaultLoadingGui();
	}
}

void ReplicatedFirst::doRemoveDefaultLoadingGui()
{
	isDefaultLoadingGuiRemoved = true;
	removeDefaultLoadingGuiSignal();
}

void ReplicatedFirst::setAllInstancesHaveReplicated()
{
	RBXASSERT(allInstancesHaveReplicated == false);

	allInstancesHaveReplicated = true;

	if (!getChildren()) // we replicated nothing, tell the loading script we are done!
	{
		removeDefaultLoadingGuiOnGameLoaded = true;
	}
	else
	{
		startLocalScripts();
	}
    
    finishedReplicatingSignal();
}

void ReplicatedFirst::startLocalScript(shared_ptr<Instance> instance)
{
	if (LocalScript* script = Instance::fastDynamicCast<LocalScript>(instance.get()))
	{
		if (scriptShouldRun(script))
		{
			script->restartScript();
		}
	}
}

void ReplicatedFirst::startLocalScripts()
{
	this->visitDescendants(boost::bind(&ReplicatedFirst::startLocalScript, this, _1));
	if (TeleportService::didTeleport())
	{
		TeleportService* ts = ServiceProvider::create<TeleportService>(this);
		if (DataModel* dm = DataModel::get(this))
		{
			if (TeleportService::getPreviousCreatorId() == dm->getCreatorID() && TeleportService::getPreviousCreatorType() == dm->getCreatorType())
			{
				ts->sendPlayerArrivedFromTeleportSignal(TeleportService::getCustomTeleportLoadingGui(), TeleportService::getDataTable());
			} else {
				ts->sendPlayerArrivedFromTeleportSignal(shared_ptr<Instance>(), Reflection::Variant());
			}
		}
	}
}

bool ReplicatedFirst::scriptShouldRun(BaseScript* script)
{	
	bool isAncestor = isAncestorOf(script);
	RBXASSERT(isAncestor);

	if(!isAncestor)
	{
		return false;
	}

	if (RBX::Network::Players::frontendProcessing(script) && RBX::Network::Players::backendProcessing(script))
	{
		// in play solo, this is a valid configuration, do nothing
	}
	else
	{
		if (!RBX::Network::Players::frontendProcessing(script))
		{
			return false;
		}

		if (RBX::Network::Players::backendProcessing(script))
		{
			return false;
		}
	}

	if (!getAllInstancesHaveReplicated())
	{
		return false;
	}

	if (script->fastDynamicCast<LocalScript>())
	{
		return true;
	}

	return false;
}


bool ReplicatedFirst::askAddChild(const Instance* instance) const
{
	return (Instance::fastDynamicCast<LocalScript>(instance) || 
			Instance::fastDynamicCast<GuiObject>(instance));
}