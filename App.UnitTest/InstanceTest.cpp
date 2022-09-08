#include <boost/test/unit_test.hpp>

#include "v8tree/instance.h"
#include "v8dataModel/BasicPartInstance.h"
#include "v8datamodel/PartInstance.h"
#include "v8dataModel/Tool.h"

extern const char* const sInstanceTest;
class InstanceTest : public RBX::DescribedCreatable<InstanceTest, RBX::Instance, sInstanceTest>
{
};

const char* const sInstanceTest = "InstanceTest";
RBX_REGISTER_CLASS(InstanceTest);

BOOST_AUTO_TEST_SUITE(Instance)

	BOOST_AUTO_TEST_CASE(Clone)
	{
		int count = RBX::Diagnostics::Countable<RBX::Instance>::getCount();
		{
			boost::shared_ptr<RBX::Instance> i = RBX::Creatable<RBX::Instance>::create<InstanceTest>();
			i->clone(RBX::ScriptingCreator);
		}
		BOOST_CHECK_EQUAL(count, RBX::Diagnostics::Countable<RBX::Instance>::getCount());
	}

		BOOST_AUTO_TEST_CASE(PerfDynamicCast)
		{
			shared_ptr<RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
			RBX::Instance* inst = part.get();
			for (int i=0; i<1000000; ++i)
			{
				dynamic_cast<RBX::Tool*>(inst);
			}
		}

		BOOST_AUTO_TEST_CASE(PerfIsA)
		{
			shared_ptr<RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
			RBX::Instance* inst = part.get();
			for (int i=0; i<1000000; ++i)
			{
				inst->getDescriptor().isA(RBX::PartInstance::classDescriptor());
			}
		}

		BOOST_AUTO_TEST_CASE(PerfIsAFalse)
		{
			boost::shared_ptr<RBX::Instance> part = RBX::Creatable<RBX::Instance>::create<InstanceTest>();
			RBX::Instance* inst = part.get();
			for (int i=0; i<1000000; ++i)
			{
				inst->isA<RBX::PartInstance>();
			}
		}

		BOOST_AUTO_TEST_CASE(PerfIsATool)
		{
			shared_ptr<RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
			RBX::Instance* inst = part.get();
			for (int i=0; i<1000000; ++i)
			{
				inst->isA<RBX::Tool>();
			}
		}

	BOOST_AUTO_TEST_CASE(CastTo)
	{
		shared_ptr<RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
		shared_ptr<RBX::Instance> instance = RBX::Creatable<RBX::Instance>::create<InstanceTest>();

		BOOST_CHECK(RBX::Instance::fastDynamicCast<RBX::Instance>(part.get()));

		BOOST_CHECK(!RBX::Instance::fastDynamicCast<RBX::PartInstance>(instance.get()));

		RBX::Instance* nullInstance = NULL;
		BOOST_CHECK(!RBX::Instance::fastDynamicCast<RBX::PartInstance>(nullInstance));

		RBX::Instance* inst = part.get();
		BOOST_CHECK(inst->fastDynamicCast<RBX::Instance>());

		BOOST_CHECK(!instance->fastDynamicCast<RBX::PartInstance>());
	}

	BOOST_AUTO_TEST_CASE(CastToConst)
	{
		shared_ptr<const RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
		shared_ptr<const RBX::Instance> instance = RBX::Creatable<RBX::Instance>::create<InstanceTest>();

		BOOST_CHECK(RBX::Instance::fastDynamicCast<RBX::Instance>(part.get()));

		BOOST_CHECK(!RBX::Instance::fastDynamicCast<RBX::PartInstance>(instance.get()));

		const RBX::Instance* nullInstance = NULL;
		BOOST_CHECK(!RBX::Instance::fastDynamicCast<RBX::PartInstance>(nullInstance));

		const RBX::Instance* inst = part.get();
		BOOST_CHECK(inst->fastDynamicCast<RBX::Instance>());

		BOOST_CHECK(!instance->fastDynamicCast<RBX::PartInstance>());
	}

	BOOST_AUTO_TEST_CASE(SharedCastTo)
	{
		shared_ptr<const RBX::PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
		shared_ptr<const RBX::Instance> instance = RBX::Creatable<RBX::Instance>::create<InstanceTest>();

		BOOST_CHECK(RBX::Instance::fastSharedDynamicCast<const RBX::Instance>(part));

		BOOST_CHECK(!RBX::Instance::fastSharedDynamicCast<const RBX::PartInstance>(instance));

		shared_ptr<const RBX::PartInstance> nullInstance;
		BOOST_CHECK(!RBX::Instance::fastSharedDynamicCast<const RBX::PartInstance>(nullInstance));
	}

BOOST_AUTO_TEST_SUITE_END()

