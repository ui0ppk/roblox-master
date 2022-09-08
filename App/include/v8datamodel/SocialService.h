#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX {

	extern const char* const sSocialService;
	class SocialService
		: public DescribedNonCreatable<SocialService, Instance, sSocialService>
		, public Service

	{
	public:
		enum StuffType
		{
			HEADS_STUFF		= 0,
			FACES_STUFF		= 1,
			HATS_STUFF		= 2,
			T_SHIRTS_STUFF	= 3,
			SHIRTS_STUFF	= 4,
			PANTS_STUFF		= 5,
			GEARS_STUFF 	= 6,
			TORSOS_STUFF	= 7,
			LEFT_ARMS_STUFF	= 8,
			RIGHT_ARMS_STUFF= 9,
			LEFT_LEGS_STUFF	= 10,
			RIGHT_LEGS_STUFF= 11,
			BODIES_STUFF	= 12,
			COSTUMES_STUFF	= 13,
		};

		SocialService();

		void setFriendUrl(std::string);
		void setBestFriendUrl(std::string);
		void setGroupUrl(std::string);
		void setGroupRankUrl(std::string);
		void setGroupRoleUrl(std::string);
		void setStuffUrl(std::string);
		void setPackageContentsUrl(std::string);

		void isFriendsWith(int playerId, int userId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isBestFriendsWith(int playerId, int userId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isInGroup(int playerId, int groupId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getRankInGroup(int playerId, int groupId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getRoleInGroup(int playerId, int groupId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

		void getListOfStuff(int playerId, StuffType category, int page, boost::function<void(shared_ptr<const Reflection::ValueMap>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getPackageContents(int assetId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
	private:
		template<typename ResultType>
		void dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction);
		std::string friendUrl;
        std::string bestFriendUrl;
        std::string groupUrl;
		std::string groupRankUrl;
		std::string groupRoleUrl;
		std::string stuffUrl;
		std::string packageContentsUrl;
	};

}