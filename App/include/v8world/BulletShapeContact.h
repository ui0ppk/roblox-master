/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Contact.h"
#include "V8World/Mesh.h"
#include "Voxel/Util.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

class btPersistentManifold;
class bulletNPAlgorithm;
class btConvexHullShape;
namespace RBX {
	class PolyConnector;
	class BulletShapeConnector;
	class BulletShapeContact : public Contact 
	{
	public:
		typedef RBX::FixedArray<BulletShapeConnector*, BULLET_CONTACT_ARRAY_SIZE> BulletConnectorArray;

	private:
		btPersistentManifold* bulletManifold;
		btCollisionAlgorithm* bulletNPAlgorithm;
		BulletConnectorArray polyConnectors;
		World* world;

		void removeAllConnectorsFromKernel();
		void putAllConnectorsInKernel();
		void updateClosestFeatures();
		float worstFeatureOverlap();
		void matchClosestFeatures(BulletConnectorArray& newConnectors);
		BulletShapeConnector* matchClosestFeature(BulletShapeConnector* newConnector);
		void deleteConnectors(BulletConnectorArray& deleteConnectors);
		BulletShapeConnector* newBulletShapeConnector(btCollisionObject* bulletColObj0, btCollisionObject* bulletColObj1,
													  btCollisionAlgorithm* algo, int manifoldIndex, int contactIndex, bool swapped);
		void updateContactPoints();
		void computeManifoldsWithBulletNarrowPhase(btManifoldArray& manifoldArray);

		// Contact
        void deleteAllConnectors() override;
		int numConnectors() const override					{return polyConnectors.size();}
        ContactConnector* getConnector(int i) override;
        bool computeIsColliding(float overlapIgnored) override;
        bool stepContact() override;

		void invalidateContactCache() override;

		/*implement*/ void findClosestFeatures(BulletConnectorArray& newConnectors);

	public:
		BulletShapeContact(Primitive* p0, Primitive* p1, World* ourWorld);
		~BulletShapeContact();
	};
} // namespace
