//
//  CMCrashReporterGlobal.m
//  CMCrashReporter-App
//
//  Created by Jelle De Laender on 20/01/08.
//  Copyright 2008 CodingMammoth. All rights reserved.
//  Copyright 2010 CodingMammoth. Revision. All rights reserved.
//
//  Modified by ROBLOX 2011


#import "CMCrashReporterGlobal.h"


@implementation CMCrashReporterGlobal

+ (NSString *)appName
{
	return [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
}

+ (NSString *)version
{
	return [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
}

+ (int)numberOfMaximumReports {
	if (! [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CMMaxReports"]) 
		return 0;
	
	return [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CMMaxReports"] intValue];
}


+ (BOOL)checkOnCrashes
{
	// Integration for later
	return YES;
}

+ (NSString *)osVersion
{
	return [[NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"]
			objectForKey:@"ProductVersion"];
}
@end
