#include "rbx/test/DataModelFixture.h"

#include "Client.h"
#include "ClientReplicator.h"
#include "Marker.h"

void NetworkFixture::startClient(DataModelFixture& dm, bool waitForClientToReceiveCharacter)
{
	RBX::CEvent event(true);

	{
		RBX::Reflection::Tuple args(3);
		args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&accepted, &event)));
		args.values[1] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&failed, _1, &event)));
		args.values[2] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&rejected, _1, &event)));

		const char* clientScript = "accepted, failed, rejected = ...\n"\
								   "local nc = game:GetService('NetworkClient')\n"\
								   "nc.ConnectionAccepted:connect(accepted)\n"\
								   "nc.ConnectionFailed:connect(failed)\n"\
								   "nc.ConnectionRejected:connect(rejected)\n"\
								   "nc:PlayerConnect(0, 'localhost', 53640, 0, 0)\n";

		RBX_REQUIRE_NO_EXECEPTION(dm.execute(clientScript, args));
	}
	// Raknet default timeout time is 10 or 30 seconds
	BOOST_CHECK(event.Wait(RBX::Time::Interval::from_seconds(60)));

	if (waitForClientToReceiveCharacter)
	{
		RBX::CEvent characterLoadedEvent(true);
		//boost::shared_ptr<RBX::Network::Marker> marker;
		{
			RBX::DataModel::LegacyLock l(&dm, RBX::DataModelJob::Write);

			RBX::Network::Client* client = RBX::ServiceProvider::find<RBX::Network::Client>(&dm);
			RBXASSERT(client);
			RBX::Network::ClientReplicator* replicator = client->findFirstChildOfType<RBX::Network::ClientReplicator>();
			RBXASSERT(replicator);
			RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&dm);
			RBXASSERT(players);
			RBX::Network::Player* player = players->getLocalPlayer();
			RBXASSERT(player);
            
            RBX::Reflection::Tuple args(1);
            args.values[0] = shared_ptr<RBX::Lua::GenericFunction>(new RBX::Lua::GenericFunction(boost::bind(&accepted, &characterLoadedEvent)));
            
			// hack: doing this in script, because sometimes SendMarker returns bad memory in c++ (only observed here) 
            const char* requestCharacterScript ="success = ...\n"\
                                                "local nc = game:GetService('NetworkClient')\n"\
                                                "local replicator = nc:GetChildren()[1]\n"\
                                                "local marker = replicator:SendMarker()\n"\
                                                "marker.Received:connect(function() replicator:RequestCharacter() end)\n"\
                                                "local waitTime = 0\n"\
                                                "while waitTime < 60 do\n"\
                                                "   local t = wait(1)\n"\
                                                "   waitTime = waitTime + t\n"\
                                                "   if game.Players.LocalPlayer.Character ~= nil then success() break end\n"\
                                                "end\n";
            RBX_REQUIRE_NO_EXECEPTION(dm.execute(requestCharacterScript, args));
            
		}
		BOOST_REQUIRE(characterLoadedEvent.Wait(RBX::Time::Interval::from_seconds(60)));
	}
}
