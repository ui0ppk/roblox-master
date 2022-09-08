#include "stdafx.h"

#include "V8DataModel/MarketplaceService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/LuaWebService.h"
#include "Util/Analytics.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "v8xml/WebParser.h"
#include <boost/algorithm/string.hpp>

DYNAMIC_FASTFLAGVARIABLE(CheckMarketplaceAvailable, false)
DYNAMIC_FASTFLAGVARIABLE(Order66, false)
DYNAMIC_FASTINTVARIABLE(ExpireMarketPlaceServiceCacheSeconds, 60)
DYNAMIC_FASTFLAGVARIABLE(RestrictSales, false)
FASTFLAGVARIABLE(UseNewPromptEndHandling, false)

// When the client reports an asset was purchased in game, make a second call to verify.
DYNAMIC_FASTFLAGVARIABLE(DoubleCheckPurchase, true)

// In the event the web call generates an error, allow the client message to pass
DYNAMIC_FASTFLAGVARIABLE(AllowClientFallback, true)

DYNAMIC_FASTFLAGVARIABLE(IgnoreDifferentPlayer, true)

// Reporting Rates.
DYNAMIC_FASTINTVARIABLE(PurchaseMismatchReportRate, 100)
DYNAMIC_FASTINTVARIABLE(PurchaseErrorReportRate, 100)


namespace RBX
{

    REFLECTION_BEGIN();
	// backend
	static Reflection::BoundFuncDesc<MarketplaceService, void(int, int, bool)> func_signalPromptProductFinished(&MarketplaceService::signalPromptProductPurchaseFinished, "SignalPromptProductPurchaseFinished", "userId", "productId", "success", Security::RobloxScript);
	static Reflection::BoundFuncDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool)> func_signalPromptPurchaseFinished(&MarketplaceService::signalPromptPurchaseFinished, "SignalPromptPurchaseFinished", "player", "assetId", "success", Security::RobloxScript);

	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool, MarketplaceService::CurrencyType)> event_promptPurchaseRequested(&MarketplaceService::promptPurchaseRequested, "PromptPurchaseRequested", "player", "assetId", "equipIfPurchased", "currencyType", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool, MarketplaceService::CurrencyType)> event_promptProductPurchaseRequested(&MarketplaceService::promptProductPurchaseRequested, "PromptProductPurchaseRequested", "player", "productId", "equipIfPurchased", "currencyType", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);

	static Reflection::BoundFuncDesc<MarketplaceService, void(std::string, int, int)> func_signalClientPurchaseSuccess(&MarketplaceService::signalClientPurchaseSuccess, "SignalClientPurchaseSuccess", "ticket", "playerId", "productId", Security::RobloxScript);
	static Reflection::RemoteEventDesc<MarketplaceService, void(std::string, int, int)> event_clientPurchaseSuccess(&MarketplaceService::clientPurchaseSuccess, "ClientPurchaseSuccess", "ticket", "playerId", "productId", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<const Reflection::ValueTable>)> event_serverPurchaseVerification(&MarketplaceService::serverPurchaseVerification, "ServerPurchaseVerification", "serverResponseTable", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);

	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, std::string)> event_promptThirdPartyPurchaseRequested(&MarketplaceService::promptThirdPartyPurchaseRequested, "PromptThirdPartyPurchaseRequested", "player", "productId", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
    
    static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, std::string)> event_promptNativePurchaseRequested(&MarketplaceService::promptNativePurchaseRequested, "PromptNativePurchaseRequested", "player", "productId", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

	// frontend
    
    // used by core script to automatically convert app store product purchases into roblox items
    static Reflection::BoundFuncDesc<MarketplaceService, void(shared_ptr<Instance>, std::string)> func_PromptNativePurchase(&MarketplaceService::promptNativePurchase, "PromptNativePurchase", "player", "productId", Security::RobloxScript);
	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, std::string, bool)> event_PromptNativePurchaseFinished(&MarketplaceService::nativePurchaseFinished, "NativePurchaseFinished", "player","productId","wasPurchased", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
    
    
    // used by some roblox created games to do direct purchases from app stores
	static Reflection::BoundFuncDesc<MarketplaceService, void(shared_ptr<Instance>, std::string)> func_PromptTPPurchase(&MarketplaceService::promptThirdPartyPurchase, "PromptThirdPartyPurchase", "player", "productId", Security::RobloxPlace);
	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, std::string, std::string, bool)> event_PromptTPPurchaseFinished(&MarketplaceService::thirdPartyPurchaseFinished, "ThirdPartyPurchaseFinished", "player","productId","receipt","wasPurchased", Security::RobloxPlace, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

    
    // rbx dev purchasing calls
	static Reflection::BoundYieldFuncDesc<MarketplaceService, shared_ptr<const Reflection::ValueTable>(int, MarketplaceService::InfoType)> func_getProductInfo(&MarketplaceService::getProductInfo, "GetProductInfo", "assetId","infoType",MarketplaceService::INFO_ASSET, Security::None);
	static Reflection::BoundYieldFuncDesc<MarketplaceService, bool(shared_ptr<Instance>, int)> func_playerOwnsAsset(&MarketplaceService::playerOwnsAsset, "PlayerOwnsAsset", "player", "assetId", Security::None);
	static Reflection::BoundFuncDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool, MarketplaceService::CurrencyType)> func_PromptPurchase(&MarketplaceService::promptPurchase, "PromptPurchase", "player", "assetId", "equipIfPurchased", true, "currencyType",MarketplaceService::CURRENCY_DEFAULT, Security::None);
	static Reflection::BoundFuncDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool, MarketplaceService::CurrencyType)> func_PromptProductPurchase(&MarketplaceService::promptProductPurchase, "PromptProductPurchase", "player", "productId", "equipIfPurchased", true, "currencyType",MarketplaceService::CURRENCY_DEFAULT, Security::None);
	static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<Instance>, int, bool)> event_promptPurchaseFinished(&MarketplaceService::promptPurchaseFinished, "PromptPurchaseFinished", "player", "assetId", "isPurchased", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
	static Reflection::RemoteEventDesc<MarketplaceService, void(int, int, bool)> event_promptProductPurchaseFinished(&MarketplaceService::promptProductPurchaseFinished, "PromptProductPurchaseFinished", "userId", "productId", "isPurchased", Security::None, Reflection::RemoteEventCommon::Attributes::deprecated(Reflection::RemoteEventCommon::SCRIPTING, NULL), Reflection::RemoteEventCommon::BROADCAST);

    static Reflection::RemoteEventDesc<MarketplaceService, void(shared_ptr<const Reflection::Tuple>)> event_ClientLuaDialogRequested(&MarketplaceService::clientLuaDialogRequested, "ClientLuaDialogRequested", "arguments", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<MarketplaceService, void(bool, shared_ptr<Instance>)> event_luaDialogCallbackSignal(&MarketplaceService::luaDialogCallbackSignal, "LuaDialogCallbackSignal", "value", "player", Security::RobloxScript, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::BoundFuncDesc<MarketplaceService, void(bool)> func_signalServerLuaDialogClosed(&MarketplaceService::signalServerLuaDialogClosed, "SignalServerLuaDialogClosed", "value", Security::RobloxScript);
    
    static Reflection::BoundYieldFuncDesc<MarketplaceService, shared_ptr<Instance>()>
        func_getDeveloperProductsAsync(&MarketplaceService::getDeveloperProductsAsync, "GetDeveloperProductsAsync", Security::None);
    
	static Reflection::BoundAsyncCallbackDesc<
		MarketplaceService, MarketplaceService::ProductPurchaseDecision(
		shared_ptr<const Reflection::ValueTable>)> callback_ProcessReceipt(
		"ProcessReceipt", &MarketplaceService::receiptCallback, "receiptInfo",
		&MarketplaceService::processReceiptsCallbackChanged);
    REFLECTION_END();
    
    
	 
	const char* const sMarketplaceService = "MarketplaceService";

	namespace Reflection {
	template<>
	EnumDesc<MarketplaceService::CurrencyType>::EnumDesc()
		:EnumDescriptor("CurrencyType")
	{
		addPair(MarketplaceService::CURRENCY_DEFAULT, "Default");
		addPair(MarketplaceService::CURRENCY_ROBUX, "Robux");
		addPair(MarketplaceService::CURRENCY_TIX, "Tix");
	}

	template<>
	MarketplaceService::CurrencyType& Variant::convert<MarketplaceService::CurrencyType>(void)
	{
		return genericConvert<MarketplaceService::CurrencyType>();
	}

    template<>
	EnumDesc<MarketplaceService::InfoType>::EnumDesc()
		:EnumDescriptor("InfoType")
	{
		addPair(MarketplaceService::INFO_ASSET, "Asset");
		addPair(MarketplaceService::INFO_PRODUCT, "Product");
	}

	template<>
	MarketplaceService::InfoType& Variant::convert<MarketplaceService::InfoType>(void)
	{
		return genericConvert<MarketplaceService::InfoType>();
	}

	template<>
	EnumDesc<MarketplaceService::ProductPurchaseDecision>::EnumDesc()
		:EnumDescriptor("ProductPurchaseDecision")
	{
		addPair(MarketplaceService::DECISION_NOT_PROCESSED_YET, "NotProcessedYet");
		addPair(MarketplaceService::DECISION_PURCHASE_GRANTED, "PurchaseGranted");
	}

	template<>
	MarketplaceService::ProductPurchaseDecision& Variant::convert<MarketplaceService::ProductPurchaseDecision>(void)
	{
		return genericConvert<MarketplaceService::ProductPurchaseDecision>();
	}

	} // namespace Reflection

	template<>
	bool StringConverter<MarketplaceService::CurrencyType>::convertToValue(const std::string& text, MarketplaceService::CurrencyType& value)
	{
		return Reflection::EnumDesc<MarketplaceService::CurrencyType>::singleton().convertToValue(text.c_str(), value);
	}

	template<>
	bool StringConverter<MarketplaceService::InfoType>::convertToValue(const std::string& text, MarketplaceService::InfoType& value)
	{
		return Reflection::EnumDesc<MarketplaceService::InfoType>::singleton().convertToValue(text.c_str(), value);
	}

	template<>
	bool StringConverter<MarketplaceService::ProductPurchaseDecision>::convertToValue(const std::string& text, MarketplaceService::ProductPurchaseDecision& value)
	{
		return Reflection::EnumDesc<MarketplaceService::ProductPurchaseDecision>::singleton().convertToValue(text.c_str(), value);
	}

	MarketplaceService::MarketplaceService() :
		productInfoUrl(std::string(urlApiPath()) + "/productinfo?assetId=%d")
		, playerOwnsAssetUrl("ownership/hasasset?userId=%d&assetId=%d")
		, devProductInfoUrl(std::string(urlApiPath()) + "/productDetails?productId=%d")
        , clientLuaDialogRequested()
		, luaDialogCallbackSignal()
		, receiptProcessingEnabledByUser(false)
	{
		setName(sMarketplaceService);
        

		luaDialogCallbackSignal.connect(boost::bind(&MarketplaceService::executeClientDialogCallback, this, _1, _2));
	}

	bool MarketplaceService::isValidPlayer(const shared_ptr<Instance> player, const std::string& funcName, boost::function<void(std::string)> errorFunction)
	{
		if(Network::Player* thePlayer = Instance::fastDynamicCast<Network::Player>(player.get()))
		{
			if( thePlayer->getUserID() <=0 ) // warn when we use a guest player (otherwise this call will fail in all local test scenarios!)
			{
				if (funcName != "PromptPurchase()" && funcName != "PromptProductPurchase()")
				{
					std::string longFuncName = RBX::format("MarketplaceService:%s",funcName.c_str());
					 longFuncName.append(" was called on a guest, please use this call only on players with an account.");
                
					RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, longFuncName);
					return false;
				}
			} 
		} 
		else // we don't have a player type object, are you even trying?
		{
            std::string longFuncName = RBX::format("MarketplaceService:%s",funcName.c_str());
            longFuncName.append(" player should be of type Player, but is of type ");
            longFuncName.append(player ? player->getClassNameStr().c_str() : "nil");
            
            if(errorFunction)
                errorFunction(longFuncName);
            else
                throw std::runtime_error(longFuncName);
            
            return false;
		}

		return true;
	}

	void MarketplaceService::promptProductPurchase(const shared_ptr<Instance> player, const int productId, const bool equipIfPurchased, const CurrencyType currencyType)
	{
		if(!isValidPlayer(player,"PromptProductPurchase()"))
			return;

		if(Network::Players* players = ServiceProvider::find<RBX::Network::Players>(this))
			if(Network::Players::frontendProcessing(this) &&  (player.get() != players->getLocalPlayer()) )
			{
				RBX::StandardOut::singleton()->printf( RBX::MESSAGE_WARNING, "MarketplaceService:PromptProductPurchase called from a local script, but not called on a local player. Local scripts can only prompt the local player.");
				return;
			}

		if(productId <= 0)
			throw RBX::runtime_error("MarketplaceService:PromptPurchase() second argument is not a valid productId (supplied productId was less than 0)");

		DataModel* dm = DataModel::get(this);
		RBXASSERT(dm);
		if (dm)
			if (LuaWebService* lws = dm->create<LuaWebService>())
				if (!lws->isApiAccessEnabled())
					throw RBX::runtime_error("Studio API access is not enabled. Enable it by going to the Game Settings page.");

		// fire a signal so our purchase script can show the purchase ui and eventually call signalPromptProductPurchaseFinished
		event_promptProductPurchaseRequested.fireAndReplicateEvent(this, player,productId,equipIfPurchased,currencyType);
	}

	void MarketplaceService::promptPurchase(const shared_ptr<Instance> player, const int assetId, const bool equipIfPurchased, const CurrencyType currencyType)
	{
		if(!isValidPlayer(player,"PromptPurchase()"))
			return;

		if(Network::Players* players = ServiceProvider::find<RBX::Network::Players>(this))
			if(Network::Players::frontendProcessing(this) &&  (player.get() != players->getLocalPlayer()) )
			{
				RBX::StandardOut::singleton()->printf( RBX::MESSAGE_WARNING, "MarketplaceService:PromptPurchase called from a local script, but not called on a local player. Local scripts can only prompt the local player.");
				return;
			}

		if(assetId <= 0)
			throw RBX::runtime_error("MarketplaceService:PromptPurchase() second argument is not a valid assetId (supplied assetId was less than 0)");

		DataModel* dm = DataModel::get(this);
		RBXASSERT(dm);
		if (dm)
			if (LuaWebService* lws = dm->create<LuaWebService>())
				if (!lws->isApiAccessEnabled())
					throw RBX::runtime_error("Studio API access is not enabled. Enable it by going to the Game Settings page.");

		// fire a signal so our purchase script can show the purchase ui and eventually call signalPromptPurchaseFinished
		event_promptPurchaseRequested.fireAndReplicateEvent(this, player,assetId,equipIfPurchased,currencyType);
	}

	void MarketplaceService::signalPromptPurchaseFinished(const shared_ptr<Instance> player, const int assetId, const bool isPurchase)
	{
		event_promptPurchaseFinished.fireAndReplicateEvent(this, player, assetId, isPurchase);
	}

    void MarketplaceService::processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source)
    {
        if (DFFlag::DoubleCheckPurchase && descriptor == event_promptPurchaseFinished )
        {
            if (args.size() != 3 || !args[2].isType<bool>() || !args[1].isType<int>() || !args[0].isType<shared_ptr<Instance> >())
            {
                return;
            }
            bool isPurchase = args[2].cast<bool>();
            if (isPurchase)
            {
                shared_ptr<Instance> playerInstance = args[0].cast<shared_ptr<Instance> >();
                shared_ptr<Network::Player> player = Instance::fastSharedDynamicCast<Network::Player>(playerInstance);
                if (!player)
                {
                    return;
                }

                // it is possible game logic could ban players who fake purchases.  This would result in a way
                // to get other players banned by spoofing.  
                const SystemAddress playerSystemAddress = player->getRemoteAddressAsRbxAddress();
                if (DFFlag::IgnoreDifferentPlayer && playerSystemAddress != source)
                {
                    return;
                }

                weak_ptr<DataModel> weakDataModel = weak_from(DataModel::get(this));
                int assetId = args[1].cast<int>();

                playerOwnsAsset(player, assetId,
                    boost::bind(&MarketplaceService::processPurchaseFinished, weakDataModel, args, source, _1), // the arguments should be passed by value.
                    boost::bind(&MarketplaceService::processPurchaseError, weakDataModel, args, source, _1));   // the arguments should be passed by value.
            }
            else
            {
                Super::processRemoteEvent(descriptor, args, source);
            }
        }
        else 
        {
            Super::processRemoteEvent(descriptor, args, source);
        }
    }

    void MarketplaceService::processPurchaseFinished( weak_ptr<DataModel> weakDm, Reflection::EventArguments args, SystemAddress source, const bool isOwner)
    {
        bool isPurchase = args[2].cast<bool>();

        //  purchase  own  = valid purchase
        //  purchase !own = exploit
        // !purchase  own = impossible
        // !purchase !own = valid non-purchase
        // (If a developer gives the player some additional benefits at time of purchase, this will still allow re-triggering).
        if (isPurchase == isOwner)
        {
            if (shared_ptr<DataModel> dm = weakDm.lock())
            {
                // some of the arguments are copied as this is async
                dm->submitTask(boost::bind(&MarketplaceService::processPurchaseFinishedCallback, _1, args, source), DataModelJob::Write);
            }
        }
        else
        {
            Analytics::InfluxDb::Points purchaseMismatchAnalytics;
            purchaseMismatchAnalytics.addPoint("isPurchase", isPurchase);
            purchaseMismatchAnalytics.addPoint("isOwner", isOwner);
            purchaseMismatchAnalytics.report("purchaseVerificationMismatch", DFInt::PurchaseMismatchReportRate);
        }
    }

    void MarketplaceService::processPurchaseError( weak_ptr<DataModel> weakDm, Reflection::EventArguments args, SystemAddress source, const std::string msg)
    {
        Analytics::InfluxDb::Points purchaseErrorAnalytics;
        purchaseErrorAnalytics.addPoint("msg", msg.c_str());
        purchaseErrorAnalytics.report("purchaseVerificationError", DFInt::PurchaseErrorReportRate);

        // There was an error with the web.  It may be safer to assume the client is telling the truth.
        if (DFFlag::AllowClientFallback)
        {
            if (shared_ptr<DataModel> dm = weakDm.lock())
            {
                // some of the arguments are copied as this is async
                dm->submitTask(boost::bind(&MarketplaceService::processPurchaseFinishedCallback, _1, args, source), DataModelJob::Write);
            }
        }
    }

    void MarketplaceService::processPurchaseFinishedCallback(DataModel* dm, Reflection::EventArguments args, SystemAddress source)
    {
        if (MarketplaceService* market = dm->find<MarketplaceService>())
        {
            market->Super::processRemoteEvent(event_promptPurchaseFinished, args, source);
        }
    }


	void MarketplaceService::signalPromptProductPurchaseFinished(const int userId, const int productId, const bool success)
	{
		event_promptProductPurchaseFinished.fireAndReplicateEvent(this,userId,productId,success);
	}

	template<typename ResultType>
	void MarketplaceService::dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(LuaWebService* luaWebService = ServiceProvider::create<LuaWebService>(this))
		{ 
			try
			{
				luaWebService->asyncRequest(url, LUA_WEB_SERVICE_STANDARD_PRIORITY, resumeFunction, errorFunction);
			}
			catch(RBX::base_exception&)
			{
				errorFunction("MarketplaceService:getProductInfo() - Error during dispatch");
			}
		} 
		else
		{
			errorFunction("MarketplaceService:getProductInfo() - Shutting down");
		}
	}

	void MarketplaceService::getProductInfo(const int assetId, const MarketplaceService::InfoType infoType, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(infoType == MarketplaceService::INFO_ASSET && productInfoUrl.empty())
		{
			errorFunction("MarketplaceService:GetProductInfo() productInfoUrl is empty");
			return;
		}
		else if (infoType == MarketplaceService::INFO_PRODUCT && devProductInfoUrl.empty())
		{
			errorFunction("MarketplaceService:GetProductInfo() devProductInfoUrl is empty");
			return;
		}
		else if(infoType == MarketplaceService::INFO_NONE)
		{
			errorFunction("MarketplaceService:GetProductInfo() was passed an invalid info type (NONE)");
			return;
		}

		if(assetId <= 0)
		{
			errorFunction("MarketplaceService:GetProductInfo() argument is not a valid assetId (supplied assetId was less than 0)");
			return;
		}

		std::string url = "";
		if (ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this))
		{
			url = contentProvider->getApiBaseUrl();
		}

		if( infoType == MarketplaceService::INFO_ASSET )
			url += productInfoUrl;
		else if( infoType == MarketplaceService::INFO_PRODUCT)
			url += devProductInfoUrl;

		if(url.empty())
		{
			errorFunction("MarketplaceService:GetProductInfo() could not find correct url to call");
			return;
		}

		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			std::string infoPath = "";
			if( infoType == MarketplaceService::INFO_ASSET )
				infoPath = productInfoUrl;
			else if( infoType == MarketplaceService::INFO_PRODUCT)
				infoPath = devProductInfoUrl;

			std::string infoPathFull(RBX::format(infoPath.c_str(),assetId));

			UrlToResponseMap::const_iterator it = mMap.find(infoPathFull);
			if(it != mMap.end())
			{
				ResponseCache cachedResponse = it->second;
				Time::Interval elapsedTime = Time::now<Time::Fast>() - cachedResponse.lastFetch;

				if(elapsedTime < Time::Interval(DFInt::ExpireMarketPlaceServiceCacheSeconds))
				{
					resumeFunction(cachedResponse.values);
					return;
				}
	
			}

			apiService->getAsync(infoPathFull, true, RBX::PRIORITY_DEFAULT,
				boost::bind(&MarketplaceService::onReceivedRawProductInfoSuccess, this, infoPathFull, _1, weak_from(RBX::DataModel::get(this)), resumeFunction, errorFunction),
				boost::bind(&MarketplaceService::onReceivedRawProductInfoError, this, _1, weak_from(RBX::DataModel::get(this)), errorFunction) );
		}
	}

	void MarketplaceService::onReceivedRawProductInfoSuccess(std::string url, std::string response, boost::weak_ptr<RBX::DataModel> weakDM, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<RBX::DataModel> sharedDM = weakDM.lock())
		{
			if( response.empty() )
			{
				errorFunction( RBX::format("MarketplaceService:getProductInfo() failed because rawProductInfo was empty") );
				return;
			}

			shared_ptr<const Reflection::ValueTable> values;
			std::stringstream rawProductStream;
			rawProductStream << response;

			if( WebParser::parseJSONTable(response, values) )
			{
				resumeFunction(values);
				ResponseCache responseToCache;
				responseToCache.lastFetch = Time::now<Time::Fast>();
				responseToCache.values = values;
				mMap[url] = responseToCache;
			}
			else
				errorFunction("MarketplaceService::getProductInfo() an error occured while parsing web response");
		}
	}
	void MarketplaceService::onReceivedRawProductInfoError(std::string error, boost::weak_ptr<RBX::DataModel> weakDM, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<RBX::DataModel> sharedDM = weakDM.lock())
		{
			if (!error.empty())
			{
				errorFunction( RBX::format("MarketplaceService:getProductInfo() failed because %s", error.c_str()) );
			}
			else
			{
				errorFunction("MarketplaceService:getProductInfo() failed but with no known error");
			}
		}
	}

	void MarketplaceService::processPlayerOwnsAssetResponseSuccess(std::string response, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (response.empty())
		{
			errorFunction("MarketPlaceService::PlayerOwnsAsset failed because the response was nil");
			return;
		}

		boost::algorithm::to_lower(response);

		bool isOwner = false;
		std::istringstream is(response);
		is >> std::boolalpha >> isOwner;

		resumeFunction(isOwner);
	}
	void MarketplaceService::processPlayerOwnsAssetResponseError(std::string error, boost::function<void(std::string)> errorFunction)
	{
		if (!error.empty())
		{
			errorFunction(RBX::format("MarketPlaceService::PlayerOwnsAsset failed because %s",error.c_str()));
			return;
		}
		else
		{
			errorFunction("MarketPlaceService::PlayerOwnsAsset failed with an unknown error");
		}
	}

	void MarketplaceService::playerOwnsAsset(const shared_ptr<Instance> player, const int assetId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(playerOwnsAssetUrl.empty())
		{
			errorFunction("MarketplaceService:PlayerOwnsAsset() playerOwnsAssetUrl is empty");
			return;
		}

		if(assetId <= 0)
		{
			errorFunction("MarketplaceService:PlayerOwnsAsset() second argument is not a valid assetId (supplied assetId was less than 0)");
			return;
		}

		if(!isValidPlayer(player,"PlayerOwnsAsset()",errorFunction))
		{
			resumeFunction(false);
			return;
		}

		int userId = 0;
		if(Network::Player* thePlayer = Instance::fastDynamicCast<Network::Player>(player.get()))
			userId = thePlayer->getUserID();
        
        std::string url = "";
        if (ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this))
        {
            url = contentProvider->getApiBaseUrl() + playerOwnsAssetUrl;
        }
        
		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			apiService->getAsync( RBX::format(playerOwnsAssetUrl.c_str(), userId, assetId), true, RBX::PRIORITY_DEFAULT,
				boost::bind(&MarketplaceService::processPlayerOwnsAssetResponseSuccess, this, _1, resumeFunction,errorFunction), 
				boost::bind(&MarketplaceService::processPlayerOwnsAssetResponseError, this, _1, errorFunction) );
		}
	}

	void MarketplaceService::verifyPurchaseResponseNoDMLock(std::string response, int userId, int productId)
	{
		shared_ptr<const Reflection::ValueTable> values;
		std::stringstream responseStream;
		responseStream << response;

		if( WebParser::parseJSONTable(response, values) )
			event_serverPurchaseVerification.fireAndReplicateEvent(this, values);
		else
			verifyPurchaseErrorNoDMLock("VerifyResponse Failed", userId, productId);
	}

	void MarketplaceService::verifyPurchaseErrorNoDMLock(std::string error, int userId, int productId)
	{
		int placeId = 0;
		if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
			placeId = dataModel->getPlaceID();

		shared_ptr<Reflection::ValueTable> values(rbx::make_shared<Reflection::ValueTable>());

		(*values)["playerId"] = userId;
		(*values)["placeId"] = placeId;
		(*values)["isValid"] = false;
		(*values)["productId"] = productId;
		(*values)["errorReason"] = error;

		event_serverPurchaseVerification.fireAndReplicateEvent(this, values);
	}

	void MarketplaceService::verifyPurchaseTicket(std::string ticket, int userId, int productId)
	{
		if (Network::Players::backendProcessing(this, false))
		{
			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
			{
				apiService->postAsync( RBX::format("%s/validatepurchase?receipt=%s", urlApiPath(), ticket.c_str()), std::string("GameServerVerifyPurchase"), true, RBX::PRIORITY_EXTREME, HttpService::TEXT_PLAIN,
					boost::bind(&MarketplaceService::verifyPurchaseResponseNoDMLock, this, _1, userId, productId), 
					boost::bind(&MarketplaceService::verifyPurchaseErrorNoDMLock, this, _1, userId, productId) );
			}
		}
	}

	void MarketplaceService::signalClientPurchaseSuccess(std::string ticket, int userId, int productId)
	{
		event_clientPurchaseSuccess.fireAndReplicateEvent(this,ticket,userId,productId);
	}

	bool MarketplaceService::launchClientLuaDialog(std::string message, std::string accept, std::string decline, shared_ptr<Instance> player, boost::function<void(bool, shared_ptr<Instance>)> callback)
	{
		if (Network::Player* thePlayer = Instance::fastDynamicCast<Network::Player>(player.get()))
		{
			Network::Players* players = ServiceProvider::find<Network::Players>(this);

			if (!players ||
				thePlayer->getParent() != players ||
				players->clientIsPresent(this) ||
				callbackFunctionMap.find(player) != callbackFunctionMap.end())
				return false;

			if (!playerRemovingConnection.connected())
				playerRemovingConnection = players->playerRemovingSignal.connect(boost::bind(&MarketplaceService::executeClientDialogCallback, this, false, _1));
			
			callbackFunctionMap[player] = callback;

			const RBX::SystemAddress& target = thePlayer->getRemoteAddressAsRbxAddress();

			shared_ptr<Reflection::Tuple> args(new Reflection::Tuple());
			args->values.push_back(message);
			args->values.push_back(accept);
			args->values.push_back(decline);

			Reflection::EventArguments eventArgs(1);
			eventArgs[0] = shared_ptr<const Reflection::Tuple>(args);

			if (players->serverIsPresent(this))
				raiseEventInvocation(event_ClientLuaDialogRequested, eventArgs, &target);
			else
				event_ClientLuaDialogRequested.fireEvent(this, eventArgs);

			return true;
		}
		return false;
	}

	void MarketplaceService::executeClientDialogCallback(bool value, shared_ptr<Instance> player)
	{
		if (callbackFunctionMap.find(player) == callbackFunctionMap.end())
			return;

		if (!(&callbackFunctionMap[player])->empty())
		{
			boost::function<void(bool, shared_ptr<Instance>)> callbackFunction = callbackFunctionMap[player];
			callbackFunctionMap.erase(player);
			callbackFunction(value, player);
		}
	}

	void MarketplaceService::signalServerLuaDialogClosed(bool value)
	{
		if (!Network::Players::serverIsPresent(this))
		{
			Instance* player = Network::Players::findLocalPlayer(this);

			Reflection::EventArguments args(2);
			args[0] = value;
			args[1] = shared_from(player);

			if (Network::Players::clientIsPresent(this))
				raiseEventInvocation(event_luaDialogCallbackSignal, args, NULL);
			else
				event_luaDialogCallbackSignal.fireEvent(this, args);
		}
	}

	void MarketplaceService::onServiceProvider(ServiceProvider* oldSp, ServiceProvider* newSp)
	{
		playerAddedReceiptConnection.disconnect();
		playerPurchaseReceiptConnection.disconnect();
		verifyPurchaseConnection.disconnect();

		Instance::onServiceProvider(oldSp, newSp);

		if (newSp && Network::Players::backendProcessing(newSp))
		{
			setupBackendOnlyReceiptHandling();

			Network::Players* players = ServiceProvider::create<Network::Players>(this);

			// Attach listener for player joined to query for receipts
			playerAddedReceiptConnection = players->getOrCreateChildAddedSignal()->connect(boost::bind(
				&MarketplaceService::getReceiptsForJoiningPlayer, this, _1));

			// Attach listener for player purchases
			playerPurchaseReceiptConnection = clientPurchaseSuccess.connect(boost::bind(
				&MarketplaceService::getReceiptsAfterPurchase, this, _1, _2, _3));
		}

		if (newSp)
		{
			verifyPurchaseConnection = clientPurchaseSuccess.connect(boost::bind(&RBX::MarketplaceService::verifyPurchaseTicket,
				this, _1, _2, _3));
		}
	}

	void MarketplaceService::processReceiptsCallbackChanged(const ReceiptCallback& oldValue)
	{
		if (!Network::Players::backendProcessing(this))
		{
			// Property changed callbacks are called after the value has been updated,
			// so if we are not backend processing we should revert the value
			receiptCallback.clear();
			throw RBX::runtime_error("Can only register ProcessReceipt callback on server");
		}

		receiptProcessingEnabledByUser = true;
	}

	static void alwaysApproveReceiptsCallback(Reflection::Variant,
		Reflection::AsyncCallbackDescriptor::ResumeFunction resumeFunc,
		Reflection::AsyncCallbackDescriptor::ErrorFunction)
	{
		shared_ptr<Reflection::Tuple> t(new Reflection::Tuple(1));
		t->values[0] = MarketplaceService::DECISION_PURCHASE_GRANTED;
		resumeFunc(t);
	}

	void MarketplaceService::setupBackendOnlyReceiptHandling()
	{
		RBXASSERT(Network::Players::backendProcessing(this));
		RBXASSERT(!receiptProcessingEnabledByUser);
		receiptProcessingEnabledByUser = false;
		receiptCallback = boost::bind(&alwaysApproveReceiptsCallback, _1, _2, _3);
	}

	void MarketplaceService::getReceiptsForJoiningPlayer(shared_ptr<Instance> inst)
	{
		if (shared_ptr<Network::Player> player = Instance::fastSharedDynamicCast<Network::Player>(inst))
		{
			fetchAndProcessTransactions(player->getUserID());
		}
	}

	void MarketplaceService::getReceiptsAfterPurchase(std::string response, int userId, int productId)
	{
		fetchAndProcessTransactions(userId);
	}

	void MarketplaceService::fetchAndProcessTransactions(int userId)
	{
		if (Network::Players::backendProcessing(this) && 
			ServiceProvider::create<LuaWebService>(this)->isApiAccessEnabled())
		{
			int placeId = DataModel::get(this)->getPlaceID();
			if (placeId > 0 && userId > 0)
			{
				std::string apiBaseUrl = ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();

				Http http(format("%sgametransactions/getpendingtransactions/?PlaceId=%d&PlayerId=%d",
					apiBaseUrl.c_str(), placeId, userId));
				http.additionalHeaders["Cache-Control"] = "no-cache";
				http.doNotUseCachedResponse = true;

				if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
				{
					apiService->getAsync(http, RBX::PRIORITY_EXTREME,
						boost::bind(&MarketplaceService::handleReceiptQueryResponseSuccess, weak_from(this), _1),
						boost::bind(&MarketplaceService::handleReceiptQueryResponseFailure, weak_from(this), _1)
						);

				}
			}
		}
	}

	static shared_ptr<const Reflection::ValueTable> buildReceiptInfoFromJson(const Reflection::Variant& jsonReceiptAsVariant)
	{
		shared_ptr<const Reflection::ValueTable> sourceTable =
			jsonReceiptAsVariant.cast<shared_ptr<const Reflection::ValueTable> >();
		shared_ptr<Reflection::ValueTable> receiptTable(new Reflection::ValueTable);
		(*receiptTable)["PlayerId"] = sourceTable->at("playerId");
		(*receiptTable)["PlaceIdWherePurchased"] = sourceTable->at("placeId");
		(*receiptTable)["PurchaseId"]= sourceTable->at("receipt");

		if (sourceTable->at("actionArgs").isType<shared_ptr<const Reflection::ValueArray> >())
		{
			shared_ptr<const Reflection::ValueArray> argsArray =
				sourceTable->at("actionArgs").cast<shared_ptr<const Reflection::ValueArray> >();
			for (size_t j = 0; j < argsArray->size(); ++j)
			{
				if (argsArray->at(j).isType<shared_ptr<const Reflection::ValueTable> >())
				{
					shared_ptr<const Reflection::ValueTable> kvPair =
						argsArray->at(j).cast<shared_ptr<const Reflection::ValueTable> >();

					int intValue;
					bool isNumber = StringConverter<int>::convertToValue(kvPair->at("Value").get<std::string>(), intValue);
					if (kvPair->at("Key").get<std::string>() == "productId" && isNumber)
					{
						(*receiptTable)["ProductId"] = intValue;
					}
					if (kvPair->at("Key").get<std::string>() == "currencyTypeId" && isNumber)
					{
						(*receiptTable)["CurrencyType"] = static_cast<MarketplaceService::CurrencyType>(intValue);
					}
					if (kvPair->at("Key").get<std::string>() == "unitPrice" && isNumber)
					{
						(*receiptTable)["CurrencySpent"] = intValue;
					}
				}
			}
		}

		return shared_ptr<const Reflection::ValueTable>(receiptTable);
	}

	void MarketplaceService::handleReceiptQueryResponseSuccess(weak_ptr<MarketplaceService> weakMk, std::string response)
	{
		if (!response.empty())
		{
			if (shared_ptr<MarketplaceService> m = weakMk.lock())
			{
				Reflection::Variant jsonVariant;
				WebParser::parseJSONObject(response, jsonVariant);
				if (jsonVariant.isType<shared_ptr<const Reflection::ValueArray> >())
				{
					shared_ptr<const Reflection::ValueArray> receiptArray =
						jsonVariant.cast<shared_ptr<const Reflection::ValueArray> >();
					for (size_t i = 0; i < receiptArray->size(); ++i)
					{
						if (receiptArray->at(i).isType<shared_ptr<const Reflection::ValueTable> >())
						{	
							DataModel::get(m.get())->submitTask(
								boost::bind(&MarketplaceService::processReceipt,
								weakMk, buildReceiptInfoFromJson(receiptArray->at(i))),
								DataModelJob::Write);
						}
					}
				}
			}
		}
	}

	void MarketplaceService::handleReceiptQueryResponseFailure(weak_ptr<MarketplaceService> weakMk, std::string error)
	{
		// nothing to do here, maybe in the future we will need to handle/report error
	}

	static void doNothingHttpCallbackSuccess(std::string response)
	{

	}
	static void doNothingHttpCallbackError(std::string error)
	{

	}

	static void doNothingErrorHandler(std::string error)
	{
	}

	void MarketplaceService::processReceipt(weak_ptr<MarketplaceService> weakMk, shared_ptr<const Reflection::ValueTable> receiptInfo)
	{
		if (shared_ptr<MarketplaceService> mk = weakMk.lock())
		{
			if (!mk->receiptCallback.empty())
			{
				try
				{
					// try to get metadata for logging
					int currencyType = -1;
					int currencySpent = -1;
					try
					{
						currencyType = receiptInfo->at("CurrencyType").get<CurrencyType>();
						currencySpent = receiptInfo->at("CurrencySpent").get<int>();
					}
					catch (const RBX::base_exception&)
					{
						RBXASSERT(false);
					}

					std::string receiptId = receiptInfo->at("PurchaseId").get<std::string>();
					mk->receiptCallback(receiptInfo,
						boost::bind(&MarketplaceService::processReceiptSuccessContinuation, mk,
							receiptId, currencyType, currencySpent, _1),
						boost::bind(&doNothingErrorHandler, _1));
				}
				catch (const RBX::base_exception&)
				{
				}
			}
		}
	}

	void MarketplaceService::processReceiptSuccessContinuation(shared_ptr<MarketplaceService> mk,
		std::string receiptId, int currencyType, int currencySpent, shared_ptr<const Reflection::Tuple> result)
	{
		ProductPurchaseDecision decision = DECISION_NOT_PROCESSED_YET;
		try {
			if (result->values.size() == 1)
			{
				decision = result->at(0).get<MarketplaceService::ProductPurchaseDecision>();
			}
		}
		catch (const RBX::base_exception&)
		{
		}
		if (decision == DECISION_PURCHASE_GRANTED)
		{
			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(mk.get()))
			{
				apiService->postAsync("gametransactions/settransactionstatuscomplete", format("receipt=%s", receiptId.c_str()), true, RBX::PRIORITY_EXTREME, HttpService::APPLICATION_URLENCODED,
					boost::bind(&doNothingHttpCallbackSuccess, _1), 
					boost::bind(&doNothingHttpCallbackError, _1) );
			}

			const char* currencyName = "unknown";
			if (currencyType == CURRENCY_ROBUX)
			{
				currencyName = "Robux";
			}
			else if (currencyType == CURRENCY_TIX)
			{
				currencyName = "Ticket";
			}

			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, 
				mk->receiptProcessingEnabledByUser ? "BuyDeveloperProduct" : "BuyDeveloperProduct_AutoAccepted",
				currencyName, std::max(0, currencySpent));
		}
	}

	void MarketplaceService::promptThirdPartyPurchase(const shared_ptr<Instance> player, std::string thirdPartyProductId)
	{
		if(!isValidPlayer(player,"PromptThirdPartyPurchase()"))
			return;

		if(Network::Players* players = ServiceProvider::find<RBX::Network::Players>(this))
        {
			if(Network::Players::frontendProcessing(this) &&  (player.get() != players->getLocalPlayer()) )
			{
				RBX::StandardOut::singleton()->printf( RBX::MESSAGE_WARNING, "MarketplaceService:PromptThirdPartyPurchase called from a local script, but not called on a local player. Local scripts can only prompt the local player.");
				return;
			}
        }
        
        if(thirdPartyProductId.empty())
            throw RBX::runtime_error("MarketplaceService:PromptThirdPartyPurchase() productId is empty");

        // fire a signal so the native platform can do the appropriate purchase calls
        event_promptThirdPartyPurchaseRequested.fireAndReplicateEvent(this, player, thirdPartyProductId);
	}

	void MarketplaceService::signalPromptThirdPartyPurchaseFinished(const shared_ptr<Instance> player, std::string productId, std::string receipt, const bool success)
	{
		event_PromptTPPurchaseFinished.fireAndReplicateEvent(this,player,productId,receipt,success);
	}
    
    void MarketplaceService::promptNativePurchase(const shared_ptr<Instance> player, std::string nativeProductId)
    {
		if(!isValidPlayer(player,"PromptNativePurchase()"))
			return;
        
		if(Network::Players* players = ServiceProvider::find<RBX::Network::Players>(this))
        {
			if(Network::Players::frontendProcessing(this) &&  (player.get() != players->getLocalPlayer()) )
			{
				RBX::StandardOut::singleton()->printf( RBX::MESSAGE_WARNING, "MarketplaceService:PromptNativePurchase called from a local script, but not called on a local player. Local scripts can only prompt the local player.");
				return;
			}
        }
        
        if(nativeProductId.empty())
            throw RBX::runtime_error("MarketplaceService:PromptNativePurchase() productId is empty");
        
        // fire a signal so the native platform can do the appropriate purchase calls
        event_promptNativePurchaseRequested.fireAndReplicateEvent(this, player, nativeProductId);
    }
    void MarketplaceService::signalPromptNativePurchaseFinished(const shared_ptr<Instance> player, std::string productId, const bool success)
    {
        event_PromptNativePurchaseFinished.fireAndReplicateEvent(this, player, productId, success);
    }
    
    
    void MarketplaceService::getDeveloperProductsAsync(boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
        std::string fetchUrl;
        ContentProvider* cp = DataModel::get(this)->find<ContentProvider>();
        if (cp)
            fetchUrl = format("%sdeveloperproducts/list?placeid=%i", cp->getApiBaseUrl().c_str(), DataModel::get(this)->getPlaceID());
        
		shared_ptr<Pages> pagination = Creatable<Instance>::create<StandardPages>(weak_from(DataModel::get(this)), fetchUrl, "DeveloperProducts");
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
    }

} // namespace RBX
