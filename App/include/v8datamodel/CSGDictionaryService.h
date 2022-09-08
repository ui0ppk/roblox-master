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
    class CSGMesh;

	extern const char *const sCSGDictionaryService;

	class CSGDictionaryService
		: public DescribedCreatable<CSGDictionaryService, FlyweightService, sCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox>
	{
	protected:
		typedef DescribedCreatable<CSGDictionaryService, FlyweightService, sCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT, Security::Roblox> Super;
		typedef boost::unordered_map<std::string, boost::shared_ptr<CSGMesh> > CSGMeshMap;
		CSGMeshMap cachedBREPMeshMap;
		CSGMeshMap cachedMeshMap;

		void reparentChildData(shared_ptr<RBX::Instance> sharedInstance);

		virtual void refreshRefCountUnderInstance(RBX::Instance* instance);

        boost::shared_ptr<CSGMesh> insertMesh(const std::string key, const RBX::BinaryString& meshData);

		boost::shared_ptr<CSGMesh> insertCachedMesh(const std::string key, const RBX::BinaryString& meshData);

        virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    public:

		CSGDictionaryService();

		void storeData(PartOperation& partOperation, bool forceIncrement = false);
		void retrieveData(PartOperation& partOperation);

		void storeAllDescendants(shared_ptr<RBX::Instance> instance);
		void retrieveAllDescendants(shared_ptr<RBX::Instance> instance);
		
		void reparentAllChildData();

		void insertMesh(PartOperation& partOperation);
        boost::shared_ptr<CSGMesh> getMesh(PartOperation& partOperation);

		boost::shared_ptr<CSGMesh> getCachedMesh(PartOperation& partOperation);

		void retrieveMeshData(PartOperation& partOperation);

		void storePhysicsData(PartOperation& partOperation, bool forceIncrement = false);
		void retrievePhysicsData(PartOperation& partOperation);

        void onWorkspaceLoaded();
	};
}
