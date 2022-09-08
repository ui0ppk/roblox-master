/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/BulletShapeCellContact.h"
#include "V8World/MegaClusterPoly.h"
#include "voxel/Cell.h"
#include "voxel/Grid.h"
#include "V8DataModel/MegaCluster.h"
#include "v8world/SmoothClusterGeometry.h"
#include "V8Kernel/BulletShapeConnectors.h"
#include "V8Kernel/Kernel.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"

#include "BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"

DYNAMIC_FASTFLAG(UseTerrainCustomPhysicalProperties)

namespace RBX {

using namespace Voxel;

BulletShapeCellContact::BulletShapeCellContact(Primitive* p0, Primitive* p1, const Vector3int16& cell, World* contactWorld)
	: CellMeshContact(p0, p1, Vector3int32(cell))
	, bulletNPAlgorithm(NULL)
	, world(contactWorld)
{
    Grid* grid = static_cast<MegaClusterInstance*>(p0->getOwner())->getVoxelGrid();

    Cell cellData = grid->getCell(cell);

    CellOrientation orientation = cellData.solid.getOrientation();
    CellBlock type = cellData.solid.getBlock();	
    Vector3 cellOffset = kCELL_SIZE * Vector3(cell) + Vector3(Voxel::kHALF_CELL, Voxel::kHALF_CELL, Voxel::kHALF_CELL);
    CoordinateFrame cellCFrame = CoordinateFrame(cellOffset);
    if (orientation > 0)
        Math::rotateAboutYGlobal(cellCFrame, orientation*Math::piHalff());

    btCollisionShape* bulletCellShape = rbx_static_cast<MegaClusterPoly*>(p0->getGeometry())->getBulletCellShape(type);

    // set up collision object
    bulletCollisionObject.setWorldTransform(cellCFrame.transformFromCFrame());
    bulletCollisionObject.setCollisionShape(bulletCellShape);
}

BulletShapeCellContact::BulletShapeCellContact(Primitive* p0, Primitive* p1, const Vector3int32& feature, const shared_ptr<btCollisionShape>& customShape, World* contactWorld)
	: CellMeshContact(p0, p1, feature)
	, bulletNPAlgorithm(NULL)
	, customShape(customShape)
	, world(contactWorld)
{
	btTransform identity;
	identity.setIdentity();

    bulletCollisionObject.setWorldTransform(identity);
    bulletCollisionObject.setCollisionShape(customShape.get());
}

BulletShapeCellContact::~BulletShapeCellContact()
{
	deleteConnectors(polyConnectors);

	if (bulletNPAlgorithm)
	{
		bulletNPAlgorithm->~btCollisionAlgorithm();
		world->getBulletCollisionDispatcher()->freeCollisionAlgorithm(bulletNPAlgorithm);
	}

	RBXASSERT(polyConnectors.size() == 0);
}

ContactConnector* BulletShapeCellContact::getConnector(int i)	
{
	return polyConnectors[i];
}

void BulletShapeCellContact::deleteAllConnectors()
{
	deleteConnectors(polyConnectors);
}


void BulletShapeCellContact::deleteConnectors(BulletConnectorArray& deleteConnectors)
{
	removeAllConnectorsFromKernel();

	for (size_t i = 0; i < deleteConnectors.size(); ++i) {
		RBXASSERT(!deleteConnectors[i]->isInKernel());
		delete deleteConnectors[i];
	}

	deleteConnectors.fastClear();
}


void BulletShapeCellContact::removeAllConnectorsFromKernel()
{
	Kernel* kernel = NULL;
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		if (polyConnectors[i]->isInKernel()) {
			kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
			kernel->removeConnector(polyConnectors[i]);
		}
	}
}

void BulletShapeCellContact::putAllConnectorsInKernel()
{
	Kernel* kernel = NULL;
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		if (!polyConnectors[i]->isInKernel())
		{
			kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
			kernel->insertConnector(polyConnectors[i]);
		}
	}
}

bool BulletShapeCellContact::stepContact() 
{
	if (computeIsColliding(0.0f)) {
		if (inKernel()) {
			putAllConnectorsInKernel();
		}
		return true;
	}
	else {
		removeAllConnectorsFromKernel();
		return false;
	}
}

void BulletShapeCellContact::invalidateContactCache() 
{
	deleteAllConnectors();

	if (bulletNPAlgorithm)
	{
		btManifoldArray manifoldArray;
		bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

		for (int i = 0; i < manifoldArray.size(); ++i)
            manifoldArray[i]->clearManifold();
	}
}

bool BulletShapeCellContact::computeIsColliding(float overlapIgnored)
{
	updateClosestFeatures();
	if (polyConnectors.size() > 0) 
	{
		float overlap = worstFeatureOverlap();
		if (overlap > overlapIgnored) 
		{
			return true;
		}
	}
	else if (overlapIgnored < 0)
	{
		// we have no connectors but we want to check if primitive is "close enough" to terrain, so we have to go through Bullet manifold
		btManifoldArray manifoldArray;
		bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

		for (int i = 0; i < manifoldArray.size(); ++i)
		{
			btPersistentManifold* manifold = manifoldArray[i];

			for (int j = 0; j < manifold->getNumContacts(); ++j)
			{
				btManifoldPoint& p = manifold->getContactPoint(j);

				if (p.getDistance() < -overlapIgnored)
					return true;
			}
		}
	}

	return false;
}

void BulletShapeCellContact::updateClosestFeatures()
{
	BulletConnectorArray newConnectors;

	findClosestBulletCellFeatures(newConnectors);
	
	matchClosestFeatures(newConnectors);

	deleteConnectors(polyConnectors);		// any remaining not matched

	// set bullet's manifold point references back to roblox kernel connectors
	for (size_t i = 0; i < newConnectors.size(); ++i)
	{
		btManifoldArray manifoldArray;
		bulletNPAlgorithm->getAllContactManifolds(manifoldArray);
		int manIndex = newConnectors[i]->getBulletManifoldIndex();
		int ptIndex = newConnectors[i]->getBulletPointCacheIndex();
		manifoldArray[manIndex]->getContactPoint(ptIndex).m_userPersistentData = newConnectors[i];
	}

	polyConnectors = newConnectors;			// transfer over the pointers

	updateContactPoints();

	updateContactParemeters(&bulletCollisionObject, polyConnectors);
}

void BulletShapeCellContact::updateContactParemeters(btCollisionObject* cellObj, BulletConnectorArray& connectors)
{
	// Prevent this code from Running on OLD Terrain that cannot use SmoothClusterGeometry
	// to get Triangle
	if (!(getPrimitive(0)->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER ||
		  getPrimitive(1)->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER))
		return;

	if (!(world->getUsingNewPhysicalProperties()))
		return;

	btManifoldArray manifoldArray;
	bulletNPAlgorithm->getAllContactManifolds(manifoldArray);
	for (size_t i = 0; i < connectors.size(); ++i)
	{
		BulletShapeConnector* conn = connectors[i];
		PairParams& connPoint = conn->getContactPoint();

		// In the old Bullet-Terrain code we have to make sure that
		// the indices are valid.
		if (manifoldArray.size() <= conn->getBulletManifoldIndex())
			continue;

		btPersistentManifold* manifold = manifoldArray[conn->getBulletManifoldIndex()];

		if (manifold->getNumContacts() <= conn->getBulletPointCacheIndex())
			continue; 

		const btManifoldPoint& pt = manifold->getContactPoint(conn->getBulletPointCacheIndex());
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

float BulletShapeCellContact::worstFeatureOverlap()
{
	float worstOverlap = -FLT_MAX;		// i.e. not overlapping
	RBXASSERT(polyConnectors.size() > 0);
	for (size_t i = 0; i < polyConnectors.size(); ++i) {				// may not have any overlapping features!
		float overlap = polyConnectors[i]->computeOverlap();				// computeLength returns negative
		worstOverlap = std::max(worstOverlap, overlap);
	}
	return worstOverlap;
}

void BulletShapeCellContact::matchClosestFeatures(BulletConnectorArray& newConnectors)
{
	for (size_t i = 0; i < newConnectors.size(); ++i) {
		if (BulletShapeCellConnector* match = matchClosestFeature(newConnectors[i])) {
			delete newConnectors[i];
			newConnectors.replace(i, match);
		}
	}
}

BulletShapeCellConnector* BulletShapeCellContact::matchClosestFeature(BulletShapeCellConnector* newConnector)
{
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		BulletShapeCellConnector* answer = polyConnectors[i];
		if (BulletShapeCellConnector::match(answer, newConnector)) {
			polyConnectors.fastRemove(i);
			return answer;
		}
	}
	return NULL;
}

void BulletShapeCellContact::updateContactPoints()
{
	for (size_t i = 0; i < polyConnectors.size(); ++i)
	{
		polyConnectors[i]->findValidContactAfterNarrowphase();
	}
}

// use a BulletShapeConnector to represent this connector (we don't need a specific BulletShapeCellConnector)
BulletShapeCellConnector* BulletShapeCellContact::newBulletShapeCellConnector(btCollisionObject* bulletColObj0, btCollisionObject* bulletColObj1, 
																	  btCollisionAlgorithm* algo, int manifoldIndex, int contactIndex)
{
    if(contactParams)
    {
		return new BulletShapeCellConnector(getPrimitive(0)->getBody(),
											getPrimitive(1)->getBody(),
											*contactParams,
											bulletColObj0,
											bulletColObj1,
											algo,
											manifoldIndex,
											contactIndex
											);			// id of this face
    }
    else
        return NULL;
}

void BulletShapeCellContact::findClosestBulletCellFeatures(BulletConnectorArray& newConnectors)
{
   	if (!contactParams)
        generateDataForMovingAssemblyStage();

	// prim index 0 is for terrain, 1 is for CSG prim
	// only need to update the CSG collision object since terrain is not moving
	// Early out if CFrame is invalid
	if (Math::isNanInfVector3(getPrimitive(1)->getCoordinateFrame().translation))
		return;

	getPrimitive(1)->updateBulletCollisionObject();

	btManifoldArray manifoldArray;
	computeManifoldsWithBulletNarrowPhase(manifoldArray);

	int totalNumConnectors = 0;
	for (int i = 0; i < manifoldArray.size(); i++)
	{
		btPersistentManifold* manifold = manifoldArray[i];

		for (int j = 0; j < manifold->getNumContacts(); j++)
		{
			btManifoldPoint& p = manifold->getContactPoint(j);

			bool valid = manifold->validContactDistance(p);
			float dis = p.getDistance();

			if (valid && dis < 0.0f)
			{
				RBXASSERT(totalNumConnectors < BULLET_CONTACT_ARRAY_SIZE);
				if (totalNumConnectors < BULLET_CONTACT_ARRAY_SIZE)
				{
					totalNumConnectors++;
					BulletShapeCellConnector* cylConn = newBulletShapeCellConnector(&bulletCollisionObject, 
																			getPrimitive(1)->getGeometry()->getBulletCollisionObject(),
																			bulletNPAlgorithm, i, j);
					if (cylConn)
						newConnectors.push_back(cylConn);
				}
			}
		}
	}
}

void BulletShapeCellContact::computeManifoldsWithBulletNarrowPhase(btManifoldArray& manifoldArray)
{
	btCollisionDispatcher* dispatcher;
	dispatcher = world->getBulletCollisionDispatcher();

	btCollisionObject* colObj0 = &(bulletCollisionObject);
	btCollisionObject* colObj1 = getPrimitive(1)->getGeometry()->getBulletCollisionObject();

	btCollisionObjectWrapper obj0Wrap(0, colObj0->getCollisionShape(), colObj0, colObj0->getWorldTransform(), -1, -1);
	btCollisionObjectWrapper obj1Wrap(0, colObj1->getCollisionShape(), colObj1, colObj1->getWorldTransform(), -1, -1);

	if (!bulletNPAlgorithm)
		bulletNPAlgorithm = dispatcher->findAlgorithm(&obj0Wrap,&obj1Wrap);

	btManifoldResult contactPointResult(&obj0Wrap, &obj1Wrap);
	btDispatcherInfo disInfo;
	bulletNPAlgorithm->processCollision(&obj0Wrap, &obj1Wrap, disInfo, &contactPointResult);	
	bulletNPAlgorithm->getAllContactManifolds(manifoldArray);

	if (colObj0->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE || 
		colObj1->getCollisionShape()->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
	{
		//GImpact collision algorithm should only yield 1 Manifold
		RBXASSERT(manifoldArray.size() <= 1);
		if (manifoldArray.size())
		{
			manifoldArray[0]->refreshContactPoints(colObj0->getWorldTransform(), colObj1->getWorldTransform());
		}
	}
}

} // namespace
