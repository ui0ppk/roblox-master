#pragma once

/*
	Utility class - holds Pyramid Meshes of same size, and parametric shape for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class PyramidMesh : public Allocator<PyramidMesh>
		{
		private:
			Mesh mesh;
			int NumSides;
			int NumSlices;
			Vector3 LocalCofM;

		public:
			PyramidMesh(const Vector3_2Ints& params) 
			{
				// the zero sides and slices will cause immediate bail out of mesh builder for speed.
				LocalCofM = Vector3::zero();
				mesh.makePyramid(params, LocalCofM);
			}
			const Mesh* getMesh() const {return &mesh;}
			void SetNumSides( int num ) {NumSides = num;}
			void SetNumSlices( int num ) {NumSlices = num;}
			const Vector3& GetLocalCofMFromMesh() const { return LocalCofM; }
		};
			
	

	} // namespace POLY

} // namespace RBX
