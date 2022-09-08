//
//  CMCrashReporter.m
//  CMCrashReporter-App
//
//  Created by Jelle De Laender on 20/01/08.
//  Copyright 2008 CodingMammoth. All rights reserved.
//  Copyright 2010 CodingMammoth. Revision. All rights reserved.
//
//  Modified by ROBLOX 2011

void uploadAndDeletFileAsync(const char* url, const char* file);

#import "CMCrashReporter.h"

@implementation CMCrashReporter

+ (void) submitFile:(NSString *)file
{
	NSLog(@"submitFile: %@", file);
	if ([[NSFileManager defaultManager] fileExistsAtPath:file] == NO)
		return;
	
	NSString* base = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"RbxBaseUrl"];
	NSString* crashURL = [base stringByAppendingString:@"error/dmp.ashx"];
	uploadAndDeletFileAsync(
								 [crashURL cStringUsingEncoding:NSUTF8StringEncoding], 
								 [file cStringUsingEncoding:NSUTF8StringEncoding]
								 );
}

+(void)check
{
	NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
	if ([CMCrashReporterGlobal checkOnCrashes] && ![defaults boolForKey:@"CMCrashReporterIgnoreCrashes"]) {
		NSArray *reports = [CMCrashReporter getReports];
		if ([reports count] > 0) {
			int max = MIN([CMCrashReporterGlobal numberOfMaximumReports],[reports count]);
			
			if (max == 0) max = [reports count];
			
			int i;
			for (i = 0; i < max; i++)
				[CMCrashReporter submitFile:[reports objectAtIndex:i]];
		}
	}
}



+(NSArray *)getReports
{
    // (Snow) Leopard format is AppName_Year_Month_Day
	NSString *prefix = @"RobloxPlayer";
	NSMutableArray *array = [NSMutableArray array];
	
	{
		NSString *path = [@"~/Library/Logs/CrashReporter/" stringByExpandingTildeInPath];
		NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:path];
			
		NSString *file;
		while (file = [dirEnum nextObject])
			if ([file hasPrefix:prefix])
				[array addObject:[[NSString stringWithFormat:@"~/Library/Logs/CrashReporter/%@",file] stringByExpandingTildeInPath]];
	}
	
	if (false)
	{
		NSString *path = [@"~/Library/Logs/DiagnosticReports/" stringByExpandingTildeInPath];
		NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:path];
		
		NSString *file;
		while (file = [dirEnum nextObject])
			if ([file hasPrefix:prefix])
				[array addObject:[[NSString stringWithFormat:@"~/Library/Logs/DiagnosticReports/%@",file] stringByExpandingTildeInPath]];
	}
	
	return array;
}

@end
