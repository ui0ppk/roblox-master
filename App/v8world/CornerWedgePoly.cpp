#include "stdafx.h"

#include "V8World/CornerWedgePoly.h"
#include "V8World/Mesh.h"

#include "V8World/GeometryPool.h"
#include "V8World/CornerWedgeMesh.h"
#include "Util/Math.h"


namespace RBX {

void CornerWedgePoly::buildMesh()
{
	Vector3 key = getSize();

	aCornerWedgeMesh =	CornerWedgeMeshPool::getToken(key);

	mesh = 	aCornerWedgeMesh->getMesh();

}

void CornerWedgePoly::setSize(const G3D::Vector3& _size)
{
	Super::setSize(_size);

	if (bulletCollisionObject)
		updateBulletCollisionData();
}


Matrix3 CornerWedgePoly::getMoment(float mass) const
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

Vector3 CornerWedgePoly::getCofmOffset( void ) const
{
    return aCornerWedgeMesh->GetLocalCofMFromMesh();
}

CoordinateFrame CornerWedgePoly::getSurfaceCoordInBody( const size_t surfaceId ) const
{
	CoordinateFrame aCS;

	aCS.translation = mesh->getFace(surfaceId)->getVertex(2)->getOffset();
	aCS.rotation = Math::getWellFormedRotForZVector(mesh->getFace(surfaceId)->normal());

	return aCS;
}

size_t CornerWedgePoly::getFaceFromLegacyNormalId( const NormalId nId ) const
{
	size_t faceId = -1;
	switch(nId)
	{
		case NORM_X:
			faceId = 0;
			break;
		case NORM_X_NEG:
			faceId = 2;
			break;
		case NORM_Y:
		default:
			faceId = -1;
			break;
		case NORM_Y_NEG:
			faceId = 3;
			break;
		case NORM_Z:
			faceId = 1;
			break;
		case NORM_Z_NEG:
			faceId = 4;
			break;
	}

	return faceId;
}

Vector3 CornerWedgePoly::getCenterToCorner(const Matrix3& rotation) const
{
	// we override the default poly behavior of using a sphere to define the AABB for bounds-checking
	//	  for now, we just pretend the wedge is brick-shaped, since this is the closest approximation we can get using AABBs anyways

	//need to convert RBX::POLY::Vertex to G3D::Vector3
	// use vertices 0,1,2, and 3 [bottom face]

	Vector3 maxValue = G3D::abs(rotation * mesh->getVertex(0)->getOffset());
	for (int i = 1; i < 4; ++i) {
		maxValue = maxValue.max(G3D::abs(rotation * mesh->getVertex(i)->getOffset()));
	}
	return maxValue;

}

bool CornerWedgePoly::setUpBulletCollisionData(void)
{
	if (!bulletCollisionObject)
		updateBulletCollisionData();

	return true;
}

void CornerWedgePoly::updateBulletCollisionData()
{
	if (!bulletCollisionObject)
		bulletCollisionObject.reset(new btCollisionObject());

	bulletCornerWedgeShape = BulletCornerWedgeShapePool::getToken(getSize());
	bulletCollisionObject->setCollisionShape(const_cast<btConvexHullShape*>(bulletCornerWedgeShape->getShape()));
}

} // namespace RBX
