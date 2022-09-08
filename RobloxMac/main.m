//
//  main.m
//  SimplePlayerMac
//
//  Created by Tony on 10/26/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>

#include "Roblox.h"

#include "v8datamodel/ContentProvider.h"

FASTFLAGVARIABLE(GraphicsReportingInitErrorsToGAEnabled,true)

int main(int argc, char *argv[])
{
	// Get the Location where the Resources are installed.
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourceBundle = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char resourcePath[PATH_MAX];
	if(CFURLGetFileSystemRepresentation(resourceBundle, TRUE, (UInt8 *)resourcePath, PATH_MAX))
	{
		CFRelease(resourceBundle);
		chdir(resourcePath);
        Roblox::setArgs(resourcePath, argc > 1 ? argv[1] : "f");
    }

	// Set up pool to prevent leaks
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSUserDefaults* standardUserDefaults = [NSUserDefaults standardUserDefaults];
	
	if (standardUserDefaults) 
	{
		// Forced removal of "Special Characters menu item
		//  Has to be done before the app is loaded with the current defaults
		[standardUserDefaults setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];
		
		[standardUserDefaults synchronize];
	}
	
	[pool release];
    
    // need to get client settings before creating window
    NSString* baseUrl = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"RbxBaseUrl"];
    const char* s = [baseUrl UTF8String];
    ::SetBaseURL(s);
    
    std::string appSettings;
    FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &appSettings);
    LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, appSettings, &RBX::ClientAppSettings::singleton());
    
    int mainstat = NSApplicationMain(argc,  (const char **) argv);
    return mainstat;
}
