#pragma once

#define STRING_BY_ID(id) (getStringById(id))

#ifdef _WIN32
__declspec(noinline) const char* getStringById(int id);
#elif __APPLE__ || __ANDROID__
__attribute__((noinline))  const char* getStringById(int id);
#else
#error Unsupported Platform.
#endif

enum StringIDs {
	ArgStringID = 0,
	LuaStringStringId = 1, //"lua"
	CommandOutStringId = 2, //"> %s"
	StudioASHXFmt = 3, //fmt
	StudioASHX = 4, //ashx
	RunningScript = 5, //Running script %s
	ExecScriptNewThread = 6,//Execute script in new thread, name: %s, identity: %u
	FullScriptCode = 7, //Full script code:\n %s
	EnableToCreateSBThread = 8, //Unable to create trusted sandbox thread
	EnableToCreateNewThread = 9, //Unable to create new thread
	ScriptStr = 10, //Script
	Rocky = 11,//rocky
	HasGamePassLuaWarning = 12, //Game passes can only be queried by a Script running on a ROBLOX game server
	NoTeleportInStudio = 13, //Teleporting while using ROBLOX Studio is not permitted
	LoadingScreenScriptPath = 14, // The path to the script that creates the loading gui
};