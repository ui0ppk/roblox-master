#include "stdafx.h"
#include "Script/LuaSourceContainer.h"
#include "Network/Players.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "V8DataModel/ContentProvider.h"

DYNAMIC_LOGVARIABLE(PreloadLinkedScriptsTiming, 0)
DYNAMIC_FASTFLAGVARIABLE(RejectHashesInLinkedSource, false)

using namespace RBX;

const char* const RBX::sLuaSourceContainer = "LuaSourceContainer";

const Reflection::PropDescriptor<LuaSourceContainer, ProtectedString> prop_CachedRemoteSource(
	"CachedRemoteSource", category_Data,
	&LuaSourceContainer::getCachedRemoteSource, &LuaSourceContainer::setCachedRemoteSource,
	Reflection::PropertyDescriptor::REPLICATE_CLONE);

const Reflection::PropDescriptor<LuaSourceContainer, int> prop_CachedRemoteSourceLoadState(
	"CachedRemoteSourceLoadState", category_Data,
	&LuaSourceContainer::getCachedRemoteSourceLoadState, &LuaSourceContainer::setCachedRemoteSourceLoadState,
	Reflection::PropertyDescriptor::REPLICATE_CLONE);

Reflection::RemoteEventDesc<LuaSourceContainer, void()> LuaSourceContainer::event_requestLock(
	&LuaSourceContainer::requestLock, "RequestLock", Security::None,
	Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

const Reflection::RemoteEventDesc<LuaSourceContainer, void(bool)> event_lockGrantedOrNot(
	&LuaSourceContainer::lockGrantedOrNot, "LockGrantedOrNot", "granted", Security::None,
	Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

const Reflection::RefPropDescriptor<LuaSourceContainer, Instance> prop_CurrentEditor(
	"CurrentEditor", category_Data,
	&LuaSourceContainer::getCurrentEditor, &LuaSourceContainer::setCurrentEditor,
	Reflection::PropertyDescriptor::PUBLIC_REPLICATE);

namespace
{

LuaSourceContainer* isScriptWithLinkedSource(shared_ptr<Instance> descendant, ContentId* out)
{
	if (LuaSourceContainer* baseScript = Instance::fastDynamicCast<LuaSourceContainer>(descendant.get()))
	{
		if (!baseScript->getScriptId().isNull())
		{
			*out = baseScript->getScriptId();
			return baseScript;
		}
	}
	return NULL;
}

void wrap(DataModel* dm, boost::function<void()> callback)
{
	callback();
}

}

void LuaSourceContainer::loadLinkedScripts(shared_ptr<ContentProvider> cp, Instance* root, AsyncHttpQueue::ResultJob jobType,
	boost::function<void()> callback)
{
	shared_ptr<LinkedScriptLoadData> loadData(new LinkedScriptLoadData);
	FASTLOG1(DFLog::PreloadLinkedScriptsTiming, "Starting to preload linked scripts id=%p", loadData.get());
	int counter = 0;
	root->visitDescendants(boost::bind(&LuaSourceContainer::linkedSourceCountingVisitor, _1, &counter));
	FASTLOG2(DFLog::PreloadLinkedScriptsTiming, "Done counting linked scripts id=%p count=%d",
		loadData.get(), counter);
	if (counter > 0)
	{
		loadData->scriptCount = counter;
		loadData->callbackWhenDone = callback;
		loadData->context = cp;
		loadData->jobType = jobType;
		root->visitDescendants(boost::bind(&LuaSourceContainer::linkedSourceFetchingVisitor, _1, cp, jobType, loadData));
	}
	else
	{
		AsyncHttpQueue::dispatchGenericCallback(boost::bind(&wrap, _1, callback), cp.get(), jobType);
	}
}

void LuaSourceContainer::loadLinkedScriptsForInstances(shared_ptr<ContentProvider> cp, Instances& instances, AsyncHttpQueue::ResultJob jobType,
	boost::function<void()> callback)
{
	shared_ptr<LinkedScriptLoadData> loadData(new LinkedScriptLoadData);
	FASTLOG1(DFLog::PreloadLinkedScriptsTiming, "Starting to preload linked scripts id=%p", loadData.get());
	int counter = 0;
	for (Instances::iterator itr = instances.begin(); itr != instances.end(); ++itr)
	{
		(*itr)->visitDescendants(boost::bind(&LuaSourceContainer::linkedSourceCountingVisitor, _1, &counter));
	}
	FASTLOG2(DFLog::PreloadLinkedScriptsTiming, "Done counting linked scripts id=%p count=%d",
		loadData.get(), counter);
	if (counter > 0)
	{
		loadData->scriptCount = counter;
		loadData->callbackWhenDone = callback;
		loadData->context = cp;
		loadData->jobType = jobType;
		for (Instances::iterator itr = instances.begin(); itr != instances.end(); ++itr)
		{
			(*itr)->visitDescendants(boost::bind(&LuaSourceContainer::linkedSourceFetchingVisitor, _1, cp, jobType, loadData));
		}
	}
	else
	{
		AsyncHttpQueue::dispatchGenericCallback(boost::bind(&wrap, _1, callback), cp.get(), jobType);
	}
}

void LuaSourceContainer::blockingLoadLinkedScripts(ContentProvider* cp, Instance* root)
{
	CEvent event(true);
	loadLinkedScripts(shared_from(cp), root, AsyncHttpQueue::AsyncInline, boost::bind(&CEvent::Set, &event));
	event.Wait();
}

void LuaSourceContainer::blockingLoadLinkedScriptsForInstances(ContentProvider* cp, Instances& instances)
{
	CEvent event(true);
	loadLinkedScriptsForInstances(shared_from(cp), boost::ref(instances), AsyncHttpQueue::AsyncInline, boost::bind(&CEvent::Set, &event));
	event.Wait();
}

LuaSourceContainer::LuaSourceContainer()
	: cachedRemoteSourceLoadState(NotAttemptedToLoad)
	, currentEditor()
{
}

const ContentId& LuaSourceContainer::getScriptId() const
{
	return scriptId;
}

void LuaSourceContainer::setScriptId(const ContentId& contentId)
{
	if (contentId != scriptId)
	{
		scriptId = contentId;
		onScriptIdChanged();
	}
}

const ProtectedString& LuaSourceContainer::getCachedRemoteSource() const
{
	return cachedRemoteSource;
}

void LuaSourceContainer::setCachedRemoteSource(const ProtectedString& value)
{
	if (value != cachedRemoteSource)
	{
		cachedRemoteSource = value;
		raiseChanged(prop_CachedRemoteSource);
	}
}

int LuaSourceContainer::getCachedRemoteSourceLoadState() const
{
	return cachedRemoteSourceLoadState;
}

void LuaSourceContainer::setCachedRemoteSourceLoadState(int value)
{
	RemoteSourceLoadState newLoadState = (RemoteSourceLoadState)value;
	if (newLoadState != cachedRemoteSourceLoadState)
	{
		cachedRemoteSourceLoadState = newLoadState;
		raiseChanged(prop_CachedRemoteSourceLoadState);
	}
}

Instance* LuaSourceContainer::getCurrentEditor() const
{
	return currentEditor.lock().get();
}

void LuaSourceContainer::setCurrentEditor(Instance* newEditor)
{
	shared_ptr<Instance> editor = currentEditor.lock();
	if (editor.get() != newEditor)
	{
		currentEditor = weak_from(newEditor);
		raiseChanged(prop_CurrentEditor);
	}
}

void LuaSourceContainer::processRemoteEvent(const Reflection::EventDescriptor& descriptor,
	const Reflection::EventArguments& args, const SystemAddress& source)
{
	if (descriptor == event_requestLock)
	{
		shared_ptr<Network::Player> player = Network::Players::findPlayerWithAddress(source, this);
		shared_ptr<Instance> editor = currentEditor.lock();
		bool granted = player != NULL && editor == NULL;
		if (granted)
		{
			setCurrentEditor(player.get());
		}
		Reflection::EventArguments args(1);
		args[0] = granted;
		raiseEventInvocation(event_lockGrantedOrNot, args, &source);
	}
	else
	{
		Instance::processRemoteEvent(descriptor, args, source);
	}
}

void LuaSourceContainer::linkedSourceCountingVisitor(shared_ptr<Instance> descendant, int* counter)
{
	ContentId out;
	if (isScriptWithLinkedSource(descendant, &out))
	{
		(*counter)++;
	}
}

static void applyLinkedScriptResult(weak_ptr<LuaSourceContainer> weakScript,
	AsyncHttpQueue::RequestResult result, shared_ptr<const std::string> loadedSource)
{
	if (shared_ptr<LuaSourceContainer> script = weakScript.lock())
	{
		if (result == AsyncHttpQueue::Succeeded)
		{
			script->setCachedRemoteSourceLoadState(LuaSourceContainer::Loaded);
			script->setCachedRemoteSource(ProtectedString::fromTrustedSource(*loadedSource));
		}
		else
		{
			script->setCachedRemoteSourceLoadState(LuaSourceContainer::FailedToLoad);
			StandardOut::singleton()->printf(MESSAGE_ERROR, "Failed to load source (%s) for %s. The LinkedSource name may have changed or been removed.",
				script->getScriptId().c_str(), script->getFullName().c_str());
		}
	}
}

void LuaSourceContainer::linkedSourceLoadedHandler(weak_ptr<LuaSourceContainer> weakScript,
	AsyncHttpQueue::RequestResult result, shared_ptr<const std::string> loadedSource,
	shared_ptr<LinkedScriptLoadData> metadata)
{
	int completedScripts;
	{
		boost::mutex::scoped_lock l(metadata->scriptApplyResultClosuresMutex);
		metadata->scriptApplyResultClosures.push_back(boost::bind(&applyLinkedScriptResult,
			weakScript, result, loadedSource));
		completedScripts = metadata->scriptApplyResultClosures.size();
	}

	if (completedScripts == metadata->scriptCount)
	{
		AsyncHttpQueue::dispatchGenericCallback(boost::bind(
			&LuaSourceContainer::updateScriptInstancesUnderWriteLock, _1, metadata),
			metadata->context.get(), metadata->jobType);
	}
}

void LuaSourceContainer::updateScriptInstancesUnderWriteLock(DataModel* /*unused*/,
	shared_ptr<LinkedScriptLoadData> metadata)
{
	FASTLOG1(DFLog::PreloadLinkedScriptsTiming, "Done preloading linked scripts id=%p", metadata.get());
	DataModel* dm = DataModel::get(metadata->context.get());
	boost::scoped_ptr<DataModel::scoped_write_transfer> writeTransfer;
	if (!dm->currentThreadHasWriteLock())
	{
		writeTransfer.reset(new DataModel::scoped_write_transfer(dm));
	}
	{
		// This mutex lock isn't strictly necessary, this function is expected to only
		// be run after all scripts have been loaded/failed to load.
		boost::mutex::scoped_lock l(metadata->scriptApplyResultClosuresMutex);
		for (auto closure : metadata->scriptApplyResultClosures)
		{
			closure();
		}
	}
	metadata->callbackWhenDone();
}

static void reportNamedLinkedSourceUsage(DataModel* dm)
{
	std::string placeIdStr = format("%d", dm->getPlaceID());
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "Load Named LinkedSource",
		placeIdStr.c_str());
}

void LuaSourceContainer::linkedSourceFetchingVisitor(shared_ptr<Instance> descendant, shared_ptr<ContentProvider> cp,
	AsyncHttpQueue::ResultJob jobType, shared_ptr<LinkedScriptLoadData> metadata)
{
	ContentId out;
	if (LuaSourceContainer* LuaSourceContainer = isScriptWithLinkedSource(descendant, &out))
	{
		if (DataModel* dm = DataModel::get(cp.get()))
		{
			if (out.isNamedAsset())
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&reportNamedLinkedSourceUsage, dm));
			}
			out.convertAssetId(cp->getBaseUrl(), dm->getUniverseId());
			out.convertToLegacyContent(cp->getBaseUrl());

			if (out.isHttp())
			{
				// Detect if user is trying to override server place id url parameter.
				// This check is overly zealous, we can revise it if we get complaints.
				std::string lowerVersion = out.toString();
				std::transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), &::tolower);
				if (lowerVersion.find("serverplaceid=") != std::string::npos || (DFFlag::RejectHashesInLinkedSource && lowerVersion.find("#") != std::string::npos))
				{
					linkedSourceLoadedHandler(weak_from(LuaSourceContainer), AsyncHttpQueue::Failed,
						shared_ptr<const std::string>(), metadata);
					return;
				}
				else
				{
					out = ContentId(out.toString() + format("&serverplaceid=%d", dm->getPlaceID()));
				}
			}

			cp->loadContentString(out, ContentProvider::PRIORITY_SCRIPT,
				boost::bind(&LuaSourceContainer::linkedSourceLoadedHandler, weak_from(LuaSourceContainer), _1, _2, metadata),
				AsyncHttpQueue::AsyncInline);
		}
	}
}
