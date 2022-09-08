//
//  Controller.m
//  Roblox
//
//  Created by Dharmendra on 14/01/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "Controller.h"
#import "CustomUIObjects.h"
#include "CookiesEngine.h"
#include <string>
#include <sstream>
#include <semaphore.h>

#define RAPIDJSON_ASSERT(exp) (void)(exp)
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"

#include "BootstrapperSettings.h"
#include "RobloxServicesTools.h"
#include "InfluxDbHelper.h"

//Apple script code for activating an application
#define ACTIVATE_APP    @"tell application \"%@\" \n activate \n end tell"

//client upgrade related
#define MAX_TICKS       20
#ifdef TARGET_IS_STUDIO
#define UPDATE_MESSAGE  @"Roblox Studio will update in %d seconds..."
#else
#define UPDATE_MESSAGE  @"Roblox will update in %d seconds..."
#endif


//cookies related
#define RBX_COOKIES_PATH_KEY "CPath"
#define RBX_COOKIES_PATH     "/tmp/roblox/rbxcsettings.rbx" //should this file go in Application Support folder?

static NSString * const sAnalyticsPath = @"Analytics/Measurement.ashx?IpFilter=primary&SecondaryFilterName=guid&SecondaryFilterValue=";

static NSString * const sDownloadDir                =@"Download";
static NSString * const sBackupDir                  =@"Backup";

//file to store update related information
static NSString * const sLocalUpdateInfoFile        =@"updateInfo.plist";

//Keys used in Preference file
static NSString * const sPFInstallHostKey           =@"RbxInstallHost";
static NSString * const sPFBaseServerKey            =@"RbxBaseUrl";
static NSString * const sPFPluginVersion            =@"RbxPluginVersion";

//transient keys for plugin and application interaction
static NSString * const sPFIsPluginHandlingKey      =@"RbxIsPluginHandling";

//keys used in updateInfo.plist
static NSString * const sBaseServerKey              =@"BaseServer";
static NSString * const sSetupServerKey             =@"SetupServer";
static NSString * const sServerVersionKey           =@"ServerVersion";
static NSString * const sEmbeddedKey                =@"Embedded";

//keys used in RobloxPlayer-Info.list
static NSString * const sRPBaseUrlKey               =@"RbxBaseUrl";
static NSString * const sRPInstallHostKey           =@"RbxInstallHost";

//relaunch daemon
static NSString * const sRelaunchApplicationDaemon  =@"Contents/Resources/ReLaunchApplication";


static NSString * const sStudioBootstrapperZipFile	=@"RobloxStudio.zip";
static NSString * const sStudioBootstrapperApplication	=@"RobloxStudio.app";

static NSString * const sServerStudioVersionFile = @"versionStudio";
static NSString * const sPreferenceFileID        = @"com.Roblox.Roblox";

static NSString * const sURLHandlerArgsSeparator = @"+";

//version update and target related consts
#ifdef TARGET_IS_STUDIO

//TODO: replace all occurances of Roblox with appName
static NSString * const sAppName					=@"ROBLOX Studio";

static NSString * const sServerVersionFile          = sServerStudioVersionFile;
static NSString * const sBootstrapperVersionFile    =@"RobloxStudioVersion.txt";

static NSString * const sClientApplicationBI        =@"com.roblox.RobloxStudio";
static NSString * const sClientApplication          =@"RobloxStudio.app";
static NSString * const sClientZipFile              =@"RobloxStudioApp.zip";

static NSString * const sBootstrapperApplication    =@"RobloxStudio.app";
static NSString * const sBootstrapperZipFile        =@"RobloxStudio.zip";

static NSString * const sNPAPIPlugin                =@"NPRoblox.plugin";

static NSString * const sTransformedApplication     =@"RobloxStudio.app";

static NSString * const sPFClientAppPathKey         =@"RbxStudioAppPath";
static NSString * const sPFBootstrapperPathKey      =@"RbxStudioBootstrapperPath";
static NSString * const sPFIsUptoDateKey            =@"RbxIsStudioUptoDate";
static NSString * const sPFUpdateCancelledKey       =@"RbxStudioUpdateCancelled";

static NSString * const sURLHandlerPrefix           =@"roblox-studio";

static const char* sWaitingSemaphoreName            ="/robloxStudioStartedEvent";
static const char* sSettingsGroup                   ="WindowsStudioBootstrapperSettings";

#else

static NSString * const sAppName					=@"ROBLOX";

static NSString * const sServerVersionFile          =@"version";
static NSString * const sBootstrapperVersionFile    =@"RobloxVersion.txt";

static NSString * const sClientApplicationBI        =@"com.roblox.RobloxPlayer";
static NSString * const sClientApplication          =@"RobloxPlayer.app";
static NSString * const sClientZipFile              =@"RobloxPlayer.zip";

static NSString * const sBootstrapperApplication    =@"Roblox.app";
static NSString * const sBootstrapperZipFile        =@"Roblox.zip";

static NSString * const sNPAPIPlugin                =@"NPRoblox.plugin";

static NSString * const sTransformedApplication     =@"Roblox.app";

static NSString * const sPFClientAppPathKey         =@"RbxClientAppPath";
static NSString * const sPFBootstrapperPathKey      =@"RbxBootstrapperPath";
static NSString * const sPFIsUptoDateKey            =@"RbxIsUptoDate";
static NSString * const sPFUpdateCancelledKey       =@"RbxUpdateCancelled";

static NSString * const sURLHandlerPrefix           =@"roblox-player";

static const char* sWaitingSemaphoreName            ="/robloxPlayerStartedEvent";
static const char* sSettingsGroup                   ="WindowsBootstrapperSettings";

#endif

static NSString * const sContentsFolder             =@"Contents";

static NSString * const sAppAdminInstallPath        =@"/Applications";
static NSString * const sPluginAdminInstallPath     =@"/Library/Internet Plug-ins";

static NSString * const sAppUserInstallPath         =@"~/Applications";
static NSString * const sPluginUserInstallPath      =@"~/Library/Internet Plug-ins";

@implementation Controller

- (void) initData
{
    srand(time(0));
    
    startTime = [NSDate date];
    [startTime retain];
    
	isInstalling              = YES;
	isDownloadingBootstrapper = NO;
	isDownloadingStudioBootstrapper = NO;
	isReportStatEnabled       = NO;
	isEmbedded                = NO;
	justEmbedded              = NO;
	installSuccess            = NO;
	isMac10_5				  = NO;
	
	
    [self setupInfluxDb];
    
	//get the appropriate server information from the infolist
	baseServer  = [[NSBundle mainBundle] objectForInfoDictionaryKey:sBaseServerKey];
	[baseServer retain];
	
	setupServer = [[NSBundle mainBundle] objectForInfoDictionaryKey:sSetupServerKey];
	[setupServer retain];
	
	//get server version
	serverVersion = [[self stringWithContentsOfURL:[NSString stringWithFormat:@"%@%@", setupServer, sServerVersionFile]] retain];
    serverStudioVersion = [[self stringWithContentsOfURL:[NSString stringWithFormat:@"%@%@", setupServer, sServerStudioVersionFile]] retain];
	
	//check if user is having write permissions in /Applications folder
	isAdminUser               = [[NSFileManager defaultManager] isWritableFileAtPath:sAppAdminInstallPath];
	
	launchedFromReadOnlyFS = NO;
	
	// check if the bundle path is writable
	NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
	launchedFromReadOnlyFS = ([bundlePath hasPrefix:@"/Volumes/"] && ![[NSFileManager defaultManager] isWritableFileAtPath:bundlePath]);
	
	NSString *isEmbeddedStr = [self getValueFromKey:sEmbeddedKey];
	isEmbedded = (isEmbeddedStr != nil && [isEmbeddedStr isEqualToString:@"true"]);
	
	isClientAppChanged = [self isVersionChanged];
    NSLog(@"Client app changed - %d", isClientAppChanged);
	
	if (isInstalling)
	{
		//first set cookies path
		CFPreferencesSetAppValue(CFSTR(RBX_COOKIES_PATH_KEY), CFSTR(RBX_COOKIES_PATH), (CFStringRef)sClientApplicationBI);
		
		std::wstring path = CookiesEngine::getCookiesFilePath();
        
        if (path.length() > 0) {
            CookiesEngine engine(path);
            CookiesEngine::reportValue(engine, "rbx_evt_initial_install_start", "{\"os\" : \"OSX\"}");
        }
	}
	
	//report stat enabling should be done only when we are connected to internet
	if (serverVersion)
	{
		reportStatGuid = rand();
		
		//see if we need to enable reportStat
		
		NSRange const findTestRange = [baseServer rangeOfString:@"test"];
		if (findTestRange.location != NSNotFound)
			isReportStatEnabled = YES;
		else
		{
			isReportStatEnabled = ((reportStatGuid % 10) == 0);	// Push only 10%
			//isReportStatEnabled = ((reportStatGuid % 100) == 0); // Push 1%	
		}
	}
	
	NSLog(@"setupServer = %@", setupServer);
	NSLog(@"serverVersion = %@", serverVersion);
	NSLog(@"bundlePath = %@", [[NSBundle mainBundle] bundlePath]);
	NSLog(@"launchedFromReadOnlyFS = %@", launchedFromReadOnlyFS ? @"Yes" : @"No");
	NSLog(@"isReportStatEnabled = %@", isReportStatEnabled ? @"Yes" : @"No");
}

- (void) cleanUp
{	
	//cancel download
    if (updateDownload) 
	{
        [updateDownload cancel];
        [self endUpdateDownload];
    }
	//TODO: cleanup NSString also
}

- (NSString*) osVersion
{
    SInt32 versionMajor=0, versionMinor=0, versionBugFix=0;
    Gestalt(gestaltSystemVersionMajor, &versionMajor);
    Gestalt(gestaltSystemVersionMinor, &versionMinor);
    Gestalt(gestaltSystemVersionBugFix, &versionBugFix);
    return [NSString stringWithFormat:@"%d.%d.%d", (int)versionMajor, (int)versionMinor, (int)versionBugFix];
}

- (void) verifyVersion
{
	NSString *osVerStr = [self osVersion];
	NSRange range = [osVerStr rangeOfString:@"10.5"] ;
	NSString *urlString = @"http://blog.roblox.com/2012/09/roblox-discontinuing-support-for-mac-os-x-10-5/";
	
	if (range.location == 0 && range.length == 4) 
	{
		isMac10_5 = YES;
		NSString *msg = [NSString stringWithFormat:@"Mac OS X Version %@", osVerStr];
		NSLog(@"Unsupported Mac OSX Version %@", osVerStr);
		
		if (NSAlertFirstButtonReturn == [self showAlertPanel:msg messageText:@"You are running a version of Mac OS X which is not supported" buttonText:@"OK"])
		{
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
		}
		
		//[self exitApplication];
		return;
	}
}

- (void) awakeFromNib
{
    //update background color to make it less transparent
    NSColor* color = [NSColor colorWithDeviceRed:256.0f/256.0f green:256.0f/256.0f blue:256.0f/256.0f alpha:1.0f];
    [progressPanel setBackgroundColor:color];
    [progressPanel update];
    [progressField setTextColor:[NSColor colorWithDeviceRed:26.0f/256.0f green:26.0f/256.0f blue:26.0f/256.0f alpha:0.6f]];
    
    [progressPanelButton setImageName:@"btn_cancel"];
    [[progressPanelButton cell] setHighlightsBy:NSNoCellMask];
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
    NSLog(@"applicationWillFinishLaunching");
    
    // install event filter to handle URL event
    [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self andSelector:@selector(handleURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* tempString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    urlHandlerString = [[tempString stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] retain];
    
    NSLog(@"URL - %@", urlHandlerString);
}

- (void) applicationDidFinishLaunching:(NSNotification*) aNotification
{
    bootstrapperSettings = NULL;
    
    // Check if we need to just show install success dialog?
    bool installSuccessDialogReq = false;
    NSArray* args = [[NSProcessInfo processInfo] arguments];
    for (int i = 0; i < [args count]; i++)
    {
        NSString* arg = [args objectAtIndex:i];
        if ([arg isEqualToString:@"-installsuccess"]) {
            installSuccessDialogReq = true;
        }
    }
    
    NSLog(@"install success - %d", installSuccessDialogReq);
    if (installSuccessDialogReq)
    {
        if ([self getSettings].GetValueShowInstallSuccessPrompt())
        {
            [self showInstallSuccessDialog];
        }
        else
        {
#ifdef TARGET_IS_STUDIO
            [self installDialogClosing:nil];
#else
            NSString *urlString = [NSString stringWithFormat: @"%@download/thankyou", [[NSBundle mainBundle] objectForInfoDictionaryKey:sBaseServerKey]];
            NSLog(@"install success url string - %@", urlString);
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
            [self exitApplication];
#endif
        }
        return;
    }
    
#ifdef TARGET_IS_STUDIO
    NSLog(@"Target is studio");
    [progressField setStringValue: @"Configuring ROBLOX Studio..."];
    // update image
    NSSize newSize;
    newSize.width = 90;
    newSize.height = 90;
    NSImage *image = [NSImage imageNamed:@"RobloxStudio"];
    [image setSize:newSize];
    [progressPanelImage setImage:image];
#endif
    
    if (urlHandlerString && urlHandlerString.length > 0)
    {
        // set process to front, so we do not redirect to download page
        ProcessSerialNumber psn;
        GetCurrentProcess(&psn);
        SetFrontProcess(&psn);
        
        //show progress dialog now...
        [self showProgressDialog];
    }
    
    [self verifyVersion];
    
    //initialize data
    [self initData];
    
    if (launchedFromReadOnlyFS)
    {
        //if we are not connected to internet then we cannot launch application in read only file system
        if (!serverVersion)
        {
            NSLog(@"Unable to connect to Roblox server");
            NSString *msg = [NSString stringWithFormat:@"%@ cannot connect to the Internet", sAppName];
            [self showAlertPanel:msg messageText:@"Unable to connect" buttonText:@"Cancel"];
            
            [self exitApplication];
            return;
        }
        
        NSLog(@"Application launched from read only file system (dmg)");
        [self reportStat:@"Launched From .dmg"];
        
        //copy bootstrapper at the desired location
        if ([self copyDirectory:sContentsFolder source:[[NSBundle mainBundle] bundlePath] destination:[self getClientApplication]])
        {
            //make sure we don't get warning while launching application again
            NSString * cmd = [NSString stringWithFormat:@"xattr -w com.apple.quarantine \"\" %@", [self getClientApplication]];
            system([cmd UTF8String]);
            
            //launch update
            NSLog(@"applicationDidFinishLaunching - relaunchApplication");
            [self relaunchApplication:[self getClientApplication] extraArguments:nil];
        }
        else
        {
            NSLog(@"Application installation failed");
            [self reportStat:@"Client installation failed"];
        }
        
        [self exitApplication];
        
        return;
    }
    
    //make sure we modify isUptoDate flag
    CFPreferencesSetAppValue((CFStringRef)sPFIsUptoDateKey, CFSTR("false"), (CFStringRef) sPreferenceFileID);
    CFPreferencesSetAppValue((CFStringRef)sPFUpdateCancelledKey, CFSTR("false"), (CFStringRef) sPreferenceFileID);
    CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
    
    //check if we need to handle command line options
    if ([self handleCommandLineOptions])
        return;
    
    //save protocol options if any
    [self saveProtocolArguments];
    
    //embedded case should "only" be handled by command line mode (no interactive support)
    if (isEmbedded && [urlHandlerArguments count] < 1)
    {
        NSLog(@"ERROR: Embedded application launched without command line options");
        
        [self reportStat:@"ERROR: Embedded Application Launched"];
        [self exitApplication];
        return;
    }
    
    //if we are not connected to internet then we cannot launch application in read only file system
    if (!serverVersion)
    {
        NSLog(@"Unable to connect to Roblox server");
        NSString *msg = [NSString stringWithFormat: @"%@ cannot connect to the Internet.", sAppName];
        [self showAlertPanel:msg messageText:@"Unable to connect" buttonText:@"Cancel"];
        
        [self exitApplication];
        return;
    }
    
    // if we have protocol arguments and no update is required, launch application NOW
    if (([urlHandlerArguments count] > 0) && !isClientAppChanged && ![self isBootstrapperUpdateRequired])
    {
        [self launchClientApplication:urlHandlerArguments];
        return;
    }
    
    NSLog(@"Application launched from app");
    [self reportStat:@"Launched From .app"];
    
    [self updateApplications];
}

- (BOOL) handleCommandLineOptions
{
	NSString *checkRequired = [[NSUserDefaults standardUserDefaults] stringForKey:@"check"];
	
	NSLog(@"Command Line: Handling command line options");
	
	if (checkRequired && ([checkRequired isEqualToString:@"yes"] || [checkRequired isEqualToString:@"true"]))
		return [self handleCheckOption];
	
	NSLog(@"Command Line: No command line arguments");
	
	return NO;
}

- (BOOL) handleCheckOption
{
	// if version has not changed just quit the application
	if (!isClientAppChanged && ![self isBootstrapperUpdateRequired])
	{
		NSLog(@"Command Line: No version change detected.");
		
		[self reportStat:@"Command Line Option: Version Not Changed"];
		
		installSuccess = YES;
		
		//check if we need to launch player
		NSString *playerLaunchRequired = [[NSUserDefaults standardUserDefaults] stringForKey:@"launchPlayer"];
        NSString *checkStudioRequired  = [[NSUserDefaults standardUserDefaults] stringForKey:@"checkStudio"];
		if (playerLaunchRequired && [playerLaunchRequired isEqualToString:@"true"])
		{
			//Just in case, check if we need to deploy plugin
			if ([self isNPAPIPluginDeploymentRequired])
			{
				NSLog(@"Plugin deployment in progress...");
				[self deployNPAPIPlugin];
				NSLog(@"Plugin deployment done...");
			}
			
			CFPreferencesSetAppValue((CFStringRef)sPFIsUptoDateKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
			CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
			
			NSLog(@"handleCheckOption - relaunchApplication");
            if ([urlHandlerArguments count] > 0)
            {
                [self launchClientApplication:urlHandlerArguments];
            }
            else
            {
                [self launchClientApplication:nil];
            }
		}
        else if (checkStudioRequired && [checkStudioRequired isEqualToString:@"true"])
        {
#ifndef TARGET_IS_STUDIO
            // check if we need to deploy Studio bootstrapper
            if (![self isStudioInstalled] || ![self applicationExistsInDock:[self getStudioApp] checkLink: YES])
            {
                NSLog(@"handleCheckOption - downloadStudioBootstrapper called");
                [self downloadStudioBootstrapper];
            }
            else
            {
                // save settings to correctly configure studio on launch of Build or Edit
                CFPreferencesSetAppValue(CFSTR("RbxStudioAppPath"), (CFStringRef) [self getStudioApp], (CFStringRef) sPreferenceFileID);
                CFPreferencesSetAppValue(CFSTR("RbxStudioBootstrapperPath"), (CFStringRef) [self getStudioApp], (CFStringRef) sPreferenceFileID);
                CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
                [self exitApplication];
            }
#endif
        }
		else
		{
			[self exitApplication];
		}
		
		return YES;
	}
	
	[self reportStat:@"Command Line Option: Version Changed"];
	
	parentProcessId = [[[NSUserDefaults standardUserDefaults] stringForKey:@"ppid"] intValue];
	
	[self initUpdateData];
	
	NSString *showUpdateUI = [[NSUserDefaults standardUserDefaults] stringForKey:@"updateUI"];
	NSLog(@"ppid: %d, showUpdateUI: %@", parentProcessId, showUpdateUI);
    
    // check if we have URL handler arguments
    NSString *urlStringArgument = [[NSUserDefaults standardUserDefaults] stringForKey:@"urlString"];
    if (urlStringArgument && [urlStringArgument length] > 0)
    {
        NSLog(@"Got URL arguments - %@", urlStringArgument);
        urlHandlerString = [urlStringArgument retain];
        //save protocol options if any
        [self saveProtocolArguments];
    }
    
	//by default we will show the update panel
	if (showUpdateUI != nil && [showUpdateUI isEqualToString:@"false"])
	{
		[self onUpdatePanelButtonClicked:NULL];
	}
	else 
	{
		[updatePanel     makeKeyAndOrderFront:self];
		[updatePanel     setLevel:NSModalPanelWindowLevel];
		
		updateTimer = [NSTimer scheduledTimerWithTimeInterval  : 1.0
													   target  : self
													   selector: @selector(onTimerTick:)
													   userInfo: nil 
													   repeats : YES];
	}
	
	return YES;
}

- (BOOL) saveProtocolArguments
{
    if (!urlHandlerString || ![urlHandlerString hasPrefix:sURLHandlerPrefix])
        return false;
    
    NSArray *listItems = [urlHandlerString componentsSeparatedByString:sURLHandlerArgsSeparator];
    if (listItems)
    {
        int count = [listItems count];
        NSString* item;
        
        urlHandlerArguments = [[NSMutableArray alloc] init];
        for (int i = 1; i < count; i++)
        {
            NSLog (@"Element %i = %@", i, [listItems objectAtIndex: i]);
            
            item = [listItems objectAtIndex: i];
            
            NSRange range = [item rangeOfString:@":"];
            if (range.location != NSNotFound)
            {
                if ([item hasPrefix:@"gameinfo"])
                {
                    [urlHandlerArguments addObject:@"-ticket"];
                    [urlHandlerArguments addObject:[item substringFromIndex:range.location+1]];
                }
                else if ([item hasPrefix:@"browsertrackerid"])
                {
                    [urlHandlerArguments addObject:@"-browserTrackerId"];
                    [urlHandlerArguments addObject:[item substringFromIndex:range.location+1]];
                    // assign to a local variable also so we can set into client requests
                    browserTrackerId = [item substringFromIndex:range.location+1];
                }
#ifndef TARGET_IS_STUDIO
                else if ([item hasPrefix:@"launchmode"])
                {
                    //TODO
                }
                else if ([item hasPrefix:@"placelauncherurl"])
                {
                    [urlHandlerArguments addObject:@"-scriptURL"];
                    [urlHandlerArguments addObject:[item substringFromIndex:range.location+1]];
                }
#else
                else if ([item hasPrefix:@"launchmode"])
                {
                    NSString* mode = [item substringFromIndex:range.location+1];
                    if ([mode isEqualToString:@"build"])
                        [urlHandlerArguments addObject:@"-build"];
                    else if ([mode isEqualToString:@"ide"])
                        [urlHandlerArguments addObject:@"-ide"];
                }
                else if ([item hasPrefix:@"script"])
                {
                    [urlHandlerArguments addObject:@"-script"];
                    [urlHandlerArguments addObject:[item substringFromIndex:range.location+1]];
                }
#endif
            }
#ifdef TARGET_IS_STUDIO
            if ([item hasPrefix:@"avatar"])
            {
                NSRange range = [item rangeOfString:@":"];
                if (range.location == NSNotFound || [[item substringFromIndex:range.location+1] isEqualToString:@"true"])
                {
                    [urlHandlerArguments addObject:@"-avatar"];
                }
            }
#endif
        }
        
#ifdef TARGET_IS_STUDIO
        [urlHandlerArguments addObject:@"-url"];
#else
        [urlHandlerArguments addObject:@"-authURL"];
#endif
        // replace http with https
        NSString* urlString =[NSString stringWithFormat:@"%@Login/Negotiate.ashx", baseServer];
        urlString = [urlString stringByReplacingOccurrencesOfString:@"http://" withString:@"https://"];
        [urlHandlerArguments addObject:urlString];
        
#ifdef TARGET_IS_STUDIO
        [urlHandlerArguments addObject:@"-plugin"];
#endif
    }

    return true;
}

- (void) initUpdateData
{
	ticks = 1;
	
	//create update panel
	updatePanel = NSGetAlertPanel(@"New updates available", @"", @"Update", @"Cancel", nil);
	
	//get appropriate fields from the panel
    NSEnumerator *objectEnum = [[[updatePanel contentView] subviews] objectEnumerator];
    NSView       *subView    = nil;
	int       textFieldCount = 0;
	
    while (subView = [objectEnum nextObject]) 
	{
        if ([subView isKindOfClass:[NSTextField class]]) 
		{
            textFieldCount++;
			//we need to update second text field
            if (textFieldCount == 2) 
                updateTextField = (NSTextField *)subView;
        }
		else if ([subView isKindOfClass:[NSButton class]]) 
		{
			//get update button's notifications
			[(NSButton *)subView setTarget:self];
			[(NSButton *)subView setAction:@selector(onUpdatePanelButtonClicked:)];
		}
    }
    
    [updateTextField setStringValue:[NSString stringWithFormat:UPDATE_MESSAGE, MAX_TICKS]];
}

- (void) onTimerTick:(NSTimer *)timer
{
	//if max ticks
	if (ticks >= MAX_TICKS)
	{
		[self onUpdatePanelButtonClicked:NULL];
		return;
	}
	
	//update panel text field with appropriate count
	[updateTextField setStringValue:[NSString stringWithFormat:UPDATE_MESSAGE, MAX_TICKS - ticks]];
	ticks++;
}

- (void) onUpdatePanelButtonClicked:(id)sender
{
	[updateTimer invalidate];
	[updatePanel orderOut:nil];
	
	if (sender && ([[(NSButton *)sender title] isEqualToString:@"Cancel"]))
	{
		NSLog(@"Update cancelled...");
		
		[self reportStat:@"Update User Cancelled"];
        [self reportInstallMetricsToInfluxDb:TRUE result:@"Cancelled" message:@"User Cancelled"];
        [self exitApplication];
		return;
	}
	
	NSLog(@"Updating application...");
	
	[updatePanel close];
	[updatePanel release];
	
	updateTextField = nil;
	updatePanel     = nil;
	updateTimer     = nil;
	
	//kill parent process
	ProcessSerialNumber psn;
	NSLog(@"killing processId: %d", parentProcessId);
	if (parentProcessId > 0 && (GetProcessForPID(parentProcessId, &psn) != procNotFound)) {
		KillProcess(&psn);
	} else {
		NSLog(@"Killing process failed");
	}

	
	//application requires an update
	[self updateApplications];
}

- (void) updateApplications
{
	//must make sure the client application bundle is writable for an update
	justEmbedded = isEmbedded ? NO : YES;
	if (![[NSFileManager defaultManager] isWritableFileAtPath:[self getClientApplication]])
	{
		//TODO: Authorize to get an access?
		[self reportStat:[NSString stringWithFormat:@"Client Deployment Failed"] requestMessage:@"App bundle readonly"];
        [self reportInstallMetricsToInfluxDb:!isInstalling result:@"Failed" message:@"App bundle readonly"];
        
		NSLog(@"Application bundle is not writable, no upgrade.");
		NSString *msg = [NSString stringWithFormat: @"An error occured and %@ cannot continue.\nIf problem persists please download a new version.", sAppName];
        [self showAlertPanel:msg messageText:@"Unable to upgrade" buttonText:@"Cancel"];
        
        // set cancelled key to true
        CFPreferencesSetAppValue((CFStringRef)sPFUpdateCancelledKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
        CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
		
		[self exitApplication];
		return;
	}
	justEmbedded = NO;
	
    if (!isInstalling)
    {
        NSString *txt = [NSString stringWithFormat: @"Upgrading %@...", sAppName];
        [progressField setStringValue:txt];
    }
	//show progress dialog now...
	[self showProgressDialog];
	
	//update applications appropriately
	if ([self isBootstrapperUpdateRequired])
	{
		NSLog(@"Bootstrapper update in progress...");
		
		[self reportStat:@"Bootstrapper Update Started"];
		[self downloadBootstrapper];
	}
	else 
	{
		NSLog(@"Client application update in progress...");
		
		//deploy plugin as early as possible so browser can take control of proceedings
		if (isInstalling)
		{
            // repair chrome
            [self validateAndFixChromeState];
            [self reportClientStatusSync:@"BootstrapperInstalling"];
            
			//set only the server information
			CFPreferencesSetAppValue((CFStringRef)sPFBaseServerKey, baseServer, (CFStringRef) sPreferenceFileID);
			CFPreferencesSetAppValue((CFStringRef)sPFInstallHostKey, [[NSURL URLWithString:setupServer] host], (CFStringRef) sPreferenceFileID);
			CFPreferencesSetAppValue((CFStringRef)sPFClientAppPathKey, NULL, (CFStringRef) sPreferenceFileID);
			CFPreferencesSetAppValue((CFStringRef)sPFBootstrapperPathKey, NULL, (CFStringRef) sPreferenceFileID);
			// remove Studio related keys also
#ifndef TARGET_IS_STUDIO
			 CFPreferencesSetAppValue(CFSTR("RbxStudioAppPath"), NULL, (CFStringRef) sPreferenceFileID);
             CFPreferencesSetAppValue(CFSTR("RbxStudioBootstrapperPath"), NULL, (CFStringRef) sPreferenceFileID);
#endif
			CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
            
            // set plug-in version
            NSString* bundle = [[NSBundle mainBundle] resourcePath];
            NSLog(@"Bootstrapper Path: %@", bundle);
            bundle = [bundle stringByAppendingString:@"/../Plugins/NPRoblox.plugin/Contents/Info.plist"];
            NSLog(@"Plugin Plist Path: %@", bundle);
            NSDictionary *list = [NSDictionary dictionaryWithContentsOfFile:bundle];
            NSString *version = [list objectForKey:@"CFBundleShortVersionString"];
            CFPreferencesSetAppValue((CFStringRef)sPFPluginVersion, version, (CFStringRef) sPreferenceFileID);
            CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
		}
        else
        {
            // send BootstrapperInstalling event for Update as well
            [self reportClientStatusSync:@"BootstrapperInstalling"];
        }
		
		//Deploy plugin
		if ([self isNPAPIPluginDeploymentRequired] || isInstalling)
		{
			NSLog(@"Plugin deployment in progress...");
			[self deployNPAPIPlugin];
			NSLog(@"Plugin deployment done...");
		}
		
		[self reportStat:[NSString stringWithFormat:@"Client %@ Started", isInstalling ? @"Install" : @"Update"]];
		[self downloadClientApplication];
	}	
}

- (void) downloadClientApplication
{
	//TODO: for part update create an array of all files that can be downloaded and then download them one by one
	
	NSString *appSource      = [self getURL:sClientZipFile];
	NSString *appDestination = [self getTempPath:sClientZipFile];
	
	NSLog(@"Client Application URL = %@", appSource);
	NSLog(@"Client Application DownloadPath = %@", appDestination);
	
	isDownloadingBootstrapper = NO;
	
	[self reportStat:@"Client Download Started"];
	[self startDownload:appSource destination:appDestination];
	//[self downloadDidFinish:NULL];
}

- (void) downloadBootstrapper
{
	NSString *appSource      = [self getURL:sBootstrapperZipFile];
	NSString *appDestination = [self getTempBackupPath:sBootstrapperZipFile];
	
	NSLog(@"Bootstrapper URL = %@", appSource);
	NSLog(@"Bootstrapper DownloadPath = %@", appDestination);
	
	isDownloadingBootstrapper = YES;
	
	[self reportStat:@"Bootstrapper Download Started"];
	[self startDownload:appSource destination:appDestination];
	//[self downloadDidFinish:NULL];
}

- (void) downloadStudioBootstrapper
{
	NSString *appSource      = [self getStudioURL:sStudioBootstrapperZipFile];
	NSString *appDestination = [self getTempPath:sStudioBootstrapperZipFile];
	NSLog(@"Bootstrapper URL = %@", appSource);
	NSLog(@"Bootstrapper DownloadPath = %@", appDestination);
	isDownloadingStudioBootstrapper = YES;
	isDownloadingBootstrapper = NO;
	[self startDownload:appSource destination:appDestination];
}

- (BOOL) startDownload:(NSString *)source destination:(NSString *)destination
{
	//end any download in progress
	[self endUpdateDownload];
	
	//create a new request
	NSURLRequest *downloadRequest = [NSURLRequest requestWithURL:[[NSURL alloc] initWithString:source] cachePolicy:NSURLRequestUseProtocolCachePolicy timeoutInterval:10.0];
    updateDownload = [[NSURLDownload alloc] initWithRequest:downloadRequest delegate:(id)self];
    if (!updateDownload) 
		return NO;
	
	NSLog(@"Download started for %@", source);
	
	[updateDownload    setDestination:destination allowOverwrite:YES];
	
	//Modify dialog properties
	[progressIndicator setIndeterminate:YES];
	[progressIndicator startAnimation:nil];
	[progressPanel     makeKeyAndOrderFront:nil];
	
	//Make dialog as modal
	[progressPanel     setLevel:NSModalPanelWindowLevel];
	
	return YES;
}

- (void)endUpdateDownload 
{
    [updateDownload release];
    updateDownload = nil;
    [downloadResponse release];
    downloadResponse = nil;
}

- (void)download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response 
{
    [downloadResponse release];
    downloadResponse = [response retain];
    downloadBytes = 0;
}

- (void)download:(NSURLDownload *)download didReceiveDataOfLength:(unsigned)length 
{
    long long expectedLength = [downloadResponse expectedContentLength];
    downloadBytes += length;
    if (expectedLength > 0) 
	{
        float percentComplete = ((float)downloadBytes / (float)expectedLength) * 100.0;
        [progressIndicator setIndeterminate:NO];
        [progressIndicator setDoubleValue:percentComplete];
    }
}

- (void)download:(NSURLDownload *)download didFailWithError:(NSError *)error 
{
	NSString *msg = [NSString stringWithFormat: @"%@ was not able to update.", sAppName];
    [self showAlertPanel:msg messageText:@"Unable to download" buttonText:@"Cancel"];
    NSLog(@"Download failed");
    
    // set cancelled key to true
    CFPreferencesSetAppValue((CFStringRef)sPFUpdateCancelledKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
    CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
	
	[self reportStat:[NSString stringWithFormat:@"ERROR: Downloading %@ Failed", isDownloadingBootstrapper ? @"Bootstrapper" : @"Client"]];
	[self reportStat:@"Download Failure Reason" requestMessage:[[error userInfo] description]];
    [self reportInstallMetricsToInfluxDb:!isInstalling result:@"Failed" message:[[error userInfo] description]];
	[self exitApplication];
}

- (void)downloadDidFinish:(NSURLDownload *)download 
{	
	NSLog(@"Download finished for %@", isDownloadingBootstrapper ? @"bootstrapper" : @"client application");
	
	if (isDownloadingBootstrapper)
	{
		NSLog(@"Deploying bootstrapper");
		[self downloadBootstrapperDidFinish];
	}
	else if(isDownloadingStudioBootstrapper)
	{
		NSLog(@"Deploying Studio bootstrapper");
		[self downloadStudioBootstrapperDidFinish];
	}
	else
	{
		NSLog(@"Deploying client application");
		[self downloadClientApplicationDidFinish];
	}
	
	if (!installSuccess)
	{
		[self reportStat:[NSString stringWithFormat:@"%@ Deployment Failed", isDownloadingBootstrapper ? @"Bootstrapper" :@"Client"]];
		
		NSString *msgTitle = [NSString stringWithFormat:@"Unable to %@", isInstalling ? @"install" : @"upgrade"];
		NSString *msg = [NSString stringWithFormat: @"An error occured and %@ cannot continue.\nIf problem persists please download a new version.", sAppName];
        [self showAlertPanel:msg messageText:msgTitle buttonText:@"Cancel"];
        
        // set cancelled key to true
        CFPreferencesSetAppValue((CFStringRef)sPFUpdateCancelledKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
        CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
		
		//also remove the temporary data
		[self removeTemporaryDirectory];
	}
	
	//exit application
	if (!isDownloadingStudioBootstrapper) {
		[self exitApplication];
	}
}

- (void) downloadBootstrapperDidFinish
{
	if ([self deployBootstrapper])
	{
		NSLog(@"Bootstrapper deployed");
		//make sure we save version information only when there is NO version change
		if (!isInstalling)
			[self saveUpdateInfo:!isClientAppChanged];
		
		NSMutableArray *extraArguments = [NSMutableArray array];
		NSString       *appPath = [self getBootstrapperApplication];
		if (isEmbedded)
		{
			NSLog(@"Extra arguments added");
			
			[extraArguments addObject:@"-check"];
			[extraArguments addObject:@"true"];
			
			[extraArguments addObject:@"-updateUI"];
			[extraArguments addObject:@"false"];
			
            if ([urlHandlerArguments count] < 1)
            {
                CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
                CFStringRef isPluginHandling = (CFStringRef)CFPreferencesCopyAppValue((CFStringRef)sPFIsPluginHandlingKey, (CFStringRef) sPreferenceFileID);
                if (isPluginHandling == NULL)
                {
                    NSLog(@"Plugin is not handling, check if we need to launch player");
                    
                    [extraArguments addObject:@"-launchPlayer"];
                    [extraArguments addObject:@"true"];
                }
                else 
                {
                    NSLog(@"Plugin handling");
                    CFRelease(isPluginHandling);
                }
            }
			
			NSBundle *bootstrapperBundle = [[NSBundle alloc]initWithPath:appPath];
			if (bootstrapperBundle != nil)
			{
				appPath = [bootstrapperBundle executablePath];
				[bootstrapperBundle release];
			}
		}
		
		installSuccess = YES;
		
		NSLog(@"Relaunching Bootstrapper");
        
        if ([urlHandlerArguments count] > 0)
        {
            [extraArguments addObject:@"-urlString"];
            [extraArguments addObject:urlHandlerString];
            NSLog(@"appending urlhandler arguments Bootstrapper");
        }
		
		[self reportStat:@"Bootstrapper Update Success"];
        [self reportInstallMetricsToInfluxDb:TRUE result:@"Success" message:@"Bootstrapper Update"];
		NSLog(@"downloadBootstrapperDidFinish - relaunchApplication");
		[self relaunchApplication:appPath extraArguments:extraArguments];
	}
	else 
	{
		NSLog(@"ERROR: Bootstrapper cannot be deployed");
        [self reportInstallMetricsToInfluxDb:TRUE result:@"Failed" message:@"Bootstrapper cannot be deployed"];
	}	
}

- (void) downloadStudioBootstrapperDidFinish
{
	//if application hasn't been handled by plugin then launch application
    
	[self deployStudioBootstrapper];
   
    // save settings to correctly configure studio on launch of Build or Edit
    CFPreferencesSetAppValue(CFSTR("RbxStudioAppPath"), (CFStringRef) [self getStudioApp], (CFStringRef) sPreferenceFileID);
    CFPreferencesSetAppValue(CFSTR("RbxStudioBootstrapperPath"), (CFStringRef) [self getStudioApp], (CFStringRef) sPreferenceFileID);
    CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
  	
	if (![self applicationExistsInDock:[self getStudioApp] checkLink: YES])
	{
		NSLog(@"adding studio bootstrapper to dock");
		[self addApplicationInDock:[self getStudioApp]];
		//delay application launch to let the creation go through
		usleep(300000);
	}
	
	[self removeTemporaryDirectory];

	isDownloadingStudioBootstrapper = NO;
	if ([self relauchRequired])
	{
        if (isInstalling)
        {
            NSLog(@"downloadStudioBootstrapperDidFinish - showInstallSuccessDialog");
            NSMutableArray *extraArgs = [NSMutableArray array];
            [extraArgs addObject:@"-installsuccess"];
            [self relaunchApplication:[self getBootstrapperApplication] extraArguments:extraArgs];
        }
        else if ([urlHandlerArguments count] > 0)
        {
            NSLog(@"downloadStudioBootstrapperDidFinish - with url arguments");
            [self relaunchApplication:[self getClientApplicationExecutable] extraArguments:urlHandlerArguments];
        }
        else
        {
            NSLog(@"downloadStudioBootstrapperDidFinish - nil arguments");
            [self relaunchApplication:[self getClientApplication] extraArguments:nil];
        }
	}
    else
    {
		[self exitApplication];
	}

}

- (void) downloadClientApplicationDidFinish
{
	NSString *txt = [NSString stringWithFormat: @"Starting %@...", sAppName];
	[progressField     setStringValue:txt];
	[progressIndicator setIndeterminate:YES];
	[progressIndicator startAnimation:nil];
	
	//take a back up of bootstrapper
	installSuccess = [self backupBootstrapper];
	
	justEmbedded = isEmbedded ? NO : YES;
	
	//deploy client application	
	if (installSuccess && [self deployClientApplication])
	{
		installSuccess = NO;
		
		NSString *clientApplication = [self getClientApplication];
		if ([[NSFileManager defaultManager] fileExistsAtPath:clientApplication])
		{
			NSLog(@"Client Application found and ready to launch from %@", clientApplication);
			
			//embed bootstrapper from temp location
			[self embedBootstrapper];
			
			//save version related information on successful launch
			[self saveUpdateInfo:YES];
			
			//also modify client info appropriately
			[self modifyClientInfoList];
			
			//also modify AppSettings.xml appropriately
#ifdef TARGET_IS_STUDIO
			[self modifyStudioAppSettings];
#endif
			
			//also register application
			FSRef clientAppRef;
			OSStatus status = FSPathMakeRef((const UInt8 *)[[self getClientApplication] fileSystemRepresentation], &clientAppRef, NULL);
			if (status == noErr)
			{
				status = LSRegisterFSRef(&clientAppRef, true);
				if (status == noErr)
					NSLog(@"Application registered");
			}
			
			// touch client application (seems to resolve dock icon issue)
			NSString *touchCommand = [NSString stringWithFormat:@"/usr/bin/touch %@",[self getClientApplication]];
			system([touchCommand UTF8String]);			
			
			[self reportStat:[NSString stringWithFormat:@"Client %@ Success", isInstalling ? @"Install" : @"Update"]];
            [self reportInstallMetricsToInfluxDb:!isInstalling result:@"Success" message:@""];
			
			//if application doesn't exist in dock then add it
			if (isInstalling && ![self applicationExistsInDock:[self getClientApplication] checkLink:NO])
			{
				[self addApplicationInDock:[self getClientApplication]];
				//delay application launch to let the creation go through
				usleep(300000);
			}
			
			installSuccess = YES;
			
			if (isInstalling)
			{
				std::wstring path = CookiesEngine::getCookiesFilePath();
				CookiesEngine engine(path);
				
				CookiesEngine::reportValue(engine, "rbx_evt_initial_install_success", "{\"os\" : \"OSX\"}");
			}
			
			//if application hasn't been handled by plugin then launch application
			if (![self isStudioInstalled] || ![self applicationExistsInDock:[self getStudioApp] checkLink: YES]) {
				NSLog(@"downloadClientApplicationDidFinish - downloadStudioBootstrapper called");
				[self downloadStudioBootstrapper];
			}else {
				//remove temporary data
				[self removeTemporaryDirectory];
				if ([self relauchRequired])
				{
                    if (isInstalling)
                    {
                        NSLog(@"downloadClientApplicationDidFinish - showInstallSuccessDialog");
                        NSMutableArray *extraArgs = [NSMutableArray array];
                        [extraArgs addObject:@"-installsuccess"];
                        [self relaunchApplication:[self getBootstrapperApplication] extraArguments:extraArgs];
                    }
                    else if ([urlHandlerArguments count] > 0)
                    {
                        NSLog(@"downloadClientApplicationDidFinish - with URL arguments");
                        [self relaunchApplication:[self getClientApplicationExecutable] extraArguments:urlHandlerArguments];
                    }
                    else
                    {
                        NSLog(@"downloadClientApplicationDidFinish - with nil arguments");
                        [self relaunchApplication:clientApplication extraArguments:nil];
                    }
				}
			}
		}
	}	
}

- (void) cancelUpdate:(id)sender 
{
	NSString *msgTitle = [NSString stringWithFormat:@"%@ in progress", isInstalling ? @"Installation" : @"Update"];
	NSString *msg      = [NSString stringWithFormat:@"%@ is %@...\nIf you cancel %@ may not work properly.", sAppName, isInstalling ? @"installing" : @"updating", sAppName];
    if (NSAlertSecondButtonReturn == [self showAlertPanel:msg messageText:msgTitle button1Text:@"Continue" button2Text:@"Cancel"])
	{
		if (isDownloadingBootstrapper)
        {
			[self reportStat:@"Bootstrapper Update User Cancelled"];
            [self reportInstallMetricsToInfluxDb:!isInstalling result:@"Cancelled" message:@"Bootstrapper User Cancelled"];
        }
		else
        {
			[self reportStat:[NSString stringWithFormat:@"Client %@ User Cancelled", isInstalling ? @"Install" : @"Update"]];
            [self reportInstallMetricsToInfluxDb:!isInstalling result:@"Cancelled" message:@"User Cancelled"];
        }
        
        // set cancelled key to true
        CFPreferencesSetAppValue((CFStringRef)sPFUpdateCancelledKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
        CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
		
		[self exitApplication];
	}
}

- (IBAction)onButtonClicked:(CustomImageButton *)sender
{
    if (sender)
    {
        if ([[sender getImageName] isEqualToString:@"btn_cancel"])
        {
            [self cancelUpdate:nil];
        }
    }
}

- (BOOL) deployClientApplication
{
	// TODO: In order to do part updates, create a struct of source, deployment destination
	
	BOOL success = NO;
	
	// unzip the zip file at temp location
	if ([self unzipFile:[self getTempPath:sClientZipFile] unzipDestination:[self getTempDownloadDir]])
	{
		NSLog(@"File %@ unzipped successfully", sClientZipFile);
		
		NSString *tempClientApp = [self getTempPath:sClientApplication];
		//make sure client application has been unzipped successfully
		if ([[NSFileManager defaultManager] fileExistsAtPath:tempClientApp])
		{
			//now copy downloaded app
			success = [self copyDirectory:sContentsFolder source:tempClientApp destination:[self getClientApplication]];
			NSLog(@"Copy Directory: %@", success ? @"Successful" : @"Unsuccessful");
		}
	}
	
	return success;
}

- (BOOL) deployBootstrapper
{
	BOOL success = NO;
	
	// unzip the zip file at temp location
	if ([self unzipFile:[self getTempBackupPath:sBootstrapperZipFile] unzipDestination:[self getTempBackupDir]])
	{
		NSLog(@"file %@ unzipped successfully", sBootstrapperZipFile);
		
		// avoid getting into a circular loop, check for the version of downloaded bootstrapper
		NSBundle *downloadedBundle = [[NSBundle alloc]initWithPath:[self getTempBackupPath:sBootstrapperApplication]];
		if (downloadedBundle != nil)
		{
			NSString *downloadedBundleVersionSTR   = [downloadedBundle objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
			NSString *serverBootstrapperVersionSTR = [self stringWithContentsOfURL:[self getURL:sBootstrapperVersionFile]];
			
			int downloadedBundleVersion = [downloadedBundleVersionSTR intValue];
			int serverBootstrapperVersion = [serverBootstrapperVersionSTR intValue];
			
			NSLog(@"downloadedBundleVersion = %@", downloadedBundleVersionSTR);
			NSLog(@"serverBootstrapperVersion = %@", serverBootstrapperVersionSTR);
			
			//server version and downloaded version should be same
			if (serverBootstrapperVersion == downloadedBundleVersion)	
			{
				NSLog(@"Attemping copyDirectory operation...");
				success = [self copyDirectory:sContentsFolder source:[self getTempBackupPath:sBootstrapperApplication] destination:[self getBootstrapperApplication]];
				
				NSLog(@"copyDirectory operation %@", success ? @"Successful" : @"Unsuccessful");				
				[self reportStat:[NSString stringWithFormat:@"Bootstrapper %@", success ? @"deployed" : @"cannot be deployed"]];
				
				//deploy plugin also
				[self deployNPAPIPlugin];					
			}
			
			[downloadedBundle release];
		}
	}
	
	return success;
}

- (BOOL) deployStudioBootstrapper
{
	BOOL success = NO;
    // unzip the zip file at temp location
	if ([self unzipFile:[self getTempPath:sStudioBootstrapperZipFile] unzipDestination:[self getTempDownloadDir]])
	{
		NSLog(@"File %@ unzipped successfully", sStudioBootstrapperZipFile);
		
		NSString *tempStudioBootstrapper = [self getTempPath:sStudioBootstrapperApplication];
		//make sure client application has been unzipped successfully
		if ([[NSFileManager defaultManager] fileExistsAtPath:tempStudioBootstrapper])
		{
			//now copy downloaded app
			success = [self copyDirectory:sStudioBootstrapperApplication source:[self getTempDownloadDir] destination:[self getStudioDirectory]];
			NSLog(@"Copy Directory: %@", success ? @"Successful" : @"Unsuccessful");
		}
	}
	return success;
}

- (BOOL) deployNPAPIPlugin
{
	CFPreferencesAppSynchronize((CFStringRef)sClientApplicationBI);
	
	//TODO: enable version to verify if we need to deploy plugin
	BOOL success = NO;
	
	//first remove an already existing plugin
	if (isAdminUser)
		[self unInstallNPAPIPlugin:sPluginAdminInstallPath];
	[self unInstallNPAPIPlugin:sPluginUserInstallPath];
	
	//now try to copy the plugin at the desired location
	NSBundle *bootstrapperBundle = [[NSBundle alloc]initWithPath:[self getBootstrapperApplication]];
	if (bootstrapperBundle != nil)
	{
		//if admin directly copy plugin to /Library/...
		if (isAdminUser) 
			success = [self copyDirectory:sNPAPIPlugin source:[bootstrapperBundle builtInPlugInsPath] destination:sPluginAdminInstallPath];
		
		//if we couldn't copy then try copying to ~/Library/...
		if (!success)
			success = [self copyDirectory:sNPAPIPlugin source:[bootstrapperBundle builtInPlugInsPath] destination:[sPluginUserInstallPath stringByExpandingTildeInPath]];
		
		[bootstrapperBundle release];
	}
	
	NSLog(@"NPAPI plugin deployment operation %@", success ? @"Successful" : @"Unsuccessful");
	[self reportStat:[NSString stringWithFormat:@"Plugin %@", success ? @"deployed" : @"cannot be deployed"]];
	
	return success;
}

- (void) unInstallNPAPIPlugin:(NSString *)pluginFolder
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *pluginPath = [NSString stringWithFormat:@"%@/%@", pluginFolder, sNPAPIPlugin];
	if ([fileManager fileExistsAtPath:pluginPath])
	{
		if (![self removeDirectory:pluginPath])
		{
			[[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation
														 source:[pluginPath stringByDeletingLastPathComponent]
													destination:@""
														  files:[NSArray arrayWithObject:[pluginPath lastPathComponent]]
															tag:NULL];
		}
	}
}

- (BOOL) applicationExistsInDock:(NSString*)path checkLink:(BOOL)checkLink
{
	//if (![path hasSuffix:@"/"])
	//	path = [path stringByAppendingString:@"/"];
	
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	[defaults addSuiteNamed:@"com.apple.dock"];
	
	NSArray      *dockApps     = [defaults objectForKey:@"persistent-apps"];
	NSEnumerator *dockAppsEnum = [dockApps objectEnumerator];
	NSDictionary *dict         = nil;
	NSString     *app          = nil;
	//NSString *path2 = [NSString stringWithFormat: @"file://localhost%@", path];
	NSLog(@"applicationExistsInDock - checking for: %@, in dock", path);
	while (dict = [dockAppsEnum nextObject] ) 
	{
		app = [[[dict objectForKey:@"tile-data"] objectForKey:@"file-data"] objectForKey:@"_CFURLString"];
		if (![app hasSuffix:@"/"])
			app = [app stringByAppendingString:@"/"];
		
		NSLog(@"applicationExistsInDock - Comparing with: %@ ", app); //Rather wordy logging, can be removed
		
		//if ([app isEqualToString:path] || [app isEqualToString:path2]) 
		//Could potentially say yes incorrectly in rare cases with multiple clients in dock
		if(app != nil && [app rangeOfString:path].location != NSNotFound) 
		{
			if (!checkLink)
			{
				NSLog(@"applicationExistsInDock - Application: %@ exists in dock - no URL Check", app);
				return YES;
			}
			else
			{
				NSLog(@"applicationExistsInDock - Application: %@ exists in dock - with URL Check", app);
				
				NSString *appPlistFile = [NSString stringWithFormat:@"%@/Contents/Info.plist", path];
				NSLog(@"applicationExistsInDock - Plist path: %@", appPlistFile);
				
				NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithContentsOfFile:appPlistFile];
				if (dict != nil) 
				{
					NSString *url;
					NSLog(@"applicationExistsInDock - Got plist content");
					url = [dict objectForKey:@"BaseServer"];
					if (url == nil)
					{
						url = [dict objectForKey:@"RbxBaseUrl"];
					}
						
					if (url != nil)
					{
						NSLog(@"applicationExistsInDock - base URL found = %@", url);	
						if ([url isEqualToString:baseServer])
						{
							NSLog(@"applicationExistsInDock - application is in doc");
							return YES;
						}
					}
				}
			}	
		}
	} 
	
	NSLog(@"Application does not exist in dock");
	
	return NO;
}

- (BOOL) addApplicationInDock:(NSString*)path
{
	BOOL success = NO;
	
	if (![path hasSuffix:@"/"])
		path = [path stringByAppendingString:@"/"];
	
	NSLog(@"Adding: %@ to dock", path);
	
	// Add the application to the Dock
	NSArray *args = [NSArray arrayWithObjects:@"write", @"com.apple.dock",@"persistent-apps",@"-array-add",[NSString stringWithFormat:@"<dict><key>tile-data</key><dict><key>file-data</key><dict><key>_CFURLString</key><string>%@</string><key>_CFURLStringType</key><integer>0</integer></dict></dict></dict>", path], nil];
	NSTask  *dockModTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/defaults" arguments:args];
	[dockModTask waitUntilExit];
	if (![dockModTask isRunning] && ([dockModTask terminationStatus] >= 0)) 
	{
		// Now restart the Dock
		NSTask *dockRestartTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/killall" arguments:[NSArray arrayWithObjects:@"-HUP", @"Dock", nil]];
		[dockRestartTask waitUntilExit];
		if (![dockRestartTask isRunning] && ([dockRestartTask terminationStatus] >= 0)) 
		{
			NSLog(@"Application added to dock and dock restarted");
			success = YES;
		}
	}
	
	return success;
}

- (void) relaunchApplication:(NSString *)appPath extraArguments:(NSArray *)extraArguments
{
	if(isMac10_5) 
	{
		[NSApp  terminate:self];
	}
	
	NSString *relaunchDaemonPath = [self getRelaunchDaemon];
	NSLog(@"Relaunch Daemon path=%@", relaunchDaemonPath);
    // need to get executable path if there are arguments
    NSBundle *bundle = [[NSBundle alloc]initWithPath:appPath];
    if (bundle)
    {
        if ([extraArguments count] > 0)
            appPath = [bundle executablePath];
        [bundle release];
    }
	NSLog(@"relaunchApplication path=%@, args=%@", appPath, extraArguments);
	
	if (relaunchDaemonPath)
	{		
		[self cleanUp];
		
		appPath=  [appPath stringByStandardizingPath];
		
		NSLog(@"Application re-launched from %@", appPath);		
		
		NSMutableArray *arguments = [NSMutableArray array];
		[arguments addObject:appPath];
		[arguments addObject:[NSString stringWithFormat:@"%d", [[NSProcessInfo processInfo] processIdentifier]]];
		
		if (extraArguments && [extraArguments count] > 0)
		{
			for (int ii = 0; ii < [extraArguments count]; ++ii)
			{
				NSLog(@"Relaunch Daemon: Extra Argument %d =%@", ii, [extraArguments objectAtIndex:ii]);
				[arguments addObject:[extraArguments objectAtIndex:ii]];
			}
		}
		
		[NSTask launchedTaskWithLaunchPath:relaunchDaemonPath arguments:arguments];
		[NSApp  terminate:self];
		
		return;
	}
	
	[self reportStat:@"ERROR: Application Relaunch Failed"];
	
	//Ideally code flow should not come here!!!!
	NSString *msgTitle = [NSString stringWithFormat: @"Restart %@", sAppName];
	NSString *msg = [NSString stringWithFormat: @"%@ has been upgraded to a new version.\nPlease restart %@.", sAppName, sAppName];
    [self showAlertPanel:msg messageText:msgTitle buttonText:@"OK"];
	[self exitApplication];
}

- (void)launchClientApplication:(NSArray *)extraArguments
{
    //
    [progressField setStringValue:[NSString stringWithFormat:@"Starting %@...", sAppName]];
    [self showProgressDialog];
    
    // create a timer so bootstrapper can be cleaned up
    semaphoreCleanupTimer = [NSTimer scheduledTimerWithTimeInterval  : 10.0
                                                             target  : self
                                                             selector: @selector(cleanupSemaphore)
                                                             userInfo: nil
                                                             repeats : NO];
    
    // create a new thread so we do not have to block main thread
    [NSThread detachNewThreadSelector : @selector(launchApplicationInNewThread:)
                             toTarget : self
                           withObject : extraArguments];
}

- (void)launchApplicationInNewThread:(NSArray*)extraArguments
{
    NSBundle *bundle = [[NSBundle alloc]initWithPath:[self getClientApplication]];
    if (bundle)
    {
        NSMutableArray *arguments = [NSMutableArray array];
        if (extraArguments && [extraArguments count] > 0)
        {
            for (int ii = 0; ii < [extraArguments count]; ++ii)
            {
                NSLog(@"launch application: Extra Argument %d =%@", ii, [extraArguments objectAtIndex:ii]);
                [arguments addObject:[extraArguments objectAtIndex:ii]];
            }
            
            // launch executable
            NSLog(@"Launching executable - %@", [bundle executablePath]);
            [NSTask launchedTaskWithLaunchPath:[bundle executablePath] arguments:arguments];
        }
        else
        {
            // launch application
            NSLog(@"Launching application - %@", [self getClientApplication]);
            [NSTask launchedTaskWithLaunchPath:[self getClientApplication] arguments:nil];
        }
        
        // make sure semaphore doesn't exist
        sem_unlink(sWaitingSemaphoreName);
        sem_t* waitingSemaphore = sem_open(sWaitingSemaphoreName, O_CREAT|O_EXCL, 0644, 0);
        if (waitingSemaphore != SEM_FAILED)
        {
            // if we are able successfully create semaphore then wait for Player to signal bootstrapper exit
            sem_wait(waitingSemaphore);
            sem_close(waitingSemaphore);
            sem_unlink(sWaitingSemaphoreName);
        }
        
        [bundle release];
    }
    else
    {
        NSLog(@"Client application bundle couldn't be located - %@", [self getClientApplication]);
    }
    
    [self exitApplication];
}

- (void) cleanupSemaphore:(NSTimer*) timer
{
    NSLog(@"cleanupSemaphore");
    sem_t *sem = sem_open(sWaitingSemaphoreName, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
    }
}

- (void) setupInfluxDb
{
    influxDbThrottle = rand() % 10000;
    
    std::string reporter = "MacPlayer";
#ifdef TARGET_IS_STUDIO
    reporter = "MacStudio";
#endif
    InfluxDb::init(reporter,
                   [self getSettings].GetValueInfluxUrl(),
                   [self getSettings].GetValueInfluxDatabase(),
                   [self getSettings].GetValueInfluxUser(),
                   [self getSettings].GetValueInfluxPassword());
    
    NSString *localBootstrapperVersionSTR = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
    InfluxDb::setAppVersion([localBootstrapperVersionSTR UTF8String]);
    
}

- (void) reportToInfluxDb:(int)throttle json:(NSString*)json
{
    if (influxDbThrottle >= throttle)
        return;
    
    if (InfluxDb::getReportingUrl().empty())
        return;
    
    NSString *url = [NSString stringWithCString:InfluxDb::getReportingUrl().c_str() encoding:[NSString defaultCStringEncoding]];
    
    NSArray *requestData = [NSArray arrayWithObjects:url, json, @"application/json", nil];
    
    //create a new thread
    [NSThread detachNewThreadSelector : @selector(doPostAsync:)
                             toTarget : [HttpRequestHandler class]
                           withObject : requestData];
}

- (void) reportInstallMetricsToInfluxDb:(BOOL)isUpdate result:(NSString*)result message:(NSString*)message
{
    InfluxDb points;
    points.addPoint("Type", isUpdate ? "Update" : "Install");
    points.addPoint("Result", [result UTF8String]);
    points.addPoint("Message", [message UTF8String]);
    points.addPoint("ElapsedTime", fabs((double)[startTime timeIntervalSinceNow]));
    std::string jsonStr = points.getJsonStringForPosting("MacInstall");
    [self reportToInfluxDb:[self getSettings].GetValueInfluxInstallHundredthsPercentage() json:[NSString stringWithCString:jsonStr.c_str() encoding:[NSString defaultCStringEncoding]]];
    
}

- (BOOL) isStudioInstalled
{
#ifdef TARGET_IS_STUDIO
	return YES;
#endif
	NSFileManager *fileManager = [NSFileManager defaultManager];
	return [fileManager fileExistsAtPath: [NSString stringWithFormat:@"%@/%@", [self getStudioDirectory], sStudioBootstrapperApplication]];
}

- (NSString *) getStudioDirectory
{
	NSString *bundlePath;
	if (isEmbedded) {
		bundlePath = [[self getClientApplication] stringByDeletingLastPathComponent];
	}else {
		bundlePath = [[self getBootstrapperApplication] stringByDeletingLastPathComponent];
	}
	return bundlePath;
}

- (NSString *) getStudioApp
{
	return [NSString stringWithFormat:@"%@/%@", [self getStudioDirectory], sStudioBootstrapperApplication];
}

- (BOOL) relauchRequired
{
    if ([urlHandlerArguments count] > 0)
        return true;
    
	BOOL launchedRequired = YES;
	CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
	CFStringRef isPluginHandling = (CFStringRef)CFPreferencesCopyAppValue((CFStringRef)sPFIsPluginHandlingKey, (CFStringRef) sPreferenceFileID);
	if (isPluginHandling)
	{
		launchedRequired = (CFStringCompare(isPluginHandling, CFSTR("true"), kCFCompareCaseInsensitive) != kCFCompareEqualTo);
		CFRelease(isPluginHandling);
	}
	return launchedRequired;
}

- (BOOL) isVersionChanged
{
	//if we are not connected to internet or version strings are same then we should not continue version checking
	NSString *localVersion  = [self getValueFromKey:sServerVersionKey];
	
	//if application already being downloaded then change the panel message
	if (localVersion)
		isInstalling = NO;
	
	if ((serverVersion == nil) || ((localVersion != nil) && [serverVersion isEqualToString:localVersion]))
		return NO;
	
	return YES;
}

- (BOOL) isBootstrapperUpdateRequired
{
	//update bootstrapper only if server bootstrapper version is greater than the local bootstrapper version
	NSString *serverBootstrapperVersionSTR = [self stringWithContentsOfURL:[self getURL:sBootstrapperVersionFile]];
	if (serverBootstrapperVersionSTR != nil)
	{		
		int serverBootstrapperVersion = [serverBootstrapperVersionSTR intValue];
		if (serverBootstrapperVersion > 0)
		{
			NSString *localBootstrapperVersionSTR = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
			if (localBootstrapperVersionSTR != nil)
			{
				int localBootstrapperVersion = [localBootstrapperVersionSTR intValue];
				if (localBootstrapperVersion > 0)
				{
					BOOL bootstrapperUpdate = serverBootstrapperVersion != localBootstrapperVersion;
					NSLog(@"Server version = %@, Local Version = %@", serverBootstrapperVersionSTR, localBootstrapperVersionSTR);
					NSLog(@"Bootstraper update %@", bootstrapperUpdate ? @"required" : @"not required");
					return bootstrapperUpdate;
				}
			}
		}
	}
	
	return NO;
}

- (BOOL) isNPAPIPluginDeploymentRequired
{
#ifdef TARGET_IS_STUDIO
	return NO;
#endif
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString   *sourceFullPath = [NSString stringWithFormat:@"%@/%@", isAdminUser ? sPluginAdminInstallPath : [sPluginUserInstallPath stringByExpandingTildeInPath],
								  sNPAPIPlugin];
	
	if ([fileManager fileExistsAtPath:sourceFullPath])
	{
		NSLog(@"Plugin deployment NOT required at %@", sourceFullPath);
		return NO;
	}
	
	NSLog(@"Plugin deployment required at %@", sourceFullPath);
	
	return YES;
}

- (BOOL) backupBootstrapper
{	
	//copy bootstrapper to temporary location so we can get it back
	NSString *bootstrapperAppName = [[[NSBundle mainBundle] bundlePath] lastPathComponent];
	NSLog(@"Bootstrapper for backup =%@", bootstrapperAppName);
	
	BOOL success = [self copyDirectory:bootstrapperAppName source:[[self getBootstrapperApplication] stringByDeletingLastPathComponent] destination:[self getTempBackupDir]];
	if (success)
	{
		if (![bootstrapperAppName isEqualToString:sBootstrapperApplication])
			success = [self renameDirectory:sBootstrapperApplication directoryPath:[NSString stringWithFormat:@"%@/%@", [self getTempBackupDir], bootstrapperAppName]];
	}
	
	return success;
}

- (BOOL) embedBootstrapper
{	
	//embed bootstrapper application
	return [self copyDirectory:sBootstrapperApplication source:[self getTempBackupDir] destination:[self getBootstrapperPathToEmbed]];
}
- (NSString *) getURL:(NSString *)fileName
{
	return [NSString stringWithFormat:@"%@%@-%@",setupServer, serverVersion, fileName];
}

- (NSString *) getStudioURL:(NSString *)fileName
{
	return [NSString stringWithFormat:@"%@%@-%@",setupServer, serverStudioVersion, fileName];
}

- (NSString *) getTempPath:(NSString *)fileName
{
	return [NSString stringWithFormat:@"%@/%@", [self getTempDownloadDir], fileName];
}

- (NSString *) getTempBackupPath:(NSString *)fileName
{
	return [NSString stringWithFormat:@"%@/%@", [self getTempBackupDir], fileName];
}

- (NSString *) getBootstrapperContentPath:(NSString *)fileName
{
	//if launched from read only file system then we must be having application in temporary location
	return [NSString stringWithFormat:@"%@/%@/%@", [self getBootstrapperApplication], sContentsFolder, fileName];
}

- (NSString *) getClientApplication
{
	//if launched from read only file system then we need to install application at the respective path
	if (launchedFromReadOnlyFS)
	{
		NSString *clientApp = [NSString stringWithFormat:@"%@/%@", isAdminUser ? sAppAdminInstallPath : [sAppUserInstallPath stringByExpandingTildeInPath], 
							   sTransformedApplication];
		if (isInstalling)
			[self createDirectory:clientApp];
		
		return clientApp;
	}
	
	//main bundle has just been transformed into client application
	if (justEmbedded)
		return [[NSBundle mainBundle] bundlePath];
	
	//bootstrapper is already embedded - so the client application will be 3 steps up!
	//Roblox.app (client application)/Contents/MacOS/Roblox.app (bootstrapper)
	return [[[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] 
			 stringByDeletingLastPathComponent] 
			stringByDeletingLastPathComponent];
}

- (NSString *) getClientApplicationExecutable
{
    NSString* clientAppExecutablePath;
    NSBundle *bootstrapperBundle = [[NSBundle alloc]initWithPath:[self getClientApplication]];
    if (bootstrapperBundle != nil)
    {
        clientAppExecutablePath = [bootstrapperBundle executablePath];
#ifndef TARGET_IS_STUDIO
        if (isInstalling)
            clientAppExecutablePath = [clientAppExecutablePath stringByReplacingOccurrencesOfString:@"MacOS/Roblox" withString:@"MacOS/RobloxPlayer"];
#endif
        [bootstrapperBundle release];
    }
    return clientAppExecutablePath;
}

- (NSString *) getBootstrapperApplication
{	
	//bootstrapper has just been embedded so get the correct path
	if (justEmbedded)
	{
		if (launchedFromReadOnlyFS)
			return [NSString stringWithFormat:@"%@/Contents/MacOS/%@", [self getClientApplication], sBootstrapperApplication];
		
		return [NSString stringWithFormat:@"%@/%@", [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent], sBootstrapperApplication];
	}
	
	//in rest of the cases it will always be the bundle path
	return [[NSBundle mainBundle] bundlePath];
}

- (NSString *) getRelaunchDaemon
{
	return [NSString stringWithFormat:@"%@/%@", [self getBootstrapperApplication], sRelaunchApplicationDaemon];
}

- (NSString *) getBootstrapperPathToEmbed
{		
	return [[self getBootstrapperApplication] stringByDeletingLastPathComponent];
}

- (NSString *) getTempDownloadDir
{
	if (tempDownloadDir == nil)
	{
		//TODO: use NSTemporaryDirectory API
		
		//tempDownloadDirectory = NSTemporaryDirectory();
		//if (tempDownloadDirectory == nil)
		NSString *tempDirectory = @"/tmp";
		NSString *userName = NSUserName();
		NSString *subDirectory = [NSString stringWithFormat:@".rbx_%@/%@/%@", userName, sDownloadDir, serverVersion];
		
		tempDownloadDir = [tempDirectory stringByAppendingPathComponent:subDirectory];
		
		NSLog(@"temp download dir = %@", tempDownloadDir);
		
		//make sure the destination folder doesn't exist before creating a new one
		//[self removeDirectory:tempDownloadDir];
		
		//create directory it it's not present already
		[self createDirectory:tempDownloadDir];
		
		[tempDownloadDir retain];
	}
	
	return tempDownloadDir;
}

- (NSString *) getTempBackupDir
{
	if (tempBackupDir == nil)
	{
		tempBackupDir = [NSString stringWithFormat:@"%@/%@", [self getTempDownloadDir], sBackupDir];
		[tempBackupDir retain];
		
		//create directory it it's not present already
		[self createDirectory:tempBackupDir];
	}
	
	return tempBackupDir;
}

// In order to make sure we have a timeout to fetch information from server, [NSString stringWithContentsOfURL] is not being used.
- (NSString *) stringWithContentsOfURL:(NSString *)urlString
{
    NSURLRequest *connectionRequest = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString] cachePolicy:NSURLRequestReloadIgnoringCacheData timeoutInterval:10.0];
	
	NSLog(@"urlString = %@", urlString);
	
	//NSString *stringContent = [NSString stringWithContentsOfURL:[[NSURL alloc] initWithString:urlString] encoding:NSASCIIStringEncoding error:NULL];
	//NSLog(@"stringContent = %@", stringContent);
	
	NSData    *receivedData = [NSURLConnection sendSynchronousRequest:connectionRequest returningResponse:NULL error:NULL];
	if (!receivedData)
		return nil;
	
	NSString *stringContent = [[[NSString alloc] initWithData:receivedData encoding:NSASCIIStringEncoding] autorelease];
	NSLog(@"stringContent = %@", stringContent);
	
	return stringContent;
}

- (void) saveUpdateInfo:(BOOL)saveVersionInfo
{	
	NSMutableDictionary *mutableDict = [NSMutableDictionary new];
	
	[mutableDict setObject:baseServer    forKey:sBaseServerKey];
	[mutableDict setObject:setupServer   forKey:sSetupServerKey];
	
	[mutableDict setObject:saveVersionInfo ? serverVersion : @"dummyVersion" forKey:sServerVersionKey];
	
	isEmbedded = justEmbedded ? justEmbedded : isEmbedded;
	if (isEmbedded)
		[mutableDict setObject:@"true"    forKey:sEmbeddedKey];
	
	NSLog(@"Dump of update info = %@", mutableDict);
	
	//save file -- will overwrite the existing one!
	[mutableDict writeToFile:[self getBootstrapperContentPath:sLocalUpdateInfoFile] atomically:NO];
	
	CFPreferencesSetAppValue((CFStringRef)sPFBaseServerKey, baseServer, (CFStringRef) sPreferenceFileID);
	CFPreferencesSetAppValue((CFStringRef)sPFInstallHostKey, [[NSURL URLWithString:setupServer] host], (CFStringRef) sPreferenceFileID);
	CFPreferencesSetAppValue((CFStringRef)sPFClientAppPathKey, [self getClientApplication], (CFStringRef) sPreferenceFileID);
	CFPreferencesSetAppValue((CFStringRef)sPFBootstrapperPathKey, [self getBootstrapperApplication], (CFStringRef) sPreferenceFileID);
	
	CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
}

- (NSString *) getValueFromKey:(NSString *)keyToSearch
{
	NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithContentsOfFile:[self getBootstrapperContentPath:sLocalUpdateInfoFile]];
	NSArray *keys = [dict allKeys];
	for (NSString *key in keys)
	{
		if ([keyToSearch isEqualToString:key])
		{
			NSObject *value = [dict objectForKey:key];
			if (value != nil)
			{
				return (NSString *)value;
			}
		}
	}
	
	return nil;
}

- (void) modifyClientInfoList
{
	NSString *clientInfoListFile = [NSString stringWithFormat:@"%@/Contents/Info.plist", [self getClientApplication]];
	
	NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithContentsOfFile:clientInfoListFile];
	
	if (dict)
	{
		//first remove keys to be added
		[dict removeObjectsForKeys:[NSArray arrayWithObjects:sRPInstallHostKey, sRPBaseUrlKey, nil]];
		
		//now add keys with new values
		[dict setObject:baseServer forKey:sRPBaseUrlKey];
		[dict setObject:[[NSURL URLWithString:setupServer] host] forKey:sRPInstallHostKey];
		
		// create the cookie store value
		if( [dict objectForKey:@"CPath" ] == nil )
		{
			NSLog( @"Added CPath for Cookie Store");
			[dict setObject:@"/tmp/rbxcsettings.rbx" forKey:@"CPath" ];
		}
		
		//save file back
		[dict writeToFile:clientInfoListFile atomically:NO]; 
	}
}

- (void) modifyStudioAppSettings
{
	NSLog(@"modifying AppSettings.xml");
	NSXMLDocument *appSettings;
	NSError *err=nil;
	NSString *fileName = [NSString stringWithFormat:@"%@/Contents/MacOS/AppSettings.xml", [self getClientApplication]];
	NSURL *furl = [NSURL fileURLWithPath: fileName];
	if (!furl) {
		NSLog(@"Can't create file path: %@", fileName);
		return;
	}
	appSettings = [[NSXMLDocument alloc] initWithContentsOfURL:furl
													   options:(NSXMLNodePreserveWhitespace|NSXMLNodePreserveCDATA)
														 error:&err];
	if (appSettings == nil) {
		NSLog(@"failed to get AppSettings.xml");
		return;
	}
	NSXMLNode *aNode = [appSettings rootElement];
	//search through xml file to find BaseUrl node
	while ((aNode = [aNode nextNode])) {
		if ([[aNode name] isEqualToString: @"BaseUrl"]) {
			[aNode setStringValue: baseServer];
			NSData *xmlData = [appSettings XMLDataWithOptions:NSXMLNodePrettyPrint];
			if(![xmlData writeToFile: fileName atomically:YES])
				NSLog(@"Failed to write to AppSettings.xml");
			return;
		}
	}
}

- (BOOL) unzipFile:(NSString *)zipFilePath unzipDestination:(NSString *)unzipDestination
{
	//TODO: Make this code more robust...!
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:zipFilePath] && [fileManager isWritableFileAtPath:unzipDestination])
	{
		// unzip the zip file at temp location
		NSString *unzipCommand = [NSString stringWithFormat:@"unzip -qo %@ -d %@", zipFilePath, unzipDestination];
		NSLog(@"Unzip command = %@", unzipCommand);
		
		system([unzipCommand UTF8String]);
		return YES;
	}
	
	NSLog(@"ERROR: Files cannot be unziped. From: %@  To: %@", zipFilePath, unzipDestination);
	
	[self reportStat:@"ERROR: Unzip Error"];
	return NO;
}

- (BOOL) copyDirectory:(NSString *)directoryName source:(NSString *)source destination:(NSString *)destination
{
	NSLog(@"copyDirectory: source = %@, destination = %@", source, destination );
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString   *sourceFullPath = [NSString stringWithFormat:@"%@/%@", source, directoryName];
	
	if ([fileManager fileExistsAtPath:sourceFullPath] && [fileManager isWritableFileAtPath:destination])
	{
		NSString *destinationFullPath = [NSString stringWithFormat:@"%@/%@", destination, directoryName];
		
		//make sure the destination folder doesn't exist before copying
		[self removeDirectory:destinationFullPath];
		
		NSLog(@"Copy Directory source =%@", sourceFullPath);
		NSLog(@"Copy Directory destination =%@", destinationFullPath);
		
		NSError* error = nil;   
		
		//now do a copy
		if ([fileManager copyItemAtPath:sourceFullPath toPath:destinationFullPath error:&error])
		{		
			NSLog(@"Directory sucessfully copied with NSFileManager");		
		}
		else if (error)
		{
			//On MACOSX 10.5 we have seen failures with copying "symbolic links" using NSFileManager Copy
			NSLog(@"ERROR: NSFileManager Copy Error = %@", error);
			NSLog(@"ERROR: NSFileManager Copy Error UserInfo: %@", [error userInfo]);
			
			[self reportStat:@"ERROR: NSFileManager Copy Error"];
			
			//Remove any Garbage due to earlier NSFileManager Copy, otherwise cp will fail
			[self removeDirectory:destinationFullPath];
			
			//Try to do a copy with UNIX cp command
			NSString *copyCommand = [NSString stringWithFormat:@"cp -R -p '%@' '%@'", sourceFullPath, destinationFullPath];
			NSLog(@"Copy command = %@", copyCommand);
			
			//launch system command -- verify success?
			system([copyCommand UTF8String]);
			
			NSLog(@"Directory sucessfully copied with UNIX cp");				
		}
		
		return YES;
	}
	
	NSLog(@"ERROR: Directory %@ cannot be copied. From: %@  To: %@", directoryName, source, destination);
	
	[self reportStat:@"ERROR: Copy Directory Error"];
	return NO;
}

- (BOOL) renameDirectory:(NSString *)directoryName directoryPath:(NSString *)directoryPath
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString      *destination = [directoryPath stringByDeletingLastPathComponent];
	
	if ([fileManager fileExistsAtPath:directoryPath] && [fileManager isWritableFileAtPath:directoryPath])
	{
		NSString *destinationFullPath = [NSString stringWithFormat:@"%@/%@", destination, directoryName];
		
		//make sure the destination folder doesn't exist before renaming
		[self removeDirectory:destinationFullPath];
		
		NSLog(@"Rename Directory source =%@", directoryPath);
		NSLog(@"Rename Directory destination =%@", destinationFullPath);
		
		//now do a move
		if ([fileManager moveItemAtPath:directoryPath toPath:destinationFullPath error:nil])
		{
			NSLog(@"Directory sucessfully renamed");
			return YES;
		}
	}
	
	NSLog(@"ERROR: Renaming directory From: %@  To: %@", directoryPath, directoryName);
	
	[self reportStat:@"ERROR: Rename Directory Error"];
	return NO;
}

- (BOOL) createDirectory:directoryPath
{
	[[NSFileManager defaultManager] createDirectoryAtPath:directoryPath withIntermediateDirectories:YES attributes:nil error:nil];
	return YES;
}

- (BOOL) removeDirectory:directoryPath
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:directoryPath])
	{
		NSLog(@"Removing Directory =%@", directoryPath);
		if([fileManager removeItemAtPath:directoryPath error:nil])
		{
			NSLog(@"Directory successfully removed = %@", directoryPath);
			return YES;
		}
		else 
		{
			NSLog(@"Remove Directory Failed = %@", directoryPath);
			return NO;
		}		
	}
	
	NSLog(@"WARNING: Remove Directory does not exist: %@", directoryPath);
	return NO;
}

- (void) removeTemporaryDirectory
{
	NSString *tempDir = [self getTempDownloadDir];
	while ([tempDir length] > 8)
	{
		[self removeDirectory:tempDir];
		tempDir = [tempDir stringByDeletingLastPathComponent];
	}
}

- (void) showProgressDialog
{
	[progressIndicator setIndeterminate:YES];
	[progressIndicator startAnimation:nil];
    [progressPanel     makeKeyWindow];
	[progressPanel     makeKeyAndOrderFront:nil];
	[progressPanel     setLevel:NSModalPanelWindowLevel];
}

- (void) showInstallSuccessDialog
{
    NSLog(@"Showing install success dialog");
    // first hide progress panel
    [progressPanel     orderOut:nil];
    
    // now load install success dialog
#ifndef TARGET_IS_STUDIO
    installSuccessController = [[InstallSuccessController alloc] initWithWindowNibName:@"InstallSuccessController"];
#else
    installSuccessController = [[InstallSuccessController alloc] initWithWindowNibName:@"StudioInstallSuccessPanel"];
#endif
    [installSuccessController.window     makeKeyWindow];
    [installSuccessController.window     makeKeyAndOrderFront:nil];
    [installSuccessController.window     setLevel:NSModalPanelWindowLevel];
    
    // listen to window close notification
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(installDialogClosing:)
                                                 name:NSWindowWillCloseNotification
                                               object:installSuccessController.window];
#ifdef TARGET_IS_STUDIO
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(developPageLinkClicked:)
                                                 name:@"developPageLinkClicked"
                                               object:installSuccessController.window];
#endif
}

- (void)installDialogClosing:(id)sender
{
#ifndef TARGET_IS_STUDIO
    NSLog(@"install dialog closing - closing application");
    [self exitApplication];
#else
    NSLog(@"install dialog closing - launching studio application");
    [self relaunchApplication:[self getClientApplication] extraArguments:nil];
#endif
}

#ifdef TARGET_IS_STUDIO
- (void)developPageLinkClicked:(id)sender
{
    NSLog(@"developPageLinkClicked");
    
    //first remove close observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    NSString *urlString = [NSString stringWithFormat: @"%@develop", [[NSBundle mainBundle] objectForInfoDictionaryKey:sBaseServerKey]];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
    [self exitApplication];
}
#endif

- (void) reportStat:(NSString *)requestId
{
    NSLog(@"ID: %@", requestId);
    
	if (!isReportStatEnabled)
		return;
	
	[self reportStat:requestId requestMessage:nil];
}

- (void) reportStat:(NSString *)requestId requestMessage:(NSString *)requestMessage
{
    NSLog(@"ID: %@, Message: %@", requestId, requestMessage);
    
    if (!isReportStatEnabled)
		return;
	
	//initialize reporting data
	NSString *urlString  = [[NSString stringWithFormat:@"%@%@%u&Type=INSTALLER MAC %@", baseServer, sAnalyticsPath, reportStatGuid, requestId] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
	NSArray *requestData = [NSArray arrayWithObjects:urlString, requestMessage, nil];
	
	NSLog(@"reportStatURL =%@", urlString);
	NSLog(@"requestMessage =%@", requestMessage);
	NSLog(@"New thread created...");
	
	//create a new thread
	[NSThread detachNewThreadSelector : @selector(doPostAsync:)
						     toTarget : [HttpRequestHandler class]
						   withObject : requestData];
}

- (void) reportClientStatusSync:(NSString*)status
{
    if ([browserTrackerId length] < 1)
        return;
    
    NSString *requestUrlString  = [[NSString stringWithFormat:@"%@/client-status/set?browserTrackerId=%@&status=%@", baseServer, browserTrackerId, status] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    
    NSLog(@"reportClientStatus: %@", requestUrlString);
    
    NSString *dummyText = @"";
    NSData   *postData = [dummyText dataUsingEncoding:NSUTF8StringEncoding];
    NSString *postLength = [NSString stringWithFormat:@"%d", [postData length]];
    
    NSMutableURLRequest *request = [[[NSMutableURLRequest alloc] init] autorelease];
    [request setURL:[NSURL URLWithString:requestUrlString]];
    [request setHTTPMethod:@"POST"];
    [request setValue:postLength forHTTPHeaderField:@"Content-Length"];
    [request setValue:@"application/xml" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPBody:postData];
    
    NSData* result = [NSURLConnection sendSynchronousRequest:request  returningResponse:nil error:nil];
    NSLog(@"reportClientStatus result: %@", result);
}


- (void) exitApplication
{
    [semaphoreCleanupTimer invalidate];
	//hide progress panel
	[progressPanel     orderOut:nil];
	
	if (isInstalling && !installSuccess)
	{
		//remove all keys, so installation can go on in the next stage
#ifndef TARGET_IS_STUDIO
		CFPreferencesSetAppValue((CFStringRef)sPFBaseServerKey,       NULL, (CFStringRef) sPreferenceFileID);
		CFPreferencesSetAppValue((CFStringRef)sPFInstallHostKey,      NULL, (CFStringRef) sPreferenceFileID);
#endif
		CFPreferencesSetAppValue((CFStringRef)sPFClientAppPathKey,    NULL, (CFStringRef) sPreferenceFileID);
		CFPreferencesSetAppValue((CFStringRef)sPFBootstrapperPathKey, NULL, (CFStringRef) sPreferenceFileID);
		
		CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
	}
	
	//modify isUptoData key, so plugin can continue working further
	CFPreferencesSetAppValue((CFStringRef)sPFIsUptoDateKey, CFSTR("true"), (CFStringRef) sPreferenceFileID);
	CFPreferencesAppSynchronize((CFStringRef) sPreferenceFileID);
	
	//sleep for some time to get the http request threads complete
	sleep(3);
	
	// now exit
	ExitToShell();
}

- (BootstrapperSettings&)getSettings
{
    if (!bootstrapperSettings)
    {
        std::string settingsURL = GetSettingsUrl([[[NSBundle mainBundle] objectForInfoDictionaryKey:sBaseServerKey] UTF8String], sSettingsGroup, "76E5A40C-3AE1-4028-9F10-7C62520BD94F");
        
        NSString* settingsData = [self stringWithContentsOfURL:[NSString stringWithCString:settingsURL.c_str() encoding:[NSString defaultCStringEncoding]]];
        
        bootstrapperSettings = new BootstrapperSettings;
        if (settingsData && [settingsData length] > 0)
            bootstrapperSettings->ReadFromStream([settingsData UTF8String]);
    }
    
    return *bootstrapperSettings;
}

- (void) validateAndFixChromeState
{
    NSLog(@"Trying to patch chrome");
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString* chromeLocalStateFile = [NSString stringWithFormat:@"%@/Google/Chrome/Local State", [paths firstObject]];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    NSLog(@"file path: '%@'", chromeLocalStateFile);
    if (![fileManager fileExistsAtPath:chromeLocalStateFile] || ![fileManager isWritableFileAtPath:chromeLocalStateFile])
    {
        NSLog(@"Cannot patch chrome: either file doesn't exist or readOnly");
        return;
    }
    
    [chromeLocalStateFile retain];
    
    std::ifstream ifs([chromeLocalStateFile UTF8String]);
    if (!ifs.is_open())
    {
        NSLog(@"Cannot patch chrome: file stream is not valid");
        return;
    }
    
    std::stringstream localStateStr;
    localStateStr << ifs.rdbuf();
    
    rapidjson::Document doc;
    doc.Parse<rapidjson::kParseDefaultFlags>(localStateStr.str().c_str());
    
    rapidjson::Value& ph = doc["protocol_handler"];
    rapidjson::Value& es = ph["excluded_schemes"];
    
    bool found = false;
    
    for (rapidjson::Value::MemberIterator  iter = es.MemberBegin(); iter != es.MemberEnd(); ++iter)
    {
        std::string name = iter->name.GetString();
        if (name.find("roblox") != std::string::npos)
        {
            if (iter->value.GetBool())
                found = true;
            
            iter->value.SetBool(false);
        }
    }
    
    if (found)
    {
        NSLog(@"Chrome upate required");
        
        // try detect chrome and prompt shutdown if detected
        while (found)
        {
            found = false;
            NSWorkspace *workSpace = [NSWorkspace sharedWorkspace];
            NSString *appPathIs = [workSpace fullPathForApplication:@"Google Chrome"];
            NSString *identifier = [[NSBundle bundleWithPath:appPathIs] bundleIdentifier];
            NSArray *selectedApps =
            [NSRunningApplication runningApplicationsWithBundleIdentifier:identifier];
            if (selectedApps.count > 0)
                found = true;
            
            if (found)
            {
                NSInteger result = [self showAlertPanel:@"Please close all instances of Google Chrome before continuing." messageText:@"Close Google Chrome" button1Text:@"Continue" button2Text:@"Cancel"];
                if (result == NSAlertSecondButtonReturn)
                {
                    result = [self showAlertPanel:@"Roblox might not launch correctly if Chrome is open during installation." messageText:@"Close Google Chrome" button1Text:@"Continue" button2Text:@"Cancel"];
                    // 2nd chance
                    if (result == NSAlertSecondButtonReturn)
                    {
                        break;
                    }
                }
            }
        }
        
        // save updated file
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        const char* output = buffer.GetString();
        std::ofstream ofs([chromeLocalStateFile UTF8String]);
        if (ofs.is_open())
            ofs << output;
    }

    NSLog(@"Done with chrome patching - %d", found);
    [chromeLocalStateFile release];
}

- (NSInteger)showAlertPanel:(NSString*)informativeText messageText:(NSString*)msgTxt buttonText:(NSString*)buttonText
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:msgTxt];
    [alert setInformativeText:informativeText];
    [alert addButtonWithTitle:buttonText];
    
    NSInteger result = [alert runModal];
    [alert release];
    
    return result;
}

// Button1 will be the right most, buttons are added right to left
- (NSInteger)showAlertPanel:(NSString*)informativeText messageText:(NSString*)msgTxt button1Text:(NSString*)button1Text button2Text:(NSString*)button2Text
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:msgTxt];
    [alert setInformativeText:informativeText];
    [alert addButtonWithTitle:button1Text];
    [alert addButtonWithTitle:button2Text];
    
    NSInteger result = [alert runModal];
    [alert release];
    
    return result;
}

@end

@implementation HttpRequestHandler

+ (void) doPostAsync:(NSArray *)requestData
{
	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];
	
	//create 
	HttpRequestHandler *requestHandler = [[self alloc] init];
	
	//initialize data
	NSURL *serverURL = [NSURL URLWithString:[requestData objectAtIndex:0]];
	NSData *postData = nil;
    NSString *contentType = @"text/plain";
	if ([requestData count] > 1)
    {
        if ([requestData objectAtIndex:1] != nil)
            postData  = [[requestData objectAtIndex:1] dataUsingEncoding:NSUTF8StringEncoding];
        if ([requestData objectAtIndex:2] != nil)
            contentType = (NSString*)[requestData objectAtIndex:2];
    }
	
	//now post the request
	[requestHandler doPost:serverURL dataToPost:postData contentType:contentType];
	
	//run loop to let the request finish
	[[NSRunLoop currentRunLoop] run];
	
	//do the required cleanup
	[requestHandler release];
	[pool           release];
}

- (void) dealloc
{
	[serverConnection release];
	[super            dealloc];
}

- (void) doPost:(NSURL *)serverURL dataToPost:(NSData *)dataToPost contentType:(NSString *)contentType
{	
	NSMutableURLRequest *postRequest = [[[NSMutableURLRequest alloc] initWithURL : serverURL 
																     cachePolicy : NSURLRequestReloadIgnoringCacheData 
															     timeoutInterval : 60.0] autorelease];
	
	[postRequest setValue:contentType forHTTPHeaderField:@"Content-Type"];
	
	//if there's no data to post then just do a get
	if (dataToPost != nil)
	{
		[postRequest setHTTPMethod:@"POST"];
		[postRequest setHTTPBody:dataToPost];
	}
	else 
	{
		[postRequest setHTTPMethod:@"GET"];
	}
	
	serverConnection = [[NSURLConnection alloc] initWithRequest:postRequest delegate:self];		
}

- (void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	NSLog(@"Connection failed with error");
}

- (void) connectionDidFinishLoading:(NSURLConnection *)connection
{
	NSLog(@"Connection finished loading");
}

@end
