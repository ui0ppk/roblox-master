//
//  RobloxOgreView.h
//  MacClient
//
//  Created by Tony on 12/13/10.
//  Copyright 2010 Roblox. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface RobloxOgreView : NSView
{
	class RobloxView *robloxView;
	NSResponder *appDelegate;
    NSTrackingArea* trackingArea;
    
	BOOL cursorHidden;
	BOOL controlKeyWasDown;
	BOOL fullScreen;
    
    CGPoint virtualMousePosition;
    
    NSRect nonFullScreenRect;
    NSInteger nonFullScreenWindowLevel;
}

@property (assign) BOOL cursorHidden;
@property (assign) BOOL controlKeyWasDown;
@property (assign) BOOL fullScreen;

-(id)initWithFrame:(NSRect)f;

-(CGPoint) getVirtualCursorPos;
-(void) setVirtualCursorPos:(CGPoint) newPos;

-(void) mouseDown:(NSEvent *)event;
-(void) mouseDragged:(NSEvent *)event;
-(void) mouseMoved:(NSEvent *)event;
-(void) mouseUp:(NSEvent *)event;

-(void) rightMouseDown:(NSEvent *)event;
-(void) rightMouseDragged:(NSEvent *)event;
-(void) rightMouseUp:(NSEvent *)event;

-(void) otherMouseDown:(NSEvent *)event;
-(void) otherMouseDragged:(NSEvent *)event;
-(void) otherMouseUp:(NSEvent *)event;

-(void) mouseEntered:(NSEvent *)theEvent;
-(void) mouseExited:(NSEvent *)theEvent;

-(void) keyDown:(NSEvent *)event;
-(void) keyUp:(NSEvent *)event;
-(void) flagsChanged:(NSEvent *)event;

-(void) scrollWheel:(NSEvent *)theEvent;

-(void) setRobloxView:(class RobloxView *)rbxview;
-(void) setAppDelegate:(NSResponder *)appDelegate;

-(BOOL) cursorInViewBounds;
-(void) showCursor;
-(void) hideCursor;

-(void) toggleFullScreen;
-(BOOL) inFullScreenMode;
-(void) viewDidEndLiveResize;

-(void) finishFullScreenChange;

- (float) titleBarHeight;

- (void) setCursorPositionToVirtualPositionAndShow;
- (BOOL) isMouseOverVisibleWindow;

@end
