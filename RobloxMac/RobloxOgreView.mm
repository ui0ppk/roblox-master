//
//  RobloxOgreView.m
//  MacClient
//
//  Created by Tony on 12/13/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "RobloxOgreView.h"
#import "RobloxView.h"
#import "RobloxPlayerAppDelegate.h"
#import "RBXWindow.h"
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>

#include "G3D/G3DMath.h"

#include "v8datamodel/GameBasicSettings.h"

#define MOUSE_OFFSCREEN_POSITION CGPointMake(-5000,-5000)
#define TRACKING_WIDTH_OFFSET 5.0f

DYNAMIC_FASTFLAGVARIABLE(MiddleMouseButtonEvent, true)
@implementation RobloxOgreView

@synthesize cursorHidden;
@synthesize controlKeyWasDown;
@synthesize fullScreen;

- (id)initWithFrame:(NSRect)f;
{
    if (self = [super initWithFrame:f])
    {
		robloxView = nil;
		appDelegate = nil;
		cursorHidden = NO;
		controlKeyWasDown = NO;
        fullScreen = NO;
        
        virtualMousePosition = MOUSE_OFFSCREEN_POSITION;
        
        [self updateTrackingAreas];
        
        [self setPostsBoundsChangedNotifications: YES];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidBecomeKey:)
                                                     name:NSWindowDidBecomeKeyNotification
												   object:nil];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidResignKey:)
                                                     name:NSWindowDidResignKeyNotification
												   object:nil];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(frameChangedNotification:)
                                                     name:NSViewFrameDidChangeNotification
												   object:nil];
    }
	
    return self;
}

// returns the default titlebar height for OSX applications
- (float) titleBarHeight
{
    NSRect frame = NSMakeRect (0, 0, 100, 100);
    
    NSRect contentRect = [NSWindow contentRectForFrameRect: frame
                                          styleMask: NSTitledWindowMask];
    
    return (frame.size.height - contentRect.size.height);
}

-(void)updateTrackingAreas
{
    [super updateTrackingAreas];
    
    if(trackingArea)
    {
        [self removeTrackingArea:trackingArea];
        [trackingArea release];
    }
    
    // TRACKING_WIDTH_OFFSET allows the mouse to enter a few pixels on each side of the application, otherwise mouse can get
    // trapped at a window resizing state, which can cause issues with tracking mouse events
    NSRect screenBounds = [self bounds];
    screenBounds = NSMakeRect(screenBounds.origin.x + TRACKING_WIDTH_OFFSET,screenBounds.origin.y,screenBounds.size.width - (TRACKING_WIDTH_OFFSET * 2.0f),screenBounds.size.height);
    
    trackingArea = [ [NSTrackingArea alloc] initWithRect:screenBounds
                                                 options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:trackingArea];
    
    NSPoint mouseLocation = [[self window] mouseLocationOutsideOfEventStream];
    mouseLocation = [self convertPoint: mouseLocation
                              fromView: nil];
    
    if (NSPointInRect(mouseLocation, [self bounds]))
        [self mouseEntered: nil];
    else
        [self mouseExited: nil];
}

- (void) updateBounds
{
	NSRect bounds = [self bounds];
    
	if (robloxView)
		robloxView->setBounds(bounds.size.width, bounds.size.height);
}

- (void) frameChangedNotification: (NSNotification *) notification
{
    [self updateBounds];
}

inline void flipY(RobloxOgreView *view, NSPoint &pt)
{
	BOOL flipped = [view isFlipped];
	
	if (!flipped)
	{
		NSRect bounds = [view bounds];
		CGFloat height = bounds.size.height;
		
		pt.y = height - pt.y;
	}	
}

-(void) setCursorPositionToVirtualPositionAndShow
{
    if(!CGPointEqualToPoint(virtualMousePosition, MOUSE_OFFSCREEN_POSITION))
    {
        NSRect usableScreenRect = [[NSScreen mainScreen] frame];
        
        NSPoint outsidePoint = NSMakePoint(virtualMousePosition.x + self.window.frame.origin.x,
                                           virtualMousePosition.y + usableScreenRect.size.height - self.window.frame.origin.y - self.window.frame.size.height + [self titleBarHeight]);
        [self showCursor];
        
        // this code stops the "freezing" of 250 ms when warping cursor outside of Roblox window
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        CGEventSourceSetLocalEventsSuppressionInterval(source, 0.0);
        CGAssociateMouseAndMouseCursorPosition(0);
        CGWarpMouseCursorPosition(CGPointMake(outsidePoint.x,outsidePoint.y));
        CGAssociateMouseAndMouseCursorPosition(1);
        CFRelease(source);
    }
    else
        [self showCursor];
}

-(void) setVirtualCursorPos:(CGPoint) newPos
{
    if(cursorHidden)
    {
        virtualMousePosition = newPos;
        
        // check to see if we have gone out of bounds, if so we need to turn off software rendering of mouse
        if( ![self inFullScreenMode] && !NSPointInRect( NSMakePoint(virtualMousePosition.x,virtualMousePosition.y), [self bounds]) )
            [self setCursorPositionToVirtualPositionAndShow];
    }
}

-(CGPoint) getVirtualCursorPos
{
    return virtualMousePosition;
}

-(BOOL) cursorInViewBounds
{
    if( [self inFullScreenMode] )
        return YES;
    
    NSPoint screenPos = NSMakePoint(virtualMousePosition.x,virtualMousePosition.y);
    //NSLog(@"screenPos is (%f,%f), bounds are %@",virtualMousePosition.x,virtualMousePosition.y, NSStringFromRect( [self bounds] ) );
    
	return NSPointInRect(screenPos, [self bounds]);
}

- (void) hideCursor
{
	if (cursorHidden)
		return;
	
	[NSCursor hide];
    CGAssociateMouseAndMouseCursorPosition(false);
    
	cursorHidden = YES;
}

- (void) showCursor
{
	if (!cursorHidden)
		return;
	
    CGAssociateMouseAndMouseCursorPosition(true);
	[NSCursor unhide];
    
    virtualMousePosition = MOUSE_OFFSCREEN_POSITION; // kinda hack: put sw cursor offscreen
	
	cursorHidden = NO;
}

-(void) finishFullScreenChange
{
    // Get our new dimensions, others will need them
    [self updateBounds];
    
    // We don't seem to get enter/exit events after entering/exiting full screen
    if (![self cursorInViewBounds])
        [self mouseExited:nil];
    
    [self.window makeFirstResponder:self];
}

- (void) toggleFullScreen
{
	fullScreen = !fullScreen;
	
	if (fullScreen)
	{
        NSWindow* wnd = [self window];

        nonFullScreenWindowLevel = wnd.level;
        nonFullScreenRect = wnd.frame;
        
        dispatch_async(dispatch_get_main_queue(),^{
            [wnd setStyleMask:NSBorderlessWindowMask];
            [wnd setFrame:[NSScreen mainScreen].frame display:YES];
            [wnd setLevel:NSStatusWindowLevel];
            
            [self finishFullScreenChange];
        });
	}
	else
	{
        dispatch_async(dispatch_get_main_queue(),^{
            NSWindow *wnd = [self window];
            
            [wnd setStyleMask:NSResizableWindowMask | NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask];
            [wnd setFrame:nonFullScreenRect display:YES];
            [wnd setLevel:nonFullScreenWindowLevel];
            
            [self finishFullScreenChange];
        });
	}
}

- (BOOL) inFullScreenMode
{
	return fullScreen;
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    NSWindow* keyWindow = (NSWindow*)[notification object];
    
    if(keyWindow == [self window])
        [self resignFirstResponder];
    else
        [self.window makeFirstResponder:nil];
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    NSWindow* keyWindow = (NSWindow*)[notification object];
    BOOL firstResponderIsMainWindow = [[keyWindow firstResponder] isMemberOfClass:[RBXWindow class]];
    
    if(keyWindow == [self window] || firstResponderIsMainWindow)
        [self becomeFirstResponder];
}

// Event handlers
- (BOOL)becomeFirstResponder
{
	if (robloxView)
		robloxView->handleFocus(true);
    
    if(!cursorHidden)
    {
        NSPoint loc = [NSEvent mouseLocation];
        NSRect usableScreenRect = [[[NSScreen screens] objectAtIndex:0] visibleFrame];
        
        loc.y = usableScreenRect.size.height - loc.y - [self titleBarHeight];
       
        NSRect localWindowRect = [self bounds];
        NSRect localWindowInAbsolute =  NSMakeRect(_window.frame.origin.x, usableScreenRect.size.height - _window.frame.origin.y - _window.frame.size.height,
                                                   localWindowRect.size.width, localWindowRect.size.height);
        
        if( NSPointInRect(loc,localWindowInAbsolute) )
        {
            [self hideCursor];
            virtualMousePosition = CGPointMake(loc.x - localWindowInAbsolute.origin.x, loc.y - localWindowInAbsolute.origin.y);
            
            if(robloxView)
                robloxView->handleMouseInside(true);
        }
    }
    
	return YES;
}

- (BOOL)resignFirstResponder
{
	if (robloxView)
		robloxView->handleFocus(false);
    
    [self setCursorPositionToVirtualPositionAndShow];
	
	return YES;
}

-(void) setRobloxView:(RobloxView *)rbxview
{
	robloxView = rbxview;
	
	if (robloxView)
	{
		// Tell Roblox window how big it is early
		[self updateBounds];
	}
	else
	{
		// Clean up Ogre stuff
		//[self setOgreWindow:nil];

		// make sure cursor shows up etc
		[self showCursor];
		
	}
}

-(void) setAppDelegate:(NSResponder *)appDel
{
	self->appDelegate = appDel;
    
    if (RBX::GameBasicSettings::singleton().getFullScreen() && !fullScreen)
    {
        [self toggleFullScreen];
    }
}


- (void)viewDidEndLiveResize
{
	[self updateBounds];
}

static RBX::KeyCode getModifierKeyCode(unsigned short keyCode)
{	
	if(keyCode == kVK_CapsLock)
	{
		return RBX::SDLK_CAPSLOCK;
	}  
	if (keyCode == kVK_Shift || keyCode == kVK_RightShift)
	{
		return RBX::SDLK_LSHIFT;
	} 
	if (keyCode == kVK_Control || keyCode == kVK_RightControl)
	{
		return RBX::SDLK_LCTRL;
	}
	if (keyCode == kVK_Option || keyCode == kVK_RightOption)
	{
		return RBX::SDLK_LALT;
	}
	
	return RBX::SDLK_UNKNOWN;
}

static RBX::KeyCode keyCodeTOUIKeyCode(unsigned int c, unsigned int keyCode, NSUInteger modifiers)
{
	// Look for keypad-related, translate (somehow) to numeric keypad codes
	// right now it seems to always behave as if Num Lock is on, sending the number
	// ASCII chars instead of the arrows etc.
	if (modifiers & NSNumericPadKeyMask)
	{
	}
	
	if (c == 127)
		return RBX::SDLK_BACKSPACE;
	
	// try ASCII table. RBX keycodes in ASCII range are mapped exactly
	if (c <= 127)
	{
		// for A-Z we'll need the lower case versions
		if (isalpha(c))
		{
			c = tolower(c);
		}
		
		//NSLog([NSString stringWithFormat:@"keyCodeTOUIKeyCode %d", c]);
		return (RBX::KeyCode) c;
	}
	
	RBX::KeyCode rbxKey = RBX::SDLK_UNKNOWN;
	
	// Now try the gnarlier key codes. The arrows, function keys etc. are mapped to unicode chars in the higher range
	switch(c)
	{
		case NSUpArrowFunctionKey :
			rbxKey = RBX::SDLK_UP;
			break;
		case NSDownArrowFunctionKey :
			rbxKey = RBX::SDLK_DOWN;
			break;
		case NSLeftArrowFunctionKey :
			rbxKey = RBX::SDLK_LEFT;
			break;
		case NSRightArrowFunctionKey :
			rbxKey = RBX::SDLK_RIGHT;
			break;
		case NSF1FunctionKey :
			rbxKey = RBX::SDLK_F1;
			break;
		case NSF2FunctionKey :
			rbxKey = RBX::SDLK_F2;
			break;
		case NSF3FunctionKey :
			rbxKey = RBX::SDLK_F3;
			break;
		case NSF4FunctionKey :
			rbxKey = RBX::SDLK_F4;
			break;
		case NSF5FunctionKey :
			rbxKey = RBX::SDLK_F5;
			break;
		case NSF6FunctionKey :
			rbxKey = RBX::SDLK_F6;
			break;
		case NSF7FunctionKey :
			rbxKey = RBX::SDLK_F7;
			break;
		case NSF8FunctionKey :
			rbxKey = RBX::SDLK_F8;
			break;
		case NSF9FunctionKey :
			rbxKey = RBX::SDLK_F9;
			break;
		case NSF10FunctionKey :
			rbxKey = RBX::SDLK_F10;
			break;
		case NSF11FunctionKey :
			rbxKey = RBX::SDLK_F11;
			break;
		case NSF12FunctionKey :
			rbxKey = RBX::SDLK_F12;
			break;
		case NSF13FunctionKey :
			rbxKey = RBX::SDLK_F13;
			break;
		case NSF14FunctionKey :
			rbxKey = RBX::SDLK_F14;
			break;
		case NSF15FunctionKey :
			rbxKey = RBX::SDLK_F15;
			break;
		case NSInsertFunctionKey :
			rbxKey = RBX::SDLK_INSERT;
			break;
		case NSDeleteFunctionKey :
			rbxKey = RBX::SDLK_DELETE;
			break;
		case NSHomeFunctionKey :
			rbxKey = RBX::SDLK_HOME;
			break;
		case NSEndFunctionKey :
			rbxKey = RBX::SDLK_END;
			break;
		case NSPageUpFunctionKey :
			rbxKey = RBX::SDLK_PAGEUP;
			break;
		case NSPageDownFunctionKey :
			rbxKey = RBX::SDLK_PAGEDOWN;
			break;
		default :
			rbxKey = RBX::SDLK_UNKNOWN;
			break;
	}
	
	return rbxKey;
}

static bool modifierKeyPressed(unsigned short keyCode, NSUInteger modifiers)
{
    if ( (keyCode == kVK_CapsLock) &&  (modifiers & NSAlphaShiftKeyMask) )
        return true;
	if ( (keyCode == kVK_Shift) && (modifiers & NSShiftKeyMask) )
        return true;
	if ( (keyCode == kVK_Control) && (modifiers & NSControlKeyMask) )
        return true;
	if ( (keyCode == kVK_Option) && (modifiers & NSAlternateKeyMask) )
        return true;
	
	return false;
}

static RBX::ModCode modifiersToUIModCode(NSUInteger modifiers)
{
	unsigned int modCode = 0;
	
	if( modifiers & NSAlphaShiftKeyMask )
		modCode = modCode | RBX::KMOD_CAPS; 
	if (modifiers & NSShiftKeyMask)
		modCode = modCode | RBX::KMOD_LSHIFT;
	if (modifiers & NSControlKeyMask)
		modCode = modCode | RBX::KMOD_LCTRL;
	if (modifiers & NSAlternateKeyMask)
		modCode = modCode | RBX::KMOD_LALT;
    if (modifiers & NSCommandKeyMask)
        modCode = modCode | RBX::KMOD_LMETA;
	
	return (RBX::ModCode) modCode;
}

- (void)mouseEntered:(NSEvent *)theEvent
{
	if (robloxView && [[NSApplication sharedApplication] isActive] && self.window.isKeyWindow)
	{
		[self hideCursor];
        
        NSPoint loc;
        if (theEvent)
            loc = [theEvent locationInWindow];
        else
            loc = [[self window] mouseLocationOutsideOfEventStream];
        
        NSPoint myloc = [self convertPoint:loc fromView:nil];
        flipY(self, myloc);

        virtualMousePosition = CGPointMake(myloc.x, myloc.y);
		
		robloxView->handleMouseInside(true);
	}
}

- (void)mouseExited:(NSEvent *)theEvent
{
    if (robloxView && [[NSApplication sharedApplication] isActive])
    {
        //NSLog(@"mouseExited inside");
        
        [self showCursor];
        
        robloxView->handleMouseInside(false);
    }
}

-(BOOL) isMouseOverVisibleWindow
{
    NSPoint absMousePos = NSMakePoint(virtualMousePosition.x + self.window.frame.origin.x,
                                      virtualMousePosition.y + self.window.frame.origin.y);
    NSArray* windows = [[NSApplication sharedApplication] windows];
    
    for(NSWindow* aWindow in windows)
    {
        if (aWindow != self.window &&
            [aWindow isVisible] &&
            aWindow.level >= self.window.level &&
            NSPointInRect(absMousePos, aWindow.frame))
        {
            [aWindow makeKeyAndOrderFront:self];
            [aWindow makeFirstResponder:nil];
            return YES;
        }
    }
    
    return NO;
}

-(void)mouseMoved:(NSEvent *)event
{
    BOOL firstResponderIsMainWindow = [[self.window firstResponder] isMemberOfClass:[RBXWindow class]];
    
    if (robloxView &&
        [[NSApplication sharedApplication] isActive] &&
        [[NSApplication sharedApplication] keyWindow] == self.window &&
        ([self.window firstResponder] == self || firstResponderIsMainWindow) &&
        [self cursorInViewBounds])
    {
        // if we can see some other one of our windows open, give it focus
        if ([self isMouseOverVisibleWindow])
            return;
        
        [[self window] makeFirstResponder:self];
        
        NSPoint loc = [event locationInWindow];
        NSPoint myloc = [self convertPoint:loc fromView:nil];
        flipY(self, myloc);

        if(!cursorHidden)
            [self hideCursor];
        
       // NSLog(@"mouseMoved at %@ (window) %@ (view) (%f,%f) (delta)", NSStringFromPoint(loc), NSStringFromPoint(myloc),[event deltaX],[event deltaY]);
        
        if (robloxView)
            robloxView->handleMouse(RobloxView::MOUSE_MOVE, [event deltaX], [event deltaY], modifiersToUIModCode([event modifierFlags]));
    }
}


-(void)mouseDown:(NSEvent *)event
{
    if (event.modifierFlags & NSControlKeyMask)
	{
		controlKeyWasDown = YES;
        return [self rightMouseDown:event];
	}

	NSPoint loc = [event locationInWindow];
	NSPoint myloc = [self convertPoint:loc fromView:nil];
    
	// flip mouse coords from Y-up to Y-down
	flipY(self, myloc);
    
    if(!cursorHidden)
    {
        [self hideCursor];
        
        virtualMousePosition = CGPointMake(myloc.x, myloc.y);
		robloxView->handleMouseInside(true);
    }

    //NSLog(@"mouseDown at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));

	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_LEFT_BUTTON_DOWN, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
}

-(void)mouseUp:(NSEvent *)event
{
    if (event.modifierFlags & NSControlKeyMask || controlKeyWasDown)
	{
		controlKeyWasDown = false;
        return [self rightMouseUp:event];
	}
	
	NSPoint loc = [event locationInWindow];
	NSPoint myloc = [self convertPoint:loc fromView:nil];
	
	// NSLog(@"mouseUp at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));
    
	// possibly flip mouse coords from Y-up to Y-down
	flipY(self, myloc);
	
	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_LEFT_BUTTON_UP, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
}

-(void)mouseDragged:(NSEvent *)event
{
    if (event.modifierFlags & NSControlKeyMask || controlKeyWasDown)
        return [self rightMouseDragged:event];
	
	NSPoint loc = [event locationInWindow];
	NSPoint myloc = [self convertPoint:loc fromView:nil];

    //NSLog(@"mouseDragged at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));

	// possibly flip mouse coords from Y-up to Y-down
	flipY(self, myloc);
	
	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_MOVE, [event deltaX], [event deltaY], modifiersToUIModCode([event modifierFlags]));
}

-(void)rightMouseDown:(NSEvent *)event
{
	[[self window] makeFirstResponder:self];

	NSPoint loc = [event locationInWindow];
	NSPoint myloc = [self convertPoint:loc fromView:nil];
	
    //NSLog(@"mouseDown at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));
	
	// possibly flip mouse coords from Y-up to Y-down
	flipY(self, myloc);
	
	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_RIGHT_BUTTON_DOWN, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
}

-(void)rightMouseDragged:(NSEvent *)event
{
	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_MOVE, [event deltaX], [event deltaY], modifiersToUIModCode([event modifierFlags]));
}

-(void)rightMouseUp:(NSEvent *)event
{
	NSPoint loc = [event locationInWindow];
	NSPoint myloc = [self convertPoint:loc fromView:nil];
	
	// NSLog(@"mouseUp at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));	
	
	// possibly flip mouse coords from Y-up to Y-down
	flipY(self, myloc);
	
	if (robloxView)
		robloxView->handleMouse(RobloxView::MOUSE_RIGHT_BUTTON_UP, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
}

-(void)otherMouseDown:(NSEvent *)event
{
	if (DFFlag::MiddleMouseButtonEvent)
	{
		if ([event buttonNumber] == (NSInteger)2)
		{
			[[self window] makeFirstResponder:self];

			NSPoint loc = [event locationInWindow];
			NSPoint myloc = [self convertPoint:loc fromView:nil];
			
			//NSLog(@"mouseDown at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));
			
			// possibly flip mouse coords from Y-up to Y-down
			flipY(self, myloc);
			
			if (robloxView)
				robloxView->handleMouse(RobloxView::MOUSE_MIDDLE_BUTTON_DOWN, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
		}
	}
}

-(void)otherMouseDragged:(NSEvent *)event
{
	if (DFFlag::MiddleMouseButtonEvent)
	{
		if (robloxView)
			robloxView->handleMouse(RobloxView::MOUSE_MOVE, [event deltaX], [event deltaY], modifiersToUIModCode([event modifierFlags]));
	}
}

-(void)otherMouseUp:(NSEvent *)event
{
	if (DFFlag::MiddleMouseButtonEvent)
	{
		if ([event buttonNumber] == (NSInteger)2)
		{
			NSPoint loc = [event locationInWindow];
			NSPoint myloc = [self convertPoint:loc fromView:nil];
			
			// NSLog(@"mouseUp at %@ (window) %@ (view)", NSStringFromPoint(loc), NSStringFromPoint(myloc));	
			
			// possibly flip mouse coords from Y-up to Y-down
			flipY(self, myloc);
			
			if (robloxView)
				robloxView->handleMouse(RobloxView::MOUSE_MIDDLE_BUTTON_UP, (int) myloc.x, (int) myloc.y, modifiersToUIModCode([event modifierFlags]));
		}
	}
}

-(void)keyDown:(NSEvent *)event
{
    if( ![event isARepeat])
    {
        NSString *chars = [event characters];
        unsigned short keyCode = [event keyCode];
        NSUInteger modifiers = [event modifierFlags];
        
        if (robloxView)
        {
            NSUInteger len = [chars length];
            if (len > 0)
            {
                RBX::KeyCode rbxKey = keyCodeTOUIKeyCode([chars characterAtIndex:0], keyCode, modifiers);
                robloxView->handleKey(RobloxView::KEY_DOWN, rbxKey, modifiersToUIModCode(modifiers));
            }
            
        }
    }
}

-(void)flagsChanged:(NSEvent *)event
{
	RBX::KeyCode modKeyCode = getModifierKeyCode([event keyCode]);
	RBX::ModCode UIModCode = modifiersToUIModCode([event modifierFlags]);
    
	if( modifierKeyPressed([event keyCode], [event modifierFlags]) )
        robloxView->handleKey(RobloxView::KEY_DOWN, modKeyCode, UIModCode);
	else
        robloxView->handleKey(RobloxView::KEY_UP, modKeyCode, UIModCode);
}

-(void)keyUp:(NSEvent *)event
{
    if(![event isARepeat])
    {
        NSString *chars = [event characters];
        NSUInteger modifiers = [event modifierFlags];
        
        if (robloxView)
        {
            NSUInteger len = [chars length];
            if (len > 0)
            {
                RBX::KeyCode keyCode = keyCodeTOUIKeyCode([chars characterAtIndex:0], [event keyCode], modifiers);
                robloxView->handleKey(RobloxView::KEY_UP, keyCode, modifiersToUIModCode(modifiers));
            }
            
        }
	}
}

#define RBXDELTAY 120.f

-(void)scrollWheel:(NSEvent *)theEvent
{
    // Get scroll info
	CGFloat deltaY = [theEvent deltaY];
    CGFloat deltaX = [theEvent deltaX];
    // NSLog(@"Got a scroll deltaX:%g deltaY:%g deltaZ:%g", deltaX, deltaY, deltaZ);
    if (robloxView)
    {
        //NSLog (@"Transducing deltaY to %g", rbxDeltaY);
        if (deltaY)
        {
            // Convert (bizarre) Cocoa deltaY values to (even more bizarre) Windows values
            float rbxDeltaY = deltaY > 0.f ? RBXDELTAY : -RBXDELTAY;
            robloxView->handleScrollWheel(rbxDeltaY, virtualMousePosition.x,  virtualMousePosition.y);
        }
        // Horizontal scrolling reports as Delta in X-Axis
        else if (deltaX)
        {
            float rbxDeltaX = deltaX > 0.f ? RBXDELTAY : -RBXDELTAY;
            robloxView->handleScrollWheel(rbxDeltaX, virtualMousePosition.x,  virtualMousePosition.y);
        }
    }
}


@end
