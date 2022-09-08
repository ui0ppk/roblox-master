/**
 * SharedLauncher.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
    #include "atlutil.h"
    #include "ProcessInformation.h"
#endif

#include "format_string.h"

#define LAUNCHER_STARTED_EVENT_NAME _T("rbxLauncherStarted")

namespace SharedLauncher
{
	static const char* FileLocationArgument = "-fileLocation";      // File to open
	static const char* ScriptArgument       = "-script";		    // Script to execute (LUA)
	static const char* AuthUrlArgument      = "-url";			    // Url to hit to authenticate (passing auth from website)
	static const char* AuthTicketArgument   = "-ticket";		    // Ticket to tack onto url to authenticate (passing auth from website)
	static const char* StartEventArgument   = "-startEvent";	    // Callback to notify launcher of start of application
    static const char* ReadyEventArgument   = "-readyEvent";	    // Callback to notify launcher that application is loaded and ready
    static const char* ShowEventArgument    = "-showEvent";	        // Event to wait for before showing the window
	static const char* TestModeArgument     = "-testMode";          // Play solo, Start Server, and Start Player
	static const char* IDEArgument          = "-ide";			    // Indicates launching studio in advanced mode (full IDE)
	static const char* BuildArgument		= "-build";			    // Indicates launching studio in build mode (limited IDE docks)
    static const char* DebuggerArgument		= "-debugger";		    // Causes the program to wait for a debugger to attach on startup
    static const char* AvatarModeArgument   = "-avatar";            // Avatar Mode sets up the in-game GUI with an avatar to run around with
	static const char* RbxDevArgument		= "-rbxdev";            // RbxDev starts the mobile development deployer (currently in development as of 1/17/2014), allows mobile devices to connect to a game
	static const char* BrowserTrackerId	= "-browserTrackerId";	// Passed in from website used to log launch status

	enum LaunchMode
	{
		Play,
		Play_Protocol,
		Build,
		Edit
	};

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
	CRegKey GetKey(CString& out_operation, bool isStudioKey, bool is64bits = false);
	HRESULT PreStartGame(const CLSID& clsid);
	__declspec(dllexport) HRESULT StartGame(simple_logger<wchar_t> &logger, BSTR authenTicket, BSTR authenticationUrl, BSTR script, const CLSID& clsid, bool silentMode, TCHAR *guidName, bool startInHiddenMode, TCHAR *unhideEventName, LaunchMode launchMode);
	HRESULT StartGame(simple_logger<char> &logger, BSTR authenTicket, BSTR authenticationUrl, BSTR script, const CLSID& clsid, bool silentMode, TCHAR *guidName, bool startInHiddenMode, TCHAR *unhideEventName, LaunchMode editMode);
	HRESULT get_InstallHost(BSTR* pVal, const CLSID& clsid);
	HRESULT get_Version(BSTR* pVal, const CLSID& clsid);

	HRESULT get_IsUpToDate(simple_logger<wchar_t> &logger, VARIANT_BOOL* pVal, CProcessInformation& isUpToDateProcessInfo, const CLSID& clsid);
	HRESULT Update(const CLSID& clsid);
#endif

    struct EditArgs
	{
        std::string fileName;
		std::string authUrl;
		std::string authTicket;
		std::string script;
		std::string readyEvent;
		std::string showEventName;
        std::string launchMode;
		std::string avatarMode;
		std::string browserTrackerId;
	};

    std::wstring generateEditCommandLine(const EditArgs& editArgs);
    bool parseEditCommandArg(wchar_t** args,int& index,int count,EditArgs& editArgs);
}
