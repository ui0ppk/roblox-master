//
//  RBXMessagesPollingService
//  Periodically polls the server for user notifications
//
//  Created by Kyler Mulherin on 9/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBXMessagesPollingService.h"
#import "LoginManager.h"
#import "FastLog.h"
#import "RobloxNotifications.h"

//------ Server Fast Flags ---------
FASTFLAGVARIABLE(NotificationPollingServiceEnabled, true);
FASTINTVARIABLE(NotificationPollingServiceInterval, 10);

@implementation RBXMessagesPollingService

+(id) sharedInstance
{
    static dispatch_once_t rbxNotServObj = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxNotServObj, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    
    return _sharedObject;
}
-(id) init
{
    //counter
    _totalMessages = 0;
    _totalFriendRequests = 0;
    _requestCompleted = YES;
    
    //observers
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(beginPolling)             name:RBX_NOTIFY_DID_LEAVE_GAME    object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pausePolling)             name:RBX_NOTIFY_GAME_JOINED      object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(decrementMessagesTotal)   name:RBX_NOTIFY_READ_MESSAGE     object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(pulseNotification)        name:RBX_NOTIFY_LOGIN_SUCCEEDED   object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(clearTotalMessages)       name:RBX_NOTIFY_LOGGED_OUT object:nil];
    
    //since friend requests are handled on the web, there is no way to communicate natively that the total has decremented
    
    return self;
}
-(void) clean
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self deactivateNotificationTimer];
}

//server flags
- (BOOL)isServiceEnabled
{
    return FFlag::NotificationPollingServiceEnabled;
}
- (int)getPollingInterval
{
    return FInt::NotificationPollingServiceInterval;
}



//service functions
-(void) beginPolling { [self toggleTimer:YES];}
-(void) pausePolling { [self toggleTimer:NO]; }
-(void) toggleTimer:(BOOL)isOn
{
    //use fast flags to simply escape if this service is not enabled
    if (! [self isServiceEnabled]) return;
    
    //to ensure that the timer is started and stopped on the same thread, dispatch the command here
    dispatch_async(dispatch_get_main_queue(), ^
   {
       _requestCompleted = YES;
       
       if (isOn)    [self activateNotificationTimer];
       else         [self deactivateNotificationTimer];
   });
}
-(void) activateNotificationTimer
{
    if (!_notificationTimer)
    {
        _notificationTimer = [NSTimer scheduledTimerWithTimeInterval:[self getPollingInterval]
                                                              target:self
                                                            selector:@selector(fetchNotificationsFromServer)
                                                            userInfo:nil
                                                             repeats:YES];
    }
}
-(void) deactivateNotificationTimer
{
    if (_notificationTimer)
        [_notificationTimer invalidate];
    
    _notificationTimer = nil;
}


//count and total functions
-(NSDictionary*) createNotification
{
    return [NSDictionary dictionaryWithObjects:@[[NSNumber numberWithInt:_totalMessages],[NSNumber numberWithInt:_totalFriendRequests]]
                                       forKeys:@[@"unreadMessages", @"friendRequests"]];
}
-(void) fetchNotificationsFromServer
{
    //escape if the user is not logged into an account
    if (![[UserInfo CurrentPlayer] userLoggedIn]) { return; }
    
    //do not perform multiple requests, wait for one to complete before requesting another
    if (!_requestCompleted) return;
    
    _requestCompleted = NO;
    
    [RobloxData fetchTotalNotificationsForUser:[UserInfo CurrentPlayer].userId WithCompletion:^(RBXNotifications* notifications)
     {
         _requestCompleted = YES;
         if (notifications)
         {
             if (_totalMessages != notifications.unreadMessages)
             {
                 _totalMessages = notifications.unreadMessages;
                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_INBOX_UPDATED object:nil];
             }
             if (_totalFriendRequests != notifications.unansweredFriendRequests)
             {
                 _totalFriendRequests = notifications.unansweredFriendRequests;
                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FRIEND_REQUESTS_UPDATED object:nil];
             }
         }
     }];
}
-(void) decrementMessagesTotal
{
    _totalMessages = (_totalMessages - 1 <= 0) ? 0 : _totalMessages - 1;
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_NEW_MESSAGES_TOTAL object:nil];
}
-(void) decrementFriendRequestsTotal
{
    _totalFriendRequests = (_totalFriendRequests - 1 <= 0) ? 0 : _totalFriendRequests - 1;
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_NEW_FRIEND_REQUESTS_TOTAL object:nil];
}
-(void) clearTotalMessages
{
    //the user has logged out, clear the counter and notify any listeners
    _totalMessages = 0;
    _totalFriendRequests = 0;
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_NEW_MESSAGES_TOTAL object:[self createNotification]];
}
-(void) pulseNotification
{
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_NEW_MESSAGES_TOTAL          object:[self createNotification]];
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_NEW_FRIEND_REQUESTS_TOTAL   object:[self createNotification]];
}
@end
