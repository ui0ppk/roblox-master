#pragma once
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/LRUCache.h"
#include "Util/ContentId.h"

namespace RBX {

	class KeyframeSequence;
	extern const char* const sKeyframeSequenceProvider;

	class KeyframeSequenceProvider
		: public DescribedNonCreatable<KeyframeSequenceProvider, Instance, sKeyframeSequenceProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL>
		, public Service
	{
	private:
		int activeKeyframeId;
		std::map<std::string, shared_ptr<KeyframeSequence> > activeKeyframeTable;

		typedef SizeEnforcedLRUCache<std::string, shared_ptr<KeyframeSequence> > KeyframeSequenceCache;

		boost::mutex keyframeSequenceCacheMutex;			//synchronizes the keyframeSequenceCache
		KeyframeSequenceCache keyframeSequenceCache;

		shared_ptr<KeyframeSequence> privateGetKeyframeSequence(ContentId assetId, bool blocking, bool useCache, const std::string& contextString = "", const Instance* contextInstance = NULL);
        
        void JSONHttpHelper(const std::string* response, const std::exception* exception, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);
	public:
		KeyframeSequenceProvider();

		ContentId registerKeyframeSequence(shared_ptr<Instance> keyframeSequence);
		ContentId registerActiveKeyframeSequence(shared_ptr<Instance> keyframeSequence);

		shared_ptr<Instance> getKeyframeSequenceLua(ContentId assetId);
		shared_ptr<Instance> getKeyframeSequenceByIdLua(int assetId, bool useCache);
		shared_ptr<const KeyframeSequence> getKeyframeSequence(ContentId assetId, const Instance* context);
		void getAnimations(int userId, int page, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);
	};
}
