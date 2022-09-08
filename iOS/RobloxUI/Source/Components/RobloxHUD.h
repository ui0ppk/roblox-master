//
//  RobloxHUD.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/21/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RobloxHUD : NSObject

// Messages
+(void) showMessage:(NSString*)message;
+(void) hideMessage:(BOOL)animate;

+(void) prompt:(NSString*)message
        onOK:(void(^)())onOKBlock
        onCancel:(void(^)())onCancelBlock;
+(void) prompt:(NSString*)message
     withTitle:(NSString*)title
          onOK:(void(^)())onOKBlock
      onCancel:(void(^)())onCancelBlock;
+(void) prompt:(NSString*)message
     withTitle:(NSString*)title;

// Spinner
+(void) showSpinnerWithLabel:(NSString*)labelText dimBackground:(BOOL)dim;
+(void) hideSpinner:(BOOL)animate;

@end
