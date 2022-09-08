#include "stdafx.h"

#include "V8DataModel/SocialService.h"
#include "Util/LuaWebService.h"
#include "V8Xml/WebParser.h"

FASTFLAGVARIABLE(EnableLuaFollowers, true)
DYNAMIC_FASTFLAGVARIABLE(UserServerFollowers, false)

namespace RBX
{
const char* const sSocialService = "SocialService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetFriendUrl(&SocialService::setFriendUrl, "SetFriendUrl", "friendUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetBestFriendUrl(&SocialService::setBestFriendUrl, "SetBestFriendUrl", "bestFriendUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetGroupUrl(&SocialService::setGroupUrl, "SetGroupUrl", "groupUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetGroupRankUrl(&SocialService::setGroupRankUrl, "SetGroupRankUrl", "groupRankUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetGroupRoleUrl(&SocialService::setGroupRoleUrl, "SetGroupRoleUrl", "groupRoleUrl", Security::LocalUser);

static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetStuffUrl(&SocialService::setStuffUrl, "SetStuffUrl", "stuffUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<SocialService, void(std::string)> func_SetPackageContentsUrl(&SocialService::setPackageContentsUrl, "SetPackageContentsUrl", "stuffUrl", Security::LocalUser);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<SocialService::StuffType>::EnumDesc()
:EnumDescriptor("Stuff")
{
	addPair(SocialService::HEADS_STUFF,		"Heads");		
	addPair(SocialService::FACES_STUFF,		"Faces");
	addPair(SocialService::HATS_STUFF,		"Hats");
	addPair(SocialService::T_SHIRTS_STUFF,	"TShirts");
	addPair(SocialService::SHIRTS_STUFF,	"Shirts");
	addPair(SocialService::PANTS_STUFF,		"Pants");
	addPair(SocialService::GEARS_STUFF,		"Gears");
	addPair(SocialService::TORSOS_STUFF,	"Torsos");
	addPair(SocialService::LEFT_ARMS_STUFF,	"LeftArms");
	addPair(SocialService::RIGHT_ARMS_STUFF,"RightArms");
	addPair(SocialService::LEFT_LEGS_STUFF,	"LeftLegs");
	addPair(SocialService::RIGHT_LEGS_STUFF,"RightLegs");
	addPair(SocialService::BODIES_STUFF,	"Bodies");	
	addPair(SocialService::COSTUMES_STUFF,	"Costumes");	
}

template<>
SocialService::StuffType& Variant::convert<SocialService::StuffType>(void)
{
	return genericConvert<SocialService::StuffType>();
}
}//namespace Reflection

template<>
bool StringConverter<SocialService::StuffType>::convertToValue(const std::string& text, SocialService::StuffType& value)
{
	return Reflection::EnumDesc<SocialService::StuffType>::singleton().convertToValue(text.c_str(),value);
}

SocialService::SocialService()
{
	setName(sSocialService);
}

void SocialService::setFriendUrl(std::string value)
{
	friendUrl = value;
}
void SocialService::setBestFriendUrl(std::string value)
{
	bestFriendUrl = value;
}
void SocialService::setGroupUrl(std::string value)
{
	groupUrl = value;
}
void SocialService::setGroupRankUrl(std::string value)
{
	groupRankUrl = value;
}
void SocialService::setGroupRoleUrl(std::string value)
{
	groupRoleUrl = value;
}
void SocialService::setStuffUrl(std::string value)
{
	stuffUrl = value;
}
void SocialService::setPackageContentsUrl(std::string value)
{
	packageContentsUrl = value;
}
template<typename ResultType>
void SocialService::dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
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

void SocialService::getRankInGroup(int playerId, int groupId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (groupRankUrl.empty())
		errorFunction("No groupRankUrl set");
	else
	{
		std::string url(RBX::format(groupRankUrl.c_str(), playerId, groupId));
		dispatchRequest(url, resumeFunction, errorFunction);
	}
}

void SocialService::getRoleInGroup(int playerId, int groupId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (groupRoleUrl.empty())
		errorFunction("No groupRoleUrl set");
	else
	{
		std::string url(RBX::format(groupRoleUrl.c_str(), playerId, groupId));
		dispatchRequest(url, resumeFunction, errorFunction);
	}
}

void SocialService::isFriendsWith(int playerId, int userId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(friendUrl.empty())
		errorFunction("No friendUrl set");
	else
	{
		std::string url(RBX::format(friendUrl.c_str(), playerId, userId));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}
void SocialService::isBestFriendsWith(int playerId, int userId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(bestFriendUrl.empty())
		errorFunction("No bestFriendUrl set");
	else
	{
		std::string url(RBX::format(bestFriendUrl.c_str(), playerId, userId));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}
void SocialService::isInGroup(int playerId, int groupId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(groupUrl.empty())
		errorFunction("No groupUrl set");
	else
	{
		std::string url(RBX::format(groupUrl.c_str(), playerId, groupId));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}
void SocialService::getListOfStuff(int playerId, StuffType category, int page, boost::function<void(shared_ptr<const Reflection::ValueMap>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(stuffUrl.empty()){
		errorFunction("No stuffUrl set");
	}
	else
	{
		std::string url(RBX::format(stuffUrl.c_str(), playerId, page, category));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}
void SocialService::getPackageContents(int assetId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	
	if(packageContentsUrl.empty()){
		errorFunction("No packageContentsUrl set");
	}
	else
	{
		std::string url(RBX::format(packageContentsUrl.c_str(), assetId));
	    dispatchRequest(url, resumeFunction, errorFunction);
	}
}

}