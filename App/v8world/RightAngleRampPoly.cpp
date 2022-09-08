#include "stdafx.h"

#include "V8World/RightAngleRampPoly.h"
#include "V8World/Mesh.h"

#include "V8World/GeometryPool.h"
#include "V8World/RightAngleRampMesh.h"
#include "Util/Math.h"


namespace RBX {

void RightAngleRampPoly::buildMesh()
{
	Vector3 key = getSize();

	aRightAngleRampMesh =	RightAngleRampMeshPool::getToken(key);

	mesh = 	aRightAngleRampMesh->getMesh();

}

Matrix3 RightAngleRampPoly::getMoment(float mass) const
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

Vector3 RightAngleRampPoly::getCofmOffset( void ) const
{
    return aRightAngleRampMesh->GetLocalCofMFromMesh();
}

size_t RightAngleRampPoly::getFaceFromLegacyNormalId( const NormalId nId ) const
{
	size_t faceId = -1;
	switch(nId)
	{
		case NORM_X:
		default:
			faceId = -1;
			break;
		case NORM_X_NEG:
			faceId = 3;
			break;
		case NORM_Y:
			faceId = 1;
			break;
		case NORM_Y_NEG:
			faceId = 0;
			break;
		case NORM_Z:
			faceId = 2;
			break;
		case NORM_Z_NEG:
			faceId = 5;
			break;
	}

	return faceId;
}

} // namespace RBX