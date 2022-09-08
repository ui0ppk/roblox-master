#pragma once

/*
	Utility class - holds MegaCluster dummy Meshes of same size for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class MegaClusterMesh : public Allocator<MegaClusterMesh>
		{
		private:
			Mesh mesh;
			Vector3 LocalCofM;

		public:
			MegaClusterMesh(const Vector3& size) 
			{
				mesh.makeBlock(size);
			}
			const Mesh* getMesh() const {return &mesh;}
			Vector3 GetLocalCofMFromMesh( void ) { return LocalCofM; }
		};

	} // namespace POLY
} // namespace RBX