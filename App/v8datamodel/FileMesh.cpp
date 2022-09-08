/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/FileMesh.h"

using namespace RBX;

const char* const RBX::sFileMesh = "FileMesh";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<FileMesh, MeshId> desc_meshId("MeshId", category_Data, &FileMesh::getMeshId, &FileMesh::setMeshId);
static Reflection::PropDescriptor<FileMesh, TextureId> desc_textureId("TextureId", category_Data, &FileMesh::getTextureId, &FileMesh::setTextureId);
REFLECTION_END();

FileMesh::FileMesh()
{}


void FileMesh::setMeshId(const MeshId& value)
{
	if (meshId!=value)
	{
		meshId = value;
		raisePropertyChanged(desc_meshId);
	}
}

void FileMesh::setTextureId(const TextureId& value)
{
	if (textureId!=value)
	{
		textureId = value;
		raisePropertyChanged(desc_textureId);
	}
}