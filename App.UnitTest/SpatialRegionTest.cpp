#include <boost/test/unit_test.hpp>

#include "Util/SpatialRegion.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( SpatialRegionTest )

BOOST_AUTO_TEST_CASE( RegionContainingVoxel ) {
	SpatialRegion::Id id = SpatialRegion::regionContainingVoxel(Vector3int16::zero());
	BOOST_CHECK_EQUAL(Vector3int16::zero(), id.value());

	id = SpatialRegion::regionContainingVoxel(SpatialRegion::getMaxVoxelOffsetInsideRegion());
	BOOST_CHECK_EQUAL(Vector3int16::zero(), id.value());

	id = SpatialRegion::regionContainingVoxel(Vector3int16(-1,-1,-1));
	BOOST_CHECK_EQUAL(Vector3int16(-1,-1,-1), id.value());

	id = SpatialRegion::regionContainingVoxel(Vector3int16::zero() -
		SpatialRegion::getMaxVoxelOffsetInsideRegion());
	BOOST_CHECK_EQUAL(Vector3int16(-1,-1,-1), id.value());
}

BOOST_AUTO_TEST_CASE( GlobalCoordStuds ) {
    const int kOffset = 0;

	SpatialRegion::Id id(Vector3int16(kOffset,0,kOffset));
	BOOST_CHECK_EQUAL(Vector3int32::zero(),
		SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(id));
	BOOST_CHECK_EQUAL(Vector3int32(64, 32, 64),
		SpatialRegion::centerOfRegionInGlobalCoordStuds(id));
	BOOST_CHECK_EQUAL(Vector3int32(128, 64, 128),
		SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(id));

	id = SpatialRegion::Id(Vector3int16(kOffset-8,0,kOffset-8));
	BOOST_CHECK_EQUAL(Vector3int32(-1024, 0, -1024),
		SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(id));
	BOOST_CHECK_EQUAL(Vector3int32(-960, 32, -960),
		SpatialRegion::centerOfRegionInGlobalCoordStuds(id));
	BOOST_CHECK_EQUAL(Vector3int32(-896, 64, -896),
		SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(id));
}

BOOST_AUTO_TEST_SUITE_END()
