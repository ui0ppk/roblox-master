#include "stdafx.h"

#include "V8DataModel/ChatService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8Datamodel/HttpRbxApiService.h"
#include "v8xml/WebParser.h"

#include "Util/RobloxGoogleAnalytics.h"

#include "network/WebChatFilter.h"
#include "Network/Players.h"

DYNAMIC_FASTFLAG(UseComSiftUpdatedWebChatFilterParamsAndHeader)

namespace RBX
{
const char* const sChatService = "Chat";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<ChatService, void(shared_ptr<Instance>, std::string, ChatService::ChatColor)> func_chat(&ChatService::chat, "Chat", "partOrCharacter", "message", "color", ChatService::CHAT_BLUE, Security::None);

static Reflection::BoundYieldFuncDesc<ChatService, std::string(std::string, shared_ptr<Instance>)> func_filterString(&ChatService::filterStringForPlayer, "FilterStringForPlayerAsync", "stringToFilter",  "playerToFilterFor", Security::None);

static Reflection::RemoteEventDesc<ChatService, void(shared_ptr<Instance>, std::string, ChatService::ChatColor)> event_Chatted(&ChatService::chattedSignal, "Chatted", "part", "message", "color", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<ChatService::ChatColor>::EnumDesc()
	:EnumDescriptor("ChatColor")
{
	addPair(ChatService::CHAT_BLUE,  "Blue");
	addPair(ChatService::CHAT_GREEN, "Green");
	addPair(ChatService::CHAT_RED,	 "Red");
}

template<>
ChatService::ChatColor& Variant::convert<ChatService::ChatColor>(void)
{
	return genericConvert<ChatService::ChatColor>();
}
}//namespace Reflection

template<>
bool RBX::StringConverter<ChatService::ChatColor>::convertToValue(const std::string& text, ChatService::ChatColor& value)
{
	return Reflection::EnumDesc<ChatService::ChatColor>::singleton().convertToValue(text.c_str(),value);
}
ChatService::ChatService()
{
	setName(sChatService);
}

void ChatService::chat(shared_ptr<Instance> instance, std::string message, ChatService::ChatColor chatColor)
{
	if(!instance)
		throw std::runtime_error("partOrCharacter must be non-nil");
	if(!Workspace::contextInWorkspace(instance.get()))
		throw std::runtime_error("partOrCharacter must be in the Workspace");

	if(instance->fastDynamicCast<ModelInstance>())
	{
		Network::Players* players = ServiceProvider::create<Network::Players>(this);
		if (!players->playerFromCharacter(instance))
			throw std::runtime_error("partOrCharacter is not a legal character");
	}
	else if (!instance->fastDynamicCast<PartInstance>())
	{
		throw std::runtime_error("partOrCharacter must be a Part or a Character");
	}
			
	if(message.empty())
		throw std::runtime_error("message must be non-empty");

	event_Chatted.fireAndReplicateEvent(this, instance, message, chatColor);
}

void ChatService::gotFilteredStringSuccess(std::string response, Network::Player* player, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (!response.empty())
	{
		Reflection::Variant jsonVariant;
		WebParser::parseJSONObject(response, jsonVariant);
		if (jsonVariant.isType<shared_ptr<const Reflection::ValueTable> >())
		{
			shared_ptr<const Reflection::ValueTable> filteredStringTable = jsonVariant.cast<shared_ptr<const Reflection::ValueTable> >();
			Reflection::Variant dataVariant = filteredStringTable->at("data");
			
			if (dataVariant.isType<shared_ptr<const Reflection::ValueTable> >())
			{
				shared_ptr<const Reflection::ValueTable> dataTable = dataVariant.cast<shared_ptr<const Reflection::ValueTable> >();
				Reflection::Variant whiteListPolicyFilterResult = dataTable->at(kWebChatWhiteListPolicy);
				Reflection::Variant blackListPolicyFilterResult = dataTable->at(kWebChatBlackListPolicy);

				if (player->getChatFilterType() == Network::Player::CHAT_FILTER_WHITELIST)
				{
					std::string whiteListString = whiteListPolicyFilterResult.get<std::string>();
					resumeFunction(whiteListString);
					return;
				}
				else
				{
					std::string blackListString = blackListPolicyFilterResult.get<std::string>();
					resumeFunction(blackListString);
					return;
				}
			}
		}

		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ChatService:FilterString", "Filter failed because could not parse response");

		errorFunction("ChatService:FilterString could not parse response");
	}
	else
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ChatService:FilterString", "Filter failed because response was empty");

		errorFunction("ChatService:FilterString returned an empty response");
	}
}

void ChatService::gotFilterStringError(std::string error, boost::function<void(std::string)> errorFunction)
{
	errorFunction(error);
}

static void sendFilterChatStats(int placeId)
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ChatService:FilterStringForPlayerAsync", RBX::format("placeId: %i", placeId).c_str());
}

void ChatService::filterStringForPlayer(std::string stringToFilter, shared_ptr<Instance> playerToFilterFor, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(flag, boost::bind(sendFilterChatStats, DataModel::get(this)->getPlaceID()));
	}

	if (!Network::Players::serverIsPresent(this))
	{
		errorFunction("ChatService:FilterString called on a client, only works from server.");
		return;
	}

	Network::Player* playerFilter = Instance::fastDynamicCast<Network::Player>(playerToFilterFor.get());

	if (!playerFilter)
	{
		errorFunction("ChatService:FilterString called without a valid Player object.");
		return;
	}

	std::stringstream params;
	RBX::HttpAux::AdditionalHeaders headers;
	if (DFFlag::UseComSiftUpdatedWebChatFilterParamsAndHeader)
	{
		DataModel* dataModel = DataModel::get(playerFilter);

		RBX::Network::ConstructModerationFilterTextParamsAndHeaders(
			stringToFilter, 
			playerFilter->getUserID(), 
			dataModel->getPlaceID(), 
			dataModel->getGameInstanceID(), 
			/*out*/ params, 
			/*out*/ headers);
	}
	else
	{
		params << "filters=" << kWebChatWhiteListPolicy;
		params << "&filters=" << kWebChatBlackListPolicy;
		params << "&text=" << Http::urlEncode(stringToFilter);
	}

	if(RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		if (DFFlag::UseComSiftUpdatedWebChatFilterParamsAndHeader)
		{
			apiService->postAsyncWithAdditionalHeaders("moderation/filtertext", params.str(), true, RBX::PRIORITY_DEFAULT, RBX::HttpService::APPLICATION_URLENCODED, headers,
				boost::bind(&ChatService::gotFilteredStringSuccess, this, _1, playerFilter, resumeFunction, errorFunction),
				boost::bind(&ChatService::gotFilterStringError, this, _1, errorFunction) );
		}
		else
		{
			apiService->postAsync("moderation/filtertext", params.str(), true, RBX::PRIORITY_DEFAULT, RBX::HttpService::APPLICATION_URLENCODED,
				boost::bind(&ChatService::gotFilteredStringSuccess, this, _1, playerFilter, resumeFunction, errorFunction),
				boost::bind(&ChatService::gotFilterStringError, this, _1, errorFunction) );
		}
	}
	else
	{
		errorFunction("ChatService:FilterString could not find ApiService.");
		return;
	}
}

}
