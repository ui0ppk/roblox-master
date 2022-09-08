#pragma once

#include "V8Tree/Service.h"

namespace RBX {

	namespace Network {
		class Player;
	}
	extern const char *const sPersonalServerService;

	class PersonalServerService 
		: public DescribedNonCreatable<PersonalServerService, Instance, sPersonalServerService>
		, public Service
	{
	public:
		enum PrivilegeType
		{	
			PRIVILEGE_OWNER = 255,
			PRIVILEGE_ADMIN = 240,
			PRIVILEGE_MEMBER = 128,
			PRIVILEGE_VISITOR = 10,
			PRIVILEGE_BANNED = 0,
		};

		PersonalServerService();

		void setPersonalServerGetRankUrl(std::string url);
		void setPersonalServerSetRankUrl(std::string url);
		void setPersonalServerRoleSetsUrl(std::string url);

		void getWebRoleSets(int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getRank(Network::Player* player, int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void setRank(Network::Player* player, int PlaceId, int newRank, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		int nextRankUp(int currentRank);
		int nextRankDown(int currentRank);

		void promote(shared_ptr<Instance> instance);
		void demote(shared_ptr<Instance> instance);

		void moveToRank(Network::Player* player, int rank);

		int getCurrentPrivilege(int rank);

		std::string getRoleSets() const { return roleSets; }
		void setRoleSets(std::string value) { roleSets = value; }

	private:
		template<typename ResultType>
		void dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction);

		std::string personalServerGetRankUrl;
		std::string personalServerSetRankUrl;
		std::string personalServerRoleSetsUrl;

		std::string roleSets;
		
		void setPrivilege(Network::Player* player, PrivilegeType privilegeType);
	};
}
