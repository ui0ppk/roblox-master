#include "stdafx.h"

#include "V8DataModel/PersonalServerService.h"
#include "Util/LuaWebService.h"
#include "Network/Players.h"

namespace RBX
{
const char* const sPersonalServerService = "PersonalServerService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<PersonalServerService, void(std::string)> func_SetPersonalServerGetRankUrl(&PersonalServerService::setPersonalServerGetRankUrl, "SetPersonalServerGetRankUrl", "personalServerGetRankUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<PersonalServerService, void(std::string)> func_SetPersonalServerSetRankUrl(&PersonalServerService::setPersonalServerSetRankUrl, "SetPersonalServerSetRankUrl", "personalServerSetRankUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<PersonalServerService, void(std::string)> func_SetPersonalServerBuildToolsUrl(&PersonalServerService::setPersonalServerRoleSetsUrl, "SetPersonalServerRoleSetsUrl", "personalServerRoleSetsUrl", Security::LocalUser);

static Reflection::BoundFuncDesc<PersonalServerService, void(shared_ptr<Instance>)> func_Promote(&PersonalServerService::promote, "Promote", "player", Security::RobloxScript);
static Reflection::BoundFuncDesc<PersonalServerService, void(shared_ptr<Instance>)> func_Demote(&PersonalServerService::demote, "Demote", "player", Security::RobloxScript);

static Reflection::BoundYieldFuncDesc<PersonalServerService, std::string(int)>	func_GetRoleSets(&PersonalServerService::getWebRoleSets, "GetRoleSets", "placeId", Security::RobloxScript);

static Reflection::PropDescriptor<PersonalServerService, std::string> prop_roleSets("RoleSets", category_Behavior, &PersonalServerService::getRoleSets, &PersonalServerService::setRoleSets, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<PersonalServerService::PrivilegeType>::EnumDesc()
:EnumDescriptor("PrivilegeType")
{
	addPair(PersonalServerService::PRIVILEGE_OWNER,		"Owner");		
	addPair(PersonalServerService::PRIVILEGE_ADMIN,		"Admin");
	addPair(PersonalServerService::PRIVILEGE_MEMBER,	"Member");
	addPair(PersonalServerService::PRIVILEGE_VISITOR,	"Visitor");
	addPair(PersonalServerService::PRIVILEGE_BANNED,	"Banned");
}

template<>
PersonalServerService::PrivilegeType& Variant::convert<PersonalServerService::PrivilegeType>(void)
{
	return genericConvert<PersonalServerService::PrivilegeType>();
}
}//namespace Reflection

template<>
bool StringConverter<PersonalServerService::PrivilegeType>::convertToValue(const std::string& text, PersonalServerService::PrivilegeType& value)
{
	return Reflection::EnumDesc<PersonalServerService::PrivilegeType>::singleton().convertToValue(text.c_str(),value);
}

PersonalServerService::PersonalServerService(void)
	: personalServerGetRankUrl("")
	, personalServerSetRankUrl("")
	, personalServerRoleSetsUrl("")
	, roleSets("")
{
	Instance::propArchivable.setValue(this, false);
}

void PersonalServerService::setPersonalServerGetRankUrl(std::string url)
{
	personalServerGetRankUrl = url;
}
void PersonalServerService::setPersonalServerSetRankUrl(std::string url)
{
	personalServerSetRankUrl = url;
}
void PersonalServerService::setPersonalServerRoleSetsUrl(std::string url)
{
	personalServerRoleSetsUrl = url;
}
template<typename ResultType>
void PersonalServerService::dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(LuaWebService* luaWebService = ServiceProvider::create<LuaWebService>(this))
	{ 
		try
		{
			luaWebService->asyncRequest(url, LUA_WEB_SERVICE_STANDARD_PRIORITY, resumeFunction, errorFunction);
		}
		catch(RBX::base_exception&)
		{
			errorFunction("Error during dispatch");
		}
	} 
	else
	{
		errorFunction("Shutting down");
	}
}

void PersonalServerService::getWebRoleSets(int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(personalServerRoleSetsUrl.empty())
		errorFunction("No personalServerRoleSetsUrl set");
	else if(!Network::Players::backendProcessing(this))
		errorFunction("getWebRoleSets should only be called from gameserver");
	else
	{
		std::string url(RBX::format(personalServerRoleSetsUrl.c_str(), placeId));
		dispatchRequest(url, resumeFunction, errorFunction);
	}
}

void PersonalServerService::moveToRank(Network::Player* player, int rank)
{
	player->setPersonalServerRank(rank);
}

void PersonalServerService::getRank(Network::Player* player, int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(personalServerGetRankUrl.empty())
		errorFunction("No personalServerGetRankUrl set");
	else
	{
		std::string url(RBX::format(personalServerGetRankUrl.c_str(), placeId, player->getUserID()));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}

void PersonalServerService::setRank(Network::Player* player, int placeId, int newRank, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(personalServerSetRankUrl.empty())
		errorFunction("No personalServerSetRankUrl set");
	else
	{
		std::string url(RBX::format(personalServerSetRankUrl.c_str(), placeId, player->getUserID(), newRank));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}

void PersonalServerService::setPrivilege(Network::Player* player, PrivilegeType privilegeType)
{
	moveToRank(player,(int) privilegeType);
}

int PersonalServerService::getCurrentPrivilege(int rank)
{
	if(rank < PRIVILEGE_VISITOR)
		return PRIVILEGE_BANNED;
	else if( (rank >= PRIVILEGE_VISITOR) && (rank < PRIVILEGE_MEMBER) )
		return PRIVILEGE_VISITOR;
	else if( (rank >= PRIVILEGE_MEMBER) && (rank < PRIVILEGE_ADMIN) )
		return PRIVILEGE_MEMBER;
	else if( (rank >= PRIVILEGE_ADMIN) && (rank < PRIVILEGE_OWNER) )
		return PRIVILEGE_ADMIN;
	else if(rank >= PRIVILEGE_OWNER)
		return PRIVILEGE_OWNER;

	return rank;
}

int PersonalServerService::nextRankUp(int currentRank)
{
	switch(currentRank)
	{
	case PRIVILEGE_OWNER:
		return currentRank;
	case PRIVILEGE_ADMIN:
		return PRIVILEGE_OWNER;
	case PRIVILEGE_MEMBER:
		return PRIVILEGE_ADMIN;
	case PRIVILEGE_VISITOR:
		return PRIVILEGE_MEMBER;
	case PRIVILEGE_BANNED:
		return PRIVILEGE_VISITOR;
	default:
		return currentRank;
	}
}

int PersonalServerService::nextRankDown(int currentRank)
{
	switch(currentRank)
	{
	case PRIVILEGE_OWNER:
		return PRIVILEGE_ADMIN;
	case PRIVILEGE_ADMIN:
		return PRIVILEGE_MEMBER;
	case PRIVILEGE_MEMBER:
		return PRIVILEGE_VISITOR;
	case PRIVILEGE_VISITOR:
		return PRIVILEGE_BANNED;
	case PRIVILEGE_BANNED:
		return currentRank;
	default:
		return currentRank;
	}
}

void PersonalServerService::promote(shared_ptr<Instance> instance)
{
	if(instance.get())
		if(Network::Player* player = dynamic_cast<Network::Player*>(instance.get()))
			moveToRank(player, nextRankUp(player->getPersonalServerRank()));
}
		
void PersonalServerService::demote(shared_ptr<Instance> instance)
{
	if(instance.get())
		if(Network::Player* player = dynamic_cast<Network::Player*>(instance.get()))
			moveToRank(player, nextRankDown(player->getPersonalServerRank()));
}

}
