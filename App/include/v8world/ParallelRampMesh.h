#pragma once

/*
	Utility class - holds ParallelRamp Meshes of same size for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class ParallelRampMesh : public Allocator<ParallelRampMesh>
		{
		private:
			Mesh mesh;
			Vector3 LocalCofM;

		public:
			ParallelRampMesh(const Vector3& size) 
			{
				mesh.makeParallelRamp(size, LocalCofM);
			}
			const Mesh* getMesh() const {return &mesh;}
			const Vector3& GetLocalCofMFromMesh() const { return LocalCofM; }
		};

	} // namespace POLY
} // namespace RBX