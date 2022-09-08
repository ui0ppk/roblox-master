//
//  CMCrashReporterGlobal.h
//  CMCrashReporter-App
//
//  Created by Jelle De Laender on 20/01/08.
//  Copyright 2008 CodingMammoth. All rights reserved.
//  Copyright 2010 CodingMammoth. Revision. All rights reserved.

//

#import <Cocoa/Cocoa.h>


@interface CMCrashReporterGlobal : NSObject {

}

+ (NSString *)appName;
+ (NSString *)version;

+ (BOOL)checkOnCrashes;

+ (NSString *)osVersion;

+ (int)numberOfMaximumReports;
@end
