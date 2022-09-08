#pragma once

#include "Util/G3DCore.h"
#include "Voxel/Util.h"

namespace RBX { namespace Voxel {

// Read-only view of a contiguous, axis-aligned subsection of the entire voxel
// grid.
template<class InternalStorageType>
class Region {
public:
	class iterator;
	class xline_iterator;

	Region();
	Region(const InternalStorageType* internalStorage,
		const Vector3int16& minCoords, const Vector3int16& maxCoords);

	// Returns true if all cells in this iteration are definitely empty.
	// May return false if all cells are empty, but will never return true
	// if some cells are set.
	bool isGuaranteedAllEmpty() const;
	
	// returns true if the global coordinate is queryable in this region
	bool contains(const Vector3int16& globalCoord) const;

	// methods for querying voxel related data for a global voxel coordinate.
	inline const Cell& voxelAt(const Vector3int16& globalCoord) const;
	inline CellMaterial materialAt(const Vector3int16& globalCoord) const;
	inline bool hasWaterAt(const Vector3int16& globalCoord) const;

	// methods to make this an iterable container
	iterator begin() const;
	const iterator& end() const;

	xline_iterator xLineBegin() const;
	const xline_iterator& xLineEnd() const;

	// Support methods
	Region& operator=(const Region& other);
	bool operator==(const Region& other) const;

private:
	static const Region kEndRegion;
	static const iterator kEndIterator;
	static const xline_iterator kEndXLineIterator;

	const InternalStorageType* internalStorage;
	Vector3int16 minCoords;
	Vector3int16 maxCoords;

	inline const Cell& voxelAtSkipAllEmptyCheck(const Vector3int16& globalCoord) const;
	inline bool hasWaterAtSkipAllEmptyCheck(const Cell& cell,
		const Vector3int16& globalCoord) const;

};

// Iterator for accessing all voxels inside a Region sequentially.
// Iterates in Y-Z-X order (x axis is least significant ordered, y axis is
// most significant).
template<class InternalStorageType>
class Region<InternalStorageType>::iterator {
private:
	const Region<InternalStorageType>& owningRegion;

	const Vector3int16 rangeSize;
	unsigned int pointerSkipAtEndOfXLine;
	unsigned int pointerSkipAtEndOfZLine;

	// Internal iteration counters
	unsigned int xCounter, zCounter;
	bool reachedEnd;

	// cached values (saved so that they aren't re-derived on each access)
	Vector3int16 currentLocation; 
	unsigned int currentIndex;
	const Cell* currentCell;

public:
	iterator(const Region<InternalStorageType>& owningRegion);

	//////////////////////////////////////////////////
	// Reading data

	// Read at current location
	inline const Vector3int16& getCurrentLocation() const;
	inline const Cell& getCellAtCurrentLocation() const;
	inline bool hasWaterAtCurrentLocation() const;
	inline CellMaterial getMaterialAtCurrentLocation() const;

	// Read neighbors of current location.
	// These methods should only be used when the caller knows that it is safe
	// to query those cells (that the cells are contained in the Region)
	inline const Cell& getNeighborCell(FaceDirection direction) const;
	inline const Cell& getNeighborCell(FaceDirection direction1, FaceDirection direction2) const;
	inline CellMaterial getNeighborMaterial(FaceDirection direction) const;
	inline CellMaterial getNeighborMaterial(FaceDirection direction1, FaceDirection direction2) const;
	inline const Cell& getArbitraryNeighborCell(const Vector3int16& neighborOffsets) const;
	inline bool hasWaterAtNeighbor(const FaceDirection& direction) const;

	////////////////////////////////////////////
	// Normal iterator business

	// ++prefix form
	inline iterator& operator++();

	// operator== for terminating condition
	inline bool operator==(const iterator& other);
	inline bool operator!=(const iterator& other);
};

// Limited iterator. Iterates over the region, in increments equal to the
// X-Axis dimension of the region (aka over the the "xline"s of the region).
// Allows working with the entire line in one shot, to enhance performance of
// bulk operations like copying.
template<class InternalStorageType>
class Region<InternalStorageType>::xline_iterator {
	const Region& owningRegion;
	const unsigned int lineSize;
	const int minZ;
	const unsigned int zDimSize;
	const int maxY;

	unsigned int pointerSkipAtEndOfXLine;
	unsigned int pointerSkipAtEndOfZLine;

	unsigned int zDimCounter;

	Vector3int16 currentLocation;
	unsigned int currentIndex;
	const Cell* currentCell;
	bool reachedEnd;
public:

	xline_iterator(const Region& owningRegion);

	const Vector3int16& getCurrentLocation() const;
	unsigned int getLineSize() const;

	// Returns a contiguous array of Cells that has exactly lineSize elements.
	// The first cell corresponds to the current location, and progress along
	// the positive X axis.
	const Cell* getLineCells() const;
	// Returns a contiguous array of unsigned char with exactly lineSize/2
	// elements. This is half-byte material information for the x line.
	const unsigned char* getLineMaterials() const;

	bool operator==(const xline_iterator& other) const;
	bool operator!=(const xline_iterator& other) const;
	inline xline_iterator& operator++();
};

} }

#include "Voxel/Region.inl"
#include "Voxel/Region.iterator.inl"
#include "Voxel/Region.xline_iterator.inl"
