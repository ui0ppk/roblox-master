#pragma once

/*
	Utility class - holds CornerWedge Meshes of same size for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class CornerWedgeMesh : public Allocator<CornerWedgeMesh>
		{
		private:
			Mesh mesh;
			Vector3 LocalCofM;

		public:
			CornerWedgeMesh(const Vector3& size) 
			{
				mesh.makeCornerWedge(size, LocalCofM);
			}
			const Mesh* getMesh() const {return &mesh;}
			const Vector3& GetLocalCofMFromMesh() const { return LocalCofM; }
		};

	} // namespace POLY
} // namespace RBX