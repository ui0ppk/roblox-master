//
//  RBHybridModuleSocial.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleSocial.h"
#import "RBHybridCommand.h"
#import "RBHybridWebView.h"
#import "NSDictionary+Parsing.h"
#import "UIViewController+Helpers.h"
#import "RBHybridModuleSocial_FBActivity.h"
#import "RBHybridModuleSocial_TwitterActivity.h"
#import "RBHybridModuleSocial_GooglePlusActivity.h"
#import <Social/Social.h>

#define MODULE_ID @"Social"

@implementation RBHybridModuleSocial

-(instancetype) init
{
    self = [super initWithModuleID:MODULE_ID];
    if(self)
    {
        [self registerFunction:@"presentShareDialog" withSelector:@selector(presentShareDialog:)];
    }
    return self;
}

/**
* Present a share dialog
* @param {string} text - Text to share
* @param {string} link - Link to embed
* @param {string} [imageURL=null] - link
* @param {Function} [callback] - Callback
* @instance
*/
-(void)presentShareDialog:(RBHybridCommand*)command
{
    NSMutableArray* items = [NSMutableArray array];
    
    // Collect arguments
    NSString* text = [command.params stringForKey:@"text" withDefault:@""];
    [items addObject:text];
    
    NSString* link = [command.params stringForKey:@"link"];
    NSURL* linkURL = nil;
    if(link != nil)
    {
        linkURL = [NSURL URLWithString:link];
        [items addObject:linkURL];
    }
    
    NSString* imageURLStr = [command.params stringForKey:@"imageURL" withDefault:nil];
    NSURL* imageURL = nil;
    UIImage* image = nil;
    if(imageURL != nil)
    {
        imageURL = [NSURL URLWithString:imageURLStr];
        NSData* imgData = [NSData dataWithContentsOfURL:imageURL];
        image = [UIImage imageWithData:imgData];
        
        [items addObject:image];
    }
    
    // Do the action
    dispatch_async(dispatch_get_main_queue(), ^
    {
        RBHybridModuleSocial_FBActivity* facebookActivity = [[RBHybridModuleSocial_FBActivity alloc] init];
        facebookActivity.parentController = command.originWebView.controller;
        facebookActivity.text = text;
        facebookActivity.link = linkURL;
        facebookActivity.imageURL = imageURL;
        facebookActivity.image = image;
        
        RBHybridModuleSocial_TwitterActivity* twitterActivity = [[RBHybridModuleSocial_TwitterActivity alloc] init];
        twitterActivity.parentController = command.originWebView.controller;
        twitterActivity.text = text;
        twitterActivity.link = linkURL;
        twitterActivity.imageURL = imageURL;
        twitterActivity.image = image;
        
        // For Apple submission problems, I am disbaling GP until they fix the library.
        RBHybridModuleSocial_GooglePlusActivity* googlePlusActivity = [[RBHybridModuleSocial_GooglePlusActivity alloc] init];
        googlePlusActivity.text = text;
        googlePlusActivity.link = linkURL;
        googlePlusActivity.imageURL = imageURL;
        googlePlusActivity.image = image;
        
        NSArray* activities = @[facebookActivity, twitterActivity, googlePlusActivity];
        
        UIViewController* parentController = command.originWebView.controller;
        
        UIActivityViewController* activityController = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:activities];
        activityController.excludedActivityTypes = @[
            UIActivityTypePostToFacebook,
            UIActivityTypePostToTwitter,
            UIActivityTypePostToWeibo,
            UIActivityTypePostToFlickr,
            UIActivityTypePostToVimeo
        ];
        
        if( [activityController respondsToSelector:@selector(popoverPresentationController:)] )
        {
            activityController.popoverPresentationController.sourceView = parentController.view;
            activityController.popoverPresentationController.permittedArrowDirections = 0;
            activityController.popoverPresentationController.sourceRect = CGRectInset(parentController.view.bounds, 200, 200);
        }
        
        // Setup callbacks
        // setCompletionWithItemsHandler is avaialble only in iOS8
        if( [activityController respondsToSelector:@selector(setCompletionWithItemsHandler:)] )
        {
            activityController.completionWithItemsHandler = ^(NSString *activityType, BOOL completed, NSArray *returnedItems, NSError *activityError)
            {
                [self onActionCompletedForCommand:command withStatus:completed];
            };
        }
        else
        {
            activityController.completionHandler = ^(NSString *activityType, BOOL completed)
            {
                [self onActionCompletedForCommand:command withStatus:completed];
            };
        }
        
        [parentController presentViewController:activityController animated:YES completion:nil];
    });
}

-(void)onActionCompletedForCommand:(RBHybridCommand*)command withStatus:(BOOL)completed
{
    [command executeCallback:completed];
}

@end
