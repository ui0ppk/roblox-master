#pragma once

#include "Voxel/Cell.h"

namespace RBX { namespace Voxel {

template <class ValueType> ChunkMap<ValueType>::ChunkMap()
{
}

template <class ValueType> ValueType& ChunkMap<ValueType>::insert(const SpatialRegion::Id& id)
{
    return values[id];
}

template <class ValueType> const ValueType* ChunkMap<ValueType>::find(const SpatialRegion::Id& id) const
{
	typename ValueMap::const_iterator it = values.find(id);

    return (it == values.end()) ? NULL : &it->second;
}

template <class ValueType> ValueType* ChunkMap<ValueType>::find(const SpatialRegion::Id& id)
{
	typename ValueMap::iterator it = values.find(id);

    return (it == values.end()) ? NULL : &it->second;
}

template <class ValueType> void ChunkMap<ValueType>::erase(const SpatialRegion::Id& id)
{
    values.erase(id);
}

template <class ValueType> std::vector<SpatialRegion::Id> ChunkMap<ValueType>::getChunks() const
{
    std::vector<SpatialRegion::Id> result;
    result.reserve(values.size());

    for (typename ValueMap::const_iterator it = values.begin(); it != values.end(); ++it)
        result.push_back(it->first);

    return result;
}

template <class ValueType> size_t ChunkMap<ValueType>::size() const
{
    return values.size();
}

} }
