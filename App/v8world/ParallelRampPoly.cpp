#include "stdafx.h"

#include "V8World/ParallelRampPoly.h"
#include "V8World/Mesh.h"

#include "V8World/GeometryPool.h"
#include "V8World/ParallelRampMesh.h"
#include "Util/Math.h"


namespace RBX {

void ParallelRampPoly::buildMesh()
{
	Vector3 key = getSize();

	aParallelRampMesh =	ParallelRampMeshPool::getToken(key);

	mesh = 	aParallelRampMesh->getMesh();

}

Matrix3 ParallelRampPoly::getMoment(float mass) const
{
	Vector3 size = getSize();

	float area = 2 * (size.x * size.y + size.y * size.z + size.z * size.x);
	Vector3 I;
	for (int i = 0; i < 3; i++) {
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;
		float x = size[i];		// main axis;
		float y = size[j];
		float z = size[k];
		
		float Ix = (mass / (2.0f * area)) * (		(y*y*y*z/3.0f)
												+	(y*z*z*z/3.0f)
												+	(x*y*z*z)
												+	(x*y*y*y/3.0f)
												+	(x*y*y*z)
												+	(x*z*z*z/3.0f)	);
		I[i] = Ix;
	}
	return Math::fromDiagonal(I);
}

Vector3 ParallelRampPoly::getCofmOffset( void ) const
{
    return aParallelRampMesh->GetLocalCofMFromMesh();
}

} // namespace RBX