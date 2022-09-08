//
//  NotificationHelper.h
//  RobloxMobile
//
//  Created by Ganesh on 5/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UILocalNotification.h>
#include <string>
#include <boost/function.hpp>
#include "reflection/Type.h"

@interface NotificationHelper : NSObject

+ (id) sharedInstance;
-(void)scheduleLocalNotification:(int)userId withAlertID:(int) alertID withAlertMessage:(std::string) alertMessage  showInMinutesFromNow:(int) minutesFromNow;
-(void)cancelAllNotification:(int)userId;
-(void)cancelNotification:(int)userId withAlertID:(int) alertID;
-(void)handleNotification:(UILocalNotification*)notification;
-(void)getScheduledNotifications    :(int) userId
              withResumeFunction    :(boost::function<void(shared_ptr<const RBX::Reflection::ValueArray>)>) resumeFn
              withErrorFunction     :(boost::function<void(std::string)>) errFn;

@end

