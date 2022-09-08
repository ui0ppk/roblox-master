/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PartOperationAsset.h"
#include "V8DataModel/PartOperation.h"
#include "V8DataModel/GameBasicSettings.h"

#include "G3D/G3dMath.h"
#include "G3D/CollisionDetection.h"
#include "Network/Players.h"
#include "Reflection/Reflection.h"
#include "RBX/RbxTime.h"
#include "Util/BinaryString.h"
#include "Util/NormalId.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/SurfaceType.h"
#include "Util/stringbuffer.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/CSGMesh.h"
#include "V8World/ContactManager.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/BulletGeometryPoolObjects.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/FlyweightService.h"
#include "V8World/TriangleMesh.h"
#include "V8DataModel/ContentProvider.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/SerializerBinary.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/SolidModelContentProvider.h"

FASTFLAGVARIABLE(CSGFixForNoChildData, true)

namespace RBX
{
using namespace Reflection;

const Reflection::PropDescriptor<PartOperationAsset, BinaryString> PartOperationAsset::desc_ChildData("ChildData", category_Data, &PartOperationAsset::getChildData, &PartOperationAsset::setChildData, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
const Reflection::PropDescriptor<PartOperationAsset, BinaryString> PartOperationAsset::desc_MeshData("MeshData", category_Data, &PartOperationAsset::getMeshData, &PartOperationAsset::setMeshData, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);

const char* const sPartOperationAsset = "PartOperationAsset";

void setAssetOnMatchingPartOperations(shared_ptr<Instance> descendant, const ContentId& url, const BinaryString& key)
{
    if(PartOperation* partOperation = RBX::Instance::fastDynamicCast<PartOperation>(descendant.get()))
    {
        if (partOperation->hasAsset())
            return;

        if (partOperation->getChildData() == key)
        {
            partOperation->setAssetId(url);

			BinaryString noValue;
			partOperation->setChildData(noValue);
			partOperation->setMeshData(noValue);
        }
    }
}

void publishPartOperations(shared_ptr<Instance> descendant, RBX::Time startTime, const int timeoutMills)
{
    if (timeoutMills != -1 && (startTime - RBX::Time::nowFast()).msec() > timeoutMills)
        return;

    if(shared_ptr<PartOperation> partOperation = Instance::fastSharedDynamicCast<PartOperation>(descendant))
    {
        if (partOperation->hasAsset())
		{
			const BinaryString childKey = partOperation->getChildData();
			const BinaryString meshKey = partOperation->getMeshData();
			
			if (meshKey.value().empty() && childKey.value().empty())
				return;

			RBX::ContentId contentId = partOperation->getAssetId();
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "RemoveLeftoverCSGData", contentId.c_str());

			DataModel* dataModel = DataModel::get(partOperation.get());

			if (CacheableContentProvider* mcp = ServiceProvider::create<SolidModelContentProvider>(dataModel))
			{
				if (boost::shared_ptr<PartOperationAsset> partOperationAsset = boost::static_pointer_cast<PartOperationAsset>(mcp->blockingRequestContent(contentId, true)))
				{
					//ChildData
					if (!childKey.value().empty() && !partOperationAsset->getChildData().value().empty())
					{
						NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::find<NonReplicatedCSGDictionaryService>(dataModel);

						if (nrDictionaryService->isHashKey(childKey.value()))
							nrDictionaryService->retrieveData(*partOperation);

						partOperation->setChildData(BinaryString(""));
					}

					//MeshData
					if (!meshKey.value().empty() && !partOperationAsset->getMeshData().value().empty())
					{
						CSGDictionaryService* dictionaryService = ServiceProvider::find< CSGDictionaryService >(dataModel);

						if (dictionaryService->isHashKey(meshKey.value()))
							dictionaryService->retrieveMeshData(*partOperation);

						partOperation->setMeshData(BinaryString(""));
					}
				}
			}

			return;
		}

        DataModel* dataModel = DataModel::get(partOperation.get());

        CSGDictionaryService* dictionaryService = ServiceProvider::create< CSGDictionaryService >(dataModel);
        NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(dataModel);
        ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(dataModel);

        const BinaryString meshKey = partOperation->getMeshData();
        const BinaryString childKey = partOperation->getChildData();

        if (meshKey.value().empty() || childKey.value().empty())
            return;

        if (!dictionaryService->isHashKey(meshKey.value()) || !nrDictionaryService->isHashKey(childKey.value()))
        {
            RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "PublishCSGFailure", "HashKeyNotFound");
            return;
        }

        const BinaryString meshData = dictionaryService->peekAtData(meshKey);
        BinaryString childData = nrDictionaryService->peekAtData(childKey);

        if (childData.value().empty())
            childData = dictionaryService->peekAtData(childKey);
        bool validChildData = !childData.value().empty();

        shared_ptr<PartOperationAsset> partOperationAsset = Creatable<Instance>::create<PartOperationAsset>();
        partOperationAsset->setMeshData(meshData);
        partOperationAsset->setChildData(childData);

        std::stringstream stream;
        RBX::Instances instances;
        instances.push_back(partOperationAsset);
        SerializerBinary::serialize(stream, instances);

        std::string baseUrl = contentProvider->getBaseUrl();
        RBX::Http http(RBX::format("%s/ide/publish/uploadnewasset?assetTypeName=SolidModel&name=SolidModel&description=SolidModel&isPublic=True&genreTypeId=1&allowComments=False", baseUrl.c_str()));
        try
        {
            std::string response;
            http.post(stream, RBX::Http::kContentTypeApplicationXml, true, response);

            int newAssetId;
            std::stringstream istream(response);
            istream >> newAssetId;
            std::string assetId;
            assetId = RBX::format("%s/asset/?id=%d", baseUrl.c_str(), newAssetId);
            ContentId contentId = RBX::ContentId(assetId.c_str());

            partOperation->setAssetId(contentId);
            
			BinaryString noValue;
			partOperation->setChildData(noValue);
			partOperation->setMeshData(noValue);
            
            if (validChildData || !FFlag::CSGFixForNoChildData)
                dataModel->visitDescendants(boost::bind(&setAssetOnMatchingPartOperations, _1, contentId, childKey));
            
            if (validChildData || !FFlag::CSGFixForNoChildData)
                dictionaryService->removeStringData(meshKey);
            nrDictionaryService->removeStringData(childKey);
        }
        catch(std::exception&)
        {
            throw DataModel::SerializationException("Failed to upload union.  Exceeded limit.  Try again in a few minutes.");
        }
        // awagnerTODO: pass childata to LRU cache in content provider
    }
}

bool PartOperationAsset::publishAll(DataModel* dataModel, int timeoutMills)
{
    RBX::Time startPublish = RBX::Time::nowFast();

    dataModel->visitDescendants(boost::bind(&publishPartOperations, _1, startPublish, timeoutMills));

    RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_STUDIO, "SolidModelPublishAll", static_cast<int>((RBX::Time::nowFast() - startPublish).msec()));

	CSGDictionaryService* dictionaryService = ServiceProvider::find< CSGDictionaryService >(dataModel);
	NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::find<NonReplicatedCSGDictionaryService>(dataModel);

	dictionaryService->clean();
	nrDictionaryService->clean();

    return true;
}

bool PartOperationAsset::publishSelection(DataModel* dataModel, int timeoutMills)
{
    RBX::Time startPublish = RBX::Time::nowFast();

    RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(dataModel);
    for (RBX::Instances::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
    {
        publishPartOperations(*iter, startPublish, timeoutMills);
    }

    RobloxGoogleAnalytics::trackUserTiming(GA_CATEGORY_STUDIO, "SolidModelPublishSelection", static_cast<int>((RBX::Time::nowFast() - startPublish).msec()));

	CSGDictionaryService* dictionaryService = ServiceProvider::find< CSGDictionaryService >(dataModel);
	NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::find<NonReplicatedCSGDictionaryService>(dataModel);

	dictionaryService->clean();
	nrDictionaryService->clean();

    return true;
}

} //namespace
