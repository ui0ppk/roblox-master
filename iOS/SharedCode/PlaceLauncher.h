//
//  PlaceLauncher.h
//  RobloxMobile
//
//  Created by David York on 9/25/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//
#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIViewController.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>

#include "v8datamodel/Game.h"
#include "rbx/signal.h"

#include "v8tree/Instance.h"

enum JoinGameRequest {
    JOIN_GAME_REQUEST_PLACEID,
    JOIN_GAME_REQUEST_USERID,
    JOIN_GAME_REQUEST_PRIVATE_SERVER,
    JOIN_GAME_REQUEST_GAME_INSTANCE
};

class Teleporter;
class RobloxView;
@class GameViewController;

@interface RBXGameLaunchParams : NSObject
    @property int targetId;
    @property JoinGameRequest joinRequestType;
    @property (retain, nonatomic) NSString* joinRequestString;
    @property (retain, nonatomic) NSString* accessCode;
    @property (retain, nonatomic) NSString* gameInstanceId;
    
    +(RBXGameLaunchParams*) InitParamsForFollowUser:(int)userID;
    +(RBXGameLaunchParams*) InitParamsForJoinPlace:(int)placeID;
    +(RBXGameLaunchParams*) InitParamsForJoinPrivateServer:(int)placeID withAccessCode:(NSString*)vpsAccessCode;
    +(RBXGameLaunchParams*) InitParamsForJoinGameInstance:(int)placeID withInstanceID:(NSString*)instanceID;

    -(std::string) getStringJoinURL;
@end

@interface PlaceLauncher : NSObject
{
    RobloxView* rbxView;
    
    BOOL isCurrentlyPlayingGame;
    BOOL isLeavingGame;
    int lastPlaceId;
    
    boost::scoped_ptr<Teleporter> teleporter;
    BOOL hasReceivedMemoryWarning;
    
    // player join tracking
    rbx::signals::connection childConnection;
    rbx::signals::connection playerConnection;
    
    shared_ptr<RBX::Game> currentGame;
}

+ (id)sharedInstance;

// game creation/destruction functions
-(BOOL) startGameLocal:(int)portId ipAddress:(NSString*) ipAddress controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL) presentGameAutomatically userId:(int) userId;
-(BOOL) startGame:(RBXGameLaunchParams*)params controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL)presentGameAutomatically;
-(BOOL) startGameWithJoinScript:(NSString*)joinScript controller:(UIViewController*)lastNonGameController presentGameAutomatically:(BOOL) presentGameAutomatically;

-(void) injectJoinScript:(NSString*)joinUrlScript;

-(BOOL) appActive;

-(void) leaveGame;
-(void) leaveGame:(BOOL) userRequestedLeave;
-(void) leaveGameShutdown:(BOOL) userRequestedLeave;
-(void) createGame:(shared_ptr<RBX::Game>)game presentGameAutomatically:(BOOL) presentGameAutomatically;
-(void) finishGameSetup:(boost::shared_ptr<RBX::Game>)game gameViewController:(GameViewController*) gameController;
-(void) deleteRobloxView:(BOOL) resetCurrentGame;
-(void) presentGameViewController;

-(void)teleport:(NSString*)ticket withAuthentication:(NSString*)url withScript:(NSString*)script;

// iOS events
-(void) applicationDidReceiveMemoryWarning;
-(void) clearCachedContent;

-(void) disableViewBecauseGoingToBackground;
-(void) enableViewBecauseGoingToForeground;

// game setup functions
-(void) placeDidFinishLoading;
-(void) setLastNonGameController:(UIViewController*)lastNonGameController needsOnline:(BOOL) needsOnline;
-(void) handleStartGameFailure;

// game state functions
-(BOOL) getIsCurrentlyPlayingGame;

// utility functions
-(void) checkPlacePartCount;
-(void) setLastPlaceId:(int) lastId;

// player join tracking
-(void) childAdded:(shared_ptr<RBX::Instance>)child;
-(void) playerLoaded:(shared_ptr<RBX::Instance>)child;
-(void) closeChildConnections;

// current game
-(shared_ptr<RBX::Game>) getCurrentGame;

@end
