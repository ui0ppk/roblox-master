/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "v8datamodel/SolidModelContentProvider.h"
#include "V8DataModel/PartOperationAsset.h"
#include "V8DataModel/CSGMesh.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/FlyweightService.h"
#include "V8DataModel/ContentProvider.h"
#include "V8Xml/SerializerBinary.h"

namespace RBX {

	const char* const sSolidModelContentProvider = "SolidModelContentProvider";
	SolidModelContentProvider::SolidModelContentProvider()
		:DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>(CACHE_ENFORCE_MEMORY_SIZE, 1024 * 1024 * 32)
	{
		setName(sSolidModelContentProvider);
	}

	TaskScheduler::StepResult SolidModelContentProvider::ProcessTask(const std::string& id, shared_ptr<const std::string> data)
	{
		if(data){
			boost::shared_ptr<CacheableContentProvider::CachedItem> solidModel(new CacheableContentProvider::CachedItem());

            RBX::Instances instances;
            std::stringstream ss(*data);
            SerializerBinary::deserialize(ss, instances);

            if (instances.size() > 0)
            {
                if (shared_ptr<RBX::PartOperationAsset> partOperationAsset = RBX::Instance::fastSharedDynamicCast<RBX::PartOperationAsset>(instances.front()))
                {
                    const BinaryString& meshString = partOperationAsset->getMeshData();
                    shared_ptr<CSGMesh> csgmesh(CSGMeshFactory::singleton()->createMesh());
                    csgmesh->fromBinaryString(meshString.value());
                    partOperationAsset->setRenderMesh(csgmesh);
                    solidModel->data = partOperationAsset;
                }
            }
            else
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "SolidModelContentProvider failed to process %s because 'could not fetch'", id.c_str());
                markContentFailed(id);
                return TaskScheduler::Stepped;
            }

			updateContent(id,solidModel);
            solidModel->requestResult = AsyncHttpQueue::Succeeded;

			--pendingRequests;
			return TaskScheduler::Stepped;
		}
		else
        {
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "SolidModelContentProvider failed to process %s because 'could not fetch'", id.c_str());
		}
		markContentFailed(id);
		return TaskScheduler::Stepped;
	}

	void SolidModelContentProvider::updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> solidModel)
	{
		if (solidModel->data)
		{
			boost::shared_ptr<RBX::PartOperationAsset> partOperationAsset = boost::static_pointer_cast<RBX::PartOperationAsset>(solidModel->data);
            boost::shared_ptr<CSGMesh> meshData = partOperationAsset->getRenderMesh();
            lruCache->insert(id, solidModel, meshData->getIndices().size() * sizeof(unsigned int) + meshData->getVertices().size() * sizeof(CSGVertex));
		}
		else
			lruCache->insert(id, solidModel, 0);
	}

}



