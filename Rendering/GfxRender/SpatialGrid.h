#pragma once

#include <boost/unordered_map.hpp>
#include <boost/pool/pool_alloc.hpp>

#include "v8datamodel/PartInstance.h"
#include "v8world/Primitive.h"
#include "Util.h"

namespace RBX
{
namespace Graphics
{

	struct SpatialGridIndex
	{
		enum Flags
		{
			fLarge = 1 << 0,
			fFW = 1 << 1
		};
		
		Vector3int16 position;
		unsigned short flags;
		
		SpatialGridIndex(): position(0, 0, 0), flags(0)
		{
		}
		
		SpatialGridIndex(const Vector3int16& position, unsigned short flags): position(position), flags(flags)
		{
		}
	};
	
	inline bool operator==(const SpatialGridIndex& lhs, const SpatialGridIndex& rhs)
	{
		return lhs.position == rhs.position && lhs.flags == rhs.flags;
	}
	
	inline bool operator!=(const SpatialGridIndex& lhs, const SpatialGridIndex& rhs)
	{
		return lhs.position != rhs.position || lhs.flags != rhs.flags;
	}
	
	inline size_t hash_value(const SpatialGridIndex& value)
	{
		size_t result = 0;
		boost::hash_combine(result, value.position.x);
		boost::hash_combine(result, value.position.y);
		boost::hash_combine(result, value.position.z);
		boost::hash_combine(result, value.flags);
		return result;
	}
	
	template <class Cluster> class SpatialGrid
	{
	public:
		struct Cell
		{
			Cluster* cluster;
		};
		
		SpatialGrid(const Vector3& cellExtents, float largeCoeff)
			: mInvCellExtents(Vector3::one() / cellExtents)
			, mCellExtentsLarge(cellExtents * largeCoeff)
		{
		}
		
		~SpatialGrid()
		{
			for (typename SpatialMap::iterator it = mMap.begin(); it != mMap.end(); ++it)
				delete it->second.cluster;
		}
		
		Cell* requestCell(const SpatialGridIndex& index)
		{
			return &mMap[index];
		}
		
		size_t removeCell(const SpatialGridIndex& index)
		{
			return mMap.erase(index);
		}
		
		const Cell* getCell(const SpatialGridIndex& index) const
		{
			typename SpatialMap::const_iterator it = mMap.find(index);
			
			return (it != mMap.end()) ? &it->second : NULL;
		}
		
		SpatialGridIndex getIndexUnsafe(RBX::PartInstance* part, unsigned short flags) const
		{
			Primitive* prim = part->getPartPrimitive();
			
			const Vector3& center = prim->getCoordinateFrameUnsafe().translation; // This never was safe
			const Vector3& size = prim->getSize();
			
			Vector3int16 gridPos = fastFloorInt(center * mInvCellExtents);
			bool large = size.x >= mCellExtentsLarge.x || size.y >= mCellExtentsLarge.y || size.z >= mCellExtentsLarge.z;
			
			return SpatialGridIndex(gridPos, flags | (large ? SpatialGridIndex::fLarge : 0));
		}

		std::vector<Cluster*> getClusters() const
		{
			std::vector<Cluster*> result;
			
			for (typename SpatialMap::const_iterator it = mMap.begin(); it != mMap.end(); ++it)
				result.push_back(it->second.cluster);
				
			return result;
		}

	private:
		typedef boost::unordered_map<SpatialGridIndex, Cell, boost::hash<SpatialGridIndex>, std::equal_to<SpatialGridIndex>, boost::fast_pool_allocator<SpatialGridIndex> > SpatialMap;
		SpatialMap mMap;
		
		Vector3 mInvCellExtents;
		Vector3 mCellExtentsLarge;
	};

}
}
