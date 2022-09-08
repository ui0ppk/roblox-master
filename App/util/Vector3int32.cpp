#include "stdafx.h"

#include "Util/Vector3int32.h"

namespace RBX {

	std::ostream& operator<<(std::ostream& os, const Vector3int32& v) {
		return os << G3D::format("(%d, %d, %d)", v.x, v.y, v.z);
	}

	::std::size_t hash_value(const RBX::Vector3int32& v)
	{
		size_t seed = 0;
		boost::hash_combine(seed, v.x);
		boost::hash_combine(seed, v.y);
		boost::hash_combine(seed, v.z);
		return seed;
	}
}
