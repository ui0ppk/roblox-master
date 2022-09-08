/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/ContactManager.h"
#include "V8World/ContactManagerSpatialHash.h"
#include "V8World/BallPolyContact.h"
#include "V8World/PolyPolyContact.h"
#include "V8World/PolyCellContact.h"
#include "V8World/BallCellContact.h"
#include "V8World/BulletShapeContact.h"
#include "V8World/BulletShapeCellContact.h"
#include "V8World/BulletContact.h"
#include "V8World/TriangleMesh.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "V8World/Contact.h"
#include "V8World/Joint.h"
#include "V8world/Mechanism.h"
#include "V8World/SpatialFilter.h"
#include "rbx/Debug.h"
#include "Util/Profiling.h"
#include "V8DataModel/MegaCluster.h"
#include "V8World/MegaClusterPoly.h"
#include "V8World/SmoothClusterGeometry.h"
#include "v8world/Buoyancy.h"
#include "Voxel/Grid.h"
#include "rbx/DenseHash.h"
#include "v8world/Tolerance.h"

#include "voxel2/Grid.h"
#include "voxel2/Conversion.h"

#include <boost/math/special_functions/fpclassify.hpp>

#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"

#include "rbx/Profiler.h"

LOGGROUP(TerrainCellListener)
DYNAMIC_LOGVARIABLE(DeferredVoxelUpdates, 0)
DYNAMIC_LOGGROUP(PartStreamingRequests)
FASTINTVARIABLE(IntersectingOthersCallsAllowedOnSpawn, 5)

DYNAMIC_FASTFLAGVARIABLE(ContactManagerOptimizedQueryExtents, false)

#define LARGE_TERRAIN_CONTACT_VOLUME (4 * 4 * 4)

namespace RBX {

Vector3 ContactManager::dummySurfaceNormal;
PartMaterial ContactManager::dummySurfaceMaterial;
    
ContactManager::ContactManager(World* world)
	: world(world),
      myMegaClusterPrim(NULL)
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
	, spatialHash(new ContactManagerSpatialHash(world, this))
	, profilingBroadphase(new Profiling::CodeProfiler("Broadphase"))
#pragma warning(pop)
{}

ContactManager::~ContactManager()
{
	WriteValidator writeValidator(concurrencyValidator);
	delete spatialHash;
}

void ContactManager::fastClear()
{
	spatialHash->fastClear();
}

void ContactManager::doStats()			// spit out hash stats
{
	spatialHash->doStats();
}

bool ContactManager::intersectingMySimulation(Primitive* check, RBX::SystemAddress myLocalAddress, float overlapIgnored)
{
	RBXASSERT_FISHING(check->getNetworkOwner() != myLocalAddress);

	if (Assembly* assembly = check->getAssembly()) {
		Contact* c = check->getFirstContact();
		while (c) {
			Primitive* other = c->otherPrimitive(check);
			
			if (Assembly* otherAssembly = other->getAssembly()) {
				if (	(otherAssembly != assembly)
					&&	(otherAssembly->getAssemblyPrimitive()->getNetworkOwner() == myLocalAddress)
					&&	(c->computeIsCollidingUi(overlapIgnored))	)
				{
					return true;
				}
			}
			c = check->getNextContact(c);
		}
	}
	return false;
}

bool ContactManager::intersectingOthers(Primitive* check, const std::set<Primitive*>& checkSet, float overlapIgnored)
{
	Contact* c = check->getFirstContact();


	while (c) {
		Primitive* other = c->otherPrimitive(check);

		if (	(checkSet.find(other) == checkSet.end())
			&&	c->computeIsCollidingUi(overlapIgnored)  ) 
		{
			return true;
		}
		c = check->getNextContact(c);
	}

	// This is to support US15509 Deferred Contact for memory saving 
	DenseHashSet<Primitive*> primsFound(NULL);
	getSpatialHash()->getPrimitivesOverlapping(check->getFastFuzzyExtents(), primsFound);

	if (primsFound.size() == 0)
		return false;

	for (DenseHashSet<Primitive*>::const_iterator it = primsFound.begin(); primsFound.end() != it; ++it) 
	{
		if (checkSet.find(*it) != checkSet.end())
			continue;
		Contact* c = createContact(check, *it);
		if (!c)
			continue;
		bool colliding = c->computeIsCollidingUi(overlapIgnored);
		delete c;
		if (colliding)
			return true;
	}

	return false;
}


bool ContactManager::intersectingOthers(const G3D::Array<Primitive*>& check, float overlapIgnored)
{
	std::set<Primitive*> checkSet(check.begin(), check.end());

	for (int i = 0; i < check.size(); ++i) {
		if (intersectingOthers(check[i], checkSet, overlapIgnored)) {
			return true;
		}
	}
	return false;
}

bool ContactManager::intersectingOthers(Primitive* check, float overlapIgnored)
{
	std::set<Primitive*> checkSet;
	checkSet.insert(check);

	return intersectingOthers(check, checkSet, overlapIgnored);
}

shared_ptr<const Instances> ContactManager::getPartCollisions(Primitive* check)
{
	shared_ptr<Instances> instanceList = shared_ptr<Instances>(new Instances());

	Contact* c = check->getFirstContact();
	while (c) {
		Primitive* other = c->otherPrimitive(check);

		if (check != other)
		{
			PartInstance* partInstance = PartInstance::fromPrimitive(other);

			if (RBX::Instance::fastDynamicCast<MegaClusterInstance>(partInstance))
			{
				shared_ptr<Instance> instance = shared_from(static_cast<Instance*>(partInstance));
				if (c->computeIsCollidingUi(2E-4))
				{
					instanceList->push_back(instance);
					break;
				}
			}
		}
		c = check->getNextContact(c);
	}	

	DenseHashSet<Primitive*> primsFound(NULL);
	getSpatialHash()->getPrimitivesOverlapping(check->getFastFuzzyExtents(), primsFound);
	
	for (DenseHashSet<Primitive*>::const_iterator it = primsFound.begin(); primsFound.end() != it; ++it) 
	{
		if (*it == check)
			continue;

		Contact* c = createContact(check, *it);
		if (!c)
			continue;
		bool colliding = c->computeIsCollidingUi(2E-4);
		delete c;

		if (colliding)
		{
			PartInstance* partInstance = PartInstance::fromPrimitive(*it);

			if (partInstance)
			{
				shared_ptr<Instance> instance = shared_from(partInstance);

				instanceList->push_back(instance);
			}
		}
	}

	return instanceList;
}

static void CheckPrimitive(Primitive* primitive, const Extents* extents, G3D::Array<Primitive*>* found)
{
	if (extents->overlapsOrTouches(primitive->getFastFuzzyExtents())) {
		found->append(primitive);
	}
}

void ContactManager::getPrimitivesTouchingExtents(const Extents& extents,
												  const Primitive* ignore,
												  int maxCount, 
												  G3D::Array<Primitive*>& found)
{
	boost::unordered_set<Primitive*> tempBuffer;
	spatialHash->getPrimitivesTouchingGrids(extents, ignore, maxCount, tempBuffer);
	std::for_each(tempBuffer.begin(), tempBuffer.end(), boost::bind(&CheckPrimitive, _1, &extents, &found));
}

// does the same as above, just passes an Instance* instead of a Primitive* [is there a shared base class for both Primitive and Instance? didn't see one...
void ContactManager::getPrimitivesTouchingExtents(const Extents& extents,
                                                  const boost::unordered_set<const Primitive*>& ignorePrimitives,
                                                  int maxCount,
                                                  G3D::Array<Primitive*>& found)
{
	boost::unordered_set<Primitive*> tempBuffer;
	spatialHash->getPrimitivesTouchingGrids(extents, ignorePrimitives, maxCount, tempBuffer);
	std::for_each(tempBuffer.begin(), tempBuffer.end(), boost::bind(&CheckPrimitive, _1, &extents, &found));
}

void ContactManager::getPrimitivesOverlapping(const Extents& extents, DenseHashSet<Primitive*>& result)
{
	if (DFFlag::ContactManagerOptimizedQueryExtents)
		spatialHash->getPrimitivesOverlappingRec(extents, result);
	else
		spatialHash->getPrimitivesOverlapping(extents, result);
}

Primitive* ContactManager::getSlowHit(	const G3D::Array<Primitive*>& primitives,
										const RbxRay& unitRay, 
										const G3D::Array<const Primitive*>& ignorePrim,
										const HitTestFilter* filter,
										Vector3& hitPoint,
                                        Vector3& surfaceNormal,
                                        PartMaterial& surfaceMaterial,
										float maxDistance,
										bool& stopped) const
{
	RBXASSERT(unitRay.direction().isUnit());

	Primitive* bestPrimitive = NULL;
	float bestOffset = maxDistance;
	float stopOffset = maxDistance;
	stopped = false;

	for (int i = 0; i < primitives.size(); ++i) 
	{
		Primitive* p = primitives[i];
		if (!p->getBody()) {
			RBXCRASH("getSlowHit with deleted primitive");
		}

		bool dontIgnore = ignorePrim.find(p) == ignorePrim.end();
		HitTestFilter::Result filterResult = filter ? filter->filterResult(p) : HitTestFilter::INCLUDE_PRIM;
		if (dontIgnore && (filterResult != HitTestFilter::IGNORE_PRIM)) {
			Vector3 worldPos = p->getCoordinateFrame().translation;
			float radius = p->getRadius();

			Vector3 rayToModel = worldPos - unitRay.origin();
			float rayToPerpPoint = unitRay.direction().dot(rayToModel);

			// perp point could be behind the ray - example: very large plane.  This is just the center of the object
			Vector3 perpPointOnRay = unitRay.origin() + (rayToPerpPoint * unitRay.direction());	// on the ray
			Vector3 perpPointToCenter = worldPos - perpPointOnRay;
			float perpOffset = perpPointToCenter.length();

			// first test - just check a circle....
			if (perpOffset <= radius) 
			{
				// second test - G3D collision detection stuff
				Vector3 thisHitPoint;
                Vector3 thisSurfaceNormal;
				if (p->hitTest(unitRay, thisHitPoint, thisSurfaceNormal)) {
					float distanceToHit = unitRay.direction().dot(thisHitPoint - unitRay.origin());
					if (distanceToHit > 0.0f)	// i.e. - can't hit behind the origin
					{
						switch (filterResult)
						{
						case HitTestFilter::INCLUDE_PRIM:
							{
								if (distanceToHit < bestOffset) {
									bestOffset = distanceToHit;
									bestPrimitive = p;
									hitPoint = thisHitPoint;
                                    surfaceNormal = thisSurfaceNormal;
									surfaceMaterial = PartInstance::fromPrimitive(p)->getRenderMaterial();
								}
								break;
							}
						case HitTestFilter::STOP_TEST:
							{
								if (distanceToHit < stopOffset) {
									stopOffset = distanceToHit;
									stopped = true;
								}
								break;
							}
						default:
							{
								RBXASSERT(0);
								break;
							}
						}
					}
				}
			}
		}
	}

	if (stopped && bestPrimitive && (bestOffset < stopOffset)) { 
		stopped = false;
	}
	return bestPrimitive;
}


Primitive* ContactManager::getHit(	const RbxRay& worldRay, 
									const std::vector<const Primitive*>* ignorePrim,
									const HitTestFilter* filter,
                                    Vector3& hitPoint,
									bool terrainCellsAreCubes,
                                    bool ignoreWater,
                                    Vector3& surfaceNormal,
                                    PartMaterial& surfaceMaterial) const
{
	ReadOnlyValidator readOnlyValidator(concurrencyValidator);

	G3D::Array<const Primitive*> tempIgnore;
    
    if (ignorePrim)
    	tempIgnore = *ignorePrim;

	bool stopped;
	Primitive* answer = getFastHit(	worldRay,
									tempIgnore,
									filter,
                                    hitPoint,
									stopped,
									terrainCellsAreCubes,
									ignoreWater,
                                    surfaceNormal,
                                    surfaceMaterial);

	return stopped ? NULL : answer;
}


// TODO: Refactor to user std::vector, begin/end iterators
Primitive* ContactManager::getFastHit(	const RbxRay& worldRay, 
										const G3D::Array<const Primitive*>& ignorePrim,
										const HitTestFilter* filter,
                                        Vector3& hitPointWorld,
										bool& stopped,
										bool terrainCellsAreCubes,
                                        bool ignoreWater,
                                        Vector3& surfaceNormal,
                                        PartMaterial& surfaceMaterial) const
{
	RBXPROFILER_SCOPE("Physics", "getFastHit");

	// don't do raycast if incoming ray does not have good (i.e. finite)
	// floating point values in it.
	for (int index = 0; index < 3; ++index) {
		if (!boost::math::isfinite(worldRay.origin()[index]) ||
				!boost::math::isfinite(worldRay.direction()[index])) {
			hitPointWorld = worldRay.direction() + worldRay.origin();
			stopped = false; // filter didn't stop
			return NULL;
		}
	}

	RBXASSERT(worldRay.direction().magnitude() < 5000.0f);

	G3D::Array<Primitive*> primitives;
	Vector3int32 grid = SpatialHashStatic::realToHashGrid(0, worldRay.origin());

	float maxDistance = worldRay.direction().magnitude();
	RBXASSERT(maxDistance < 5000.0f);
	maxDistance = std::min(5000.0f, maxDistance);
	const RbxRay unitRay = worldRay.unit();

	// check first against megacluster, so only have to check once
	Vector3 megaHitPointWorld;
    Vector3 megaSurfaceNormal;
    PartMaterial megaSurfaceMaterial;
	float distanceToMegaHit = -1;
	if ( myMegaClusterPrim && unitRay.direction().isUnit()){  // the latter check is necessary, unfortunately
		bool dontIgnore = ignorePrim.find(myMegaClusterPrim) == ignorePrim.end();
		HitTestFilter::Result filterResult = filter ? filter->filterResult(myMegaClusterPrim) : HitTestFilter::INCLUDE_PRIM;
		if (dontIgnore && filterResult == HitTestFilter::INCLUDE_PRIM)
		{
            bool collided;
			if (myMegaClusterPrim->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
            {
                unsigned char voxelMaterial = Voxel2::Conversion::kMaterialDefault;
				collided = static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->castRay(unitRay, megaHitPointWorld, megaSurfaceNormal, voxelMaterial, maxDistance, ignoreWater);
                megaSurfaceMaterial = Voxel2::Conversion::getMaterialFromVoxelMaterial(voxelMaterial);

            }
			else
            {
                collided = static_cast<MegaClusterPoly*>(myMegaClusterPrim->getGeometry())->hitTest(unitRay, megaHitPointWorld, megaSurfaceNormal, maxDistance, terrainCellsAreCubes, ignoreWater);
                megaSurfaceMaterial = GRASS_MATERIAL;
            }

			distanceToMegaHit = unitRay.direction().dot(megaHitPointWorld - unitRay.origin());
			if (!collided) distanceToMegaHit = maxDistance;
		}
	}

	do {
		primitives.fastClear();
		spatialHash->getPrimitivesInGrid(grid, primitives);

		if (Primitive* answer = getSlowHit(	primitives,
											unitRay,
											ignorePrim,
											filter,
											hitPointWorld,
                                            surfaceNormal,
                                            surfaceMaterial,
											maxDistance,
											stopped))
		{
			Extents hashGrid = SpatialHashStatic::hashGridToRealExtents(0, grid);
			if (hashGrid.fuzzyContains(hitPointWorld, 0.001f)) {
				if ( myMegaClusterPrim && distanceToMegaHit > 0.0f ){
					float thisDistanceToHit = unitRay.direction().dot(hitPointWorld - unitRay.origin());
					if (thisDistanceToHit < distanceToMegaHit) return answer;
				}
				else return answer;
			}
		}

		if ( myMegaClusterPrim && distanceToMegaHit > 0.0f && distanceToMegaHit < maxDistance){
			Extents hashGrid = SpatialHashStatic::hashGridToRealExtents(0, grid);
			if (hashGrid.fuzzyContains(megaHitPointWorld, .001f)){
				hitPointWorld = megaHitPointWorld;
                surfaceNormal = megaSurfaceNormal;
                surfaceMaterial = megaSurfaceMaterial;
				return myMegaClusterPrim;
			}
		}
	}
	while (spatialHash->getNextGrid(grid, unitRay, maxDistance));

	return NULL;
}

bool ContactManager::terrainCellsInRegion3(Region3 region) const
{
	if (!myMegaClusterPrim)
		return false;

	Vector3 minV = region.minPos();
	Vector3 maxV = region.maxPos();

	if (myMegaClusterPrim->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
		return static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->findCellsInBoundingBox(minV, maxV);
	else
		return static_cast<MegaClusterPoly*>(myMegaClusterPrim->getGeometry())->cellsInBoundingBox(minV, maxV);
}


template <class Set>
static bool anyExtentsOverlapsOrTouchesPrimitives(const Extents &origin, const Set &primitives)
{
	for (typename Set::const_iterator it = primitives.begin(); primitives.end() != it; ++it) {
		Extents primExtents = (*it)->getExtentsWorld();
		if (primExtents.overlapsOrTouches(origin)) {
			return true;
		}
	}
	return false;
}

// Perform a linear scan up to find a place large enough in searchExtents to fit spaceNeeded.
Vector3 ContactManager::findUpNearestLocationWithSpaceNeeded(const float maxSearchDepth, const Vector3 &startCenter,
															 const Vector3 &spaceNeededToCorner)
{
	bool foundFittingLocation = false;
	Vector3 calcCenter = startCenter;
	int intersectingOthersChecks = 0;

	for (Vector3 searchCenter = startCenter; !foundFittingLocation && searchCenter.y <= maxSearchDepth;) {
		// Create a search box twice the height of the spaceNeededToCorner so we catch collisions
		// above us as we pop our "local box" on top of primitives found in the first half
		// of our search box.
		Extents searchExtents = Extents::fromCenterCorner(
			searchCenter + Vector3(0.0f, spaceNeededToCorner.y / 2.0f, 0.0f),
			spaceNeededToCorner * Vector3(1.0f, 2.0f, 1.0f));

		DenseHashSet<Primitive*> primsFound(NULL);
		getSpatialHash()->getPrimitivesOverlapping(searchExtents, primsFound);

		if (primsFound.size()) {
			// Scan any primitives in searchExtents and find the lowest available location above us, if there is one fitting spaceNeededToCorner.
			for (DenseHashSet<Primitive*>::const_iterator it = primsFound.begin(); primsFound.end() != it; ++it) {
				Extents primExtents = (*it)->getExtentsWorld();
				if (primExtents.overlapsOrTouches(searchExtents) && primExtents.max().y <= searchExtents.center().y) {
					// Create a local extents that has space dimensions we require for our new virtual location.
					Vector3 adjSpaceNeeded = spaceNeededToCorner - Vector3(Tolerance::maxOverlapOrGap(), Tolerance::maxOverlapOrGap(), Tolerance::maxOverlapOrGap());
					Vector3 localCenter(calcCenter.x, primExtents.max().y + adjSpaceNeeded.y + Tolerance::maxOverlapOrGap(), calcCenter.z);
					Extents local = Extents::fromCenterCorner(localCenter, adjSpaceNeeded);

					// Should not be touching any other primitives or terrain at "local".
					if (intersectingOthersChecks < FInt::IntersectingOthersCallsAllowedOnSpawn) {
						intersectingOthersChecks++;
						boost::shared_ptr<RBX::PartInstance> tempPart;
						tempPart = Creatable<Instance>::create<PartInstance>();
						CoordinateFrame frame(localCenter);
						tempPart->setCoordinateFrame(frame);
						Vector3 size = Math::toGrid(local.size(), 0.001f);
						tempPart->setPartSizeXml(size);
						if (!intersectingOthers(tempPart->getPartPrimitive(), 0.001f) && !terrainCellsInRegion3(Region3(local))) {
							if (foundFittingLocation && localCenter.y < calcCenter.y) {
								calcCenter.y = localCenter.y;
							} else if (!foundFittingLocation) {
								calcCenter.y = localCenter.y;
								foundFittingLocation = true;
							}
						}
					} else {
						if (!anyExtentsOverlapsOrTouchesPrimitives(local, primsFound) && !terrainCellsInRegion3(Region3(local))) {
							if (foundFittingLocation && localCenter.y < calcCenter.y) {
								calcCenter.y = localCenter.y;
							} else if (!foundFittingLocation) {
								calcCenter.y = localCenter.y;
								foundFittingLocation = true;
							}
						}
					}
				}
			}

			searchCenter.y += spaceNeededToCorner.y * 2;
		} else {
			// If we didn't find any primitives, there may still be terrain above us, find the lowest available terrain location above us and use it.
			if (!terrainCellsInRegion3(Region3(searchExtents))) {
				foundFittingLocation = true;
				calcCenter.y = searchCenter.y;
			} else {
				searchCenter.y += Voxel::kCELL_SIZE;

				// Align to terrain grid.
				int bottomCenterY = static_cast<int>(searchCenter.y - spaceNeededToCorner.y);
				bottomCenterY -= bottomCenterY % Voxel::kCELL_SIZE;
				calcCenter.y = bottomCenterY + spaceNeededToCorner.y;
			}
		}
	}

	return calcCenter;
}

Contact* ContactManager::createContact(	Primitive* p0, Primitive* p1)
{
	RBXASSERT(!p0->getGeometry()->isTerrain() && !p1->getGeometry()->isTerrain());

	// Skip Contact creation (for narrow phase) if either primitive is set to not collide 
	// AND neither primitive is reporting touches
	if ((p0->getPreventCollide() || p1->getPreventCollide()) && 
		(!p0->getOwner()->reportTouches() && !p1->getOwner()->reportTouches()) &&
		(!p0->getDragging() && !p1->getDragging()))
		return NULL;

	// Handle the case where one or other is a CSG part that will collide using Bullet Narrow Phase
	if (p0->getCollideType() == Geometry::COLLIDE_BULLET ||
		p1->getCollideType() == Geometry::COLLIDE_BULLET)
    {
        if (world->getUsingPGSSolver())
            return setUpbulletCollisionShapes(p0, p1) ? new BulletContact(world, p0, p1) : NULL;
        else
            return setUpbulletCollisionShapes(p0, p1) ? new BulletShapeContact(p0, p1, world) : NULL;
    }
    
	if (p0->getCollideType() > p1->getCollideType()) {
		std::swap(p0, p1);
	}
	Geometry::CollideType t0 = p0->getCollideType();
	Geometry::CollideType t1 = p1->getCollideType();

	switch (t0) 
	{			
		case (Geometry::COLLIDE_BALL):
			{
				switch (t1)
				{
				case (Geometry::COLLIDE_BALL):	
					return new BallBallContact(p0, p1);

				case (Geometry::COLLIDE_BLOCK):
					return new BallBlockContact(p0, p1);

				case (Geometry::COLLIDE_POLY):
					return new BallPolyContact(p0, p1);

				default:
					RBXASSERT(0); return NULL;
				}
			}
		case (Geometry::COLLIDE_BLOCK):					// ball, block, poly
			{
				switch (t1)
				{
				case (Geometry::COLLIDE_BLOCK):
					return new BlockBlockContact(p0, p1);

				case (Geometry::COLLIDE_POLY):
					return new PolyPolyContact(p0, p1);

				default:
					RBXASSERT(0); return NULL;
				}
			}
		case (Geometry::COLLIDE_POLY):
			{
				return new PolyPolyContact(p0, p1);
			}
        default:
            RBXASSERT(0);
	}
	RBXCRASH("ContactManager::createContact"); 
	return NULL;
}

void ContactManager::onNewPair(Primitive* p0, Primitive* p1)
{
	RBXASSERT_VERY_FAST(Primitive::getContact(p0, p1) == NULL);
	const Assembly* a0 = Assembly::getConstPrimitiveAssembly(p0);
	const Assembly* a1 = Assembly::getConstPrimitiveAssembly(p1);
	if (a0 == a1)
		return;
	if (a0->computeIsGrounded() && a1->computeIsGrounded())
		return;
	const Assembly* root0 = Mechanism::getConstMovingAssemblyRoot(a0);
	const Assembly* root1 = Mechanism::getConstMovingAssemblyRoot(a1);
	if ((world->getSpatialFilter()->getSimSendFilter().mode == SimSendFilter::dPhysClient) &&
		!SpatialFilter::simulatingPhase(root0->getFilterPhase()) &&
		!SpatialFilter::simulatingPhase(root1->getFilterPhase()))
		return;
    Contact* c = createContact(p0, p1);

	if (c)
	{
		world->insertContact(c);
		RBXASSERT_VERY_FAST(Primitive::getContact(p0, p1) != NULL);
	}
}

void ContactManager::releasePair(Primitive* p0, Primitive* p1)
{
    if( p0 && p1 )
    {
        // for mega cluster contact only
        if (p0->getGeometry()->isTerrain() || p1->getGeometry()->isTerrain())
        {
            Contact* c = Primitive::getContact(p0, p1);
            while (c)
            {
                world->destroyContact(c);
                c = Primitive::getContact(p0, p1);
            }
        }
        else
        {
	        Contact* c = Primitive::getContact(p0, p1);

			RBXASSERT_VERY_FAST(c);

	        if (c)
            {
		        world->destroyContact(c);
	        }
	        else 
            {
		        RBXASSERT(0);			// this should never happen - something amiss in adding/removing contacts from Spatial Hash stage
	        }
        }
        RBXASSERT_VERY_FAST(Primitive::getContact(p0, p1) == NULL);		// should be clear now
    }
}

void ContactManager::onPrimitiveAdded(Primitive* p)
{
    if (p->getGeometry()->isTerrain())
	{
        RBXASSERT(!myMegaClusterPrim);
        myMegaClusterPrim = p;

		if (p->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
		{
			static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->updateAllChunks();

			getSmoothGrid()->connectListener(this);
		}
		else
		{
            FASTLOG2(FLog::TerrainCellListener,
                     "ContactManager: connecting %p to vg %p",
                     this, getVoxelGrid());
            getVoxelGrid()->connectListener(this);
		}
	}
	WriteValidator writeValidator(concurrencyValidator);
	spatialHash->onPrimitiveAdded(p, false);
}

void ContactManager::onPrimitiveRemoved(Primitive* p)
{
	if (p->getGeometry()->isTerrain())
	{
		if (p->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
		{
			getSmoothGrid()->disconnectListener(this);
		}
		else
		{
            FASTLOG2(FLog::TerrainCellListener,
                     "ContactManager: disconnecting %p to vg %p",
                     this, getVoxelGrid());
            getVoxelGrid()->disconnectListener(this);
		}

        RBXASSERT(myMegaClusterPrim);
		myMegaClusterPrim = NULL;
	}

	WriteValidator writeValidator(concurrencyValidator);
	spatialHash->onPrimitiveRemoved(p);
}

void ContactManager::onPrimitiveExtentsChanged(Primitive* p)
{
	WriteValidator writeValidator(concurrencyValidator);
	spatialHash->onPrimitiveExtentsChanged(p);
}

void ContactManager::onPrimitiveGeometryChanged(Primitive* p)
{
	WriteValidator writeValidator(concurrencyValidator);
	G3D::Array<Primitive*> oldContactingPrimitives;

	// 1. Nuke all existing contacts for this primitive
	while (Contact* c = p->getFirstContact()) {
		oldContactingPrimitives.append(c->otherPrimitive(p));
		world->destroyContact(c);
	}

	// 2. Rebuild new ones
	for (int i = 0; i < oldContactingPrimitives.size(); ++i) {
		Primitive* other = oldContactingPrimitives[i];
		if (other->getGeometry()->isTerrain())
		{
			checkTerrainContact(p);
		}
		else
		{
			Contact* newContact = createContact(p, other);
			if( newContact )
				world->insertContact(newContact);
		}
	}
}

void ContactManager::onPrimitiveAssembled(Primitive* p)
{

	WriteValidator writeValidator(concurrencyValidator);
	spatialHash->onPrimitiveAssembled(p);

}

struct AppendPrimitivePredicate
{
	std::vector<Primitive*>* storage;

	void operator()(Primitive* p)
	{
		storage->push_back(p);
	}
};

void ContactManager::onAssemblyMovedFromStep(Assembly& a)
{
	WriteValidator writeValidator(concurrencyValidator);

	// Gather
	tempPrimitives.clear();

	AppendPrimitivePredicate pred = { &tempPrimitives };
	a.visitPrimitives(pred);

	// Notify
	for (Primitive* p: tempPrimitives)
		spatialHash->onPrimitiveExtentsChanged(p);
}

Primitive* ContactManager::getHitLegacy(const RbxRay& originDirection, 
										const Primitive* ignorePrim,		// set to NULL to not use	
										const HitTestFilter* filter,				// set to NULL to not use
										Vector3& hitPointWorld,
										float& distanceToHit,
										const float& maxSearchDepth,
										bool ignoreWater) const
{
	RBXASSERT(originDirection.direction().isUnit());
	RbxRay worldRay = RbxRay::fromOriginAndDirection(originDirection.origin(), originDirection.direction());
	worldRay.direction() *= maxSearchDepth;

	std::vector<const Primitive*> ignorePrims;
	if (ignorePrim) {
		ignorePrims.push_back(ignorePrim);
	}

	Primitive* answer = getHit(worldRay,
							&ignorePrims,
							filter,
							hitPointWorld,
							false,
							ignoreWater);

	distanceToHit = answer ? (hitPointWorld - worldRay.origin()).magnitude() : 0.0f;
	return answer;
}


bool ContactManager::primitiveIsExcludedFromSpatialHash(Primitive* p)
{
    return p->getGeometry()->isTerrain();
}

bool ContactManager::checkMegaClusterWaterContact(Primitive* otherPrim, const Vector3int16& extentStart,
												  const Vector3int16& extentEnd, const Vector3int16& extentSize)
{
	bool touchedWater = false;
	bool touchingWater =  false;

	Vector3int16 gridCell;
	for (gridCell.y = extentStart.y; gridCell.y <= extentEnd.y; gridCell.y += ( extentSize.y > 1 ? extentSize.y - 1 : 1 ))
		for (gridCell.z = extentStart.z; gridCell.z <= extentEnd.z; gridCell.z += ( extentSize.z > 1 ? extentSize.z - 1 : 1 ))
			for (gridCell.x = extentStart.x; gridCell.x <= extentEnd.x; gridCell.x += ( extentSize.x > 1 ? extentSize.x - 1 : 1 ))
				if ( !getVoxelGrid()->getWaterCell(gridCell).isEmpty() )
	            {
		            touchingWater = true;
		            break;
	            }

	// Check center of extent
	if ( !touchingWater )
	{
		Vector3int16 middleCell = ( extentStart + extentEnd ) / 2;
		if ( ( ( middleCell.x > extentStart.x && middleCell.x < extentEnd.x ) ||
			   ( middleCell.y > extentStart.y && middleCell.y < extentEnd.y ) ||
			   ( middleCell.z > extentStart.z && middleCell.z < extentEnd.z ) ) &&
			 !getVoxelGrid()->getWaterCell( middleCell ).isEmpty() )
	    {
		    touchingWater = true;
	    }
	}

	// remove buoyancy contact if the primitive no longer touches water
	for (int i = 0; i < otherPrim->getNumContacts(); ++i)
	{
		if (otherPrim->getContactOther(i) != myMegaClusterPrim)
			continue;

        Contact* contact = otherPrim->getContact(i);
        if (contact->getContactType() != Contact::Contact_Buoyancy)
            continue;

		touchedWater = true;
		if (!touchingWater)
			world->destroyContact(contact);
	}
	if ( !touchedWater && touchingWater )
		world->insertContact(BuoyancyContact::create(myMegaClusterPrim, otherPrim));
	return touchingWater;
}

// TBD: Optimize this later
bool ContactManager::checkMegaClusterBigTerrainContact(Primitive* otherPrim)
{
	std::vector<Vector3int16> cellsUnfiltered;
	MegaClusterPoly* mCPoly = static_cast<MegaClusterPoly*>(myMegaClusterPrim->getGeometry());
	mCPoly->findCellsTouchingGeometry(myMegaClusterPrim->getCoordinateFrame(), *(otherPrim->getConstGeometry()), otherPrim->getCoordinateFrame(), &cellsUnfiltered);
	if (cellsUnfiltered.size() == 0)
		return false;

    typedef boost::unordered_set<Vector3int16, Vector3int16::boost_compatible_hash_value> CellSet;
    CellSet cells;
    cells.insert(cellsUnfiltered.begin(), cellsUnfiltered.end());

	// remove cells from "cells" map if we already have contact with this primitive and that particular cell
	// so we aren't deleting Contact objects and recreating duplicates (for coherence)
	// The other pre-existing contacts that are *not* in the new cells list should be removed,
	// since they are no longer relevant
	for (int i = 0; i < otherPrim->getNumContacts(); ++i)
	{
		if (otherPrim->getContactOther(i) != myMegaClusterPrim)
			continue;

        Contact* contact = otherPrim->getContact(i);
        if (contact->getContactType() != Contact::Contact_Cell)
            continue;

		CellSet::iterator it = cells.find(static_cast<CellContact*>(contact)->getGridFeature().toVector3int16());
		if( it != cells.end() )
			cells.erase(it);
		else
			world->destroyContact(contact);
	} 

	// the cells map will only have non-preexisting contacts, so create them
	for(CellSet::iterator i = cells.begin(); i != cells.end(); i++ )
	{
		if( otherPrim->getCollideType() == Geometry::COLLIDE_BALL )
			world->insertContact(new BallCellContact(myMegaClusterPrim, otherPrim, *i));
		else if (otherPrim->getCollideType() == Geometry::COLLIDE_BULLET && otherPrim->getGeometry()->setUpBulletCollisionData())
            world->insertContact(new BulletShapeCellContact(myMegaClusterPrim, otherPrim, *i, world));
		else
            world->insertContact(new PolyCellContact(myMegaClusterPrim, otherPrim, *i));
	}

	return true;
}

bool ContactManager::checkMegaClusterSmallTerrainContact(Primitive* otherPrim, const Vector3int16& extentStart, const Vector3int16& extentEnd,
														 const Vector3int16& extentSize, bool cellChanged)
{
    bool touchingTerrain = false;

    RBXASSERT(extentSize.x * extentSize.y * extentSize.z <= LARGE_TERRAIN_CONTACT_VOLUME);

    bool cellMap[LARGE_TERRAIN_CONTACT_VOLUME];
    memset(cellMap, 0, LARGE_TERRAIN_CONTACT_VOLUME);

    // Copy data from terrain to cell map
    Vector3int16 gridCell(0, 0, 0);

    if (SpatialRegion::regionContainingVoxel(extentStart) == SpatialRegion::regionContainingVoxel(extentEnd))
    {
        Voxel::Grid::Region region = getVoxelGrid()->getRegion(extentStart, extentEnd);

        int i = 0;

        for (gridCell.y = extentStart.y; gridCell.y <= extentEnd.y; ++gridCell.y)
        {
            for (gridCell.z = extentStart.z; gridCell.z <= extentEnd.z; ++gridCell.z)
            {
                for (gridCell.x = extentStart.x; gridCell.x <= extentEnd.x; ++gridCell.x)
                {
                    cellMap[i] = region.voxelAt(gridCell).solid.getBlock() != Voxel::CELL_BLOCK_Empty;
                    touchingTerrain = touchingTerrain || cellMap[i];
                    ++i;
                }
            }
        }
    }
    else
    {
        int i = 0;

        for (gridCell.y = extentStart.y; gridCell.y <= extentEnd.y; ++gridCell.y)
        {
            for (gridCell.z = extentStart.z; gridCell.z <= extentEnd.z; ++gridCell.z)
            {
                for (gridCell.x = extentStart.x; gridCell.x <= extentEnd.x; ++gridCell.x)
                {
                    cellMap[i] = getVoxelGrid()->getCell(gridCell).solid.getBlock() != Voxel::CELL_BLOCK_Empty;
                    touchingTerrain = touchingTerrain || cellMap[i];
                    ++i;
                }
            }
        }
    }

    Vector3int16 extentCell(0, 0, 0);   // Cell within the AA extent of the primitive

    // Remove cell contacts if they no longer touch the primitive
    for (int i = 0; i < otherPrim->getNumContacts(); ++i)
    {
        if (otherPrim->getContactOther(i) != myMegaClusterPrim)
            continue;

        Contact* contact = otherPrim->getContact(i);
        if (contact->getContactType() != Contact::Contact_Cell)
            continue;

        const Vector3int32& gridCell = static_cast<CellContact*>(contact)->getGridFeature();

        int offsetX = gridCell.x - extentStart.x;
        int offsetY = gridCell.y - extentStart.y;
        int offsetZ = gridCell.z - extentStart.z;

        if ((unsigned)offsetX >= (unsigned)extentSize.x || (unsigned)offsetY >= (unsigned)extentSize.y || (unsigned)offsetZ >= (unsigned)extentSize.z)
        {
            world->destroyContact(contact);
            continue;
        }

        bool& cellSolid = cellMap[offsetX + extentSize.x * (offsetZ + extentSize.z * offsetY)];

        if (cellChanged && !cellSolid)
        {
            world->destroyContact(contact);
            continue;
        }

        // A contact already exists for this cell so don't create a new one
        cellSolid = false;
    }

    int i = 0;

    for (gridCell.y = extentStart.y; gridCell.y <= extentEnd.y; ++gridCell.y)
    {
        for (gridCell.z = extentStart.z; gridCell.z <= extentEnd.z; ++gridCell.z)
        {
            for (gridCell.x = extentStart.x; gridCell.x <= extentEnd.x; ++gridCell.x)
            {
                if (cellMap[i])
                {
                    if (otherPrim->getCollideType() == Geometry::COLLIDE_BALL)
                        world->insertContact(new BallCellContact(myMegaClusterPrim, otherPrim, gridCell));
					else if (otherPrim->getCollideType() == Geometry::COLLIDE_BULLET && otherPrim->getGeometry()->setUpBulletCollisionData())
						world->insertContact(new BulletShapeCellContact(myMegaClusterPrim, otherPrim, gridCell, world));
					else
                        world->insertContact(new PolyCellContact(myMegaClusterPrim, otherPrim, gridCell));
                }

                ++i;
            }
        }
    }

    return touchingTerrain;
}

void ContactManager::checkMegaClusterContact(Primitive* otherPrim, bool checkTerrain, bool checkWater, bool cellChangd)
{
	if( !myMegaClusterPrim )
		return;

	// Skip Contact creation (for narrow phase) for any primitive that is set to not collide 
	// AND is not reporting touches
	if (otherPrim->getPreventCollide() && !otherPrim->getOwner()->reportTouches() && !otherPrim->getDragging())
		return;

	unsigned int nonEmptyCellCount = getVoxelGrid()->getNonEmptyCellCount();

	if( nonEmptyCellCount == 0 )
		return;

    Extents otherExtents = otherPrim->getExtentsWorld();

    // Check if it overlaps with terrain and clamp it to the overlapped area if it does
    if (!otherExtents.clampToOverlap(Voxel::getTerrainExtents()))
    {
        releasePair(myMegaClusterPrim, otherPrim);
        return;
    }

    const Vector3int16 extentStart = Voxel::worldToCell_floor(otherExtents.min());
    const Vector3int16 extentEnd = Voxel::worldToCell_floor(otherExtents.max());

	const Vector3int16 extentSize(extentEnd - extentStart + Vector3int16::one());

	bool touchingTerrain = false;
	bool touchingWater = false;

	if ( checkTerrain )
	{
		if ( extentSize.x * extentSize.y * extentSize.z > LARGE_TERRAIN_CONTACT_VOLUME )
			touchingTerrain = checkMegaClusterBigTerrainContact(otherPrim);
		else {
            touchingTerrain = checkMegaClusterSmallTerrainContact(otherPrim, extentStart, extentEnd, extentSize, cellChangd);
		}
	} 

	if ( checkWater ) {
        touchingWater = checkMegaClusterWaterContact(otherPrim, extentStart, extentEnd, extentSize);
	}
	if ( cellChangd )
		world->ticklePrimitive(otherPrim, true);
	if ( !touchingWater && !touchingTerrain )
		releasePair(myMegaClusterPrim, otherPrim);
}

static TerrainPartitionSmooth::ChunkResult* findChunk(std::vector<TerrainPartitionSmooth::ChunkResult>& chunks, const Vector3int32& id)
{
    for (size_t i = 0; i < chunks.size(); ++i)
        if (chunks[i].id == id)
            return &chunks[i];

    return NULL;
}

static bool isAnyChunkTouchingWater(const std::vector<TerrainPartitionSmooth::ChunkResult>& chunks)
{
    for (size_t i = 0; i < chunks.size(); ++i)
		if (chunks[i].touchesWater)
            return true;

    return false;
}

void ContactManager::checkSmoothClusterContact(Primitive* otherPrim, bool cellChanged)
{
	RBXASSERT(myMegaClusterPrim && getSmoothGrid());

	if (!getSmoothGrid()->isAllocated())
		return;

    // Skip Contact creation (for narrow phase) for any primitive that is set to not collide 
    // AND is not reporting touches
    if (otherPrim->getPreventCollide() && !otherPrim->getOwner()->reportTouches() && !otherPrim->getDragging())
        return;

	Geometry* otherGeom = otherPrim->getGeometry();
    if (!otherGeom->setUpBulletCollisionData())
        return;

    Extents otherExtentsLocal(-0.5f * otherGeom->getSize(), 0.5f * otherGeom->getSize());
	Extents otherExtentsWorld = otherExtentsLocal.toWorldSpace(otherPrim->getCoordinateFrameUnsafe());

	tempChunks.clear();
	std::vector<TerrainPartitionSmooth::ChunkResult>& chunks = tempChunks;

	static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->getTerrainPartition()->findChunksTouchingExtents(otherExtentsWorld, &chunks);

    bool touchingWater = isAnyChunkTouchingWater(chunks);
    bool wasTouchingWater = false;

    for (int i = 0; i < otherPrim->getNumContacts(); )
    {
        if (otherPrim->getContactOther(i) == myMegaClusterPrim)
		{
            Contact* contact = otherPrim->getContact(i);
            Contact::ContactType contactType = contact->getContactType();
			RBXASSERT(contactType == Contact::Contact_Cell || contactType == Contact::Contact_Buoyancy);

			if (contactType == Contact::Contact_Cell)
			{
				TerrainPartitionSmooth::ChunkResult* chunk = findChunk(chunks, static_cast<CellContact*>(contact)->getGridFeature());

				if (chunk && chunk->touchesSolid)
					chunk->touchesSolid = false; // mark chunk as not-touching so that the code below does not create new contacts
				else
				{
					world->destroyContact(contact);
                    continue;
				}
			}
			else
			{
				if (touchingWater)
					wasTouchingWater = true;
				else
				{
					world->destroyContact(contact);
                    continue;
				}
			}
		}

        ++i;
    }

	for (size_t i = 0; i < chunks.size(); ++i)
	{
		const TerrainPartitionSmooth::ChunkResult& chunk = chunks[i];

		if (chunk.touchesSolid)
			if (shared_ptr<btCollisionShape> shape = static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->getBulletChunkShape(chunk.id))
            {
                if (world->getUsingPGSSolver())
                    world->insertContact(new BulletCellContact(world, myMegaClusterPrim, otherPrim, chunk.id, shape));
                else
                    world->insertContact(new BulletShapeCellContact(myMegaClusterPrim, otherPrim, chunk.id, shape, world));
            }
    }

	if (touchingWater && !wasTouchingWater)
		world->insertContact(BuoyancyContact::create(myMegaClusterPrim, otherPrim));

	if (cellChanged)
		world->ticklePrimitive(otherPrim, true);
}

void ContactManager::checkTerrainContact(Primitive* otherPrim)
{
	if (myMegaClusterPrim)
	{
		if (myMegaClusterPrim->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
			checkSmoothClusterContact(otherPrim, false);
		else
			checkMegaClusterContact(otherPrim, true, true, false);
	}
}

void ContactManager::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	updatedTerrainRegions.insert(SpatialRegion::regionContainingVoxel(info.position));
}

void ContactManager::onTerrainRegionChanged(const Voxel2::Region& region)
{
    std::vector<Vector3int32> chunks = region.expand(1).getChunkIds(TerrainPartitionSmooth::kChunkSizeLog2);

    updatedTerrainChunks.insert(chunks.begin(), chunks.end());
}

Voxel::Grid* ContactManager::getVoxelGrid()
{
	return boost::polymorphic_downcast<MegaClusterInstance*>(myMegaClusterPrim->getOwner())->getVoxelGrid();
}

Voxel2::Grid* ContactManager::getSmoothGrid()
{
	return boost::polymorphic_downcast<MegaClusterInstance*>(myMegaClusterPrim->getOwner())->getSmoothGrid();
}


void ContactManager::applyDeferredMegaClusterChanges()
{
    for (UpdatedTerrainRegionsSet::const_iterator itr = updatedTerrainRegions.begin(); itr != updatedTerrainRegions.end(); ++itr)
    {
		const SpatialRegion::Id regionId = *itr;
		Extents extents(SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(regionId).toVector3(),
			SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(regionId).toVector3());

		FASTLOG3(DFLog::DeferredVoxelUpdates, "Deferred voxel region update: starting region (%d,%d,%d)",
			regionId.value().x, regionId.value().y, regionId.value().z);
		DenseHashSet<Primitive*> primitives(NULL);
		getPrimitivesOverlapping(extents, primitives);
		size_t count = 0;
		for (DenseHashSet<Primitive*>::const_iterator pitr = primitives.begin(); pitr!=primitives.end(); ++pitr) {
			if (myMegaClusterPrim) {
				releasePair(myMegaClusterPrim, *pitr);

                world->destroyTerrainWeldJointsWithEmptyCells(myMegaClusterPrim, regionId, *pitr);
			}
			checkMegaClusterContact(*pitr, true, true, true);
			++count;
		}
		FASTLOG1(DFLog::DeferredVoxelUpdates, "Done updating region. Updated %u parts", count);
	}

	updatedTerrainRegions.clear();
}

void ContactManager::applyDeferredSmoothClusterChanges()
{
	DenseHashSet<Primitive*> primitives(NULL);

	// Gather primitives from all chunks
	for (auto& id: updatedTerrainChunks)
	{
		Voxel2::Region region = Voxel2::Region::fromChunk(id, TerrainPartitionSmooth::kChunkSizeLog2).expand(1);

		Extents extents(region.begin().toVector3() * Voxel::kCELL_SIZE, region.end().toVector3() * Voxel::kCELL_SIZE);

		getPrimitivesOverlapping(extents, primitives);
	}

	// Update chunks
	for (auto& id: updatedTerrainChunks)
		static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->updateChunk(id);

	// Update primitives
	for (Primitive* prim: primitives)
	{
		releasePair(myMegaClusterPrim, prim);

		checkSmoothClusterContact(prim, true);

		world->destroyTerrainWeldJointsNoTouch(myMegaClusterPrim, prim);
	}

    updatedTerrainChunks.clear();

	static_cast<SmoothClusterGeometry*>(myMegaClusterPrim->getGeometry())->garbageCollectIncremental();
}

void ContactManager::applyDeferredTerrainChanges()
{
	RBXPROFILER_SCOPE("Physics", "applyDeferredTerrainChanges");

	if (myMegaClusterPrim)
	{
		if (myMegaClusterPrim->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
            applyDeferredSmoothClusterChanges();
		else
			applyDeferredMegaClusterChanges();
	}
}

bool ContactManager::setUpbulletCollisionShapes(Primitive* p0, Primitive* p1)
{
	// for CSG parts return false if decomp is not set
	// for legacy parts make sure the bullet geometry is set
	// return false if anything is amiss
	
	if (p0->getGeometry()->setUpBulletCollisionData() && p1->getGeometry()->setUpBulletCollisionData())
		return true;
	else
		return false;
}

} // namespace
