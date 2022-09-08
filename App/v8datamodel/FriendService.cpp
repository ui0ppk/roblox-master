#include "stdafx.h"

#include "V8DataModel/FriendService.h"
#include "Network/Players.h"
#include "V8Xml/WebParser.h"
#include "Util/Http.h"
#include "rbx/Log.h"
#include "Util/LuaWebService.h"
#include "v8datamodel/HttpRbxApiService.h"

namespace RBX
{

const char* const sFriendService = "FriendService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetCreateFriendRequestUrl(&FriendService::setCreateFriendRequestUrl, "SetCreateFriendRequestUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetDeleteFriendRequestUrl(&FriendService::setDeleteFriendRequestUrl, "SetDeleteFriendRequestUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetMakeFriendUrl(&FriendService::setMakeFriendUrl, "SetMakeFriendUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetBreakFriendUrl(&FriendService::setBreakFriendUrl, "SetBreakFriendUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetGetFriendsUrl(&FriendService::setGetFriendsUrl, "SetGetFriendsUrl", "url", Security::LocalUser);

static Reflection::BoundFuncDesc<FriendService, void(bool)>func_SetEnabled(&FriendService::setEnable, "SetEnabled", "enable", Security::LocalUser);

static Reflection::RemoteEventDesc<FriendService, void(int,int,FriendService::FriendEventType)> desc_friendEventReplicating(&FriendService::friendEventReplicatingSignal, "RemoteFriendEventSignal", "userId", "userId", "event", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FriendService, void(int,int,FriendService::FriendStatus)> desc_friendStatusReplicating(&FriendService::friendStatusReplicatingSignal,  "RemoteFriendStatusSignal", "userId", "userId", "status", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::BoundFuncDesc<FriendService, void(std::string)> func_SetFriendsOnlineUrl(&FriendService::setFriendsOnlineUrl, "SetFriendsOnlineUrl", "url", Security::LocalUser);
REFLECTION_END();

FASTSTRINGVARIABLE(FriendsOnlineUrl, "/my/friendsonline")

FriendService::FriendService()
	:enable(false)
{
	setName(sFriendService);
	//setMakeFriendUrl("http://siteapp3.roblox.com/Friend/CreateFriend?firstUserId=%d&secondUserId=%d");
	//setGetFriendsUrl("http://siteapp3.roblox.com/Friend/AreFriends?userId=%d");
	//setBreakFriendUrl("http://siteapp3.roblox.com/Friend/BreakFriend?firstUserId=%d&secondUserId=%d");
}

void FriendService::setEnable(bool value)
{
	enable = value;
}
static int countNumberParams(const std::string& value)
{
	int count = 0;
	std::string::size_type pos = value.find("%d");
	while(pos != std::string::npos){
		count++;
		pos = value.find("%d", pos + 2);
	}
	return count;
}

void FriendService::processServiceSuccess(int maxFriends, std::string result, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,			
									  boost::function<void(std::string)> errorFunction)
{
	if(result.empty()) 
	{
		errorFunction("Invalid response received"); 
		return;
	}

	shared_ptr<const Reflection::ValueArray> inputTable;
	shared_ptr<Reflection::ValueArray> lTable(rbx::make_shared<Reflection::ValueArray>());
	if(WebParser::parseJSONArray(result, inputTable))
	{
		*lTable = *inputTable;		
			
		if ((int)(lTable->size()) > maxFriends) 
		{
			lTable->erase(lTable->begin() + maxFriends, lTable->end()); 
		}
		resumeFunction(lTable);
	}
	else 
	{
		errorFunction("FriendService error occurred");		
	}	
}

void FriendService::processServiceError(std::string error, boost::function<void(std::string)> errorFunction)
{
	errorFunction( RBX::format("FriendService: Request Failed because %s", error.c_str()) );
}

void FriendService::getFriendsOnline(int maxFriends, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction) 
{	
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		apiService->getAsync(FString::FriendsOnlineUrl, true, RBX::PRIORITY_DEFAULT,
			boost::bind(&FriendService::processServiceSuccess, this, maxFriends, _1, resumeFunction, errorFunction),
			boost::bind(&FriendService::processServiceError, this, _1, errorFunction) );
	}
}

void FriendService::setFriendsOnlineUrl(std::string url)
{
	friendsOnlineUrl = url;
}

void FriendService::setCreateFriendRequestUrl(std::string value)
{
	if(countNumberParams(value) != 2)
		throw std::runtime_error("CreateFriendRequestUrl requires 2 %%d parameters");

	createFriendRequestUrl = value;
}
void FriendService::setDeleteFriendRequestUrl(std::string value)
{
	if(countNumberParams(value) != 2)
		throw std::runtime_error("DeleteFriendRequestUrl requires 2 %%d parameters");

	deleteFriendRequestUrl = value;
}
void FriendService::setMakeFriendUrl(std::string value)
{
	if(countNumberParams(value) != 2)
		throw std::runtime_error("MakeFriendUrl requires 2 %%d parameters");

	makeFriendUrl = value;
}
void FriendService::setBreakFriendUrl(std::string value)
{
	if(countNumberParams(value) != 2)
		throw std::runtime_error("BreakFriendUrl requires 2 %%d parameters");

	breakFriendUrl = value;
}	

void FriendService::setGetFriendsUrl(std::string value)
{
	if(countNumberParams(value) != 1)
		throw std::runtime_error("GetFriendsUrl requires 1 %d parameters");

	getBulkFriendsUrl = value;
}

void FriendService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(friendStatusConnection.connected()){
		friendStatusConnection.disconnect();
	}
	if(friendEventConnection.connected()){
		friendEventConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
	if(newProvider){
		friendStatusConnection = friendStatusReplicatingSignal.connect(boost::bind(&FriendService::friendStatusReplicationChanged, this, _1, _2, _3));
		friendEventConnection = friendEventReplicatingSignal.connect(boost::bind(&FriendService::friendEventReplicationChanged, this, _1, _2, _3));
	}
}


static void DontCareResponse(std::string*, std::exception*)
{

}

void FriendService::issueFriendRequestOrMakeFriendship(int userId, int otherUserId)
{
	const bool isGuest = (userId < 0) || (otherUserId < 0);
	if(userId == otherUserId){
		//Can't friend yourself
		return;
	}

	//Already Friends
	if(getFriendStatus(userId, otherUserId) == FRIEND_STATUS_FRIEND)
		return;

	if(!createFriendRequestUrl.empty() && !isGuest){
		//Fire and forget on the website
		Http(RBX::format(createFriendRequestUrl.c_str(), userId, otherUserId))
			.post("", Http::kContentTypeDefaultUnspecified, false, &DontCareResponse);
	}	

	if(friendRequestSet.find(std::pair<int,int>(otherUserId, userId)) != friendRequestSet.end()){
		//We are the second half of the request, so this is an "accept" event
		desc_friendEventReplicating.fireAndReplicateEvent(this, userId, otherUserId, ACCEPT_REQUEST);

		friendRequestSet.erase(std::pair<int,int>(otherUserId, userId));
		friendRequestSet.erase(std::pair<int,int>(userId, otherUserId));

		storeAndReplicateFriendStatus(userId, otherUserId, FRIEND_STATUS_FRIEND);

		if(!makeFriendUrl.empty() && !isGuest){
			Http(RBX::format(makeFriendUrl.c_str(), userId, otherUserId))
				.post("", Http::kContentTypeDefaultUnspecified, false, &DontCareResponse);
		}
	}
	else{
		if(friendRequestSet.find(std::pair<int,int>(userId, otherUserId)) == friendRequestSet.end()){
			friendRequestSet.insert(std::pair<int,int>(userId, otherUserId));
	
			//FireEvent that a request was issued
			desc_friendEventReplicating.fireAndReplicateEvent(this, userId, otherUserId, ISSUE_REQUEST);
		}

		storeAndReplicateFriendStatus(userId, otherUserId, FRIEND_STATUS_FRIEND_REQUEST_SENT);
	}
}

void FriendService::rejectFriendRequestOrBreakFriendship(int userId, int otherUserId)
{
	const bool isGuest = (userId < 0) || (otherUserId < 0);

	if(userId == otherUserId)
		//Can't unfriend yourself
		return;

	//Already Not Friends
	if(getFriendStatus(userId, otherUserId) == FRIEND_STATUS_NOT_FRIEND)
		return;
	
	if(!breakFriendUrl.empty()  && !isGuest){
		//Fire and forget on the website
		Http(RBX::format(breakFriendUrl.c_str(), userId, otherUserId))
			.post("", Http::kContentTypeDefaultUnspecified, false, &DontCareResponse);
	}

	if(!deleteFriendRequestUrl.empty() && !isGuest){
		//Fire and forget on the website
		Http(RBX::format(deleteFriendRequestUrl.c_str(), userId, otherUserId))
			.post("", Http::kContentTypeDefaultUnspecified, false, &DontCareResponse);
	}

	if(friendRequestSet.find(std::pair<int,int>(userId, otherUserId)) != friendRequestSet.end()){
		friendRequestSet.erase(std::pair<int,int>(userId, otherUserId));

		//Send a notification that we removed this request
		desc_friendEventReplicating.fireAndReplicateEvent(this, userId, otherUserId, REVOKE_REQUEST);
	}	

	if(friendRequestSet.find(std::pair<int,int>(otherUserId, userId)) != friendRequestSet.end()){
		friendRequestSet.erase(std::pair<int,int>(otherUserId, userId));
		
		//Send a notification that we rejected this request
		desc_friendEventReplicating.fireAndReplicateEvent(this, userId, otherUserId, DENY_REQUEST);
	}

	storeAndReplicateFriendStatus(userId, otherUserId, FRIEND_STATUS_NOT_FRIEND);
}

void FriendService::ProcessBulkFriendResponse(weak_ptr<FriendService> weakFriendService, int userId, std::set<int> userIdsRequested, std::string* response, std::exception* error)
{
	if(shared_ptr<FriendService> friendService = weakFriendService.lock()){
		shared_ptr<FriendStatusInternalMap> map(new FriendStatusInternalMap());
		
		for(std::set<int>::const_iterator iter = userIdsRequested.begin(), end=userIdsRequested.end(); iter != end; ++iter)
		{
			(*map)[*iter] = FRIEND_STATUS_NOT_FRIEND;
		}
		if(response && response->size() > 1){
			try
			{
				std::auto_ptr<std::istream> stream(new std::istringstream(response->substr(1)));
				int otherUserId;
				char comma;
				(*stream) >> otherUserId >> comma;
				while(*stream)
				{
					(*map)[otherUserId] = FRIEND_STATUS_FRIEND;
					(*stream) >> otherUserId >> comma;
				}
			}
			catch(RBX::base_exception& e)
			{
				RBX::Log::current()->writeEntry(Log::Error, e.what());
			}
		}
		else{
			for(std::set<int>::const_iterator iter = userIdsRequested.begin(), end=userIdsRequested.end(); iter != end; ++iter)
			{
				(*map)[*iter] = FRIEND_STATUS_NOT_FRIEND;
			}
		}
		DataModel::get(friendService.get())->submitTask(boost::bind(&FriendService::StoreFriendsHelper, weakFriendService, userId, map), DataModelJob::Write);
	}
}

void FriendService::StoreFriendsHelper(weak_ptr<FriendService> weakFriendService, int userId, shared_ptr<FriendStatusInternalMap> friends)
{
	if(!friends) 
		return;

	if(shared_ptr<FriendService> friendService = weakFriendService.lock()){
		for(FriendStatusInternalMap::iterator iter = friends->begin(), end=friends->end(); iter != end; ++iter){
			friendService->storeAndReplicateFriendStatus(userId, iter->first, iter->second);
		}
	}
}

void FriendService::playerAdded(int userId)
{
	if (Network::Players::backendProcessing(this, false)){
		if(players.size() > 0){
			if(!getBulkFriendsUrl.empty() && userId >= 0){
				
				bool nonGuests = false;
				std::ostringstream ostr;
				ostr << RBX::format(getBulkFriendsUrl.c_str(), userId);
				for(std::set<int>::const_iterator iter = players.begin(), end=players.end(); iter != end; ++iter)
				{
					if((*iter) >= 0){
						ostr << "&otherUserIds=" << (*iter) ;
						nonGuests = true;
					}
				}

				if(nonGuests){
					Http(ostr.str()).get(boost::bind(&ProcessBulkFriendResponse, weak_from(this), userId, players, _1, _2));
				}
				else{
					//All Guests, so they can't be friends
					ProcessBulkFriendResponse(weak_from(this), userId, players, NULL, NULL);
				}
			}
			else{
				ProcessBulkFriendResponse(weak_from(this), userId, players, NULL, NULL);
			}
		}
	}
	if(players.count(userId) == 0)
		players.insert(userId);
}

void FriendService::playerRemoving(int userId)
{
	if (Network::Players::backendProcessing(this, false)){
		//Clear out the data associated with this player
		for(FriendRequestSet::iterator iter = friendRequestSet.begin(), end = friendRequestSet.end(); iter != end; )
		{
			if(iter->first == userId || iter->second == userId){
				friendRequestSet.erase(iter++);
			}
			else{
				++iter;
			}
		}
	}

	for(FriendStatusMap::iterator iter = friendStatusTable.begin(), end = friendStatusTable.end(); iter != end; )
	{
		if(iter->first == userId){
			friendStatusTable.erase(iter++);
		}
		else{
			if(iter->first < userId){
				std::map<int, FriendStatus>::iterator iter2 = iter->second.find(userId);
				if(iter2 != iter->second.end()){
					iter->second.erase(iter2);
				}
			}
			++iter;
		}
	}
	players.erase(userId);
}

static FriendService::FriendStatus InvertFriendStatus(FriendService::FriendStatus status)
{
	switch(status)
	{
	case FriendService::FRIEND_STATUS_FRIEND_REQUEST_SENT:
		return FriendService::FRIEND_STATUS_FRIEND_REQUEST_RECEIVED;
	case FriendService::FRIEND_STATUS_FRIEND_REQUEST_RECEIVED:
		return FriendService::FRIEND_STATUS_FRIEND_REQUEST_SENT;
	default:
		return status;
	}
}

FriendService::FriendStatus FriendService::getFriendStatus(int userId, int otherUserId) const
{
	if(userId > otherUserId){
		return InvertFriendStatus(getFriendStatus(otherUserId, userId));
	}

	if(userId == otherUserId){
		return FRIEND_STATUS_FRIEND;
	}

	FriendStatusMap::const_iterator iter = friendStatusTable.find(userId);
	if(iter == friendStatusTable.end()){
		return FRIEND_STATUS_UNKNOWN;
	}
	FriendStatusInternalMap::const_iterator iter2 = iter->second.find(otherUserId);
	if(iter2 == iter->second.end()){
		return FRIEND_STATUS_UNKNOWN;
	}
	return iter2->second;
}


void FriendService::friendEventReplicationChanged(int userId, int otherUserId, FriendEventType friendEvent)
{
	if(Network::Players* players = ServiceProvider::find<Network::Players>(this)){
		players->friendEventFired(userId, otherUserId, friendEvent);
	}	
}
void FriendService::friendStatusReplicationChanged(int userId, int otherUserId, FriendStatus status)
{
	if(!Network::Players::backendProcessing(this, false)) {
		storeAndReplicateFriendStatus(userId, otherUserId, status);
	}

	if(Network::Players* players = ServiceProvider::find<Network::Players>(this)){
		players->friendStatusChanged(userId, otherUserId, status);
		players->friendStatusChanged(otherUserId, userId, InvertFriendStatus(status));
	}
}
void FriendService::storeAndReplicateFriendStatus(int userId, int otherUserId, FriendStatus friendStatus)
{
	if(userId > otherUserId){
		storeAndReplicateFriendStatus(otherUserId, userId, InvertFriendStatus(friendStatus));
		return;
	}

	friendStatusTable[userId][otherUserId] = friendStatus;
	if (Network::Players::backendProcessing(this, false)) {		
		desc_friendStatusReplicating.fireAndReplicateEvent(this, userId, otherUserId, friendStatus);
	}
}

namespace Reflection {
	template<>
	EnumDesc<FriendService::FriendStatus>::EnumDesc()
	:EnumDescriptor("FriendStatus")
	{
		addPair(FriendService::FRIEND_STATUS_UNKNOWN,				   "Unknown");
		addPair(FriendService::FRIEND_STATUS_NOT_FRIEND,			   "NotFriend");
		addPair(FriendService::FRIEND_STATUS_FRIEND,				   "Friend");
		addPair(FriendService::FRIEND_STATUS_FRIEND_REQUEST_SENT,	   "FriendRequestSent");
		addPair(FriendService::FRIEND_STATUS_FRIEND_REQUEST_RECEIVED, "FriendRequestReceived");
	}

	template<>
	FriendService::FriendStatus& Variant::convert<FriendService::FriendStatus>(void)
	{
		return genericConvert<FriendService::FriendStatus>();
	}
	template<>
	EnumDesc<FriendService::FriendEventType>::EnumDesc()
	:EnumDescriptor("FriendRequestEvent")
	{
		addPair(FriendService::ISSUE_REQUEST,	"Issue");
		addPair(FriendService::REVOKE_REQUEST,	"Revoke");
		addPair(FriendService::ACCEPT_REQUEST,	"Accept");
		addPair(FriendService::DENY_REQUEST,	"Deny");
	}

	template<>
	FriendService::FriendEventType& Variant::convert<FriendService::FriendEventType>(void)
	{
		return genericConvert<FriendService::FriendEventType>();
	}
}
template<>
bool StringConverter<FriendService::FriendStatus>::convertToValue(const std::string& text, FriendService::FriendStatus& value)
{
	return Reflection::EnumDesc<FriendService::FriendStatus>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool StringConverter<FriendService::FriendEventType>::convertToValue(const std::string& text, FriendService::FriendEventType& value)
{
	return Reflection::EnumDesc<FriendService::FriendEventType>::singleton().convertToValue(text.c_str(),value);
}

}

