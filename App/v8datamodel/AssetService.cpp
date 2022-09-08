#include "stdafx.h"

#include "V8DataModel/AssetService.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/MarketplaceService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "Network/Players.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/standardout.h"
#include "Util/LuaWebService.h"

#include "V8Xml/WebParser.h"
#include "Util/Http.h"

#include <boost/lexical_cast.hpp>


DYNAMIC_FASTINTVARIABLE(CreatePlacePerMinute, 5)
DYNAMIC_FASTINTVARIABLE(CreatePlacePerPlayerPerMinute, 1)

DYNAMIC_FASTINTVARIABLE(SavePlacePerMinute, 10)

namespace RBX {
	const char* const sAssetService = "AssetService";

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<AssetService, void(std::string)> func_SetPlaceAccessUrl(&AssetService::setPlaceAccessUrl, "SetPlaceAccessUrl", "accessUrl", Security::LocalUser);
	static Reflection::BoundFuncDesc<AssetService, void(std::string)> func_SetAssetRevertUrl(&AssetService::setAssetRevertUrl, "SetAssetRevertUrl", "revertUrl", Security::LocalUser);
	static Reflection::BoundFuncDesc<AssetService, void(std::string)> func_setAssetVersionsUrl(&AssetService::setAssetVersionsUrl, "SetAssetVersionsUrl", "versionsUrl", Security::LocalUser);	

	static Reflection::BoundYieldFuncDesc<AssetService, bool(int, int)> func_revertAsset(&AssetService::revertAsset, "RevertAsset", "placeId", "versionNumber", Security::None); 
	static Reflection::BoundYieldFuncDesc<AssetService, bool(int, AssetService::AccessType, shared_ptr<const Reflection::ValueArray>)> 
		func_updatePermissions(&AssetService::setPlacePermissions, "SetPlacePermissions", "placeId", "accessType", AssetService::EVERYONE, "inviteList", shared_ptr<const Reflection::ValueArray>(), Security::None);
	static Reflection::BoundYieldFuncDesc<AssetService, shared_ptr<const Reflection::ValueTable>(int)> func_getPlacePermissions(&AssetService::getPlacePermissions, "GetPlacePermissions", "placeId", Security::None); 
	static Reflection::BoundYieldFuncDesc<AssetService, shared_ptr<const Reflection::ValueTable>(int, int)> func_getAssetVersions(&AssetService::getAssetVersions, "GetAssetVersions", "placeId", "pageNum", 1, Security::None);

	static Reflection::BoundYieldFuncDesc<AssetService, int(int)> func_getCreatorAssetID(&AssetService::getCreatorAssetID, "GetCreatorAssetID", "creationID", Security::None); 

	static Reflection::BoundYieldFuncDesc<AssetService, int(std::string, int, std::string)>  func_createPlace
		(&AssetService::createPlaceAsync, "CreatePlaceAsync", "placeName","templatePlaceID","description","",Security::None);

	static Reflection::BoundYieldFuncDesc<AssetService, int(shared_ptr<Instance>, std::string, int, std::string)>  func_createPlaceInPlayerInventory
		(&AssetService::createPlaceInPlayerInventoryAsync, "CreatePlaceInPlayerInventoryAsync",  "player",
			"placeName", "templatePlaceID", "description", "",Security::None);
	static Reflection::BoundYieldFuncDesc<AssetService, void()> func_savePlace
		(&AssetService::savePlaceAsync, "SavePlaceAsync", Security::None);

	static Reflection::BoundYieldFuncDesc<AssetService, shared_ptr<Instance>()>
		func_getGamePlacesAsync(&AssetService::getGamePlacesAsync, "GetGamePlacesAsync", Security::None);
    REFLECTION_END();

	namespace Reflection {
		template<>
		EnumDesc<AssetService::AccessType>::EnumDesc()
			:EnumDescriptor("AccessType")
		{
			addPair(AssetService::ME,			"Me");		
			addPair(AssetService::FRIENDS,		"Friends");
			addPair(AssetService::EVERYONE,		"Everyone");
			addPair(AssetService::INVITEONLY,	"InviteOnly");			
		}

		template<>
		AssetService::AccessType& Variant::convert<AssetService::AccessType>(void)
		{
			return genericConvert<AssetService::AccessType>();
		}
	}//namespace Reflection

	template<>
	bool StringConverter<AssetService::AccessType>::convertToValue(const std::string& text, AssetService::AccessType& value)
	{
		return Reflection::EnumDesc<AssetService::AccessType>::singleton().convertToValue(text.c_str(),value);
	}

	
	AssetService::AssetService()
		:placeAccessUrl("")
		,assetRevertUrl("")
		,assetVersionsUrl("")
		,createPlaceThrottle(&DFInt::CreatePlacePerMinute, &DFInt::CreatePlacePerPlayerPerMinute)
		,savePlaceThrottle(&DFInt::SavePlacePerMinute)
	{
		setName(sAssetService);
	}
	

	void AssetService::setPlaceAccessUrl(std::string url)
	{
		placeAccessUrl = url; 
	}	

	void AssetService::setAssetRevertUrl(std::string url)
	{
		assetRevertUrl = url; 
	}

	void AssetService::setAssetVersionsUrl(std::string url)
	{
		assetVersionsUrl = url; 
	}


	std::string enumToString(AssetService::AccessType eType) 
	{
		switch(eType)
		{
		case AssetService::ME:
			return std::string("Me");
		case AssetService::FRIENDS:
			return std::string("Friends");
		case AssetService::EVERYONE:
			return std::string("Everyone");
		case AssetService::INVITEONLY:
			return std::string("InviteOnly");
		default:
			return std::string("Me");
		}
	}

	void AssetService::processServiceResults(std::string *result, const std::exception* httpException, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!httpException) 
		{
			if(!result || result->empty()) 
			{
				errorFunction("Invalid response received"); 
			}
			shared_ptr<const Reflection::ValueTable> lTable;
			std::stringstream resultStream;
			resultStream << *result;
            
            
			if( WebParser::parseJSONTable(*result, lTable) )
				resumeFunction(lTable);
			else 
				errorFunction("AssetService error occurred");		
		}		
		else 
		{
			errorFunction(RBX::format("Http exception occurred %s", httpException->what())); 
		}
	}

	namespace {
		static void sendCreatePlaceStats()
		{
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CreatePlace");
		}
		static void sendCreatePlaceInPlayerInventoryStats()
		{
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CreatePlaceInPlayerInventory");
		}
	} // namespace

	static void CreatePlaceSuccessHelper(weak_ptr<DataModel> dm, const std::string& response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> sharedDM = dm.lock())
		{
			if (!response.empty())
			{
				int newPlaceID = 0;
				try 
				{
					newPlaceID = boost::lexical_cast<int>(response.c_str());
				} 
				catch (boost::bad_lexical_cast const& e) 
				{
					errorFunction( RBX::format("Game:CreatePlace response was not a valid placeId because %s",e.what()) );
					return;
				}

				if (newPlaceID > 0)
				{
					resumeFunction( newPlaceID );
				}
				else
				{
					errorFunction("Game:CreatePlace response was not a valid placeId because id <= 0");
				}
			}
			else
			{
				errorFunction("Game:CreatePlace had no valid response");
			}
		}
	}

	static void CreatePlaceErrorHelper(weak_ptr<DataModel> dm, const std::string& error, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> sharedDM = dm.lock())
		{
			if (!error.empty())
			{
				errorFunction( RBX::format("Game:CreatePlace received and error: %s.",error.c_str()) );
			}
			else
			{
				errorFunction("Game:CreatePlace had no valid response in error helper.");
			}
		}
	}

	bool AssetService::checkCreatePlaceAccess(const std::string& placeName, int templatePlaceId, std::string& message)
	{
		DataModel* dm = DataModel::get(this);
		RBXASSERT(dm);

		if(!Network::Players::backendProcessing(this))
		{
			message = "CreatePlaceAsync can only be called from a server script, aborting create function";
			return false;
		}

		if (placeName.empty())
		{
			message = "CreatePlaceAsync placeName argument is empty!";
			return false;
		}

		if (templatePlaceId <= 0)
		{
			message = "CreatePlaceAsync templatePlaceId <= 0, should be a positive value";
			return false;
		}

		if(dm->getPlaceID() <= 0)
		{
			message = "CreatePlaceAsync called on a Place with id <= 0, place should be opened with Edit button to access CreatePlace";
			return false;
		}

		if(!createPlaceThrottle.checkLimit(Network::Players::getPlayerCount(this)))
		{
			message = "CreatePlaceAsync requests limit reached";
			return false;
		}

		if (LuaWebService* lws = dm->create<LuaWebService>())
		{
			if (!lws->isApiAccessEnabled())
			{
				message = "Studio API access is not enabled. Enable it by going to the Game Settings page.";
				return false;
			}
		}

		return true;
	}

	void AssetService::createPlaceAsync(std::string placeName, int templatePlaceId, std::string desc, 
		boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string message;
		if(!checkCreatePlaceAccess(placeName, templatePlaceId, message))
		{
			errorFunction(message);
			return;
		}
		createPlaceAsyncInternal(true, placeName, templatePlaceId, desc, shared_ptr<Instance>(), resumeFunction, errorFunction);
	}

	void AssetService::createPlaceInPlayerInventoryAsync(shared_ptr<Instance> player, std::string placeName, int templatePlaceId, 
		std::string desc, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string message;
		if(!checkCreatePlaceAccess(placeName, templatePlaceId, message))
		{
			errorFunction(message);
			return;
		}

		MarketplaceService* marketPlaceService = ServiceProvider::create<MarketplaceService>(this);
		marketPlaceService->launchClientLuaDialog("Do you allow game to create new place in your inventory?", "Yes", "No", player, 
			boost::bind(&AssetService::createPlaceAsyncInternal, this, _1, placeName, templatePlaceId, desc, _2, resumeFunction, errorFunction));
	}

	void AssetService::createPlaceAsyncInternal(bool check, std::string placeName, int templatePlaceId, std::string desc,
		shared_ptr<Instance> playerInstance, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!check)
		{
			StandardOut::singleton()->print(MESSAGE_WARNING, "User didn't allow place creation");
			resumeFunction(-1);
			return;
		}

		shared_ptr<Network::Player> player = fastSharedDynamicCast<Network::Player>(playerInstance);
 		if(player && player->getUserID() <= 0)
 		{
 			errorFunction("CreatePlaceAsync requires player to be logged in");
 			return;
 		}

		if(player)
		{
			Network::Players* players = ServiceProvider::find<Network::Players>(this);
			if(!players || players->getPlayerByID(player->getUserID()) != player)
			{
				errorFunction("Player no longer valid");
				return;
			}
		}

		DataModel* dm = DataModel::get(this);
		
		std::string parameters = RBX::format("?currentPlaceId=%d&placeName=%s&templatePlaceId=%d",
			dm->getPlaceID(),RBX::Http::urlEncode(placeName).c_str(),templatePlaceId);
		std::string baseUrl = ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();

		std::string playerParameter = "";
		if(player)
			playerParameter = RBX::format("&playerId=%i", player->getUserID());

		if (player)
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendCreatePlaceInPlayerInventoryStats, flag);
		} 
		else
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendCreatePlaceStats, flag);
		}
		

		try
		{
			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
			{
				// now post to the website
				Http createHttp((baseUrl + "universes/new-place" + parameters + playerParameter).c_str());
#ifndef _WIN32
				createHttp.setAuthDomain(baseUrl);
#endif
                std::string postString("CreatePlacePost");
				apiService->postAsync(createHttp, postString, RBX::PRIORITY_SERVER_ELEVATED, HttpService::TEXT_PLAIN,
					boost::bind(&CreatePlaceSuccessHelper, shared_from(dm), _1, resumeFunction, errorFunction),
					boost::bind(&CreatePlaceErrorHelper, shared_from(dm), _1, errorFunction));
			}
		}
		catch (base_exception &e)
		{
			errorFunction( RBX::format("Game:CreatePlace failed because %s",e.what()) );
		}
	}

	namespace {
		static void sendSavePlaceStats()
		{
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SavePlace");
		}
	} // namespace

	void AssetService::savePlaceAsync(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		DataModel* dm = DataModel::get(this);
		dm->checkFetchExperimentalFeatures();

		if (dm->getPlaceID() <= 0)
		{
			errorFunction("Game:SavePlace placeID is not valid!");
			return;
		}

		if(!savePlaceThrottle.checkLimit())
		{
			errorFunction("Game:SavePlace requests limit reached");
			return;
		}

		if(Network::Players::frontendProcessing(this))
		{
			errorFunction("Game:SavePlace can only be called from a server script, aborting save function");
			return;
		}
		else if(Network::Players::backendProcessing(this))
		{
			if (LuaWebService* lws = dm->create<LuaWebService>())
			{
				if (!lws->isApiAccessEnabled())
				{
					errorFunction("Studio API access is not enabled. Enable it by going to the Game Settings page.");
					return;
				}
			}

			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(&sendSavePlaceStats, flag);
			}

			// construct the url
			std::string parameters = "?assetId=" + boost::lexical_cast<std::string>(dm->getPlaceID());
			parameters += "&isAppCreation=true";
			std::string baseUrl = ServiceProvider::create<ContentProvider>(this)->getBaseUrl();

			// save the place
			dm->uploadPlace(baseUrl + "ide/publish/UploadExistingAsset" + parameters, SAVE_ALL, resumeFunction, errorFunction);
		}
		else
		{
			errorFunction("Game:SavePlace could not determine if client or server");
			return;
		}
	}

	void AssetService::getAssetVersions(int assetId, int pageNum, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)		
	{
		Http http(RBX::format(assetVersionsUrl.c_str(), assetId, pageNum)); 
		http.get(boost::bind(&AssetService::processServiceResults, this, _1, _2, resumeFunction, errorFunction));
	}
	
	void AssetService::getPlacePermissions(int placeId, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		Http http(RBX::format(placeAccessUrl.c_str(), placeId));		
		http.get(boost::bind(&AssetService::processServiceResults, this, _1, _2, resumeFunction, errorFunction)); 	
	}

	void AssetService::httpPostHelper(std::string* response, std::exception* httpException, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!httpException)
		{				
			if(!response) 
				resumeFunction(false);
			if(*response=="") 
				resumeFunction(false);
			else
				resumeFunction(true); 
		}
		else
		{
			errorFunction(httpException->what());
		}
	}

	void AssetService::revertAsset(int assetId, int versionNumber, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{		
		Http http(RBX::format(assetRevertUrl.c_str(), assetId, versionNumber));
		std::string in; 
		http.post(in, Http::kContentTypeDefaultUnspecified, true,
			boost::bind(&AssetService::httpPostHelper, this, _1, _2, resumeFunction, errorFunction));		
	}

	void AssetService::setPlacePermissions(int placeId, AccessType type, shared_ptr<const Reflection::ValueArray> inviteList, 
		boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{			
		std::string url = placeAccessUrl; 		
		url = RBX::format(url.append("/update?").c_str(), placeId); 
		url = RBX::format(url.append("access=%s").c_str(), enumToString(type).c_str()); 		

		if(type == INVITEONLY && inviteList) 
		{		
			
			for(Reflection::ValueArray::const_iterator iter = inviteList->begin(); iter != inviteList->end(); ++iter)
			{
				std::stringstream convert;								
				convert << iter->get<std::string>();
				url.append("&players=").append(convert.str()); 
			}
		}
		Http http(url); 
		std::string in; 
		http.post(in, Http::kContentTypeDefaultUnspecified, true,
			boost::bind(&AssetService::httpPostHelper, this, _1, _2, resumeFunction, errorFunction));		
	}

	void AssetService::getCreatorAssetIDSuccessHelper(std::string response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{	
		if( response.length() == 0 )
		{
			resumeFunction(0);
			return;
		}

		int creatorID = 0;
		try 
		{
			creatorID = boost::lexical_cast<int>(response);
		} 
		catch( boost::bad_lexical_cast const& e)
		{
			errorFunction( RBX::format("AssetService:GetCreatorAssetID response could not convert to number because %s",e.what()) );
			return;
		}

		if(creatorID <= 0)
		{
			errorFunction("AssetService:GetCreatorAssetID response converted but is an invalid assetID");
			return;
		}

		resumeFunction(creatorID);
	}

	void AssetService::getCreatorAssetIDErrorHelper(std::string error, boost::function<void(std::string)> errorFunction)
	{
		if (error.length() == 0)
		{
			errorFunction("AssetService::GetCreatorAssetID did not get a response or an error.");
		}

		errorFunction(RBX::format("AssetService:GetCreatorAssetID error: %s", error.c_str()));
	}

	void AssetService::getCreatorAssetID(int creationID, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (creationID <= 0)
		{
			errorFunction("creationID is not a valid number (should be a positive integer)");
			return;
		}

		std::string baseUrl = RBX::ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();
		std::string parameters =  RBX::format("?creationID=%d",creationID);

		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			apiService->getAsync("GetCreatorAssetID" + parameters, true, RBX::PRIORITY_DEFAULT,
				boost::bind(&AssetService::getCreatorAssetIDSuccessHelper, this, _1, resumeFunction, errorFunction),
				boost::bind(&AssetService::getCreatorAssetIDErrorHelper, this, _1, errorFunction) );
		}
	}

	void AssetService::getGamePlacesAsync(boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		std::string fetchUrl;
		ContentProvider* cp = DataModel::get(this)->find<ContentProvider>();
		if (cp)
			fetchUrl = format("%suniverses/get-universe-places?placeid=%i", cp->getApiBaseUrl().c_str(), DataModel::get(this)->getPlaceID());

	
		shared_ptr<Pages> pagination = Creatable<Instance>::create<StandardPages>(weak_from(DataModel::get(this)), fetchUrl, "Places");
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
	}
}
