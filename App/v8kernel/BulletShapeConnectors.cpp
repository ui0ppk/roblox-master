#include "stdafx.h"

#include "V8Kernel/BulletShapeConnectors.h"
#include "V8Kernel/Body.h"
#include "Util/Math.h"

#include "BulletCollision/NarrowphaseCollision/btPersistentManifold.h"
#include "btBulletCollisionCommon.h"

namespace RBX {
//////////////////////////////////////////////////////////////////////////////////////////

/*
	Position == position in world coordinates of deepest penetration point.
	Length == (negative) value - amount of penetration
	Normal == points "away" from the b0 object, into the b1 object
*/
BulletShapeConnector::~BulletShapeConnector()
{
	if (bulletAlgo)
	{
		btManifoldArray manArray;
		bulletAlgo->getAllContactManifolds(manArray);
		for (int i = 0; i < manArray[bulletManifoldIndex]->getNumContacts(); i++)
		{
			if (manArray[bulletManifoldIndex]->getContactPoint(i).m_userPersistentData == this)
			{
				manArray[bulletManifoldIndex]->getContactPoint(i).m_userPersistentData = NULL;
			}
		}
	}
}

bool BulletShapeConnector::validObjectCFrames()
{
	//Check if the Vector3's are Nans
	return (!(Math::isNanInfVector3(geoPair.body0->getCoordinateFrame().translation) || (Math::isNanInfVector3(geoPair.body1->getCoordinateFrame().translation))));
}

void BulletShapeConnector::updateBulletCollisionObjects()
{	
	geoPair.body0->updateBulletCollisionObject(bulletCollisionObject0);
	geoPair.body1->updateBulletCollisionObject(bulletCollisionObject1);
}

void BulletShapeConnector::refreshIndividualPoint(bool swapped, Vector3 pt0InWorld, Vector3 pt1InWorld, btManifoldArray& manArray)
{
	if ((manArray[bulletManifoldIndex]->getNumContacts() > bulletPointCacheIndex))
	{
		bool pointInvalid = false;
		double contactThreshold = (double) manArray[bulletManifoldIndex]->getContactBreakingThreshold();
		{
			btManifoldPoint &connectorPoint = manArray[bulletManifoldIndex]->getContactPoint(bulletPointCacheIndex);
			updatePointWithTransform(swapped, connectorPoint);
			pointInvalid = isPointInvalid( connectorPoint, contactThreshold );
		}

		if (pointInvalid)
		{
			recalculateValidPoints(manArray, pt0InWorld, pt1InWorld);
		}
	}
}

void BulletShapeConnector::updatePointWithTransform(bool swapped, btManifoldPoint& manifoldPoint)
{
	btTransform tr0;
	btTransform tr1;
	if (swapped)
	{
		tr1 = bulletCollisionObject0->getWorldTransform();
		tr0 = bulletCollisionObject1->getWorldTransform();
	}
	else
	{
		tr0 = bulletCollisionObject0->getWorldTransform();
		tr1 = bulletCollisionObject1->getWorldTransform();
	}
	manifoldPoint.m_positionWorldOnA = tr0( manifoldPoint.m_localPointA );
	manifoldPoint.m_positionWorldOnB = tr1( manifoldPoint.m_localPointB );
	manifoldPoint.m_distance1 = (manifoldPoint.m_positionWorldOnA -  manifoldPoint.m_positionWorldOnB).dot(manifoldPoint.m_normalWorldOnB);
	manifoldPoint.m_lifeTime++;
}

bool BulletShapeConnector::isPointInvalid(btManifoldPoint& manifoldPoint, double validThreshold)
{
	if (!(manifoldPoint.m_distance1 <= validThreshold))
	{
		return true;
	}
	else
	{
		btVector3 projectedPoint = manifoldPoint.m_positionWorldOnA - manifoldPoint.m_normalWorldOnB * manifoldPoint.m_distance1;
		btVector3 projectedDifference = manifoldPoint.m_positionWorldOnB - projectedPoint;
		btScalar distance2d = projectedDifference.dot(projectedDifference);
		if (distance2d > gContactThresholdOrthogonalFactor * gContactThresholdOrthogonalFactor * validThreshold * validThreshold)
		{
			return true;
		}
	}

	return false;
}

void BulletShapeConnector::findValidContactAfterNarrowphase()
{
	updateConnectorPointFromManifold(false);
	ContactConnector::updateContactPoint();
}

void BulletShapeConnector::updateConnectorPointFromManifold(bool refreshContacts)
{
	btManifoldArray manifoldArray;
	bulletAlgo->getAllContactManifolds(manifoldArray);

	RBXASSERT(manifoldArray.size() > bulletManifoldIndex);
	Vector3 pt0InWorld, pt1InWorld;
	bool hasValidPoints = false;
	bool swapped = !(manifoldArray[bulletManifoldIndex]->getBody0() == bulletCollisionObject0);
	if (manifoldArray[bulletManifoldIndex]->getNumContacts() > bulletPointCacheIndex &&
		(manifoldArray[bulletManifoldIndex]->getContactPoint(bulletPointCacheIndex).m_userPersistentData == this))
	{
		if (refreshContacts)
		{
			refreshIndividualPoint(swapped, pt0InWorld, pt1InWorld, manifoldArray);
		}

		if (swapped)
		{
			hasValidPoints = foundValidContactPointFromBulletManifold(manifoldArray[bulletManifoldIndex], pt1InWorld, pt0InWorld);
		}
		else
		{
			hasValidPoints = foundValidContactPointFromBulletManifold(manifoldArray[bulletManifoldIndex], pt0InWorld, pt1InWorld);
		}
	}

	if (hasValidPoints)
	{
		const btManifoldPoint& conPoint = manifoldArray[bulletManifoldIndex]->getContactPoint(bulletPointCacheIndex);

		contactPoint.position = pt0InWorld;
		contactPoint.normal = pt0InWorld - pt1InWorld;
		contactPoint.length = conPoint.getDistance() < 0.0 ? -contactPoint.normal.unitize() : contactPoint.normal.unitize();
	}
	else
	{
		contactPoint.length = 1.0;
	}
}

bool BulletShapeConnector::recalculateValidPoints(btManifoldArray& btManArray, Vector3& pt0InWorld, Vector3& pt1InWorld)
{
	btManArray.clear();

	btCollisionObjectWrapper obj0Wrap(0, bulletCollisionObject0->getCollisionShape(), bulletCollisionObject0 , bulletCollisionObject0 ->getWorldTransform(), -1, -1);
	btCollisionObjectWrapper obj1Wrap(0, bulletCollisionObject1->getCollisionShape(), bulletCollisionObject1, bulletCollisionObject1->getWorldTransform(), -1, -1);
	btManifoldResult contactPointResult(&obj0Wrap, &obj1Wrap);
	btDispatcherInfo disInfo;
	bulletAlgo->processCollision(&obj0Wrap, &obj1Wrap, disInfo, &contactPointResult);

	bulletAlgo->getAllContactManifolds(btManArray);
	if (bulletCollisionObject0->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE || 
		bulletCollisionObject1->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
	{
		//GImpact collision algorithm should only yield 1 Manifold
		RBXASSERT(btManArray.size() <= 1);
		if (btManArray.size())
		{
			btManArray[0]->refreshContactPoints(bulletCollisionObject0->getWorldTransform(), bulletCollisionObject1->getWorldTransform());
		}
	}


	// processCollision may swap points if we use btCompoundShape, we have to detect this.
	if (btManArray[bulletManifoldIndex]->getBody0() == bulletCollisionObject0)
	{
		if (foundValidContactPointFromBulletManifold(btManArray[bulletManifoldIndex], pt0InWorld, pt1InWorld))
			return true;
	}
	else
	{
		if (foundValidContactPointFromBulletManifold(btManArray[bulletManifoldIndex], pt1InWorld, pt0InWorld))
			return true;
	}

	return false;
}

bool BulletShapeConnector::foundValidContactPointFromBulletManifold(btPersistentManifold* man, Vector3& p0World, Vector3& p1World)
{
	realignConnectorsToBulletContacts();

	// After the bullet refresh, this connector may no longer be valid, so we must check and skip if necessary
	// To be valid, the bulletPointCacheIndex must within range and the m_userPersistentData must match "this" connector
	if ((bulletPointCacheIndex < man->getNumContacts() && man->getContactPoint(bulletPointCacheIndex).m_userPersistentData == this))
	{
		btManifoldPoint conPoint = man->getContactPoint(bulletPointCacheIndex);
		p1World = Vector3(conPoint.getPositionWorldOnB().x(), conPoint.getPositionWorldOnB().y(), conPoint.getPositionWorldOnB().z());
		p0World = p1World + conPoint.getDistance() * Vector3(conPoint.m_normalWorldOnB.x(), conPoint.m_normalWorldOnB.y(), conPoint.m_normalWorldOnB.z());

		return true;
	}
	else // skip this contact by making its length > 0 so it is ignored by the kernel force solve
		return false;
}

void BulletShapeConnector::realignConnectorsToBulletContacts()
{
	btManifoldArray manifoldArray;
	bulletAlgo->getAllContactManifolds(manifoldArray);
	for (int i = 0; i < manifoldArray[bulletManifoldIndex]->getNumContacts(); i++)
	{
		BulletShapeConnector* conn = rbx_static_cast<BulletShapeConnector*>(manifoldArray[bulletManifoldIndex]->getContactPoint(i).m_userPersistentData);
		if (conn)
			conn->setBulletManifoldPointIndex(i);
	}
}

void BulletShapeConnector::updateContactPoint()
{	
	//Prevent Nans from being calculated in Physics
	if (!validObjectCFrames())
		return;

	updateBulletCollisionObjects();
	updateConnectorPointFromManifold();

	ContactConnector::updateContactPoint();
}

void BulletShapeCellConnector::updateBulletCollisionObjects()
{
	geoPair.body1->updateBulletCollisionObject(bulletCollisionObject1);
}

void BulletShapeCellConnector::updateContactPoint()
{	
	updateBulletCollisionObjects();
	updateConnectorPointFromManifold();


	ContactConnector::updateContactPoint();
}


} // namespace
