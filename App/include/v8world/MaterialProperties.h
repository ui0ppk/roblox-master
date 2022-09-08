#pragma once
#include "util/PartMaterial.h"


DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

namespace RBX {

class PhysicalProperties;
class ContactParams;
class Primitive;

class MaterialProperties
{
private:
	// Helpers
	static float calculateUsingWeightedAverage(float weightA, float coeffA, float weightB, float coeffB);
	// Used on PartInstance initialization and property setting
	static float getDefaultMaterialFriction(PartMaterial material);
	static float getDefaultMaterialFrictionWeight(PartMaterial material);
	static float getDefaultMaterialElasticity(PartMaterial material);
	static float getDefaultMaterialElasticityWeight(PartMaterial material);
	static float getDefaultMaterialDensity(PartMaterial material);
public:
	// Physical Behavior functions
	// Update Contact Parameters between two primitives, and Primitive to Terrain
	static void updateContactParamsPrims(ContactParams& params, Primitive* prim0, Primitive* prim1);
	static void updateContactParamsPrimMaterial(ContactParams& params, Primitive* prim, Primitive* otherPrim, PartMaterial otherMaterial);

	static float getDensity(Primitive* prim);

	// For humanoid Behavior
	static float frictionBetweenMaterials(PartMaterial materialA, PartMaterial materialB);
	static float frictionBetweenPrimAndMaterial(Primitive* primA, PartMaterial materialB);

	// Property defaults helper
	static PhysicalProperties generatePhysicalMaterialFromPartMaterial(PartMaterial material);
	static PhysicalProperties getPrimitivePhysicalProperties(Primitive* prim);

};


} // NAMESPACE RBX