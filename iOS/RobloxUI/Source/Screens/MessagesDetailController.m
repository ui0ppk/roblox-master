//
//  MessagesDetailController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/19/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "MessagesDetailController.h"
#import "RBMessageComposeScreenController.h"
#import "RobloxImageView.h"
#import "RobloxTheme.h"
#import "NSString+stripHtml.h"
#import <UIKit/NSAttributedString.h>
#import "Flurry.h"
#import "RobloxNotifications.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "UserInfo.h"
#import "RobloxInfo.h"
#import "RBInfiniteTableView.h"
#import "RBProfileViewController.h"
#import "RBMobileWebViewController.h"
#import "RBWebGamePreviewScreenController.h"

#define SPINNER_ICON_FRAME CGRectMake(0, 0, 32, 32)
#define ITEMS_PER_REQUEST 20
#define AVATAR_SIZE CGSizeMake(110, 110)

//---METRICS---
#define MDC_didSelectMessage    @"MESSAGES DETAIL SCREEN - Message Selected"
#define MDC_didRefreshMessages  @"MESSAGES DETAIL SCREEN - Messages Refreshed"
#define MDC_openExternalLink    @"MESSAGES DETAIL SCREEN - Open External Link"
#define MDC_openLocalLink       @"MESSAGES DETAIL SCREEN - Open Local Link"
#define MDC_openProfile         @"MESSAGES DETAIL SCREEN - Open Profile"
#define MDC_openGameDetail      @"MESSAGES DETAIL SCREEN - Open Game Detail"
#define MDC_launchGame          @"MESSAGES DETAIL SCREEN - Launch Game"

#pragma mark -
#pragma mark Custom Cell

@implementation MessageCell

- (void) setMessageData:(RBXMessageInfo *)messageData
{
    _messageData = messageData;
    
    if(_messageData != nil)
    {
        self.messageLabel.text = messageData.subject;
        self.dateLabel.text = messageData.date;
        self.playerNameLabel.text = messageData.senderUsername;
        self.isRead = messageData.isRead;
        self.playerAvatar.hidden = NO;
        [self.playerAvatar loadAvatarForUserID:[messageData.senderUserID integerValue] withSize:AVATAR_SIZE completion:nil];
        
        NSMutableAttributedString* message = [[NSMutableAttributedString alloc] init];
        [message appendAttributedString:[[NSAttributedString alloc] initWithString:self.messageData.subject
                                                                        attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                    [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f], NSForegroundColorAttributeName,
                                                                                    [UIFont fontWithName:@"SourceSansPro-Semibold" size:12], NSFontAttributeName,
                                                                                    nil]]];
        
        [message appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"  -  %@", [self.messageData.body stringByStrippingHTML]]
                                                                        attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                    [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f], NSForegroundColorAttributeName,
                                                                                    [UIFont fontWithName:@"SourceSansPro-Regular" size:12], NSFontAttributeName,
                                                                                    nil]]];
        self.messageLabel.attributedText = message;
    }
    else
    {
        self.messageLabel.text = @"";
        self.dateLabel.text = @"";
        self.playerNameLabel.text = @"";
        self.isRead = YES;
        self.playerAvatar.hidden = YES;
        self.messageLabel.attributedText = nil;
    }
}

- (void)setIsRead:(BOOL)isRead
{
    _isRead = isRead;
    
    // Create subject/message preview
    //[RobloxTheme applyToMessagePreview:self isRead:isRead];
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
        self.unreadIndicator.alpha = 1;
        self.unreadIndicator.opaque = YES;
    }
}

@end


#pragma mark -
#pragma mark Detail controller

@interface MessagesDetailController () <RBInfiniteTableViewDelegate, UIWebViewDelegate>

@end

@implementation MessagesDetailController
{
    // Left panel
    IBOutlet UILabel* _inboxTitle;
    IBOutlet RBInfiniteTableView* _messagesTable;
    UIRefreshControl* _refreshIndicator;
    
    // Right panel
    IBOutlet UIView* _messageContainer;
    IBOutlet UIWebView* _messageBody;
    IBOutlet UILabel* _messageSubject;
    IBOutlet UILabel* _messageDetails;
    IBOutlet UIButton* _replyButton;
    IBOutlet UIButton* _archiveButton; //also doubles as an unarchiveButton
    IBOutlet UIButton* _builderButton;
    IBOutlet UIButton* _reportAbuseButton;
    IBOutlet RobloxImageView* _messageImage;
    RBXMessageInfo* _selectedMessage;
    RBActivityIndicatorView* _loadingSpinner;
    
    BOOL _initialized;
    NSMutableArray* _messages;
    RBXAnalyticsCustomData _inboxType;
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
    
    _messagesTable.infiniteDelegate = self;
    
    _messages = [NSMutableArray array];
    
    [RobloxTheme applyToTableHeaderTitle:_inboxTitle];
    
    _builderButton.hidden = YES;
    [_builderButton setTitle:@">" forState:UIControlStateNormal];
    [RobloxTheme applyToGamePreviewBuilderButton:_builderButton];
    [RobloxTheme applyToGamePreviewBuilderButton:_archiveButton];
    [RobloxTheme applyToGamePreviewBuilderButton:_reportAbuseButton];
    
    _initialized = NO;

    //set the Flurry events
    [self setFlurryEventsForExternalLinkEvent:MDC_openExternalLink
                              andWebViewEvent:MDC_openLocalLink
                          andOpenProfileEvent:MDC_openProfile
                       andOpenGameDetailEvent:MDC_openGameDetail];
    _inboxType = RBXACustomSectionInbox;
}
- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextMessages];
}
- (void) viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    //center the loading spinner in the right hand column
    [_loadingSpinner centerInFrame:_messageContainer.frame];
}

#pragma mark -
#pragma mark Mutators
-(void) setTypeOfMessages:(RBXMessageType)typeOfMessages
{
    _typeOfMessages = typeOfMessages;
    switch (_typeOfMessages)
    {
        case RBXMessageTypeArchive:
        {
            _inboxType = RBXACustomSectionArchive;
            _inboxTitle.text = NSLocalizedString(@"ArchivedMessagesWord", nil);
            [_archiveButton setTitle:NSLocalizedString(@"UnarchivedMessagesWord", nil) forState:UIControlStateNormal];
        } break;
            
        case RBXMessageTypeSent:
        {
            _inboxType = RBXACustomSectionSent;
            _inboxTitle.text = NSLocalizedString(@"SentMessagesWord", nil);
            [_archiveButton setTitle:NSLocalizedString(@"ArchivedMessagesWord", nil) forState:UIControlStateNormal];
        } break;
            
        default:
        {
            _inboxType = RBXACustomSectionInbox;
            _inboxTitle.text = NSLocalizedString(@"InboxWord", nil);
            [_archiveButton setTitle:NSLocalizedString(@"ArchivedMessagesWord", nil) forState:UIControlStateNormal];
        } break;
    }
}

#pragma mark -
#pragma mark Data Functions

- (void) loadData
{
    if(!_initialized)
    {
        _initialized = YES;
        [_loadingSpinner startAnimating];
        
        [_messagesTable loadElementsAsync];
    }
}
- (void) refreshMessages:(UIRefreshControl*)refreshControl
{
    if (refreshControl)
    {
        [Flurry logEvent:MDC_didRefreshMessages];
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRefresh withContext:RBXAContextMessages withCustomData:_inboxType];
    }
    
    //make a request to pull down the messages from the server
    [RobloxData fetchMessages:_typeOfMessages pageNumber:0 pageSize:ITEMS_PER_REQUEST completion:^(NSArray *messages)
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

            //handle the remaining messages : add the new ones, remove the old ones 
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
                @try
                {
                    //animate the change
                    [_messagesTable beginUpdates];
                    [_messagesTable deleteRowsAtIndexPaths:setToRemove withRowAnimation:UITableViewRowAnimationAutomatic];
                    [_messagesTable insertRowsAtIndexPaths:setToAdd withRowAnimation:UITableViewRowAnimationAutomatic];
                    [_messagesTable endUpdates];
                }
                @catch (NSException* e)
                {
                    NSLog(@"Failed to animate the refresh changes");
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

- (void) asyncRequestItemsForTableView:(RBInfiniteTableView*)tableView numItemsToRequest:(NSUInteger)itemsToRequest completionHandler:(void(^)())completionHandler
{
    if(_messagesTable.numItems > _messages.count)
    {
        void(^block)(NSArray*) = ^(NSArray* messages)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [_loadingSpinner stopAnimating];
                [_messages addObjectsFromArray:messages];
                
                completionHandler();
            });
        };
        
        int curPageNum =  floor(_messages.count / itemsToRequest);
        int remainder = _messages.count - (curPageNum * itemsToRequest);
        int requestAmt = itemsToRequest - remainder;
        [RobloxData fetchMessages:_typeOfMessages pageNumber:curPageNum pageSize:requestAmt completion:block];
        //[RobloxData fetchMessages:_typeOfMessages startIndex:_messages.count numItems:itemsToRequest completion:block];
    }
}

- (NSUInteger)numItemsInInfiniteTableView:(RBInfiniteTableView*)tableView
{
    return _messages.count;
}

- (UITableViewCell*) infiniteTableView:(RBInfiniteTableView*)tableView cellForItemAtIndexPath:(NSIndexPath*)indexPath
{
    static NSString* CellIdentifier = @"MessageCell";
    
    MessageCell* cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    if(indexPath.row < _messages.count)
    {
        RBXMessageInfo* message = _messages[indexPath.row];
        cell.messageData = message;
        cell.isRead = (_typeOfMessages == RBXMessageTypeSent) ? YES : [message isRead];
    }
    else
    {
        cell.messageData = nil;
    }
    
    return cell;
}

- (void) infiniteTableView:(RBInfiniteTableView*)tableView didSelectItemAtIndexPath:(NSIndexPath*)indexPath
{
    [Flurry logEvent:MDC_didSelectMessage];
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonReadMessage withContext:RBXAContextMessages withCustomData:_inboxType];
    if (_messages.count < indexPath.row)
        return;
    _selectedMessage = _messages[indexPath.row];
    
    _messageSubject.text = _selectedMessage.subject;
    _messageDetails.text = [NSString stringWithFormat:@"%@ %@, %@", NSLocalizedString(@"ByWord", nil), _selectedMessage.senderUsername, _selectedMessage.date];
    
    //check if the message is from Roblox or a system message to the user, and hide the Reply button
    bool isFromRoblox = [_selectedMessage.senderUserID isEqualToNumber:[NSNumber numberWithInt:1]];
    bool isSystemMessage = [_selectedMessage.senderUserID isEqualToNumber:[UserInfo CurrentPlayer].userId];
    _replyButton.hidden = (isFromRoblox || isSystemMessage);
    _reportAbuseButton.hidden = YES; //(isFromRoblox || isSystemMessage);
    _archiveButton.hidden = (isFromRoblox || isSystemMessage);
    
    NSString* htmlMessage = [NSString stringWithFormat:@"<span style=\"font-family: %@; font-size: %i; color: %@\"> %@ </span>",
                             @"SourceSansPro-Regular",
                             12,
                             @"#343434",
                             _selectedMessage.body];
    
    if (!isFromRoblox && !isSystemMessage)
    {
        //search the message for clickable links
        NSError* error;
        NSDataDetector* urlDetector = [[NSDataDetector alloc] initWithTypes:NSTextCheckingTypeLink error:&error];
        NSArray* foundURLs = [urlDetector matchesInString:htmlMessage options:0 range:NSMakeRange(0, [htmlMessage length])];
        
        //encase the URLs in <a> tags
        for (int i = foundURLs.count - 1; i >= 0; i--)
        {
            NSTextCheckingResult* result = foundURLs[i];
            NSString* urlString = [[NSString stringWithFormat:@"%@", result.URL] stringByRemovingPercentEncoding];
            htmlMessage = [htmlMessage stringByReplacingCharactersInRange:result.range
                                                               withString:[NSString stringWithFormat:@"<a href=\"%@\">%@</a>", urlString , urlString]];
        }
    }
    
    //load the html message to the screen
    [_messageBody loadHTMLString:htmlMessage baseURL:nil];
    
    _messageImage.animateInOptions = RBXImageViewAnimateInAlways;
    [_messageImage loadAvatarForUserID:[_selectedMessage.senderUserID integerValue] withSize:AVATAR_SIZE completion:nil];
    
    _messageContainer.hidden = NO;
    
    // Mark the message as read
    if(!_selectedMessage.isRead)
    {
        [RobloxData markMessageAsRead:_selectedMessage.messageID completion:^(BOOL success)
        {
            if(success)
            {
                RBXMessageInfo* weakRefMessage = _selectedMessage;
                dispatch_async(dispatch_get_main_queue(), ^
                {
                    MessageCell* cell = (MessageCell*) [tableView cellForRowAtIndexPath:indexPath];
                    [cell setIsRead:YES];
                    weakRefMessage.isRead = YES;
                    
                    //decrement the message badge
                    if (_typeOfMessages == RBXMessageTypeInbox)
                        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_READ_MESSAGE object:self];
                });
            }
        }];
    }
    
    
    
    //allow the user to link to the message sender's profile
    _builderButton.hidden = isSystemMessage;
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if( [segue.identifier isEqualToString:@"replyToMessage"] )
    {
        RBXMessageInfo* message = _messages[[_messagesTable indexPathForSelectedRow].row];
        
        RBMessageComposeScreenController* controller = segue.destinationViewController;
        [controller replyToMessage:message];
    }
}


#pragma mark -
#pragma mark Button Actions
- (IBAction)builderButtonTouchUpInside:(id)sender
{
    [Flurry logEvent:MDC_openProfile];
    [self pushProfileControllerWithUserID:_selectedMessage.senderUserID];
}
- (IBAction)archiveButtonTouchUpInside:(id)sender
{
    if (_typeOfMessages != RBXMessageTypeArchive)
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonArchive withContext:RBXAContextMessages withCustomDataString:@"archive"];
        [RobloxData archiveMessage:_selectedMessage.messageID completion:^(BOOL success)
        {
            if (success)
                [self refreshMessages:nil];
        }];
    }
    else
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonArchive withContext:RBXAContextMessages withCustomDataString:@"unarchive"];
        [RobloxData unarchiveMessage:_selectedMessage.messageID completion:^(BOOL success)
        {
            if (success)
                [self refreshMessages:nil];
        }];
    }
}
- (IBAction)reportAbuseButtonTouchUpInside:(id)sender
{
    //Report the message
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
