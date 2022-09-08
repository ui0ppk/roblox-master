//
//  CrashReporter.h
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 6/25/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "util/standardout.h"
#import "rbx/signal.h"

@interface CrashReporter : NSObject
{
    rbx::signals::scoped_connection messageOutConnection;
}


- (NSString*) activeCrashReporterString;
- (void) tryLogMessage:(const RBX::StandardOutMessage&) message;
- (void) logStringKeyValue:(NSString*) key withValue:(NSString*) value;
- (void) logBoolKeyValue:(NSString*) key withValue:(BOOL) value;
- (void) logIntKeyValue:(NSString*) key withValue:(int) value;
- (void) logFloatKeyValue:(NSString*) key withValue:(float) value;

+(CrashReporter*) sharedInstance;

@end




