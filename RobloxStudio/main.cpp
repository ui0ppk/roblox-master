/**
 * main.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

// Precompiled Header
#include "stdafx.h"

// Qt Headers
#include <QApplication>
#include <QFile>

// Roblox Headers
#include "FastLog.h"
#include "RobloxServicesTools.h"


// Roblox Studio Headers
#include "RobloxApplicationManager.h"
#include "RobloxMainWindow.h"
#include "RobloxSettings.h"
#include "StudioUtilities.h"

#ifdef _WIN32
    #include "VersionInfo.h"
	#include "LogManager.h"
	#include "DumpErrorUploader.h"
    #include "SharedLauncher.h"
#endif

#ifdef QT_ROBLOX_STUDIO
    #include "SharedLauncher.h"
#endif

#ifdef Q_WS_MAC
    #ifndef _DEBUG
        #define ENABLE_BREAKPAD_CRASH_REPORTER
    #endif
#endif

#ifdef ENABLE_APPLE_CRASH_REPORTER
    #include "AppleCrashReporter.h"
#endif

#ifdef ENABLE_BREAKPAD_CRASH_REPORTER
    #include "BreakpadCrashReporter.h"
#endif

#include "rbx/Profiler.h"

LOGGROUP(RobloxWndInit)

static bool UploadDmp = false;
static bool showVersion = false;

/**
 * Given an argument name, get the parameter for the argument.
 */
QString getArgParameter(const QStringList& args,const QString& arg)
{
    const int index = args.indexOf(arg) + 1;
    return index > 0 && index < args.size() ? args[index] : "";
}

/**
 * Gets the file name and script name from the commandline arguments.
 */
void parseCommandLineArgs(QMap<QString,QString>& argMap)
{
    const QStringList args = QApplication::arguments();

	argMap[SharedLauncher::FileLocationArgument] = getArgParameter(args, SharedLauncher::FileLocationArgument);
	argMap[SharedLauncher::ScriptArgument]       = getArgParameter(args, SharedLauncher::ScriptArgument);
	argMap[SharedLauncher::BrowserTrackerId]	 = getArgParameter(args, SharedLauncher::BrowserTrackerId);
	argMap[SharedLauncher::TestModeArgument]     = args.contains(SharedLauncher::TestModeArgument) ? "TRUE" : "FALSE";
	argMap[SharedLauncher::IDEArgument]          = args.contains(SharedLauncher::IDEArgument) ? "TRUE" : "FALSE";
    argMap[SharedLauncher::BuildArgument]        = args.contains(SharedLauncher::BuildArgument) ? "TRUE" : "FALSE";
    argMap[SharedLauncher::AvatarModeArgument]   = args.contains(SharedLauncher::AvatarModeArgument) ? "TRUE" : "FALSE";
    	
	argMap[SharedLauncher::DebuggerArgument]     = args.contains(SharedLauncher::DebuggerArgument) ? "TRUE" : "FALSE";
	argMap[SharedLauncher::StartEventArgument]   = getArgParameter(args, SharedLauncher::StartEventArgument);
    argMap[SharedLauncher::ReadyEventArgument]   = getArgParameter(args, SharedLauncher::ReadyEventArgument);
    argMap[SharedLauncher::ShowEventArgument]    = getArgParameter(args, SharedLauncher::ShowEventArgument);
    
    /// args used in edit mode (when we launch from website)
	argMap[SharedLauncher::AuthUrlArgument]      = getArgParameter(args, SharedLauncher::AuthUrlArgument);
	argMap[SharedLauncher::AuthTicketArgument]   = getArgParameter(args, SharedLauncher::AuthTicketArgument);

	argMap[StudioUtilities::EmulateTouchArgument] = args.contains(StudioUtilities::EmulateTouchArgument) ? "TRUE" : "FALSE";
	argMap[StudioUtilities::StudioWidthArgument]  = getArgParameter(args, StudioUtilities::StudioWidthArgument);
	argMap[StudioUtilities::StudioHeightArgument] = getArgParameter(args, StudioUtilities::StudioHeightArgument);

	if (args.indexOf("-dmp") >= 0 || args.indexOf("-d") >= 0)
		UploadDmp = true;

	if (args.indexOf("--version") >= 0)
		showVersion = true;


    // handle special cases where we just open a file
    if (args.size() == 1)
    {
        // default to IDE mode with no arguments
        argMap[SharedLauncher::IDEArgument] = "TRUE";
    }
    else if ( args.size() == 2 && QFile::exists(args[1]) ) 
    {
        // one arg that's a file
        argMap[SharedLauncher::IDEArgument] = "TRUE";
        argMap[SharedLauncher::FileLocationArgument] = args[1];
    }
    else if ( args.size() == 3 && 
             (args[1] == SharedLauncher::IDEArgument || args[1] == SharedLauncher::FileLocationArgument) &&
             QFile::exists(args[2]) )
    {
        // two args that's IDE or FileLocation and a file
        argMap[SharedLauncher::IDEArgument] = "TRUE";
        argMap[SharedLauncher::FileLocationArgument] = args[2];
    }
}

#ifdef _WIN32

bool attachCurrentProcessToDebugger()
{
    if ( ::IsDebuggerPresent() == FALSE )
    {
        STARTUPINFO         sSi = {};
        PROCESS_INFORMATION sPi = {};
        TCHAR               szBuf[2560];
        DWORD               dwExitCode;

        szBuf[0] = '"';
        ::GetSystemDirectory(szBuf + 1,2000);
        int length = strlen(szBuf);
        if ( (length > 0) && (szBuf[length] != '/') && (szBuf[length] != '\\') )
            szBuf[length++] = '\\';
        sprintf_s(
            szBuf + length,
            2560 - length,
            "VSJitDebugger.exe\" -p %lu",
            ::GetCurrentProcessId() );

        BOOL created = ::CreateProcess(NULL,szBuf,NULL,NULL,FALSE,0,NULL,NULL,&sSi,&sPi);
        if ( created == TRUE )
        {
            ::WaitForSingleObject(sPi.hProcess,INFINITE);
            ::GetExitCodeProcess(sPi.hProcess,&dwExitCode);
            if ( dwExitCode != 0 ) // if exit code is zero, a debugger was selected
                created = FALSE;
        }

        if ( sPi.hThread )
            ::CloseHandle(sPi.hThread);
        if ( sPi.hProcess )
            ::CloseHandle(sPi.hProcess);

        if ( created == FALSE )
            return false;

        // wait for debugger to get started
        for ( int i = 0 ; i < 5 * 60 ; ++i )
        {
            if ( ::IsDebuggerPresent() != FALSE )
                break;
            ::Sleep(200);
        }
    }

    // force a debug break
#if defined _M_IX86
    _asm 
	{
        int 3
    };
#elif defined _M_X64
    __debugbreak();
#else
    ::DebugBreak();
#endif
    return true;
}

#endif

class RobloxApplication: public QApplication
{
public:
    RobloxApplication(int &argc, char **argv): QApplication(argc, argv)
	{
	}

    bool notify(QObject* receiver, QEvent* event) override
	{
		RBXPROFILER_SCOPE("Qt", "$Event");
		RBXPROFILER_LABELF("Qt", "%s::%d", receiver->objectName().toStdString().c_str(), event->type());

		return QApplication::notify(receiver, event);
	}
};

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // This was compiled with sse2.  If this is placed too late, there is a good
    // chance some float operation will occur.
    if (!G3D::System::hasSSE2())
    {
        MessageBoxA(NULL, "This platform lacks SSE2 support.", "ROBLOX", MB_OK);
        return false;
    }
    CoInitialize(NULL);
#endif

#ifdef Q_OS_MACX
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8)
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
#endif
        
    RobloxApplication app(argc,argv);
    Q_INIT_RESOURCE(RobloxStudio);
	
	// Platform independent settings
	QApplication::setOrganizationName("Roblox");
    QApplication::setOrganizationDomain("roblox.com");
    QApplication::setApplicationName("RobloxStudio");  
    QApplication::setApplicationVersion(RobloxSettings::getVersionString());

	//add plugins path for loading images in webpage
#ifdef _WIN32
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath()+"/imageformats");
#endif

	int retVal = -1;

	try
	{
#ifdef ENABLE_BREAKPAD_CRASH_REPORTER
		CrashReporter::setupCrashReporter();
#endif

		QMap<QString, QString> argMap;
		parseCommandLineArgs(argMap);

#ifdef _WIN32
        if ( argMap[SharedLauncher::DebuggerArgument] == "TRUE" )
            attachCurrentProcessToDebugger();

		if (UploadDmp)
		{
			// initialize crash reporter here so we can catch any crash that might happen during upload
			MainLogManager* mainLogManager = MainLogManager::getMainLogManager();
			if (mainLogManager)
            {
				mainLogManager->WriteCrashDump();
				mainLogManager->EnableImmediateCrashUpload(false);
			}

			boost::scoped_ptr<DumpErrorUploader> dumpErrorUploader;
			dumpErrorUploader.reset(new DumpErrorUploader(false, "Studio"));
			dumpErrorUploader->Upload(GetDmpUrl(RobloxSettings::getBaseURL().toStdString(), true));
			return retVal;
		}

		if (showVersion)
		{
			CVersionInfo vi;
			vi.Load(_AtlBaseModule.m_hInst);
			std::string versionNumber = "ROBLOX Studio Version #: ";
			versionNumber+= vi.GetFileVersionAsDotString();
				
			QMessageBox::information(
				NULL,
				"ROBLOX", 
				versionNumber.c_str());
			return 0;
		}


#endif

		RobloxApplicationManager::instance().registerApplication();
		RobloxMainWindow rbxMainWin(argMap);

#ifdef ENABLE_APPLE_CRASH_REPORTER
		AppleCrashReporter::check();
#endif
		// Following is required so that when we launch from QProcess new instance of RobloxStudio should come to the top of Z-Order
		rbxMainWin.activateWindow();
		rbxMainWin.raise();

		retVal = app.exec();

#ifdef ENABLE_BREAKPAD_CRASH_REPORTER
		CrashReporter::shutDownCrashReporter();
#endif	
	}

	catch (std::runtime_error const& exp)
	{
		FASTLOG1(FLog::RobloxWndInit, "Application closed due to an exception: '%s'", exp.what());
	}

	RobloxApplicationManager::instance().unregisterApplication();
	return retVal;
}
