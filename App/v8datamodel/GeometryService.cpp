#include "stdafx.h"

#include "v8datamodel/GeometryService.h"
#include "v8datamodel/datamodel.h"
#include "v8world/contactmanager.h"
#include "V8DataModel/Workspace.h"
#include "V8World/World.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/PartInstance.h"

#include "rbx/Profiler.h"

const char* const RBX::sGeometryService = "Geometry";

using namespace RBX;


GeometryService::GeometryService(void)
	:DescribedNonCreatable<GeometryService, Instance, sGeometryService>(sGeometryService),
	workspace(NULL)
{
}

static void addInstanceToIgnorePrimitiveSet(shared_ptr<RBX::Instance> instance, boost::unordered_set<const Primitive*>& primitiveSet)
{
	PartInstance *part = Instance::fastDynamicCast<PartInstance>(instance.get());
	if (part) 
	{
		Primitive* newPrimitive = part->getPartPrimitive();
		primitiveSet.insert(newPrimitive);
	}
}

void GeometryService::getPartsTouchingExtentsWithIgnore(
				const Extents& extents, 
				const Instances* ancestors,
				int maxCount,
				G3D::Array<PartInstance*>& found)
{
	if (workspace)
	{
		RBXASSERT(found.size() == 0);
		ContactManager* contactManager = workspace->getWorld()->getContactManager();
		foundPrimitives.fastClear();
		
		//populate the ignore-set
		boost::unordered_set<const Primitive*> ignorePrimitives;
		if (ancestors){
			// go through list of all instances to-be-ignored
			for (Instances::const_iterator ancestorIt = ancestors->begin(); ancestorIt != ancestors->end(); ++ancestorIt){
				Instance* ancestor = ancestorIt->get();
				if (ancestor){
					// add ancestor itself, if is a part
					const PartInstance * ancestorPart = ancestor->fastDynamicCast<PartInstance>();
					if (ancestorPart) 
					{
						const Primitive* ancestorPrimitive = ancestorPart->getConstPartPrimitive();
						ignorePrimitives.insert(ancestorPrimitive);
					}
	
					// add all descendants of ancestor
					ancestor->visitDescendants(boost::bind(&addInstanceToIgnorePrimitiveSet, _1, boost::ref(ignorePrimitives)));
				}
			}
		}
		contactManager->getPrimitivesTouchingExtents(extents, ignorePrimitives, maxCount, foundPrimitives);

		// just need to convert from Primitives to PartInstances (could do this a tiny bit more efficiently in CheckPrimitive if overload that)
		for (int i = 0; i < foundPrimitives.size(); ++i) {
			PartInstance* part = PartInstance::fromPrimitive(foundPrimitives[i]);
			found.append(part);
		}
	}
}

void GeometryService::getPartsTouchingExtents(
				const Extents& extents, 
				const Primitive* ignore,
				int maxCount,
				G3D::Array<PartInstance*>& found)
{
	if (workspace)
	{
		RBXASSERT(found.size() == 0);
		ContactManager* contactManager = workspace->getWorld()->getContactManager();
		foundPrimitives.fastClear();
		contactManager->getPrimitivesTouchingExtents(extents, ignore, maxCount, foundPrimitives);
		for (int i = 0; i < foundPrimitives.size(); ++i) {
			found.append(PartInstance::fromPrimitive(foundPrimitives[i]));
		}
	}
}

template<class IgnoreType>
Vector3 GeometryService::getHitLocationPartFilterDescendents(IgnoreType *ancestor, RBX::RbxRay ray, shared_ptr<PartInstance>& result, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells)
{
	RBXPROFILER_SCOPE("Physics", "getHitLocationPartFilterDescendents");

	Primitive* hitPrim = NULL;
	Vector3 v3result = getHitLocationFilterDescendents(ancestor, ray, &hitPrim, surfaceNormal, surfaceMaterial, terrainCellsAreCubes, ignoreWaterCells);
	if(hitPrim){
		result = shared_from(PartInstance::fromPrimitive(hitPrim));
		return v3result;
	}
	else{
		result.reset();
		return v3result;
	}
}

// explicitly instantiate IgnoreType so linker doesn't QQ
template Vector3 GeometryService::getHitLocationPartFilterDescendents<Instance>(Instance*, RBX::RbxRay, shared_ptr<PartInstance>&, Vector3&, PartMaterial&, bool, bool);
template Vector3 GeometryService::getHitLocationPartFilterDescendents<const Instances>(const Instances*, RBX::RbxRay, shared_ptr<PartInstance>&, Vector3&, PartMaterial&, bool, bool);

Vector3 GeometryService::getHitLocationFilterDescendents(Instance *ancestor, RBX::RbxRay ray, Primitive **hitPrim, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells)
{
	if (!workspace) return Vector3::zero();

	FilterDescendents filter(shared_from(ancestor));

	ContactManager& contactManager = *workspace->getWorld()->getContactManager();

	Vector3 hitPoint;

	*hitPrim = contactManager.getHit(	ray,
														NULL,
														&filter,
                                                        hitPoint,
														terrainCellsAreCubes,
                                                        ignoreWaterCells,
                                                        surfaceNormal,
                                                        surfaceMaterial);

	if (!(*hitPrim)) {
		hitPoint = ray.unit().origin() + ray.direction();
	}

	return hitPoint;
	
}

Vector3 GeometryService::getHitLocationFilterDescendents(const Instances *ancestors, RBX::RbxRay ray, Primitive **hitPrim, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes,  bool ignoreWaterCells)
{
	if (!workspace) return Vector3::zero();

	FilterDescendentsList filter(ancestors);

	ContactManager& contactManager = *workspace->getWorld()->getContactManager();

	Vector3 hitPoint;

	*hitPrim = contactManager.getHit(	ray,
														NULL,
														&filter,
														hitPoint,
                                                        terrainCellsAreCubes,
                                                        ignoreWaterCells,
                                                        surfaceNormal,
                                                        surfaceMaterial);

	if (!(*hitPrim)) {
		hitPoint = ray.unit().origin() + ray.direction();
	}

	return hitPoint;
	
}

Vector3 GeometryService::getHitLocationFilterStairs(Instance *ancestor, RBX::RbxRay ray, Primitive **hitPrim)
{
	if (!workspace) return Vector3::zero();

	FilterDescendents filter(shared_from(ancestor));

	ContactManager& contactManager = *workspace->getWorld()->getContactManager();

	Vector3 hitPoint;
	std::vector<const Primitive *> dummy; // required, fix in CM later

	*hitPrim = contactManager.getHit(	ray,
														&dummy, // can't be NULL
														&filter,
														hitPoint);

	if (!(*hitPrim)) {
		hitPoint = ray.unit().origin() + ray.direction();
	}

	return hitPoint;
	
}

void GeometryService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	workspace = NULL;

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		workspace = ServiceProvider::find<Workspace>(newProvider);
	}

}
