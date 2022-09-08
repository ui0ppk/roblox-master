/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"
#include "V8World/MaterialProperties.h"
#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "v8kernel/ContactParams.h"
#include "v8kernel/Constants.h"
#include "util/PhysicalProperties.h"

FASTFLAGVARIABLE(ModifyDefaultMaterialProperties, false)

// Thousandths
FASTINTVARIABLE(PhysicalPropDensity_PLASTIC_MATERIAL,			700)
FASTINTVARIABLE(PhysicalPropDensity_SMOOTH_PLASTIC_MATERIAL,	700)
FASTINTVARIABLE(PhysicalPropDensity_NEON_MATERIAL,				700)
FASTINTVARIABLE(PhysicalPropDensity_WOOD_MATERIAL,				350)
FASTINTVARIABLE(PhysicalPropDensity_WOODPLANKS_MATERIAL,		350)
FASTINTVARIABLE(PhysicalPropDensity_MARBLE_MATERIAL,			2563)
FASTINTVARIABLE(PhysicalPropDensity_SLATE_MATERIAL,				2691)
FASTINTVARIABLE(PhysicalPropDensity_CONCRETE_MATERIAL,			2403)
FASTINTVARIABLE(PhysicalPropDensity_GRANITE_MATERIAL,			2691)
FASTINTVARIABLE(PhysicalPropDensity_BRICK_MATERIAL,				1922)
FASTINTVARIABLE(PhysicalPropDensity_PEBBLE_MATERIAL,			2403)
FASTINTVARIABLE(PhysicalPropDensity_COBBLESTONE_MATERIAL,		2691)
FASTINTVARIABLE(PhysicalPropDensity_RUST_MATERIAL,				7850)
FASTINTVARIABLE(PhysicalPropDensity_DIAMONDPLATE_MATERIAL,		7850)
FASTINTVARIABLE(PhysicalPropDensity_ALUMINUM_MATERIAL,			7700)
FASTINTVARIABLE(PhysicalPropDensity_METAL_MATERIAL,				7850)
FASTINTVARIABLE(PhysicalPropDensity_GRASS_MATERIAL,				900)
FASTINTVARIABLE(PhysicalPropDensity_SAND_MATERIAL,				1602)
FASTINTVARIABLE(PhysicalPropDensity_FABRIC_MATERIAL,			700)
FASTINTVARIABLE(PhysicalPropDensity_ICE_MATERIAL,				919)
FASTINTVARIABLE(PhysicalPropDensity_AIR_MATERIAL,				0)
FASTINTVARIABLE(PhysicalPropDensity_WATER_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropDensity_ROCK_MATERIAL,				2691)
FASTINTVARIABLE(PhysicalPropDensity_GLACIER_MATERIAL,			919)
FASTINTVARIABLE(PhysicalPropDensity_SNOW_MATERIAL,				900)
FASTINTVARIABLE(PhysicalPropDensity_SANDSTONE_MATERIAL,			2691)
FASTINTVARIABLE(PhysicalPropDensity_MUD_MATERIAL,				900)
FASTINTVARIABLE(PhysicalPropDensity_BASALT_MATERIAL,			2691)
FASTINTVARIABLE(PhysicalPropDensity_GROUND_MATERIAL,			900)
FASTINTVARIABLE(PhysicalPropDensity_CRACKED_LAVA_MATERIAL,		2691)


FASTINTVARIABLE(PhysicalPropFriction_PLASTIC_MATERIAL,			300)
FASTINTVARIABLE(PhysicalPropFriction_SMOOTH_PLASTIC_MATERIAL,	200)
FASTINTVARIABLE(PhysicalPropFriction_NEON_MATERIAL,				300)
FASTINTVARIABLE(PhysicalPropFriction_WOOD_MATERIAL,				480)
FASTINTVARIABLE(PhysicalPropFriction_WOODPLANKS_MATERIAL,		480)
FASTINTVARIABLE(PhysicalPropFriction_MARBLE_MATERIAL,			200)
FASTINTVARIABLE(PhysicalPropFriction_SLATE_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_CONCRETE_MATERIAL,			700)
FASTINTVARIABLE(PhysicalPropFriction_GRANITE_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_BRICK_MATERIAL,			800)
FASTINTVARIABLE(PhysicalPropFriction_PEBBLE_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_COBBLESTONE_MATERIAL,		500)
FASTINTVARIABLE(PhysicalPropFriction_RUST_MATERIAL,				700)
FASTINTVARIABLE(PhysicalPropFriction_DIAMONDPLATE_MATERIAL,		350)
FASTINTVARIABLE(PhysicalPropFriction_ALUMINUM_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_METAL_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_GRASS_MATERIAL,			400)
FASTINTVARIABLE(PhysicalPropFriction_SAND_MATERIAL,				500)
FASTINTVARIABLE(PhysicalPropFriction_FABRIC_MATERIAL,			350)
FASTINTVARIABLE(PhysicalPropFriction_ICE_MATERIAL,				20)
FASTINTVARIABLE(PhysicalPropFriction_AIR_MATERIAL,				10)
FASTINTVARIABLE(PhysicalPropFriction_WATER_MATERIAL,			5)
FASTINTVARIABLE(PhysicalPropFriction_ROCK_MATERIAL,				500)
FASTINTVARIABLE(PhysicalPropFriction_GLACIER_MATERIAL,			50)
FASTINTVARIABLE(PhysicalPropFriction_SNOW_MATERIAL,				300)
FASTINTVARIABLE(PhysicalPropFriction_SANDSTONE_MATERIAL,		500)
FASTINTVARIABLE(PhysicalPropFriction_MUD_MATERIAL,				300)
FASTINTVARIABLE(PhysicalPropFriction_BASALT_MATERIAL,			700)
FASTINTVARIABLE(PhysicalPropFriction_GROUND_MATERIAL,			450)
FASTINTVARIABLE(PhysicalPropFriction_CRACKED_LAVA_MATERIAL,		650)

FASTINTVARIABLE(PhysicalPropElasticity_PLASTIC_MATERIAL,		500)
FASTINTVARIABLE(PhysicalPropElasticity_SMOOTH_PLASTIC_MATERIAL,	500)
FASTINTVARIABLE(PhysicalPropElasticity_NEON_MATERIAL,			200)
FASTINTVARIABLE(PhysicalPropElasticity_WOOD_MATERIAL,			200)
FASTINTVARIABLE(PhysicalPropElasticity_WOODPLANKS_MATERIAL,		200)
FASTINTVARIABLE(PhysicalPropElasticity_MARBLE_MATERIAL,			170)
FASTINTVARIABLE(PhysicalPropElasticity_SLATE_MATERIAL,			200)
FASTINTVARIABLE(PhysicalPropElasticity_CONCRETE_MATERIAL,		200)
FASTINTVARIABLE(PhysicalPropElasticity_GRANITE_MATERIAL,		200)
FASTINTVARIABLE(PhysicalPropElasticity_BRICK_MATERIAL,			150)
FASTINTVARIABLE(PhysicalPropElasticity_PEBBLE_MATERIAL,			170)
FASTINTVARIABLE(PhysicalPropElasticity_COBBLESTONE_MATERIAL,	170)
FASTINTVARIABLE(PhysicalPropElasticity_RUST_MATERIAL,			200)
FASTINTVARIABLE(PhysicalPropElasticity_DIAMONDPLATE_MATERIAL,	250)
FASTINTVARIABLE(PhysicalPropElasticity_ALUMINUM_MATERIAL,		250)
FASTINTVARIABLE(PhysicalPropElasticity_METAL_MATERIAL,			250)
FASTINTVARIABLE(PhysicalPropElasticity_GRASS_MATERIAL,			100)
FASTINTVARIABLE(PhysicalPropElasticity_SAND_MATERIAL,			50)
FASTINTVARIABLE(PhysicalPropElasticity_FABRIC_MATERIAL,			50)
FASTINTVARIABLE(PhysicalPropElasticity_ICE_MATERIAL,			150)
FASTINTVARIABLE(PhysicalPropElasticity_AIR_MATERIAL,			10)
FASTINTVARIABLE(PhysicalPropElasticity_WATER_MATERIAL,			10)
FASTINTVARIABLE(PhysicalPropElasticity_ROCK_MATERIAL,			170)
FASTINTVARIABLE(PhysicalPropElasticity_GLACIER_MATERIAL,		150)
FASTINTVARIABLE(PhysicalPropElasticity_SNOW_MATERIAL,			30)
FASTINTVARIABLE(PhysicalPropElasticity_SANDSTONE_MATERIAL,		150)
FASTINTVARIABLE(PhysicalPropElasticity_MUD_MATERIAL,			70)
FASTINTVARIABLE(PhysicalPropElasticity_BASALT_MATERIAL,			150)
FASTINTVARIABLE(PhysicalPropElasticity_GROUND_MATERIAL,			100)
FASTINTVARIABLE(PhysicalPropElasticity_CRACKED_LAVA_MATERIAL,	150)

FASTINTVARIABLE(PhysicalPropFWeight_PLASTIC_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_SMOOTH_PLASTIC_MATERIAL,	1000)
FASTINTVARIABLE(PhysicalPropFWeight_NEON_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_WOOD_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_WOODPLANKS_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropFWeight_MARBLE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_SLATE_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_CONCRETE_MATERIAL,			300)
FASTINTVARIABLE(PhysicalPropFWeight_GRANITE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_BRICK_MATERIAL,				300)
FASTINTVARIABLE(PhysicalPropFWeight_PEBBLE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_COBBLESTONE_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropFWeight_RUST_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_DIAMONDPLATE_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropFWeight_ALUMINUM_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_METAL_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_GRASS_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_SAND_MATERIAL,				5000)
FASTINTVARIABLE(PhysicalPropFWeight_FABRIC_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_ICE_MATERIAL,				3000)
FASTINTVARIABLE(PhysicalPropFWeight_AIR_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_WATER_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_ROCK_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_GLACIER_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_SNOW_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_SANDSTONE_MATERIAL,			5000)
FASTINTVARIABLE(PhysicalPropFWeight_MUD_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropFWeight_BASALT_MATERIAL,			300)
FASTINTVARIABLE(PhysicalPropFWeight_GROUND_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropFWeight_CRACKED_LAVA_MATERIAL,		1000)

FASTINTVARIABLE(PhysicalPropEWeight_PLASTIC_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_SMOOTH_PLASTIC_MATERIAL,	1000)
FASTINTVARIABLE(PhysicalPropEWeight_NEON_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_WOOD_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_WOODPLANKS_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropEWeight_MARBLE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_SLATE_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_CONCRETE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_GRANITE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_BRICK_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_PEBBLE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_COBBLESTONE_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropEWeight_RUST_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_DIAMONDPLATE_MATERIAL,		1000)
FASTINTVARIABLE(PhysicalPropEWeight_ALUMINUM_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_METAL_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_GRASS_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_SAND_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_FABRIC_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_ICE_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_AIR_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_WATER_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_ROCK_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_GLACIER_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_SNOW_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_SANDSTONE_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_MUD_MATERIAL,				1000)
FASTINTVARIABLE(PhysicalPropEWeight_BASALT_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_GROUND_MATERIAL,			1000)
FASTINTVARIABLE(PhysicalPropEWeight_CRACKED_LAVA_MATERIAL,		1000)

using namespace RBX;

static float getPropertyValueFromFlag(const int& flagVal)
{
	return (float)flagVal / 1000.0;
}

PhysicalProperties MaterialProperties::generatePhysicalMaterialFromPartMaterial(PartMaterial material)
{
	return PhysicalProperties(	getDefaultMaterialDensity(material),
								getDefaultMaterialFriction(material),
								getDefaultMaterialElasticity(material),
								getDefaultMaterialFrictionWeight(material),
								getDefaultMaterialElasticityWeight(material));
}

PhysicalProperties MaterialProperties::getPrimitivePhysicalProperties(Primitive* prim)
{
	if (prim->getPhysicalProperties().getCustomEnabled())
	{
		return prim->getPhysicalProperties();
	}
	else
	{
		return generatePhysicalMaterialFromPartMaterial(prim->getPartMaterial());
	}
}

float MaterialProperties::frictionBetweenPrimAndMaterial(Primitive* primA, PartMaterial materialB)
{
	PhysicalProperties aProp = primA->getPhysicalProperties();
	PhysicalProperties bProp = MaterialProperties::generatePhysicalMaterialFromPartMaterial(materialB);
	return calculateUsingWeightedAverage(aProp.getFrictionWeight(), aProp.getFriction(), bProp.getFrictionWeight(), bProp.getFriction());
}

float MaterialProperties::frictionBetweenMaterials(PartMaterial materialA, PartMaterial materialB)
{
	PhysicalProperties aProp = MaterialProperties::generatePhysicalMaterialFromPartMaterial(materialA);
	PhysicalProperties bProp = MaterialProperties::generatePhysicalMaterialFromPartMaterial(materialB);
	return calculateUsingWeightedAverage(aProp.getFrictionWeight(), aProp.getFriction(), bProp.getFrictionWeight(), bProp.getFriction());
}

void MaterialProperties::updateContactParamsPrims(ContactParams& params, Primitive* prim0, Primitive* prim1)
{
	const PhysicalProperties& prop0 = getPrimitivePhysicalProperties(prim0);
	const PhysicalProperties& prop1 = getPrimitivePhysicalProperties(prim1);

	params.kFriction	= calculateUsingWeightedAverage(prop0.getFrictionWeight(),	prop0.getFriction(), 
														prop1.getFrictionWeight(),	prop1.getFriction());
	params.kElasticity	= calculateUsingWeightedAverage(prop0.getElasticityWeight(),prop0.getElasticity(), 
														prop1.getElasticityWeight(),prop1.getElasticity());
	params.kSpring		= std::min( prim0->getJointK(), prim1->getJointK() );
	params.kNeg			= params.kSpring * Constants::getElasticMultiplier(params.kElasticity);
}

void MaterialProperties::updateContactParamsPrimMaterial(ContactParams& params, Primitive* prim, Primitive* otherPrim, PartMaterial otherMaterial)
{
	const PhysicalProperties& propPrim = getPrimitivePhysicalProperties(prim);
	const PhysicalProperties& propMaterial = generatePhysicalMaterialFromPartMaterial(otherMaterial);

	params.kFriction	= calculateUsingWeightedAverage(propPrim.getFrictionWeight(),		propPrim.getFriction(), 
														propMaterial.getFrictionWeight(),	propMaterial.getFriction());
	params.kElasticity	= calculateUsingWeightedAverage(propPrim.getElasticityWeight(),		propPrim.getElasticity(), 
														propMaterial.getElasticityWeight(), propMaterial.getElasticity());
	params.kSpring		= std::min( prim->getJointK(), otherPrim->getJointK() );
	params.kNeg			= params.kSpring * Constants::getElasticMultiplier(params.kElasticity);
}

float MaterialProperties::getDensity(Primitive* prim)
{
	if (prim->getPhysicalProperties().getCustomEnabled())
	{
		return prim->getPhysicalProperties().getDensity();
	}
	else
	{
		PartMaterial mat = prim->getPartMaterial();
		return getDefaultMaterialDensity(mat);
	}
}

float MaterialProperties::calculateUsingWeightedAverage(float weightX, float X, float weightY, float Y)
{
	// Divide by 0 guard
	float denominator = weightX + weightY;
	if (denominator <= 0.0f)
	{
		denominator = 0.1f;
	}
	// Simply get the weighted Average
	return (weightX * X + weightY * Y)/(weightX + weightY);
}

float MaterialProperties::getDefaultMaterialFriction(PartMaterial material)
{
	if (FFlag::ModifyDefaultMaterialProperties)
	{
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_PLASTIC_MATERIAL);	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return getPropertyValueFromFlag(FInt::PhysicalPropFriction_SMOOTH_PLASTIC_MATERIAL);	//TEFLON
			case NEON_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_NEON_MATERIAL);
			case WOOD_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_WOOD_MATERIAL);	//WOOD			
			case WOODPLANKS_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_WOODPLANKS_MATERIAL);  //WOOD
			case MARBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFriction_MARBLE_MATERIAL);
			case SLATE_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFriction_SLATE_MATERIAL);
			case CONCRETE_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_CONCRETE_MATERIAL);	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropFriction_GRANITE_MATERIAL);
			case BRICK_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFriction_BRICK_MATERIAL);
			case PEBBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFriction_PEBBLE_MATERIAL);
			case COBBLESTONE_MATERIAL:    return getPropertyValueFromFlag(FInt::PhysicalPropFriction_COBBLESTONE_MATERIAL);
			case RUST_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_RUST_MATERIAL);	//IRON
			case DIAMONDPLATE_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_DIAMONDPLATE_MATERIAL);  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return getPropertyValueFromFlag(FInt::PhysicalPropFriction_ALUMINUM_MATERIAL);  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFriction_METAL_MATERIAL);	//STEEL
			case GRASS_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFriction_GRASS_MATERIAL);
			case SAND_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFriction_SAND_MATERIAL);
			case FABRIC_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFriction_FABRIC_MATERIAL);
			case ICE_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFriction_ICE_MATERIAL);	//ICE
			case AIR_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropFriction_AIR_MATERIAL);
			case WATER_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFriction_WATER_MATERIAL);
			case ROCK_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFriction_ROCK_MATERIAL);
			case GLACIER_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropFriction_GLACIER_MATERIAL);
			case SNOW_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFriction_SNOW_MATERIAL);
			case SANDSTONE_MATERIAL:      return getPropertyValueFromFlag(FInt::PhysicalPropFriction_SANDSTONE_MATERIAL);
			case MUD_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropFriction_MUD_MATERIAL);
			case BASALT_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFriction_BASALT_MATERIAL);
			case GROUND_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFriction_GROUND_MATERIAL);
			case CRACKED_LAVA_MATERIAL:   return getPropertyValueFromFlag(FInt::PhysicalPropFriction_CRACKED_LAVA_MATERIAL);
			default:;
		}
	}
	else
	{
		// All uncommented values are guesses
		// http://www.engineeringtoolbox.com/friction-coefficients-d_778.html
		// Other results were adjusted from rought Testing results:
		// Ice, Acrylic (Plastic), Steel, Diamond Plate, Aluminum, Wood, Brick
		// See: https://docs.google.com/spreadsheets/d/1dwJYlGHSCAb0OU101bTWH-Q4eJyYSmXk85sQKBIVTuo/edit#gid=70389283
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return 0.30;	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return 0.20;	//TEFLON
			case NEON_MATERIAL:			  return 0.30;
			case WOOD_MATERIAL:			  return 0.48;	//WOOD			
			case WOODPLANKS_MATERIAL:	  return 0.48;  //WOOD
			case MARBLE_MATERIAL:         return 0.20;
			case SLATE_MATERIAL:          return 0.40;
			case CONCRETE_MATERIAL:		  return 0.70;	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return 0.40;
			case BRICK_MATERIAL:          return 0.80;
			case PEBBLE_MATERIAL:         return 0.40;
			case COBBLESTONE_MATERIAL:    return 0.50;
			case RUST_MATERIAL:			  return 0.70;	//IRON
			case DIAMONDPLATE_MATERIAL:	  return 0.35;  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return 0.40;  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return 0.40;	//STEEL
			case GRASS_MATERIAL:          return 0.40;
			case SAND_MATERIAL:           return 0.50;
			case FABRIC_MATERIAL:         return 0.35;
			case ICE_MATERIAL:			  return 0.02;	//ICE
			case AIR_MATERIAL:            return 0.01;
			case WATER_MATERIAL:          return 0.005;
			case ROCK_MATERIAL:           return 0.50;
			case GLACIER_MATERIAL:        return 0.05;
			case SNOW_MATERIAL:           return 0.30;
			case SANDSTONE_MATERIAL:      return 0.50;
			case MUD_MATERIAL:            return 0.30;
			case BASALT_MATERIAL:         return 0.70;
			case GROUND_MATERIAL:         return 0.45;
			case CRACKED_LAVA_MATERIAL:   return 0.65;
			default:;
		}
	}
	RBXASSERT(false); return 1;
}

float MaterialProperties::getDefaultMaterialFrictionWeight(PartMaterial material)
{
	if (FFlag::ModifyDefaultMaterialProperties)
	{
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_PLASTIC_MATERIAL);	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_SMOOTH_PLASTIC_MATERIAL);	//TEFLON
			case NEON_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_NEON_MATERIAL);
			case WOOD_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_WOOD_MATERIAL);	//WOOD			
			case WOODPLANKS_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_WOODPLANKS_MATERIAL);  //WOOD
			case MARBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_MARBLE_MATERIAL);
			case SLATE_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_SLATE_MATERIAL);
			case CONCRETE_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_CONCRETE_MATERIAL);	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_GRANITE_MATERIAL);
			case BRICK_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_BRICK_MATERIAL);
			case PEBBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_PEBBLE_MATERIAL);
			case COBBLESTONE_MATERIAL:    return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_COBBLESTONE_MATERIAL);
			case RUST_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_RUST_MATERIAL);	//IRON
			case DIAMONDPLATE_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_DIAMONDPLATE_MATERIAL);  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_ALUMINUM_MATERIAL);  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_METAL_MATERIAL);	//STEEL
			case GRASS_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_GRASS_MATERIAL);
			case SAND_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_SAND_MATERIAL);
			case FABRIC_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_FABRIC_MATERIAL);
			case ICE_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_ICE_MATERIAL);	//ICE
			case AIR_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_AIR_MATERIAL);
			case WATER_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_WATER_MATERIAL);
			case ROCK_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_ROCK_MATERIAL);
			case GLACIER_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_GLACIER_MATERIAL);
			case SNOW_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_SNOW_MATERIAL);
			case SANDSTONE_MATERIAL:      return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_SANDSTONE_MATERIAL);
			case MUD_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_MUD_MATERIAL);
			case BASALT_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_BASALT_MATERIAL);
			case GROUND_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_GROUND_MATERIAL);
			case CRACKED_LAVA_MATERIAL:   return getPropertyValueFromFlag(FInt::PhysicalPropFWeight_CRACKED_LAVA_MATERIAL);
			default:;
		}
	}
	else
	{
		switch (material)
		{
			// Add materials that need override here.
			// If no override present for material, it's 1.0f.
			// This is to prevent the painful life of anyone adding more materials
			// as they already have to deal with Density, Elasticity, Friction

			// example
			/*
			case ICE_MATERIAL:			return 10;
			*/
			case ICE_MATERIAL:			return 3;
			case BRICK_MATERIAL:		return 0.3;
			case CONCRETE_MATERIAL:     return 0.3;
			case SANDSTONE_MATERIAL:    return 5;
			case BASALT_MATERIAL:		return 0.3;
			case SAND_MATERIAL:			return 5;
			default:;
		}
	}
	return 1;
}

float MaterialProperties::getDefaultMaterialElasticity(PartMaterial material)
{
	if (FFlag::ModifyDefaultMaterialProperties)
	{
		switch(material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_PLASTIC_MATERIAL);	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_SMOOTH_PLASTIC_MATERIAL);	//TEFLON
			case NEON_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_NEON_MATERIAL);
			case WOOD_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_WOOD_MATERIAL);	//WOOD			
			case WOODPLANKS_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_WOODPLANKS_MATERIAL);  //WOOD
			case MARBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_MARBLE_MATERIAL);
			case SLATE_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_SLATE_MATERIAL);
			case CONCRETE_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_CONCRETE_MATERIAL);	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_GRANITE_MATERIAL);
			case BRICK_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_BRICK_MATERIAL);
			case PEBBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_PEBBLE_MATERIAL);
			case COBBLESTONE_MATERIAL:    return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_COBBLESTONE_MATERIAL);
			case RUST_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_RUST_MATERIAL);	//IRON
			case DIAMONDPLATE_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_DIAMONDPLATE_MATERIAL);  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_ALUMINUM_MATERIAL);  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_METAL_MATERIAL);	//STEEL
			case GRASS_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_GRASS_MATERIAL);
			case SAND_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_SAND_MATERIAL);
			case FABRIC_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_FABRIC_MATERIAL);
			case ICE_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_ICE_MATERIAL);	//ICE
			case AIR_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_AIR_MATERIAL);
			case WATER_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_WATER_MATERIAL);
			case ROCK_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_ROCK_MATERIAL);
			case GLACIER_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_GLACIER_MATERIAL);
			case SNOW_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_SNOW_MATERIAL);
			case SANDSTONE_MATERIAL:      return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_SANDSTONE_MATERIAL);
			case MUD_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_MUD_MATERIAL);
			case BASALT_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_BASALT_MATERIAL);
			case GROUND_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_GROUND_MATERIAL);
			case CRACKED_LAVA_MATERIAL:   return getPropertyValueFromFlag(FInt::PhysicalPropElasticity_CRACKED_LAVA_MATERIAL);
			default:;
		}
	}
	else
	{
		// Estimates based on limited Subset testing:
		// Brick, Aluminum, Wood, Acrylic (Plastic) were tested
		// See: https://docs.google.com/spreadsheets/d/1dwJYlGHSCAb0OU101bTWH-Q4eJyYSmXk85sQKBIVTuo/edit#gid=1422912501
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return 0.50;	
			case SMOOTH_PLASTIC_MATERIAL: return 0.50;	
			case NEON_MATERIAL:			  return 0.20;
			case WOOD_MATERIAL:			  return 0.20;			
			case WOODPLANKS_MATERIAL:	  return 0.20; 
			case MARBLE_MATERIAL:         return 0.17;
			case SLATE_MATERIAL:          return 0.20;
			case CONCRETE_MATERIAL:		  return 0.20;	
			case GRANITE_MATERIAL:        return 0.20;
			case BRICK_MATERIAL:          return 0.15;
			case PEBBLE_MATERIAL:         return 0.17;
			case COBBLESTONE_MATERIAL:    return 0.17;
			case RUST_MATERIAL:			  return 0.20;	
			case DIAMONDPLATE_MATERIAL:	  return 0.25;
			case ALUMINUM_MATERIAL:       return 0.25;  
			case METAL_MATERIAL:          return 0.25;	
			case GRASS_MATERIAL:          return 0.10;
			case SAND_MATERIAL:           return 0.05;
			case FABRIC_MATERIAL:         return 0.05;
			case ICE_MATERIAL:			  return 0.15;	
			case AIR_MATERIAL:            return 0.01;
			case WATER_MATERIAL:          return 0.10;
			case ROCK_MATERIAL:           return 0.17;
			case GLACIER_MATERIAL:        return 0.15;
			case SNOW_MATERIAL:           return 0.03;
			case SANDSTONE_MATERIAL:      return 0.15;
			case MUD_MATERIAL:            return 0.07;
			case BASALT_MATERIAL:         return 0.15;
			case GROUND_MATERIAL:         return 0.10;
			case CRACKED_LAVA_MATERIAL:   return 0.15;
			default:;
		}
	}
	RBXASSERT(false); return 0.50;
}

float MaterialProperties::getDefaultMaterialElasticityWeight(PartMaterial material)
{
	if (FFlag::ModifyDefaultMaterialProperties)
	{
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_PLASTIC_MATERIAL);	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_SMOOTH_PLASTIC_MATERIAL);	//TEFLON
			case NEON_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_NEON_MATERIAL);
			case WOOD_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_WOOD_MATERIAL);	//WOOD			
			case WOODPLANKS_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_WOODPLANKS_MATERIAL);  //WOOD
			case MARBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_MARBLE_MATERIAL);
			case SLATE_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_SLATE_MATERIAL);
			case CONCRETE_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_CONCRETE_MATERIAL);	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_GRANITE_MATERIAL);
			case BRICK_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_BRICK_MATERIAL);
			case PEBBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_PEBBLE_MATERIAL);
			case COBBLESTONE_MATERIAL:    return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_COBBLESTONE_MATERIAL);
			case RUST_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_RUST_MATERIAL);	//IRON
			case DIAMONDPLATE_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_DIAMONDPLATE_MATERIAL);  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_ALUMINUM_MATERIAL);  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_METAL_MATERIAL);	//STEEL
			case GRASS_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_GRASS_MATERIAL);
			case SAND_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_SAND_MATERIAL);
			case FABRIC_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_FABRIC_MATERIAL);
			case ICE_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_ICE_MATERIAL);	//ICE
			case AIR_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_AIR_MATERIAL);
			case WATER_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_WATER_MATERIAL);
			case ROCK_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_ROCK_MATERIAL);
			case GLACIER_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_GLACIER_MATERIAL);
			case SNOW_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_SNOW_MATERIAL);
			case SANDSTONE_MATERIAL:      return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_SANDSTONE_MATERIAL);
			case MUD_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_MUD_MATERIAL);
			case BASALT_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_BASALT_MATERIAL);
			case GROUND_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_GROUND_MATERIAL);
			case CRACKED_LAVA_MATERIAL:   return getPropertyValueFromFlag(FInt::PhysicalPropEWeight_CRACKED_LAVA_MATERIAL);
			default:;
		}
	}
	else
	{
		switch (material)
		{
			// Add materials that need override here.
			// If no override present for material, it's 1.0f.
			// This is to prevent the painful life of anyone adding more materials
			// as they already have to deal with Density, Elasticity, Friction

			// example
			/*
			case ICE_MATERIAL:			return 10;
			*/
			case ICE_MATERIAL:			return 1;
			default:;
		}
	}
	return 1;
}

float MaterialProperties::getDefaultMaterialDensity(PartMaterial material)
{
	if (FFlag::ModifyDefaultMaterialProperties)
	{
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_PLASTIC_MATERIAL);	//NYLON
			case SMOOTH_PLASTIC_MATERIAL: return getPropertyValueFromFlag(FInt::PhysicalPropDensity_SMOOTH_PLASTIC_MATERIAL);	//TEFLON
			case NEON_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_NEON_MATERIAL);
			case WOOD_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_WOOD_MATERIAL);	//WOOD			
			case WOODPLANKS_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_WOODPLANKS_MATERIAL);  //WOOD
			case MARBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropDensity_MARBLE_MATERIAL);
			case SLATE_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropDensity_SLATE_MATERIAL);
			case CONCRETE_MATERIAL:		  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_CONCRETE_MATERIAL);	//CONCRETE SEE 11.7.4.3 of https://law.resource.org/pub/us/cfr/ibr/001/aci.318.1995.pdf
			case GRANITE_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropDensity_GRANITE_MATERIAL);
			case BRICK_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropDensity_BRICK_MATERIAL);
			case PEBBLE_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropDensity_PEBBLE_MATERIAL);
			case COBBLESTONE_MATERIAL:    return getPropertyValueFromFlag(FInt::PhysicalPropDensity_COBBLESTONE_MATERIAL);
			case RUST_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_RUST_MATERIAL);	//IRON
			case DIAMONDPLATE_MATERIAL:	  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_DIAMONDPLATE_MATERIAL);  //GUESS: based on Aluminum and Steel experimental data.
			case ALUMINUM_MATERIAL:       return getPropertyValueFromFlag(FInt::PhysicalPropDensity_ALUMINUM_MATERIAL);  //GUESS: Manually tested small bricks of Aluminum
			case METAL_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropDensity_METAL_MATERIAL);	//STEEL
			case GRASS_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropDensity_GRASS_MATERIAL);
			case SAND_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropDensity_SAND_MATERIAL);
			case FABRIC_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropDensity_FABRIC_MATERIAL);
			case ICE_MATERIAL:			  return getPropertyValueFromFlag(FInt::PhysicalPropDensity_ICE_MATERIAL);	//ICE
			case AIR_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropDensity_AIR_MATERIAL);
			case WATER_MATERIAL:          return getPropertyValueFromFlag(FInt::PhysicalPropDensity_WATER_MATERIAL);
			case ROCK_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropDensity_ROCK_MATERIAL);
			case GLACIER_MATERIAL:        return getPropertyValueFromFlag(FInt::PhysicalPropDensity_GLACIER_MATERIAL);
			case SNOW_MATERIAL:           return getPropertyValueFromFlag(FInt::PhysicalPropDensity_SNOW_MATERIAL);
			case SANDSTONE_MATERIAL:      return getPropertyValueFromFlag(FInt::PhysicalPropDensity_SANDSTONE_MATERIAL);
			case MUD_MATERIAL:            return getPropertyValueFromFlag(FInt::PhysicalPropDensity_MUD_MATERIAL);
			case BASALT_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropDensity_BASALT_MATERIAL);
			case GROUND_MATERIAL:         return getPropertyValueFromFlag(FInt::PhysicalPropDensity_GROUND_MATERIAL);
			case CRACKED_LAVA_MATERIAL:   return getPropertyValueFromFlag(FInt::PhysicalPropDensity_CRACKED_LAVA_MATERIAL);
			default:;
		}
	}
	else
	{
		// http://www.simetric.co.uk/si_materials.htm
		switch (material)
		{
			case LEGACY_MATERIAL:
			case PLASTIC_MATERIAL: 
			case SMOOTH_PLASTIC_MATERIAL: 
			case NEON_MATERIAL:
				return 0.7;
			case WOOD_MATERIAL: return 0.350;				// Pine
			case WOODPLANKS_MATERIAL: return 0.350;
			case MARBLE_MATERIAL: return 2.563;
			case SLATE_MATERIAL: return 2.691;
			case CONCRETE_MATERIAL: return 2.403;
			case GRANITE_MATERIAL: return 2.691;
			case BRICK_MATERIAL: return 1.922;
			case PEBBLE_MATERIAL: return 2.403;
			case COBBLESTONE_MATERIAL: return 2.691;
			case RUST_MATERIAL: return 7.850;
			case DIAMONDPLATE_MATERIAL: return 7.850;
			case ALUMINUM_MATERIAL: return 7.700;
			case METAL_MATERIAL: return 7.850;
			case GRASS_MATERIAL: return 0.9;
			case SAND_MATERIAL: return 1.602;
			case FABRIC_MATERIAL: return 0.7;
			case ICE_MATERIAL: return 0.919;
			case AIR_MATERIAL: return 0;
			case WATER_MATERIAL: return 1;
			case ROCK_MATERIAL: return 2.691;
			case GLACIER_MATERIAL: return 0.919;
			case SNOW_MATERIAL: return 0.9;
			case SANDSTONE_MATERIAL: return 2.691;
			case MUD_MATERIAL: return 0.9;
			case BASALT_MATERIAL: return 2.691;
			case GROUND_MATERIAL: return 0.9;
			case CRACKED_LAVA_MATERIAL: return 2.691;
			default:;
		}
	}
	RBXASSERT(false); return 1;
}