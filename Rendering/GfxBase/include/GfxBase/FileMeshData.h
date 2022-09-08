#pragma once

#include "MeshFileStructs.h"

#include "util/Object.h"
#include "util/G3DCore.h"

#include <vector>

namespace RBX
{
	struct FileMeshData
	{
		std::vector<FileMeshVertexNormalTexture3d> vnts;
		std::vector<FileMeshFace> faces;
		AABox aabb;
	};

    shared_ptr<FileMeshData> ReadFileMesh(const std::string& data);
	
	// writes the newest version always.
	// remember: set ostream to binary!
	void WriteFileMesh(std::ostream& f, const FileMeshData& mesh);
}
