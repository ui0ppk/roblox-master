#pragma once

/*
	Utility class - holds Wedge Meshes of same size for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class WedgeMesh : public Allocator<WedgeMesh>
		{
		private:
			Mesh mesh;

		public:
			WedgeMesh(const Vector3& size) 
			{
				mesh.makeWedge(size);
			}
			const Mesh* getMesh() const {return &mesh;}
		};

	} // namespace POLY
} // namespace RBX
