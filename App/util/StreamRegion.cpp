#include "stdafx.h"

#include "util/StreamRegion.h"

namespace RBX {
	namespace StreamRegion {

		std::size_t hash_value(const Id& key) {
			Id::boost_compatible_hash_value hash;
			return hash(key);
		}
	}
}