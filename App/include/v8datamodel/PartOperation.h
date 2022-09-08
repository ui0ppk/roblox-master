/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/BinaryString.h"
#include "V8DataModel/CSGMesh.h"
#include "V8DataModel/PartInstance.h"
#include "Util/ContentID.h"
#include <boost/shared_ptr.hpp>

namespace RBX {

enum CollisionFidelity
{
	CollisionFidelity_Default,
	CollisionFidelity_Hull,
	CollisionFidelity_Box
};

extern const char* const sPartOperation;
class PartOperation
	: public DescribedCreatable<PartOperation, PartInstance, sPartOperation>
{

protected:
	BinaryString childData;
	BinaryString meshData;
	BinaryString physicsData;
	CollisionFidelity collisionFidelity;

	boost::shared_ptr<CSGMesh> cachedMesh;

	Vector3 initialSize;
    bool usePartColor;
  	FormFactor formFactor;
    bool hasAssetId;
    ContentId assetId;

	bool hasMeshData() { return meshData.value() != "" && meshData.value().size() != 0; }

	void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
	static bool renderCollisionData;
	typedef DescribedCreatable<PartOperation, PartInstance, sPartOperation> Super;

	PartOperation();

	static const Reflection::PropDescriptor<PartOperation, Vector3> desc_InitialSize;
	static const Reflection::PropDescriptor<PartOperation, bool> desc_UsePartColor;
    static const Reflection::EnumPropDescriptor<PartOperation, FormFactor> desc_FormFactor;
	static const Reflection::PropDescriptor<PartOperation, ContentId> desc_AssetId;

	/* override */ virtual PartType getPartType() const;

	static const Reflection::PropDescriptor<PartOperation, BinaryString> desc_ChildData;
	static const Reflection::PropDescriptor<PartOperation, BinaryString> desc_MeshData;
	static const Reflection::PropDescriptor<PartOperation, BinaryString> desc_PhysicsData;
	static const Reflection::EnumPropDescriptor<PartOperation, CollisionFidelity> prop_CollisionFidelity;
    
    const BinaryString peekChildData(RBX::Instance* context = NULL);
    const BinaryString& getChildData() const;
  const BinaryString& getChildDataBlocking(RBX::Instance* context = NULL) const;
	void setChildData(const BinaryString& cData);

	const BinaryString& getMeshData() const { return meshData; }
	void setMeshData(const BinaryString& mData);

    void setMesh(CSGMesh* mesh);
	void refreshMesh();
	shared_ptr<CSGMesh> getMesh();
    shared_ptr<CSGMesh> getRenderMesh();

	void setCollisionFidelity(const CollisionFidelity value);
	CollisionFidelity getCollisionFidelity() const { return collisionFidelity; }

	const BinaryString& getPhysicsData() const { return physicsData; }
	void setPhysicsData(const BinaryString& mData);
	bool hasPhysicsData() const { return physicsData.value() != ""; }

	virtual void clearData();
	
    Vector3 getInitialSize() const { return initialSize; }
	void setInitialSize(const Vector3& size);

    bool getUsePartColor() const { return usePartColor; }
    void setUsePartColor(bool usePartColorValue);

    bool hasAsset() const { return hasAssetId; }
    const ContentId& getAssetId() const { return assetId; }
    void setAssetId(ContentId id);

    Vector3 getSizeDifference();

    /*override*/ virtual void setPartSizeXml(const Vector3& rbxSize);
  	/*override*/ virtual bool hasThreeDimensionalSize();
    /*override*/ virtual float getMinimumYDimension() const;
    /*override*/ virtual float getMinimumXOrZDimension() const;
    /*override*/ virtual const Vector3& getMinimumUiSize() const;
  	/*override*/ virtual Vector3 uiToXmlSize(const Vector3& uiSize) const;
    
    /*override*/ virtual FormFactor getFormFactor() const { return formFactor; }
	void setFormFactor(FormFactor value);
    
    static size_t getMaximumTriangleCount();
    static size_t getMaximumMeshStreamSize();
	bool createPhysicsData(const CSGMesh* mData);
	void createPlaceholderPhysicsData();
	void trySetPhysicsData();
	bool checkDecompExists();
	void processOutOfDateData();
	void setBulletCollisionObject();
	std::string getNonKeyPhysicsData() const;
	std::string generateHashKey(const std::string& dataString) const;
	void setBulletObjectsScale(const Vector3& newBoundingBoxSize);
	Vector3 calculateSizeDifference(const Vector3& newBoundingBoxSize);
	Vector3 calculateAdjustedSizeDifference(const Vector3& newBoundingBoxSize);

};

static std::vector<btVector3> getCSGVertexPositions(const std::vector<CSGVertex> &vertices); 

extern const char* const sUnionOperation;
class UnionOperation
	: public DescribedCreatable<UnionOperation, PartOperation, sUnionOperation>
{
public:
	typedef DescribedCreatable<UnionOperation, PartOperation, sUnionOperation> Super;
	UnionOperation();
};

extern const char* const sNegateOperation;
class NegateOperation
	: public DescribedCreatable<NegateOperation, PartOperation, sNegateOperation>
{
public:
	typedef DescribedCreatable<NegateOperation, PartOperation, sNegateOperation> Super;
	NegateOperation();
};

} // namespace
