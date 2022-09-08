/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "DataModelMesh.h"
#include "Util/MeshId.h"
#include "Util/TextureId.h"

namespace RBX
{
	extern const char* const sFileMesh;
	class FileMesh
		: public DescribedCreatable<FileMesh, DataModelMesh, sFileMesh>
	{	
	protected :
		TextureId textureId;
		MeshId meshId;
				
	public:
		FileMesh();

		const MeshId& getMeshId() const {return meshId;}
		const TextureId& getTextureId() const {return textureId;}
		
		// These are made virtual because SpecialMesh automatically changes the enum type if this is called
		virtual void setMeshId(const MeshId& value);
		virtual void setTextureId(const TextureId& value);
		
		
	};
}