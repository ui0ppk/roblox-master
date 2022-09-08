//
//  TcpViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/17/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TcpViewController : UIViewController <NSStreamDelegate>
{
    NSInputStream* inputStream;
    NSOutputStream* outputStream;
    
    NSString* host;
}

-(void) writeToOutputStream:(NSString*) str;
-(NSArray*) convertStreamToArray;

-(BOOL) listenForActions:(NSString*) host;

-(void) resetStreams;
-(void) resetStreamsOnMainThread;
-(void) tryToDestroyStreams;

@end
