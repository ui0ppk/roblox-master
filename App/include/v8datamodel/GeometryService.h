#pragma once

#include "V8Tree/Service.h"
#include "Util/G3DCore.h"
#include "Util/Extents.h"
#include "G3D/Array.h"

#include "util/PartMaterial.h"

namespace RBX {

class PartInstance;
class Primitive;
class ContactManager;
class World;
class Workspace;
class Instance;

extern const char* const sGeometryService;
class GeometryService 
	: public DescribedNonCreatable<GeometryService, Instance, sGeometryService>
	, public Service
{
private:
	typedef DescribedNonCreatable<GeometryService, Instance, sGeometryService> Super;
	Workspace *workspace;
	G3D::Array<Primitive*> foundPrimitives;

public:
	GeometryService();

	Vector3 getHitLocationFilterStairs(Instance *ancestor, RBX::RbxRay ray, Primitive **hitPrim);

	Vector3 getHitLocationFilterDescendents(Instance *ancestor, RBX::RbxRay ray, Primitive **hitPrim, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);
	Vector3 getHitLocationFilterDescendents(const Instances *ancestors, RBX::RbxRay ray, Primitive **hitPrim, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);

	// we template this function to avoid heavy code duplication: IgnoreType is currently either Instance or Instances
	template<class IgnoreType>
	Vector3 getHitLocationPartFilterDescendents(IgnoreType *ancestor, RBX::RbxRay ray, shared_ptr<PartInstance>& result, Vector3& surfaceNormal, PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);

	void getPartsTouchingExtents(
				const Extents& extents, 
				const Primitive* ignore,
				int maxCount,
				G3D::Array<PartInstance*>& found);

	void getPartsTouchingExtentsWithIgnore(
				const Extents& extents, 
				const Instances* ancestors,
				int maxCount,
				G3D::Array<PartInstance*>& found);

protected:
	virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};

}
