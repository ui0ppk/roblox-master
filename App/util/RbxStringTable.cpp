#include "util/RbxStringTable.h"

static const char *stringsTable[] = 
	{
		"arg", //ArgStringID
		"Lua",//LuaStringStringId
		"> %s",//CommandOutStringId
		"loadfile('%s/game/studio.ashx')()",//StudioASHXFmt
		"Studio.ashx",//StudioASHX
		"Running script %s",//RunningScript
		"Execute script in new thread, name: %s, identity: %u", //ExecScriptNewThread
		"Full script code:\n %s", //FullScriptCode
		"Unable to create trusted sandbox thread", //EnableToCreateSBThread
		"Unable to create new thread", //EnableToCreateNewThread
		"Script", //ScriptStr
		"rocky", //Rocky
		"Game passes can only be queried by a Script running on a ROBLOX game server", //HasGamePassLuaWarning
		"Teleporting while using ROBLOX Studio is not permitted.", //NoTeleportInStudio
		"fonts/LoadingScript.lua" //LoadingScreenScriptPath
    };

const char* getStringById(int id)
{
	volatile int realId = id+1;
	realId -= 1;
	return stringsTable[realId];
}
