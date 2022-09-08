//
//  RBMessageComposeScreenController.m
//  RobloxMobile
//
//  Created by alichtin on 9/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBMessageComposeScreenController.h"
#import "UIView+Position.h"
#import "RobloxTheme.h"
#import "RobloxData.h"
#import "RobloxHUD.h"
#import "UIAlertView+Blocks.h"
#import "RobloxImageView.h"
#import "RBXEventReporter.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 510.f, 315.f)
#define NAV_BAR_HEIGHT 46.0
#define AVATAR_SIZE CGSizeMake(110, 110)

@interface RBMessageComposeScreenController () <UIWebViewDelegate, UIScrollViewDelegate>

@end

@implementation RBMessageComposeScreenController
{
    RBXMessageInfo* _replyToMessage;
    NSNumber* _recipientID;
    NSString* _recipientName;

    IBOutlet UINavigationBar* _navBar;
    IBOutlet UITextView *_messageTextView;
    IBOutlet UIWebView *_conversationBody;
    IBOutlet RobloxImageView *_friendAvatar;
    IBOutlet UIView *_conversationView;
    
    NSString* analyticsResponseType;
}

- (void) replyToMessage:(RBXMessageInfo*)message;
{
    _replyToMessage = message;
    _recipientID = message.senderUserID;
    _recipientName = message.senderUsername;
    
    analyticsResponseType = @"reply";
}

- (void) sendMessageToUser:(NSNumber*)userID name:(NSString*)userName
{
    _recipientID = userID;
    _recipientName = userName;
    
    analyticsResponseType = @"sendNew";
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Stylize items
    [RobloxTheme applyToModalPopupNavBar:_navBar];
    
    // ReplyTo setup
    if(_replyToMessage != nil)
    {
        [_friendAvatar loadAvatarForUserID:[_replyToMessage.senderUserID integerValue] withSize:AVATAR_SIZE completion:nil];
        
        NSString* htmlMessage = [NSString stringWithFormat:@"<div style='border-radius: 5px; background-color:#bee7ff; padding:10px; padding-top:0px;'>"
                                                           @"    <p>"
                                                           @"        <span style='font-family: SourceSansPro-Semibold; font-size: 12; color: #474747'>%@</span>"
                                                           @"        <span style='font-family: SourceSansPro-Regular; font-size: 12; color: #808080'>%@ %@</span>"
                                                           @"    </p>"
                                                           @"    <p style='font-family: SourceSansPro-Regular; font-size: 14; color: #343434'>"
                                                           @"        %@"
                                                           @"    </p>"
                                                           @"</div>",
                                                         _replyToMessage.senderUsername,
                                                         NSLocalizedString(@"WroteAtPhrase", nil),
                                                         _replyToMessage.date,
                                                         _replyToMessage.body];
        [_conversationBody loadHTMLString:htmlMessage baseURL:nil];
        _conversationBody.delegate = self;
        _conversationBody.scrollView.delegate = self;
        
        _navBar.topItem.title = [NSString stringWithFormat:NSLocalizedString(@"ReplyToPhrase", nil), _recipientName];
    }
    else
    {
        _conversationView.hidden = YES;
        _messageTextView.frame = DEFAULT_VIEW_SIZE;
        _messageTextView.y = NAV_BAR_HEIGHT;
        _messageTextView.height = _messageTextView.height - NAV_BAR_HEIGHT;
        
        _navBar.topItem.title = [NSString stringWithFormat:NSLocalizedString(@"MessageToPhrase", nil), _recipientName];
    }
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    //due to an issue in timing, it is best to wait for the presentation to finish before
    //setting the text to become the first responder, so just hold on a tic
    [_messageTextView performSelector:@selector(becomeFirstResponder) withObject:nil afterDelay:0.25f];
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    if (SYSTEM_VERSION_LESS_THAN(@"8"))
    {
        self.view.superview.bounds = DEFAULT_VIEW_SIZE;
    }
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    if(_replyToMessage == nil)
    {
        _messageTextView.frame = CGRectUnion(_messageTextView.frame, _conversationView.frame);
    }
}

- (IBAction)closeButtonTouched:(id) sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextDraftMessage withCustomDataString:analyticsResponseType];
    
    if ([_messageTextView.text length] > 0)
    {
        //warn the user before the close
        UIAlertView* aWarning = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"MessageReplyConfirmExitTitle", nil)
                                                           message:NSLocalizedString(@"MessageReplyConfirmExitMessage", nil)
                                                          delegate:nil
                                                 cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                                 otherButtonTitles:NSLocalizedString(@"MessageReplyConfirmExitWord", nil), nil];
        
        [aWarning showWithCompletion:^(UIAlertView *alertView, NSInteger buttonIndex)
         {
             switch (buttonIndex)
             {
                 case 0:
                     break;
                 case 1:
                 {
                     [self dismissViewControllerAnimated:YES completion:nil];
                     break;
                 }
                 default:
                     break;
             }
         }];
    }
    else
    {
        [self dismissViewControllerAnimated:YES completion:nil];
    }
}

- (IBAction)sendButtonTouched:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSubmit withContext:RBXAContextDraftMessage withCustomDataString:analyticsResponseType];
    [RobloxHUD showSpinnerWithLabel:@"" dimBackground:YES];
    
    [_messageTextView resignFirstResponder];
    
    if(_replyToMessage != nil)
    {
        [RobloxData sendMessage:_messageTextView.text
                        subject:_replyToMessage.subject
                    recipientID:_recipientID
                 replyMessageID:[NSNumber numberWithInteger:_replyToMessage.messageID]
                     completion:^(BOOL success, NSString* errorMessage)
                     {
                         [self onSendMessageComplete:success errorMessage:errorMessage];
                     }];
    }
    else
    {
        [RobloxData sendMessage:_messageTextView.text
                        subject:@"Message"
                    recipientID:_recipientID
                 replyMessageID:nil
                     completion:^(BOOL success, NSString* errorMessage)
                     {
                         [self onSendMessageComplete:success errorMessage:errorMessage];
                     }];
    }
}

- (void)onSendMessageComplete:(BOOL)success errorMessage:(NSString*)errorMessage
{
    dispatch_async(dispatch_get_main_queue(), ^
    {
        [RobloxHUD hideSpinner:YES];
        
        if(success)
        {
            [RobloxHUD showMessage:NSLocalizedString(@"MessageSentPhrase", nil)];
            
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.5f * NSEC_PER_SEC), dispatch_get_main_queue(), ^
            {
                [self dismissViewControllerAnimated:YES completion:nil];
            });
        }
        else
        {
            [RobloxHUD showMessage:errorMessage];
        }
    });
}

@end
