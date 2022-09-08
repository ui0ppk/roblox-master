#pragma once 
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX {
	extern const char* const sAssetService;
	class AssetService
		:public DescribedNonCreatable<AssetService, Instance, sAssetService>
		,public Service
	{
		ThrottlingHelper createPlaceThrottle, savePlaceThrottle;

		public:	

			enum AccessType
			{	
				ME = 0,
				FRIENDS = 1, 
				EVERYONE = 2,				
				INVITEONLY = 3,				
			};

			AssetService(); 			
			void setPlaceAccessUrl(std::string url); 
			void setAssetRevertUrl(std::string url); 
			void setAssetVersionsUrl(std::string url); 

			void revertAsset(int assetId, int versionNumber, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction); 			
			void getAssetVersions(int assetId, int pageNum, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);
			void getPlacePermissions(int placeId, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);
			void setPlacePermissions(int placeId, AccessType type, shared_ptr<const Reflection::ValueArray> inviteList,
				boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
			
			void getCreatorAssetID(int creationID, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

			void createPlaceAsync(std::string placeName, int templatePlaceId, std::string desc, 
				boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

			void createPlaceInPlayerInventoryAsync(shared_ptr<Instance> player, std::string placeName, int templatePlaceId, std::string desc,
				boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

			void savePlaceAsync(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

			void getGamePlacesAsync(boost::function<void(shared_ptr<Instance>) > resumeFunction,
				boost::function<void(std::string)> errorFunction);


		private:
			std::string placeAccessUrl; 			
			std::string assetRevertUrl;
			std::string assetVersionsUrl; 
			void httpPostHelper(std::string* response, std::exception* httpException, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
			void processServiceResults(std::string *result, const std::exception* httpException, 
				boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction); 		

			void getCreatorAssetIDSuccessHelper(std::string response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
			void getCreatorAssetIDErrorHelper(std::string error, boost::function<void(std::string)> errorFunction);

			void createPlaceAsyncInternal(bool check, std::string placeName, int templatePlaceId, std::string desc, shared_ptr<Instance> player,
				boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);

			bool checkCreatePlaceAccess(const std::string& placeName, int templatePlaceId, std::string& message);

	};
}