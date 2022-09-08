/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SpatialHashMultiRes.h"
#include "V8World/Tolerance.h"

namespace RBX {

// statics - SpatialHashStatic
const int SpatialHashStatic::cellMinSize = 8;
const int SpatialHashStatic::maxLevelForAnchored = 2;				// level 2

// sort by the dot product of( the position of the offsets in space by the visitDir ).
	struct SortOffsetByVisitDirInt
    {
		SortOffsetByVisitDirInt(const Vector3& visitDir) 
			: visitDir(visitDir) {};
		const Vector3& visitDir;

		bool operator()(int offseta, int offsetb)
		{
			Vector3 va(float(offseta & 1), float(offseta & 2), float(offseta & 4));
			Vector3 vb(float(offsetb & 1), float(offsetb & 2), float(offsetb & 4));
			return va.dot(visitDir) < vb.dot(visitDir);
		}
	};

void SpatialHashStatic::makeVisitOrder(int* offsets, const Vector3& visitDir)
{
	for(int i = 0; i < 8; ++i)
	{
		offsets[i] = i;
	}

	SortOffsetByVisitDirInt sortPred(visitDir);

	std::sort(offsets, offsets+8, sortPred);
}

/*
See: http://graphics.ethz.ch/Downloads/Publications/Papers/2003/tes03/p_tes03.pdf
"Optimized Spatial Hashing for Collision Detection of Deformable Objects"

hash functions. It gets three values, describing
vertex position (x, y, z), and returns a hash
hash(x,y,z) = ( x p1 xor y p2 xor z p3) mod
where p1, p2, p3 are large prime numbers,
our case 73856093, 19349663, 83492791, respectively.
The value n is the hash table size.
*/

/*
	12/5/05 - 
	Try using these numbers:

	10243, 12553, 14771

*/

int SpatialHashStatic::getHash(int level, const Vector3int32& grid)
{
	unsigned int x = grid.x & 0xFFFFFFFF;
	unsigned int y = grid.y & 0xFFFFFFFF;
	unsigned int z = grid.z & 0xFFFFFFFF;

	unsigned int xProd = x * 73856093;
	unsigned int yProd = y * 19349663;
	unsigned int zProd = z * 83492791;

	int hash = xProd ^ yProd ^ zProd;
    
    return hash & (numBuckets(level) - 1);
}

void SpatialHashStatic::computeMinMax(const int level, const Extents& extents, Vector3int32& min, Vector3int32& max)
{
	RBXASSERT(level != -1);

	Extents temp = extents.clampInsideOf(Tolerance::maxExtents());

	RBXASSERT_IF_VALIDATING(Extents::unit().contains(temp.min() * 1e-6f));		// i.e. +/- 1,000,000
	RBXASSERT_IF_VALIDATING(Extents::unit().contains(temp.max() * 1e-6f));		// i.e. +/- 1,000,000

	min = realToHashGrid(level, temp.min());
	max = realToHashGrid(level, temp.max());
}

} // namespace

