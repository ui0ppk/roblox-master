#pragma once

/*
	Utility class - holds Prism Meshes of same size, and parametric shape for use by Geometry Pool.
*/

#include "Util/Memory.h"
#include "V8World/Mesh.h"


namespace RBX {

	namespace POLY {

		class PrismMesh : public Allocator<PrismMesh>
		{
		private:
			Mesh mesh;
			int NumSides;
			int NumSlices;
			Vector3 LocalCofM;

		public:
			PrismMesh(const Vector3_2Ints& params) 
			{
				// the zero sides and slices will cause immediate bail out of mesh builder for speed.
				LocalCofM = Vector3::zero();
				mesh.makePrism(params, LocalCofM);
			}
			const Mesh* getMesh() const {return &mesh;}
			void SetNumSides( int num ) {NumSides = num;}
			void SetNumSlices( int num ) {NumSlices = num;}
			const Vector3& GetLocalCofMFromMesh() const { return LocalCofM; }
		};
			
	

	} // namespace POLY

} // namespace RBX
