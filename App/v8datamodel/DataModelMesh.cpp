/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/DataModelMesh.h"
#include "V8DataModel/PartInstance.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/PartCookie.h"

#include "humanoid/Humanoid.h"

namespace RBX {

const char *const sDataModelMesh = "DataModelMesh";

namespace Reflection {
template<>
EnumDesc<DataModelMesh::LODType>::EnumDesc()
	:EnumDescriptor("LevelOfDetailSetting")
{
	addPair(DataModelMesh::HIGH_LOD, "High");
	addPair(DataModelMesh::MEDIUM_LOD, "Medium");
	addPair(DataModelMesh::LOW_LOD, "Low");
}
}//namespace Reflection

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<DataModelMesh, DataModelMesh::LODType> desc_levelOfDetailX("LODX", category_Data, &DataModelMesh::getLevelOfDetailX, &DataModelMesh::setLevelOfDetailX, RBX::Reflection::PropertyDescriptor::STREAMING);
static Reflection::EnumPropDescriptor<DataModelMesh, DataModelMesh::LODType> desc_levelOfDetailY("LODY", category_Data, &DataModelMesh::getLevelOfDetailY, &DataModelMesh::setLevelOfDetailY, RBX::Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<DataModelMesh, Vector3> desc_scale("Scale", category_Data, &DataModelMesh::getScale, &DataModelMesh::setScale);
static Reflection::PropDescriptor<DataModelMesh, Vector3> desc_vertColor("VertexColor", category_Data, &DataModelMesh::getVertColor, &DataModelMesh::setVertColor);
static Reflection::PropDescriptor<DataModelMesh, Vector3> desc_offset("Offset", category_Data, &DataModelMesh::getOffset, &DataModelMesh::setOffset);
REFLECTION_END();

DataModelMesh::DataModelMesh()
	: scale(1.0,1.0,1.0)
	, vertColor(1.0,1.0,1.0)
	, offset(0.0,0.0,0.0)
	, LODx(DataModelMesh::HIGH_LOD)
	, LODy(DataModelMesh::HIGH_LOD)
{
	setName("Mesh");
}

bool DataModelMesh::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<PartInstance>(instance)!=NULL;
}


float DataModelMesh::getAlpha() const
{
	// Alpha for a special shape is the alpha of it's parent part
	const Instance *i = this->getParent();
	if (i)
	{
		const PartInstance *pi = i->fastDynamicCast<PartInstance>();
		if (pi) return pi->getTransparencyUi();
	}
	return 0;
}


void DataModelMesh::setScale(const G3D::Vector3& value)
{
	// Sanitize the value by replacing non-finite components with 0
	// This avoids issues with tesselation while generating geometry
	G3D::Vector3 fixedValue = value;
	
	for (int i = 0; i < 3; ++i)
		if (Math::isNanInf(fixedValue[i]))
			fixedValue[i] = 0;

	if (scale!=fixedValue)
	{
		scale = fixedValue;
		raisePropertyChanged(desc_scale);
	}
}

void DataModelMesh::setVertColor(const G3D::Vector3& value)
{
	if (vertColor!=value)
	{
		vertColor = value;
		raisePropertyChanged(desc_vertColor);
	}
}

void DataModelMesh::setOffset(const G3D::Vector3 &value)
{
	if (offset != value)
	{
		offset = value;
		raisePropertyChanged(desc_offset);
	}
}

void DataModelMesh::setLevelOfDetailX(LODType value)
{
	if (LODx != value)
	{
		LODx= value;
		raisePropertyChanged(desc_levelOfDetailX);
	}
}

void DataModelMesh::setLevelOfDetailY(LODType value)
{
	if (LODy != value)
	{
		LODy = value;
		raisePropertyChanged(desc_levelOfDetailY);
	}
}
} // Namespace RBX
