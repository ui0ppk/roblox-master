#pragma once

#import <Cocoa/Cocoa.h>
#import "CustomUIObjects.h"

@interface InstallSuccessController : NSWindowController
{
    IBOutlet id      logoImage;
    IBOutlet id      customImageButton;
    IBOutlet id      backToDevelopPageButton;
}

- (IBAction)onButtonClicked:(CustomImageButton *)sender;

@end
