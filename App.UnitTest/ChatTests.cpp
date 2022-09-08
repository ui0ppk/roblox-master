#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>

#include "rbx/test/ScopedFastFlagSetting.h"
#include "rbx/test/DataModelFixture.h"
#include "network/Player.h"
#include "network/Players.h"
#include "Server.h"

#include "Network/ChatFilter.h"
#include "network/WebChatFilter.h"
#include "util/Http.h"
#include "util/Statistics.h"

using namespace RBX;
using namespace Network;

BOOST_AUTO_TEST_SUITE(ChatTests)

namespace
{
void filterMessageHelper(
	const std::string message,
	const RBX::Network::ChatFilter::FilteredChatMessageCallback filteredCallback)
{
	ChatFilter::Result result;
	if ("Bullshit" == message)
	{
		result.blacklistFilteredMessage = "Bull####";
		result.whitelistFilteredMessage = "########";
	}
	else if ("Hello world" == message)
	{
		result.blacklistFilteredMessage = "Hello world";
		result.whitelistFilteredMessage = "Hello world";
	}
	else
	{
		throw std::runtime_error("invalid message passed");
	}
	filteredCallback(result);
}
} // namespace

static void onFilteredChatted(const ChatMessage& message, const std::string& expectedFilteredMessage, const char* source, const char* destination, shared_ptr<RBX::CEvent> event)
{
	if (expectedFilteredMessage.empty())
		BOOST_ERROR("Not expecting chat message");

	BOOST_CHECK_EQUAL(message.source ? message.source->getName() : "null", source);
	BOOST_CHECK_EQUAL(message.destination ? message.destination->getName() : "null", destination);
	BOOST_CHECK_EQUAL(message.message, expectedFilteredMessage);
	
	if (event)
		event->Set();
};

class MockWebChatFilter : public WebChatFilter
{
public:
	/*override*/
	virtual void filterMessage(
		shared_ptr<RBX::Network::Player> sender,
		shared_ptr<RBX::Instance> receiver,
		const std::string& message,
		const RBX::Network::ChatFilter::FilteredChatMessageCallback filteredCallback)
	{
		RBXASSERT(sender);
		boost::function0<void> f = boost::bind(&filterMessageHelper,  message, filteredCallback);
		boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, RBX::MESSAGE_ERROR, false);
		boost::thread(thread_wrapper(g, "rbx_webChatFilterHttpPost"));
	}
};

class ChatTester
{
protected:
	DataModelFixture serverDm;
	DataModelFixture client1Dm;
	DataModelFixture client2Dm;
public:
	ChatTester()
	{
		ScopedFastFlagSetting debugForceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);

		NetworkFixture::startServer(serverDm);
        NetworkFixture::startClient(client1Dm);

        {
            RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

            RBX::Reflection::Tuple args;
            //serverDm.execute("game:GetService('ContentProvider'):SetBaseUrl('http://www.gametest1.robloxlabs.com')", args);
            serverDm.execute("game.Players:SetChatFilterUrl('http://www.gametest1.robloxlabs.com/Game/ChatFilter.ashx')", args, RBX::Security::WebService);
            ::SetBaseURL("http://www.gametest1.robloxlabs.com/");
            serverDm->create<RBX::Network::WebChatFilter>();
        }

		while (true)
		{
			// Wait for players to replicate
			G3D::System::sleep(0.2);

			// Confirm the server has the Player from the client
			{
				RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
				Players* players = serverDm->create<Network::Players>();
				if (players->numChildren() == 1)
				{
					Player* player = dynamic_cast<Player*>(players->getChild(0));
					Security::Impersonator impersonate(Security::WebService);
					player->setNeutral(false);
					player->setName("Player1");
					player->setUserId(1);
					break;
				}
			}
		}

		NetworkFixture::startClient(client2Dm);

		while (true)
		{
			// Wait for players to replicate
			G3D::System::sleep(0.2);

			// Confirm the server has the Player from the client
			{
				RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
				Players* players = serverDm->create<Network::Players>();
				if (players->numChildren() == 2)
				{
					Player* player = dynamic_cast<Player*>(players->getChild(1));
					Security::Impersonator impersonate(Security::WebService);
					player->setNeutral(false);
					player->setName("Player2");
					player->setUserId(2);
					break;
				}
			}
		}

		while (true)
		{
			// Wait for players to replicate
			G3D::System::sleep(0.2);

			{
				RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
				Players* players = client2Dm->create<Network::Players>();
				if (!players->findFirstChildByName("Player1"))
					continue;
				if (!players->findFirstChildByName("Player2"))
					continue;
				break;
			}
		}
	}
};

class PublicChatTester : public ChatTester
{
public:
	void test()
	{
		shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Hello world", "Player1", "null", event));
		}

		// Send some chat
		client1Dm.execute("game.Players:Chat('Hello world')");

		// Wait for the message
        BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));

        {
            connection.disconnect();
            RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
            connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "########", "Player1", "null", event));
        }

        // Send some chat
        client1Dm.execute("game.Players:Chat('Bullshit')");

        // Wait for message
        BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));
	}
};

#ifdef _WIN32
class PublicCustomFilteredChatTester : public ChatTester
{
	void loadChatInfo(shared_ptr<RBX::Instance> instance)
	{
		instance->fastDynamicCast<Network::Player>()->setChatInfo(Player::CHAT_FILTER_BLACKLIST);
	}
public:
	void test()
	{
		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

            RBX::Reflection::Tuple args;
			serverDm.execute("game:GetService('ContentProvider'):SetBaseUrl('http://www.roblox.com')", args);

			serverDm->create<Network::Players>()->visitChildren(boost::bind(&PublicCustomFilteredChatTester::loadChatInfo, this, _1));
		}

		shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Bull####", "Player1", "null", event));
		}

		// Send some profanity
		client1Dm.execute("game.Players:Chat('Bullshit')");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	

		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection.disconnect();
			connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Hello world", "Player1", "null", event));
		}

		client1Dm.execute("game.Players:Chat('Hello world')");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	
	}
};
#endif // _WIN32

class PublicChatTester2 : public ChatTester
{
public:
	void test()
	{
		shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			connection = serverDm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Hello world", "Player1", "null", event));
		}

		// Send some chat
		client1Dm.execute("game.Players:Chat('Hello world')");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	
	}
};

class TeamChatTester : public ChatTester
{
public:
	void test()
	{
		shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Hello world", "Player1", "null", event));
		}

		// Send some chat
		client1Dm.execute("game.Players:TeamChat('Hello world')");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	
	}
};

class WhisperChatTester : public ChatTester
{
public:
	void test()
	{
		shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection = client2Dm->create<Network::Players>()->chatMessageSignal.connect(boost::bind(&onFilteredChatted, _1, "Hello world", "Player1", "Player2", event));
		}

		// Send some chat
		client1Dm.execute("game.Players:WhisperChat('Hello world', game.Players.Player2)");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	
	}
};

BOOST_AUTO_TEST_CASE(Chat)
{
	PublicChatTester tester;
	tester.test();
}

#ifdef _WIN32
BOOST_AUTO_TEST_CASE(WebChatFilterChat)
{
	PublicCustomFilteredChatTester tester;
	tester.test();
}
#endif

BOOST_AUTO_TEST_CASE(Chat2)
{
	PublicChatTester2 tester;
	tester.test();
}

#if 1
BOOST_AUTO_TEST_CASE(TeamChat)
{
	TeamChatTester tester;
	tester.test();
}
#endif

BOOST_AUTO_TEST_CASE(WhisperChat)
{
	WhisperChatTester tester;
	tester.test();
}

static void onReportAbuse(AbuseReport r, shared_ptr<RBX::CEvent> event)
{
	BOOST_CHECK_EQUAL(r.submitterID, 10);
	BOOST_CHECK_EQUAL(r.allegedAbuserID, 10);
	BOOST_CHECK_EQUAL(r.comment, "AbuserID:10;Reason;Hello world");
	BOOST_CHECK_EQUAL(r.placeID, 1818);
	BOOST_CHECK_EQUAL(r.gameJobID, "11");
	event->Set();
};

BOOST_AUTO_TEST_CASE(ReportAbuse)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	shared_ptr<RBX::CEvent> event(new RBX::CEvent(false));

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		serverDm->setPlaceID(1818, false);
		serverDm->jobId = "11";
	}

	while (true)
	{
		// Wait for players to replicate
		G3D::System::sleep(0.2);

		// Confirm the server has the Player from the client
		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			RBX::Security::Impersonator impersonate(RBX::Security::WebService);
			Network::Players* players = serverDm->create<Network::Players>();
			if (players->numChildren() == 1)
			{
				Player* player = boost::polymorphic_downcast<Player*>(players->getChild(0));
				player->setUserId(10);
				break;
			}
		}
	}

	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		RBX::Security::Impersonator impersonate(RBX::Security::WebService);
		Player* player = boost::polymorphic_downcast<Player*>(clientDm->create<Network::Players>()->getChild(0));
		player->setUserId(10);
	}

	// Test ReportAbuse
	{
		// prepare to read the signal
		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			connection = serverDm->create<Network::Players>()->abuseReportedReceived.connect(boost::bind(&onReportAbuse, _1, event));
		}

		// Send some chat
		clientDm.execute("game.Players:ReportAbuse(game.Players:GetChildren()[1], 'Reason', 'Hello world')");

		// Wait for the message
		BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(10)));	
	}
}


BOOST_AUTO_TEST_SUITE_END()

