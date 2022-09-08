#include "stdafx.h"

#include "V8DataModel/GamePassService.h"
#include "Util/LuaWebService.h"
#include "V8Xml/WebParser.h"
#include "Network/Player.h"
#include "V8DataModel/Workspace.h"
#include "util/RbxStringTable.h"

namespace RBX
{
const char* const sGamePassService = "GamePassService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<GamePassService, void(std::string)> func_SetPlayerHasPassUrl(&GamePassService::setPlayerHasPassUrl, "SetPlayerHasPassUrl", "playerHasPassUrl", Security::LocalUser);

static Reflection::BoundYieldFuncDesc<GamePassService, bool(shared_ptr<Instance>, int)> func_PlayerHasPass(&GamePassService::playerHasPass, "PlayerHasPass", "player", "gamePassId", Security::None);
REFLECTION_END();

GamePassService::GamePassService()
{
	setName(sGamePassService);
}

void GamePassService::setPlayerHasPassUrl(std::string value)
{
	playerHasPassUrl = value;
}

template<typename ResultType>
void GamePassService::dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
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

void GamePassService::playerHasPass(shared_ptr<Instance> playerInstance, int gamePassId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (playerHasPassUrl.empty())
		errorFunction("No playerHasPassUrl set");
	else if (shared_ptr<Network::Player> player = Instance::fastSharedDynamicCast<Network::Player>(playerInstance))
	{
		// make sure this is only called on the server
		if(!Workspace::serverIsPresent(this)){
			// only warn them in this case; don't kill their script
			StandardOut::singleton()->print(MESSAGE_WARNING, STRING_BY_ID(HasGamePassLuaWarning));
			resumeFunction(false);
			return;
		}
		int playerId = player->getUserID();
		std::string url(RBX::format(playerHasPassUrl.c_str(), playerId, gamePassId));
		dispatchRequest(url, resumeFunction, errorFunction);
	}
	else
	{
		errorFunction("Not a valid Player");
	}
}


}
