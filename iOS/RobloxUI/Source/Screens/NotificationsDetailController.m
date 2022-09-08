//
//  MessagesDetailController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/19/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "NotificationsDetailController.h"
#import "RobloxImageView.h"
#import "RobloxTheme.h"
#import "NSString+stripHtml.h"
#import <UIKit/NSAttributedString.h>
#import "Flurry.h"
#import "RobloxNotifications.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "RBXEventReporter.h"

#define AVATAR_SIZE CGSizeMake(110, 110)
#define SPINNER_ICON_FRAME CGRectMake(0, 0, 32, 32)

//---METRICS---
#define NDC_didPressNotification    @"NOTIFICATIONS DETAIL SCREEN - Notification Selected"
#define NDC_didRefreshMessages      @"NOTIFICATIONS DETAIL SCREEN - Notifications Refreshed"
#define NDC_openExternalLink        @"NOTIFICATIONS DETAIL SCREEN - Open External Link"
#define NDC_openLocalLink           @"NOTIFICATIONS DETAIL SCREEN - Open Local Link"
#define NDC_openProfile             @"NOTIFICATIONS DETAIL SCREEN - Open Profile"
#define NDC_openGameDetail          @"NOTIFICATIONS DETAIL SCREEN - Open Game Detail"
#define NDC_launchGame              @"NOTIFICATIONS DETAIL SCREEN - Launch Game"

#pragma mark -
#pragma mark Custom Cell

@implementation NotificationCell

- (void) setMessageData:(RBXMessageInfo *)messageData
{
    _messageData = messageData;
    
    self.messageLabel.text = messageData.subject;
    self.dateLabel.text = messageData.date;
    self.playerNameLabel.text = messageData.senderUsername;
    self.isRead = messageData.isRead;
    
    NSMutableAttributedString* message = [[NSMutableAttributedString alloc] init];
    [message appendAttributedString:[[NSAttributedString alloc] initWithString:self.messageData.subject
                                                                    attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f], NSForegroundColorAttributeName,
                                                                                [UIFont fontWithName:@"SourceSansPro-Semibold" size:12], NSFontAttributeName,
                                                                                nil]]];
    
    [message appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"  -  %@", [self.messageData.message stringByStrippingHTML]]
                                                                    attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f], NSForegroundColorAttributeName,
                                                                                [UIFont fontWithName:@"SourceSansPro-Regular" size:12], NSFontAttributeName,
                                                                                nil]]];
    self.messageLabel.attributedText = message;
}

- (void)setIsRead:(BOOL)isRead
{
    _isRead = isRead;
    
    // Create subject/message preview
    //[RobloxTheme applyToNotificationPreview:self isRead:isRead];
    self.playerNameLabel.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    
    self.dateLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:12];
    self.dateLabel.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    if(isRead)
    {
        self.playerNameLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
        self.unreadIndicator.opaque = NO;
        self.unreadIndicator.alpha = 1;
        [UIView animateWithDuration:0.3 animations:^
         {
             self.unreadIndicator.alpha = 0;
         }
         completion:^(BOOL finished)
         {
             self.unreadIndicator.opaque = YES;
             self.unreadIndicator.hidden = YES;
         }];
    }
    else
    {
        self.playerNameLabel.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
        self.unreadIndicator.hidden = NO;
    }
}

@end


#pragma mark -
#pragma mark Detail controller

@interface NotificationsDetailController () <UITableViewDelegate, UITableViewDataSource, UIWebViewDelegate>

@end

@implementation NotificationsDetailController
{
    // Left panel
    IBOutlet UILabel* _title;
    IBOutlet UITableView* _messagesTable;
    UIRefreshControl* _refreshIndicator;
    
    // Right panel
    IBOutlet UIView* _messageContainer;
    IBOutlet UILabel* _messageSubject;
    IBOutlet UILabel* _messageDetails;
    IBOutlet UIWebView* _messageBody;
    RBActivityIndicatorView* _loadingSpinner;
    
    BOOL _initialized;
    NSMutableArray* _messages;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    // Stylize message
    _messageSubject.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
    _messageSubject.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    
    _messageDetails.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:12];
    _messageDetails.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    _messageContainer.hidden = YES;
    _messageSubject.text = @"";
    _messageDetails.text = @"";
    _messageBody.delegate = self;
    [_messageBody loadHTMLString:@"" baseURL:nil];
    
    //pull to refresh controller
    _refreshIndicator = [[UIRefreshControl alloc] init];
    [_refreshIndicator addTarget:self action:@selector(refreshMessages:) forControlEvents:UIControlEventValueChanged];
    [_messagesTable addSubview:_refreshIndicator];
    
    _loadingSpinner = [[RBActivityIndicatorView alloc] initWithFrame:SPINNER_ICON_FRAME];
    [self.view addSubview:_loadingSpinner];
    
    _messagesTable.dataSource = self;
    _messagesTable.delegate = self;

    _messages = [NSMutableArray array];
    
    _title.text = NSLocalizedString(@"NotificationsWord", nil);
    [RobloxTheme applyToTableHeaderTitle:_title];
    
    _initialized = NO;
    
    //set the flurry events
    [self setFlurryEventsForExternalLinkEvent:NDC_openExternalLink
                              andWebViewEvent:NDC_openLocalLink
                          andOpenProfileEvent:NDC_openProfile
                       andOpenGameDetailEvent:NDC_openGameDetail];
}
-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextMessages];
}

- (void) viewDidLayoutSubviews
{
    //center the loading spinner in the right hand column
    [_loadingSpinner centerInFrame:_messageContainer.frame];
}

- (void) loadData
{
    if(!_initialized)
    {
        _initialized = YES;
        [_loadingSpinner startAnimating];
        
        [RobloxData fetchNotificationsWithCompletion:^(NSArray *messages)
         {
             [_messages addObjectsFromArray:messages];
             dispatch_async(dispatch_get_main_queue(), ^
                            {
                                [_loadingSpinner stopAnimating];
                                [_messagesTable reloadData];
                            });
         }];
    }
}

- (void) refreshMessages:(UIRefreshControl*)refreshControl
{
    if (refreshControl)
    {
        [Flurry logEvent:NDC_didRefreshMessages];
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRefresh withContext:RBXAContextMessages withCustomData:RBXACustomSectionNotifications];
    }
    
    //make a request to pull down the messages from the server
    [RobloxData fetchNotificationsWithCompletion:^(NSArray *messages)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            //loop through the list of messages and look for differences between the two sets
            //since messageIDs are sorted chronologically on the server, we can easily check what messages are new, the same, or missing in the new list
            NSMutableArray* mergedMessages = [NSMutableArray arrayWithCapacity:([messages count] + [_messages count])];
            NSMutableArray* setToAdd = [NSMutableArray arrayWithCapacity:[messages count]];
            NSMutableArray* setToRemove = [NSMutableArray arrayWithCapacity:[_messages count]];
             
            int i = 0; int j = 0;
            while (i < messages.count && j < _messages.count)
            {
                RBXMessageInfo* messageNew = messages[i];
                RBXMessageInfo* messageOld = _messages[j];
                if ([messageNew messageID] > [messageOld messageID])
                {
                    [setToAdd addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                    [mergedMessages addObject:messages[i]];
                    i++;
                }
                else if ([messageNew messageID] == [messageOld messageID])
                {
                    [mergedMessages addObject:messages[i]];
                    i++; j++;
                }
                else
                {
                    [setToRemove addObject:[NSIndexPath indexPathForRow:j inSection:0]];
                    j++;
                }
            }
             
            //handle the remaining messages
            for (; i <  messages.count; i++)
            {
                [setToAdd addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                [mergedMessages addObject:messages[i]];
            }
            for (; j < _messages.count; j++)
                [setToRemove addObject:[NSIndexPath indexPathForRow:j inSection:0]];
            
             
            //assign the combined list of messages to the table array
            _messages = mergedMessages;
            
            //stop the animation and refresh the table
        
            if (refreshControl)
                [refreshControl endRefreshing];

            if (_initialized)
            {
                //animate the change
                @try
                {
                    [_messagesTable beginUpdates];
                    [_messagesTable deleteRowsAtIndexPaths:setToRemove withRowAnimation:UITableViewRowAnimationAutomatic];
                    [_messagesTable insertRowsAtIndexPaths:setToAdd withRowAnimation:UITableViewRowAnimationAutomatic];
                    [_messagesTable endUpdates];
                }
                @catch (NSException* e)
                {
                    NSLog(@"Failed to animate the update to the notifications");
                }
            }
            else
            {
                _initialized = YES;
            }

            //mark messages as unread if they have changed
            [_messagesTable reloadData];
          });
     }];
}

#pragma mark -
#pragma mark Table delegates

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _messages.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString* CellIdentifier = @"NotificationCell";
    RBXMessageInfo* message = _messages[indexPath.row];
    NotificationCell* cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    [cell setMessageData:message];
    [cell setIsRead:[message isRead]];
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonReadMessage withContext:RBXAContextMessages withCustomData:RBXACustomSectionNotifications];
    [Flurry logEvent:NDC_didPressNotification];
    RBXMessageInfo* message = _messages[indexPath.row];
    
    _messageSubject.text = message.subject;
    _messageDetails.text = [NSString stringWithFormat:@"%@ %@, %@", NSLocalizedString(@"ByWord", nil), message.senderUsername, message.date];
    
    NSString* htmlMessage = [NSString stringWithFormat:@"<span style=\"font-family: %@; font-size: %i; color: %@\">%@</span>",
                             @"SourceSansPro-Regular",
                             12,
                             @"#343434",
                             message.message];
    [_messageBody loadHTMLString:htmlMessage baseURL:nil];
    
    _messageContainer.hidden = NO;
    
    // Mark the message as read
    if(!message.isRead)
    {
        [RobloxData markMessageAsRead:message.messageID completion:^(BOOL success)
        {
            if(success)
            {
                dispatch_async(dispatch_get_main_queue(), ^
                {
                    NotificationCell* cell = (NotificationCell*) [tableView cellForRowAtIndexPath:indexPath];
                    [cell setIsRead:YES];
                    [message setIsRead:YES];
                    
                    //refresh the notification badge
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_READ_MESSAGE object:self];
                });
            }
        }];
    }
}

#pragma mark -
#pragma mark WebViewDelegate

-(BOOL) webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType
{
    if (inType == UIWebViewNavigationTypeLinkClicked)
    {
        [self handleWebRequestWithPopout:inRequest.URL];
    }
    
    return inType != UIWebViewNavigationTypeLinkClicked;
}


@end
