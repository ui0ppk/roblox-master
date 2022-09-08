//
//  InstallSuccessController.m
//  Roblox
//
//  Created by Dharmendra Rohit on 24/02/15.
//
//

#import "InstallSuccessController.h"

#ifndef TARGET_IS_STUDIO
static NSString* const buttonImageName = @"btn_ok";
#else
static NSString* const buttonImageName = @"btn_launchstudio";
#endif

@implementation InstallSuccessController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    NSColor* color = [NSColor colorWithDeviceRed:256.0f/256.0f green:256.0f/256.0f blue:256.0f/256.0f alpha:1.0f];
    [self.window setBackgroundColor:color];
    [self.window update];
    
    [customImageButton setImageName:buttonImageName];
    [[customImageButton cell] setHighlightsBy:NSNoCellMask];
    
#ifdef TARGET_IS_STUDIO
    [backToDevelopPageButton setImageName:@"txt_backto"];
    [[backToDevelopPageButton cell] setHighlightsBy:NSNoCellMask];
#endif
}

- (void)onButtonClicked:(CustomImageButton *)sender
{
    NSLog(@"InstallSuccessController - onButtonClicked");
    
    [self.window orderOut:nil];
#ifdef TARGET_IS_STUDIO
    if (sender == backToDevelopPageButton)
    {
        NSLog(@"InstallSuccessController - backToDevelopPageButton");
        // Posting notification from another object
        [[NSNotificationCenter defaultCenter] postNotificationName:@"developPageLinkClicked"
                                                            object:self.window
                                                          userInfo:nil];
    }
    else
#endif
    {
        [self close];
    }
}

@end
