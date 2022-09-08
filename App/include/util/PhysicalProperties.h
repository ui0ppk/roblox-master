#pragma once
#include <boost/functional/hash.hpp>
#include "Util/Math.h"

namespace RBX {

enum PhysicalPropertiesMode
{
    PhysicalPropertiesMode_Legacy,
    PhysicalPropertiesMode_Default,
    PhysicalPropertiesMode_NewPartProperties
};
    
    
class PhysicalProperties
{
private:
	bool customEnabled;
	float density;
	float elasticity;
	float friction;
	float frictionWeight;
	float elasticityWeight;

	static float minDen() { return 0.01f; }
	static float maxDen() { return 100.0f;}
	static float minFri() { return 0.0f; } // Negative friction Generates energy
	static float maxFri() { return 2.0f; } 
	static float minFrW() { return 0.0f; }
	static float maxFrW() { return 100.0f;}
	static float minEla() { return 0.0f; } // Negative Elasticity causes penetration
	static float maxEla() { return 1.0f; } // Elasticity > 1 causes energy gain
	static float minElW() { return 0.0f; }
	static float maxElW() { return 100.0f;}


public:
	// Default Constructor for initializing part instances
	PhysicalProperties():
	customEnabled(false),
	density(0),
	friction(0),
	elasticity(0),
	frictionWeight(0),
	elasticityWeight(0)
	{
	}

	// Constructor for enabling Custom
	PhysicalProperties(float density_, float friction_, float elasticity_, float frictionWeight_ = 1.0f, float elasticityWeight_ = 1.0f):
	customEnabled(true),
	density			(G3D::clamp(density_, minDen(), maxDen())),
	friction		(G3D::clamp(friction_, minFri(), maxFri())),
	elasticity		(G3D::clamp(elasticity_, minEla(), maxEla())),
	frictionWeight	(G3D::clamp(frictionWeight_, minFrW(), maxFrW())),
	elasticityWeight(G3D::clamp(elasticityWeight_, minElW(), maxElW()))
	{
	}

	size_t hashCode() const;

	bool getCustomEnabled() const
	{
		return customEnabled;
	}

	void setCustomEnabled( bool value )
	{
		customEnabled = value;
	}

	float getDensity() const
	{
		return density;
	}

	float getFriction() const
	{
		return friction;
	}

	float getElasticity() const
	{
		return elasticity;
	}

	float getFrictionWeight() const
	{
		return frictionWeight;
	}

	float getElasticityWeight() const
	{
		return elasticityWeight;
	}

	//Operators

	bool operator==(const PhysicalProperties& other) const 
	{
		return ((customEnabled	== other.customEnabled) &&
			   (density			== other.density) &&
			   (friction		== other.friction) &&
			   (elasticity		== other.elasticity) &&
			   (frictionWeight  == other.frictionWeight) &&
			   (elasticityWeight== other.elasticityWeight));
	}

	bool operator!=(const PhysicalProperties& other) const 
	{
		return !(*this == other);
	}
};

size_t hash_value(const PhysicalProperties& properties);

}; //Namespace RBX