#pragma once

#include "V8Tree/Service.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {

	extern const char *const sBadgeService;

	class BadgeService 
		: public DescribedCreatable<BadgeService, Instance, sBadgeService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
	private:
		std::string awardBadgeUrl;
		std::string hasBadgeUrl;
		std::string isBadgeDisabledUrl;
		std::string isBadgeLegalUrl;

		int placeId;
		int cooldownTime;

		boost::recursive_mutex badgeAwardSync;
		std::map<int, std::set<int> > badgeAwardCache;

		boost::recursive_mutex badgeQuerySync;
		std::map<int, std::set<int> > badgeQueryCache;
		
		boost::recursive_mutex badgeIsDisabledSync;
		std::map<int, bool> badgeIsDisabledCache;
		
		boost::recursive_mutex badgeIsLegalSync;
		std::map<int, bool> badgeIsLegalCache;

		struct HotUserHasBadge
		{
			int userId;
			int badgeId;
			RBX::Time expiration;
			HotUserHasBadge(int userId, int badgeId, int cooldownTime);
			bool expired() const;
		};
		std::list<HotUserHasBadge> hotBadges;
		bool isHasBadgeHot(int userId, int badgeId);
	public:
		BadgeService();
		~BadgeService()
		{}

		rbx::remote_signal<void(std::string, int, int)> badgeAwardedSignal; 	

		void userHasBadge(int userId, int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void awardBadge(int userId, int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isDisabled(int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isLegal(int badgeId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		void setPlaceId(int placeId);
		void setHasBadgeCooldown(int seconds);

		void setAwardBadgeUrl(std::string);
		void setHasBadgeUrl(std::string);
		void setIsBadgeDisabledUrl(std::string);
		void setIsBadgeLegalUrl(std::string);
	private:
		static void hasBadgeResultHelper(weak_ptr<BadgeService>, int userId, int badgeId, std::string* response, std::exception* err,
										 boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void hasBadgeResult(int userId, int badgeId, std::string* response, std::exception* err,
							boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		static void awardBadgeResultHelper(weak_ptr<BadgeService>, int userId, int badgeId, std::string* response, std::exception* err,
							  			   boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void awardBadgeResult(int userId, int badgeId, std::string* response, std::exception* err,
							  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		static void isDiabledResultHelper(weak_ptr<BadgeService>, int badgeId, std::string* response, std::exception* err,
										  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isDisabledResult(int badgeId, std::string* response, std::exception* err,
							  boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		static void isLegalResultHelper(weak_ptr<BadgeService>, int badgeId, std::string* response, std::exception* err,
										boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isLegalResult(int badgeId, std::string* response, std::exception* err,
						   boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

	};
}