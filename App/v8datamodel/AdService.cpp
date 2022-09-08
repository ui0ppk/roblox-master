//
//  AdService.cpp
//  App
//
//  Created by Ben Tkacheff on 4/16/14.
//
//
#include "stdafx.h"

#include "V8DataModel/AdService.h"
#include "V8DataModel/UserInputService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "Network/Players.h"
#include "v8xml/WebParser.h"

FASTFLAGVARIABLE(EnableVideoAds, true)

namespace RBX
{
	const char* const sAdService = "AdService";
    
    REFLECTION_BEGIN();
	static Reflection::RemoteEventDesc<AdService, void(int, UserInputService::Platform, bool)> event_sendServerRecordImpression(&AdService::sendServerRecordImpression, "SendServerRecordImpression", "userId", "platform", "wasSuccessful", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

	static Reflection::RemoteEventDesc<AdService, void(bool, int, std::string)> event_sendClientAdVerificationResults(&AdService::sendClientVideoAdVerificationResults, "ClientAdVerificationResults", "canShowAd", "userId", "errorMessage", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
    static Reflection::RemoteEventDesc<AdService, void(int, UserInputService::Platform)> event_serverAdVerification(&AdService::sendServerVideoAdVerification, "ServerAdVerification", "userId", "platform", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
    
    static Reflection::BoundFuncDesc<AdService, void()> func_showVideoAd(&AdService::showVideoAd, "ShowVideoAd", Security::None);
    static Reflection::EventDesc<AdService, void(bool)> event_videoClosed(&AdService::videoAdClosedSignal, "VideoAdClosed", "adShown");
    REFLECTION_END();
   
    AdService::AdService() :
        showingVideoAd(false)
    {
        setName(sAdService);
    }
    
    bool AdService::canUseService()
    {
        return FFlag::EnableVideoAds;
    }
    
    void AdService::showVideoAd()
    {
        if (!canUseService())
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, AdService is currently off.");
            videoAdClosedSignal(false);
            return;
        }
        
        if (showingVideoAd)
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "AdService:ShowVideoAd called while currently showing video.");
            videoAdClosedSignal(false);
            return;
        }
        
        if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
        {
            if (!inputService->getTouchEnabled())
            {
                StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, AdService:ShowVideoAd only works on touch devices currently.");
                videoAdClosedSignal(false);
                return;
            }
        }
        
        if (!Network::Players::frontendProcessing(this))
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "AdService:ShowVideoAd must be called from a local script!");
            videoAdClosedSignal(false);
            return;
        }
        
		if (Network::Player* player =  Network::Players::findLocalPlayer(this))
		{
			if (RBX::ServiceProvider::find<RBX::UserInputService>(this))
			{
				event_serverAdVerification.fireAndReplicateEvent(this, player->getUserID(), UserInputService::getPlatform());
			}
			else
			{
				StandardOut::singleton()->printf(MESSAGE_WARNING, "AdService:ShowVideoAd could not find UserInputService, not showing ad.");
				videoAdClosedSignal(false);
				return;
			}
		}
		else
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "AdService:ShowVideoAd could not find LocalPlayer, not showing ad.");
			videoAdClosedSignal(false);
			return;
		}
    }
    
    void AdService::receivedServerShowAdMessage(const bool success, int userId, const std::string& errorMessage)
    {
        if (!RBX::Network::Players::clientIsPresent(this))
        {
            return;
        }
        
		Network::Player* player =  Network::Players::findLocalPlayer(this);
		if (!player)
		{
			return;
		}

		if (player->getUserID() != userId)
		{
			return;
		}

        if (success)
        {
            showingVideoAd = true;
            playVideoAdSignal();
        }
        else
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "AdService:ShowVideoAd cannot show video ad, failed verification because %s", errorMessage.c_str());
            videoAdClosedSignal(false);
        }
    }

	void AdService::verifyCanPlayVideoAdReceivedResponseNoDMLock(const std::string& response, int userId)
	{
		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
		bool parseResult = WebParser::parseJSONTable(response, jsonResult);

		if( parseResult )
		{
			Reflection::Variant successVar = jsonResult->at("success");

			if (successVar.get<bool>())
			{
				event_sendClientAdVerificationResults.fireAndReplicateEvent(this, true, userId, "");
			}
			else
			{
				if (jsonResult->at("errorMessage").isString())
				{
					verifyCanPlayVideoAdReceivedError(jsonResult->at("errorMessage").get<std::string>(), userId);
				}
				else
				{
					verifyCanPlayVideoAdReceivedError("JSON returned invalid format", userId);
				}
			}
		}
		else
		{
			verifyCanPlayVideoAdReceivedError("Unable to parse JSON response", userId);
		}
	}

	void AdService::verifyCanPlayVideoAdReceivedError(const std::string& error, int userId)
	{
		RBX::DataModel::LegacyLock dmLock( RBX::DataModel::get(this), RBX::DataModelJob::Write);
		event_sendClientAdVerificationResults.fireAndReplicateEvent(this, false, userId, error);
	}

	void AdService::verifyCanPlayVideoAdReceivedErrorNoDMLock(const std::string& error, int userId)
	{
		event_sendClientAdVerificationResults.fireAndReplicateEvent(this, false, userId, error);
	}

	std::string AdService::platformToWebString(const UserInputService::Platform userPlatform)
	{
		switch (userPlatform)
		{
		case UserInputService::PLATFORM_IOS:
			return "ios";
		case UserInputService::PLATFORM_ANDROID:
			return "android";
		default:
			return "unknown";
			break;
		}
	}

	void AdService::checkCanPlayVideoAd(int userId, UserInputService::Platform userPlatform)
	{
		if (!RBX::Network::Players::serverIsPresent(this))
		{
			return;
		}

		if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
		{
            std::string data = RBX::format("userId=%d&placeId=%d&deviceOSType=%s", userId, dataModel->getPlaceID(), platformToWebString(userPlatform).c_str());

			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
			{
				apiService->postAsync(RBX::format("adimpression/validate-request?%s", data.c_str()),
					std::string("ServerCanPlayAd"),
					true, RBX::PRIORITY_DEFAULT, HttpService::TEXT_PLAIN,
					boost::bind(&AdService::verifyCanPlayVideoAdReceivedResponseNoDMLock, this, _1, userId), 
					boost::bind(&AdService::verifyCanPlayVideoAdReceivedErrorNoDMLock, this, _1, userId) );
			}
		}
	}
    
    void AdService::videoAdClosed(bool didPlay)
    {
		showingVideoAd = false;
        videoAdClosedSignal(didPlay);

		if (Network::Player* player =  Network::Players::findLocalPlayer(this))
		{
			if (RBX::ServiceProvider::find<RBX::UserInputService>(this))
			{
				event_sendServerRecordImpression.fireAndReplicateEvent(this, player->getUserID(), UserInputService::getPlatform(), didPlay);
			}
		}
    }
    
    static void ForgetResponse(const std::string& response)
    {
        
    }

	void AdService::sendAdImpression(int userId, UserInputService::Platform platform, bool didPlay)
	{
		if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
		{
			if (Network::Players::serverIsPresent(dataModel))
			{
				std::string apiBaseUrl = ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();
				// fire and forget, we don't care about results on the client, at least for now
                std::string data = RBX::format("userId=%i&placeId=%d&deviceOSType=%s&wasSuccessful=%s", userId, dataModel->getPlaceID(), platformToWebString(platform).c_str(), didPlay ? "true" : "false");

				if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
				{
					apiService->postAsync( RBX::format("adimpression/record-impression?%s", data.c_str()),
						std::string("ServerSendAdImpression"),
						true, RBX::PRIORITY_DEFAULT, HttpService::TEXT_PLAIN,
						boost::bind(&ForgetResponse,_1),
						boost::bind(&ForgetResponse,_1));
				}
			}
		}
	}
	
	void AdService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		Super::onServiceProvider(oldProvider,newProvider);

		if (newProvider && Network::Players::frontendProcessing(newProvider))
		{
			sendClientVideoAdVerificationResults.connect( boost::bind(&AdService::receivedServerShowAdMessage, this, _1, _2, _3) );
		}
	}
}