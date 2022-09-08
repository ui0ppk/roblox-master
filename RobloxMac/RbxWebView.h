//
//  RbxWebView.h
//  MacClient
//
//  Created by Ben Tkacheff on 9/25/13.
//
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

typedef enum
{
    ROBLOX_APP_SHUTDOWN_CODE_LEAVE_GAME,
    ROBLOX_APP_SHUTDOWN_CODE_WINDOW_CLOSE,
    ROBLOX_APP_SHUTDOWN_CODE_QUIT
} RobloxAppShutdownCode;

typedef enum
{
    GAMETYPE_PLAY,
    GAMETYPE_PLAY_PROTOCOL
} RobloxGameType;

typedef enum
{
    SUCCESS,
    FAILED,
    RETRY,
    GAME_FULL,
    USER_LEFT,
} RequestPlaceInfoResult;

@interface RbxWebView : NSWindowController
{
    WebView *webView;
}

-(id) initWithWindowNibName:(NSString*) name;
-(void) dealloc;

@property (weak) IBOutlet WebView *webView;

@end
