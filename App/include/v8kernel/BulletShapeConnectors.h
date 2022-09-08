/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Kernel/ContactParams.h"
#include "V8Kernel/PolyConnectors.h"
#include "Util/G3DCore.h"
#include "rbx/Debug.h"

#include "BulletCollision/NarrowphaseCollision/btPersistentManifold.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "btBulletCollisionCommon.h"


namespace RBX {

	class BulletShapeConnector :	public PolyConnector, 
									public Allocator<BulletShapeConnector>
	{
	protected:
		btCollisionObject* bulletCollisionObject0;
		btCollisionObject* bulletCollisionObject1;
		btCollisionAlgorithm* bulletAlgo;
		int bulletManifoldIndex;
		int bulletPointCacheIndex;

		void updateConnectorPointFromManifold(bool refreshContacts = true);
		void realignConnectorsToBulletContacts();
		bool foundValidContactPointFromBulletManifold(btPersistentManifold* man, Vector3& p0World, Vector3& p1World);

	private:
		/*override*/ GeoPairType getConnectorType() const {return BULLET_SHAPE_CONNECTOR;}
		bool validObjectCFrames();
		virtual void updateBulletCollisionObjects();

	public:
		BulletShapeConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams,
			btCollisionObject* bulletColObj0,
			btCollisionObject* bulletColObj1,
			btCollisionAlgorithm* algo,
			int manifoldIndex,
			int cacheIndex
			)
			: PolyConnector(b0, b1, contactParams, 0, 0)
			, bulletCollisionObject0(bulletColObj0)
			, bulletCollisionObject1(bulletColObj1)
			, bulletAlgo(algo)
			, bulletManifoldIndex(manifoldIndex)
			, bulletPointCacheIndex(cacheIndex)
		{
		}

		~BulletShapeConnector();

		/*override*/ void updateContactPoint();
		void findValidContactAfterNarrowphase();
		bool recalculateValidPoints(btManifoldArray& btManArray, Vector3& pt0InWorld, Vector3& pt1InWorld);
		void setBulletManifoldPointIndex(int index) { bulletPointCacheIndex = index; }
		int getBulletManifoldIndex(void) { return bulletManifoldIndex;}
		int getBulletPointCacheIndex(void) { return bulletPointCacheIndex;}

		void refreshIndividualPoint(bool swapped, Vector3 pt0InWorld, Vector3 pt1InWorld, btManifoldArray& manArray);
		void updatePointWithTransform(bool swapped, btManifoldPoint& manifoldPoint);
		bool isPointInvalid( btManifoldPoint& manifoldPoint, double validThreshold);

		static bool match(BulletShapeConnector* oldCon, BulletShapeConnector* newCon) 
		{
			return ((oldCon->bulletManifoldIndex == newCon->bulletManifoldIndex)
				&&	(oldCon->bulletPointCacheIndex == newCon->bulletPointCacheIndex)
				&&	(oldCon->getConnectorType() == newCon->getConnectorType()));
		}
	};

	class BulletShapeCellConnector : public BulletShapeConnector
	{
	private:
		/*override*/ GeoPairType getConnectorType() const {return BULLET_SHAPE_CELL_CONNECTOR;}
		/*override*/ void updateBulletCollisionObjects();


	public:
		BulletShapeCellConnector(
			Body* b0, 
			Body* b1,
			const ContactParams& contactParams,
			btCollisionObject* bulletColObj0,
			btCollisionObject* bulletColObj1,
			btCollisionAlgorithm* algo,
			int manifoldIndex,
			int cacheIndex
			)
			: BulletShapeConnector(b0, b1, contactParams, bulletColObj0, bulletColObj1, algo, manifoldIndex, cacheIndex)
		{
		}
		~BulletShapeCellConnector() {}

		/*override*/ void updateContactPoint();

		static bool match(BulletShapeCellConnector* oldCon, BulletShapeCellConnector* newCon) 
		{
			return ((oldCon->bulletManifoldIndex == newCon->bulletManifoldIndex)
				&&	(oldCon->bulletPointCacheIndex == newCon->bulletPointCacheIndex)
				&&	(oldCon->getConnectorType() == newCon->getConnectorType()));
		}
	};

} // namespace RBX