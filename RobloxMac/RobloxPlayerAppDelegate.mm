//
//  RolboxPlayerAppDelegate.mm
//  SimplePlayerMac
//
//  Created by Tony on 10/26/10.
//  Copyright 2010 ROBLOX Corporation. All rights reserved.
//

#include <semaphore.h>

#import "RBXWindow.h"
#import "RobloxPlayerAppDelegate.h"
#import "Roblox.h"
#include "RobloxView.h"
#include "Util/FileSystem.h"
#include "Util/Guid.h"
#include "Util/MachineIdUploader.h"
#include "Util/Statistics.h"
#include "Util/Analytics.h"
#include "rbx/RbxDbgInfo.h"
#include "WebKit/WebPreferences.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/Datamodel.h"
#include "ObjectiveCUtilities.h"
#include "util/HttpAsync.h"
#include "RobloxServicesTools.h"
#include "FunctionMarshaller.h"
#include "util/http.h"
#include "util/ProgramMemoryChecker.h"
#include "util/safeToLower.h"
#include "network/API.h"
#include "script/LuaVM.h"

#include "v8xml/WebParser.h"

#include "util/RobloxGoogleAnalytics.h"
#include "rbx/SystemUtil.h"

#include <boost/algorithm/string/replace.hpp>

#import <AppKit/NSAlert.h>
#import <Foundation/NSThread.h>

FASTINTVARIABLE(RequestPlaceInfoTimeoutMS, 2000)
FASTINTVARIABLE(RequestPlaceInfoRetryCount, 5)
DYNAMIC_FASTINT(JoinInfluxHundredthsPercentage)
DYNAMIC_FASTINTVARIABLE(MacInfluxHundredthsPercentage, 1000)
FASTFLAG(GraphicsReportingInitErrorsToGAEnabled)
DYNAMIC_FASTFLAGVARIABLE(MacInferredCrashReporting, false)
FASTINTVARIABLE(InferredCrashReportingHundredthsPercentage, 1000)
FASTFLAG(UseBuildGenericGameUrl)
FASTFLAG(PlaceLauncherUsePOST)


@implementation RobloxPlayerAppDelegate

@synthesize window;
@synthesize mainView;
@synthesize robloxView;
@synthesize ogreView;
@synthesize mainMenu;

RBX::Analytics::InfluxDb::Points analyticsPoints;

-(id) init
{
    if(self = [super init])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowWillClose:)
                                                     name:@"NSWindowWillCloseNotification"
												   object:nil];

        [self checkUpdater:NO];
    }
    return self;
}

- (BOOL) checkUpdater:(BOOL) showUpdateOptionsDialog
{
	NSString* bootstrapperExec = [NSString stringWithFormat:@"%@/%@/%@", [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent], @"Roblox.app", @"Contents/MacOS/Roblox"];
	NSLog(@"Bootstrapper Executable = %@", bootstrapperExec);

	// Check for Bootstrapper existence
	if ([[NSFileManager defaultManager] isExecutableFileAtPath:bootstrapperExec])
	{
		NSMutableArray *arguments = [NSMutableArray array];

		// Check version #
		[arguments addObject:@"-check"];
		[arguments addObject:@"true"];

		// Kill Process by passing in the process id
		NSString *pid = [NSString stringWithFormat:@"%d", [[NSProcessInfo processInfo] processIdentifier]];
		NSLog(@"Process ID = %@", pid);
		[arguments addObject:@"-ppid"];
		[arguments addObject:pid];

		// Show/Hide the Update Dialog Option
		NSString* strShowUpdateDialog = [NSString stringWithFormat:@"%@", showUpdateOptionsDialog ? @"true" : @"false"];
		NSLog(@"Show Update Dialog = %@", strShowUpdateDialog);

		[arguments addObject:@"-updateUI"];
		[arguments addObject:strShowUpdateDialog];

		[NSTask launchedTaskWithLaunchPath:bootstrapperExec arguments:arguments];
    }

    // For all conditions just return YES for time being
    return YES;
}

// Application delegate methods
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	if( running && [self requestShutdownClient:ROBLOX_APP_SHUTDOWN_CODE_QUIT] )
		// push terminat¡on until later
		return NSTerminateLater;
	else
    {
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"AppLaunch"];
        [[NSUserDefaults standardUserDefaults] synchronize];

		BreakpadRelease(breakpad);
		return NSTerminateNow;
	}
}

-(void)addLogFile:(NSString *)file
{
	NSLog(@"Adding breakpad log: %@", file);
	BreakpadAddLogFile(breakpad, file);
}

-(void) addBreakPadKeyValue:(NSString *)breakPadKey withValue:(NSString *)value
{
	// We are not checking for NULL checks for any parameters as it is done inside of breakpad fn
	BreakpadAddUploadParameter(breakpad, breakPadKey, value);
}

-(void) addBreakPadKeyValueFromDictionary:(NSDictionary *) info
{
	NSString *key = [info objectForKey:@"key"];
	NSString *value = [info objectForKey:@"value"];

	[self addBreakPadKeyValue:key withValue:value];

}


-(void) addDbgInfoToBreakPad
{
	// Create all the MetaData for the crash reporter service
	[self addBreakPadKeyValue:@"RobloxProduct" withValue:@"RobloxPlayer"];

	NSString* temp = [NSString stringWithFormat:@"%lu", RBX::RbxDbgInfo::s_instance.cPhysicalTotal];
	[self addBreakPadKeyValue:@"cPhysicalTotal" withValue:temp];

	temp = [NSString stringWithFormat:@"%lu", RBX::RbxDbgInfo::s_instance.cbPageSize];
	[self addBreakPadKeyValue:@"cbPageSize" withValue:temp];

	temp = [NSString stringWithFormat:@"%lu", RBX::RbxDbgInfo::s_instance.TotalVideoMemory];
	[self addBreakPadKeyValue:@"TotalVideoMemory" withValue:temp];

	temp = [NSString stringWithFormat:@"%lu", RBX::RbxDbgInfo::s_instance.NumCores];
	[self addBreakPadKeyValue:@"NumCores" withValue:temp];

	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.PlaceCounter];
	[self addBreakPadKeyValue:@"PlaceCounter" withValue:temp];

	if(RBX::RbxDbgInfo::s_instance.Place0 > 0)
	{
	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.Place0];
	[self addBreakPadKeyValue:@"Place0" withValue:temp];
	}

	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.Place1];
	[self addBreakPadKeyValue:@"Place1" withValue:temp];

	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.Place2];
	[self addBreakPadKeyValue:@"Place2" withValue:temp];

	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.Place3];
	[self addBreakPadKeyValue:@"Place3" withValue:temp];

	temp = [NSString stringWithFormat:@"%ld", RBX::RbxDbgInfo::s_instance.PlayerID];
	[self addBreakPadKeyValue:@"PlayerID" withValue:temp];

	temp = [NSString stringWithFormat:@"%s", RBX::RbxDbgInfo::s_instance.GfxCardName];
	[self addBreakPadKeyValue:@"GfxCardName" withValue:temp];

	temp = [NSString stringWithFormat:@"%s", RBX::RbxDbgInfo::s_instance.GfxCardDriverVersion];
	[self addBreakPadKeyValue:@"GfxCardDriverVersion" withValue:temp];

	temp = [NSString stringWithFormat:@"%s", RBX::RbxDbgInfo::s_instance.GfxCardVendorName];
	[self addBreakPadKeyValue:@"GfxCardVendorName" withValue:temp];

	temp = [NSString stringWithFormat:@"%s", RBX::RbxDbgInfo::s_instance.AudioDeviceName];
	[self addBreakPadKeyValue:@"AudioDeviceName" withValue:temp];

	temp = [NSString stringWithFormat:@"%s", RBX::RbxDbgInfo::s_instance.ServerIP];
	[self addBreakPadKeyValue:@"ServerIP" withValue:temp];
}

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	[self leaveGame];
	Roblox::shutdownInstance();
}

- (BOOL)windowShouldClose:(id)sender
{
	return !running || ![self requestShutdownClient:ROBLOX_APP_SHUTDOWN_CODE_WINDOW_CLOSE];
}

- (void) leaveGame
{
	bool bLeavingGame = running;
	running = NO;

	if (bLeavingGame && robloxView)
	{
		delete robloxView;
		robloxView = NULL;
		[ogreView setRobloxView:nil];
	}

}

- (BOOL) requestShutdownClient:(RobloxAppShutdownCode)code
{
	shutdownCode = code;
	return robloxView ? robloxView->requestShutdownClient() : false;
}

- (void) machineIdKick_onlyRunFromMainThread {
    [self leaveGame];
    NSAlert *alert = [[NSAlert alloc] init];
    NSString *nMessage = [[NSString alloc] initWithUTF8String:RBX::MachineIdUploader::kBannedMachineMessage];
    [alert setMessageText:nMessage];
    [alert runModal];
    [nMessage release];
    [alert release];
    [self handleLeaveGame];
}

- (void) doMachineIdCheck {
    if (RBX::MachineIdUploader::uploadMachineId(GetBaseURL().c_str()) ==
            RBX::MachineIdUploader::RESULT_MachineBanned) {
        [self performSelectorOnMainThread:@selector(machineIdKick_onlyRunFromMainThread)
                               withObject:nil
                            waitUntilDone:NO];
    }
}

-(void) reportStats
{
    BOOL crash = [[NSUserDefaults standardUserDefaults] boolForKey:@"AppLaunch"];
    // We did not had a clean exit last time, report a crash
    RBX::Analytics::InfluxDb::Points points;
    points.addPoint("Session" , crash == YES ? "Crash" : "Success" );
    points.report("Mac-RobloxPlayer-SessionReport-Inferred", FInt::InferredCrashReportingHundredthsPercentage);

    RBX::Analytics::EphemeralCounter::reportCounter(crash == YES ? "Mac-ROBLOXPlayer-Session-Inferred-Crash" : "Mac-ROBLOXPlayer-Session-Inferred-Success", 1, true);


    [[NSUserDefaults standardUserDefaults] setBool:TRUE forKey:@"AppLaunch"];
    [[NSUserDefaults standardUserDefaults] synchronize];

}


RBX::HttpFuture loginAsync(const std::string& userName, const std::string& passWord)
{
    std::string loginUrl = RBX::format("%slogin/v1", GetBaseURL().c_str());
    std::string postData = RBX::format("{\"username\":\"%s\", \"password\":\"%s\"}", userName.c_str(), passWord.c_str());

    boost::replace_all(loginUrl, "http", "https");
    boost::replace_all(loginUrl, "www", "api");
    RBX::HttpPostData d(postData, RBX::Http::kContentTypeApplicationJson, false);
    return RBX::HttpAsync::post(loginUrl,d);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application

	// checking args for additional behavior
	//  - if the process has the -t or --test flag to show the test menu
	//  - if the process was lunched via -plugin to disable internet plugin loading
	NSArray* args = [[NSProcessInfo processInfo] arguments];
	Boolean showTestMenu = NO;
	Boolean disableInternetPlugins = NO;
	int testPlaceID = 0;
    NSString* userName = @"";
    NSString* passWord = @"";
	quitOnLeave = NO;
    gameType = GAMETYPE_PLAY;

    [self.window setAcceptsMouseMovedEvents:NO];

    // loop through arguments
    for (int i = 0; i < [args count]; i++)
    {
        NSString* arg = [args objectAtIndex:i];

        if ([arg isEqualToString:@"-t"] || [arg isEqualToString:@"--test"]) {
            showTestMenu = YES;
        }

        if ([arg isEqualToString:@"-plugin"]) {
            disableInternetPlugins = YES;
        }

        if ([arg isEqualToString:@"--place" ] || [arg isEqualToString:@"--id"]) {
            i++;
            if (i < [args count]) {
                testPlaceID = [[args objectAtIndex:i] intValue];
            }
        }
        if ([arg isEqualToString:@"--userName" ]) {
            i++;
            if (i < [args count]) {
                userName = [args objectAtIndex:i];
            }
        }
        if ([arg isEqualToString:@"--passWord" ]) {
            i++;
            if (i < [args count]) {
                passWord = [args objectAtIndex:i];
            }
        }

        // [arg isEqualToString:@"--md5" ] obfuscated
        if ([arg characterAtIndex:0] == '-' &&
            [arg characterAtIndex:1] == '-' &&
            [arg characterAtIndex:2] == 'm' &&
            [arg characterAtIndex:3] == 'd' &&
            [arg characterAtIndex:4] == '5')
        {
            i++;
            if (i < [args count]) {
                RBX::DataModel::hash = [[args objectAtIndex:i] UTF8String];
            }
        }
    }

	// We MUST disable plug in loading from standard preferences or else any of our HTTP access calls
	//  can load internet plugins... this can cause strange and undesirable side effects as we're running
	//  them within our own process.
	// Specifically, Adobe Flash now uses hardware accelerated graphics and handles their OpenGL contexts
	//  and resources in an unsafe manner that makes it completely incompatible with running another OpenGL
	//  program within the same view/process.
	// If a plugin is _absolutely_ needed for some web access in Roblox, we need to find a way to handle
	//  not loading Flash.
	[[WebPreferences standardPreferences] setPlugInsEnabled:!disableInternetPlugins];

	NSString* baseUrl = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"RbxBaseUrl"];
	const char* s = [baseUrl UTF8String];
	::SetBaseURL(s);

    NSMutableDictionary *plist = [[NSMutableDictionary alloc] init];
    [plist addEntriesFromDictionary:[[NSBundle mainBundle] infoDictionary]];
    NSString* version = [plist objectForKey:@"CFBundleShortVersionString"];
    NSLocale* locale = [NSLocale currentLocale];
    NSString* country = [locale displayNameForKey:NSLocaleIdentifier value:[locale localeIdentifier]];
    RBX::Analytics::setAppVersion([version UTF8String]);
    RBX::Analytics::setReporter("Mac Player");
    RBX::Analytics::setLocation([country UTF8String]);

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSUserDefaults *userDefaultArgs = [NSUserDefaults standardUserDefaults];

	NSString* ticket = [userDefaultArgs stringForKey:@"ticket"];
	NSString* authURL = [userDefaultArgs stringForKey:@"authURL"];
    NSString* scriptURL = [userDefaultArgs stringForKey:@"scriptURL"];

    // Perform the initial hash of the program memory
    RBX::initialProgramHash = RBX::ProgramMemoryChecker().getLastCompletedHash();
    RBX::pmcHash.nonce = RBX::initialProgramHash;

    LuaSecureDouble::initDouble();

    if(!(ticket && authURL && scriptURL))
    {
        if (testPlaceID == 0)
        {
            NSString *urlString = [NSString stringWithFormat: @"%@games.aspx", baseUrl];
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
            [NSApp terminate:nil];
        }
    }


	if( !Roblox::initInstance(self, false))
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"OK"];
        [alert setMessageText:@"Cannot connect to ROBLOX."];
        [alert setInformativeText:@"Please check your internet connection and try again"];
        [alert setAlertStyle:NSWarningAlertStyle];


        if ([alert runModal] == NSAlertFirstButtonReturn) {
            [NSApp terminate:nil];
            return;
        }
    }

    if (DFFlag::MacInferredCrashReporting)
    {
        [NSThread detachNewThreadSelector:@selector(reportStats)
                                 toTarget:self
                               withObject:nil];
    }


    analyticsPoints.addPoint("BaseInit", RBX::Time::nowFastSec());

    NSString* browserTrackerId = [userDefaultArgs stringForKey:@"browserTrackerId"];
    if (browserTrackerId && [browserTrackerId length] > 0)
    {
        RBX::Stats::setBrowserTrackerId([browserTrackerId UTF8String]);
        RBX::Stats::reportGameStatus("AppStarted");
    }

	if (testPlaceID != 0) {
		std::string stdAuthenticationUrl;
		std::string stdTicket;
		std::string stdScriptUrl;
        RBX::HttpFuture loginResult;
        if (userName.length && passWord.length)
        {
            loginResult = loginAsync([userName UTF8String], [passWord UTF8String]);
            loginResult.wait();
        }
		if (requestPlaceInfo(testPlaceID, stdAuthenticationUrl, stdTicket, stdScriptUrl))
		{
			ticket = [NSString stringWithCString: stdTicket.c_str() encoding: [NSString defaultCStringEncoding]];
			authURL = [NSString stringWithCString: stdAuthenticationUrl.c_str() encoding: [NSString defaultCStringEncoding]];
			scriptURL = [NSString stringWithCString: stdScriptUrl.c_str() encoding: [NSString defaultCStringEncoding]];
		}
	}

    NSMenuItem* testMenu = [self.mainMenu itemWithTitle:@"Test"];
    [testMenu setHidden:!showTestMenu];

    // check if we have a place launcher script?
    NSRange const testRange = [scriptURL rangeOfString:@"PlaceLauncher.ashx" options:NSCaseInsensitiveSearch];
    if (testRange.location != NSNotFound)
    {
        gameType = GAMETYPE_PLAY_PROTOCOL;
        analyticsPoints.addPoint("Mode", "Protocol");

        // show main window
        [mainView setHidden: NO];
        [self.window makeKeyAndOrderFront:self];

        // first authenticate user
        std::string ticketStr([ticket UTF8String]);
        std::string compound([authURL UTF8String]);
        if (!ticketStr.empty())
        {
            compound += "?suggest=";
            compound += ticketStr;
        }
        std::string result;
        try
        {
            RBX::Http http(compound.c_str());
            http.setAuthDomain(::GetBaseURL());
            http.get(result);
        }
        catch (std::exception)
        {
        }

        // initialize game
        try
        {
            self->robloxView = RobloxView::init_game( (void *) ogreView, (void *) self, false);
        }
        catch (RBX::base_exception& e)
        {
            [RobloxPlayerAppDelegate reportRenderViewInitError:e.what()];
        }

        [self setUIMessage:@"Requesting Server..."];
        RBX::Stats::reportGameStatus("AcquiringGame");

        // do other initializations
        [ogreView setRobloxView:self->robloxView];
        [ogreView setAppDelegate:self];

        [self.window makeFirstResponder:ogreView];

        [self setupGameServices];
        [self addDbgInfoToBreakPad];

        // application must be visible now
        sem_t *sem = sem_open("/robloxPlayerStartedEvent", 0);
        if (sem != SEM_FAILED)
        {
            sem_post(sem);
            sem_close(sem);
        }

        quitOnLeave = YES;
        running = YES;

        // launch a new thread to query place information
        [NSThread detachNewThreadSelector:@selector(LaunchPlaceThread:)
                                 toTarget:self
                               withObject:[NSArray arrayWithObjects:scriptURL, ticket, authURL, nil]];

        analyticsPoints.addPoint("PlaceLauncherThreadStarted", RBX::Time::nowFastSec());

        // set process to front
        ProcessSerialNumber psn;
        GetCurrentProcess(&psn);
        SetFrontProcess(&psn);
    }
    else
    {
        analyticsPoints.addPoint("Mode", "PlugIn");
        NSLog(@"Ticket = %@", ticket);
        NSLog(@"authURL = %@", authURL);
        NSLog(@"scriptURL = %@", scriptURL);

        //Test start place
        /*
        ticket = @"F88C41ACECB7A813F06C27DD6BAC69CDF3FA36CF0F51A8A1615EA771BACC7992F0F4B75ADF85796F6C439A1B25CD3566EEEA99BDD9B0950D820897E947B4D8DF8AC26DD62A91EE7387E8860B2039B21A71B3F7866F0D3EB535E2998F6EA7AC3CD975EE898D3562FC259AEF490DDAB65483C40495E1B91977B8469FC9DA40033EE905E20CAC85F612EAAFBE87164EDE0663549A6E8A13B9B7355078688B52E3541F82F7893E6A84E843011CF5233D2880F704026D55968AD0BA310BEFB173124B5913062C237571D5F13BC7F8C357BEF38DE7738CA625AC54B474EA123FAE520784ABE92CE300B8F272E346D0A298578B17F4D7F7E0F9CC282B2338B0C902BCEC9A88D93EDD3E2B9ABB20D27E46697A5776BE74415E6A6509B4523DBEDF2076A89A35DB874FC92BF77F331348FBEF6A7E520EB816DE12E77DA46AB69EC8A05F7DE715A28D";
        authURL = @"http://www.roblox.com/Login/Negotiate.ashx";
        scriptURL = @"http://www.roblox.com/Game/Join.ashx?ticket=15381924%0aaamazing%0ahttp%3a%2f%2fwww.roblox.com%2fAsset%2fCharacterFetch.ashx%3fuserId%3d15381924%26placeId%3d63557496%0aae11ca68-72d4-4b9f-b60f-299733791eb7%0a10%2f26%2f2011+6%3a52%3a49+PM%0aXLKM2J7HRRzP%2bcbDolTzyTw9AJVTk4pamL0RkNSXITHA9nPNBGZZ%2fQQHS%2fKmmX%2bR8QGuupTyWxk8vYlDyvb4jee5Do3Mos%2fkKPne%2b%2fm8Vpo9L9Nb0LNiKxO%2f%2b%2bVjlo1jtYljExC4MKlDa%2bIwE1tvGUuo9R5DVZX3CMbrUMnSdZA%3d&gameId=ae11ca68-72d4-4b9f-b60f-299733791eb7&placeId=63557496&isTeleport=False";
        */

        if(ticket && authURL && scriptURL)
        {
            [mainView setHidden: NO];
            [self.window makeKeyAndOrderFront:self];
            [self StartGame:ticket withAuthentication:authURL withScript:scriptURL];
            quitOnLeave = YES;

            analyticsPoints.addPoint("JoinScriptTaskSubmitted", RBX::Time::nowFastSec());
            analyticsPoints.report("ClientLaunch.Mac", DFInt::JoinInfluxHundredthsPercentage);

            [self setUIMessage:@"Loading..."];

            // application must be visible now
            sem_t *sem = sem_open("/robloxPlayerStartedEvent", 0);
            if (sem != SEM_FAILED)
            {
                sem_post(sem);
                sem_close(sem);
            }

            // this needs to happen after we've hit the login authenticator
            [NSThread detachNewThreadSelector:@selector(doMachineIdCheck)
                                     toTarget:self
                                   withObject:nil];
        }
    }

	[pool release];
}


extern "C" void writeFastLogDumpHelper(const char *fileName, int numEntries);

bool BreakpadCallback(int exception_type, int exception_code, mach_port_t crashing_thread, void *context)
{
    RBX::Analytics::InfluxDb::Points points;
    points.addPoint("SessionReport" , "MacCrash");
    points.report("Mac-RobloxPlayer-SessionReport", DFInt::MacInfluxHundredthsPercentage);

    // FIXME: https://jira.roblox.com/browse/CLI-10561
    // These calls are not safe to be called in crash handling context:
    // 1. They allocate
    // 2. There's a code path that leads to interacting with dead NSRunLoop, thus freezing crash generation
    RBX::Analytics::EphemeralCounter::reportCounter("Mac-ROBLOXPlayer-Crash", 1, true);
    RBX::Analytics::EphemeralCounter::reportCounter("ROBLOXPlayer-Crash", 1, true);

	Roblox::releaseTerminateWaiter();
	if (context)
		writeFastLogDumpHelper((const char *)context, 200);

	return true;
}

void InitBreakpadCallback(BreakpadRef breakpad)
{
	//get the log file
    boost::filesystem::path logFile = RBX::FileSystem::getLogsDirectory();
    std::string guid;
    RBX::Guid::generateRBXGUID(guid);
	logFile /= "log_" + guid.substr(3,6) + ".txt";

	//we will set the log file as context so can get it back (DO NOT delete)
	char *context = new char[strlen(logFile.c_str())+1];
	strcpy(context, logFile.c_str());

	BreakpadSetFilterCallback(breakpad, BreakpadCallback, (void *)context);

	//add log file with Breakpad
	BreakpadAddLogFile(breakpad, [NSString stringWithFormat:@"%s", logFile.c_str()]);
}

static BreakpadRef InitBreakpad(void) {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSMutableDictionary *plist = [[NSMutableDictionary alloc] init];
	[plist addEntriesFromDictionary:[[NSBundle mainBundle] infoDictionary]];

	NSString* base = [plist objectForKey:@"RbxBaseUrl"];
    NSString* breakpadUrl = [NSString stringWithUTF8String:GetBreakpadUrl([base UTF8String], true).c_str()];
	[plist setValue:breakpadUrl forKey:@BREAKPAD_URL];

    // Out-of-process breakpad is broken on 10.10 and newer
    [plist setValue:[NSNumber numberWithBool:TRUE] forKey:@BREAKPAD_IN_PROCESS];
    [plist setValue:NSTemporaryDirectory() forKey:@BREAKPAD_DUMP_DIRECTORY];

	BreakpadRef breakpad = BreakpadCreate(plist);

	InitBreakpadCallback(breakpad);

	NSString* version = [plist objectForKey:@"CFBundleShortVersionString"];
	BreakpadAddUploadParameter(breakpad, @"Version", version);

	NSString* session = [NSString stringWithFormat:@"%s", RBX::Guid().readableString(6).substr(0,5).c_str()];
	BreakpadAddUploadParameter(breakpad, @"Session", session);

	[pool release];
	return breakpad;
}

- (void)awakeFromNib
{
	breakpad = InitBreakpad();
}

- (void)windowWillClose:(NSNotification *)notification
{
    NSWindow* closingWindow = (NSWindow*)notification.object;

    if (closingWindow == self.window)
    {
        [self leaveGame];

        if(rbxWebView)
            [rbxWebView close];
    }
    else if (closingWindow == rbxWebView.window )
    {
        rbxWebView = nil;

        dispatch_async(dispatch_get_main_queue(), ^{
            [self.window makeKeyAndOrderFront:nil];
        });
    }

}

- (void)handleLeaveGame
{
	if(quitOnLeave)
		[NSApp terminate:nil];

	if ([ogreView inFullScreenMode])
	{
		[ogreView toggleFullScreen];
		return;
	}

	if( ![self requestShutdownClient:ROBLOX_APP_SHUTDOWN_CODE_LEAVE_GAME] )
	{
		[self leaveGame];

		Roblox::preloadGame(false);

		[NSCursor unhide];
	}
}

- (void)handleShutdownClient
{
	switch (shutdownCode) {
			//There was a bug where this case did not shut down the app on a 10.7 machine but worked on 10.5,10.6
		case ROBLOX_APP_SHUTDOWN_CODE_QUIT:
			[NSApp replyToApplicationShouldTerminate:YES];
			break;

		case ROBLOX_APP_SHUTDOWN_CODE_LEAVE_GAME:
			[self leaveGame];

			Roblox::preloadGame(false);

			[NSCursor unhide];
			break;

		case ROBLOX_APP_SHUTDOWN_CODE_WINDOW_CLOSE:
			[self.window close];
			break;
		default:
			break;
	}
}

- (void)handleToggleFullScreen
{
	if (ogreView)
		[ogreView toggleFullScreen];
}

-(bool) inFullScreenMode
{
    bool retVal = false;
	if (ogreView)
		retVal = [ogreView inFullScreenMode];

    return retVal;
}


// Action methods
- (IBAction)crash:(id)sender
{
	Roblox::testCrash();
}

- (void)teleport:(NSString *)ticket withAuthentication:(NSString *)url withScript:(NSString *)script
{
	NSLog(@"ticket = %@", ticket);
	NSLog(@"authURL = %@", url);
	NSLog(@"script = %@", script);

	running = NO;

	if (robloxView)
	{
        if (RBX::RunService* runService = robloxView->getDataModel()->find<RBX::RunService>())
			runService->stopTasks();

		delete robloxView;
		robloxView = NULL;
		[ogreView setRobloxView:nil];
	}

	Roblox::preloadGame(false);

	[self StartGame:ticket withAuthentication:url withScript:script];

	// need to trigger a mouse tracking update on ogreView
	if (self.ogreView != nil)
		[self.ogreView updateTrackingAreas];
}

-(void) setupDataModelServices:(RBX::DataModel*) dataModel
{
    if (RBX::GuiService* guiService = dataModel->find<RBX::GuiService>())
        openUrlConnection = guiService->openUrlWindow.connect( boostFuncFromSelector_1<std::string>(@selector(openUrlWindow:), self) );
}

-(void) setupGameServices
{
    [self setupDataModelServices:robloxView->getDataModel().get()];
}

-(void)StartGame:(NSString *)ticket withAuthentication:(NSString *)url withScript:(NSString *)script
{
	NSLog(@"ticket = %@", ticket);
	NSLog(@"authURL = %@", url);
	NSLog(@"script = %@", script);

	NSLog(@"StartGame: %@", script);

	NSUInteger len = [script length];
	char *scriptBuf = new char[len + 1];
	BOOL ok = [script getCString:scriptBuf maxLength:len+1 encoding:NSUTF8StringEncoding];

	if (ok)
	{
		char *urlBuf = new char[[url length]  + 1];
		ok = [url getCString:urlBuf maxLength:[url length] + 1 encoding:NSUTF8StringEncoding];

		char *ticketBuf = new char[[ticket length]  + 1];
		ok = [ticket getCString:ticketBuf maxLength:[ticket length] + 1 encoding:NSUTF8StringEncoding];

		std::string ticketStr(ticketBuf);
		std::string compound(urlBuf);
		if (!ticketStr.empty())
		{
			compound += "?suggest=";
			compound += ticketStr;
		}
		std::string result;
		try
		{
			RBX::Http http(compound.c_str());
			http.setAuthDomain(::GetBaseURL());
			http.get(result);
		}
		catch (std::exception)
		{
		}

        try
        {
            self->robloxView = RobloxView::start_game( (void *) ogreView, (void *) self, std::string(scriptBuf), false);
        }
        catch (RBX::base_exception& e)
        {
            [RobloxPlayerAppDelegate reportRenderViewInitError:e.what()];
        }


        [self addDbgInfoToBreakPad];

		[ogreView setRobloxView:robloxView];
		[ogreView setAppDelegate:self];

		[self.window makeFirstResponder:ogreView];

        [self setupGameServices];

		running = YES;
	}
}

-(void) openUrlWindow:(std::string) url
{
    if(rbxWebView)
        return;

    rbxWebView = [[RbxWebView alloc] initWithWindowNibName:@"RbxWebView"];

    dispatch_async(dispatch_get_main_queue(), ^{
        [rbxWebView.webView setResourceLoadDelegate:(id<WebResourceLoadDelegate>)self];

        NSRect webViewRect = rbxWebView.window.frame;
        NSRect windowRect = self.window.frame;

        [rbxWebView.webView setMainFrameURL:[NSString stringWithUTF8String:url.c_str()]];

        [rbxWebView.window
            setFrame:NSMakeRect(windowRect.origin.x + windowRect.size.width/2 - webViewRect.size.width/2, windowRect.origin.y + windowRect.size.height/2 - webViewRect.size.height/2, webViewRect.size.width, webViewRect.size.height)
         display:YES];

        [rbxWebView.window setLevel:ogreView.window.level + 1];
        [rbxWebView.window makeKeyAndOrderFront:ogreView.window];
        [rbxWebView.window makeFirstResponder:rbxWebView];
    });
}

-(void) ShutdownDataModel
{
    if(robloxView)
        robloxView->doShutdownDataModel();
}

static std::string readStringValue(shared_ptr<const RBX::Reflection::ValueTable> jsonResult, std::string name)
{
    RBX::Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<std::string>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected string result for %s", name.c_str()));
    }
}

static int readIntValue(shared_ptr<const RBX::Reflection::ValueTable> jsonResult, std::string name)
{
    RBX::Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<int>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected int result for %s", name.c_str()));
    }
}

bool requestPlaceInfo(int testPlaceID, std::string& authenticationUrl,
								   std::string& ticket,
								   std::string& scriptUrl)
{
    std::string requestURL;
    if (FFlag::UseBuildGenericGameUrl)
    {
        requestURL = BuildGenericGameUrl(GetBaseURL().c_str(), RBX::format("Game/PlaceLauncher.ashx?request=RequestGame&placeId=%d&isPartyLeader=false&gender=&isTeleport=true", testPlaceID));
    }
    else
    {
        requestURL = RBX::format("%sGame/PlaceLauncher.ashx?request=RequestGame&placeId=%d&isPartyLeader=false&gender=&isTeleport=true",
                                  GetBaseURL().c_str(), testPlaceID);
    }

    int retries = FInt::RequestPlaceInfoRetryCount;
    while (retries >= 0)
    {
        if (requestPlaceInfo(requestURL, authenticationUrl, ticket, scriptUrl) == SUCCESS)
            return true;
        else
        {
            retries--;
            sleep(FInt::RequestPlaceInfoTimeoutMS/1000);
        }
    }

    return false;
}

RequestPlaceInfoResult requestPlaceInfo(const std::string& requestURL, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl)
{
    try
    {
        std::string response;
        if (FFlag::PlaceLauncherUsePOST)
        {
            std::istringstream input("");
            RBX::Http(requestURL).post(input, RBX::Http::kContentTypeDefaultUnspecified, false, response);
        }
        else
        {
            RBX::Http(requestURL).get(response);
        }
            
        std::stringstream jsonStream;
        jsonStream << response;
        shared_ptr<const RBX::Reflection::ValueTable> jsonResult(rbx::make_shared<const RBX::Reflection::ValueTable>());
        bool parseResult = RBX::WebParser::parseJSONTable(jsonStream.str(), jsonResult);
        if (parseResult)
        {
            int status = readIntValue(jsonResult, "status");
            if (status == 2)
            {
                authenticationUrl = readStringValue(jsonResult, "authenticationUrl");
                ticket = readStringValue(jsonResult, "authenticationTicket");
                scriptUrl = readStringValue(jsonResult, "joinScriptUrl");
                return SUCCESS;
            }
            else if (status == 6)
                return GAME_FULL;
            else if (status == 10)
                return USER_LEFT;
            else
            {
                // 0 or 1 is not an error - it is a sign that we should wait
                if (status == 0 || status == 1)
                    return RETRY;
            }
        }
    }
    catch (RBX::base_exception& e)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Exception when requesting place info: %s. Retrying...", e.what());
    }

    return FAILED;
}

- (void) executeJoinScript:(NSString*)joinScript
{
    NSLog(@"executeJoinScript - %@", joinScript);
    if (robloxView)
        robloxView->executeJoinScript([joinScript UTF8String], false, (gameType == GAMETYPE_PLAY_PROTOCOL));

    // this needs to happen after we've hit the login authenticator
    [NSThread detachNewThreadSelector:@selector(doMachineIdCheck)
                             toTarget:self
                           withObject:nil];

}

- (void) setUIMessage:(NSString*)message
{
    NSLog(@"setUIMessage - %@", message);
    if (robloxView)
        robloxView->setUIMessage([message UTF8String]);
}

-(void) LaunchPlaceThread:(NSArray*) parameters
{
    NSLog(@"LaunchPlaceThread: num args - %d", parameters.count);

    // Index in array: 0 - scriptURL, 1 - ticket, 2 - authURL
    if (parameters.count == 3)
    {
        std::string placeLauncherUrl([[parameters objectAtIndex:0] UTF8String]);
        std::string authenticationUrl, authenticationTicket, joinScriptUrl;

        RBX::Time startTime = RBX::Time::nowFast();
        const std::string reportCategory = "PlaceLauncherDuration";

        // parse request type
        std::string requestType;
        std::string url = placeLauncherUrl;
        RBX::safeToLower(url);

        std::string requestArg = "request=";
        int requestPos = url.find(requestArg);
        if (requestPos != std::string::npos)
        {
            int requestEndPos = url.find("&", requestPos);
            if (requestEndPos == std::string::npos)
                requestEndPos = url.length();

            int requestTypeStartPos = requestPos+requestArg.size();
            requestType = url.substr(requestTypeStartPos, requestEndPos-requestTypeStartPos);
        }

        NSLog(@"Request: %s", requestType.c_str());

        int retries = FInt::RequestPlaceInfoRetryCount;
        RequestPlaceInfoResult res;
        while (retries >= 0)
        {
            res = requestPlaceInfo(placeLauncherUrl, authenticationUrl, authenticationTicket, joinScriptUrl);
            switch (res)
            {
                case SUCCESS:
                {
                    // execute join script and return
                    [self performSelectorOnMainThread:@selector(executeJoinScript:)
                                           withObject:[NSString stringWithUTF8String:joinScriptUrl.c_str()]
                                        waitUntilDone:NO];
                    RBX::Analytics::EphemeralCounter::reportStats(reportCategory + "_Success_" + requestType, (RBX::Time::nowFast() - startTime).seconds());

                    analyticsPoints.addPoint("RequestType", requestType.c_str());
                    analyticsPoints.addPoint("JoinScriptTaskSubmitted", RBX::Time::nowFastSec());
                    analyticsPoints.report("ClientLaunch.Mac", DFInt::JoinInfluxHundredthsPercentage);
                    return;
                }
                    break;
                case FAILED:
                    retries--;
                case RETRY:
                    sleep(FInt::RequestPlaceInfoTimeoutMS/1000);
                    break;
                case GAME_FULL:
                    // game full, wait!
                    [self performSelectorOnMainThread:@selector(setUIMessage:)
                                           withObject:@"The game you requested is currently full. Waiting for an opening..."
                                        waitUntilDone:NO];
                    sleep(FInt::RequestPlaceInfoTimeoutMS/1000);
                    break;
                case USER_LEFT:
                    [self performSelectorOnMainThread:@selector(setUIMessage:)
                                           withObject:@"The user has left the game."
                                        waitUntilDone:NO];
                    retries = -1;
                    break;
                default:
                    break;
            }
        }

        if (res == FAILED)
        {
            RBX::Analytics::EphemeralCounter::reportStats(reportCategory + "_Failed_" + requestType, (RBX::Time::nowFast() - startTime).seconds());
            // couldn't find place info, show error!
            [self performSelectorOnMainThread:@selector(setUIMessage:)
                                   withObject:@"Error: Failed to request game, please try again"
                                waitUntilDone:NO];
        }
    }
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through

-(void)marshallFunction:(NSEvent *)evt {

	void *pClosure = (void *) [evt data1];

	// Handle the event
	if (running)
	{
		RBX::FunctionMarshaller::handleAppEvent(pClosure);
	}
	else
	{
		RBX::FunctionMarshaller::freeAppEvent(pClosure);
	}

	[evt release];
}

+(void) reportRenderViewInitError:(const char *)message
{
    RBX::StandardOut::singleton()->printf(
        RBX::MESSAGE_ERROR, "Exception while creating RobloxView: %s.", message);

    if (FFlag::GraphicsReportingInitErrorsToGAEnabled)
    {
        std::string message = RBX::SystemUtil::getGPUMake() + " | " + RBX::SystemUtil::osVer();
        const char* label = "GraphicsInitError";
        RBX::RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_GAME, label , message.c_str(), 0);
    }

    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"There was a problem while initializing graphics."];
    [alert setInformativeText:[NSString stringWithUTF8String:message]];
    [alert setAlertStyle:NSWarningAlertStyle];
    [alert runModal];
    [NSApp terminate:nil];
}

@end
