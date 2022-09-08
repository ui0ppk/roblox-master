//
//  main.m
//  Roblox
//
//  Created by Dharmendra on 13/01/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Controller.h"

int main(int argc, char *argv[])
{
	NSApplicationMain(argc,  (const char **) argv);
	
	/*NSAppleScript *openSharedFolderScript = [[NSAppleScript alloc] initWithSource:@"tell application \"RobloxPlayer\" \n select menu item 2 of menu 1 \n end tell"];
    [openSharedFolderScript executeAndReturnError:nil];
    [openSharedFolderScript release];
	 */
	
	return 0;
}
