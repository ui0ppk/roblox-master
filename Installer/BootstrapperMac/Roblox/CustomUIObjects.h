#pragma once

@interface SplashScreenPanel: NSPanel
{
    
}
@end

@interface CustomImageButton : NSButton
{
    NSString       *buttonImageName;
    NSTrackingArea *trackingArea;
}

- (void) setImageName:(NSString*)imgName;
- (NSString*) getImageName;

@end
