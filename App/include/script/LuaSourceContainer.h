#pragma once

#include "Util/AsyncHttpQueue.h"
#include "Util/ContentId.h"
#include "Util/ProtectedString.h"
#include "V8Tree/Instance.h"

namespace RBX
{

class ContentProvider;

extern const char* const sLuaSourceContainer;
class LuaSourceContainer
	: public DescribedNonCreatable<LuaSourceContainer, Instance, sLuaSourceContainer>
{
public:
	enum RemoteSourceLoadState
	{
		NotAttemptedToLoad,
		Loaded,
		FailedToLoad
	};

	static void loadLinkedScripts(shared_ptr<ContentProvider> cp, Instance* root, AsyncHttpQueue::ResultJob jobType, boost::function<void()> callback);
	static void loadLinkedScriptsForInstances(shared_ptr<ContentProvider> cp, Instances& instances, AsyncHttpQueue::ResultJob jobType, boost::function<void()> callback);
	static void blockingLoadLinkedScripts(ContentProvider* cp, Instance* root);
	static void blockingLoadLinkedScriptsForInstances(ContentProvider* cp, Instances& instances);
	static Reflection::RemoteEventDesc<LuaSourceContainer, void()> event_requestLock;

	LuaSourceContainer();

	const ContentId& getScriptId() const;
	void setScriptId(const ContentId& contentId);
	const ProtectedString& getCachedRemoteSource() const;
	void setCachedRemoteSource(const ProtectedString& value);
	int getCachedRemoteSourceLoadState() const;
	void setCachedRemoteSourceLoadState(int value);
	Instance* getCurrentEditor() const;
	void setCurrentEditor(Instance* newEditor);
	virtual void fireSourceChanged() {};

	rbx::remote_signal<void()> requestLock;
	rbx::remote_signal<void(bool)> lockGrantedOrNot;

protected:
	virtual void onScriptIdChanged() {}
	void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source) override;

private:
	struct LinkedScriptLoadData
	{
		rbx::atomic<int> scriptCount;
		boost::function<void()> callbackWhenDone;
		shared_ptr<Instance> context;
		AsyncHttpQueue::ResultJob jobType;

		boost::mutex scriptApplyResultClosuresMutex;
		std::vector<boost::function<void()> > scriptApplyResultClosures;
	};

	static void linkedSourceCountingVisitor(shared_ptr<Instance> descendant, int* counter);
	static void linkedSourceLoadedHandler(weak_ptr<LuaSourceContainer> weakScript, AsyncHttpQueue::RequestResult result,
		shared_ptr<const std::string> loadedSource, shared_ptr<LinkedScriptLoadData> metadata);
	static void updateScriptInstancesUnderWriteLock(DataModel* dm, shared_ptr<LinkedScriptLoadData> metadata);
	static void linkedSourceFetchingVisitor(shared_ptr<Instance> descendant, shared_ptr<ContentProvider> cp,
		AsyncHttpQueue::ResultJob jobType, shared_ptr<LinkedScriptLoadData> metadata);

	ContentId scriptId;
	ProtectedString cachedRemoteSource;
	RemoteSourceLoadState cachedRemoteSourceLoadState;
	weak_ptr<Instance> currentEditor;
};
}