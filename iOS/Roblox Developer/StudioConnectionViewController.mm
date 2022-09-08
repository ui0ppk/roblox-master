//
//  StudioConnectionViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "StudioConnectionViewController.h"
#import "PlaceLauncher.h"
#import "StudioConnector.h"
#import "MainViewController.h"
#import "ObjectiveCUtilities.h"
#import "GameViewController.h"
#import "UIStyleConverter.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "RobloxNotifications.h"

#include "script/ScriptContext.h"

#define CORE_SCRIPT_BUFFER_SIZE_BYTES 2000000
#define CORE_SCRIPT_END_TAG @"RbxEnd"
#define CORE_SCRIPT_SOURCE_TAG @"RbxScriptSource"
#define CORE_SCRIPT_NAME_TAG @"RbxScriptName"
#define CORE_SCRIPT_DONE_TAG @"RbxCoreScriptEnd"

@interface StudioConnectionViewController ()

@end

@implementation StudioConnectionViewController

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        connectionState = STATE_NONE;
        isVisible = NO;
    }
    return self;
}

- (void) dealloc
{
    [self stopConnection];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    connectionState = STATE_CONNECTSERVER;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(gotDidLeaveGameNotification:)
                                                 name:RBX_NOTIFY_GAME_DID_LEAVE
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(gotStartLeaveGameNotification:)
                                                 name:RBX_NOTIFY_GAME_START_LEAVING
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(pairingDidEnd:)
                                                 name:[[StudioConnector sharedInstance] getPairEndedNotificationString]
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appDidEnterBackground:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appDidBecomeActive:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [UIStyleConverter convertToLabelStyle:self.connectingLabel];
    [UIStyleConverter convertToLoadingStyle:self.loadingSpinner];
    [UIStyleConverter convertToButtonBlueStyle:self.connectToStudioButton];
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToNavigationBarStyle];
    }
    
    self.connectingLabel.text = NSLocalizedString(@"RbxDevWaiting", nil);
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    RBX::ScriptContext::setAdminScriptPath("");
    allCoreScriptsData = nil;
    coreScriptRequestState = REQUEST_CORESCRIPT_EXISTS;
}

-(void) stopConnection
{
    [self tryToDestroyStreams];
    [[StudioConnector sharedInstance] stopPairing];
}

-(void) startConnection
{
    if (connectionState == STATE_CONNECTSERVER)
    {
        [[StudioConnector sharedInstance] tryToPairWithStudioUsingCurrentCode];
    }
    
    if (connectionState == STATE_CONNECTSERVER || connectionState == STATE_LAUNCHGAME)
    {
        self.connectingLabel.text = NSLocalizedString(@"RbxDevWaiting", nil);
    }
    else
    {
        [self.connectToStudioButton setHidden:NO];
        [self.loadingSpinner setHidden:YES];
        self.connectingLabel.text = NSLocalizedString(@"RbxDevPressConnect", nil);
    }
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [UIViewController attemptRotationToDeviceOrientation];
    
    [self stopConnection];
    [self startConnection];
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    isVisible = YES;
}

-(void) viewDidDisappear:(BOOL)animated
{
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    isVisible = NO;
    
    [super viewDidDisappear:animated];
}

-(void) appDidBecomeActive:(NSNotification *) aNotification
{
    if (isVisible)
    {
        [self startConnection];
    }
}

-(void) appDidEnterBackground:(NSNotification *) aNotification
{
    if (isVisible)
    {
        [self stopConnection];
    }
}

-(void) pairingDidEnd:(NSNotification *) aNotification
{
    NSString* didPair = [[aNotification userInfo] objectForKey:@"didPair"];
    if ( [didPair isEqualToString:@"true"] )
    {
        NSString* ipAddr = [[aNotification userInfo] objectForKey:@"ipString"];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            self.connectingLabel.text = NSLocalizedString(@"RbxDevWaiting", nil);
        });
        
        if (connectionState == STATE_CONNECTSERVER)
        {
            if (![self listenForActions:ipAddr])
            {
                [[StudioConnector sharedInstance] tryToPairWithStudioUsingCurrentCode];
            }
        }
    }
}

- (IBAction) connectToStudioForPlaySession:(UIButton *)sender
{
    connectionState = STATE_CONNECTSERVER;
    [self.connectToStudioButton setHidden:YES];
    [self.loadingSpinner setHidden:false];
    [[StudioConnector sharedInstance] tryToPairWithStudioUsingCurrentCode];
}

- (IBAction) closeButtonPressed:(UIBarButtonItem *)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void) gotStartLeaveGameNotification:(NSNotification *)aNotification
{
    NSString* userRequestedLeave = [[aNotification userInfo] objectForKey:@"UserRequestedLeave"];
    if ( userRequestedLeave && [userRequestedLeave isEqualToString:@"TRUE"] )
    {
        connectionState = STATE_NONE;
    }
}

- (void) gotDidLeaveGameNotification:(NSNotification *)aNotification
{
    NSString* userRequestedLeave = [[aNotification userInfo] objectForKey:@"UserRequestedLeave"];
    if ( userRequestedLeave && [userRequestedLeave isEqualToString:@"TRUE"] )
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            self.connectingLabel.text = NSLocalizedString(@"RbxDevPressConnect", nil);
        });
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            self.connectingLabel.text = NSLocalizedString(@"RbxDevWaiting", nil);
        });
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void) checkForGameShutdownSignal:(NSArray*) words
{
    if ( [words count] == 1 && [words[0] isEqualToString:@"!exit"])
    {
        if ([[PlaceLauncher sharedInstance] getIsCurrentlyPlayingGame])
            [[PlaceLauncher sharedInstance] leaveGame];
        [self resetStreamsOnMainThread];
        connectionState = STATE_CONNECTSERVER;
    }
}

-(int) getLoggedInUserId
{
    NSNumber* currentUserId = [UserInfo CurrentPlayer].userId;
    
    if (!currentUserId)
    {
        return 0;
    }
    
    if (currentUserId <= 0)
    {
        return 0;
    }
    
    return [currentUserId intValue];
}

-(void) playerAdded:(shared_ptr<RBX::Instance>) newPlayer
{
    int userId = [self getLoggedInUserId];
    
    if (userId > 0)
    {
        if (shared_ptr<RBX::Game> game = [[PlaceLauncher sharedInstance] getCurrentGame])
        {
            if(RBX::Network::Players* players = game->getDataModel()->create<RBX::Network::Players>())
            {
                if(RBX::Network::Player* localPlayer = players->getLocalPlayer())
                {
                    if ( RBX::Network::Player* rawNewPlayer = RBX::Instance::fastDynamicCast<RBX::Network::Player>(newPlayer.get()) )
                    {
                        if (localPlayer == rawNewPlayer)
                        {
                            RBX::Security::Impersonator impersonate(RBX::Security::WebService);
                            rawNewPlayer->setUserId(userId);
                        }
                    }
                }
            }
        }
    }
}

-(BOOL) checkForCoreScriptSubstitutionSignal:(NSArray*) words
{
    if ( [words count] == 2)
    {
        if( [words[0] isEqualToString:@"RbxReadyForCoreScripts"] )
        {
            return ([words[1] isEqualToString:@"true"]);
        }
    }
    
    return NO;
}

-(void) checkForGameLaunchSignal:(NSArray*) words
{
    if ( [words count] == 3 && [words[0] isEqualToString:@"RbxReadyForPlay"])
    {
        NSString* port = words[1];
        NSString* ip = words[2];
        if([port length] > 0 && [ip length] > 0 )
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                self.connectingLabel.text = NSLocalizedString(@"RbxDevFinalizing", nil);
                
                [[UIApplication sharedApplication] setStatusBarHidden:YES];
                
                [[PlaceLauncher sharedInstance] startGameLocal:[port intValue] ipAddress:ip controller:self presentGameAutomatically:YES userId:0];
                shared_ptr<RBX::Game> game = [[PlaceLauncher sharedInstance] getCurrentGame];
                
                connectionState = STATE_CONNECTSERVER;
                
                if (RBX::Network::Players* players = game->getDataModel()->create<RBX::Network::Players>())
                {
                    BOOL setUserId = NO;
                    
                    int userId = [self getLoggedInUserId];
                    
                    if (userId > 0 && players->numChildren() > 0)
                    {
                        if(RBX::Network::Player* player = players->getLocalPlayer())
                        {
                            RBX::Security::Impersonator impersonate(RBX::Security::WebService);
                            player->setUserId(userId);
                            setUserId = YES;
                        }
                    }
                    
                    if (!setUserId)
                    {
                        players->onDemandWrite()->childAddedSignal.connect( boostFuncFromSelector_1< shared_ptr<RBX::Instance> >(@selector(playerAdded:), self) );
                    }
                }
                
            });
        }
    }
}

-(BOOL)directoryAlreadyExists:(NSString *)directoryName Name:(NSString *)name
{
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSString *path = [directoryName stringByAppendingPathComponent:name];
    
    return [fileManager fileExistsAtPath:path];
}

-(void)createDirectory:(NSString *)directoryName atFilePath:(NSString *)filePath
{
    if ([self directoryAlreadyExists:filePath Name:directoryName])
    {
        return;
    }
    
    NSString *filePathAndDirectory = [filePath stringByAppendingPathComponent:directoryName];
    NSError *error;
    
    if (![[NSFileManager defaultManager] createDirectoryAtPath:filePathAndDirectory
                                   withIntermediateDirectories:NO
                                                    attributes:nil
                                                         error:&error])
    {
        NSLog(@"Create directory error: %@", error);
    }
}

-(void) createFile:(NSString*) filePath fileData:(NSString*) fileData
{
    NSString* fileDirectory = [filePath stringByDeletingLastPathComponent];
    
    if ( ![[NSFileManager defaultManager] fileExistsAtPath:fileDirectory] )
    {
        NSError *dirError;
        if (![[NSFileManager defaultManager] createDirectoryAtPath:fileDirectory
                                       withIntermediateDirectories:NO
                                                        attributes:nil
                                                             error:&dirError])
        {
            NSLog(@"Create directory error: %@", dirError);
        }
    }
    
    NSError *error;
    if ( ![fileData writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&error])
    {
        NSLog(@"Write to file error: %@", error);
    }
}

-(void) convertStreamToCoreScripts
{
    @synchronized(allCoreScriptsData)
    {
        if ([inputStream hasBytesAvailable])
        {
            uint8_t *buffer = NULL;
            buffer = (uint8_t *) calloc(CORE_SCRIPT_BUFFER_SIZE_BYTES, sizeof(uint8_t));
            
            int len = [inputStream read:buffer maxLength:(sizeof(uint8_t) * CORE_SCRIPT_BUFFER_SIZE_BYTES)];
            if (len > 0)
            {
                NSString *output = [[NSString alloc] initWithBytes:buffer length:len encoding:NSASCIIStringEncoding];
                
                if (allCoreScriptsData == nil)
                {
                    allCoreScriptsData = [[NSMutableString alloc] initWithString:output];
                }
                else if (output != nil)
                {
                    [allCoreScriptsData appendString:[output copy]];
                }
            }
            
            free(buffer);
        }

        if (allCoreScriptsData && ([allCoreScriptsData rangeOfString:CORE_SCRIPT_DONE_TAG].location != NSNotFound))
        {
            NSMutableDictionary* coreScriptDictionary = [[NSMutableDictionary alloc] init];

            NSArray *scripts = [allCoreScriptsData componentsSeparatedByString:CORE_SCRIPT_NAME_TAG];
            for (NSString* __strong script in scripts)
            {
                script = [script stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceCharacterSet]];
                
                NSRange scriptNameRange = [script rangeOfString:CORE_SCRIPT_END_TAG];
                if (scriptNameRange.location != NSNotFound)
                {
                    NSString* scriptName = [script substringWithRange:NSMakeRange(0,scriptNameRange.location)];
                    BOOL hasDoneTag = ([script rangeOfString:CORE_SCRIPT_DONE_TAG].location != NSNotFound);
                    
                    NSRange scriptSourceStartRange = [script rangeOfString:CORE_SCRIPT_SOURCE_TAG];
                    
                    int endOffset = 0;
                    if (hasDoneTag)
                    {
                        endOffset = [CORE_SCRIPT_DONE_TAG length] + [CORE_SCRIPT_END_TAG length] + 1;
                    }
                    else
                    {
                        endOffset = [CORE_SCRIPT_END_TAG length];
                    }
                    
                    NSString* source = [script substringWithRange:NSMakeRange(scriptSourceStartRange.location + [CORE_SCRIPT_SOURCE_TAG length],
                                                                              [script length] - endOffset - [CORE_SCRIPT_SOURCE_TAG length] - scriptSourceStartRange.location)];

                    [coreScriptDictionary setObject:source forKey:scriptName];
                }
            }

            allCoreScriptsData = nil;
            
            NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
            
            [self createDirectory:@"CoreScripts" atFilePath:documentsDirectory];
            
            for (NSString* __strong scriptName in coreScriptDictionary)
            {
                NSString* scriptSource = [coreScriptDictionary objectForKey:scriptName];
                
                scriptName = [scriptName stringByAppendingString:@".lua"];
                // replace any windows style directory stuff with more unix friendly version
                scriptName = [scriptName stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
                [self createFile:[documentsDirectory stringByAppendingPathComponent:scriptName] fileData:scriptSource];
            }
            
            RBX::ScriptContext::setAdminScriptPath([documentsDirectory UTF8String]);
            
            coreScriptRequestState = REQUEST_OVER;
        }
    }
}

- (void)stream:(NSStream *)theStream handleEvent:(NSStreamEvent)streamEvent
{
    switch (streamEvent)
    {
        case NSStreamEventOpenCompleted:
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                self.connectingLabel.text = NSLocalizedString(@"RbxDevStudioFound", nil);
            });
            break;
        }
        case NSStreamEventHasSpaceAvailable:
        {
            if(theStream == outputStream && (connectionState != STATE_LAUNCHGAME) && ![[PlaceLauncher sharedInstance] getIsCurrentlyPlayingGame])
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    self.connectingLabel.text = NSLocalizedString(@"RbxDevConnectionCreated", nil);
                });
                
                if (coreScriptRequestState == REQUEST_CORESCRIPT_EXISTS)
                {
                    coreScriptRequestState = REQUEST_CORESCRIPT_WAITING_RESPONSE;
                    [self writeToOutputStream:@"RbxReadyForCoreScripts"];
                }
                else if (coreScriptRequestState == REQUEST_CORESCRIPT_NEXT_STREAM)
                {
                    coreScriptRequestState = REQUEST_CORESCRIPT_EXPECT_NOW;
                }
            }
            break;
        }
        case NSStreamEventHasBytesAvailable:
            if (theStream == inputStream)
            {
                if (coreScriptRequestState == REQUEST_CORESCRIPT_EXPECT_NOW)
                {
                    [self convertStreamToCoreScripts];
                    if (coreScriptRequestState == REQUEST_OVER)
                    {
                        connectionState = STATE_LAUNCHGAME;
                        [self writeToOutputStream:@"RbxReadyForPlay"];
                    }
                }
                else
                {
                    NSArray* words = [self convertStreamToArray];
                    if(words)
                    {
                        if ( coreScriptRequestState == REQUEST_CORESCRIPT_WAITING_RESPONSE )
                        {
                            if ([self checkForCoreScriptSubstitutionSignal:words])
                            {
                                coreScriptRequestState = REQUEST_CORESCRIPT_NEXT_STREAM;
                                [self writeToOutputStream:@"RbxSendCoreScripts"];
                            }
                            else
                            {
                                coreScriptRequestState = REQUEST_OVER;
                                connectionState = STATE_LAUNCHGAME;
                                [self writeToOutputStream:@"RbxReadyForPlay"];
                            }
                        }
                        else if (coreScriptRequestState == REQUEST_OVER)
                        {
                            if ( ![[PlaceLauncher sharedInstance] getIsCurrentlyPlayingGame] )
                            {
                                [self checkForGameLaunchSignal:words];
                            }
                            else
                            {
                                [self checkForGameShutdownSignal:words];
                            }
                        }
                    }
                }
            }
            break;
        case NSStreamEventEndEncountered:
            if ([[PlaceLauncher sharedInstance] getIsCurrentlyPlayingGame])
                [[PlaceLauncher sharedInstance] leaveGame];
            break;
        case NSStreamEventErrorOccurred:
            [self resetStreams];
            connectionState = STATE_NONE;
            break;
        default:
            break;
    }
}

@end
