#import "CustomUIObjects.h"

@implementation SplashScreenPanel
- (BOOL) canBecomeKeyWindow
{
    return TRUE;
}
@end

@implementation CustomImageButton

- (void) setImageName:(NSString*)imgName
{
    buttonImageName = imgName;
    [buttonImageName retain];
    
    [self setImage:[NSImage imageNamed:buttonImageName]];
}

- (NSString*) getImageName
{
    return buttonImageName;
}

- (void)mouseEntered:(NSEvent *)event
{
    [self setImage:[NSImage imageNamed:[NSString stringWithFormat:@"%@-ON", buttonImageName]]];
}

- (void)mouseExited:(NSEvent *)event
{
    [self setImage:[NSImage imageNamed:buttonImageName]];
}

- (void)mouseDown:(NSEvent *)event
{
    [self performClick:event];
    [self setImage:[NSImage imageNamed:buttonImageName]];
}

- (void)mouseUp:(NSEvent *)event
{
    [super mouseUp:event];
    [self setImage:[NSImage imageNamed:[NSString stringWithFormat:@"%@-ON", buttonImageName]]];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    
    if (trackingArea)
    {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }
    
    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp;
    trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

@end