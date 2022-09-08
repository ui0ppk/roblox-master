#include "stdafx.h"

#include "V8DataModel/BadgeService.h"
#include "V8DataModel/DebrisService.h"
#include "V8DataModel/Workspace.h"
#include "Network/Players.h"
#include "Util/Http.h"
#include "util/standardout.h"

namespace RBX {

	const char *const sBadgeService = "BadgeService";

    REFLECTION_BEGIN();
	static Reflection::BoundYieldFuncDesc<BadgeService, bool(int,int)> func_UserHasBadge(&BadgeService::userHasBadge, "UserHasBadge", "userId", "badgeId", Security::None);
	static Reflection::BoundYieldFuncDesc<BadgeService, bool(int,int)> func_AwardBadge(&BadgeService::awardBadge, "AwardBadge", "userId", "badgeId", Security::None);
	static Reflection::BoundYieldFuncDesc<BadgeService, bool(int)> func_IsDisabled(&BadgeService::isDisabled, "IsDisabled", "badgeId", Security::None);
	static Reflection::BoundYieldFuncDesc<BadgeService, bool(int)> func_IsLegal(&BadgeService::isLegal, "IsLegal", "badgeId", Security::None);

	static Reflection::BoundFuncDesc<BadgeService, void(int)> func_SetHasBadgeCooldown(&BadgeService::setHasBadgeCooldown, "SetHasBadgeCooldown", "seconds", Security::LocalUser);
	static Reflection::BoundFuncDesc<BadgeService, void(int)> func_SetPlaceId(&BadgeService::setPlaceId, "SetPlaceId", "placeId", Security::LocalUser);
	static Reflection::BoundFuncDesc<BadgeService, void(std::string)> func_SetAwardBadgeUrl(&BadgeService::setAwardBadgeUrl,	"SetAwardBadgeUrl",		"url", Security::LocalUser);
	static Reflection::BoundFuncDesc<BadgeService, void(std::string)> func_SetHasBadgeUrl(&BadgeService::setHasBadgeUrl,		"SetHasBadgeUrl",		"url", Security::LocalUser);
	static Reflection::BoundFuncDesc<BadgeService, void(std::string)> func_SetDisabledUrl(&BadgeService::setIsBadgeDisabledUrl,	"SetIsBadgeDisabledUrl","url", Security::LocalUser);
	static Reflection::BoundFuncDesc<BadgeService, void(std::string)> func_SetLegalUrl(&BadgeService::setIsBadgeLegalUrl,		"SetIsBadgeLegalUrl",  "url", Security::LocalUser);
	
	static Reflection::RemoteEventDesc<BadgeService, void(std::string, int, int)> event_BadgeAwarded(&BadgeService::badgeAwardedSignal, "BadgeAwarded", "message", "userId", "badgeId", Security::RobloxScript, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
    REFLECTION_END();

	BadgeService::BadgeService()
		: placeId(-1)
		, cooldownTime(60)
	{
		setName("BadgeService");
	}

	void BadgeService::setPlaceId(int placeId)
	{
		this->placeId = placeId;
	}
	void BadgeService::setHasBadgeCooldown(int cooldownTime)
	{
		this->cooldownTime = cooldownTime;
	}

	void BadgeService::setAwardBadgeUrl(std::string value)
	{
		awardBadgeUrl = value;
	}
	void BadgeService::setHasBadgeUrl(std::string value)
	{
		hasBadgeUrl = value;
	}
	void BadgeService::setIsBadgeDisabledUrl(std::string value)
	{
		isBadgeDisabledUrl = value;
	}
	void BadgeService::setIsBadgeLegalUrl(std::string value)
	{
		isBadgeLegalUrl = value;	
	}

	void BadgeService::hasBadgeResultHelper(weak_ptr<BadgeService> badgeService, int userId, int badgeId, std::string* response, std::exception* err,
											boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<BadgeService> safeBadgeService = badgeService.lock()){
			safeBadgeService->hasBadgeResult(userId, badgeId, response, err, resumeFunction, errorFunction);
		}
	}
	void BadgeService::hasBadgeResult(int userId, int badgeId, std::string* response, std::exception* err,
									  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!response) 
		{
			resumeFunction(false);
			return;
		}

		if(*response == "Success"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeQuerySync);
				badgeQueryCache[userId].insert(badgeId);
			}
			resumeFunction(true);
			return;
		}

		{
			boost::recursive_mutex::scoped_lock lock(badgeQuerySync);
			hotBadges.push_back(HotUserHasBadge(userId,badgeId, cooldownTime));
		}
		if(*response == "Failure"){
			resumeFunction(false);
		}
		else{
			resumeFunction(false);
		}
	}

	BadgeService::HotUserHasBadge::HotUserHasBadge(int userId, int badgeId, int cooldownTime)
		:userId(userId)
		,badgeId(badgeId)
		,expiration(RBX::Time::now<Time::Fast>() + Time::Interval(cooldownTime))
	{
	}

	bool BadgeService::HotUserHasBadge::expired() const
	{
		return RBX::Time::now<Time::Fast>() >= expiration;
	}

	
	bool BadgeService::isHasBadgeHot(int userId, int badgeId)
	{
		boost::recursive_mutex::scoped_lock lock(badgeQuerySync);
		std::list< HotUserHasBadge >::iterator end = hotBadges.end();
		for (std::list< HotUserHasBadge >::iterator iter = hotBadges.begin(); iter!=end; ++iter)
		{
			if (iter->expired())	
			{
				hotBadges.erase(iter, end);
				return false;
			}
			if (iter->userId==userId && iter->badgeId == badgeId){
				return true;
			}
		}
		return false;
	}

	void BadgeService::userHasBadge(int userId, int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!Workspace::serverIsPresent(this)){
			//Throw a friendly exception to the users that they can't get badges during user games
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, badges can only be queried by the Roblox game servers");
			resumeFunction(false);
			return;
		}

		if(ServiceProvider::find<Network::Players>(this)->getPlayerByID(userId).get() == NULL){
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, player with userId=%d is not in this level at the moment", userId);
			resumeFunction(false);
			return;
		}

		{
			bool returnResult;
			{
				boost::recursive_mutex::scoped_lock lock(badgeQuerySync);
				returnResult = badgeQueryCache[userId].find(badgeId) != badgeQueryCache[userId].end();
			}

			if(returnResult){
				//They already have the badge, so don't bother asking the webserver
				resumeFunction(true);
				return;
			}

			if(isHasBadgeHot(userId,badgeId)){
				resumeFunction(false);
				return;
			}
		}

		//Make the request and send it out
		Http http(RBX::format(hasBadgeUrl.c_str(), userId, badgeId));
		http.get(boost::bind(&BadgeService::hasBadgeResultHelper, weak_from(this), userId, badgeId, _1, _2, resumeFunction, errorFunction));
	}


	void BadgeService::isDiabledResultHelper(weak_ptr<BadgeService> badgeService, int badgeId, std::string* response, std::exception* err,
											boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<BadgeService> safeBadgeService = badgeService.lock()){
			safeBadgeService->isDisabledResult(badgeId, response, err, resumeFunction, errorFunction);
		}
	}
	void BadgeService::isDisabledResult(int badgeId, std::string* response, std::exception* err,
									    boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!response) 
		{
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsDisabledSync);
				badgeIsDisabledCache[badgeId] = true;
			}
			resumeFunction(true);
			return;
		}

		if(*response == "1"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsDisabledSync);
				badgeIsDisabledCache[badgeId] = true;
			}
			resumeFunction(true);
		}
		else if(*response == "0"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsDisabledSync);
				badgeIsDisabledCache[badgeId] = false;
			}
			resumeFunction(false);
		}
		else{
			//Really bad failure?
			resumeFunction(true);
		}
	}

	void BadgeService::isDisabled(int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!Workspace::serverIsPresent(this)){
			//Throw a friendly exception to the users that they can't get badges during user games
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, badges can only be tested if they are disabled on Roblox game servers");
			resumeFunction(true);
			return;
		}

		if(placeId == -1){
			resumeFunction(true);
			return;
		}

		{
			bool hasResult;
			bool result;
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsDisabledSync);
				//Check if we have already awarded the badge to this user in this game session
				hasResult = badgeIsDisabledCache.find(badgeId) != badgeIsDisabledCache.end();
				result = hasResult ? badgeIsDisabledCache[badgeId] : false;
			}
			if(hasResult){
				//We've already awarded this badge during this game session (or the badge is no good), so you can't get it again.
				resumeFunction(result);
				return;
			}
		}

		//Make the request and send it out
		Http http(RBX::format(isBadgeDisabledUrl.c_str(), badgeId, placeId));
		http.get(boost::bind(&BadgeService::isDiabledResultHelper, weak_from(this), badgeId, _1, _2, resumeFunction, errorFunction));
	}




	
	void BadgeService::isLegalResultHelper(weak_ptr<BadgeService> badgeService, int badgeId, std::string* response, std::exception* err,
										   boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<BadgeService> safeBadgeService = badgeService.lock()){
			safeBadgeService->isLegalResult(badgeId, response, err, resumeFunction, errorFunction);
		}
	}
	void BadgeService::isLegalResult(int badgeId, std::string* response, std::exception* err,
									 boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!response) 
		{
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsLegalSync);
				badgeIsLegalCache[badgeId] = true;
			}
			resumeFunction(true);
			return;
		}

		if(*response == "1"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsLegalSync);
				badgeIsLegalCache[badgeId] = true;
			}
			resumeFunction(true);
		}
		else if(*response == "0"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsLegalSync);
				badgeIsLegalCache[badgeId] = false;
			}
			resumeFunction(false);
		}
		else{
			//Really bad failure?
			resumeFunction(true);
		}
	}

	void BadgeService::isLegal(int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(placeId == -1){
			//We don't know the placeId, so give up and assume its okay
			resumeFunction(true);
			return;
		}

		{
			bool hasResult;
			bool result;
			{
				boost::recursive_mutex::scoped_lock lock(badgeIsLegalSync);
				//Check if we have already awarded the badge to this user in this game session
				hasResult = badgeIsLegalCache.find(badgeId) != badgeIsLegalCache.end();
				result = hasResult ? badgeIsLegalCache[badgeId] : false;
			}
			if(hasResult){
				//We've already awarded this badge during this game session (or the badge is no good), so you can't get it again.
				resumeFunction(result);
				return;
			}
		}


		//Make the request and send it out
		Http http(RBX::format(isBadgeLegalUrl.c_str(), badgeId, placeId));
		http.get(boost::bind(&BadgeService::isLegalResultHelper, weak_from(this), badgeId, _1, _2, resumeFunction, errorFunction));
	}

	void BadgeService::awardBadgeResultHelper(weak_ptr<BadgeService> badgeService, int userId, int badgeId, std::string* response, std::exception* err,
							  				  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(shared_ptr<BadgeService> safeBadgeService = badgeService.lock()){
			safeBadgeService->awardBadgeResult(userId, badgeId, response, err, resumeFunction, errorFunction);
		}
	}

	static void fireBadgeAwarded(shared_ptr<BadgeService> self, const std::string& response, int userId, int badgeId)
	{
		event_BadgeAwarded.fireAndReplicateEvent(self.get(), response, userId, badgeId);
	}

	void BadgeService::awardBadgeResult(int userId, int badgeId, std::string* response, std::exception* err,
										boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!response){
			resumeFunction(false);
			return;
		}
		if(*response == ""){
			//We failed in some way to get data back, maybe we should try again?  Or just give up?
			resumeFunction(false);
		}
		else if(*response == "0"){
			{
				boost::recursive_mutex::scoped_lock lock(badgeAwardSync);
				//It returned failure, they must already have the badge.  Add them to the cache
				badgeAwardCache[userId].insert(badgeId);
			}
			resumeFunction(false);
		}
		else{
			{
				boost::recursive_mutex::scoped_lock lock(badgeAwardSync);
				//Cache that we gave them out this badge, so we won't try again
				badgeAwardCache[userId].insert(badgeId);
			}
			
			RBX::DataModel* dataModel = (RBX::DataModel*)this->getParent();
			if (dataModel)
			{
				dataModel->submitTask(boost::bind(fireBadgeAwarded, shared_from(this), *response, userId, badgeId), DataModelJob::Write);
			}

			resumeFunction(true);
		}
	}

	void BadgeService::awardBadge(int userId, int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if(!Workspace::serverIsPresent(this)){
			//Throw a friendly exception to the users that they can't get badges during user games
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Sorry, badges can only be awarded by Roblox game servers");
			resumeFunction(false);
			return;
		}

		Network::Players* players = ServiceProvider::find<Network::Players>(this);
		RBXASSERT(players);

		if (!players->getPlayerByID(userId))
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "The player with userId=%d is not present at the moment", userId);
			resumeFunction(false);
			return;
		}

		{
			bool alreadyAwarded;
			{
				boost::recursive_mutex::scoped_lock lock(badgeAwardSync);
				//Check if we have already awarded the badge to this user in this game session
				alreadyAwarded = (badgeAwardCache[userId].find(badgeId) != badgeAwardCache[userId].end());
			}
			if(alreadyAwarded){
				StandardOut::singleton()->printf(MESSAGE_WARNING, "We already gave out badgeId=%d to userId=%d", badgeId, userId);
				//We've already awarded this badge during this game session (or the badge is no good), so you can't get it again.
				resumeFunction(false);
				return;
			}
		}

		//Make the request and send it out
		Http http(RBX::format(awardBadgeUrl.c_str(), userId, badgeId, placeId));

		// switch get to post to fix exploit in badge service
		//http.get(boost::bind(&BadgeService::awardBadgeResultHelper, weak_from(this), userId, badgeId, _1, _2, resumeFunction, errorFunction));
		std::string in;
		http.post(in, Http::kContentTypeDefaultUnspecified, false,
			boost::bind(&BadgeService::awardBadgeResultHelper, weak_from(this), userId, badgeId, _1, _2, resumeFunction, errorFunction));
	}
}
