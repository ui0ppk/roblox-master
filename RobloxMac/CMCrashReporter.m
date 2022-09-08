//
//  CMCrashReporter.m
//  CMCrashReporter-App
//
//  Created by Jelle De Laender on 20/01/08.
//  Copyright 2008 CodingMammoth. All rights reserved.
//  Copyright 2010 CodingMammoth. Revision. All rights reserved.
//
//  Modified by ROBLOX 2011

#import "CMCrashReporter.h"

@implementation CMCrashReporter

+ (void) submitFile:(NSString *)file
{
	NSMutableData* regData = [[NSMutableData alloc] initWithCapacity:100];
	NSString* s = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
	[regData appendData:[s dataUsingEncoding:NSASCIIStringEncoding]];
	
	NSURL *url = [NSURL URLWithString:[CMCrashReporterGlobal crashReportURL]];
	NSMutableURLRequest* post = [NSMutableURLRequest requestWithURL:url];
	[post setHTTPMethod: @"POST"];
	[post setHTTPBody:regData];
	
	NSURLResponse* response;
	NSError* error;
#warning TODO: Async
	NSData* result = [NSURLConnection sendSynchronousRequest:post returningResponse:&response error:&error];
	NSString *res = [[[NSString alloc] initWithData:result encoding:NSASCIIStringEncoding] autorelease];
	NSString *compare = [res stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
	BOOL success = ([compare isEqualToString:@"ok"]);
	if (success)
		if ([[NSFileManager defaultManager] fileExistsAtPath:file])
		{
			NSError *error = nil;
			[[NSFileManager defaultManager] removeItemAtPath:file error:&error];
		}
}

+(void)check
{
#warning TODO: Turn on CMCrashReporter
#if 0	
	NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
	if ([CMCrashReporterGlobal checkOnCrashes] && ![defaults boolForKey:@"CMCrashReporterIgnoreCrashes"]) {
		NSArray *reports = [CMCrashReporter getReports];
		if ([reports count] > 0) {
			int max = MIN([CMCrashReporterGlobal numberOfMaximumReports],[reports count]);
			
			if (max == 0) max = [reports count];
			
			for (int i = 0; i < max; i++)
				[CMCrashReporter submitFile:[reports objectAtIndex:i]];
		}
	}
#endif
}



+(NSArray *)getReports
{
	NSFileManager *fileManager = [NSFileManager defaultManager];
	
	if ([CMCrashReporterGlobal isRunningLeopard]) {
		// (Snow) Leopard format is AppName_Year_Month_Day
		NSString *file;
		NSString *path = [@"~/Library/Logs/CrashReporter/" stringByExpandingTildeInPath];
		NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:path];

		NSMutableArray *array = [NSMutableArray array];
		while (file = [dirEnum nextObject])
			if ([file hasPrefix:[CMCrashReporterGlobal appName]])
				[array addObject:[[NSString stringWithFormat:@"~/Library/Logs/CrashReporter/%@",file] stringByExpandingTildeInPath]];
		
		return array;
	} else {
		// Tiger Formet is AppName.crash.log
		NSString *path = [[NSString stringWithFormat:@"~/Library/Logs/CrashReporter/%@.crash.log",[CMCrashReporterGlobal appName]] stringByExpandingTildeInPath];
		if ([fileManager fileExistsAtPath:path]) return [NSArray arrayWithObject:path];
		else return nil;
	}
}
@end
