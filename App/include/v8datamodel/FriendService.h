#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX {

	extern const char* const sFriendService;
	class FriendService
		: public DescribedCreatable<FriendService, Instance, sFriendService, Reflection::ClassDescriptor::INTERNAL>
		, public Service

	{
	private:
		typedef DescribedCreatable<FriendService, Instance, sFriendService, Reflection::ClassDescriptor::INTERNAL> Super;
	public:
		enum FriendEventType
		{
			ISSUE_REQUEST,
			REVOKE_REQUEST,
			ACCEPT_REQUEST,
			DENY_REQUEST
		};

		enum FriendStatus
		{
			FRIEND_STATUS_UNKNOWN					= 0,
			FRIEND_STATUS_NOT_FRIEND				= 1,
			FRIEND_STATUS_FRIEND					= 2,
			FRIEND_STATUS_FRIEND_REQUEST_SENT		= 3,
			FRIEND_STATUS_FRIEND_REQUEST_RECEIVED	= 4,
		};

		FriendService();

		void setCreateFriendRequestUrl(std::string);
		void setDeleteFriendRequestUrl(std::string);
		void setMakeFriendUrl(std::string);
		void setBreakFriendUrl(std::string);	
		void setGetFriendsUrl(std::string);

		FriendStatus getFriendStatus(int playerId, int otherPlayerId) const;

		void setEnable(bool value);
		bool getEnable() const { return enable;}
		void playerAdded(int userId);
		void playerRemoving(int userId);

		void issueFriendRequestOrMakeFriendship(int userId, int otherUserId);
		void rejectFriendRequestOrBreakFriendship(int userId, int otherUserId);

		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		//Should be private
		rbx::remote_signal<void(int, int, FriendEventType)> friendEventReplicatingSignal;
		rbx::remote_signal<void(int, int, FriendStatus)> friendStatusReplicatingSignal;		
		
		
		void getFriendsOnline(int maxFriends, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction); 		
		void setFriendsOnlineUrl(std::string url); 
	private:


		void friendStatusReplicationChanged(int playerId, int otherPlayerId, FriendStatus status);
		rbx::signals::scoped_connection friendStatusConnection;
		
		void friendEventReplicationChanged(int playerId, int otherPlayerId, FriendEventType status);
		rbx::signals::scoped_connection friendEventConnection;

		std::string getBulkFriendsUrl;

		std::string breakFriendUrl;
		std::string makeFriendUrl;
		std::string deleteFriendRequestUrl;
		std::string createFriendRequestUrl;		
		std::string friendsOnlineUrl; 		

		void storeAndReplicateFriendStatus(int userId, int otherUserId, FriendStatus friendStatus);

		//friendStatusTable[smallUserId][bigUserId] = Status
		typedef std::map<int, FriendStatus> FriendStatusInternalMap;
		typedef std::map<int, FriendStatusInternalMap > FriendStatusMap;
		FriendStatusMap friendStatusTable;

		typedef std::set<std::pair<int, int> > FriendRequestSet;
		FriendRequestSet friendRequestSet;

		std::set<int> players;		
		template<typename ResultType> 
		void dispatchRequest(const std::string&, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction); 

		bool enable;

		void processServiceSuccess(int maxFriends, std::string result, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,			
			boost::function<void(std::string)> errorFunction);
		void processServiceError(std::string error, boost::function<void(std::string)> errorFunction);
		static void ProcessBulkFriendResponse(weak_ptr<FriendService> weakFriendService, int userId, std::set<int> userIdsRequested, std::string* response, std::exception* error);
		static void StoreFriendsHelper(weak_ptr<FriendService> weakFriendService, int userId, shared_ptr<FriendStatusInternalMap> friends);

	};

}
