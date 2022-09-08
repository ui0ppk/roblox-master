#pragma once

#include "Util/SpatialRegion.h"

#include <vector>
#include <boost/unordered_map.hpp>

namespace RBX { namespace Voxel {

// Associative container for mapping SpatialRegion::Id to a value type.
// ValueType should implement no-arg constructor and assignment operator.
template<class ValueType>
class ChunkMap {
	typedef boost::unordered_map<SpatialRegion::Id, ValueType, SpatialRegion::Id::boost_compatible_hash_value> ValueMap;

public:
	ChunkMap();

	// Mutating accessor, will insert a new ValueType if the id wasn't already
	// contained in this container.
	ValueType& insert(const SpatialRegion::Id& id);

	// Constant accessor, returns NULL if id is not contained.
	const ValueType* find(const SpatialRegion::Id& id) const;
	ValueType* find(const SpatialRegion::Id& id);

	// Removes the key/value pair for the given id. Does nothing if the key
	// is not present.
	void erase(const SpatialRegion::Id& id);

    // Get all chunks
    std::vector<SpatialRegion::Id> getChunks() const;

    // Get number of chunks
    size_t size() const;

private:
	ValueMap values;
};

} }

#include "ChunkMap.inl"
