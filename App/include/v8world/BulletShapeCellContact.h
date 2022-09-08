/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/CellContact.h"
#include "V8World/Mesh.h"
#include "Voxel/Util.h"

#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"


class btPersistentManifold;
class bulletNPAlgorithm;
class btConvexHullShape;
namespace RBX {
	class PolyConnector;
	class BulletShapeCellConnector;
	class BulletShapeCellContact : public CellMeshContact
	{
	public:
		typedef RBX::FixedArray<BulletShapeCellConnector*, BULLET_CONTACT_ARRAY_SIZE> BulletConnectorArray;

	private:
		btCollisionAlgorithm* bulletNPAlgorithm;

		btCollisionObject bulletCollisionObject; // collision object for the cell involved in contact

		shared_ptr<btCollisionShape> customShape;

		BulletConnectorArray polyConnectors;

		World* world;

		void removeAllConnectorsFromKernel();
		void putAllConnectorsInKernel();
		void updateClosestFeatures();
		float worstFeatureOverlap();
		void deleteConnectors(BulletConnectorArray& deleteConnectors);
		void matchClosestFeatures(BulletConnectorArray& newConnectors);
		BulletShapeCellConnector* matchClosestFeature(BulletShapeCellConnector* newConnector);
		// Terrain Materials
		void updateContactParemeters(btCollisionObject* cellObj, BulletConnectorArray& connectors);


		// use a BulletShapeConnector to represent this connector (we don't need a specific BulletShapeCellConnector)
		BulletShapeCellConnector* newBulletShapeCellConnector(btCollisionObject* bulletColObj0, btCollisionObject* bulletColObj1,
															  btCollisionAlgorithm* algo, int manifoldIndex, int contactIndex);

		void updateContactPoints();
		void computeManifoldsWithBulletNarrowPhase(btManifoldArray& manifoldArray);

		// Contact
        void deleteAllConnectors() override;
		int numConnectors() const override					{return polyConnectors.size();}
        ContactConnector* getConnector(int i) override;
        bool computeIsColliding(float overlapIgnored) override;
        bool stepContact() override;

		void invalidateContactCache() override;

		void findClosestFeatures(ConnectorArray& newConnectors) override {RBXASSERT(0);} // don't use this when using btCompound Narrow Phase
																							  // since it generates too many connectors
		void findClosestBulletCellFeatures(BulletConnectorArray& newConnectors);

	public:
        BulletShapeCellContact(Primitive* p0, Primitive* p1, const Vector3int16& cell, World* contactWorld);
        BulletShapeCellContact(Primitive* p0, Primitive* p1, const Vector3int32& feature, const shared_ptr<btCollisionShape>& customShape, World* contactWorld);
		~BulletShapeCellContact();
	};
} // namespace
