#include <boost/test/unit_test.hpp>

#include "rbx/test/ScopedFastFlagSetting.h"
#include "rbx/test/DataModelFixture.h"

#include "v8datamodel/Test.h"
#include "v8datamodel/FastLogSettings.h"
#include "v8datamodel/Value.h"

#include "v8xml/SerializerBinary.h"
#include "v8xml/Serializer.h"
#include "v8xml/XmlSerializer.h"

#include "script/DebuggerManager.h"
#include "script/Script.h"
#include "script/ScriptContext.h"

FASTFLAG(LuaDebugger)

struct DebuggerData
{
	int breakpoints[3];
	std::string breakpointConditions[3];
	std::string watch;
	int noBreakpointLine;
};

#define DEFAULT_SCRIPT_CODE "print 'Script Start'\n\
function test1()\n\
	local num = 1\n\
	num = num + 10\n\
	print'test1 called'\n\
end\n\
test1()\n\
print 'Script End'"

static void initFlatDataHierarchy(RBX::Instance* parent, int numScripts)
{
	for (int ii = 0; ii < numScripts; ++ii)
	{
		//create script
		shared_ptr<RBX::Script> scriptInstance = RBX::Creatable<RBX::Instance>::create<RBX::Script>();
		scriptInstance->setEmbeddedCode(RBX::ProtectedString::fromTestSource(DEFAULT_SCRIPT_CODE));

		std::ostringstream s;
		s << "Script-" << ii;
		scriptInstance->setName(s.str());
		scriptInstance->setParent(parent);

		//add dummy instance of some other type
		shared_ptr<RBX::IntValue> intvalue = RBX::Creatable<RBX::Instance>::create<RBX::IntValue>();
		intvalue->setParent(parent);
	}
}

static void initNestedDataHierarchy(RBX::Instance* parent, int numScripts)
{
	for (int ii = 0; ii < numScripts; ++ii)
	{
		//create script
		shared_ptr<RBX::Script> scriptInstance = RBX::Creatable<RBX::Instance>::create<RBX::Script>();
		scriptInstance->setEmbeddedCode(RBX::ProtectedString::fromTestSource(DEFAULT_SCRIPT_CODE));

		std::ostringstream s;
		s << "Script-" << ii;
		scriptInstance->setName(s.str());
		scriptInstance->setParent(parent);

		//add dummy instance of some other type
		shared_ptr<RBX::IntValue> intvalue = RBX::Creatable<RBX::Instance>::create<RBX::IntValue>();
		intvalue->setParent(parent);

		initNestedDataHierarchy(scriptInstance.get(), ii);
	}
}

static void addDebuggerData(shared_ptr<RBX::Instance> instance, shared_ptr<RBX::Scripting::DebuggerManager> debuggerManager, const DebuggerData &debuggerData)
{
	shared_ptr<RBX::Script> script = RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance);
	if (script)
	{
		//there shouldn't be any debugger present for this script
		RBX::Scripting::ScriptDebugger* debugger = debuggerManager->findDebugger(script.get());
		BOOST_CHECK(debugger == NULL);

		//now add debugger
		debugger = debuggerManager->addDebugger(script.get()).get();
		BOOST_CHECK(debugger != NULL);

		//add breapoints, breakpoint conditions and watch
		for (int ii = 0; ii < 3; ++ii)
		{
			shared_ptr<RBX::Scripting::DebuggerBreakpoint> bp = debugger->setBreakpoint(debuggerData.breakpoints[ii]);
			BOOST_CHECK(bp != NULL);

			if (!debuggerData.breakpointConditions[ii].empty())
				RBX::Scripting::DebuggerBreakpoint::prop_Condition.setValue(bp.get(), debuggerData.breakpointConditions[ii]);			
		}

		if (!debuggerData.watch.empty())
			debugger->addWatch(debuggerData.watch);
	}
}

static void verifyDebuggerData(shared_ptr<RBX::Instance> instance, shared_ptr<RBX::Scripting::DebuggerManager> debuggerManager, const DebuggerData &debuggerData)
{
	shared_ptr<RBX::Script> script = RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance);
	if (script)
	{
		//verify added debugger
		RBX::Scripting::ScriptDebugger* debugger = debuggerManager->findDebugger(script.get());
		BOOST_CHECK(debugger != NULL);
		BOOST_CHECK(debugger == debuggerManager->addDebugger(script.get()).get());

		RBX::Scripting::DebuggerBreakpoint* bp;
		for (int ii = 0; ii < 3; ++ii)
		{
			bp = debugger->findBreakpoint(debuggerData.breakpoints[ii]);
			BOOST_CHECK(bp != NULL);
			BOOST_CHECK(bp->getCondition() == debuggerData.breakpointConditions[ii]);
		}	

		//there shouldn't be any breakpoint on line specified
		bp = debugger->findBreakpoint(debuggerData.noBreakpointLine);
		BOOST_CHECK(bp == NULL);

		//verify added watch
		if (!debuggerData.watch.empty())
		{
			const RBX::Scripting::ScriptDebugger::Watches& watches = debugger->getWatches();
			BOOST_CHECK(watches.size() == 1);
			BOOST_CHECK(watches[0]->getExpression() == debuggerData.watch);
		}
	}
}

BOOST_AUTO_TEST_SUITE(LuaDebugger)

BOOST_AUTO_TEST_CASE(FlatDataSerialization)
{
	ScopedFastFlagSetting luadebugger("LuaDebugger", true);
		
	//default debugger data
	DebuggerData debuggerData;
	debuggerData.breakpoints[0] = 1; debuggerData.breakpoints[1] = 3; debuggerData.breakpoints[2] = 5; 
	debuggerData.breakpointConditions[2] = "num > 10";
	debuggerData.watch = "num";
	debuggerData.noBreakpointLine = 7;

	RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
	
	std::stringstream binaryDataStreamOriginal(std::ios_base::out | std::ios_base::in | std::ios_base::binary);

	{		
		DataModelFixture dm;
		RBX::Workspace* workspace = dm.dataModel->getWorkspace();
		BOOST_CHECK(workspace != NULL);

		//set datamodel in debugger manager
		debuggerManager.setDataModel(dm.dataModel.get());
		BOOST_CHECK(debuggerManager.getDataModel() != NULL);

		RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);		
		
		//initialize default data and add debugger data
		workspace->replenishCamera();
		initFlatDataHierarchy(workspace, 5);
		workspace->visitChildren(boost::bind(&addDebuggerData, _1, shared_from(&debuggerManager), debuggerData));

		//verify debugger data
		workspace->visitChildren(boost::bind(&verifyDebuggerData, _1, shared_from(&debuggerManager), debuggerData));
		
		//store binary representation of debugger manager		
		RBX::SerializerBinary::serialize(binaryDataStreamOriginal, &debuggerManager);
		BOOST_CHECK(binaryDataStreamOriginal.str().empty() != true);

		//clean up debugger manager
		debuggerManager.setDataModel(NULL);
	}
	
	DataModelFixture dm;
	RBX::Workspace* workspace = dm.dataModel->getWorkspace();
	BOOST_CHECK(workspace != NULL);

	//set datamodel in debugger manager
	debuggerManager.setDataModel(dm.dataModel.get());
	BOOST_CHECK(debuggerManager.getDataModel() != NULL);

	RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);	

	//initialize default data and restore debugger data from the stored binary representation
	workspace->replenishCamera();
	initFlatDataHierarchy(workspace, 5);
	RBX::SerializerBinary::deserialize(binaryDataStreamOriginal, &debuggerManager);
	
	//verify previous and current binary representation
	std::stringstream binaryDataStreamNew(std::ios_base::out | std::ios_base::in | std::ios_base::binary);
	RBX::SerializerBinary::serialize(binaryDataStreamNew, &debuggerManager);
	BOOST_CHECK(binaryDataStreamNew.str().empty() != true);
	
	//check if we have the same binary data
	BOOST_CHECK_EQUAL(binaryDataStreamOriginal.str(), binaryDataStreamNew.str());

	//verify debugger data (created from binary representation)
	workspace->visitChildren(boost::bind(&verifyDebuggerData, _1, shared_from(&debuggerManager), debuggerData));

	//clean up debugger manager
	debuggerManager.setDataModel(NULL);
}

BOOST_AUTO_TEST_CASE(NestedDataSerialization)
{
	ScopedFastFlagSetting luadebugger("LuaDebugger", true);

	//default debugger data
	DebuggerData debuggerData;
	debuggerData.breakpoints[0] = 1; debuggerData.breakpoints[1] = 3; debuggerData.breakpoints[2] = 5; 
	debuggerData.breakpointConditions[2] = "num > 10";
	debuggerData.watch = "num";
	debuggerData.noBreakpointLine = 7;

	RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
	std::stringstream binaryDataStreamOriginal(std::ios_base::out | std::ios_base::in | std::ios_base::binary);

	{		
		DataModelFixture dm;
		RBX::Workspace* workspace = dm.dataModel->getWorkspace();
		BOOST_CHECK(workspace != NULL);

		//set datamodel in debugger manager
		debuggerManager.setDataModel(dm.dataModel.get());
		BOOST_CHECK(debuggerManager.getDataModel() != NULL);

		RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);
		
		//initialize default data and add debugger data
		workspace->replenishCamera();
		initNestedDataHierarchy(workspace, 5);
		workspace->visitDescendants(boost::bind(&addDebuggerData, _1, shared_from(&debuggerManager), debuggerData));

		//verify debugger data
		workspace->visitDescendants(boost::bind(&verifyDebuggerData, _1, shared_from(&debuggerManager), debuggerData));
		
		//store binary representation of debugger manager
		RBX::SerializerBinary::serialize(binaryDataStreamOriginal, &debuggerManager);
		BOOST_CHECK(binaryDataStreamOriginal.str().empty() != true);

		//clean up debugger manager
		debuggerManager.setDataModel(NULL);
	}

	DataModelFixture dm;
	RBX::Workspace* workspace = dm.dataModel->getWorkspace();
	BOOST_CHECK(workspace != NULL);

	//set datamodel in debugger manager
	debuggerManager.setDataModel(dm.dataModel.get());
	BOOST_CHECK(debuggerManager.getDataModel() != NULL);

	RBX::DataModel::LegacyLock lock(&dm, RBX::DataModelJob::Write);

	//initialize default data and restore debugger data from the stored binary representation
	workspace->replenishCamera();
	initNestedDataHierarchy(workspace, 5);
	RBX::SerializerBinary::deserialize(binaryDataStreamOriginal, &debuggerManager);
	
	//verify previous and current binary representation
	std::stringstream binaryDataStreamNew(std::ios_base::out | std::ios_base::in | std::ios_base::binary);
	RBX::SerializerBinary::serialize(binaryDataStreamNew, &debuggerManager);
	BOOST_CHECK(binaryDataStreamNew.str().empty() != true);
	
	//check if we have the same binary data
	BOOST_CHECK_EQUAL(binaryDataStreamOriginal.str(), binaryDataStreamNew.str());

	//verify debugger data (created from binary representation)
	workspace->visitDescendants(boost::bind(&verifyDebuggerData, _1, shared_from(&debuggerManager), debuggerData));

	//clean up debugger manager
	debuggerManager.setDataModel(NULL);
}

BOOST_AUTO_TEST_SUITE_END()
