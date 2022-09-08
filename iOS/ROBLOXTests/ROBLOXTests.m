//
//  ROBLOXTests.m
//  ROBLOXTests
//
//  Created by Ashish Jain on 10/27/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import <XCTest/XCTest.h>

@interface ROBLOXTests : XCTestCase

@end

@implementation ROBLOXTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
    
    NSLog(@"setup");
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
    
    NSLog(@"tearDown");
}

- (void)testExample {
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
    
    NSLog(@"testExample");
}

- (void)testPerformanceExample {
    
    NSLog(@"testPerformanceExample");
    
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
