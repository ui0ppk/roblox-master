/*
 *  RobloxLauncher.h
 *  NPRobloxMac
 *
 *  Created by Aaron Wilkowski on 12/22/11.
 *  Copyright 2011 Roblox. All rights reserved.
 *
 */


class IRobloxLauncher {
public:
	virtual ~IRobloxLauncher() {}
	
	virtual int HelloWorld() = 0;
	virtual int Hello(char *retMessage) = 0;
	virtual int StartGame(char *cbufAuth, char *cbufScript) = 0;
	virtual int PreStartGame() = 0;
	virtual int GetInstallHost(char *cbufHost) = 0;
	virtual int Update() = 0;
	virtual int IsUpToDate(bool *isUpToDate) = 0;
	virtual int Put_AuthenticationTicket(char *cbufTicket) = 0;
	virtual int IsGameStarted(bool *isStarted) = 0;
	virtual int SetSilentModeEnabled(bool silentMode) = 0;
	virtual int UnhideApp() = 0;
	virtual int BringAppToFront() = 0;
	virtual int ResetLaunchState() = 0;
	virtual int SetStartInHiddenMode(bool hiddenMode) = 0;
	virtual int GetKeyValue(char *key, char *value) = 0;
	virtual int SetKeyValue(char *key, char *value) = 0;
	virtual int DeleteKeyValue(char *key) = 0;
	
};

/*
typedef struct {
	NPP         instance;
	NPObject  * so;
	IRobloxLauncher * launcher;
} NP_ROBLOX_INSTANCE;
*/
