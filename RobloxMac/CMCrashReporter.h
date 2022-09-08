//
//  CMCrashReporter.h
//  CMCrashReporter-App
//
//  Created by Jelle De Laender on 20/01/08.
//  Copyright 2008 CodingMammoth. All rights reserved.
//  Copyright 2010 CodingMammoth. Revision. All rights reserved.
//
//  Current version: 1.1 (September 2010)
//

#import <Cocoa/Cocoa.h>
#import "CMCrashReporterGlobal.h"

@interface CMCrashReporter : NSObject {

}
+ (void)check;
+ (NSArray *)getReports;
@end
