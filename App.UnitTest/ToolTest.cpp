#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/Tool.h"

using namespace RBX;

struct CallbackCounter {
	shared_ptr<Mouse> expectedMouse;
	unsigned int count;
	CallbackCounter() : count(0) {}

	void callback(const shared_ptr<Instance> mouse) {
		BOOST_CHECK_EQUAL(expectedMouse.get(), mouse.get());
		count++;
	}
};

BOOST_AUTO_TEST_SUITE( ToolTest ) // avoid name conflict with "Tool"

BOOST_AUTO_TEST_CASE( EquipFiresForConnectionsBeforeAndAfterEquip )
{
	shared_ptr<Tool> tool = Creatable<Instance>::create<Tool>();
	shared_ptr<Mouse> mouse = Creatable<Instance>::create<Mouse>();
	CallbackCounter beforeEquip, afterEquip;
	beforeEquip.expectedMouse = mouse;
	afterEquip.expectedMouse = mouse;

	BOOST_CHECK_EQUAL(0, beforeEquip.count);
	BOOST_CHECK_EQUAL(0, afterEquip.count);

	boost::function<void(shared_ptr<Instance>)> beforeFun =
		boost::bind(&CallbackCounter::callback, &beforeEquip, _1);
	tool->equippedSignal.connect(beforeFun);
	tool->equippedSignal.equipped(mouse);

	BOOST_CHECK_EQUAL(1, beforeEquip.count);
	BOOST_CHECK_EQUAL(0, afterEquip.count);

	boost::function<void(shared_ptr<Instance>)> afterFun =
		boost::bind(&CallbackCounter::callback, &afterEquip, _1);
	tool->equippedSignal.connect(afterFun);

	BOOST_CHECK_EQUAL(1, beforeEquip.count);
	BOOST_CHECK_EQUAL(1, afterEquip.count);

	tool->equippedSignal.unequipped();

	BOOST_CHECK_EQUAL(1, beforeEquip.count);
	BOOST_CHECK_EQUAL(1, afterEquip.count);

	CallbackCounter afterUnequip;
	
	boost::function<void(shared_ptr<Instance>)> afterUnequipFun =
		boost::bind(&CallbackCounter::callback, &afterUnequip, _1);
	tool->equippedSignal.connect(afterUnequipFun);

	BOOST_CHECK_EQUAL(1, beforeEquip.count);
	BOOST_CHECK_EQUAL(1, afterEquip.count);
	BOOST_CHECK_EQUAL(0, afterUnequip.count);

	shared_ptr<Mouse> mouse2 = Creatable<Instance>::create<Mouse>();
	beforeEquip.expectedMouse = mouse2;
	afterEquip.expectedMouse = mouse2;
	afterUnequip.expectedMouse = mouse2;

	tool->equippedSignal.equipped(mouse2);

	BOOST_CHECK_EQUAL(2, beforeEquip.count);
	BOOST_CHECK_EQUAL(2, afterEquip.count);
	BOOST_CHECK_EQUAL(1, afterUnequip.count);
}

BOOST_AUTO_TEST_SUITE_END()

