/*
 *  Roblox.h
 *  MacClient
 *
 *  Created by Tony on 1/26/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "Util/StandardOut.h"
#include <v8datamodel/TeleportService.h>
#include <semaphore.h>
#include <v8datamodel/FastLogSettings.h>

extern void callTerminateApp();

namespace RBX
{
	class Game;
}

class Roblox
{
public :
	
	enum RunMode {
		RUN_CLIENT,
		RUN_SERVER,
		RUN_FILE,
		RUN_DEVELOPER,
	};
	
private :
	static boost::scoped_ptr<boost::thread> singleRunningInstance;
	static sem_t *uniqSemaphore;
	static bool needTerminateCall;

	static shared_ptr<RBX::TeleportCallback> callback;
	static void *theInstance;
	static bool initialized;
	static std::string assetFolder;
	static RunMode runMode;
	static RBX::ClientAppSettings appSettings;
	
	static void onMessageOut(const RBX::StandardOutMessage& message);
	static void globalShutdown();

	static void terminateWaiter();
	static bool isOtherRunning();
	static void terminateOther();
public :
    
    static bool globalInit(bool isApp);
	static bool initInstance(void *instance, bool isApp);
	static void shutdownInstance();
	static void releaseTerminateWaiter();
	
	static void setArgs(const char *gameFolder, const char *runMode);
	
	static void sendAppEvent(void *pClosure);
	static void postAppEvent(void *pClosure);
	static void processAppEvents();

	static void addLogToBreakpad(const char* log);
	static void addBreakPadKeyValue(const char* key, int value);
	
	
	// Specific handlers for verbs events etc.
	static void handleLeaveGame(void *appWindow);
	static void handleShutdownClient(void *appWindow);
	static void handleToggleFullScreen(void *appWindow);
	static bool inFullScreenMode(void *appWindow);
	
	static RunMode getRunMode() { return runMode; }
	
	static void preloadGame(bool isApp);
	
	static boost::shared_ptr<RBX::Game> getpreloadedGame(const bool isApp);
	static void relinquishGame(boost::shared_ptr<RBX::Game>& game);

	static void testCrash();	
	
};
