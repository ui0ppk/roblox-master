//
//  ABTestManager.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 3/29/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "ABTestManager.h"
#import "iOSSettingsService.h"
#import "RobloxWebUtility.h"
#import "RobloxInfo.h"
#import "RBXEventReporter.h"
#import "RBXFunctions.h"

#define ENROLLMENT_STATUS_ENROLLED @"Enrolled"
#define ENROLLMENT_STATUS_DECLINED @"Declined"
#define ENROLLMENT_STATUS_INACTIVE @"Inactive"
#define ENROLLMENT_STATUS_NO_EXPERIMENT @"NoExperiment"

// Experiment Names
#define EXPERIMENT_MOBILE_GUEST_MODE @"MobileGuestMode"
#define EXPERIMENT_GUEST_FLAVOR_TEXT @"GuestFlavorText"

// Enrollment Numbers
// Note - Most tests involve an A-A-B variation style
//      so usually the behavior should be shown on variations of 2
DYNAMIC_FASTINTVARIABLE(EnrolledVariationMobileGuestMode, 2)
DYNAMIC_FASTINTVARIABLE(EnrolledVariationGuestFlavorText, 2)

@interface ABTestManager ()

@property (nonatomic, strong) NSMutableDictionary *enrollments;
@property (nonatomic, strong) NSString *browserTrackerId;

@property (nonatomic, strong) NSArray *experimentsBrowserTracker;
@property (nonatomic, strong) NSArray *experimentsUser;

@property (nonatomic) BOOL userInTestMobileGuestMode;
@property (nonatomic) BOOL userInTestGuestFlavorText;

@end

@implementation ABTestManager

+(id) sharedInstance
{
    static dispatch_once_t rbxTutorialManPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxTutorialManPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[ABTestManager alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    self = [super init];
    if (self)
    {
        #ifdef ROBLOX_DEVELOPER
        //declare all the ROBLOX DEVELOPER experiments
        self.experimentsBrowserTracker  = @[];
        self.experimentsUser            = @[];
        
        #else
        
        //declare all the ROBLOX MOBILE experiments
        self.experimentsBrowserTracker  = @[ EXPERIMENT_MOBILE_GUEST_MODE, EXPERIMENT_GUEST_FLAVOR_TEXT ];
        self.experimentsUser            = @[];
        #endif
        
        //initialize the experimental values
        self.userInTestMobileGuestMode = NO;
        self.userInTestGuestFlavorText = NO;
        
        
        //fetch a browser tracker
        self.browserTrackerId = [self getBrowserTracker];
        self.enrollments = [NSMutableDictionary dictionary];
    }
    return self;
}

#pragma mark - Accessors
-(BOOL) IsPartOfTestMobileGuestMode { return [self isPartOfTest:EXPERIMENT_MOBILE_GUEST_MODE]; }
-(BOOL) IsPartOfTestGuestFlavorMode { return [self isPartOfTest:EXPERIMENT_GUEST_FLAVOR_TEXT]; }

-(BOOL) IsInTestMobileGuestMode { return self.userInTestMobileGuestMode; }
-(BOOL) IsInTestGuestFlavorMode { return self.userInTestGuestFlavorText; }


#pragma mark -  Helper functions

-(NSString*) getBrowserTracker
{
    //there is an old browser tracker stored in the defaults from legacy code, erase it (8/12/2015)
    //TO DO - after a few releases, remove this code. Current Version : 207
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:BROWSER_TRACKER_KEY_OLD])
    {
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:BROWSER_TRACKER_KEY_OLD];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
    
    NSString* bt = [RobloxData browserTrackerId];
    if ([RBXFunctions isEmpty:bt])
        [RobloxData initializeBrowserTrackerWithCompletion:^(bool success, NSString *browserTracker) {
            if (success)
                _browserTrackerId = browserTracker;
        }];
    
    return bt;
}

-(NSDictionary*) getExperiments:(NSArray*)experiments withTargetType:(ABTestSubjectType)subject withTargetId:(id)target
{
    int expCount = experiments ? [experiments count] : 0;
    if (expCount <= 0)
        return @{};
    
    //create the post data
    int i = 0;
    NSString* postData = @"{\"enrollments\":[";
    while (i < expCount)
    {
        NSString* strExperiment = [NSString stringWithFormat:@"%@{\"ExperimentName\":\"%@\",\"SubjectType\":\"%li\",\"SubjectTargetId\":\"%@\"}",
                                   (i > 0) ? @"," : @"",
                                   experiments[i],
                                   (long)subject,
                                   target];
        postData = [postData stringByAppendingString:strExperiment];
        i++;
    }
    postData = [postData stringByAppendingString:@"]}"];
    
    return [RobloxData enrollInABTests:postData];
}

-(NSDictionary*) getBrowserTrackerExperiments
{
    if ([RBXFunctions isEmpty:self.browserTrackerId])
        return @{};
    
    return [self getExperiments:self.experimentsBrowserTracker withTargetType:ABTestSubjectTypeBrowserTracker withTargetId:self.browserTrackerId];
}

-(NSDictionary*) getUserExperiments:(NSNumber*)userId
{
    if (!userId)
        return @{};
    
    return [self getExperiments:self.experimentsUser withTargetType:ABTestSubjectTypeUser withTargetId:userId];
}


-(void) fetchExperimentsForBrowserTracker
{
    if (self.experimentsBrowserTracker.count <= 0)
        return;
    
    [self.enrollments setValuesForKeysWithDictionary:[self getBrowserTrackerExperiments]];
    
    [self checkEnrollment];
}

-(void) fetchExperimentsForUser:(NSNumber*)userId
{
    //this function gets called when the user logs in
    if (self.experimentsUser.count <= 0 || userId == nil)
        return;
    
    //clear out any of of the old experiments from the last user
    for (int i = 0; i < self.experimentsUser.count; i++)
        if (self.enrollments[self.experimentsUser[i]])
            [self.enrollments setNilValueForKey:self.experimentsUser[i]];
    
    //add the experiments for the new user
    [self.enrollments setValuesForKeysWithDictionary:[self getUserExperiments:userId]];
    
    [self checkEnrollment];
}

#pragma mark - Tests
-(void) checkEnrollment
{
    //call all checks here
    [self checkTestMobileGuestMode];
    [self checkTestGuestFlavorText];
    

    //----------- DEBUG -------------
    NSLog(@"");
    NSLog(@"=== AB TEST ENROLLMENTS ===");
    NSLog(@" - Mobile Guest Mode : %@", self.userInTestMobileGuestMode ? @"Enrolled" : @"Not Enrolled");
    NSLog(@" - Guest Flavor Text : %@", self.userInTestGuestFlavorText ? @"Enrolled" : @"Not Enrolled");
    NSLog(@"");
}
-(BOOL) isPartOfTest:(NSString*)testName
{
    RBXABTest* test = (RBXABTest*)self.enrollments[testName];
    if ([test isKindOfClass:[RBXABTest class]])
    {
        return ([test.status isEqualToString:ENROLLMENT_STATUS_ENROLLED]);
    }
    
    return NO;
}
-(BOOL) isEnrolled:(RBXABTest*)test inVariation:(int)variation
{
    if (![RBXFunctions isEmpty:test] && [test isKindOfClass:[RBXABTest class]])
    {
        //first check if the test is active for everyone
        if (test.isLockedOn)
            return YES;
            
        //Activate the AB Test behavior if the user is both enrolled in the test and in the correct test variation
        BOOL isEnrolled = ([test.status isEqualToString:ENROLLMENT_STATUS_ENROLLED]);
        BOOL isInVariation = (test.variation == variation);
        
        return (isInVariation && isEnrolled);
    }
    
    return NO;
}
-(void) saveTestStatus:(RBXABTest*)test
{
    // Store the returned test data to NSUserDefaults
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:test];
    [[NSUserDefaults standardUserDefaults] setObject:data forKey:[self keyForTestWithName:test.experimentName]];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSString *) keyForTestWithName:(NSString *)testName
{
    NSString *experimentKey = [NSString stringWithFormat:@"%@_KEY", testName];
    return experimentKey;
}

-(void) checkTestMobileGuestMode
{
    //This AB Test is designed to remove the Welcome Screen.
    //When the user starts the app or logs out, they are taken to the Games Page.
    
    self.userInTestMobileGuestMode = NO;
    
    //check if we have disabled the test from the server
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    bool mobileGuestModeEnabled = iOSSettings->GetValueEnableABTestMobileGuestMode();
    if (!mobileGuestModeEnabled) { return; }

    //parse the result of the enrollment
    if ([RBXFunctions isEmpty:self.enrollments[EXPERIMENT_MOBILE_GUEST_MODE]])
        return;
    
    RBXABTest *experiment = self.enrollments[EXPERIMENT_MOBILE_GUEST_MODE];
    NSString *experimentName = nil;
    if ([experiment isKindOfClass:[RBXABTest class]]) {
        experimentName = experiment.experimentName;
    }
    
    if (![RBXFunctions isEmpty:[[NSUserDefaults standardUserDefaults] objectForKey:[self keyForTestWithName:experimentName]]]) {
        RBXABTest *test = [NSKeyedUnarchiver unarchiveObjectWithData:[[NSUserDefaults standardUserDefaults] objectForKey:[self keyForTestWithName:experimentName]]];
        self.userInTestMobileGuestMode = [self isEnrolled:test inVariation:DFInt::EnrolledVariationMobileGuestMode];
        return;
    }
    
    self.userInTestMobileGuestMode = [self isEnrolled:self.enrollments[EXPERIMENT_MOBILE_GUEST_MODE] inVariation:DFInt::EnrolledVariationMobileGuestMode];
    [self saveTestStatus:experiment];
}

-(void) checkTestGuestFlavorText
{
    //This AB Test is designed to improve the Guest experience.
    //When the user taps on a home
    
    self.userInTestGuestFlavorText = NO; //YES FOR DEBUG
    
    //check if we have disabled the test from the server
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    bool guestFlavorTextEnabled = iOSSettings->GetValueEnableABTestGuestFlavorText();
    if (!guestFlavorTextEnabled) { return; }
    
    //parse the result of the enrollment
    if ([RBXFunctions isEmpty:self.enrollments[EXPERIMENT_GUEST_FLAVOR_TEXT]])
        return;

    RBXABTest *experiment = self.enrollments[EXPERIMENT_GUEST_FLAVOR_TEXT];
    NSString *experimentName = nil;
    if ([experiment isKindOfClass:[RBXABTest class]]) {
        experimentName = experiment.experimentName;
    }

    if (![RBXFunctions isEmpty:[[NSUserDefaults standardUserDefaults] objectForKey:[self keyForTestWithName:experimentName]]]) {
        RBXABTest *test = [NSKeyedUnarchiver unarchiveObjectWithData:[[NSUserDefaults standardUserDefaults] objectForKey:[self keyForTestWithName:experimentName]]];
        if ([test isKindOfClass:[RBXABTest class]]) {
            if (test.isLockedOn) {
                self.userInTestGuestFlavorText = YES;
                return;
            }

            BOOL isEnrolled = ([test.status isEqualToString:ENROLLMENT_STATUS_ENROLLED]);
            BOOL isInVariation = (test.variation == DFInt::EnrolledVariationGuestFlavorText);
            
            BOOL enrollmentStatus = (isInVariation && isEnrolled);
            self.userInTestGuestFlavorText = enrollmentStatus;
            return;
        }
    }
    
    self.userInTestGuestFlavorText = [self isEnrolled:self.enrollments[EXPERIMENT_GUEST_FLAVOR_TEXT] inVariation:DFInt::EnrolledVariationGuestFlavorText];
}

- (void) removeStoredExperiments
{
    for (NSString *experimentName in self.experimentsBrowserTracker) {
        RBXABTest *test = [NSKeyedUnarchiver unarchiveObjectWithData:[[NSUserDefaults standardUserDefaults] objectForKey:[self keyForTestWithName:experimentName]]];
        if ([test isKindOfClass:[RBXABTest class]]) {
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:[self keyForTestWithName:experimentName]];
        }
    }
    
    [[NSUserDefaults standardUserDefaults] synchronize];
}

@end
