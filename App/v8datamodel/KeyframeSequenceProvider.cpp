#include "stdafx.h"

#include "V8DataModel/KeyframeSequenceProvider.h"
#include "V8DataModel/InsertService.h"
#include "V8DataModel/KeyframeSequence.h"
#include "V8DataModel/ContentProvider.h"
#include "Util/AnimationId.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/WebParser.h"
#include "V8Xml/XmlSerializer.h"
#include "V8Xml/SerializerBinary.h"
#include "Network/Players.h"
#include <boost/algorithm/string.hpp>

#include "rbx/Profiler.h"

DYNAMIC_FASTFLAGVARIABLE(AnimationAllowProdUrls, true);
DYNAMIC_FASTFLAGVARIABLE(DontUseInsertServiceOnAnimLoad, false)
DYNAMIC_FASTFLAGVARIABLE(AnimationFailedToLoadContext, false)

namespace RBX
{

const char* const sKeyframeSequenceProvider = "KeyframeSequenceProvider";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<KeyframeSequenceProvider, ContentId(shared_ptr<Instance>)> func_setActiveKeyframeSequence(&KeyframeSequenceProvider::registerActiveKeyframeSequence, "RegisterActiveKeyframeSequence", "keyframeSequence", Security::None);
static Reflection::BoundFuncDesc<KeyframeSequenceProvider, ContentId(shared_ptr<Instance>)> func_setKeyframeSequence(&KeyframeSequenceProvider::registerKeyframeSequence, "RegisterKeyframeSequence", "keyframeSequence", Security::None);
static Reflection::BoundFuncDesc<KeyframeSequenceProvider, shared_ptr<Instance>(ContentId)> func_getKeyframeSequence(&KeyframeSequenceProvider::getKeyframeSequenceLua, "GetKeyframeSequence", "assetId", Security::None);
static Reflection::BoundFuncDesc<KeyframeSequenceProvider, shared_ptr<Instance>(int, bool)> func_getKeyframeSequenceById(&KeyframeSequenceProvider::getKeyframeSequenceByIdLua, "GetKeyframeSequenceById", "assetId", "useCache", Security::None);

static const Reflection::BoundYieldFuncDesc<KeyframeSequenceProvider, shared_ptr<const Reflection::ValueTable>(int,int)> func_getAnimations(&KeyframeSequenceProvider::getAnimations, "GetAnimations", "userId", "page", 1, Security::None);
REFLECTION_END();

KeyframeSequenceProvider::KeyframeSequenceProvider()
	:activeKeyframeId(0)
	,keyframeSequenceCache(100)
{

}
ContentId KeyframeSequenceProvider::registerActiveKeyframeSequence(shared_ptr<Instance> instance)
{
	if(Network::Players::clientIsPresent(this, false) || Network::Players::serverIsPresent(this, false)){
		throw RBX::runtime_error("Usage error: RegisterActiveKeyframeSequence can only be used in Solo mode.");
	}

	if(shared_ptr<KeyframeSequence> keyframeSequence = Instance::fastSharedDynamicCast<KeyframeSequence>(instance)){
		std::string id = RBX::format("active://%d", activeKeyframeId++);
		activeKeyframeTable[id] = keyframeSequence;
		return ContentId(id);
	}
	throw std::runtime_error("Argument must be a 'KeyframeSequence' object");
}

static bool itIsInScope(Instance*)
{
	return true;
}

ContentId KeyframeSequenceProvider::registerKeyframeSequence(shared_ptr<Instance> keyframeSequence)
{
	if((Network::Players::clientIsPresent(this, false) && !Network::Players::isCloudEdit(this)) || Network::Players::serverIsPresent(this, false)){
		throw RBX::runtime_error("Usage error: RegisterKeyframeSequence can only be used in Solo mode.");
	}

	std::stringstream stream;

    Instances instances;
    instances.push_back(keyframeSequence);

    SerializerBinary::serialize(stream, instances);

	return ContentProvider::registerContent(stream);
}

shared_ptr<Instance> KeyframeSequenceProvider::getKeyframeSequenceByIdLua(int assetId, bool useCache)
{
	if(assetId <= 0)
	{
		throw RBX::runtime_error("GetKeyframeSequenceById assetId is not a positive number!");
	}

	std::string parameters =  RBX::format("asset/?id=%d", assetId);
	std::string url = ServiceProvider::create<ContentProvider>(this)->getBaseUrl() + parameters;

	shared_ptr<KeyframeSequence> result = privateGetKeyframeSequence(ContentId::fromUrl(url), true, false, "KeyframeSequenceProvider:GetKeyframeSequenceById()");
	if(result){
		return result->clone(EngineCreator);
	}
	
	return shared_ptr<Instance>();
}


shared_ptr<Instance> KeyframeSequenceProvider::getKeyframeSequenceLua(ContentId assetId)
{
	shared_ptr<KeyframeSequence> result = privateGetKeyframeSequence(assetId, true, true, "KeyframeSequenceProvider:GetKeyframeSequence()");
	if(result){
		return result->clone(EngineCreator);
	}

	return shared_ptr<Instance>();
}

shared_ptr<const KeyframeSequence> KeyframeSequenceProvider::getKeyframeSequence(ContentId assetId, const Instance* context)
{
	return privateGetKeyframeSequence(assetId, false, true, "", context);
}


//Operates with a DataModel::write task
static void CopyKeyframeSequenceData(boost::weak_ptr<KeyframeSequence> weakKeyframeSequence, boost::shared_ptr<KeyframeSequence> keyframeSequenceData)
{
    RBXPROFILER_SCOPE("Animation", "copyKeyframeSequence");
    
	if(boost::shared_ptr<KeyframeSequence> keyframeSequence = weakKeyframeSequence.lock()){
		keyframeSequence->copyKeyframeSequence(keyframeSequenceData.get());
	}
}

static void copyLoadedData(const Instances instances,
	boost::weak_ptr<KeyframeSequenceProvider> keyframeSequenceProvider,
	boost::weak_ptr<KeyframeSequence> keyframeSequence,
	bool synchronous)
{
	if(instances.size() == 1){
		if(boost::shared_ptr<KeyframeSequence> newKeyframeSequence = Instance::fastSharedDynamicCast<KeyframeSequence>(instances[0])){
			if(boost::shared_ptr<KeyframeSequenceProvider> safeKeyframeSequenceProvider = keyframeSequenceProvider.lock()){
				if(synchronous){
					CopyKeyframeSequenceData(keyframeSequence, newKeyframeSequence);
				}
				else{
					if(DataModel* dataModel = DataModel::get(safeKeyframeSequenceProvider.get())){
						dataModel->submitTask(boost::bind(&CopyKeyframeSequenceData, keyframeSequence, newKeyframeSequence), DataModelJob::Write);
					}
				}
			}
		}
	}
}

static void loadDataFromInsertService(boost::shared_ptr<Instance> ist,
	boost::weak_ptr<KeyframeSequenceProvider> keyframeSequenceProvider,
	boost::weak_ptr<KeyframeSequence> keyframeSequence,
	bool synchronous, const std::string& context)
{
	if (shared_ptr<const Instances> children = ist->getChildren2())
	{
		copyLoadedData(*children, keyframeSequenceProvider, keyframeSequence, synchronous);
	}
	else if (DFFlag::AnimationFailedToLoadContext)
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "Animation failed to load : %s", context.c_str());
	}
    // Note: this cleanup isn't guaranteed to always work, if the client shuts down
    // before it can tell the server that the animation can be cleaned up, the server
    // will never clean up the animation.
    if (shared_ptr<KeyframeSequenceProvider> kp = keyframeSequenceProvider.lock())
    {
        if (InsertService* insertService = ServiceProvider::find<InsertService>(kp.get()))
        {
            insertService->internalDelete(ist);
        }
        else
        {
            // This callback was called from insert service when an asset loaded. How
            // can the insert service not exist?
            RBXASSERT(false);
        }
    }
}


//Does not operate in DataModel::write thread
static void KeyframeLoaderHelper(AsyncHttpQueue::RequestResult result, std::istream* stream, 
			   				     boost::weak_ptr<KeyframeSequenceProvider> keyframeSequenceProvider, boost::weak_ptr<KeyframeSequence> keyframeSequence,
								 bool synchronous, const std::string& context)
{
	if (result == AsyncHttpQueue::Succeeded)
	{
        Instances instances;
        Serializer().loadInstances(*stream, instances);
        copyLoadedData(instances, keyframeSequenceProvider, keyframeSequence, synchronous);
	}
	else if (result == AsyncHttpQueue::Failed && DFFlag::AnimationFailedToLoadContext)
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "Animation failed to load : %s", context.c_str());
	}
}

static void SyncKeyframeLoaderHelper(AsyncHttpQueue::RequestResult result, std::istream* stream, 
									 boost::weak_ptr<KeyframeSequenceProvider> keyframeSequenceProvider, boost::weak_ptr<KeyframeSequence> keyframeSequence, const std::string context)
{
	return KeyframeLoaderHelper(result, stream, keyframeSequenceProvider, keyframeSequence, true, context);
}

static void AsyncKeyframeLoaderHelper(AsyncHttpQueue::RequestResult result, std::istream* stream, 
									  boost::weak_ptr<KeyframeSequenceProvider> keyframeSequenceProvider, boost::weak_ptr<KeyframeSequence> keyframeSequence, const std::string context)
{
	return KeyframeLoaderHelper(result, stream, keyframeSequenceProvider, keyframeSequence, false, context);
}

static void doNothingErrorHandler(std::string err, const std::string context) 
{
	if (DFFlag::AnimationFailedToLoadContext)
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "Animation failed to load : %s", context.c_str());		
	}
}

shared_ptr<KeyframeSequence> KeyframeSequenceProvider::privateGetKeyframeSequence(ContentId assetId, bool blocking, bool useCache, const std::string& contextString, const Instance* contextInstance)
{
	std::string assetIdNumber;
	if(assetId.isHttp() || assetId.isAsset() || assetId.isAssetId())
	{
		std::string url = ""; 
		std::string assetString(assetId.c_str()); 		
		unsigned pos = 12; // this is the size of the AssetId header "rbxassetid://"
		if (!assetId.isAssetId()) 
		{
			pos = assetString.find_last_of("=");  // if it's not an AssetId, find the id in the string
		}
		if (DFFlag::AnimationAllowProdUrls && boost::starts_with(assetId.toString(), "http://www.roblox.com"))	{
			url.append("http://www.roblox.com"); 
		}
		else {
			url.append(ServiceProvider::create<ContentProvider>(this)->getBaseUrl()); 
		}
		assetIdNumber = assetString.substr(pos+1, assetString.length());
		url.append("/asset/?id=").append(assetIdNumber); 		
		url.append("&serverplaceid=").append(format("%d", DataModel::get(this)->getPlaceIDOrZeroInStudio()));
		assetId = ContentId(url); 
	}
	AnimationId animationId(assetId);
	if(animationId.isActive()){
		if(activeKeyframeTable.find(animationId.toString()) != activeKeyframeTable.end()){
			return activeKeyframeTable[animationId.toString()];
		}
		throw std::runtime_error(RBX::format("Unknown 'active' animation: '%s'", animationId.c_str()));
	}

	shared_ptr<KeyframeSequence> keyframeSequence;
	{
		boost::mutex::scoped_lock lock(keyframeSequenceCacheMutex);
		if(useCache && keyframeSequenceCache.fetch(assetId.toString(), &keyframeSequence)){
			return keyframeSequence;
		}

		//We don't have it loaded yet, so create a new one
		keyframeSequence = Creatable<Instance>::create<KeyframeSequence>();

		//Put it in the cache now, we'll "update" it when the data comes back
		keyframeSequenceCache.insert(assetId.toString(), keyframeSequence);
	}

	std::string newContext = "";
	if (DFFlag::AnimationFailedToLoadContext)
	{
		if (contextString != "")
		{
			newContext = contextString;
		}
		else if (contextInstance) 
		{
			newContext = contextInstance->getFullName();
		}
	}

	if (!DFFlag::DontUseInsertServiceOnAnimLoad && !blocking && (assetId.isHttp() || assetId.isAsset()))
	{
		RBXASSERT(!blocking);
		int assetId = -1;
		bool parsedAssetIdFromString = false;
		try
		{
			assetId = boost::lexical_cast<int>(assetIdNumber);
			parsedAssetIdFromString = true;
		}
		catch (...) {}
		if (parsedAssetIdFromString)
		{
			ServiceProvider::create<InsertService>(this)->loadAsset(assetId,
				boost::bind(&loadDataFromInsertService, _1,  weak_from(this), boost::weak_ptr<KeyframeSequence>(keyframeSequence), false, newContext),
				boost::bind(&doNothingErrorHandler, _1, newContext));
		}
	}
	else
	{
		if(blocking){
			std::auto_ptr<std::istream> stream = ServiceProvider::create<ContentProvider>(this)->getContent(assetId);
			SyncKeyframeLoaderHelper(AsyncHttpQueue::Succeeded, stream.get(), weak_from(this), keyframeSequence, newContext);
		}
		else{
			ServiceProvider::create<ContentProvider>(this)->getContent(assetId, ContentProvider::PRIORITY_ANIMATION, boost::bind(&AsyncKeyframeLoaderHelper, _1, _2, weak_from(this), boost::weak_ptr<KeyframeSequence>(keyframeSequence), newContext));
		}
	}

	return keyframeSequence;
}

void KeyframeSequenceProvider::JSONHttpHelper(const std::string* response, const std::exception* exception, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    RBX::DataModel::LegacyLock dmLock( RBX::DataModel::get(this), RBX::DataModelJob::Write);
    
    if(response && !exception)
    {
        std::stringstream stream;
        stream << (*response);
        
        shared_ptr<const Reflection::ValueTable> valueTable;
        
        try
        {
            WebParser::parseJSONTable(*response, valueTable);
        }
        catch(base_exception &e)
        {
            errorFunction( RBX::format("App:JSONHttpHelper error parsing web response: %s",e.what()) );
            return;
        }
        
        resumeFunction(valueTable);
    }
    else if(exception)
        errorFunction( RBX::format("App:JSONHttpHelper had an issue because %s",exception->what()) );
    else
        resumeFunction(shared_ptr<Reflection::ValueTable>());
}

void KeyframeSequenceProvider::getAnimations(int userId, int page, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(userId <= 0)
	{
		errorFunction("App:GetAnimations userId is not a positive number!");
		return;
	}

	if(page <= 0)
	{
		errorFunction("App:GetAnimations page is a non-positive number, please specify a positive number.");
		return;
	}

	std::string parameters =  RBX::format("ownership/assets?page=%d&userID=%d&assetTypeId=24",page,userId);

	RBX::Http getAnimationsHttp( (ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl() + parameters).c_str() );
#ifndef _WIN32
	getAnimationsHttp.setAuthDomain(ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl().c_str());
#endif
    getAnimationsHttp.get(boost::bind(&KeyframeSequenceProvider::JSONHttpHelper, this, _1, _2, resumeFunction, errorFunction));
}


}
