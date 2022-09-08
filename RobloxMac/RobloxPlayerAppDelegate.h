//
//  SimplePlayerMacAppDelegate.h
//  SimplePlayerMac
//
//  Created by Tony on 10/26/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//
#pragma once

#import <Cocoa/Cocoa.h>
#import <Breakpad.h>

#import "RobloxOgreView.h"

#include "rbx/signal.h"
#import "RbxWebView.h"
#import "RBXWindow.h"

class RobloxView;

namespace RBX
{
    class DataModel;
}

@interface RobloxPlayerAppDelegate : NSResponder <NSWindowDelegate>
{
    RBXWindow *window;
	NSView *mainView;
    RbxWebView *rbxWebView;
	RobloxOgreView *ogreView;
	RobloxView *robloxView;
	NSMenu *mainMenu;
	BOOL running;
	BOOL quitOnLeave;
	RobloxAppShutdownCode shutdownCode;
	BreakpadRef breakpad;
    rbx::signals::scoped_connection openUrlConnection;
    rbx::signals::scoped_connection closeUrlConnection;
    RobloxGameType gameType;
}

-(id) init;

- (BOOL) checkUpdater:(BOOL) showUpdateOptionsDialog;

-(void)marshallFunction:(NSEvent *)evt;

-(void)addLogFile:(NSString *)file;
-(void)addBreakPadKeyValue:(NSString *)breakPadKey withValue:(NSString *)value;
-(void)addDbgInfoToBreakPad;

-(void)applicationDidFinishLaunching:(NSNotification *)aNotification;
-(NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;
-(void)applicationWillTerminate:(NSNotification *)aNotification;

-(void)windowWillClose:(NSNotification *)notification;

-(void)ShutdownDataModel;
-(void)leaveGame;
-(void)handleLeaveGame;
-(BOOL)requestShutdownClient:(RobloxAppShutdownCode)code;
-(void)handleShutdownClient;

-(void)handleToggleFullScreen;
-(bool)inFullScreenMode;

-(void)teleport:(NSString *)ticket withAuthentication:(NSString *)url withScript:(NSString *)script;
-(void)StartGame:(NSString *)ticket withAuthentication:(NSString *)url withScript:(NSString *)script;

bool requestPlaceInfo(int testPlaceID, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl);
RequestPlaceInfoResult requestPlaceInfo(const std::string& placeLauncherUrl, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl);

-(void) openUrlWindow:(std::string) url;

- (IBAction)crash:(id)sender;

-(void) setupGameServices;
-(void) setupDataModelServices:(RBX::DataModel*) dataModel;

+(void) reportRenderViewInitError:(const char *)message;

@property (assign) IBOutlet RBXWindow *window;
@property (assign) IBOutlet NSView *mainView;

@property (readwrite, assign) RobloxView *robloxView;
@property (assign) IBOutlet RobloxOgreView *ogreView;
@property (assign) IBOutlet NSMenu *mainMenu;

@end
