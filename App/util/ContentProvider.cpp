/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "v8datamodel/ContentProvider.h"
#include "Util/ScriptInformationProvider.h"
#include "RbxAssert.h"
#include "v8xml/serializer.h"
#include "v8xml/xmlserializer.h"
#include "Util/http.h"
#include <string.h>
#include "Util/StandardOut.h"
#include "Util/FileSystem.h"
#include "Util/ThreadPool.h"
#include "Util/Statistics.h"
#include "V8DataModel/DataModel.h"
#include "rbx/Crypt.h"
#include "StringConv.h"
#include "RobloxServicesTools.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "ATLPath.h"
#include "FastLog.h"
#endif

#include <fstream>
#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

// Prevent ContentFilter from calling the stats handler
// TODO: Filter even harder so that ContentFilter *only* calls asset handler

#define BOOST_DATE_TIME_NO_LIB
#include "boost/date_time/posix_time/posix_time.hpp"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

#ifdef RBX_TEST_BUILD
#include "Util/Statistics.h"
#endif

#include "XStudioBuild.h"

using namespace boost::posix_time;
namespace fs = boost::filesystem;

LOGGROUP(ContentProviderCleanup)
LOGGROUP(ContentProviderRequests)
LOGGROUP(ContentProviderRequestsFullContent)
FASTFLAGVARIABLE(NoCacheForLocalContent, false)
DYNAMIC_FASTINTVARIABLE(ContentProviderThreadPoolSize, 16)
DYNAMIC_FASTFLAGVARIABLE(ContentProviderHttpCaching, false)

DYNAMIC_FASTFLAGVARIABLE(ImageFailedToLoadContext, false)
DYNAMIC_FASTFLAG(UrlReconstructToAssetGame)

using namespace boost::posix_time;

#ifdef _WIN32
#define APPLOG(message) do { if (appLog != NULL) {RBX::mutex::scoped_lock lock(*appLogLock); appLog->writeEntry(RBX::Log::Information, message); } } while(0)
#else
#define APPLOG(message) {}
#endif

namespace {
    inline std::string pathToString(const fs::path &path)
    {
#ifdef _WIN32
		return RBX::utf8_encode(path.wstring());
#else
        return path.string();
#endif
    }

	inline fs::path stringToPath(const std::string& str)
	{
#ifdef _WIN32
		return RBX::utf8_decode(str.c_str());
#else
        return fs::path(str);
#endif
	}
}

namespace RBX {
	extern void ThrowLastError(const char* message);

	const char* const sContentProvider = "ContentProvider";

	Log *ContentProvider::appLog = NULL;
	RBX::mutex *ContentProvider::appLogLock = NULL;
    boost::mutex ContentProvider::preloadContentBlockingMutex;

	float ContentProvider::PRIORITY_MFC = 0;
	float ContentProvider::PRIORITY_SCRIPT = 10;
	float ContentProvider::PRIORITY_MESH = 20;
    float ContentProvider::PRIORITY_SOLIDMODEL = 25;
	float ContentProvider::PRIORITY_INSERT = 30;
	float ContentProvider::PRIORITY_CHARACTER = 40;
	float ContentProvider::PRIORITY_ANIMATION = 50;
	float ContentProvider::PRIORITY_TEXTURE = 55;
	float ContentProvider::PRIORITY_DECAL = 60;
	float ContentProvider::PRIORITY_DEFAULT = 70;
	float ContentProvider::PRIORITY_SOUND = 80;
	float ContentProvider::PRIORITY_GUI = 90;
	float ContentProvider::PRIORITY_SKY = 100;

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setBaseUrl( &ContentProvider::setBaseUrl, "SetBaseUrl", "url", Security::LocalUser);
	Reflection::PropDescriptor<ContentProvider, std::string> ContentProvider::desc_baseUrl("BaseUrl", category_Data, &ContentProvider::getBaseUrl, NULL);
	static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setAssetUrl( &ContentProvider::setBaseUrl, "SetAssetUrl", "url", Security::LocalUser);
	static Reflection::BoundFuncDesc<ContentProvider, void(int)> func_setThreadPool( &ContentProvider::setThreadPool, "SetThreadPool", "count", Security::LocalUser);
	static Reflection::BoundFuncDesc<ContentProvider, void(int)> func_setCacheSize( &ContentProvider::setCacheSize, "SetCacheSize", "count", Security::LocalUser);
	static Reflection::BoundFuncDesc<ContentProvider, void(ContentId)> func_preload( &ContentProvider::preloadContent, "Preload", "contentId", Security::None);
	static Reflection::BoundYieldFuncDesc<ContentProvider, void(shared_ptr<const Reflection::ValueArray>)> func_preloadAsync( &ContentProvider::preloadContentBlockingList, "PreloadAsync", "contentIdList", Security::None);

	static Reflection::PropDescriptor<ContentProvider, int> desc_requestQueueSize("RequestQueueSize", category_Data, &ContentProvider::getRequestQueueSize, NULL);
    REFLECTION_END();

	const std::string& ContentProvider::getBaseUrl() const
	{
		return baseUrl;
	}

	const std::string ContentProvider::getApiBaseUrl() const
	{
		return getApiBaseUrl(baseUrl);
	}

	const std::string ContentProvider::getUnsecureApiBaseUrl() const
	{
		return getUnsecureApiBaseUrl(baseUrl);
	}

	std::string ContentProvider::getApiBaseUrl(const std::string& baseUrl)
	{
		std::string apiBaseUrl = getUnsecureApiBaseUrl(baseUrl);
		if(!apiBaseUrl.empty())
		{
			std::size_t foundPos = apiBaseUrl.find("https");
			if(foundPos == std::string::npos)
			{
				foundPos = apiBaseUrl.find("http");
				if (foundPos != std::string::npos)
				{
					apiBaseUrl.replace(foundPos,4,"https");
				}
			}
            return apiBaseUrl;
		}

		return baseUrl;
	}

	std::string ContentProvider::getUnsecureApiBaseUrl(const std::string& baseUrl)
	{
		if(!baseUrl.empty())
		{
			return ReplaceTopSubdomain(baseUrl, "api");
		}

		return baseUrl;
	}

	
	void ContentProvider::setBaseUrl(std::string url)
	{   
		if (this->baseUrl != url)
		{
			this->baseUrl = url;
			raisePropertyChanged(desc_baseUrl);
		}
	}
	void ContentProvider::setThreadPool(int count)
	{
		contentCache->setThreadPool(count);
	}
	void ContentProvider::setCacheSize(int count)
	{
		contentCache->setCacheSize(count);
	}
	shared_ptr<const Reflection::ValueArray> ContentProvider::getFailedUrls()
	{
		return contentCache->getFailedUrls();
	}
	int ContentProvider::getRequestQueueSize() const
	{
		return contentCache->getRequestQueueSize();
	}
	shared_ptr<const Reflection::ValueArray> ContentProvider::getRequestQueueUrls()
	{
		return contentCache->getRequestQueueUrls();
	}
	shared_ptr<const Reflection::ValueArray> ContentProvider::getRequestedUrls()
	{
		return contentCache->getRequestedUrls();
	}

	bool ContentProvider::findLocalFile(const std::string& url, std::string* filename)
	{
		ContentId id(url);

		if (id.isAsset() || id.isAppContent())
		{
			*filename = findAsset(id);
			if (filename->empty())
				return false;
		}
		else if(!id.isHttp())
		{
			//This will only work if contendId is actually a hash
			*filename = findHashFile(id);

			if(filename->empty()){
				return false;
			}
		}

		return true;
	}
	
	ContentProvider::ContentProvider()
		: afm(NULL)
	{
		setName(sContentProvider);

		contentCache.reset(new AsyncHttpCache<CachedContent>(this,&findLocalFile, DFInt::ContentProviderThreadPoolSize, 1000));

        if (DFFlag::ContentProviderHttpCaching)
        {
            contentCache->setCachePolicy(HttpCache::PolicyFinalRedirect);
        }

	}

	ContentProvider::~ContentProvider()
	{
			RBX::FileSystem::clearCacheDirectory("ContentProvider");
		}

	void ContentProvider::verifyRequestedScriptSignature(const ProtectedString& source, const std::string& assetId, bool required)
	{
		const char* script = source.getSource().c_str();

		try
		{
			verifyScriptSignature(source, required);
            
            // search for the asset header
            // looks like "--rbxassetid%1818%"
            const char* assetHeader = "--rbxassetid%";
            const char* idInScript = strstr(script, assetHeader); // will find first occurrence, which should be safe
            if (idInScript)
            {
                idInScript += strlen(assetHeader); // now points to asset id in script, terminated by another %
                if (strncmp(idInScript, assetId.c_str(), assetId.length()) == 0 && // ensure match
                    idInScript[assetId.length()] == '%') // terminated here
                {
                    return; // correct
                }
                else
                {
                    throw std::runtime_error(""); // mismatch
                }
            }

			if (required)
			{
				throw std::runtime_error("");
			}
		}
		catch (RBX::base_exception&)
		{
			//Intentionally strip out the exception call stack location
			throw std::runtime_error("");
		}
	}

	void ContentProvider::verifyScriptSignature(const ProtectedString& source, bool required)
	{
		const char* script = source.getSource().c_str();

		try
		{
            // sig can be behind a Lua comment
            // looks like "--rbxsig%MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCtfLLFT36v5r9bNP7STBteDU5a%"
            const char* sigHeader = "--rbxsig%";
            if (strncmp(script, sigHeader, strlen(sigHeader)) == 0)
            {
                const char* sigStart = script + strlen(sigHeader);
                const char* sigEnd = strchr(sigStart, '%');
                if (!sigEnd)
                {
                    throw std::runtime_error("");
                }
                std::string signature(sigStart, sigEnd - sigStart);
                const char* signedScript = sigEnd + 1; // skip terminal %. we signed the text after this signature.

                // verify now!, will throw runtime_error
                Crypt().verifySignatureBase64(signedScript, signature);
                
                return;
            }
            
            if (required)
            {
                throw std::runtime_error("");
            }
		}
		catch (RBX::base_exception&)
		{
			//Intentionally strip out the exceptions
			throw std::runtime_error("");
		}
	}

	void ContentProvider::onHeartbeat(const Heartbeat& heartbeatEvent)
	{
		contentCache->onHeartbeat(heartbeatEvent);
	}

	void ContentProvider::setAssetFetchMediator(AssetFetchMediator* afm)
	{
		this->afm = afm;
	}

	std::string ContentProvider::getAssetFile(const char* filePath) {
		return std::string(assetFolder() + filePath);
	}

	bool ContentProvider::hasContent(const RBX::ContentId& id) {
		return isContentLoaded(id);
	}

	bool ContentProvider::isUrlBad(RBX::ContentId id) {
		id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
		id.convertToLegacyContent(baseUrl);
		return contentCache->isUrlBad(id.toString());
	}

	bool ContentProvider::isRequestQueueEmpty()
	{
		return contentCache->isRequestQueueEmpty();
	}

	bool ContentProvider::registerFile(const ContentId& id, CachedContent* item)
	{
		if (!item->filename)
		{
			std::istringstream ss(*item->data);
			ContentId hash = registerContent(ss);
			std::string filename = findHashFile(hash);
			if (filename.size()==0)
				return false;

			item->filename.reset(new std::string(filename));

			contentCache->insertCacheItem(id.c_str(),*item);
		}
		return true;
	}

	static void returnResponse(AsyncHttpQueue::RequestResult result, boost::function<void (AsyncHttpQueue::RequestResult)> callback)
	{
		callback(result);
	}

	static void parseString(
		const RBX::ContentId& id, 
		AsyncHttpQueue::RequestResult result, 
		std::istream* stream, 
		boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback, 
		shared_ptr<const std::string> response, 
		shared_ptr<std::exception> requestError)
	{
		shared_ptr<std::string> returnedResponse(new std::string());
		if (result==AsyncHttpQueue::Succeeded)
		{

			try{
				FASTLOGS(FLog::ContentProviderRequests, "Got content as string: %s", id.c_str());

				std::ostringstream strbuffer;
				strbuffer << stream->rdbuf();
				*returnedResponse = strbuffer.str();
			}
			catch(std::runtime_error e)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Content failed to load for %s because %s", id.c_str(), e.what());
				result = AsyncHttpQueue::Failed;
			}
		}

		callback(result, returnedResponse, requestError);
	}

	static void parseContent(
		const RBX::ContentId& id, 
		AsyncHttpQueue::RequestResult result, 
		std::istream* stream, 
		boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback, 
		shared_ptr<const std::string> response, 
		shared_ptr<std::exception> requestError)
	{
		shared_ptr<Instances> instances(new Instances());

		if (result==AsyncHttpQueue::Succeeded)
		{
			try
			{
				FASTLOGS(FLog::ContentProviderRequests, "Parsing content: %s", id.c_str());

				Serializer().loadInstances(*stream, *instances);
			}
			catch(std::runtime_error e)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Content failed to parse for %s because %s", id.c_str(), e.what());
				result = AsyncHttpQueue::Failed;
			}
		}

		callback(result, instances, requestError);
	}

	void ContentProvider::preloadContentWithCallback(RBX::ContentId id, float priority, boost::function<void (AsyncHttpQueue::RequestResult)> callback, AsyncHttpQueue::ResultJob jobType, const std::string& expectedType)
	{
		getContent(id, priority, boost::bind(&returnResponse, _1, callback), jobType, expectedType);
	}

	void ContentProvider::preloadContentResultCallback(AsyncHttpQueue::RequestResult results, ContentId id)
	{
		if (results != AsyncHttpQueue::Succeeded)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "ContentProvider:Preload() failed for %s", id.c_str());
		}
	}

	void ContentProvider::preloadContent(RBX::ContentId id)
	{
		if (DFFlag::ImageFailedToLoadContext) 
		{
			AsyncHttpQueue::RequestCallback callback =  boost::bind(&ContentProvider::preloadContentResultCallback, this, _1, id);
			AsyncHttpQueue::RequestResult requestResult = privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, &callback);
			if (requestResult != AsyncHttpQueue::Waiting)
				preloadContentResultCallback(requestResult, id);
		}
		else
		{
			privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, NULL);
		}
	}
	
	void ContentProvider::preloadContentBlockingListHelper(AsyncHttpQueue::RequestResult results, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction, PreloadAsyncRequest *pRequest, ContentId id)
	{
		boost::mutex::scoped_lock lock(preloadContentBlockingMutex);

		pRequest->outstanding--;
		if (results != AsyncHttpQueue::Succeeded)
		{
			if (DFFlag::ImageFailedToLoadContext) 
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "ContentProvider:PreloadAsync() failed for %s", id.c_str());
			}
			pRequest->failed++;
		}

		if (pRequest->outstanding == 0)
		{
			resumeFunction();
			delete pRequest;
		}
	}

	void ContentProvider::preloadContentBlockingList(shared_ptr<const Reflection::ValueArray> idList, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		const Reflection::ValueArray *pList = idList.get();

		if (pList && pList->size() > 0) 
		{
			for (size_t i = 0; i < pList->size(); i++)
			{
				Reflection::Variant v = (*pList)[i];
				ContentId id = v.get<RBX::ContentId>();

				if (!id.isNull() && !id.isAssetId() && !id.isHttp())
				{
					errorFunction("PreloadAsync: Bad format of asset " + v.get<std::string>());
					return;
				}
			}

			PreloadAsyncRequest *pRequest = new PreloadAsyncRequest(pList->size());
			for (size_t i = 0; i < pList->size(); i++)
			{
				Reflection::Variant v = (*pList)[i];
				ContentId id = v.get<RBX::ContentId>();
				AsyncHttpQueue::RequestCallback callback =  boost::bind(&ContentProvider::preloadContentBlockingListHelper, this, _1, resumeFunction, errorFunction, pRequest, id);
				AsyncHttpQueue::RequestResult requestResult = privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, &callback);
				if (requestResult != AsyncHttpQueue::Waiting)
					preloadContentBlockingListHelper(requestResult, resumeFunction, errorFunction, pRequest, id);
			}
		}
		else {
			resumeFunction();
		}
	}

    void ContentProvider::clearContent()
    {
        contentCache->clearCache();
    }

	void ContentProvider::invalidateCache(ContentId id)
	{
		id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
		id.convertToLegacyContent(baseUrl);
		contentCache->invalidateCacheItemOrFailure(id.toString());
	}

	void ContentProvider::loadContent(
		const RBX::ContentId& id,
		float priority, 
		boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback, 
		AsyncHttpQueue::ResultJob jobType)
	{
		getContent(id, priority, boost::bind(&parseContent, id, _1, _2, callback, _3, _4), jobType);
	}

	void ContentProvider::loadContentString(
		const RBX::ContentId& id, 
		float priority, 
		boost::function<void (AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback, 
		AsyncHttpQueue::ResultJob jobType)
	{
		getContent(id, priority, boost::bind(&parseString, id, _1, _2, callback, _3, _4), jobType);
	}

	static void InvokeFileCallback(AsyncHttpQueue::RequestCallback callback, boost::shared_ptr<const std::string> filename)
	{
		std::ifstream fileStream(utf8_decode(*filename).c_str(), std::ios_base::in | std::ios_base::binary);
        shared_ptr<std::exception> nullException;
		callback(AsyncHttpQueue::Succeeded, &fileStream, shared_ptr<const std::string>(), nullException);
	}

	void ContentProvider::getContent(
		const RBX::ContentId& id, 
		float priority, 
		AsyncHttpQueue::RequestCallback callback, 
		AsyncHttpQueue::ResultJob jobType, 
		const std::string &expectedType)
	{
		ContentId activeId = id;
		RBXASSERT(this!=NULL);

		CachedContent item;

		AsyncHttpQueue::RequestResult requestResult = privateLoadContent(activeId, AsyncHttpRequest, priority, &item, &callback, jobType, expectedType);
		switch(requestResult){
			case AsyncHttpQueue::Succeeded:
				if(item.data){
                    shared_ptr<std::exception> nullException;
					AsyncHttpQueue::dispatchCallback(callback, this, AsyncHttpQueue::Succeeded,item.data, jobType, nullException);
				}
				else{
					AsyncHttpQueue::dispatchGenericCallback(boost::bind(&InvokeFileCallback, callback, item.filename), this, jobType);
				}
				break;
			case AsyncHttpQueue::Failed:
				AsyncHttpQueue::dispatchCallback(callback, this, AsyncHttpQueue::Failed, boost::shared_ptr<const std::string>(new std::string("")), jobType, shared_ptr<std::exception>(new std::runtime_error("Temp read failed.")));
				break;
            default:
                break;
		}
	}

	shared_ptr<const std::string> ContentProvider::getContentString(RBX::ContentId id)
	{
		APPLOG("ContentProvider::getContentString");

		shared_ptr<const std::string> result = requestContentString(id, ContentProvider::PRIORITY_SCRIPT);
		if (!result)
		{
			CachedContent item;
			if (!blockingLoadContent(id, &item))
				throw RBX::runtime_error("Unable to load %s", id.c_str());

			result = requestContentString(id, ContentProvider::PRIORITY_SCRIPT);
			if (!result)
				throw RBX::runtime_error("Unable to retrieve cache of %s", id.c_str());
		}
		RBXASSERT(result);
		return result;
	}

	shared_ptr<const std::string> ContentProvider::requestContentString(const RBX::ContentId& id, float priority)
	{
		RBXASSERT(this!=NULL);
		APPLOG("ContentProvider::requestContentString - enter");
		RBX::ContentId activeId = id;

		CachedContent item;
		if (AsyncHttpQueue::Succeeded != privateLoadContent(activeId, AsyncHttpRequest, priority, &item, NULL))
			return shared_ptr<const std::string>();

		if (!item.data)
		{
			APPLOG("ContentProvider::requestContentString - putting data into cache");
            
			// Convert file to string data
            
			std::ifstream stream(utf8_decode(*item.filename).c_str(), std::ios_base::in | std::ios_base::binary);
			std::ostringstream data;
            
            #if (defined(_DEBUG) && defined(__APPLE__))
                #include "TargetConditionals.h"
                #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
                    data << stream.rdbuf(); // iOS in debug does not like boost copy here for some reason
                #else
                    boost::iostreams::copy(stream,data);
                #endif
            #else
                boost::iostreams::copy(stream,data);
            #endif
            
			item.data.reset(new std::string(data.str()));
            
            if(!FFlag::NoCacheForLocalContent)
                contentCache->insertCacheItem( activeId.c_str(), item );
		}
		APPLOG("ContentProvider::requestContentString - leave");
		return item.data;
	}

	bool ContentProvider::blockingLoadContent(ContentId id, CachedContent* result, const std::string& expectedType)
	{
		APPLOG(RBX::format("ContentProvider::requestContentString %s", id.c_str()).c_str());
		return privateLoadContent(id, SyncHttpRequest, -1, result, NULL, AsyncHttpQueue::AsyncInline, expectedType) == AsyncHttpQueue::Succeeded;
	}
	bool ContentProvider::isContentLoaded(ContentId id)
	{
		return privateLoadContent(id, NoHttpRequest, -1, NULL, NULL) == AsyncHttpQueue::Succeeded;
	}

	AsyncHttpQueue::RequestResult ContentProvider::privateLoadContent(RBX::ContentId& id, RequestType httpRequestType, float priority, CachedContent* result, AsyncHttpQueue::RequestCallback* callback, AsyncHttpQueue::ResultJob jobType, const std::string& expectedType)
	{
		APPLOG("ContentProvider::privateLoadContent");

		if (id.isNull())
			return AsyncHttpQueue::Failed;

		FASTLOGS(FLog::ContentProviderRequests, "Content requested: %s", id.c_str());

		if (afm)
		{
			if (boost::optional<std::string> content = afm->findCachedAssetOrEmpty(id, DataModel::get(this)->getUniverseId()))
			{
				*result = CachedContent(shared_ptr<std::string>(new std::string(content.get())), shared_ptr<std::string>());
				return AsyncHttpQueue::Succeeded;
			}
		}

		id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
		id.convertToLegacyContent(baseUrl);

		if (!id.reconstructAssetUrl(baseUrl))
			return AsyncHttpQueue::Failed;

		if (contentCache->findCacheItem(id.toString(), result))
		{
			APPLOG("ContentProvider::privateLoadContent - found item in cache");
			return AsyncHttpQueue::Succeeded;
		}
	
		if( httpRequestType != FullAsyncRequest )
		{
			APPLOG("ContentProvider::privateLoadContent - httpRequestType != FullAsyncRequest");
			if (id.isAsset())
			{
                
				shared_ptr<const std::string> filename(new std::string(findAsset(id)));
				if (filename->empty())
					return AsyncHttpQueue::Failed;
				contentCache->insertCacheItem(id.toString(), CachedContent(filename));
                return contentCache->findCacheItem(id.toString(), result)  ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
			}

#ifdef RBX_TEST_BUILD
			if (id.isFile())
			{
				const std::string cfilename = id.toString().substr(7);
				boost::filesystem::path path;

				if (boost::filesystem::path(cfilename).is_absolute())
				{
					path = cfilename;
				}
				else
				{
					boost::filesystem::path root = GetDefaultFilePath();
					boost::filesystem::path path = root / cfilename;

					struct stat buffer;
					// Try once to find the path.  If it fails, try once more with the
					// parent directory.  The logic for this is that RobloxStudio
					// will look for scripts relative to the RBXL, but RobloxTest
					// will look for scripts relative to the ProjectDir for RobloxTest,
					// which is one higher than the RBXL.  So, in the case of
					// RobloxStudio, we forcefully push one directory higher if the file
					// wasn't found next to the RBXL.
					if (-1 == stat(path.string().c_str(), &buffer))
					{
						path = root / ".." / cfilename;
					}
				}

				shared_ptr<const std::string> filename(new std::string(path.string()));
				contentCache->insertCacheItem(id.toString(), CachedContent(filename));
				return contentCache->findCacheItem(id.toString(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
			}
#endif

			// If content is not http, try seeing if it's hashed on disk (according to Erik, for legacy embedded content feature)
			if (!id.isHttp())
			{
				//This will only work if contendId is actually a hash
				shared_ptr<const std::string> filename(new std::string(findHashFile(id)));
				if (!filename->empty())
				{
					contentCache->insertCacheItem(id.toString(), CachedContent(filename));
					return contentCache->findCacheItem(id.toString(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
				}
			}
		}

		// If we get all the way here then try to request it for later use
		switch (httpRequestType)
		{
		case NoHttpRequest:
			break;
		case AsyncHttpRequest:
			if (!id.isHttp()){ 
				break;
			}
			//Fallthrough
		case FullAsyncRequest:
			{
				FASTLOGS(FLog::ContentProviderRequests, "Content requested: %s", id.c_str());
				APPLOG("ContentProvider::privateLoadContent - requestType = FullAsyncRequest");
				contentCache->asyncRequest(id.c_str(), priority, callback, jobType, false, expectedType);
				return AsyncHttpQueue::Waiting;
			}

		case SyncHttpRequest:
			{
				APPLOG("ContentProvider::privateLoadContent - requestType = SyncHttpRequest");
				if(contentCache->syncRequest(id.c_str(), expectedType)){
					return contentCache->findCacheItem(id.c_str(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Failed;
				}
				return AsyncHttpQueue::Waiting;
			}
		}

		return AsyncHttpQueue::Failed;
	}

	bool ContentProvider::isHttpUrl(const std::string& s)
	{
		if (s.find("http://")==0)
			return true;
		if (s.find("https://")==0)
			return true;
		return false;
	}
    
    bool ContentProvider::assetFolderAlreadyInit = false;
    fs::path ContentProvider::assetFolderPath;
    fs::path ContentProvider::platformAssetFolderPath;
    std::string ContentProvider::assetFolderString;
    std::string ContentProvider::platformAssetFolderString;

	std::string ContentProvider::assetFolder()
	{
		return assetFolderString;
	}

	std::string ContentProvider::platformAssetFolder()
	{
        return platformAssetFolderString;
	}

	bool ContentProvider::isUrl(const std::string& s)
	{
		if (isHttpUrl(s))
			return true;
		if (s.find("rbxasset://")==0 && s.length() > std::string("rbxasset://").length())
			return true;
        if (s.find("rbxassetid://")==0 && s.length() > std::string("rbxassetid://").length())
            return true;
		if (ContentId(s).isNamedAsset())
			return true;
		return false;
	}

	void ContentProvider::blockingLoadInstances(ContentId id, std::vector<shared_ptr<Instance> >& instances)
	{
		boost::shared_ptr<const std::string> str = getContentString(id);

		std::istringstream stream(*str);

		Serializer().loadInstances(stream, instances);
	}

	std::string ContentProvider::getFile(RBX::ContentId ticket)
	{
		CachedContent item;
		if (!blockingLoadContent(ticket, &item) || !registerFile(ticket, &item))
			throw RBX::runtime_error("Unable to load %s", ticket.c_str());

		return *item.filename;
	}

} // namespace



#include <fstream>
#include <sstream>
#include "Util/MD5Hasher.h"

namespace RBX
{
	static fs::path getCachePath(const char* subFolder, bool createPath)
	{
		// Returns something like "C:\Documents and Settings\All Users\Application Data\Roblox\Cache"
        if (createPath)
        {
            static const fs::path path = RBX::FileSystem::getCacheDirectory(true, subFolder);
            return path;
        }
        else
        {
            static const fs::path path = RBX::FileSystem::getCacheDirectory(false, subFolder);
            return path;
        }	
	}


	static void appendSlashIfRequired(fs::path& path)
	{
		std::string pathStr = path.string();
		if ( pathStr[pathStr.length() - 1] != '/' && pathStr[pathStr.length() - 1] != '\\') 
			path /= "/";		
	}
	
	static fs::path getLocalCachePath(bool createPath)
	{
        static rbx::atomic<int> caching;
		static bool isCached = false;
		static std::string cachedResult;
		
		if (isCached)
			return fs::path(cachedResult.c_str());

		fs::path path = getCachePath("ContentProvider", createPath);
        boost::system::error_code ec;
		if (!boost::filesystem::exists(path, ec))
			return "";
		
		appendSlashIfRequired(path);	

		if (caching.compare_and_swap(1, 0) == 0)
		{
			cachedResult = std::string(path.string());
			isCached = true;
		}
		return path;
	}

	RBX::ContentId ContentProvider::registerContent(std::istream& content)
	{
		RBX::ContentId contentId;
		{
			// Get the hash
			boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
			hasher->addData(content);
			contentId = RBX::ContentId(hasher->toString());
		}

		// Get/Create the local cache folder
		fs::path filePath = getLocalCachePath(true);
		filePath /= contentId.c_str();
        boost::system::error_code ec;
		if (!boost::filesystem::exists(filePath, ec))
		{
			// TODO: Threading risk here!!!!

			// Save it to the store
			std::ofstream outStream(filePath.string().c_str(), std::ios_base::out | std::ios::binary);

			content.clear();
			content.seekg( 0, std::ios_base::beg );
			do
			{
				char buffer[1024];
				content.read(buffer, 1024);
				outStream.write(buffer, content.gcount());
			}
			while (content.gcount()>0);
		}

		return contentId;
	}

    bool ContentProvider::isInSandbox(const fs::path& path, const fs::path& sandbox)
    {
        boost::system::error_code errBase, errPath;
        std::string canonicalPath = pathToString(fs::canonical(path,errPath));
        std::string canonicalBase = pathToString(fs::canonical(sandbox,errBase));
        if (errBase || errPath)
        {
            return false;
        }
        else if (canonicalPath.compare(0, canonicalBase.size(), canonicalBase) != 0)
        {
            return false;
        }
        return true;
    }

	std::string ContentProvider::findAsset(RBX::ContentId contentId)
	{
		RBXASSERT(contentId.isAsset() || contentId.isAppContent());
		const char* pathName = contentId.c_str();

        bool isPlatformAsset = true;
        fs::path queryName(pathName);
		fs::path filePath = platformAssetFolderPath;

		if (contentId.isAsset())
		{
			pathName += 11;
			filePath /= pathName;
		}
		else if (contentId.isAppContent())
		{
			pathName += 9;
			if ( strcmp(pathName, "xbox/localgamerpic") == 0 )
			{
				filePath = RBX::FileSystem::getUserDirectory(true, DirAppData, "");
				filePath /= "gamerpic.png";
			}
		}

		
        boost::system::error_code ec;
        if (!boost::filesystem::exists(filePath, ec))
		{
            isPlatformAsset = false;
			filePath = assetFolderPath / pathName;
            if (!boost::filesystem::exists(filePath, ec))
            {
				return "";
            }
		}

        // sandbox to assetFolder() or platformAssetFolder()
        if (contentId.isAppContent() || isInSandbox(filePath, (isPlatformAsset ? platformAssetFolderPath : assetFolderPath)))
        {
			return pathToString(filePath); 

        }

		return "";                               
	}

	std::string ContentProvider::findHashFile(RBX::ContentId contentId)
	{
		// TODO: Confirm that this is a valid "hash" ID

		// Get/Create the local cache folder
		fs::path cachePath = getLocalCachePath(true);
		// TODO: Handle case where cachePath==""

		fs::path filePath = cachePath;
		filePath /= contentId.c_str();

        if (!isInSandbox(filePath, cachePath))
        {
            return "";
        }
		
        boost::system::error_code ec;
        if (!boost::filesystem::exists(filePath, ec))
			return "";

		return pathToString(filePath);
	}

	void ContentProvider::setAssetFolder(const char* sPath)
	{
        if (!assetFolderAlreadyInit)
		{
            if (RBX::Log::current())
                RBX::Log::current()->writeEntry(Log::Information, RBX::format("setAssetFolder %s", sPath).c_str());

			fs::path path = fs::system_complete( stringToPath(sPath) );

            if (!fs::exists(path))
                throw RBX::runtime_error("The path '%s' does not exist", path.string().c_str());
            if (!is_directory(path))
                throw RBX::runtime_error("'%s' is not a directory", path.string().c_str());

            appendSlashIfRequired(path);

#if defined(RBX_PLATFORM_DURANGO)
            fs::path platformAssetFolderModifier = "../PlatformContent/durango/";
#elif defined(RBX_PLATFORM_IOS)
            fs::path platformAssetFolderModifier = "../ios/";
#elif defined(__APPLE__) || defined(_WIN32)
            fs::path platformAssetFolderModifier = "../PlatformContent/pc/";
#   if  ENABLE_XBOX_STUDIO_BUILD
            platformAssetFolderModifier = "../PlatformContent/Durango/";
#   endif
#elif defined(__ANDROID__)
            fs::path platformAssetFolderModifier = "../android/";
#else
#error Unsupported Platform
#endif

            assetFolderString = pathToString(path);
            platformAssetFolderString = assetFolderString + platformAssetFolderModifier.string();

            assetFolderPath = fs::canonical(path);
            platformAssetFolderPath = fs::canonical(path / platformAssetFolderModifier);
            assetFolderAlreadyInit = true;
        }
	}

	std::auto_ptr<std::istream> ContentProvider::getContent(const RBX::ContentId& ticket, const std::string& expectedType)
	{
		APPLOG("ContentProvider::getContent - ticket");

		CachedContent item;
		if (!blockingLoadContent(ticket, &item, expectedType))
			throw RBX::runtime_error("Unable to load %s", ticket.c_str());
		
		if (item.data)
		{
			return std::auto_ptr<std::istream>(new std::istringstream(*item.data));
		}
		else
		{
			std::ifstream* stream = new std::ifstream(utf8_decode(*item.filename).c_str(), std::ios_base::in | std::ios_base::binary);
			return std::auto_ptr<std::istream>(stream);
		}
	}
}
