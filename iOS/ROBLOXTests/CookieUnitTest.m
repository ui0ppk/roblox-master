//
//  CookieUnitTest.m
//  RobloxMobile
//
//  Created by Ashish Jain on 10/27/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import <XCTest/XCTest.h>

#include <sstream>
#include "util/Http.h"
#include "util/HttpPlatformImpl.h"
#include "util/FileSystem.h"
#include <curl/curl.h>

DYNAMIC_LOGGROUP(HttpTrace)

@interface CookieUnitTest : XCTestCase

typedef void(^AnalyzeBrowserTrackerBlock)(int);

@property (nonatomic, strong) __block NSString *initialBtId;
@property (nonatomic, copy) AnalyzeBrowserTrackerBlock analyzeBrowserTrackerBlock;
@property (nonatomic, strong) NSString *RBXEventTrackerV2Value;

@property (nonatomic, strong) NSMutableDictionary *oreoDictionary;

@end

@implementation CookieUnitTest

CURL *curl;

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
    
    NSLog(@"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    NSLog(@"");
    NSLog(@"%@ SetUp", self.name);
    
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
    
    NSLog(@"");
    NSLog(@"%@ TearDown", self.name);
    NSLog(@"==========================================================");
}

- (void) testBrowserTrackerCookieName {
    NSLog(@"testBrowserTrackerCookieName");

    XCTestExpectation *expectation = [self expectationWithDescription:@"testBrowserTrackerCookieName"];
    
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.roblox.com"]];
    [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            
            NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookies];
            BOOL found = NO;
            
            for (NSHTTPCookie *cookie in cookies) {
                if ([cookie.name isEqualToString:@"RBXEventTrackerV2"]) {
                    found = YES;
                }
            }
            
            if (YES == found) {
                XCTAssert(YES, @"BrowserTracker cookie name is still RBXEventTrackerV2");
            } else {
                XCTAssertFalse(@"BrowserTracker cookie name is not RBXEventTrackerV2");
            }
            
            [expectation fulfill];
            
        });
    }] resume];
    
    [self waitForExpectationsWithTimeout:5.0 handler:^(NSError *error) {
        if (error) {
            NSLog(@"Timeout error: %@", error);
        }
    }];
}

- (void) testAnalyzeBrowserTracker {
    NSLog(@"testAnalyzeBrowserTracker");

    CookieUnitTest __weak *weakSelf = self;
    XCTestExpectation __block *expectation = [self expectationWithDescription:@"testAnalyzeBrowserTracker"];
    
    // Setting the analyzeBrowserTrackerBlock property needs to be set on 'weakSelf' otherwise the call
    // to XCTAssertFalse throws an error about a possible retain cycle
    weakSelf.analyzeBrowserTrackerBlock = ^(int iteration) {
        if (iteration < 5) {
            NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.roblox.com"]];
            [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                
                NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookies];
                if (cookies.count == 0) {
                    XCTAssertFalse(@"No cookies returned");
                } else {
                    for (NSHTTPCookie *cookie in cookies) {
                        if ([cookie.name isEqualToString:@"RBXEventTrackerV2"]) {
                            if (nil == weakSelf.initialBtId) {
                                NSLog(@"Initial Request Headers: %@", request.allHTTPHeaderFields);
                                // If the initial cookie is nil then we should populate it with whatever value we get back from the server
                                weakSelf.initialBtId = cookie.value;
                                break;
                            } else {
                                if (![weakSelf.initialBtId isEqualToString:cookie.value]) {
                                    NSString *failureMessage = [NSString stringWithFormat:@"BrowserTracker value changed from %@ to %@.\nRequest Headers: %@", weakSelf.initialBtId, cookie.value, request.allHTTPHeaderFields];
                                    NSLog(@"Iteration %d - Failed", iteration);
                                    XCTAssertFalse(failureMessage);
                                    [expectation fulfill];
                                    
                                    // Nil out the expectation otherwise, due to the recursive nature of this method multiple "fulfill" messages will
                                    // get sent to the same expectation which will result in a crash.  So as soon as we fulfill the result, we nil out
                                    // the expectation.
                                    expectation = nil;
                                }
                            }
                        }
                    }
                    
                    int iter = iteration + 1;
                    if (iter == 1) {
                        NSLog(@"Iteration %d - BtId initialized", iter);
                    } else {
                        NSLog(@"Iteration %d - Passed", iter);
                    }
                    
                    // Recursively call the browser tracker block with the updated iteration value
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                        self.analyzeBrowserTrackerBlock(iter);
                    });
                }
                
            }] resume];
        }
        else
        {
            XCTAssertTrue(@"Browser Tracker remained consistently the same");
            [expectation fulfill];
            
            // Nil out the expectation otherwise, due to the recursive nature of this method multiple "fulfill" messages will
            // get sent to the same expectation which will result in a crash.  So as soon as we fulfill the result, we nil out
            // the expectation.
            expectation = nil;
        }
    };
    
    // Initiate the test with a call to the completion handler initialized to iteration 0
    self.analyzeBrowserTrackerBlock(0);
    
    // This is the amount of time to wait for the asynchronous operations to complete before timing out the testcase
    [self waitForExpectationsWithTimeout:60 handler:^(NSError *error) {
        if (error) {
            NSLog(@"Timeout error: %@", error);
        }
    }];
}

@end