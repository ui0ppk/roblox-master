#pragma once

#include "Util/G3DCore.h"
#include "Voxel/Cell.h"
#include "Voxel/Region.h"
#include "Voxel/Water.h"

#include <boost/scoped_ptr.hpp>
#include <vector>

namespace RBX { namespace Voxel {

// Helper object for tasks that need fast access to voxel cells across
// SpatialRegion boundaries. Keeps an internal buffer of cells, size determined
// by template parameters. Buffer can be refreshed from another voxel storage
// mechanism (or another AreaCopy) repeatedly.
template<unsigned int XDim, unsigned int YDim, unsigned int ZDim>
class AreaCopy {
	// Helper object to comply with the Region API
	class Chunk {
		static const int kSize = XDim * YDim * ZDim;
		
		std::vector<Cell> cells;
		std::vector<unsigned char> materials;
		Vector3int16 firstCellLocation;
		bool isAllEmpty;

		bool contains(const Vector3int16& cellLoc) const;
		void fillEmpty(const Vector3int16& minLoc, const Vector3int16& maxLoc);
		template<class RegionType>
		void fillFromRegion(const RegionType& region);

	public:
		// Chunk API
		static const int kXOffsetMultiplier = 1;
		static const int kYOffsetMultiplier = XDim * ZDim;
		static const int kZOffsetMultiplier = XDim;

		static int kFaceDirectionToPointerOffset[7];
		static int voxelCoordOffsetToIndexOffset(const Vector3int16& offsetCoord);
		int voxelCoordToArrayIndex(const Vector3int16& globalCoord) const;
		const std::vector<Cell>& getConstData() const;
		const std::vector<unsigned char>& getConstMaterial() const;
		void fillLocalAreaInfo(const Vector3int16& loc,
			const Water::RelevantNeighbors& neighbors, Water::LocalAreaInfo* out)
			const;

		// other methods
		template<class Source>
		void loadData(const Source* source, const Vector3int16& firstCellLocation);
		bool getIsAllEmpty() const;
	};

	Chunk storage;

public:
	typedef RBX::Voxel::Region<Chunk> Region;
	static const Region kStaticEndRegion;

	Region getRegion(const Vector3int16& minCoords, const Vector3int16& maxCoords) const;

	template<class Source>
	void loadData(const Source* source, const Vector3int16& rootCell);
};

} }

#include "AreaCopy.inl"
