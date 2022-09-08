/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "Util/BinaryString.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#include <boost/unordered_map.hpp>
#include "Value.h"

namespace RBX
{

	class InstanceStringData
	{
	public:
		InstanceStringData(weak_ptr<BinaryStringValue> str)
			: ref(str)
			, count(1)
		{}

		InstanceStringData(weak_ptr<BinaryStringValue> str, int refCount)
			: ref(str)
			, count(refCount)
		{}

		weak_ptr<BinaryStringValue> ref;
		int count;
	};

	extern const char *const sFlyweightService;

	class FlyweightService
		: public DescribedCreatable<FlyweightService, Instance, sFlyweightService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox>
		, public Service
	{
	protected:
		typedef DescribedCreatable<FlyweightService, Instance, sFlyweightService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox> Super;
		typedef boost::unordered_map<std::string, InstanceStringData> FlyweightInstanceMap;

		FlyweightInstanceMap instanceMap;

		rbx::signals::scoped_connection stringChildAddedSignal;
		rbx::signals::scoped_connection stringChildRemovedSignal;

		virtual void onChildAdded(shared_ptr<RBX::Instance> childInstance);

		void storeStringData(BinaryString& str, bool forceIncrement, const std::string& name);
		void retrieveStringData(BinaryString& str);
		void incrementStringRefCounter(const BinaryString& str);

		std::string getLocalKeyHash(const std::string& str);
		std::string getLocalKeyHash(const BinaryString& str);

		virtual void refreshRefCountUnderInstance(RBX::Instance* instance) {}

		void cleanChildren();

		virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		bool isChildData(shared_ptr<RBX::Instance> childData);

	public:

		FlyweightService();

		void clean();
		virtual void refreshRefCount();
        
        static std::string createHashKey(const std::string& str);
		static std::string getHashKey(const std::string& str);

		static bool isHashKey(const std::string& str);

		std::string dataType(std::string str);

		const BinaryString peekAtData(const BinaryString& str);
        void removeStringData(const BinaryString& str);

		void printMapSizes();
	};
}
