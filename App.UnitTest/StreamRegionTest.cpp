#include <boost/test/unit_test.hpp>

#include "Util/StreamRegion.h"
#include "Util/SpatialRegion.h"
#include "Voxel/Util.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( StreamRegionTest )

BOOST_AUTO_TEST_CASE( SpatialRegionToWorldTest ) {
	SpatialRegion::Id min(0,0,0);
	SpatialRegion::Id max(15, 3, 15);

	Vector3int32 vmin = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(min);
	Vector3int32 vmax = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(max);

    BOOST_CHECK_EQUAL(0, vmin.x);
    BOOST_CHECK_EQUAL(0, vmin.y);
    BOOST_CHECK_EQUAL(0, vmin.z);

    BOOST_CHECK_EQUAL(1920, vmax.x);
    BOOST_CHECK_EQUAL(192, vmax.y);
    BOOST_CHECK_EQUAL(1920, vmax.z);
}

// Test that the smallest corner of StreamRegion limits (-16,0,-16) and (14,0,14) correspond
// to the smallest corner of SpatialRegion limits of (0,0,0) and (15,3,15).
BOOST_AUTO_TEST_CASE( SmallestCornerTest ) {
	SpatialRegion::Id spatialMin(0,0,0);
	SpatialRegion::Id spatialMax(15, 3, 15);

	Vector3int32 spatialVMin = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(spatialMin);
	Vector3int32 spatialVMax = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(spatialMax);

	StreamRegion::Id streamMin(Vector3int32(0,0,0));
	StreamRegion::Id streamMax(Vector3int32(30,3,30));

	Vector3int32 streamVMin = Vector3int32::floor(StreamRegion::extentsFromRegionId(streamMin).min());
	Vector3int32 streamVMax = Vector3int32::floor(StreamRegion::extentsFromRegionId(streamMax).min());

	BOOST_CHECK_EQUAL(spatialVMin, streamVMin);
	BOOST_CHECK_EQUAL(spatialVMax, streamVMax);
}

BOOST_AUTO_TEST_CASE( LargestCornerTest ) {
	SpatialRegion::Id spatialMin(0,0,0);
	SpatialRegion::Id spatialMax(15, 3, 15);

	Vector3int32 spatialVMin = SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(spatialMin);
	Vector3int32 spatialVMax = SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(spatialMax);

	StreamRegion::Id streamMin(Vector3int32(1,0,1));
	StreamRegion::Id streamMax(Vector3int32(31,3,31));

	Vector3int32 streamVMin = Vector3int32::floor(StreamRegion::extentsFromRegionId(streamMin).max());
	Vector3int32 streamVMax = Vector3int32::floor(StreamRegion::extentsFromRegionId(streamMax).max());

	BOOST_CHECK_EQUAL(spatialVMin, streamVMin);
	BOOST_CHECK_EQUAL(spatialVMax, streamVMax);
}

BOOST_AUTO_TEST_CASE( SmallestVoxelTest ) {
	SpatialRegion::Id spatialMin(0,0,0);
	SpatialRegion::Id spatialMax(15, 3, 15);

	Vector3int16 spatialVMin = SpatialRegion::inclusiveVoxelExtentsOfRegion(spatialMin).getMinPos();
	Vector3int16 spatialVMax = SpatialRegion::inclusiveVoxelExtentsOfRegion(spatialMax).getMinPos();

	StreamRegion::Id streamMin(Vector3int32(0,0,0));
	StreamRegion::Id streamMax(Vector3int32(30,3,30));

	Vector3int16 streamVMin = StreamRegion::getMinVoxelCoordinateInsideRegion(streamMin);
	Vector3int16 streamVMax = StreamRegion::getMinVoxelCoordinateInsideRegion(streamMax);

	BOOST_CHECK_EQUAL(spatialVMin, streamVMin);
	BOOST_CHECK_EQUAL(spatialVMax, streamVMax);
}

BOOST_AUTO_TEST_CASE( RegionIdCreateTest ) {
	Vector3 worldPos(-0.5f, 1.0f, 0.75f);
	StreamRegion::Id regionId = StreamRegion::regionContainingWorldPosition(worldPos);

	BOOST_CHECK_EQUAL(Vector3int32(-1, 0, 0), regionId.value());
}

BOOST_AUTO_TEST_CASE( ExtentsTest ) {
	Vector3 worldPos(-0.5f, 1.0f, 0.75f);
	StreamRegion::Id regionId = StreamRegion::regionContainingWorldPosition(worldPos);

	Extents extents = StreamRegion::extentsFromRegionId(regionId);
	BOOST_CHECK_EQUAL(Vector3(-64.0f, 0.0f, 0.0f), extents.min());
	BOOST_CHECK_EQUAL(Vector3(0.0f, 64.0f, 64.0f), extents.max());
}

BOOST_AUTO_TEST_SUITE_END()
