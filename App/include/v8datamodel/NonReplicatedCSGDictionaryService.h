/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/BinaryString.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "V8DataModel/FlyweightService.h"

#include <boost/unordered_map.hpp>
#include "Value.h"

namespace RBX
{
	class PartOperation;

	extern const char *const sNonReplicatedCSGDictionaryService;

	class NonReplicatedCSGDictionaryService
		: public DescribedCreatable<NonReplicatedCSGDictionaryService, FlyweightService, sNonReplicatedCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox>
	{
	protected:
		typedef DescribedCreatable<NonReplicatedCSGDictionaryService, FlyweightService, sNonReplicatedCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox> Super;

		virtual void refreshRefCountUnderInstance(RBX::Instance* instance);

		void reparentChildData(shared_ptr<RBX::Instance> childInstance);

	public:

		NonReplicatedCSGDictionaryService();

		void storeData(PartOperation& partOperation, bool forceIncrement = false);
		void retrieveData(PartOperation& partOperation);

		void storeAllDescendants(shared_ptr<RBX::Instance> instance);
		void retrieveAllDescendants(shared_ptr<RBX::Instance> instance);
	};
}
