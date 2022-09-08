//
//  StudioConnectionViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "TcpViewController.h"
#import "rbx/signal.h"

typedef enum ConnectionState
{
    STATE_NONE, // Don't try to connect to anything
    STATE_CONNECTSERVER, // Try to pair to server to get ip address
    STATE_LAUNCHGAME // Use ip address to connect to game server
} ConnectionState;

typedef enum CoreScriptRequestState
{
    REQUEST_CORESCRIPT_EXISTS, // see if server has any core scripts for us
    REQUEST_CORESCRIPT_NEXT_STREAM, // Tell server to send core scripts in next request
    REQUEST_CORESCRIPT_WAITING_RESPONSE, // Waiting for server to tell us something
    REQUEST_CORESCRIPT_EXPECT_NOW, // Server is now sending over core scripts
    REQUEST_OVER // no more corescript work
} CoreScriptRequestState;

@interface StudioConnectionViewController : TcpViewController
{
    ConnectionState connectionState;
    BOOL isVisible;
    
    CoreScriptRequestState coreScriptRequestState;
    NSMutableString* allCoreScriptsData;
}

@property (retain, nonatomic) IBOutlet UILabel *connectingLabel;
@property (retain, nonatomic) IBOutlet UIButton *connectToStudioButton;
@property (retain, nonatomic) IBOutlet UIImageView *loadingSpinner;


- (IBAction) connectToStudioForPlaySession:(UIButton *)sender;
- (IBAction) closeButtonPressed:(UIBarButtonItem *)sender;

@end
