//
//  NotificationHelper.m
//  RobloxMobile
//
//  Created by Ganesh on 5/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "NotificationHelper.h"
#import "RobloxWebUtility.h"
#import <UIKit/UIKit.h>

@implementation NotificationHelper

+(id) sharedInstance {
    static dispatch_once_t onceToken = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&onceToken, ^{
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    if(self = [super init])
    {
    }
    
    return self;
}

- (void)scheduleLocalNotification:(int)userId withAlertID:(int) alertID withAlertMessage:(std::string) alertMessage showInMinutesFromNow:(int) minutesFromNow
{
#ifdef STANDALONE_NOTIFICATION
    UILocalNotification *localNotification = [[[UILocalNotification alloc] init] autorelease];
    NSDate *currentDate = [NSDate date];
    NSDate *dateToFire = [currentDate dateByAddingTimeInterval:60*minutesFromNow];
    localNotification.fireDate = dateToFire;
    
    
    localNotification.alertBody = [NSString stringWithFormat:@"%s", alertMessage.c_str()];
    localNotification.soundName = UILocalNotificationDefaultSoundName;
    localNotification.applicationIconBadgeNumber = [[[UIApplication sharedApplication] scheduledLocalNotifications] count] + 1;
    //localNotification.applicationIconBadgeNumber = 1; // set the badge number to 1 similar to what other Game Apps do
    
    
    NSDictionary *infoDict = [NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithFormat:@"%d", userId], @"UserId", [NSString stringWithFormat:@"%d", alertID], @"AlertID", nil];
    localNotification.userInfo = infoDict;
    
    [[UIApplication sharedApplication] scheduleLocalNotification:localNotification];
    
    [self makeRoomForNewNotificationForUserId:userId];
#endif
    
}


-(void)cancelNotification:(int)userId withAlertID:(int) alertID
{
#ifdef STANDALONE_NOTIFICATION
    for(UILocalNotification *aNotif in [[UIApplication sharedApplication] scheduledLocalNotifications])
    {
        if([[aNotif.userInfo objectForKey:@"UserId"] isEqualToString:[NSString stringWithFormat:@"%d", userId]] && [[aNotif.userInfo objectForKey:@"AlertID"] isEqualToString:[NSString stringWithFormat:@"%d", alertID]] )
        {
            [[UIApplication sharedApplication] cancelLocalNotification:aNotif];
        }
    }
#endif
}


-(void)cancelAllNotification:(int)userId
{
#ifdef STANDALONE_NOTIFICATION
    for(UILocalNotification *aNotif in [[UIApplication sharedApplication] scheduledLocalNotifications])
    {
        if([[aNotif.userInfo objectForKey:@"UserId"] isEqualToString:[NSString stringWithFormat:@"%d", userId]])
        {
            [[UIApplication sharedApplication] cancelLocalNotification:aNotif];
        }
    }
#endif
}

-(void) handleNotification:(UILocalNotification*)notification
{
#ifdef STANDALONE_NOTIFICATION
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
    if (notification)
    {
        [self renumberBadgesOfPendingNotifications];
        // TO DO what do we need to do with this, we may later on want to launch the placeid
    }
#endif
}

// Why : http://stackoverflow.com/questions/5962054/iphone-incrementing-the-application-badge-through-a-local-notification
- (void)renumberBadgesOfPendingNotifications
{
    // clear the badge on the icon
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
    
    // first get a copy of all pending notifications (unfortunately you cannot 'modify' a pending notification)
    NSSortDescriptor * fireDateDesc = [NSSortDescriptor sortDescriptorWithKey:@"fireDate" ascending:YES];
    NSArray * pendingNotifications = [[[UIApplication sharedApplication] scheduledLocalNotifications] sortedArrayUsingDescriptors:@[fireDateDesc]];
    
    // if there are any pending notifications -> adjust their badge number
    if (pendingNotifications.count != 0)
    {
        // clear all pending notifications
        [[UIApplication sharedApplication] cancelAllLocalNotifications];
        
        // the for loop will 'restore' the pending notifications, but with corrected badge numbers
        // note : a more advanced method could 'sort' the notifications first !!!
        NSUInteger badgeNbr = 1;
        
        for (UILocalNotification *notification in pendingNotifications)
        {
            // modify the badgeNumber
            notification.applicationIconBadgeNumber = badgeNbr++;
            
            // schedule 'again'
            [[UIApplication sharedApplication] scheduleLocalNotification:notification];
        }
    }
}

-(void)getScheduledNotifications    :(int) userId
              withResumeFunction    :(boost::function<void(shared_ptr<const RBX::Reflection::ValueArray>)>) resumeFn
              withErrorFunction     :(boost::function<void(std::string)>) errFn
{
#ifdef STANDALONE_NOTIFICATION
    NSSortDescriptor * fireDateDesc = [NSSortDescriptor sortDescriptorWithKey:@"fireDate" ascending:YES];
    NSArray * pendingNotifications = [[[UIApplication sharedApplication] scheduledLocalNotifications] sortedArrayUsingDescriptors:@[fireDateDesc]];
    
    shared_ptr<RBX::Reflection::ValueArray> alertIds(rbx::make_shared<RBX::Reflection::ValueArray>());
    for (UILocalNotification *notification in pendingNotifications)
    {
        int alertId = [[notification.userInfo objectForKey:@"AlertID"] intValue];
        alertIds->push_back(alertId);
    }
    resumeFn(alertIds);
#endif
}



-(void)makeRoomForNewNotificationForUserId:(int) userId
{
    NSSortDescriptor * fireDateDesc = [NSSortDescriptor sortDescriptorWithKey:@"fireDate" ascending:YES];
    NSArray * pendingNotifications = [[[UIApplication sharedApplication] scheduledLocalNotifications] sortedArrayUsingDescriptors:@[fireDateDesc]];
    
    if (pendingNotifications.count != 0)
    {
        iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
        int maxNotificationsPerUser = iosSettings->GetValueMaxLocalNotificationsPerUserID();
        int maxNotifications = iosSettings->GetValueMaxLocalNotifications();
        
        int numNotificationsForUserId = 0;
        int numTotalNotifications = 0;
        for(UILocalNotification *aNotif in [[UIApplication sharedApplication] scheduledLocalNotifications])
        {
            ++numTotalNotifications;
            if([[aNotif.userInfo objectForKey:@"UserId"] isEqualToString:[NSString stringWithFormat:@"%d", userId]])
            {
                ++numNotificationsForUserId;
                if (numNotificationsForUserId > maxNotificationsPerUser)
                {
                    [[UIApplication sharedApplication] cancelLocalNotification:aNotif];
                    --numNotificationsForUserId;
                    --numTotalNotifications;
                }
            }
            else
            {
                if (numTotalNotifications > maxNotifications)
                {
                    [[UIApplication sharedApplication] cancelLocalNotification:aNotif];
                    --numTotalNotifications;
                }
            }
        }
    }
}



@end
