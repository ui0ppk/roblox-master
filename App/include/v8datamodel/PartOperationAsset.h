/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/BinaryString.h"
#include "V8DataModel/CSGMesh.h"
#include "V8DataModel/PartInstance.h"
#include "Util/ContentID.h"
#include <boost/shared_ptr.hpp>

namespace RBX {

extern const char* const sPartOperationAsset;
class PartOperationAsset : public DescribedCreatable<PartOperationAsset, Instance, sPartOperationAsset>
{
public:
    PartOperationAsset() {}

    static const Reflection::PropDescriptor<PartOperationAsset, BinaryString> desc_ChildData;
    static const Reflection::PropDescriptor<PartOperationAsset, BinaryString> desc_MeshData;

    const BinaryString& getChildData() const { return childData; }
    void setChildData(const BinaryString& cData) { childData = cData; }

    const BinaryString& getMeshData() const { return meshData; }
    void setMeshData(const BinaryString& mData) { meshData = mData; }

    static bool publishAll(DataModel* dataModel, int timeoutMills = -1);
    static bool publishSelection(DataModel* dataModel, int timeoutMills = -1);

    boost::shared_ptr<CSGMesh> getRenderMesh() { return renderMesh; }
    void setRenderMesh(boost::shared_ptr<CSGMesh> renderMeshVal) { renderMesh = renderMeshVal; }

private:
    boost::shared_ptr<CSGMesh> renderMesh;
    BinaryString meshData;
    BinaryString childData;
};

} // namespace
