#include "stdafx.h"

#include "V8World/BulletGeometryPoolObjects.h"
#include "V8World/TriangleMesh.h"

namespace RBX {

BulletDecompWrapper::BulletDecompWrapper(const std::string& str)
{
	decomp = TriangleMesh::retrieveDecomposition(str);
    
	// generate data for voxelizer
	int primCount = decomp->getNumChildShapes();
		
	extentArray.reserve(primCount);

	for (int primIter = 0; primIter < primCount; primIter++)
	{
		btTransform transform = decomp->getChildTransform(primIter);

		btVector3 bmin, bmax;
		decomp->getChildAabb(primIter, transform, bmin, bmax);
		
		Vector3 min(bmin.x(), bmin.y(), bmin.z());
		Vector3 max(bmax.x(), bmax.y(), bmax.z());
		
		ConvexExtents ce = { (min + max) * 0.5f, max - min };
		
		extentArray.push_back(ce);
	}
}

BulletDecompWrapper::~BulletDecompWrapper()
{
	if (decomp)
		delete decomp;

	decomp = NULL;
}

BulletBoxShapeWrapper::BulletBoxShapeWrapper(const Vector3& key)
{
	boxShape = new btBoxShape(btVector3(0.5*key.x, 0.5*key.y, 0.5*key.z));
	// No need to shrink, bullet automatically shrinks boxShape the appropriate amount
	boxShape->setMargin(bulletCollisionMargin);
}

BulletBoxShapeWrapper::~BulletBoxShapeWrapper()
{
	delete boxShape;
}

BulletSphereShapeWrapper::BulletSphereShapeWrapper(const float& key)
{
	// Spheres do not need a collision margin.
	sphereShape = new btSphereShape(0.5*key);
}

BulletSphereShapeWrapper::~BulletSphereShapeWrapper()
{
	delete sphereShape;
}

BulletCylinderShapeWrapper::BulletCylinderShapeWrapper(const Vector3& key)
{
    cylinderShape = new btCylinderShapeX(btVector3(0.5*key.x, 0.5*key.y, 0.5*key.z));
	// No need to shrink, bullet automatically shrinks boxShape the appropriate amount
	cylinderShape->setMargin(bulletCollisionMargin);
}

BulletCylinderShapeWrapper::~BulletCylinderShapeWrapper()
{
	delete cylinderShape;
}

BulletWedgeShapeWrapper::BulletWedgeShapeWrapper(const Vector3& key)
{
	wedgeShape = new btConvexHullShape();
	// We have to shrink the shape the right amount before applying a margin.
	float xHalfSize = (0.5 * key.x) - bulletCollisionMargin;
	float yHalfSize = (0.5 * key.y) - bulletCollisionMargin;
	float zHalfSize = (0.5 * key.z) - bulletCollisionMargin;
	wedgeShape->addPoint(btVector3(xHalfSize,    yHalfSize,   zHalfSize), false);
	wedgeShape->addPoint(btVector3(xHalfSize,   -yHalfSize,   zHalfSize), false);
	wedgeShape->addPoint(btVector3(xHalfSize,   -yHalfSize,  -zHalfSize), false);
	wedgeShape->addPoint(btVector3(-xHalfSize,   yHalfSize,   zHalfSize), false);
	wedgeShape->addPoint(btVector3(-xHalfSize,  -yHalfSize,   zHalfSize), false);
	wedgeShape->addPoint(btVector3(-xHalfSize,  -yHalfSize,  -zHalfSize), true);
	wedgeShape->setMargin(bulletCollisionMargin);
}

BulletWedgeShapeWrapper::~BulletWedgeShapeWrapper()
{
	delete wedgeShape;
}

BulletCornerWedgeShapeWrapper::BulletCornerWedgeShapeWrapper(const Vector3& key)
{
	cornerWedgeShape = new btConvexHullShape();
	// We have to shrink the shape the right amount before applying a margin
	float xHalfSize = (0.5 * key.x) - bulletCollisionMargin;
	float yHalfSize = (0.5 * key.y) - bulletCollisionMargin;
	float zHalfSize = (0.5 * key.z) - bulletCollisionMargin;

	cornerWedgeShape->addPoint(btVector3(-xHalfSize,  -yHalfSize,  -zHalfSize), false);
	cornerWedgeShape->addPoint(btVector3(-xHalfSize,  -yHalfSize,   zHalfSize), false);
	cornerWedgeShape->addPoint(btVector3( xHalfSize,   yHalfSize,  -zHalfSize), false);
	cornerWedgeShape->addPoint(btVector3( xHalfSize,  -yHalfSize,  -zHalfSize), false);
	cornerWedgeShape->addPoint(btVector3( xHalfSize,  -yHalfSize,   zHalfSize), true);		// calculates AABB when 2nd arg is true


	cornerWedgeShape->setMargin(bulletCollisionMargin);
}

BulletCornerWedgeShapeWrapper::~BulletCornerWedgeShapeWrapper()
{
	delete cornerWedgeShape;
}

} // namespace RBX
