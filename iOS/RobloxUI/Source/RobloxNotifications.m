//
//  RobloxNotifications.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/22/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RobloxNotifications.h"

//Login and Sign Up
NSString *const RBX_NOTIFY_SIGNUP_COMPLETED = @"RBXNotificationSignUpFinished";
NSString *const RBX_NOTIFY_LOGGED_OUT = @"RBXUserInfoLoggedOutNote";
NSString *const RBX_NOTIFY_LOGIN_SUCCEEDED = @"RBXLoginSuccessfulNotifier";
NSString *const RBX_NOTIFY_LOGIN_FAILED = @"RBXLoginFailedNotifier";

//Searching
NSString *const RBX_NOTIFY_SEARCH_GAMES = @"RBXNotificationSearchGames";
NSString *const RBX_NOTIFY_SEARCH_USERS = @"RBXNotificationSearchUsers";
NSString *const RBX_NOTIFY_USER_SELECTED = @"RBXNotificationUserSelected";
NSString *const RBX_NOTIFY_GAME_SELECTED = @"RBXNotificationGameSelected";

//Games
NSString *const RBX_NOTIFY_GAME_JOINED = @"RBXNotificationGameJoined";
NSString *const RBX_NOTIFY_GAME_PURCHASED = @"RBXNotificationGamePurchased";
NSString *const RBX_NOTIFY_GAME_ITEMS_UPDATED = @"RBXNotificationGameItemsUpdated";
NSString *const RBX_NOTIFY_DID_LEAVE_GAME = @"RBXNotificationDidLeaveGame";

//Messages and System Notifications
NSString *const RBX_NOTIFY_FRIENDS_UPDATED = @"RBXNotificationFriendsUpdated";
NSString *const RBX_NOTIFY_FOLLOWING_UPDATED = @"RBXNotificationsFollowingUpdated";
NSString *const RBX_NOTIFY_READ_MESSAGE = @"RBXNotificationReadMessage";
NSString *const RBX_NOTIFY_INBOX_UPDATED = @"RBXNotificationUpdatedInbox";
NSString *const RBX_NOTIFY_FRIEND_REQUESTS_UPDATED = @"RBXNotificationUpdatedFriendsRequests";
NSString *const RBX_NOTIFY_NEW_MESSAGES_TOTAL = @"RBXNotificationNewMessagesTotal";
NSString *const RBX_NOTIFY_NEW_FRIEND_REQUESTS_TOTAL = @"RBXNotificationNewFriendsRequestTotal";

//PlaceLauncher
NSString *const RBX_NOTIFY_GAME_DID_LEAVE = @"RBXDidLeaveGameNotification";
NSString *const RBX_NOTIFY_GAME_START_LEAVING = @"RBXStartLeavingGameNotification";
NSString *const RBX_NOTIFY_GAME_FINISHED_LOADING = @"RBXGameFinishedLoadingNotification";
NSString *const RBX_NOTIFY_GAME_BOUND_VIEWS = @"RBXGameBoundViewsNotification";

//Other
NSString *const RBX_NOTIFY_ROBUX_UPDATED = @"RBXNotificationRobuxUpdated";
NSString *const RBX_NOTIFY_FFLAGS_UPDATED = @"RBXNotificationFastFlagsUpdated";