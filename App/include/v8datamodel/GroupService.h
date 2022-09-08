#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"

namespace RBX {

	extern const char* const sGroupService;
	class GroupService
		: public DescribedCreatable<GroupService, Instance, sGroupService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
		private:
			static void onReceivedRawGroupInfoSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction);
			static void onReceivedRawGroupInfoError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction);
			static void onReceivedRawGetGroupsSuccess(weak_ptr<DataModel> weakDataModel, std::string response,  boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
			static void onReceivedRawGetGroupsError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction);

		public:
			GroupService();

			void getGroupInfoAsync(const int groupId, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction);
			void getAlliesAsync(const int groupId, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);
			void getEnemiesAsync(const int groupId, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);
			void getGroupsAsync(const int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction); 
	};
}
