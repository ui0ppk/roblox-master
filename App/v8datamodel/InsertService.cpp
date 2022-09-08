#include "stdafx.h"

#include "V8DataModel/InsertService.h"
#include "V8DataModel/PartInstance.h"
#include "V8Datamodel/DataModel.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Folder.h"
#include "V8DataModel/RootInstance.h"
#include "V8DataModel/Workspace.h"
#include "Util/LuaWebService.h"
#include "V8Xml/WebParser.h"
#include "Script/Script.h"
#include "Network/Players.h"
#include "Tool/DragUtilities.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "RobloxServicesTools.h"
#include "Util/Analytics.h"
#include "Util/RobloxGoogleAnalytics.h"

#include <sstream>

DYNAMIC_FASTSTRINGVARIABLE(AssetUrlPiece, "/Asset/?id=")
DYNAMIC_FASTSTRINGVARIABLE(AssetVersionUrlPiece, "/Asset/?assetversionid=")
DYNAMIC_FASTSTRINGVARIABLE(BaseSetsUrlPiece, "/Game/Tools/InsertAsset.ashx?nsets=10&type=base")
DYNAMIC_FASTSTRINGVARIABLE(CollectionUrlPiece, "/Game/Tools/InsertAsset.ashx?sid=")
DYNAMIC_FASTSTRINGVARIABLE(FreeModelUrlPiece, "/Game/Tools/InsertAsset.ashx?type=fm&rs=21&q=")
DYNAMIC_FASTSTRINGVARIABLE(FreeDecalUrlPiece, "/Game/Tools/InsertAsset.ashx?type=fd&rs=21&q=")
DYNAMIC_FASTSTRINGVARIABLE(UserSetsUrlPiece, "/Game/Tools/InsertAsset.ashx?nsets=20&type=user&userid=")
DYNAMIC_FASTSTRINGVARIABLE(AssetVersionsUrl, "/assets/%i/versions?placeId=%i")
DYNAMIC_FASTFLAGVARIABLE(GetLastestAssetVersionEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(DisableBackendInsertConnection, false)
DYNAMIC_FASTFLAGVARIABLE(GASendInsertRequestFail, true)
DYNAMIC_FASTFLAGVARIABLE(InfluxSendInsertRequestFail, true)

DYNAMIC_FASTFLAGVARIABLE(InsertServiceLoadModelErrorDoNotCreateEmpty, true)
DYNAMIC_FASTFLAGVARIABLE(InsertServiceLoadModelErrorNoLuaExceptionReturnNull, false)
DYNAMIC_FASTFLAGVARIABLE(DisableInsertServiceForTeamCreate, false)

FASTFLAG(UseBuildGenericGameUrl)

FASTFLAGVARIABLE(AllowInsertFreeModels, false)
FASTFLAGVARIABLE(InsertUnderFolder, true) // defaults true as we could potentially have (minor) data loss if set false

namespace RBX
{
const char* const sInsertService = "InsertService";

REFLECTION_BEGIN();
//Private replicate only helpers
static Reflection::RemoteEventDesc<InsertService, void(std::string, ContentId)> event_insertRequest(&InsertService::insertRequestSignal, "InsertRequest", "key", "contentId", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<InsertService, void(std::string, int, int)>	event_insertRequestAsset(&InsertService::insertRequestAssetSignal, "InsertRequestAsset", "key", "assetId",  "userId", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<InsertService, void(std::string, int, int)>	event_insertRequestAssetVersion(&InsertService::insertRequestAssetVersionSignal, "InsertRequestAssetVersion", "key", "assetVersionId",  "userId", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::RemoteEventDesc<InsertService, void(std::string, shared_ptr<Instance>)> event_insertReady(&InsertService::insertReadySignal,	"InsertReady",	 "key", "instance", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<InsertService, void(std::string, std::string)> event_insertError(&InsertService::insertErrorSignal, "InsertError",	 "key", "message", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<InsertService, void(shared_ptr<Instance>)> event_internalDelete(&InsertService::internalDeleteSignal, "InternalDelete", "instance", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

//Public functions exposed to Lua

/*Deprecated -- Nuke this when the Game service is updated */ static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetBaseCategoryUrl(&InsertService::setBaseSetsUrl,	"SetBaseCategoryUrl", "baseSetsUrl", Security::LocalUser);
/*Deprecated -- Nuke this when the Game service is updated */ static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetUserCategoryUrl(&InsertService::setUserSetsUrl,	"SetUserCategoryUrl", "userSetsUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(float)>	   func_SetTrustLevel(&InsertService::setTrustLevel,	"SetTrustLevel", "trustLevel", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetBaseSetUrl(&InsertService::setBaseSetsUrl,	"SetBaseSetsUrl", "baseSetsUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetUserSetUrl(&InsertService::setUserSetsUrl,	"SetUserSetsUrl", "userSetsUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetFreeModelSetUrl(&InsertService::setFreeModelUrl,	"SetFreeModelUrl", "freeModelUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetFreeDecalSetUrl(&InsertService::setFreeDecalUrl,	"SetFreeDecalUrl", "freeDecalUrl", Security::LocalUser);

static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetCollectionUrl(&InsertService::setCollectionUrl,		"SetCollectionUrl",   "collectionUrl",	 Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetAssetUrl(&InsertService::setAssetUrl,				"SetAssetUrl",		  "assetUrl",		 Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(std::string)> func_SetAssetVersionUrl(&InsertService::setAssetVersionUrl,	"SetAssetVersionUrl", "assetVersionUrl", Security::LocalUser);
static Reflection::BoundFuncDesc<InsertService, void(int)> func_ApproveAssetId(&InsertService::backendApproveAssetId, "ApproveAssetId", "assetId", Security::None);
static Reflection::BoundFuncDesc<InsertService, void(int)> func_ApproveAssetVersionId(&InsertService::backendApproveAssetVersionId, "ApproveAssetVersionId", "assetVersionId", Security::None);

static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(std::string,int)>	func_GetFreeModels(&InsertService::getFreeModels, "GetFreeModels","searchText","pageNum", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(std::string,int)>	func_GetFreeDecals(&InsertService::getFreeDecals, "GetFreeDecals","searchText","pageNum", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(void)>			func_GetDefaultSets(&InsertService::getBaseSets, "GetBaseSets", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(int)>			func_GetUserSets(&InsertService::getUserSets, "GetUserSets", "userId", Security::None);

/*Deprecated*/ static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(void)>	func_GetDefaultCategories(&InsertService::getBaseSets, "GetBaseCategories", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetDefaultSets));
/*Deprecated*/ static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(int)>		func_GetUserCategories(&InsertService::getUserSets, "GetUserCategories", "userId", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetUserSets));
/*Deprecated Nuke this when the Game service is updated: */ static Reflection::BoundFuncDesc<InsertService, void(bool,bool)>   func_SetAdvancedResults(&InsertService::setAdvancedResults,	"SetAdvancedResults", "enable", "user", false, Security::LocalUser);

static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<const Reflection::ValueArray>(int)>	func_GetCollectionObjects(&InsertService::getCollection, "GetCollection", "categoryId", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<Instance>(int)>								func_LoadAsset(&InsertService::loadAsset, "LoadAsset", "assetId", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<Instance>(int)>								func_LoadAssetDep(&InsertService::loadAsset, "loadAsset", "assetId", Security::None, Reflection::Descriptor::Attributes::deprecated(func_LoadAsset));
static Reflection::BoundYieldFuncDesc<InsertService, shared_ptr<Instance>(int)>								func_LoadAssetVersion(&InsertService::loadAssetVersion, "LoadAssetVersion", "assetVersionId", Security::None);
static Reflection::BoundYieldFuncDesc<InsertService, int(int)>								func_GetLastestAssetVersion(&InsertService::getLatestAssetVersion, "GetLatestAssetVersionAsync", "assetId", Security::None);
static Reflection::BoundFuncDesc<InsertService, void(shared_ptr<Instance>)>									func_Insert(&InsertService::insert, "Insert", "instance", Security::None, Reflection::Descriptor::Attributes::deprecated());

static Reflection::PropDescriptor<InsertService, bool> prop_AllowInsertFreeModels("AllowInsertFreeModels", category_Behavior, &InsertService::getAllowInsertFreeModels, &InsertService::setAllowInsertFreeModels, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);

REFLECTION_END();

InsertService::InsertService()
    : allowInsertFreeModels(false)
{
	setName(sInsertService);
    
	loadCount = 0;
    
    if (FFlag::InsertUnderFolder)
    {
        // we create a Folder to hold all instances we load
        // this is set non-archivable so we don't save any of this junk (Save Place API's, Multibuilder Studio, etc.)
        holder = Creatable<Instance>::create<Folder>();
        holder->setIsArchivable(false);
        holder->setParent(this);
    }
    else
    {
        // this preserves old behavior
        Instance::propArchivable.setValue(this, false);
    }
}

void InsertService::setTrustLevel(float value)
{
	// deprecated
}

void InsertService::setFreeModelUrl(std::string value)
{
	// deprecated
}

void InsertService::setFreeDecalUrl(std::string value)
{
	// deprecated
}

void InsertService::setBaseSetsUrl(std::string value)
{
	// deprecated
}

void InsertService::setUserSetsUrl(std::string value)
{
	// deprecated
}
void InsertService::setCollectionUrl(std::string value)
{
	// deprecated
}

void InsertService::setAssetUrl(std::string value)
{
	// deprecated
}

void InsertService::setAssetVersionUrl(std::string value)
{
	// deprecated
}

void InsertService::setAdvancedResults(bool advancedResults, bool userMode)
{
}

void InsertService::dispatchRequest(const std::string& url, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(LuaWebService* luaWebService = ServiceProvider::create<LuaWebService>(this))
	{ 
		try
		{
			luaWebService->asyncRequest(url, LUA_WEB_SERVICE_STANDARD_PRIORITY, resumeFunction, errorFunction);
		}
		catch(RBX::base_exception&)
		{
			errorFunction("Error during dispatch");
		}
	} 
	else
	{
		errorFunction("Shutting down");
	}
}

void InsertService::getFreeModels(std::string searchText,int pageNum,boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	dispatchRequest(addBaseUrlAndQuery(DFString::FreeModelUrlPiece, searchText, pageNum), resumeFunction, errorFunction);
}
void InsertService::getFreeDecals(std::string searchText,int pageNum,boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	dispatchRequest(addBaseUrlAndQuery(DFString::FreeDecalUrlPiece, searchText, pageNum), resumeFunction, errorFunction);
}
void InsertService::getBaseSets(boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	dispatchRequest(addBaseUrl(DFString::BaseSetsUrlPiece), resumeFunction, errorFunction);
}

void InsertService::getUserSets(int userId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)>	errorFunction)
{
	dispatchRequest(addBaseUrlAndId(DFString::UserSetsUrlPiece, userId), resumeFunction, errorFunction);
}

void InsertService::getCollection(int collectionId, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	dispatchRequest(addBaseUrlAndId(DFString::CollectionUrlPiece, collectionId), resumeFunction, errorFunction);
}

void InsertService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(oldProvider){
		if(Network::Players::backendProcessing(oldProvider)){
			if (backendInsertConnection.connected())
			{
				backendInsertConnection.disconnect();
			}
			backendInsertAssetConnection.disconnect();
			backendInsertAssetVersionConnection.disconnect();
            backendInternalDeleteConnection.disconnect();
		}
	}
	
	Super::onServiceProvider(oldProvider, newProvider);
	
	if(newProvider){
		if(Network::Players::backendProcessing(newProvider)){
			if (!DFFlag::DisableBackendInsertConnection)
			{
				backendInsertConnection = insertRequestSignal.connect(boost::bind(&InsertService::backendInsertRequested, this, _1, true, _2));
			}
			backendInsertAssetConnection = insertRequestAssetSignal.connect(boost::bind(&InsertService::backendInsertAssetRequested, this, _1, true, _2, _3));
			backendInsertAssetVersionConnection = insertRequestAssetVersionSignal.connect(boost::bind(&InsertService::backendInsertAssetVersionRequested, this, _1, true, _2, _3));
            backendInternalDeleteConnection = internalDeleteSignal.connect(boost::bind(&InsertService::internalDelete, this, _1));
		}
	}
}

void InsertService::insertResultsReady(std::string key, shared_ptr<Instance> container)
{
	boost::function<void(shared_ptr<Instance>)> resumeFunction = NULL;


	{
		boost::recursive_mutex::scoped_lock lock(callbackSync);
		CallbackLibrary::iterator iter = callbackLibrary.find(key);
		if(iter != callbackLibrary.end()){
			resumeFunction = iter->second.resumeFunction;
			callbackLibrary.erase(iter);
		}
	}
	if(resumeFunction){
		resumeFunction(container);
	}
}

void InsertService::insertResultsError(std::string key, std::string message)
{
	boost::function<void(std::string)> errorFunction = NULL;
	{
		boost::recursive_mutex::scoped_lock lock(callbackSync);
		CallbackLibrary::iterator iter = callbackLibrary.find(key);
		if(iter != callbackLibrary.end()){
			errorFunction = iter->second.errorFunction;
			callbackLibrary.erase(iter);
		}
	}
	if(errorFunction){
		errorFunction(message);
	}
}


void InsertService::loadAsset(int id, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	privateLoadAsset(id, false, resumeFunction, errorFunction);
}
void InsertService::loadAssetVersion(int id, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	privateLoadAsset(id, true, resumeFunction, errorFunction);
}

template<class T> bool valueTableGet(shared_ptr<const Reflection::ValueTable> valueTable, const std::string& key, T* outValue)
{
    Reflection::ValueTable::const_iterator it = valueTable->find(key);
    if(it == valueTable->end() || !it->second.isType<T>()) {
        return false;
    }
        
    *outValue = it->second.cast<T>();
        
    return true;
}

void InsertService::getLatestAssetVersionSuccess(std::string result, boost::function<void(int)> resumeFunction,			
									  boost::function<void(std::string)> errorFunction)
{
	if(result.empty()) 
	{
		errorFunction("Invalid response received"); 
		return;
	}

	shared_ptr<const Reflection::ValueArray> inputTable;
	if(WebParser::parseJSONArray(result, inputTable))
	{
		if (int(inputTable->size()) > 0) 
		{
			Reflection::ValueArray::const_iterator itBegin = inputTable->begin();
			if (itBegin->isType<shared_ptr<const Reflection::ValueTable> >()) 
			{
				shared_ptr<const Reflection::ValueTable> version = itBegin->cast<shared_ptr<const Reflection::ValueTable> >();
				int versionId;
				if (valueTableGet(version, "Id", &versionId))
				{
					resumeFunction(versionId);
				}
				else
				{
					errorFunction("getLatestAssetVersion error occurred");	
				}
			}
			else
			{
				errorFunction("getLatestAssetVersion error occurred");
			}
		} 
		else 
		{
			errorFunction("getLatestAssetVersion error occurred");	
		}
	}
	else 
	{
		errorFunction("getLatestAssetVersion error occurred");		
	}	
}

void InsertService::getLatestAssetVersionError(std::string error, boost::function<void(std::string)> errorFunction)
{
	errorFunction( RBX::format("InsertService getLatestAssetVersion : Request Failed because %s", error.c_str()) );
}

void InsertService::getLatestAssetVersion(int id, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (!DFFlag::GetLastestAssetVersionEnabled)
	{
		errorFunction("GetLastestAssetVersionAsync() is currently disabled");
		return;
	}
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		if (DataModel* dm = DataModel::get(this)) 
		{
			apiService->getAsync(RBX::format(DFString::AssetVersionsUrl.c_str(), id, dm->getPlaceID()), true, RBX::PRIORITY_DEFAULT,
				boost::bind(&InsertService::getLatestAssetVersionSuccess, this, _1, resumeFunction, errorFunction),
				boost::bind(&InsertService::getLatestAssetVersionError, this, _1, errorFunction) );
		}
	}
}

void InsertService::privateLoadAsset(int id, bool isAssetVersion, boost::function<void(shared_ptr<Instance>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	std::stringstream key;
	int userId = -1;
	if (RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this))
	{
		//If we are in a multiplary environment, prepend our "userId" to uniquely identify the request
		userId = localPlayer->getUserID();
		key << userId << '+';
	}
	key << id << '+' << loadCount++;
	const std::string keyRequest = key.str();
	
	{
		//Data might come back on a different thread, so we need a mutex on this data structure
		boost::recursive_mutex::scoped_lock lock(callbackSync);
		callbackLibrary[keyRequest].resumeFunction = resumeFunction;
		callbackLibrary[keyRequest].errorFunction = errorFunction;
	}

	if (!frontendInsertReadyConnection.connected())
	{
		//Lazy initialization of this fontendConnection (no point listening if no one is using it)
		frontendInsertReadyConnection = insertReadySignal.connect(boost::bind(&InsertService::insertResultsReady, this, _1, _2));
		frontendInsertErrorConnection = insertErrorSignal.connect(boost::bind(&InsertService::insertResultsError, this, _1, _2));
	}
	if (Network::Players::frontendProcessing(this) && !Network::Players::backendProcessing(this))
	{
		//We're a client, so have the server request the object
		if (isAssetVersion)
		{
			event_insertRequestAssetVersion.replicateEvent(this, keyRequest, id, userId);
		}
		else
		{
			event_insertRequestAsset.replicateEvent(this, keyRequest, id, userId);
		}
	}
	else
	{
		if (isAssetVersion)
		{
			backendInsertAssetVersionRequested(keyRequest, false, id, userId);
		}
		else
		{
			backendInsertAssetRequested(keyRequest, false, id, userId);
		}
	}
}

void InsertService::insert(shared_ptr<Instance> instance)
{
	if(DataModel* dataModel = (DataModel*)getParent())
	{
		if (!instance)
			throw std::runtime_error("instance must be non-nil");
			
		// make sure we aren't trying to manipulate a robloxlocked object under the wrong permission
		if (instance->getRobloxLocked())
			RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "Roblox locked in InsertService:Insert()");

		Instances instances;
		instances.push_back(instance);

		PartArray parts;
		dataModel->getWorkspace()->publicInsertRaw(instances, dataModel->getWorkspace(), parts, true);
	}
}

void InsertService::internalDelete(shared_ptr<Instance> instance)
{
    RBXASSERT(instance);
    RBXASSERT(instance->isDescendantOf(this));
    if (!instance || (!instance->isDescendantOf(this)))
    {
        return;
    }
    if (Network::Players::backendProcessing(this))
    {
        instance->setParent(NULL);
    }
    else
    {
        event_internalDelete.replicateEvent(this, instance);
    }
}

//Runs in a write task
void InsertService::backendInsertReady(std::string key, shared_ptr<Instance> pseudoRoot, AsyncHttpQueue::RequestResult requestResult, shared_ptr<std::exception> requestError)
{
	if (requestError.get() == NULL || !DFFlag::InsertServiceLoadModelErrorDoNotCreateEmpty)
	{
		try
		{
			if (FFlag::InsertUnderFolder) {
				pseudoRoot->setParent(holder.get());
			} else {
				pseudoRoot->setParent(this);
			}
		}
		catch(RBX::base_exception&)
		{
			pseudoRoot = Creatable<Instance>::create<ModelInstance>();
			if (FFlag::InsertUnderFolder) {
				pseudoRoot->setParent(holder.get());
			} else {
				pseudoRoot->setParent(this);
			}
		} 
		event_insertReady.fireAndReplicateEvent(this, key, pseudoRoot);
	}
	else
	{
		if (DFFlag::GASendInsertRequestFail)
		{
			DataModel* dm = DataModel::get(this);
			if (dm != NULL)
			{
				RBX::Analytics::GoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "InsertServiceException", "PlaceID", dm->getPlaceID());
			}
		}
		if (DFFlag::InfluxSendInsertRequestFail)
		{
			DataModel* dm = DataModel::get(this);
			if (dm != NULL)
			{
				boost::unordered_set<RBX::Analytics::InfluxDb::Point> points;
				rapidjson::Value placeIDNode;
				placeIDNode.SetInt(dm->getPlaceID());
				RBX::Analytics::InfluxDb::Point point("PlaceID",placeIDNode);
				points.insert(point);
				RBX::Analytics::InfluxDb::reportPointsV2("InsertServiceFailure", points, 0);
			}
		}

		if (DFFlag::InsertServiceLoadModelErrorNoLuaExceptionReturnNull)
		{
			event_insertReady.fireAndReplicateEvent(this, key, shared_ptr<Instance>());
		}
		else
		{
			event_insertError.fireAndReplicateEvent(this, key, requestError->what());
		}
	}
	
}

void InsertService::BackendInsertReadyHelper(weak_ptr<InsertService> insertService, std::string key, shared_ptr<Instance> pseudoRoot, AsyncHttpQueue::RequestResult requestResult, shared_ptr<std::exception> requestError)
{
	if(shared_ptr<InsertService> safeInsertService = insertService.lock())
	{
		safeInsertService->backendInsertReady(key, pseudoRoot, requestResult, requestError);
	}
}

void InsertService::backendInsertRequested(std::string key, bool clientInsert, ContentId asset)
{	
	safeInsert(asset, clientInsert, boost::bind(&InsertService::BackendInsertReadyHelper, weak_from(this), key, _1, _2, _3));
}

void InsertService::backendInsertRequested(InsertRequestType requestType, int id, std::string key, bool clientInsert)
{
	safeInsert(requestType, id, clientInsert, boost::bind(&InsertService::BackendInsertReadyHelper, weak_from(this), key, _1, _2, _3));
}

void InsertService::backendApproveAssetId(int assetId)
{
}
void InsertService::backendApproveAssetVersionId(int assetVersionId)
{
}

void InsertService::backendInsertAssetVersionRequested(std::string key, bool clientInsert, int assetVersionId, int userId)
{
	if (DFFlag::DisableBackendInsertConnection)
	{
		backendInsertRequested(ASSET_VERSION, assetVersionId, key, clientInsert);
	}
	else
	{
		ContentId contentId = ContentId(addBaseUrlAndId(DFString::AssetVersionUrlPiece, assetVersionId));
		backendInsertRequested(key, clientInsert, contentId);
	}
}

void InsertService::backendInsertAssetRequested(std::string key, bool clientInsert, int assetId, int userId)
{
	if (DFFlag::DisableBackendInsertConnection)
	{
		backendInsertRequested(ASSET, assetId, key, clientInsert);
	}
	else
	{
		ContentId contentId = ContentId(addBaseUrlAndId(DFString::AssetUrlPiece, assetId));
		backendInsertRequested(key, clientInsert, contentId);
	}
}

static void CallResultFunction(boost::function<void(shared_ptr<Instance>)> resultFunction, shared_ptr<Instance> pseudoRoot)
{
	resultFunction(pseudoRoot);
}

void InsertService::RemoteInsertItemsLoadedHelper(
	weak_ptr<InsertService> insertService, 
	weak_ptr<ContentProvider> cp, 
	AsyncHttpQueue::RequestResult requestResult, 
	shared_ptr<Instances> instances,
	shared_ptr<std::exception> requestError,
	boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult, shared_ptr<std::exception>)> resultFunction)
{
	if(shared_ptr<InsertService> insertServiceSafe = insertService.lock())
	{
		insertServiceSafe->remoteInsertItemsLoaded(cp, requestResult, instances, requestError, resultFunction);
	}
}

void InsertService::remoteInsertItemsLoaded(
	weak_ptr<ContentProvider> weakCp, 
	AsyncHttpQueue::RequestResult requestResult, 
	shared_ptr<Instances> instances,
	shared_ptr<std::exception> requestError,
	boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult requestResult, shared_ptr<std::exception>)> resultFunction)
{	
	shared_ptr<Instance> pseudoRoot = Creatable<Instance>::create<ModelInstance>();
	for(Instances::iterator iter = instances->begin(); iter != instances->end(); ++iter)
	{
		(*iter)->setParent(pseudoRoot.get());
	}

	if (DFFlag::MaterialPropertiesEnabled)
	{
		DataModel* dm = DataModel::get(this);
		if (dm && dm->getWorkspace()->getUsingNewPhysicalProperties())
		{		
			for (size_t i = 0; i < instances->size(); ++i)
				RBX::PartInstance::convertToNewPhysicalPropRecursive((*instances)[i].get());
		}
	}

	if (shared_ptr<ContentProvider> cp = weakCp.lock())
	{
		LuaSourceContainer::loadLinkedScripts(cp, pseudoRoot.get(), AsyncHttpQueue::AsyncWrite,
			boost::bind(resultFunction, pseudoRoot, requestResult, requestError));
	}
}

// add URL parameters that help govern security around inserts
void InsertService::populateExtraInsertUrlParams(std::stringstream& url, bool clientInsert)
{
    // if we allow free model insert, we don't require that a model is taken before inserting it if it's free
    // omitting the serverplaceid parameter will disable the model-must-be-taken check on the server
    if (!FFlag::AllowInsertFreeModels || !allowInsertFreeModels || clientInsert) {
        RBX::DataModel* dataModel = DataModel::get(this);
        url << "&serverplaceid=";
        url << dataModel->getPlaceIDOrZeroInStudio();
    }
    
    url << "&clientinsert=" << (clientInsert ? 1 : 0);
}

void InsertService::safeInsert(
	ContentId asset, 
	bool clientInsert, 
	boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult, shared_ptr<std::exception>)> callback)
{
	if (DFFlag::DisableInsertServiceForTeamCreate && Network::Players::isCloudEdit(this)) return;

	// Adding placeid to insert service requests
	// This is delayed until right before sending a request through
	// ContentProvider to make sure that placeid can be read from datamodel.
	std::stringstream newUrl;
	newUrl << asset.toString();
    
    populateExtraInsertUrlParams(newUrl, clientInsert);
    
    ContentId substitute(newUrl.str());
	
	ContentProvider* cp = ServiceProvider::create<ContentProvider>(this);
	cp->loadContent(substitute,ContentProvider::PRIORITY_INSERT,
		boost::bind(
			&InsertService::RemoteInsertItemsLoadedHelper, 
			weak_from(this), 
			weak_from(cp), 
			_1, // Param1 - AsyncHttpQueue::RequestResult requestResult
			_2, // Param2 - shared_ptr<Instances> instances
			_3, // Param3 - shared_ptr<std::exception> requestError
			callback
	));
}

void InsertService::safeInsert(
	InsertRequestType requestType, 
	int id, 
	bool clientInsert,
	boost::function<void(shared_ptr<Instance>, AsyncHttpQueue::RequestResult, shared_ptr<std::exception>)> callback)
{
	if (DFFlag::DisableInsertServiceForTeamCreate && Network::Players::isCloudEdit(this)) return;

	// Adding placeid to insert service requests
	// This is delayed until right before sending a request through
	// ContentProvider to make sure that placeid can be read from datamodel.
	ContentProvider* cp = ServiceProvider::create<ContentProvider>(this);
	std::stringstream newUrl;

	newUrl << ::trim_trailing_slashes(cp->getBaseUrl());
	if (requestType == ASSET)
	{
		newUrl << DFString::AssetUrlPiece;
	}
	else if (requestType == ASSET_VERSION)
	{
		newUrl << DFString::AssetVersionUrlPiece;
	}
	else
	{
		RBXASSERT(0);
	}
	newUrl << id;

    populateExtraInsertUrlParams(newUrl, clientInsert);

	ContentId substitute(newUrl.str());
	
	cp->loadContent(
		substitute,
		ContentProvider::PRIORITY_INSERT,
		boost::bind(
			&InsertService::RemoteInsertItemsLoadedHelper, 
			weak_from(this), 
			weak_from(cp), 
			_1, // Param1 - AsyncHttpQueue::RequestResult requestResult
			_2, // Param2 - shared_ptr<Instances> instances
			_3, // Param3 - shared_ptr<std::exception> requestError
			callback));
}

std::string InsertService::addBaseUrl(const std::string& urlPiece)
{
	if (FFlag::UseBuildGenericGameUrl)
	{
		return BuildGenericGameUrl(ServiceProvider::find<ContentProvider>(this)->getBaseUrl(), urlPiece);
	}
	else
	{
		return ::trim_trailing_slashes(ServiceProvider::find<ContentProvider>(this)->getBaseUrl()) + urlPiece;
	}
}

std::string InsertService::addBaseUrlAndId(const std::string& urlPiece, int id)
{
	std::ostringstream s;
	s << addBaseUrl(urlPiece) << id;
	return s.str();
}

std::string InsertService::addBaseUrlAndQuery(const std::string& urlPiece, const std::string& query, int page)
{
	std::ostringstream s;
	s << addBaseUrl(urlPiece) << query << "&pg=" << page;
	return s.str();
}
    
void InsertService::setAllowInsertFreeModels(bool value)
{
    if (allowInsertFreeModels != value) {
        allowInsertFreeModels = value;
        raisePropertyChanged(prop_AllowInsertFreeModels);
    }
}

}
