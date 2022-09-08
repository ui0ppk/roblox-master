/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PartOperation.h"
#include "V8DataModel/GameBasicSettings.h"

#include "G3D/G3dMath.h"
#include "G3D/CollisionDetection.h"
#include "Network/Players.h"
#include "Reflection/Reflection.h"
#include "Util/BinaryString.h"
#include "Util/NormalId.h"
#include "Util/SurfaceType.h"
#include "Util/stringbuffer.h"
#include "V8DataModel/CSGMesh.h"
#include "V8DataModel/ContentProvider.h"
#include "v8datamodel/CSGDictionaryService.h"
#include "V8DataModel/FlyweightService.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PartOperationAsset.h"
#include "V8DataModel/SolidModelContentProvider.h"
#include "V8DataModel/Workspace.h"
#include "V8World/BulletGeometryPoolObjects.h"
#include "V8World/ContactManager.h"
#include "V8World/Primitive.h"
#include "V8World/TriangleMesh.h"
#include "V8World/World.h"

FASTFLAGVARIABLE(CSGRemoveScriptScaleRestriction, false)
FASTFLAGVARIABLE(StudioCSGAssets, false)
FASTFLAGVARIABLE(CSGLoadFromCDN, false)
FASTFLAGVARIABLE(CSGLoadBlocking, false)

FASTFLAGVARIABLE(CSGPhysicsLevelOfDetailEnabled, false)
FASTFLAGVARIABLE(CSGUnionsSizeShouldNeverBe000, false)
DYNAMIC_FASTFLAGVARIABLE(TeamCreateRaiseChangedOperationForAssetId, true)

namespace RBX
{
bool PartOperation::renderCollisionData = false;
using namespace Reflection;

const Reflection::PropDescriptor<PartOperation, BinaryString> PartOperation::desc_ChildData("ChildData", category_Data, &PartOperation::getChildData, &PartOperation::setChildData, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
const Reflection::PropDescriptor<PartOperation, BinaryString> PartOperation::desc_MeshData("MeshData", category_Data, &PartOperation::getMeshData, &PartOperation::setMeshData, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);
const Reflection::PropDescriptor<PartOperation, BinaryString> PartOperation::desc_PhysicsData("PhysicsData", category_Data, &PartOperation::getPhysicsData, &PartOperation::setPhysicsData, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);
const Reflection::PropDescriptor<PartOperation, Vector3> PartOperation::desc_InitialSize("InitialSize", category_Data, &PartOperation::getInitialSize, &PartOperation::setInitialSize, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);
const Reflection::PropDescriptor<PartOperation, bool> PartOperation::desc_UsePartColor("UsePartColor", category_Data, &PartOperation::getUsePartColor, &PartOperation::setUsePartColor);
const Reflection::EnumPropDescriptor<PartOperation, PartOperation::FormFactor> PartOperation::desc_FormFactor("FormFactor", category_Data, &PartOperation::getFormFactor, &PartOperation::setFormFactor, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);
const Reflection::PropDescriptor<PartOperation, ContentId> PartOperation::desc_AssetId("AssetId", category_Data, &PartOperation::getAssetId, &PartOperation::setAssetId, Reflection::PropertyDescriptor::STREAMING, Security::Roblox);
const Reflection::EnumPropDescriptor<PartOperation, CollisionFidelity> PartOperation::prop_CollisionFidelity("CollisionFidelity", category_Behavior, &PartOperation::getCollisionFidelity, &PartOperation::setCollisionFidelity, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED);

namespace Reflection {
template<>
EnumDesc<CollisionFidelity>::EnumDesc()
	:EnumDescriptor("CollisionFidelity")
{
	addPair(CollisionFidelity_Default,		"Default");
	addPair(CollisionFidelity_Hull,			"Hull");
	addPair(CollisionFidelity_Box,			"Box");
}
}

void PartOperation::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);
	
	if (!oldProvider && newProvider)
	{
		CSGDictionaryService* dictionaryService = newProvider->create<CSGDictionaryService>(this);
		NonReplicatedCSGDictionaryService* nrDictionaryService = newProvider->create<NonReplicatedCSGDictionaryService>(this);
		dictionaryService->storeData(*this, true);
		nrDictionaryService->storeData(*this, true);
	}
	else if (!newProvider && oldProvider)
	{
		CSGDictionaryService* dictionaryService = oldProvider->create<CSGDictionaryService>(this);
		NonReplicatedCSGDictionaryService* nrDictionaryService = oldProvider->create<NonReplicatedCSGDictionaryService>(this);
		dictionaryService->retrieveData(*this);
		nrDictionaryService->retrieveData(*this);
	}
}

const BinaryString PartOperation::peekChildData(RBX::Instance* context)
{
    if (FFlag::StudioCSGAssets)
    {
        if (hasAssetId)
        {
          return getChildDataBlocking( context );
        }
    }

    RBX::CSGDictionaryService* dictionaryService = ServiceProvider::find<CSGDictionaryService>(context ? context : this);
	RBX::NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(context ? context : this);

	if (FlyweightService::isHashKey(childData.value()))
	{
		const BinaryString& str = dictionaryService->peekAtData(childData);
		if (!str.value().empty())
			return str;
		else
			return nrDictionaryService->peekAtData(childData);
	}

	return childData;
}

const BinaryString& PartOperation::getChildDataBlocking(RBX::Instance* context) const
{
  if (FFlag::StudioCSGAssets)
  {
    if (hasAssetId)
    {
      ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(context ? context : this);
            CacheableContentProvider* mcp = ServiceProvider::create<SolidModelContentProvider>(contentProvider);

            boost::shared_ptr<PartOperationAsset> partOperationAsset = boost::static_pointer_cast<PartOperationAsset>(mcp->blockingRequestContent(getAssetId(), true));

            if (partOperationAsset)
            {
                return partOperationAsset->getChildData();
            }
            else
            {
                return getChildData();
            }
        }
    }
    
    return getChildData();
}


const BinaryString& PartOperation::getChildData() const
{
    return childData;
}

void PartOperation::setChildData(const BinaryString& cData)
{
    if (childData != cData)
    {
    	childData = cData;
    	raisePropertyChanged(desc_ChildData);
    }
}

void PartOperation::setMeshData(const BinaryString& mData)
{
    if (meshData != mData)
    {
    	meshData = mData;
    	raisePropertyChanged(desc_MeshData);
    }
}

void PartOperation::refreshMesh()
{
	cachedMesh = shared_ptr<CSGMesh>();
	getMesh();
}

shared_ptr<CSGMesh> PartOperation::getMesh()
{
	if (!cachedMesh)
	{
        if (FFlag::StudioCSGAssets)
        {
            if (hasAssetId)
            {
                ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this);
                CacheableContentProvider* mcp = ServiceProvider::create<SolidModelContentProvider>(contentProvider);

                boost::shared_ptr<PartOperationAsset> partOperationAsset = boost::static_pointer_cast<PartOperationAsset>(mcp->blockingRequestContent(getAssetId(), true));

                if (partOperationAsset)
                {
                    return partOperationAsset->getRenderMesh();
                }
                else
                {
                    return shared_ptr<CSGMesh>();
                }
            }
        }
        
		CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
		BinaryString meshString = dictionaryService->peekAtData(meshData);

        if (meshString.value() == "")
            return shared_ptr<CSGMesh>();

        cachedMesh = dictionaryService->getMesh(*this);

        if (!cachedMesh)
            return shared_ptr<CSGMesh>();

        if ((cachedMesh->isBadMesh() || cachedMesh->getIndices().size() / 3 > getMaximumTriangleCount()) && Workspace::serverIsPresent(this))
			cachedMesh->clearMesh();

	}
	return cachedMesh;
}

shared_ptr<CSGMesh> PartOperation::getRenderMesh()
{
    if (FFlag::StudioCSGAssets)
    {
        if (hasAssetId)
        {
            return shared_ptr<CSGMesh>();
        }
    }

    if (!cachedMesh)
    {
        CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
        BinaryString meshString = dictionaryService->peekAtData(meshData);

        if (meshString.value() == "")
            return shared_ptr<CSGMesh>();

        cachedMesh = dictionaryService->getMesh(*this);

        if (!cachedMesh)
            return shared_ptr<CSGMesh>();

        if ((cachedMesh->isBadMesh() || cachedMesh->getIndices().size() / 3 > getMaximumTriangleCount()) && Workspace::serverIsPresent(this))
            cachedMesh->clearMesh();
        
    }
    return cachedMesh;
}


void PartOperation::setMesh(CSGMesh* mesh)
{
    cachedMesh.reset(mesh);
    setMeshData(BinaryString(cachedMesh->toBinaryString()));
}

void PartOperation::setPhysicsData(const BinaryString& mData)
{
    if (physicsData != mData)
    {
    	physicsData = mData;
    	raisePropertyChanged(desc_PhysicsData);
	}

	trySetPhysicsData();
}

void PartOperation::clearData()
{
	setChildData(BinaryString());
	setMeshData(BinaryString());
	setPhysicsData(BinaryString());
}

const char* const sPartOperation = "PartOperation";
PartOperation::PartOperation()
: initialSize(1.0f, 1.0f, 1.0f)
, usePartColor(false)
, hasAssetId(false)
, collisionFidelity(CollisionFidelity_Default)
{
	setName( "PartOperation" );
	setColor3(Color3(1.0f, 1.0f, 1.0f));
	setSurfaceType(NORM_Y, NO_SURFACE);
	setSurfaceType(NORM_Y_NEG, NO_SURFACE);
    formFactor = SYMETRIC;
	getPrimitive(this)->setGeometryType(Geometry::GEOMETRY_TRI_MESH);
}

PartType PartOperation::getPartType() const
{
	return OPERATION_PART;
}

void PartOperation::setInitialSize(const Vector3& size)
{
	initialSize = size;
	raisePropertyChanged(desc_InitialSize);
}

void PartOperation::setUsePartColor(bool usePartColorValue)
{
    usePartColor = usePartColorValue;
	raisePropertyChanged(desc_UsePartColor);
} 

Vector3 PartOperation::getSizeDifference()
{
    return calculateSizeDifference(getPartSizeUi());
}

void PartOperation::setCollisionFidelity(CollisionFidelity value)
{
	if (value != getCollisionFidelity())
	{
		collisionFidelity = value;
		raisePropertyChanged(prop_CollisionFidelity);
		RunService* rs = ServiceProvider::find<RunService>(this);
		if (rs && (rs->getRunState() == RS_RUNNING || rs->getRunState() == RS_PAUSED))
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Cannot change SolidModel CollisionFidelity during Run-Time");
		}
	}
}

void PartOperation::setPartSizeXml(const Vector3& rbxSize)
{
	if (rbxSize != getPartSizeXml())
	{
		//Has to happen before Primitive->setSize
		setBulletObjectsScale(rbxSize);

		getPartPrimitive()->setSize(rbxSize);

		raisePropertyChanged(prop_Size);

        refreshPartSizeUi();
	}
}

bool PartOperation::hasThreeDimensionalSize()
{
    return formFactor != SYMETRIC;
}

float PartOperation::getMinimumYDimension() const
{
    return getMinimumUiSize().y;
}

float PartOperation::getMinimumXOrZDimension() const
{
    return getMinimumUiSize().x;
}

const Vector3& PartOperation::getMinimumUiSize() const
{
    static Vector3 minSize(0.2f, 0.2f, 0.2f);
    return minSize;
}

Vector3 PartOperation::uiToXmlSize(const Vector3& uiSize) const
{
    if (!FFlag::CSGRemoveScriptScaleRestriction)
        return PartInstance::uiToXmlSize(uiSize);
	else if (FFlag::CSGUnionsSizeShouldNeverBe000 && uiSize.isZero())
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Cannot set SolidModel size to Vector3(0, 0, 0)");
		return getConstPartPrimitive()->getSize();
	}

	return uiSize;
}

void PartOperation::setFormFactor(FormFactor value)
{
    formFactor = value;
	raisePropertyChanged(desc_FormFactor);
}

const char* const sUnionOperation = "UnionOperation";
UnionOperation::UnionOperation()
{
	setName( "Union" );
}

const char* const sNegateOperation = "NegateOperation";
NegateOperation::NegateOperation()
{
	setName( "NegativePart" );
    setTransparency(0.1f);
	setAnchored(true);
	setCanCollide(false);
}

bool PartOperation::createPhysicsData(const CSGMesh* mData)
{
	if (FFlag::CSGPhysicsLevelOfDetailEnabled)
	{
		CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));

		if (!mData)
			return false;

		if (!(mData->isNotEmpty()))
			return false;

		try
		{
			BinaryString decompBinString;
			if (getCollisionFidelity() == CollisionFidelity_Default)
			{
				const std::vector<btVector3> vertices	= getCSGVertexPositions(mData->getVertices());
				const std::vector<unsigned int> indices = mData->getIndices();
				if (vertices.size() > 0 && indices.size() > 0)
				{
					decompBinString = BinaryString(TriangleMesh::generateDecompositionData(indices.size()/3, &indices[0], vertices.size(), (btScalar*) &vertices[0].x()));
					getPrimitive(this)->resetGeometryType(Geometry::GEOMETRY_TRI_MESH);
				}
				else
				{
					decompBinString = BinaryString();
				}
			}
			else if (getCollisionFidelity() == CollisionFidelity_Hull)
			{
				const std::vector<btVector3> vertices	= getCSGVertexPositions(mData->getVertices());
				const std::vector<unsigned int> indices = mData->getIndices();
				if (vertices.size() > 0 && indices.size() > 0)
				{
					decompBinString = BinaryString(TriangleMesh::generateConvexHullData(indices.size()/3, &indices[0], vertices.size(), &vertices[0]));
					getPrimitive(this)->resetGeometryType(Geometry::GEOMETRY_TRI_MESH);
				}
				else
				{
					decompBinString = BinaryString();
				}
			}
			else if (getCollisionFidelity() == CollisionFidelity_Box)
			{
				std::string blockPhysicsString = TriangleMesh::getBlockData();
				decompBinString = BinaryString(blockPhysicsString);
				getPrimitive(this)->resetGeometryType(Geometry::GEOMETRY_BLOCK);
			}

			if (decompBinString.value().size() > PartOperation::getMaximumMeshStreamSize())
				return false;

			if (decompBinString.value()!="")
			{
				physicsData = decompBinString;
				if (dictionaryService)
				{
					dictionaryService->storePhysicsData(*this);
					trySetPhysicsData();
				}
			}
		}
		catch (std::exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "Generating Physics Data for %s Failed: %s", getName().c_str(), e.what());
			return false;
		}

	}
	else
	{
		// do nothing in this function if we are simply using block collision
		if (getPrimitive(this)->getGeometryType() != Geometry::GEOMETRY_TRI_MESH)
		{
				return false;
		}

		TriangleMesh* primitiveMesh = rbx_static_cast<TriangleMesh*>(getPrimitive(this)->getGeometry());

		if (primitiveMesh)
		{
			CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));

			if (!mData)
				return false;

			if (!(mData->isNotEmpty()))
				return false;

			try
			{
				BinaryString decompBinString = BinaryString(primitiveMesh->generateDecompositionGeometry(getCSGVertexPositions(mData->getVertices()), mData->getIndices()));

				if (decompBinString.value().size() > PartOperation::getMaximumMeshStreamSize())
					return false;

				if (decompBinString.value()!="")
				{
					physicsData = decompBinString;
					if (dictionaryService)
					{
						dictionaryService->storePhysicsData(*this);
						trySetPhysicsData();
					}
				}
			}
			catch (std::exception& e)
			{
				StandardOut::singleton()->printf(MESSAGE_ERROR, "Generating Physics Data for %s Failed: %s", getName().c_str(), e.what());
				return false;
			}
		}
	}
	return true;
}

void PartOperation::createPlaceholderPhysicsData()
{
	// Saves empty version(0) decomposition data
	setPhysicsData(BinaryString(TriangleMesh::getPlaceholderData()));
}

size_t PartOperation::getMaximumTriangleCount()
{
    return 2500;
}

size_t PartOperation::getMaximumMeshStreamSize()
{
	 return 512000;
}

void PartOperation::trySetPhysicsData()
{
	if (getPrimitive(this)->getGeometry()->getGeometryType() == Geometry::GEOMETRY_BLOCK)
	{
		return;
	}
	TriangleMesh* primitiveMesh = rbx_static_cast<TriangleMesh*>(getPrimitive(this)->getGeometry());
	if (!primitiveMesh->getCompound())
	{
		CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
		if (dictionaryService)
		{
			// Calculate shrunken scale
			const Vector3 scale = calculateAdjustedSizeDifference(getPartSizeXml());
			const Vector3 meshScale = calculateSizeDifference(getPartSizeXml());
            std::string hashKey = generateHashKey(getNonKeyPhysicsData());

			if(primitiveMesh->setCompoundMeshData(hashKey, getNonKeyPhysicsData(), btVector3(scale.x, scale.y, scale.z)))
			{
				primitiveMesh->setStaticMeshData(hashKey, getNonKeyPhysicsData(), btVector3(meshScale.x, meshScale.y, meshScale.z));
				setBulletCollisionObject();
				return;
			}
			// Anything less than version 3 is considered placeholder data
			if (FFlag::CSGPhysicsLevelOfDetailEnabled &&  TriangleMesh::validateIsBlockData(getNonKeyPhysicsData()))
			{
				getPrimitive(this)->resetGeometryType(Geometry::GEOMETRY_BLOCK);
			}
			else if (primitiveMesh->getVersion() < PHYSICS_SERIAL_VERSION && primitiveMesh->getVersion() != -1)
			{
				processOutOfDateData();
				return;
			}
		}
	}
}

bool PartOperation::checkDecompExists()
{
	if (getPrimitive(this)->getGeometryType() == Geometry::GEOMETRY_TRI_MESH)
	{
		TriangleMesh* primitiveMesh = rbx_static_cast<TriangleMesh*>(getPrimitive(this)->getGeometry());
		if (primitiveMesh->getCompound())
			return true;
	}
	return false;
}

void visitDescendentsSetPhysics(shared_ptr<Instance> descendant, PartOperation* sourceOperation)
{
	CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(sourceOperation));
	if(PartOperation* iteratedPartOp = RBX::Instance::fastDynamicCast<PartOperation>(descendant.get()))
	{
		bool haveSameGeometry;
		/// StudioCSGAssets flag introduces some new conditions that we cannot meet with the old check. If these are enabled we must have a new condition that checks.
		if (FFlag::StudioCSGAssets)
		{
			haveSameGeometry = (sourceOperation->getMesh().get() != NULL && iteratedPartOp->getMesh().get() != NULL &&  sourceOperation->getMesh().get() == iteratedPartOp->getMesh().get());
		}
		else
		{
			haveSameGeometry = dictionaryService->getHashKey(sourceOperation->getMeshData().value()) == dictionaryService->getHashKey(iteratedPartOp->getMeshData().value());
		}

		// Checks if they are technically the same mesh (which would mean they have the same data)
		if (haveSameGeometry)
		{
			if (FFlag::CSGPhysicsLevelOfDetailEnabled)
			{
				// With introduction of Level of Detail in Physics Geometry, we are no longer guaranteed that same mesh means same Physics
				// We now have to make sure that MeshData HAS loaded (meaning the Data is not Invalid) and that it's actually out of date.
				// We potentially have to trust the CollisionFidelity ENUM here to get any benefit out of efficiency. 
				int verifyVersion = -1;
				if (!TriangleMesh::validateDataVersions(iteratedPartOp->getNonKeyPhysicsData(), verifyVersion) && verifyVersion != -1 &&
					iteratedPartOp->getCollisionFidelity() == sourceOperation->getCollisionFidelity())
				{
					iteratedPartOp->setPhysicsData(sourceOperation->getPhysicsData());
					dictionaryService->storePhysicsData(*iteratedPartOp, true);
				}
			}
			else
			{
				iteratedPartOp->setPhysicsData(sourceOperation->getPhysicsData());
				dictionaryService->storePhysicsData(*iteratedPartOp, true);
			}
		}
	}
}

void PartOperation::processOutOfDateData()
{
	if(RBX::DataModel::get(this)->isStudio())
	{
		// Three return calls to safeguard against corrupted/empty CSG mesh data.
		if (!getMesh().get())
			return;

		if (!getMesh().get()->isNotEmpty())
			return;

		if (createPhysicsData(getMesh().get()))
		{
			RBX::DataModel::get(this)->visitDescendants(boost::bind(&visitDescendentsSetPhysics, _1, this));
		}
		else
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Legacy Part not compatible with Physics, try separating and simplifying");
			getPrimitive(this)->setGeometryType(Geometry::GEOMETRY_BLOCK);
		}
	}
	else
	{
		getPrimitive(this)->setGeometryType(Geometry::GEOMETRY_BLOCK);
	}
}

void PartOperation::setBulletCollisionObject()	
{
	getPrimitive(this)->getGeometry()->setUpBulletCollisionData();
}

std::string PartOperation::getNonKeyPhysicsData() const
{
	CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
	if (dictionaryService)
		return dictionaryService->peekAtData(getPhysicsData()).value();
	else 
		return "";
}

std::string PartOperation::generateHashKey(const std::string& dataString) const
{
	CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
	if (dictionaryService)
		return FlyweightService::createHashKey(dataString);
	else
		return "";
}


void PartOperation::setBulletObjectsScale(const Vector3& newBoundingBoxSize)
{
	CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(RBX::DataModel::get(this));
	if (dictionaryService && checkDecompExists())
	{
		TriangleMesh* primitiveMesh = rbx_static_cast<TriangleMesh*>(getPrimitive(this)->getGeometry());
		primitiveMesh->updateObjectScale(generateHashKey(getNonKeyPhysicsData()), 
											getNonKeyPhysicsData(), 
											calculateAdjustedSizeDifference(newBoundingBoxSize),
											calculateSizeDifference(newBoundingBoxSize));
	}
}

Vector3 PartOperation::calculateSizeDifference(const Vector3& newBoundingBoxSize)
{
	Vector3 v = newBoundingBoxSize / initialSize;

	// If we are adjusting for BulletMargin we CANNOT force symmetry
    if (formFactor == PartInstance::SYMETRIC)
    {
        float uniformScale = std::min(std::min(v.x, v.y), v.z);
	    return Vector3(uniformScale, uniformScale, uniformScale);
    }

    return v;
}

Vector3 PartOperation::calculateAdjustedSizeDifference(const Vector3& newBoundingBoxSize)
{
	Vector3 result = calculateSizeDifference(newBoundingBoxSize) - ((Vector3(2*bulletCollisionMargin)) / initialSize);
	return result;
}

void PartOperation::setAssetId(ContentId id)
{
    if (ContentProvider::isUrl(id.toString()))
        hasAssetId = true;

	bool changed = DFFlag::TeamCreateRaiseChangedOperationForAssetId && id != assetId;

    assetId = id;

	if (changed && Network::Players::isCloudEdit(this))
	{
		raisePropertyChanged(desc_AssetId);
	}
}

std::vector<btVector3> getCSGVertexPositions(const std::vector<CSGVertex> &vertices)
{
	std::vector<btVector3> outputVec;
	for (unsigned int i = 0; i < vertices.size(); i++) 
	{
		outputVec.push_back(btVector3(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z));
	}
	return outputVec;
}

} //namespace
