#pragma once

/*
	Utility class - holds Block Meshes of same size for use by Geometry Pool.
*/

#include "V8World/Mesh.h"
#include "Util/Memory.h"

namespace RBX {

	namespace POLY {

		class BlockMesh : public Allocator<BlockMesh>
		{
			Mesh mesh;
		public:
			BlockMesh(const Vector3& size) {
				mesh.makeBlock(size);
			}
			const Mesh* getMesh() const {return &mesh;}
		};

	} // namespace POLY
} // namespace RBX
