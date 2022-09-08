/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8datamodel/SpecialMesh.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/FileMesh.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "Humanoid/Humanoid.h"

namespace RBX {
	class PartInstance;

	class PartCookie
	{
	public:
		enum
		{
			HAS_DECALS = 1 << 0,
			IS_HUMANOID_PART = 1 << 1,
			HAS_SPECIALSHAPE = 1 << 2,
			HAS_FILEMESH = 1 << 3,
			HAS_HEADMESH = 1 << 4,
			HAS_DECALS_Z_NEG = 1 << 5
		};

		static unsigned int compute(RBX::PartInstance* part);
	};

	// Return the special shape for the part - i.e. the LAST child of type DataModelMesh
	inline DataModelMesh* getSpecialShape(PartInstance* part)
	{
		if ((part->getCookie() & PartCookie::HAS_SPECIALSHAPE) == 0)
			return NULL;

		if (!part->getChildren())
			return NULL;

		const Instances& children = *part->getChildren();

		DataModelMesh* result = NULL;

		for (size_t i = 0; i < children.size(); ++i)
			if (DataModelMesh* specialShape = children[i]->fastDynamicCast<DataModelMesh>())
				result = specialShape;

		return result;
	}

	inline FileMesh* getFileMesh(DataModelMesh* specialShape)
	{
		if (specialShape)
		{
			if (SpecialShape* shape = specialShape->fastDynamicCast<SpecialShape>())
			{
				if (shape->getMeshType() == SpecialShape::FILE_MESH)
					return shape;
			}
			else if (FileMesh* shape = specialShape->fastDynamicCast<FileMesh>())
			{
				return shape;
			}
		}

		return NULL;
	}

	inline FileMesh* getFileMesh(PartInstance* part)
	{
		return getFileMesh(getSpecialShape(part));
	}

	inline Humanoid* getHumanoid(PartInstance* part)
	{
		if (part->getCookie() & PartCookie::IS_HUMANOID_PART)
			return Humanoid::modelIsCharacter(part->getParent());
		else
			return NULL;
	}

	inline float getPartReflectance(PartInstance* part)
	{
		return G3D::clamp(part->getReflectance(), 0.f, 1.f) * 0.8f;
	}

} // namespace