/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/ContentId.h"
#include "Util/MeshId.h"
#include "Util/TextureId.h"
#include "Util/G3DCore.h"
#include "FileMesh.h"

namespace RBX
{
	extern const char* const sSpecialShape;
	class SpecialShape 
		: public DescribedCreatable<SpecialShape, FileMesh, sSpecialShape>
	{
	private:
		typedef DescribedCreatable<SpecialShape, FileMesh, sSpecialShape> Super;

	public:
		// Warning - these values determine XML read/write.  Only append, never change
		typedef enum {	HEAD_MESH = 0, 
						TORSO_MESH = 1,
						WEDGE_MESH = 2,
						SPHERE_MESH = 3,
						CYLINDER_MESH = 4, 
						FILE_MESH = 5,
						BRICK_MESH = 6,
						PRISM_MESH = 7,
						PYRAMID_MESH = 8,
						PARALLELRAMP_MESH = 9,
						RIGHTANGLERAMP_MESH = 10,
						CORNERWEDGE_MESH = 11
		} MeshType;

	private:
		MeshType meshType;
		
	public:
		SpecialShape();

		MeshType getMeshType() const	{return meshType;}
		void setMeshType(MeshType value);
		
		/*** Override ***/ void setMeshId(const MeshId& value);
		/*** Override ***/ void setTextureId(const TextureId& value);

	};
}

