//
//  RobloxHUD.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/21/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBXFunctions.h"
#import "RobloxHUD.h"
#import "UIAlertView+Blocks.h"
#import "HUD/MBProgressHUD.h"
#import "UIViewController+Helpers.h"

static MBProgressHUD* messagesHUD;
static MBProgressHUD* spinnerHUD;

@implementation RobloxHUD

+(void) showMessage:(NSString*)message
{
    [RobloxHUD hideMessage:YES];
    
    UIViewController* topViewController = [UIViewController topViewController];
    
    messagesHUD = [MBProgressHUD showHUDAddedTo:topViewController.view animated:YES];
    
    // Configure for text only and offset down
    messagesHUD.mode = MBProgressHUDModeText;
    messagesHUD.detailsLabelText = message;
    messagesHUD.detailsLabelFont = [UIFont systemFontOfSize:18.0];
    messagesHUD.margin = 20.f;
    messagesHUD.removeFromSuperViewOnHide = YES;
    
    [messagesHUD hide:YES afterDelay:2];
}

+(void) hideMessage:(BOOL)animated
{
    if(messagesHUD != nil)
    {
        [messagesHUD hide:animated];
        messagesHUD = nil;
    }
}

+(void) prompt:(NSString*)message onOK:(void(^)())onOKBlock onCancel:(void(^)())onCancelBlock
{
    // Open an alert with OK and Cancel buttons
    dispatch_async(dispatch_get_main_queue(),^{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"RobloxWord", @"")
                                                        message:message
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                              otherButtonTitles:NSLocalizedString(@"OkWord", nil), nil];
        
        [alert showWithCompletion:^(UIAlertView* alertView, NSInteger buttonIndex) {
            if(buttonIndex == 1)
            {
                if(onOKBlock)
                    onOKBlock();
            }
            else
            {
                if(onCancelBlock)
                    onCancelBlock();
            }
        }];
    });
}

+(void) prompt:(NSString*)message withTitle:(NSString *)title onOK:(void (^)())onOKBlock onCancel:(void (^)())onCancelBlock
{
    // Open an alert with OK and Cancel buttons
    dispatch_async(dispatch_get_main_queue(),^{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:title
                                                        message:message
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                              otherButtonTitles:NSLocalizedString(@"OkWord", nil), nil];
        
        [alert showWithCompletion:^(UIAlertView* alertView, NSInteger buttonIndex)
        {
            if(buttonIndex == 1)
            {
                if(onOKBlock)
                    onOKBlock();
            }
            else
            {
                if(onCancelBlock)
                    onCancelBlock();
            }
        }];
    });
}

+(void) prompt:(NSString*)message
     withTitle:(NSString*)title
{
    // Open an alert with OK and Cancel buttons
    dispatch_async(dispatch_get_main_queue(),^{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:title
                                                        message:message
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"OkWord", nil)
                                              otherButtonTitles:nil, nil];
        [alert show];
    });
}



+(void) showSpinnerWithLabel:(NSString*)labelText dimBackground:(BOOL)dim
{
    [RobloxHUD hideSpinner:NO];
    
    UIViewController* topViewController = [UIViewController topViewController];
    
    spinnerHUD = [[MBProgressHUD alloc] initWithView:topViewController.view];
    [topViewController.view addSubview:spinnerHUD];
    
    spinnerHUD.labelText = labelText;
    spinnerHUD.removeFromSuperViewOnHide = YES;
    spinnerHUD.dimBackground = dim;
    spinnerHUD.color = [UIColor clearColor];
    
    [spinnerHUD show:YES];
}

+(void) hideSpinner:(BOOL)animate
{
    [RBXFunctions dispatchOnMainThread:^{
        if( spinnerHUD != nil )
        {
            [spinnerHUD hide:animate];
            spinnerHUD = nil;
        }
    }];
}



@end
