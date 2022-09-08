/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "v8datamodel/CacheableContentProvider.h"

namespace RBX {

	extern const char* const sSolidModelContentProvider;
	class SolidModelContentProvider
		: public DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
		typedef DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

	public:
		SolidModelContentProvider();
		~SolidModelContentProvider() {}

	private:
		virtual TaskScheduler::StepResult ProcessTask(const std::string& id, shared_ptr<const std::string> data);
		virtual void updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> mesh);

	};


} // namespace RBX