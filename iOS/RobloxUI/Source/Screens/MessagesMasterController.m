//
//  MessagesScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "MessagesMasterController.h"
#import "MessagesDetailController.h"
#import "NotificationsDetailController.h"
#import "FriendsRequestsDetailController.h"
#import "RobloxTheme.h"
#import "RobloxData.h"
#import "UIViewController+Helpers.h"
#import "Flurry.h"
#import "RobloxNotifications.h"
#import "RBXMessagesPollingService.h"
#import "UITabBarItem+CustomBadge.h"


//---METRICS---
#define MMC_inboxSelected           @"MESSAGES MASTER SCREEN - Inbox Selected"
#define MMC_notificationsSelected   @"MESSAGES MASTER SCREEN - Notifications Selected"
#define MMC_friendRequestsSelected  @"MESSAGES MASTER SCREEN - Friend Requests Selected"
#define MMC_openRobux               @"MESSAGES MASTER SCREEN - Open Robux"
#define MMC_openBuildersClub        @"MESSAGES MASTER SCREEN - Open Builders Club"
#define MMC_openSettings            @"MESSAGES MASTER SCREEN - Open Settings"
#define MMC_openLogout              @"MESSAGES MASTER SCREEN - Open Logout"

@interface MessagesMasterController ()

@end

@implementation MessagesMasterController
{
    IBOutlet UISegmentedControl* _messageTypeSelector;
    
    MessagesDetailController* _messagesController;
    MessagesDetailController* _messagesSentController;
    MessagesDetailController* _messagesArchiveController;
    NotificationsDetailController* _notificationsController;
    //FriendsRequestsDetailController* _friendsRequestsController;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        [self initNotificationPolling];
    }
    return self;
}
- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self initNotificationPolling];
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self setViewTheme:RBXThemeSocial];
    
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    [_messageTypeSelector removeAllSegments];
    
    // Inbox
    {
        [_messageTypeSelector insertSegmentWithTitle:NSLocalizedString(@"InboxWord", nil) atIndex:_messageTypeSelector.numberOfSegments animated:NO];
        
        _messagesController = [self.storyboard instantiateViewControllerWithIdentifier:@"MessagesDetailController"];
        _messagesController.view.hidden = YES;
        _messagesController.typeOfMessages = RBXMessageTypeInbox;
        [self addChildViewController:_messagesController];
        [self.view addSubview:_messagesController.view];
    }
    
    //Sent Messages
    {
        [_messageTypeSelector insertSegmentWithTitle:NSLocalizedString(@"SentMessagesWord", nil) atIndex:_messageTypeSelector.numberOfSegments animated:NO];
        
        _messagesSentController = [self.storyboard instantiateViewControllerWithIdentifier:@"MessagesDetailController"];
        _messagesSentController.view.hidden = YES;
        _messagesSentController.typeOfMessages = RBXMessageTypeSent;
        [self addChildViewController:_messagesSentController];
        [self.view addSubview:_messagesSentController.view];
    }
    
    //Archive
    {
        [_messageTypeSelector insertSegmentWithTitle:NSLocalizedString(@"ArchivedMessagesWord", nil) atIndex:_messageTypeSelector.numberOfSegments animated:NO];
        
        _messagesArchiveController = [self.storyboard instantiateViewControllerWithIdentifier:@"MessagesDetailController"];
        _messagesArchiveController.view.hidden = YES;
        _messagesArchiveController.typeOfMessages = RBXMessageTypeArchive;
        [self addChildViewController:_messagesArchiveController];
        [self.view addSubview:_messagesArchiveController.view];
    }

    // Notifications
    {
        [_messageTypeSelector insertSegmentWithTitle:NSLocalizedString(@"NotificationsWord", nil) atIndex:_messageTypeSelector.numberOfSegments animated:NO];
        
        _notificationsController = [self.storyboard instantiateViewControllerWithIdentifier:@"NotificationsDetailController"];
        _notificationsController.view.hidden = YES;
        [self addChildViewController:_notificationsController];
        [self.view addSubview:_notificationsController.view];
    }
    
    // Friends requests
//    {
//        [_messageTypeSelector insertSegmentWithTitle:NSLocalizedString(@"FriendRequestsPhrase", nil) atIndex:_messageTypeSelector.numberOfSegments animated:NO];
//        
//        _friendsRequestsController = [self.storyboard instantiateViewControllerWithIdentifier:@"FriendsRequestsDetailController"];
//        _friendsRequestsController.view.hidden = YES;
//        [self addChildViewController:_friendsRequestsController];
//        [self.view addSubview:_friendsRequestsController.view];
//    }
    
    [_messageTypeSelector setSelectedSegmentIndex:0];
    [self showDetailControllerForIndex:0];
    
    
    // Add navigation items
    [self addRobuxIconWithFlurryEvent:MMC_openRobux
             andBCIconWithFlurryEvent:MMC_openBuildersClub];
    
    //[self.navigationController.tabBarItem setImage:[[UIImage imageNamed:@"Icon Messages Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
    //[self.navigationController.tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Messages On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
    
}

- (void)viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeSocial];
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextMessages];
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    CGRect rect = self.view.bounds;
    _messagesController.view.frame = rect;
    _notificationsController.view.frame = rect;
    //_friendsRequestsController.view.frame = rect;
}

- (IBAction)timeFilterSelected:(id)sender
{
    [self showDetailControllerForIndex:_messageTypeSelector.selectedSegmentIndex];
}

- (void) showDetailControllerForIndex:(NSInteger)index
{
    _messagesController.view.hidden = YES;
    _messagesSentController.view.hidden = YES;
    _messagesArchiveController.view.hidden = YES;
    _notificationsController.view.hidden = YES;
    //_friendsRequestsController.view.hidden = YES;
    
    switch (index)
    {
        case 0: // Inbox
        {
            [Flurry logEvent:MMC_inboxSelected];
            _messagesController.view.hidden = NO;
            [_messagesController loadData];
            break;
        }
        case 1: //Sent
        {
            //[Flurry logEvent:MMC_inboxSelected];
            _messagesSentController.view.hidden = NO;
            [_messagesSentController loadData];
            break;
        }
        case 2: //Archived
        {
            //[Flurry logEvent:MMC_inboxSelected];
            _messagesArchiveController.view.hidden = NO;
            [_messagesArchiveController loadData];
            break;
        }
        case 3: // Notifications
        {
            [Flurry logEvent:MMC_notificationsSelected];
            _notificationsController.view.hidden = NO;
            [_notificationsController loadData];
            break;
        }
//        case 4: // Friend Requests
//        {
//            [Flurry logEvent:MMC_friendRequestsSelected];
//            _friendsRequestsController.view.hidden = NO;
//            [_friendsRequestsController loadData];
//            break;
//        }
    }
}



-(void) initNotificationPolling
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)             name:RBX_NOTIFY_NEW_MESSAGES_TOTAL object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadgeWithRefesh)   name:RBX_NOTIFY_INBOX_UPDATED     object:nil];
    [self updateBadge];
}
-(void) updateBadge
{
    dispatch_async(dispatch_get_main_queue(), ^
   {
       //dispatch the update command to the main thread for instant update
       int total = [[RBXMessagesPollingService sharedInstance] totalMessages];
       if ([self navigationController])
           if ([[self navigationController] tabBarItem])
           {
               //this should be cleaned up at some point to use a RobloxTheme constant. Roblox Theme needs to be cleaned up.
               [self.navigationController.tabBarItem setBadgeValue:((total == 0) ? nil : [NSString stringWithFormat:@"%i",total])];
               //[[[self navigationController] tabBarItem] setCustomBadgeValue:((total == 0) ? nil : [NSString stringWithFormat:@"%i",total])
               //                                                    withColor:[UIColor colorWithRed:0.2549f green:0.3882f blue:0.6f alpha:1.0f]];
           }
   });
}
-(void) updateBadgeWithRefesh
{
    //there has been an update in the total notifications, update the lists
    if (_messagesController)        [_messagesController refreshMessages:nil];
    if (_notificationsController)   [_notificationsController refreshMessages:nil];
    //if (_friendsRequestsController) [_friendsRequestsController refreshRequests:nil];
    
    [self updateBadge];
}


@end
