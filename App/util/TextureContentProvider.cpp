#include "stdafx.h"
#include "v8datamodel/TextureContentProvider.h"

LOGGROUP(TextureContentProvider);

namespace RBX {

	const char* const sTextureContentProvider = "TextureContentProvider";
	TextureContentProvider::TextureContentProvider()
		:DescribedNonCreatable<TextureContentProvider, CacheableContentProvider, sTextureContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>(CACHE_ENFORCE_MEMORY_SIZE, 1024 * 1024 * 32)
	{
		setName(sTextureContentProvider);
	}

	void TextureContentProvider::setTextureAllocator(boost::function<RBX::Image*(std::istream&, const std::string&)> textureAllocator)
	{
		mTextureAllocator = textureAllocator;
	}

	TaskScheduler::StepResult TextureContentProvider::ProcessTask(const std::string& id, shared_ptr<const std::string> data)
	{

		if(getContentStatus(id) != AsyncHttpQueue::Waiting) {
			//another callback took care of our loading, stop the requests
			--pendingRequests;
			return TaskScheduler::Stepped;
		}

		if(data){
			if(mTextureAllocator){
				std::istringstream f(*data);
				FASTLOGS(FLog::TextureContentProvider, "Allocating texture, content: %s", id);
				boost::shared_ptr<CacheableContentProvider::CachedItem> img(new CachedItem());
				img->data.reset(mTextureAllocator(f, id));
				img->requestResult = AsyncHttpQueue::Succeeded;

				FASTLOG2(FLog::TextureContentProvider, "Allocated, CachedImage: %p, RbxImage: %p", img.get(), img->data.get());
	
				updateContent(id, img);
				
				--pendingRequests;
				return TaskScheduler::Stepped;
			}
		}
		else{
			if(ContentId(id).isHttp())
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "TextureContentProvider failed to process %s because 'could not fetch'", id.c_str());
		}
		markContentFailed(id);
		return TaskScheduler::Stepped;
	}
	void TextureContentProvider::updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> item)
	{
		if (item->data)
		{
			boost::shared_ptr<Image> imgData = boost::static_pointer_cast<Image>(item->data);
			lruCache->insert(id, item, imgData->getSize());
		}
		else
			lruCache->insert(id, item, 0);	
	}

}



