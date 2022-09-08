//
//  ReLaunchApplication.m
//  Roblox
//
//  Created by Dharmendra on 03/02/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


#pragma mark Main method
int main(int argc, char *argv[])
{
	//initialize required data
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSString *appPath = [NSString stringWithCString:argv[1] encoding:NSUTF8StringEncoding];
	NSLog(@"ReLaunch application path = %@",appPath);
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:appPath])
    {
        NSTask *launchTask = nil;
        if (argc > 3)
        {
            NSMutableArray *arguments = [NSMutableArray array];
            
            for (int ii=3; ii<argc; ++ii)
            {
                NSString *arg = [NSString stringWithCString:argv[ii] encoding:NSUTF8StringEncoding];
                [arguments addObject:arg];
                NSLog(@"Extra argument %d: %@", ii, arg);
            }
            
            launchTask = [[NSTask alloc] init];

            [launchTask setLaunchPath:appPath];
            [launchTask setArguments:arguments];
        }
        
        // wait for parent application to exit
        pid_t parentPID = atoi(argv[2]);
        ProcessSerialNumber psn;
        while (GetProcessForPID(parentPID, &psn) != procNotFound)
            sleep(1);
        
        // now launch parent application again
        if (launchTask != nil)
        {
            NSLog(@"Application re-launched using task");
            [launchTask launch];
        }
        else 
        {
            NSLog(@"Application re-launched using open file");
            [[NSWorkspace sharedWorkspace] openFile:[appPath stringByExpandingTildeInPath]];
        }
    }
    else
    {
        NSLog(@"Application to re-launch not found");
    }
	
	[pool drain];
	return 0;
}
