#ifndef _DEBUG

#include <boost/test/unit_test.hpp>
#include "rbx/test/Performance.h"
#include "rbx/test/DataModelFixture.h"

#include "script/ScriptContext.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/modelinstance.h"
#include "V8DataModel/BasicPartInstance.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(MemoryTests)

BOOST_FIXTURE_TEST_CASE(EmptyDatamodel, RBX::Test::Memory)
{
	BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\tNo DataModel");

	DataModelFixture dm;
	BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\tEmpty DataModel");
}


BOOST_FIXTURE_TEST_CASE(Script, RBX::Test::Memory)
{
	BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\tNo DataModel");

	DataModelFixture dm;
	BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\tEmpty DataModel");

	{
		RBX::DataModel::LegacyLock lock(dm.dataModel, RBX::DataModelJob::Write);
		shared_ptr<Instance> model = RBX::Creatable<RBX::Instance>::create<RBX::ModelInstance>();
		//model->setParent(dm.dataModel->getWorkspace());
		BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\t" << 0 << " bricks");
		
		long start = GetBytes();

		int count = 0;
		BOOST_TEST_MESSAGE("Memory (MB)\tbricks\tB/brick");
		for (int i=1; i<=10; ++i)
		{
			for (int j=0; j<5000; ++j)
			{
				shared_ptr<PartInstance> part = RBX::Creatable<RBX::Instance>::create<RBX::PartInstance>();
				part->setCoordinateFrame(RBX::CoordinateFrame(G3D::Vector3(4*i, 4*j, 0)));
				part->setParent(model.get());
				count++;
			}
			BOOST_TEST_MESSAGE("Memory: " << GetBytes()/1000000 << "\t" << count << "\t" << (GetBytes()-start)/count);
		}
		
		model->setParent(NULL);
		model.reset();
		BOOST_TEST_MESSAGE("Memory: " << GetBytes() << "\tremoved bricks");
	}
}

BOOST_AUTO_TEST_SUITE_END()

#endif
