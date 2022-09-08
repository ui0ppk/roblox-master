/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "v8world/BulletContact.h"

#include "v8kernel/Kernel.h"
#include "v8world/Primitive.h"
#include "v8world/World.h"
#include "v8world/MaterialProperties.h"
#include "v8world/SmoothClusterGeometry.h"

#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"

DYNAMIC_FASTFLAGVARIABLE(UseTerrainCustomPhysicalProperties, false)

namespace RBX {

static void computeManifolds(World* world, btCollisionAlgorithm*& algorithm, btManifoldArray& manifoldArray, btCollisionObject* colObj0, btCollisionObject* colObj1)
{
	btCollisionObjectWrapper obj0Wrap(0, colObj0->getCollisionShape(), colObj0, colObj0->getWorldTransform(), -1, -1);
	btCollisionObjectWrapper obj1Wrap(0, colObj1->getCollisionShape(), colObj1, colObj1->getWorldTransform(), -1, -1);

	if (!algorithm)
		algorithm = world->getBulletCollisionDispatcher()->findAlgorithm(&obj0Wrap, &obj1Wrap);

	btManifoldResult contactPointResult(&obj0Wrap, &obj1Wrap);
	btDispatcherInfo disInfo;
	algorithm->processCollision(&obj0Wrap, &obj1Wrap, disInfo, &contactPointResult);
    
    manifoldArray.resize(0);
	algorithm->getAllContactManifolds(manifoldArray);

	if (colObj0->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE ||
		colObj1->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
	{
        // GImpact algorithm does not do refreshContactPoints so let's do it ourselves!
        for (int i = 0; i < manifoldArray.size(); ++i)
			manifoldArray[i]->refreshContactPoints(colObj0->getWorldTransform(), colObj1->getWorldTransform());
	}
}

static void matchConnectors(World* world, btManifoldArray& manifoldArray, BulletConnectorArray& connectors)
{
    // Mark all existing connectors as unused
    for (size_t i = 0; i < connectors.size(); ++i)
    {
        BulletConnector* conn = connectors[i];
        
        conn->bulletManifoldIndex = -1;
    }
    
    // Reassociate all contact points with existing connectors
    for (int i = 0; i < manifoldArray.size(); ++i)
    {
        btPersistentManifold* manifold = manifoldArray[i];
        
        for (int j = 0; j < manifold->getNumContacts(); ++j)
        {
            const btManifoldPoint& pt = manifold->getContactPoint(j);
            
            if (pt.getDistance() < 0)
            {
                if (pt.m_userPersistentData)
                {
                    BulletConnector* conn = static_cast<BulletConnector*>(pt.m_userPersistentData);
                    
                    RBXASSERT(conn->bulletManifoldIndex < 0);

                    conn->bulletManifoldIndex = i;
                    conn->bulletPointCacheIndex = j;
                }
            }
            else
            {
                pt.m_userPersistentData = NULL;
            }
        }
    }
    
    // Remove old connectors
    Kernel* kernel = NULL;

    for (size_t i = 0; i < connectors.size(); )
    {
        BulletConnector* conn = connectors[i];
        
        if (conn->bulletManifoldIndex < 0)
        {
            if (conn->isInKernel())
            {
                kernel = kernel ? kernel : world->getKernel(); // optimization - getKernel walks the IPipelines
                kernel->removeConnector(conn);
            }

            connectors.fastRemove(i);
            delete conn;
        }
        else
            ++i;
    }
}

static void createConnectors(btManifoldArray& manifoldArray, BulletConnectorArray& connectors, Body* body0, Body* body1, ContactParams* contactParams)
{
    for (int i = 0; i < manifoldArray.size(); ++i)
    {
        btPersistentManifold* manifold = manifoldArray[i];
        
        for (int j = 0; j < manifold->getNumContacts(); ++j)
        {
            btManifoldPoint& pt = manifold->getContactPoint(j);
            
            if (!pt.m_userPersistentData && pt.getDistance() < 0)
            {
                if (connectors.size() < connectors.capacity())
                {
                    BulletConnector* conn = new BulletConnector(body0, body1, *contactParams, i, j);
                    
                    connectors.push_back(conn);
                    
                    pt.m_userPersistentData = conn;
                }
                else
                {
                    RBXASSERT(!"Contact array is full, dropping contact");
                }
            }
        }
    }
}

static float updateContactPoints(btManifoldArray& manifoldArray, BulletConnectorArray& connectors, btCollisionObject* colObj0)
{
	float worstOverlap = -FLT_MAX;

	for (size_t i = 0; i < connectors.size(); ++i)
	{
		BulletConnector* conn = connectors[i];
        PairParams& connPoint = conn->getContactPoint();

		btPersistentManifold* manifold = manifoldArray[conn->bulletManifoldIndex];
		const btManifoldPoint& pt = manifold->getContactPoint(conn->bulletPointCacheIndex);

        Vector3 normalB = Vector3(pt.m_normalWorldOnB.x(), pt.m_normalWorldOnB.y(), pt.m_normalWorldOnB.z());
        float distance = pt.m_distance1;
        
        // GImpact likes to swap bodies
		if (manifold->getBody0() == colObj0)
		{
            connPoint.position = Vector3(pt.m_positionWorldOnA.x(), pt.m_positionWorldOnA.y(), pt.m_positionWorldOnA.z());
            connPoint.normal = -normalB;
            connPoint.length = distance;
        }
        else
        {
            connPoint.position = Vector3(pt.m_positionWorldOnB.x(), pt.m_positionWorldOnB.y(), pt.m_positionWorldOnB.z());
            connPoint.normal = normalB;
            connPoint.length = distance;
        }

		worstOverlap = std::max(worstOverlap, -connPoint.length);
	}
    
	return worstOverlap;
}

static bool areContactPointsCloserThan(const btManifoldArray& manifoldArray, float distance)
{
	for (int i = 0; i < manifoldArray.size(); ++i)
	{
		const btPersistentManifold* manifold = manifoldArray[i];

		for (int j = 0; j < manifold->getNumContacts(); ++j)
		{
			const btManifoldPoint& p = manifold->getContactPoint(j);

			if (p.getDistance() < distance)
				return true;
		}
	}
	
	return false;
}

static void removeConnectorsFromKernel(World* world, BulletConnectorArray& connectors)
{
	Kernel* kernel = NULL;

	for (size_t i = 0; i < connectors.size(); ++i)
    {
		if (connectors[i]->isInKernel())
        {
            kernel = kernel ? kernel : world->getKernel(); // optimization - getKernel walks the IPipelines
			kernel->removeConnector(connectors[i]);
		}
	}
}

static void putConnectorsInKernel(World* world, BulletConnectorArray& connectors)
{
    Kernel* kernel = NULL;

	for (size_t i = 0; i < connectors.size(); ++i)
    {
		if (!connectors[i]->isInKernel())
		{
            kernel = kernel ? kernel : world->getKernel(); // optimization - getKernel walks the IPipelines
			kernel->insertConnector(connectors[i]);
		}
	}
}

static void deleteConnectors(World* world, btManifoldArray& manifoldArray, BulletConnectorArray& connectors)
{
    removeConnectorsFromKernel(world, connectors);

    for (size_t i = 0; i < connectors.size(); ++i)
    {
        RBXASSERT(!connectors[i]->isInKernel());
        delete connectors[i];
    }
    
    connectors.fastClear();

    for (int i = 0; i < manifoldArray.size(); ++i)
        manifoldArray[i]->clearManifold();
    
    manifoldArray.resize(0);
}

static void deleteAlgorithm(World* world, btCollisionAlgorithm* algorithm)
{
    if (algorithm)
    {
        algorithm->~btCollisionAlgorithm();
        world->getBulletCollisionDispatcher()->freeCollisionAlgorithm(algorithm);
    }
}

static void clearCache(btManifoldArray& manifoldArray)
{
    for (int i = 0; i < manifoldArray.size(); ++i)
        manifoldArray[i]->clearManifold();
}

BulletContact::BulletContact(World* world, Primitive* p0, Primitive* p1)
: Contact(p0, p1)
, world(world)
, algorithm(NULL)
{
}

BulletContact::~BulletContact()
{
	deleteConnectors(world, manifoldArray, connectors);
	deleteAlgorithm(world, algorithm);
}

ContactConnector* BulletContact::getConnector(int i)	
{
	return connectors[i];
}

int BulletContact::numConnectors() const
{
    return connectors.size();
}

void BulletContact::deleteAllConnectors()
{
	deleteConnectors(world, manifoldArray, connectors);
}

bool BulletContact::stepContact() 
{
	bool result = computeIsColliding(0.0f);

	if (inKernel())
	{
		if (result)
			putConnectorsInKernel(world, connectors);
		else
			removeConnectorsFromKernel(world, connectors);
	}

	return result;
}

void BulletContact::invalidateContactCache()
{
	clearCache(manifoldArray);
}

bool BulletContact::computeIsColliding(float overlapIgnored)
{
	Primitive* prim0 = getPrimitive(0);
	Primitive* prim1 = getPrimitive(1);

	if (!Primitive::aaBoxCollide(*prim0, *prim1))
    {
		deleteConnectors(world, manifoldArray, connectors);
        return false;
    }

    if (!contactParams)
        generateDataForMovingAssemblyStage();

	prim0->updateBulletCollisionObject();
	prim1->updateBulletCollisionObject();

	computeManifolds(world, algorithm, manifoldArray, prim0->getGeometry()->getBulletCollisionObject(), prim1->getGeometry()->getBulletCollisionObject());
    matchConnectors(world, manifoldArray, connectors);
    createConnectors(manifoldArray, connectors, prim0->getBody(), prim1->getBody(), contactParams);

	float worstOverlap = updateContactPoints(manifoldArray, connectors, prim0->getGeometry()->getBulletCollisionObject());

	return worstOverlap > overlapIgnored;
}

BulletCellContact::BulletCellContact(World* world, Primitive* p0, Primitive* p1, const Vector3int32& feature, const shared_ptr<btCollisionShape>& cellShape)
: CellContact(p0, p1, feature)
, world(world)
, algorithm(NULL)
, cellShape(cellShape)
{
    btTransform identity;
    identity.setIdentity();

    cellCollisionObject.setWorldTransform(identity);
    cellCollisionObject.setCollisionShape(cellShape.get());
}

BulletCellContact::~BulletCellContact()
{
	deleteConnectors(world, manifoldArray, connectors);
	deleteAlgorithm(world, algorithm);
}

ContactConnector* BulletCellContact::getConnector(int i)
{
    return connectors[i];
}

int BulletCellContact::numConnectors() const
{
    return connectors.size();
}

void BulletCellContact::deleteAllConnectors()
{
	deleteConnectors(world, manifoldArray, connectors);
}

bool BulletCellContact::stepContact()
{
    bool result = computeIsColliding(0.0f);

	if (inKernel())
	{
		if (result)
			putConnectorsInKernel(world, connectors);
		else
			removeConnectorsFromKernel(world, connectors);
	}

	return result;
}

void BulletCellContact::invalidateContactCache()
{
	clearCache(manifoldArray);
}

bool BulletCellContact::computeIsColliding(float overlapIgnored)
{
	Primitive* prim0 = getPrimitive(0);
	Primitive* prim1 = getPrimitive(1);

    if (!contactParams)
        generateDataForMovingAssemblyStage();
    
    prim1->updateBulletCollisionObject();

	computeManifolds(world, algorithm, manifoldArray, &cellCollisionObject, prim1->getGeometry()->getBulletCollisionObject());
    matchConnectors(world, manifoldArray, connectors);
    createConnectors(manifoldArray, connectors, prim0->getBody(), prim1->getBody(), contactParams);
    
	float worstOverlap = updateContactPoints(manifoldArray, connectors, &cellCollisionObject);

	updateContactParemeters(&cellCollisionObject);
    
	// we may have no connectors but we want to check if primitive is "close enough" to terrain, so we have to go through Bullet manifold
	if (overlapIgnored < 0)
		return areContactPointsCloserThan(manifoldArray, -overlapIgnored);
	else
		return worstOverlap > overlapIgnored;
}

BulletConnector::BulletConnector(Body* b0, Body* b1, const ContactParams& contactParams, int manifoldIndex, int cacheIndex)
: ContactConnector(b0, b1, contactParams)
, bulletManifoldIndex(manifoldIndex)
, bulletPointCacheIndex(cacheIndex)
{
}

void BulletCellContact::updateContactParemeters(btCollisionObject* cellObj)
{
	if (!(world->getUsingNewPhysicalProperties()))
		return;

	for (size_t i = 0; i < connectors.size(); ++i)
	{
		BulletConnector* conn = connectors[i];
		PairParams& connPoint = conn->getContactPoint();

		btPersistentManifold* manifold = manifoldArray[conn->bulletManifoldIndex];
		const btManifoldPoint& pt = manifold->getContactPoint(conn->bulletPointCacheIndex);
		ContactParams newParams;
		int triangleIndex = manifold->getBody0() == cellObj ? pt.m_index0 : pt.m_index1;

		Primitive* terrainPrimitive = getPrimitive(0);
		if (DFFlag::UseTerrainCustomPhysicalProperties && terrainPrimitive->getPhysicalProperties().getCustomEnabled())
		{
			MaterialProperties::updateContactParamsPrims(newParams, getPrimitive(1), terrainPrimitive);
			conn->setContactParams(newParams);
		}
		else
		{
			PartMaterial terrainContactMaterial = SmoothClusterGeometry::getTriangleMaterial(cellObj->getCollisionShape(), triangleIndex, connPoint.position);
			MaterialProperties::updateContactParamsPrimMaterial(newParams, getPrimitive(1), terrainPrimitive, terrainContactMaterial);
			conn->setContactParams(newParams);
		}
	}
}

void BulletCellContact::onPrimitiveContactParametersChanged()
{
	if (!world->getUsingNewPhysicalProperties())
	{
		Contact::onPrimitiveContactParametersChanged();
	}
}

} // namespace
