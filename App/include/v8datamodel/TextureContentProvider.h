#pragma once

#ifndef _WIN32
#define HASH_STRING_DO_NOT_IMPLEMENT
#endif

#include "v8datamodel/CacheableContentProvider.h"
#include "GfxBase/Image.h"

namespace RBX {

	extern const char* const sTextureContentProvider;
	class TextureContentProvider 
		: public DescribedNonCreatable<TextureContentProvider, CacheableContentProvider, sTextureContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		typedef DescribedNonCreatable<TextureContentProvider, CacheableContentProvider, sTextureContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;
	
		boost::function<RBX::Image*(std::istream&, const std::string&)> mTextureAllocator;

	public:
		TextureContentProvider();
		~TextureContentProvider() {}

		void setTextureAllocator(boost::function<RBX::Image*(std::istream&, const std::string&)> textureAllocator);

	private:

		virtual TaskScheduler::StepResult ProcessTask(const std::string& id, shared_ptr<const std::string> data);
		virtual void updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> item);
	};


} // namespace RBX
