/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/specialmesh.h"
#include "V8DataModel/PartInstance.h"

using namespace RBX;

const char* const RBX::sSpecialShape = "SpecialMesh";

namespace RBX {
namespace Reflection {
template<>
EnumDesc<SpecialShape::MeshType>::EnumDesc()
	:EnumDescriptor("MeshType")
{
	addPair(SpecialShape::HEAD_MESH, "Head");
	addPair(SpecialShape::TORSO_MESH, "Torso");
	addPair(SpecialShape::WEDGE_MESH, "Wedge");
	addPair(SpecialShape::PRISM_MESH, "Prism", Descriptor::Attributes::deprecated());
	addPair(SpecialShape::PYRAMID_MESH, "Pyramid", Descriptor::Attributes::deprecated());
	addPair(SpecialShape::PARALLELRAMP_MESH, "ParallelRamp", Descriptor::Attributes::deprecated());
	addPair(SpecialShape::RIGHTANGLERAMP_MESH, "RightAngleRamp", Descriptor::Attributes::deprecated());
	addPair(SpecialShape::CORNERWEDGE_MESH, "CornerWedge", Descriptor::Attributes::deprecated());
	addPair(SpecialShape::BRICK_MESH, "Brick");
	addPair(SpecialShape::SPHERE_MESH, "Sphere");
	addPair(SpecialShape::CYLINDER_MESH, "Cylinder");
	addPair(SpecialShape::FILE_MESH, "FileMesh");
}
}//namespace Reflection
}//namespace RBX

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<SpecialShape, SpecialShape::MeshType> desc_meshType("MeshType", category_Data, &SpecialShape::getMeshType, &SpecialShape::setMeshType);
REFLECTION_END();

SpecialShape::SpecialShape()
: meshType(HEAD_MESH)
{
	setName("Mesh");
}

void SpecialShape::setMeshType(MeshType value)
{
	if (meshType!=value)
	{
		meshType = value;
		raisePropertyChanged(desc_meshType);
	}
}

void SpecialShape::setMeshId(const MeshId& value)
{
	if (meshId!=value)
	{
		Super::setMeshId(value);
		setMeshType(SpecialShape::FILE_MESH);
	}
}


void SpecialShape::setTextureId(const TextureId& value)
{
	if (textureId!=value)
	{
		Super::setTextureId(value);	
		setMeshType(SpecialShape::FILE_MESH);
	}
}

