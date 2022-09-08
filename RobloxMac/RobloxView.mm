//
//  RobloxView.mm
//  MacClient
//
//  Created by Tony on 2/9/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

#import "RobloxOgreView.h"

#include "RobloxView.h"
#include "FunctionMarshaller.h"
#include "RobloxPlayerAppDelegate.h"

inline void flipY(NSView *view, NSPoint &pt)
{
	BOOL flipped = [view isFlipped];
	
	if (!flipped)
	{
		NSRect bounds = [view bounds];
		CGFloat height = bounds.size.height;
		
		pt.y = height - pt.y;
	}	
}

void RobloxView::getCursorPos(G3D::Vector2 *pPos)
{
    if(RobloxOgreView* ogreView = (RobloxOgreView*) viewWindow)
    {
        CGPoint virtualCursorPos = [ogreView getVirtualCursorPos];
        pPos->x = virtualCursorPos.x;
        pPos->y = virtualCursorPos.y;
    }
}

void RobloxView::setCursorPos(G3D::Vector2 pPos)
{
    if(RobloxOgreView* ogreView = (RobloxOgreView*) viewWindow)
        [ogreView setVirtualCursorPos:CGPointMake(G3D::iRound(pPos.x), G3D::iRound(pPos.y))];
}

void RobloxView::marshalTeleport(std::string url, std::string ticket, std::string script)
{
	marshaller->Submit(boost::bind(&RobloxView::doTeleport, this, url, ticket, script));
}

void RobloxView::doTeleport(std::string url, std::string ticket, std::string script)
{
	RobloxPlayerAppDelegate *appDelegate = (RobloxPlayerAppDelegate*)appWindow;

	NSString *ticketString = [NSString stringWithCString:ticket.c_str() encoding:[NSString defaultCStringEncoding]];
	NSString *urlString = [NSString stringWithCString:url.c_str() encoding:[NSString defaultCStringEncoding]];
	NSString *scriptString = [NSString stringWithCString:script.c_str() encoding:[NSString defaultCStringEncoding]];
	
	[appDelegate teleport:ticketString withAuthentication:urlString withScript:scriptString];
}

bool RobloxView::isFullscreenMode()
{
    if(RobloxOgreView* ogreView = (RobloxOgreView*) viewWindow)
        return ogreView.fullScreen;
    
    return false;
}