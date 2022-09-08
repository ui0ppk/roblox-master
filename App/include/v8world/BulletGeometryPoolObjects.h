#pragma once

/*
	Utility class - holds Bullet Shapes for use by Geometry Pool.
*/

#include "Util/Memory.h"

#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btConvexPolyhedron.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include "BulletCollision/GImpact/btGImpactShape.h"
#include "Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "btBulletCollisionCommon.h"

// Comment this out to use btCompoundShape and the more robust narrow phase
// Uncomment to use btGImpactConvexDecompositionShape
#define USE_GIMPACT

const float bulletCollisionMargin = 0.05f;



namespace RBX {

	class BulletDecompWrapper : public Allocator<BulletDecompWrapper>
	{
	public:
	#ifdef USE_GIMPACT
		typedef btGImpactConvexDecompositionShape ShapeType;
	#else
		typedef btCompoundShape ShapeType;
	#endif
    
        struct ConvexExtents
        {
            Vector3 center;
            Vector3 size;
        };

		BulletDecompWrapper(const std::string& str);
		~BulletDecompWrapper();

		const ShapeType* getCompound() const { return decomp; }
        const std::vector<ConvexExtents>& getExtentArray() const { return extentArray; }

	private:
		ShapeType* decomp;
        std::vector<ConvexExtents> extentArray;
	};

	class BulletBoxShapeWrapper : public Allocator<BulletBoxShapeWrapper>
	{
	private:
		btBoxShape* boxShape;

	public:
		const btBoxShape* getShape(void) const { return boxShape; }
		BulletBoxShapeWrapper(const Vector3& key);
		~BulletBoxShapeWrapper();

	};

	class BulletSphereShapeWrapper : public Allocator<BulletSphereShapeWrapper>
	{
	private:
		btSphereShape* sphereShape;

	public:
		const btSphereShape* getShape(void) const { return sphereShape; }
		BulletSphereShapeWrapper(const float& key);
		~BulletSphereShapeWrapper();

	};

    class BulletCylinderShapeWrapper : public Allocator<BulletCylinderShapeWrapper>
	{
	private:
		btCylinderShape* cylinderShape;

	public:
		const btCylinderShape* getShape(void) const { return cylinderShape; }
		BulletCylinderShapeWrapper(const Vector3& key);
		~BulletCylinderShapeWrapper();
    };

	class BulletWedgeShapeWrapper : public Allocator<BulletWedgeShapeWrapper>
	{
	private:
		btConvexHullShape* wedgeShape;

	public:
		const btConvexHullShape* getShape(void) const { return wedgeShape; }
		BulletWedgeShapeWrapper(const Vector3& key);
		~BulletWedgeShapeWrapper();

	};

	class BulletCornerWedgeShapeWrapper : public Allocator<BulletCornerWedgeShapeWrapper>
	{
	private:
		btConvexHullShape* cornerWedgeShape;

	public:
		const btConvexHullShape* getShape(void) const { return cornerWedgeShape; }
		BulletCornerWedgeShapeWrapper(const Vector3& key);
		~BulletCornerWedgeShapeWrapper();

	};
} // namespace RBX
