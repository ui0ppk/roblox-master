#include "stdafx.h"

#include "V8DataModel/GroupService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "v8xml/WebParser.h"
#include "Network/Players.h"
#include "util/Http.h"
#include "util/LuaWebService.h"

FASTSTRINGVARIABLE(GroupInfoUrl, "%sgroups/%i")
FASTSTRINGVARIABLE(GroupAlliesUrl, "%sgroups/%i/allies")
FASTSTRINGVARIABLE(GroupEnemiesUrl, "%sgroups/%i/enemies")
FASTSTRINGVARIABLE(GetGroupsUrl, "%susers/%i/groups")

DYNAMIC_FASTFLAGVARIABLE(GetGroupsAsyncEnabled, false)

namespace RBX
{
	const char* const sGroupService = "GroupService";

    REFLECTION_BEGIN();
	static Reflection::BoundYieldFuncDesc<GroupService, Reflection::Variant(int)> func_getGroupInfo(&GroupService::getGroupInfoAsync, "GetGroupInfoAsync", "groupId", Security::None);
	static Reflection::BoundYieldFuncDesc<GroupService, shared_ptr<Instance>(int)> func_getAlliesAsync(&GroupService::getAlliesAsync, "GetAlliesAsync", "groupId", Security::None);
	static Reflection::BoundYieldFuncDesc<GroupService, shared_ptr<Instance>(int)> func_getEnemiesAsync(&GroupService::getEnemiesAsync, "GetEnemiesAsync", "groupId", Security::None);
	static Reflection::BoundYieldFuncDesc<GroupService, shared_ptr<const Reflection::ValueArray>(int)> func_getGroupsAsync(&GroupService::getGroupsAsync, "GetGroupsAsync", "userId", Security::None);
    REFLECTION_END();

	GroupService::GroupService()
	{
		setName(sGroupService);
	}

	void GroupService::getGroupInfoAsync(const int groupId, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (groupId <= 0)
		{
			errorFunction("GroupService:GetGroupInfoAsync() argument is not a valid groupId");
			return;
		}
		
		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			apiService->getAsync(format(FString::GroupInfoUrl.c_str(), "", groupId), true, RBX::PRIORITY_DEFAULT,
				boost::bind(&GroupService::onReceivedRawGroupInfoSuccess, weak_from(DataModel::get(this)), _1, resumeFunction, errorFunction), 
				boost::bind(&GroupService::onReceivedRawGroupInfoError, weak_from(DataModel::get(this)), _1, errorFunction) );
		}
	}

	void GroupService::onReceivedRawGroupInfoSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
		{
			shared_ptr<const Reflection::ValueTable> table;
			std::string errorMessage;

			if (LuaWebService::parseWebJSONResponseHelper(&response, NULL, table, errorMessage))
				resumeFunction(table);
			else
				errorFunction(format("GroupService:GetGroupInfoAsync() failed because %s", errorMessage.c_str()));
		}
	}

	void GroupService::onReceivedRawGroupInfoError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
		{
			if (!error.empty())
			{
				errorFunction(format("GroupService:GetGroupInfoAsync() failed because %s", error.c_str()));
			}
			else
			{
				errorFunction("GroupService:GetGroupInfoAsync() failed because of an unknown error.");
			}
		}
	}

	void GroupService::getGroupsAsync(int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (!DFFlag::GetGroupsAsyncEnabled)
		{
			errorFunction("GroupService:GetGroupsAsync() is not yet enabled!");
			return;
		}

		if (userId <= 0)
		{
			errorFunction("GroupService:GetGroupsAsync() argument is not a valid userId");
			return;
		}
		
		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			apiService->getAsync(format(FString::GetGroupsUrl.c_str(), "", userId), true, RBX::PRIORITY_DEFAULT,
				boost::bind(&GroupService::onReceivedRawGetGroupsSuccess, weak_from(DataModel::get(this)), _1, resumeFunction, errorFunction), 
				boost::bind(&GroupService::onReceivedRawGetGroupsError, weak_from(DataModel::get(this)), _1, errorFunction) );
		}
	}

	void GroupService::onReceivedRawGetGroupsSuccess(weak_ptr<DataModel> weakDataModel, std::string result, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,			
									  boost::function<void(std::string)> errorFunction)
	{
		if (result.empty()) 
		{
			errorFunction("GroupService:GetGroupsAsync() Invalid response received"); 
			return;
		}

		shared_ptr<const Reflection::ValueArray> outputTable;
		if (WebParser::parseJSONArray(result, outputTable))
		{
			resumeFunction(outputTable);
		}
		else 
		{
			errorFunction("GroupService:GetGroupsAsync() error occurred");		
		}	
	}

	void GroupService::onReceivedRawGetGroupsError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction)
	{
		if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
		{
			if (!error.empty())
			{
				errorFunction(format("GroupService:GetGroupsAsync() failed because %s", error.c_str()));
			}
			else
			{
				errorFunction("GroupService:GetGroupsAsync() failed because of an unknown error.");
			}
		}
	}

    void GroupService::getAlliesAsync(int groupId, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
		std::string url;
        if (ContentProvider* cp = DataModel::get(this)->create<ContentProvider>())
            url = format(FString::GroupAlliesUrl.c_str(), cp->getApiBaseUrl().c_str(), groupId);
        
		shared_ptr<Pages> pagination = Creatable<Instance>::create<StandardPages>(weak_from(DataModel::get(this)), url, "Groups");
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
    }

    void GroupService::getEnemiesAsync(int groupId, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
    {
		std::string url;
        if (ContentProvider* cp = DataModel::get(this)->create<ContentProvider>())
            url = format(FString::GroupEnemiesUrl.c_str(), cp->getApiBaseUrl().c_str(), groupId);
        
		shared_ptr<Pages> pagination = Creatable<Instance>::create<StandardPages>(weak_from(DataModel::get(this)), url, "Groups");
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
    }
}

// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag9 = 0;
    };
};
