#pragma once

// Helpful utility classes you can use to do DataModel-driven tests

#include "v8datamodel/datamodel.h"
#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/CommonVerbs.h"
#include "script/LuaSettings.h"
#include "Util/Http.h"
#include "Util/Profiling.h"
#include "Util/StandardOut.h"
#include "script/LuaSettings.h"
#include "script/ScriptContext.h"
#include "Network/API.h"
#include "Network/Players.h"
#include "rbx/test/test_tools.h"
#include "gui/ProfanityFilter.h"

// Use this fixture to create a DataModel
class DataModelFixture
{
	static void init()
	{
		static boost::shared_ptr<RBX::ProfanityFilter> s_profanityFilter;
		s_profanityFilter = RBX::ProfanityFilter::getInstance();

		RBX::Profiling::init(false);

		static RBX::FactoryRegistrator registerFactoryObjects;

		// This is needed before you can instantiate a DataModel
		RBX::Http::init(RBX::Http::WinHttp, RBX::Http::CookieSharingSingleProcessMultipleThreads);

		RBX::GameSettings::singleton();
		RBX::LuaSettings::singleton();
		RBX::DebugSettings::singleton();
		RBX::PhysicsSettings::singleton();
	}
	boost::scoped_ptr<RBX::CommonVerbs> commonVerbs;
public:
	boost::shared_ptr<RBX::DataModel> dataModel;
	RBX::DataModel* operator&() { return dataModel.get(); }
	RBX::DataModel* operator->() { return dataModel.get(); }

	std::auto_ptr<RBX::Reflection::Tuple> execute(const char* script, const RBX::Reflection::Tuple& args, RBX::Security::Identities identity = RBX::Security::CmdLine_);
	std::auto_ptr<RBX::Reflection::Tuple> execute(const char* script)
	{
		RBX::Reflection::Tuple args;
		return execute(script, args);
	}

	void run()
	{
		RBX::RunService* runService = RBX::ServiceProvider::create<RBX::RunService>(dataModel.get());
		BOOST_REQUIRE(runService);
		runService->run();
	}

	DataModelFixture()
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(&init, flag);
		dataModel = RBX::DataModel::createDataModel(true, new RBX::NullVerb(NULL,""), false);

		{
			RBX::DataModel::LegacyLock lock(dataModel.get(), RBX::DataModelJob::Write);
			commonVerbs.reset(new RBX::CommonVerbs(dataModel.get()));
		}
	}
	~DataModelFixture()
	{
		commonVerbs.reset();
		RBX::DataModel::closeDataModel(dataModel);
	}
};


class NetworkFixture
{
public:
	NetworkFixture()
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(&RBX::Network::initWithoutSecurity, flag);
	}

	static void startServer(DataModelFixture& dm)
	{
		const char* serverScript = "local ns = game:GetService('NetworkServer')\n"\
								   "ns:Start(53640, 0)\n";

		RBX_REQUIRE_NO_EXECEPTION(dm.execute(serverScript));
	}

	static void startClient(DataModelFixture& dm, bool waitForClientToReceiveCharacter=false);

private:
	static shared_ptr<RBX::Reflection::Tuple> accepted(RBX::CEvent* event)
	{
		//BOOST_TEST_MESSAGE("connection accepted");
		event->Set();
		return shared_ptr<RBX::Reflection::Tuple>();
	};

	static shared_ptr<RBX::Reflection::Tuple> failed(shared_ptr<const RBX::Reflection::Tuple> args, RBX::CEvent* event)
	{
		std::string reason = args->values[1].get<std::string>();
		BOOST_TEST_MESSAGE(reason);
		event->Set();
		return shared_ptr<RBX::Reflection::Tuple>();
	};

	static shared_ptr<RBX::Reflection::Tuple> rejected(shared_ptr<const RBX::Reflection::Tuple> args, RBX::CEvent* event)
	{
		BOOST_TEST_MESSAGE("connection rejected");
		event->Set();
		return shared_ptr<RBX::Reflection::Tuple>();
	};
};


// Use this fixture for debugging
class OutputLoggingFixture
{
	rbx::signals::scoped_connection messageConnection;
	static void print(const RBX::StandardOutMessage& message)
	{
		switch (message.type)
		{
		case RBX::MESSAGE_INFO:
			std::cout << "RBX INFO: " << message.message << '\n';
			break;
		case RBX::MESSAGE_WARNING:
			std::cout << "RBX WARNING: " << message.message << '\n';
			break;
		case RBX::MESSAGE_ERROR:
			std::cout << "RBX ERROR: " << message.message << '\n';
			break;
		default:
			std::cout << "RBX: " << message.message << '\n';
			break;
		}
	}
public:
	OutputLoggingFixture()
	{
		messageConnection = RBX::StandardOut::singleton()->messageOut.connect(&OutputLoggingFixture::print);
	}
};
