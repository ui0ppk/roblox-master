//  PlaceLauncher.h
#pragma once

#include <string>

#include <boost/iostreams/copy.hpp>

#include "LogManager.h"
#include "Teleporter.h"

#include "v8datamodel/Game.h"
#include "v8tree/Instance.h"
#include "util/standardout.h"
#include "rbx/signal.h"

enum JoinGameRequest {
    JOIN_GAME_REQUEST_PLACEID,
    JOIN_GAME_REQUEST_USERID,
    JOIN_GAME_REQUEST_PRIVATE_SERVER,
    JOIN_GAME_REQUEST_GAME_INSTANCE
};

class Teleporter;
class RobloxView;

struct StartGameParams
{
	int viewWidth;
	int viewHeight;
	void* view;
	int placeId;
	int userId;
	std::string accessCode;
	std::string gameId;
	std::string assetFolderPath;
	bool isTouchDevice;
	JoinGameRequest joinRequestType;
};

class PlaceLauncher
{
    RobloxView* rbxView;
    boost::scoped_ptr<Teleporter> teleporter;
    
    bool isCurrentlyPlayingGame;
    bool isLeavingGame;
    int lastPlaceId;
    StartGameParams gameParams;

    // player join tracking
    rbx::signals::connection childConnection;
    rbx::signals::connection playerConnection;

    shared_ptr<RBX::Game> currentGame;


    PlaceLauncher();

    PlaceLauncher(PlaceLauncher const&);
    void operator=(PlaceLauncher const&);


    shared_ptr<RBX::Game> setupGame( const StartGameParams& sgp );
    shared_ptr<RBX::Game> setupPreloadedGame( const StartGameParams& sgp );
    bool startGame( boost::function0 <void> scriptFunction, shared_ptr<RBX::Game> preloadedGame);
    bool prepareGame(const StartGameParams& sgp);
    void deleteRobloxView( bool resetCurrentGame );
    void finishGameSetup( boost::shared_ptr<RBX::Game> game);

public:
    static PlaceLauncher& getPlaceLauncher();

    RobloxView* getRbxView() { return rbxView; }

    bool startGame( const StartGameParams& sgp );
    void leaveGame ( bool userRequestedLeave);
    void teleport( std::string ticket, std::string authUrl, std::string script);

    static void handleStartGameFailure(int status);

    weak_ptr<RBX::Game> getCurrentGame() { return currentGame; }

	void shutDownGraphics();
	void startUpGraphics(void *wnd, unsigned int width, unsigned int height);
};
