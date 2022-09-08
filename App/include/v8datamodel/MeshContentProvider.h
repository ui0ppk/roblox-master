#pragma once

#include "v8datamodel/CacheableContentProvider.h"

namespace RBX {

	extern const char* const sMeshContentProvider;
	class MeshContentProvider 
		: public DescribedNonCreatable<MeshContentProvider, CacheableContentProvider, sMeshContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		typedef DescribedNonCreatable<MeshContentProvider, CacheableContentProvider, sMeshContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

	public:
		MeshContentProvider();
		~MeshContentProvider() {}

	private:
		virtual TaskScheduler::StepResult ProcessTask(const std::string& id, shared_ptr<const std::string> data);
		virtual void updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> mesh);

	};


} // namespace RBX