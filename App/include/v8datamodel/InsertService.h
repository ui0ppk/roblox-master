#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "V8DataModel/ContentProvider.h"
#include "rbx/signal.h"
#include "rbx/boost.hpp"
#include "boost/thread.hpp"
#include <boost/unordered_set.hpp>

namespace RBX {

	extern const char* const sInsertService;
	class InsertService
		: public DescribedCreatable<InsertService, Instance, sInsertService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
		, public Service

	{
	private:
		typedef DescribedCreatable<InsertService, Instance, sInsertService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;

	public:
		InsertService();

		rbx::remote_signal<void(std::string, ContentId)> insertRequestSignal;
		rbx::remote_signal<void(std::string, int, int)> insertRequestAssetSignal;
		rbx::remote_signal<void(std::string, int, int)> insertRequestAssetVersionSignal;
		rbx::remote_signal<void(std::string, shared_ptr<Instance>)> insertReadySignal;
		rbx::remote_signal<void(std::string, std::string)> insertErrorSignal;
        rbx::remote_signal<void(shared_ptr<Instance>)> internalDeleteSignal;

		void setTrustLevel(float);
		void setFreeModelUrl(std::string);
		void setFreeDecalUrl(std::string);
		void setBaseSetsUrl(std::string);
		void setUserSetsUrl(std::string);
		void setCollectionUrl(std::string);
		void setAssetUrl(std::string);
		void setAssetVersionUrl(std::string);
		void setAdvancedResults(bool advancedResults, bool userMode);

		void getFreeDecals(std::string searchText, int pageNum, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getFreeModels(std::string searchText, int pageNum, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getBaseSets(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getUserSets(int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getCollection(int collectionId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void loadAsset(int id, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void loadAssetVersion(int id, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getLatestAssetVersion(int id, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void insert(shared_ptr<Instance> instance);
        void internalDelete(shared_ptr<Instance> instance);

		enum InsertRequestType { ASSET, ASSET_VERSION };

		void safeInsert(ContentId asset, bool clientInsert, boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult requestResult, shared_ptr<std::exception> requestError)> callback);
		void safeInsert(InsertRequestType requestType, int id, bool clientInsert, boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult requestResult, shared_ptr<std::exception> requestError)> callback);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		
		void backendApproveAssetId(int assetId);
		void backendApproveAssetVersionId(int assetVersionId);

        bool getAllowInsertFreeModels() const { return allowInsertFreeModels; }
        void setAllowInsertFreeModels(bool value);

    private:
        void dispatchRequest(const std::string& url, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction);
        
		void privateLoadAsset(int id, bool isAssetVersion, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction);

        void populateExtraInsertUrlParams(std::stringstream& url, bool clientInsert);

		rbx::signals::connection backendInsertConnection;
		rbx::signals::connection backendInsertAssetConnection;
		rbx::signals::connection backendInsertAssetVersionConnection;
        rbx::signals::scoped_connection backendInternalDeleteConnection;
		void backendInsertRequested(std::string key, bool clientInsert, ContentId contentId);
		void backendInsertRequested(InsertRequestType requestType, int id, std::string key, bool clientInsert);
		void backendInsertAssetRequested(std::string key, bool clientInsert, int assetId, int userId);
		void backendInsertAssetVersionRequested(std::string key, bool clientInsert, int assetVersionId, int userId);

		void getLatestAssetVersionSuccess(std::string result, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getLatestAssetVersionError(std::string error, boost::function<void(std::string)> errorFunction);

		void backendInsertReady(
			std::string key, 
			shared_ptr<Instance> pseudoRoot, 
			AsyncHttpQueue::RequestResult requestResult,
			shared_ptr<std::exception> requestError);
		static void BackendInsertReadyHelper(
			weak_ptr<InsertService> insertService, 
			std::string key, 
			shared_ptr<Instance> pseudoRoot, 
			AsyncHttpQueue::RequestResult requestResult, 
			shared_ptr<std::exception> requestError);

		void remoteInsertItemsLoaded(
			weak_ptr<ContentProvider> weakCp, 
			AsyncHttpQueue::RequestResult requestResult, 
			shared_ptr<Instances> instances,
			shared_ptr<std::exception> requestError,
			boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult, shared_ptr<std::exception>)> resultFunction);
		
		static void RemoteInsertItemsLoadedHelper(
			weak_ptr<InsertService> insertService, 
			weak_ptr<ContentProvider> cp, 
			AsyncHttpQueue::RequestResult requestResult, 
			shared_ptr<Instances> instances,
			shared_ptr<std::exception> requestError,
			boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult, shared_ptr<std::exception>)> resultFunction);


		rbx::signals::connection frontendInsertReadyConnection;
		rbx::signals::connection frontendInsertErrorConnection;
		void insertResultsReady(std::string key, shared_ptr<Instance> container);
		void insertResultsError(std::string key, std::string message);
		std::string addBaseUrl(const std::string& urlPiece);
		std::string addBaseUrlAndId(const std::string& urlPiece, int id);
		std::string addBaseUrlAndQuery(const std::string& urlPiece, const std::string& query, int page);

		boost::recursive_mutex callbackSync;		// synchronizes the callbackLibrary
		struct Callback
		{
			boost::function<void(shared_ptr<Instance>)> resumeFunction;
			boost::function<void(std::string)>		errorFunction;
		};
		typedef std::map<std::string, Callback> CallbackLibrary;
		CallbackLibrary callbackLibrary;
		rbx::atomic<int> loadCount;	// used to uniquely identify a load asset request
        shared_ptr<Instance> holder; // items are initially loaded into this Folder
        
        bool allowInsertFreeModels;
	};

}

