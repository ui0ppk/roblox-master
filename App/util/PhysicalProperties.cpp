#include "stdafx.h"

#include "util/PhysicalProperties.h"

namespace RBX {

size_t PhysicalProperties::hashCode() const
{
	size_t seed = 0;
	boost::hash_combine(seed, customEnabled);
	boost::hash_combine(seed, density);
	boost::hash_combine(seed, friction);
	boost::hash_combine(seed, frictionWeight);
	boost::hash_combine(seed, elasticity);
	boost::hash_combine(seed, elasticityWeight);
	return seed;
}

size_t hash_value(const PhysicalProperties& properties)
{
		return properties.hashCode();
}

}
