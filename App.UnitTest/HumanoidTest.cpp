#include <boost/test/unit_test.hpp>

#include "Humanoid/Humanoid.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(HumanoidTest)

BOOST_AUTO_TEST_CASE(SetVelocityBeforeHumanoidIsPutIntoWorld) {
	shared_ptr<Humanoid> humanoid = Creatable<Instance>::create<Humanoid>();
	Vector3 direction(1,2,3);
	direction = direction.unit();
	// assert that we can set the walk direction before creating current state
	humanoid->setWalkDirection(direction);
	BOOST_REQUIRE(humanoid->getWalkDirection().fuzzyEq(direction));
}

BOOST_AUTO_TEST_SUITE_END()