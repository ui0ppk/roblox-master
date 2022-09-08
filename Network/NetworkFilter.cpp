#include "NetworkFilter.h"

#include "network/Player.h"
#include "Replicator.h"
#include "humanoid/Humanoid.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/Hopper.h"
#include "v8datamodel/Teams.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/SkateboardPlatform.h"
#include "v8datamodel/Seat.h"
#include "v8datamodel/StarterPlayerService.h"
#include "Network/Players.h"

namespace RBX {
	namespace Reflection {
		template<>
		EnumDesc<Network::FilterResult>::EnumDesc()
			:EnumDescriptor("FilterResult")
		{
			addPair(Network::Reject, "Rejected");
			addPair(Network::Accept, "Accepted");
		}

		template<>
		Network::FilterResult& Variant::convert<Network::FilterResult>(void)
		{
			return genericConvert<Network::FilterResult>();
		}

	}

	template<>
	bool StringConverter<Network::FilterResult>::convertToValue(const std::string& text, Network::FilterResult& value)
	{
		return Reflection::EnumDesc<Network::FilterResult>::singleton().convertToValue(text.c_str(),value);
	}
}

using namespace RBX;
using namespace Reflection;
using namespace Network;


NetworkFilter::NetworkFilter(Replicator* replicator)
:replicator(replicator)
{
}

NetworkFilter::~NetworkFilter(void)
{
}

template <class T, FilterResult R>
static bool filterInside(Instance* parent, FilterResult& result)
{
	if (!parent)
		return false;

	if (Instance::fastDynamicCast<T>(parent))
	{
		result = R;
		return true;
	}

	return filterInside<T, R>(parent->getParent(), result);
}


bool RBX::Network::NetworkFilter::filterChangedProperty( Instance* instance, const Reflection::PropertyDescriptor& desc, FilterResult& result )
{
	// if desc==Instance::propParent, then filterParent will be called, so those rules apply, too

	// Can't change items in these services:
	if (filterInside<StarterGuiService, Reject>(instance, result))
		return true;
	if (filterInside<StarterPackService, Reject>(instance, result))
		return true;
	if (filterInside<StarterPlayerService, Reject>(instance, result))
		return true;
	if (filterInside<Teams, Reject>(instance, result))
		return true;

	// Allow/Reject some physics properties
	if (desc == PartInstance::prop_NetworkIsSleeping)
	{
		result = Accept;
		return true;
	}

#if 0
	// DE3665
	if (desc == PartInstance::prop_NetworkOwner)
	{
		result = Reject;
		return true;
	}
#endif

	if (desc == Player::prop_SimulationRadius)
	{
		if (replicator->getPlayer().get() == instance)
			result = Accept;
		else
			result = Reject;
		return true;
	}

	if (Humanoid* humanoid = Instance::fastDynamicCast<Humanoid>(instance))
	{
		Network::Player* player = Network::Players::getPlayerFromCharacter(Humanoid::getCharacterFromHumanoid(humanoid));

		if (player == replicator->getPlayer().get())
			if (humanoid->isLegalForClientToChange(desc))
			{
				result = Accept;
				return true;
			}
	}
	
	return false;
}

bool RBX::Network::NetworkFilter::filterNew( Instance* instance, Instance* parent, FilterResult& result )
{
	if (filterParent(instance, parent, result))
		return true;

	return false;
}

bool RBX::Network::NetworkFilter::filterDelete( Instance* instance, FilterResult& result )
{
	if (filterParent(instance, NULL, result))
		return true;

	return false;
}

bool RBX::Network::NetworkFilter::filterParent( Instance* instance, Instance* parent, FilterResult& result )
{
	if (Player* player = Instance::fastDynamicCast<Player>(instance))
	{
		if (!player->getParent())
		{
			// This Player is just being inserted into the world, so it is presumably the local player. 
			// Let it go if it is going to the right place:
			result = Instance::fastDynamicCast<Players>(parent) ? Accept : Reject;
			return true;
		}
	}

	// Can't move to these services:
	if (filterInside<StarterGuiService, Reject>(parent, result))
		return true;
	if (filterInside<StarterPackService, Reject>(parent, result))
		return true;
	if (filterInside<StarterPlayerService, Reject>(parent, result))
		return true;
	if (filterInside<Teams, Reject>(parent, result))
		return true;

	// Can't move when in these services:
	if (filterInside<StarterGuiService, Reject>(instance, result))
		return true;
	if (filterInside<StarterPackService, Reject>(instance, result))
		return true;
	if (filterInside<StarterPlayerService, Reject>(instance, result))
		return true;
	if (filterInside<Teams, Reject>(instance, result))
		return true;

	// Can't set if parent is locked
	if (instance && instance->getIsParentLocked())
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "trying to set locked parent!");
		result = Reject;
		return true;
	}

	return false;
}

bool RBX::Network::NetworkFilter::filterEvent( Instance* instance, const Reflection::EventDescriptor& desc, FilterResult& result )
{
	return false;
}

boost::unordered_set<std::string> StrictNetworkFilter::propertyWhiteList;
boost::unordered_set<std::string> StrictNetworkFilter::eventWhiteList;

StrictNetworkFilter::StrictNetworkFilter(Replicator* replicator) : replicator(replicator), instanceBeingRemovedFromLocalPlayer(NULL)
{
	// these lists should only contain scriptable properties or events, all non-scriptable (REPLICATE_ONLY) items are not filtered

	if (propertyWhiteList.size() == 0)
	{
		std::string props[] = {"Jump", "Sit", "Throttle", "Steer", "SimulationRadius"}; 
		propertyWhiteList.insert(props, props + sizeof(props)/sizeof(std::string));
	}

	if (eventWhiteList.size() == 0)
	{
		std::string events[] = {"OnServerEvent", 
			"PromptProductPurchaseFinished", "PromptPurchaseFinished",
			"PromptPurchaseRequested", "ClientPurchaseSuccess", "ServerPurchaseVerification",
			"Activated", "Deactivated", "SimulationRadiusChanged", "LuaDialogCallbackSignal", "ServerAdVerification",
			"ClientAdVerificationResults"
		}; 
		eventWhiteList.insert(events, events + sizeof(events)/sizeof(std::string));
	}
}

void StrictNetworkFilter::onChildRemoved(Instance* removed, const Instance* oldParent)
{
	instanceBeingRemovedFromLocalPlayer = NULL;

	if (Player* player = replicator->findTargetPlayer())
	{
		if (ModelInstance* playerModel = player->getCharacter())
		{
			if (oldParent == playerModel || oldParent->isDescendantOf(playerModel))
			{
				instanceBeingRemovedFromLocalPlayer = removed;
				return;
			}
		}
		
		if ((oldParent == player)|| (oldParent->isDescendantOf(player)))
		{
			instanceBeingRemovedFromLocalPlayer = removed;
		}
	}
}

FilterResult StrictNetworkFilter::filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
	if (desc == Instance::propParent)
		return Accept; // should be handled by filterParent

	if (desc.isScriptable() || desc.canXmlWrite() || desc.canXmlRead())
	{
		if (propertyWhiteList.find(desc.name.toString()) != propertyWhiteList.end())
			return Accept;

		return Reject;
	}

	// accept all non-scriptable properties
	return Accept;
}

bool isOrUnderAncestor(const Instance* instance, const Instance* newParent, const Instance* ancestor)
{
	const Instance* oldParent = instance->getParent();

	// originating parent must be player or player character
	if (oldParent != newParent)
	{
		// only server code should enter here
		RBXASSERT(Players::backendProcessing(instance));

		if (!oldParent)
			return false;

		// originating parent must be player or player character
		if (oldParent == ancestor || oldParent->isDescendantOf(ancestor))
			return true;
	}
	else if (newParent == ancestor || newParent->isDescendantOf(ancestor))
	{
		// only client should enter here
		RBXASSERT(Players::frontendProcessing(instance));
		return true;
	}

	return false;
}

FilterResult StrictNetworkFilter::filterParent(Instance* instance, Instance* newParent)
{
	FilterResult result = Reject;

	RBX::Weld* weld = NULL;
	weld = Instance::fastDynamicCast<RBX::Weld>(instance);

	if (instance == instanceBeingRemovedFromLocalPlayer)
	{
		// allow parents to be changed from local player
		result = Accept;
	}
	else if (newParent && 
		(instance->fastDynamicCast<RBX::Tool>() 
		|| weld
		|| instance->fastDynamicCast<RBX::Accoutrement>()
		|| instance->fastDynamicCast<RBX::Seat>()
		|| instance->fastDynamicCast<RBX::SkateboardPlatform>())) // Only accept reparenting of tools, hats, and seats
	{
		if (Player* player = replicator->findTargetPlayer())
		{
			if (ModelInstance* playerModel = player->getCharacter())
			{
				if (isOrUnderAncestor(instance, newParent, playerModel))
				{
					result = Accept;

					if (weld)
					{
						if (weld->getName() != "RightGrip" && (weld->getPart0() && weld->getPart0()->isDescendantOf(playerModel)))
							result = Reject;
					}
				}
			}
			
			if (result == Reject)
			{
				if (isOrUnderAncestor(instance, newParent, player))
					result = Accept;
			}
		}
	}

	// this should be set in onChildRemoved, which should happen right before filterParent, so reset after ever use
	instanceBeingRemovedFromLocalPlayer = NULL; 

	return result;
}

FilterResult StrictNetworkFilter::filterEvent(Instance* instance, const Reflection::EventDescriptor& desc)
{
	if (desc.isScriptable())
	{
		// accept only white listed scriptable events
		if (eventWhiteList.find(desc.name.toString()) != eventWhiteList.end())
			return Accept;

		return Reject;
	}

	// accept non-scriptable events
	return Accept;
}

FilterResult StrictNetworkFilter::filterNew(const Instance* instance, const Instance* parent)
{
	// filter all new instances except local player
	if (const Player* player = Instance::fastDynamicCast<const Player>(instance))
	{
		if (const Players* players = Instance::fastDynamicCast<const Players>(instance->getParent()))
		{
			if (players->getConstLocalPlayer() == player) // Client
			{
				return Accept; 
			}
		}
		else // Server
		{
			// This Player is just being inserted into the world, so it is presumably the local player. 
			// Let it go if it is going to the right place:
			return Instance::fastDynamicCast<Players>(parent) ? Accept : Reject;
		}
	}

	// TODO: create StarterGear on server
	if (Instance::fastDynamicCast<StarterGear>(instance))
		return Accept;
	else if (Instance::fastDynamicCast<Weld>(instance)) // allow new tool weld under the player character model
	{
		if (Player* player = replicator->findTargetPlayer())
		{
			if (ModelInstance* playerModel = player->getCharacter())
			{
				if (instance->getParent()) // Client, parent should already be set
				{
					RBXASSERT(Players::frontendProcessing(instance));
					return isOrUnderAncestor(instance, parent, playerModel) ? Accept : Reject;
				}
				else
				{
					if (parent)
					{
						// Server, parent haven't been set yet
						RBXASSERT(Players::backendProcessing(parent));

						if (parent->isDescendantOf(playerModel))
							return Accept;
					}
				}
			}
		}
	}

	return Reject;
}

FilterResult StrictNetworkFilter::filterDelete(const Instance* instance) 
{
	if (Player* player = replicator->findTargetPlayer())
	{
		if (ModelInstance* playerModel = player->getCharacter())
		{
			if (instance->isDescendantOf(playerModel))
				return Accept;
		}
		else
		{
			if (instance->isDescendantOf(player))
				return Accept;
		}
	}

	return Reject;
}

FilterResult StrictNetworkFilter::filterTerrainCellChange()
{
	return Reject; 
}


