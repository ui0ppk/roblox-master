#include "stdafx.h"

#include "V8World/WedgePoly.h"

#include "V8World/GeometryPool.h"
#include "V8World/WedgeMesh.h"
#include "Util/Math.h"


namespace RBX {

void WedgePoly::buildMesh()
{
	Vector3 key = getSize();

	wedgeMesh =	WedgeMeshPool::getToken(key);
	mesh = 	wedgeMesh->getMesh();
}

void WedgePoly::setSize(const G3D::Vector3& _size)
{
	Super::setSize(_size);

	if (bulletCollisionObject)
		updateBulletCollisionData();
}

Matrix3 WedgePoly::getMoment(float mass) const
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

Vector3 WedgePoly::getCofmOffset( void ) const
{
	Vector3 ans(0.0f, -getSize().y/6.0f, getSize().z/6.0f);
	//Vector3 ans(0.0f, 0.0f, 0.0f); 

	return ans;
}

CoordinateFrame WedgePoly::getSurfaceCoordInBody( const size_t surfaceId ) const
{
    RBXASSERT( surfaceId != (size_t)-1 );
    CoordinateFrame aCS;
    if( surfaceId == (size_t)-1 )
	    return aCS;
	// the face reference coord origin is the midpoint b/t the first and second vertex
	//aCS.translation = 0.5 * (mesh->getFace(surfaceId)->getVertex(0)->getOffset() + mesh->getFace(surfaceId)->getVertex(1)->getOffset());

	aCS.translation = mesh->getFace(surfaceId)->getVertex(2)->getOffset();
	aCS.rotation = Math::getWellFormedRotForZVector(mesh->getFace(surfaceId)->normal());

	return aCS;
}

size_t WedgePoly::getFaceFromLegacyNormalId( const NormalId nId ) const
{
	size_t faceId = (size_t)-1;
	switch(nId)
	{
		case NORM_X:
			faceId = 2;
			break;
		case NORM_X_NEG:
			faceId = 3;
			break;
		case NORM_Y:
			faceId = 1;
			break;
		case NORM_Y_NEG:
			faceId = 4;
			break;
		case NORM_Z:
			faceId = 0;
			break;
		// Maps to same mesh face as NORM_Y
		case NORM_Z_NEG:
			faceId = 1;
			break;
        default:
            break;
	}

	return faceId;
}

Vector3 WedgePoly::getCenterToCorner(const Matrix3& rotation) const
{
	// we override the default poly behavior of using a sphere to define the AABB for bounds-checking
	//	  for now, we just pretend the wedge is brick-shaped, since this is the closest approximation we can get using AABBs anyways

	//need to convert RBX::POLY::Vertex to G3D::Vector3
	// use vertices 0,1,3, and 4 [back face]
	//[0, 1, 2, 3 for corner wedges]

	Vector3 maxValue = G3D::abs(rotation * mesh->getVertex(0)->getOffset());
	for (int i = 1; i <= 4; ++i) {
		if (i == 2) continue;
		maxValue = maxValue.max(G3D::abs(rotation * mesh->getVertex(i)->getOffset()));
	}
	return maxValue;

}

bool WedgePoly::setUpBulletCollisionData(void)
{
	if (!bulletCollisionObject)
		updateBulletCollisionData();

	return true;
}

void WedgePoly::updateBulletCollisionData()
{
	if (!bulletCollisionObject)
		bulletCollisionObject.reset(new btCollisionObject());

	bulletWedgeShape = BulletWedgeShapePool::getToken(getSize());
	bulletCollisionObject->setCollisionShape(const_cast<btConvexHullShape*>(bulletWedgeShape->getShape()));
}


} // namespace RBX
