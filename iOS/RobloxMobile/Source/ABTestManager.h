//
//  ABTestManager.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 3/29/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RobloxData.h"

@interface ABTestManager : NSObject

//static instance
+(id)sharedInstance;

-(void) fetchExperimentsForBrowserTracker;
-(void) fetchExperimentsForUser:(NSNumber*)userId;

- (void) removeStoredExperiments;

//Accessors
//Enrollment tests
-(BOOL) IsPartOfTestMobileGuestMode;
-(BOOL) IsPartOfTestGuestFlavorMode;

//App Behavioral tests
-(BOOL) IsInTestMobileGuestMode;
-(BOOL) IsInTestGuestFlavorMode;
@end
