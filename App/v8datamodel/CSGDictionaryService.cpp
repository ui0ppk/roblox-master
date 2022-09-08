/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/CSGDictionaryService.h"
#include "../App/include/v8datamodel/PartOperation.h"
#include "v8datamodel/Value.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"

FASTFLAG(StudioCSGAssets)
FASTFLAG(CSGLoadFromCDN)
FASTFLAG(IgnoreBlankDataOnStore)

namespace {

const std::string localKeyTag("CSGK");
const size_t minKeySize = 4;

} 

namespace RBX
{
	const char *const sCSGDictionaryService = "CSGDictionaryService";

	CSGDictionaryService::CSGDictionaryService()
	{
		setName("CSGDictionaryService");
	}

	void CSGDictionaryService::reparentAllChildData()
	{
		if (numChildren() <= 0)
			return;

		std::vector<shared_ptr<RBX::Instance> > children;

		children.insert(children.begin(), getChildren()->begin(), getChildren()->end());

		for (std::vector<shared_ptr<RBX::Instance> >::const_iterator iter = children.begin(); iter != children.end(); ++iter)
			reparentChildData(*iter);
	}

	void CSGDictionaryService::reparentChildData(shared_ptr<RBX::Instance> childInstance)
	{
		if (!isChildData(childInstance))
			return;

		NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(DataModel::get(this));
		childInstance->setParent(nrDictionaryService);

		if (shared_ptr<RBX::BinaryStringValue> bStrValue = RBX::Instance::fastSharedDynamicCast<RBX::BinaryStringValue>(childInstance))
		{
			std::string key = createHashKey(bStrValue->getValue().value());
			instanceMap.erase(key);
		}
	}

	void CSGDictionaryService::storeData(PartOperation& partOperation, bool forceIncrement)
	{
		BinaryString tmpString;

		tmpString = partOperation.getMeshData();
		if (FFlag::IgnoreBlankDataOnStore && tmpString.value().size() > 0 )
		{
			storeStringData(tmpString, forceIncrement, "MeshData");
			partOperation.setMeshData(tmpString);
		}

		tmpString = partOperation.getPhysicsData();
        if ( FFlag::IgnoreBlankDataOnStore && tmpString.value().size() > 0 )
        {
		    storeStringData(tmpString, forceIncrement, "PhysicsData");
		}
        partOperation.setPhysicsData(tmpString);
	}

	void CSGDictionaryService::retrieveData(PartOperation& partOperation)
	{
		BinaryString tmpString;

		tmpString = partOperation.getMeshData();
		retrieveStringData(tmpString);
		partOperation.setMeshData(tmpString);

		tmpString = partOperation.getPhysicsData();
		retrieveStringData(tmpString);
		partOperation.setPhysicsData(tmpString);
	}

	void CSGDictionaryService::storeAllDescendants(shared_ptr<Instance> instance)
	{
		if (instance->getChildren())
			for (RBX::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
				storeAllDescendants(*iter);

		if (shared_ptr<PartOperation> childOperation = RBX::Instance::fastSharedDynamicCast<PartOperation>(instance))
			storeData(*childOperation);
	}

	void CSGDictionaryService::retrieveAllDescendants(shared_ptr<Instance> instance)
	{
		if (instance->getChildren())
			for (RBX::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
				retrieveAllDescendants(*iter);

		if (shared_ptr<PartOperation> childOperation = RBX::Instance::fastSharedDynamicCast<PartOperation>(instance))
			retrieveData(*childOperation);
	}

	void CSGDictionaryService::refreshRefCountUnderInstance(RBX::Instance* instance)
	{
		if (RBX::PartOperation* partOperation = RBX::Instance::fastDynamicCast<RBX::PartOperation>(instance))
			storeData(*partOperation, true);

		if (instance->getChildren())
			for (RBX::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
				refreshRefCountUnderInstance(iter->get());
	}

    boost::shared_ptr<CSGMesh> CSGDictionaryService::insertMesh(const std::string key, const RBX::BinaryString& meshData)
    {
        shared_ptr<CSGMesh> mesh = shared_ptr<CSGMesh>(CSGMeshFactory::singleton()->createMesh());
        mesh->fromBinaryString(meshData.value());
		cachedMeshMap.insert(std::make_pair(key, mesh));
        return cachedMeshMap.at(key);
    }

	void CSGDictionaryService::insertMesh(PartOperation& partOperation)
	{
		const BinaryString& keyIn = partOperation.getMeshData();

		if (keyIn.value().size() > minKeySize)
		{
			const std::string key = getLocalKeyHash(keyIn);
			if (strncmp(keyIn.value().c_str(), localKeyTag.c_str(), minKeySize) == 0)
			{
				if (!cachedBREPMeshMap.count(key))
				{
					shared_ptr<CSGMesh> mesh = partOperation.getMesh();
					cachedBREPMeshMap.insert(std::make_pair(key, mesh));
					if (cachedMeshMap.count(key))
						cachedMeshMap.erase(key);
				}
			}
		}
	}

    boost::shared_ptr<CSGMesh> CSGDictionaryService::getMesh(PartOperation& partOperation)
    {
        const BinaryString& keyIn = partOperation.getMeshData();

   		if (keyIn.value().size() > minKeySize)
		{
            std::string key = getLocalKeyHash(keyIn);
			
			if (strncmp(keyIn.value().c_str(), localKeyTag.c_str(), minKeySize) == 0)
			{
				if (cachedBREPMeshMap.count(key))
				{
					return cachedBREPMeshMap.at(key);
				}
				else if (cachedMeshMap.count(key))
				{
					return cachedMeshMap.at(key);
				}
				else if (instanceMap.count(key))
				{
					if (shared_ptr<RBX::BinaryStringValue> meshData = instanceMap.at(key).ref.lock())
						return insertMesh(key, meshData->getValue());
				}
			}
        }
        return  boost::shared_ptr<CSGMesh>();
    }

	void CSGDictionaryService::retrieveMeshData(PartOperation& partOperation)
	{
		BinaryString tmpString = partOperation.getMeshData();
		retrieveStringData(tmpString);
		partOperation.setMeshData(tmpString);
	}

	void CSGDictionaryService::storePhysicsData(PartOperation& partOperation, bool forceIncrement)
	{
		BinaryString tmpString = partOperation.getPhysicsData();
		storeStringData(tmpString, forceIncrement, "PhysicsData");
		partOperation.setPhysicsData(tmpString);
	}

	void CSGDictionaryService::retrievePhysicsData(PartOperation& partOperation)
	{
		BinaryString tmpString = partOperation.getPhysicsData();
		retrieveStringData(tmpString);
		partOperation.setPhysicsData(tmpString);
	}

    void CSGDictionaryService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
        FlyweightService::onServiceProvider(oldProvider, newProvider);

        if (FFlag::CSGLoadFromCDN && !oldProvider && newProvider)
        {
            DataModel* dataModel = DataModel::get(this);

            if (!dataModel)
                return;

            dataModel->workspaceLoadedSignal.connect(boost::bind(&CSGDictionaryService::onWorkspaceLoaded, shared_from(this)));
        }
    }

    void findAndStripMeshData(std::set<BinaryString>* keys, shared_ptr<Instance> descendant)
    {
        if(shared_ptr<PartOperation> partOperation = Instance::fastSharedDynamicCast<PartOperation>(descendant))
        {
            if (partOperation->hasAsset() &&
                partOperation->getMeshData().value().size() > 0)
            {
                keys->insert(partOperation->getMeshData());
                partOperation->setMeshData(BinaryString());
            }
        }
    }

    void CSGDictionaryService::onWorkspaceLoaded()
    {
        std::set<BinaryString> keys;

        DataModel* dataModel = DataModel::get(this);

        if (!dataModel)
            return;

        dataModel->visitDescendants(boost::bind(&findAndStripMeshData, &keys, _1));

        for (std::set<BinaryString>::iterator iter = keys.begin();
             iter != keys.end();
             ++iter)
        {
            removeStringData((*iter));
        }
    }

}
