//
//  GameKitHelper.m
//  RobloxMobile
//
//  Created by Ganesh on 5/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameKitHelper.h"
#import "OnlineGameViewController.h"
#import "RobloxInfo.h"
#import "LoginManager.h"


@interface NSString (URLEncoding)
-(NSString *)urlEncodeUsingEncoding:(NSStringEncoding)encoding;
@end


@implementation NSString (URLEncoding)
-(NSString *)urlEncodeUsingEncoding:(NSStringEncoding)encoding {
	return (NSString *)CFURLCreateStringByAddingPercentEscapes(NULL,
                                                               (CFStringRef)self,
                                                               NULL,
                                                               (CFStringRef)@"!*'\"();:@&=+$,/?%#[]% ",
                                                               CFStringConvertNSStringEncodingToEncoding(encoding));
}
@end


@implementation GameKitHelper


+(id) sharedInstance {
    static dispatch_once_t onceToken = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&onceToken, ^{
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    if(self = [super init])
    {
        gameCenterActiveNotification = [[NSString alloc] initWithString:@"RBXGameCenterActiveNotifier"];
        gameCenterDisabledNotification = [[NSString alloc] initWithString:@"RBXGameCenterDisabledNotifier"];
    }
    
    return self;
}

-(void) dealloc
{
    [gameCenterActiveNotification release];
    [gameCenterDisabledNotification release];
    [super dealloc];
}

-(NSString*) getGameCenterActiveNotification
{
    return gameCenterActiveNotification;
}

-(NSString*) getGameCenterDisabledNotification
{
    return gameCenterDisabledNotification;
}




-(void) authenticateLocalPlayer
{

    GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
    [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterActiveNotification object:self userInfo:nil];
    
    localPlayer.authenticateHandler = ^(UIViewController *viewController, NSError *error)
    {
        
        // Apple has auntheticated the GC User
        if (localPlayer.authenticated)
        {
            NSLog(@"Logged in with GameCenter");
            
            // Ask Apple to generate the signature, certificate for our server verification if he is authenticated Game Center with the player id
            [localPlayer generateIdentityVerificationSignatureWithCompletionHandler:^(NSURL *publicKeyUrl, NSData *signature, NSData *salt, uint64_t timestamp, NSError *error) {
                
                // Apple server errored out
                if (error)
                {
                    [[LoginManager sharedInstance] logoutRobloxUser];
                    NSLog(@"Apple Game Center Server Errored out: %@", error.description);
                    [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                }
                else // Apple Server cert generation successful
                {
                    
#if 0
                    NSLog(@"Public Key URL: %@", publicKeyUrl);
                    NSLog(@"Timestamp: %@", [NSString stringWithFormat:@"%llu", timestamp]);
                    NSLog(@"Signature: %@", [signature base64EncodedStringWithOptions:0]);
                    NSLog(@"Salt: %@", [salt base64EncodedStringWithOptions:0]);
                    NSLog(@"Player ID: %@", localPlayer.playerID);
                    NSLog(@"Display Name: %@", localPlayer.displayName);
                    NSLog(@"Alias Name: %@", localPlayer.alias);
                    NSLog(@"Bundle ID: %@", [[NSBundle mainBundle] bundleIdentifier]);
                    NSLog(@"Is UnderAge: %@", localPlayer.isUnderage ? @"Yes" : @"No" );
#endif
                    
                    
                    
                    //https://api.sitetest3.robloxlabs.com/apple-game-center/authorize?playerId=G:2030020262&signature=V9GPDkwA+k3BYfiYLbauEwkTZP8yiw2YEJKFdmCukcyKmsTQZmvT4WAqbRL6QNrqLzXAn41veo9Tg6XWjv0WgXm7bPXNzTkIXjmLBUHXg0Y8cogxr8bQe+ClNev0FMgmU9eX+kcYdFy4U0124KdmRYYobq2e1RViiI4pM9oqWFg3DQ+rouAqIK15S3zXAM4ZaUt+qYFnqApp0nlOjL3vZCtrwybCrWphx6SWoWOOa3RtZ44vl/F192bFumw2/KFt6mwOgOWlVn5Fvk55gIJ0faQs96/0MFhpskkHr5e2ZYjhlXU78e63m2GplZlDcjs6s/Zo3yTVhJiKu0W3byRulg==&publicKeyUrl=https://sandbox.gc.apple.com/public-key/gc-sb.cer&salt=12NX6A==&timestamp=1400192105946&bundleid=com.roblox.Age-of-Kings
                    
                    
                    // URL for sigin of Game Center user, assuming the GameCenter equivalent account is already created with Roblox
                    NSString* finalURL =[NSString stringWithFormat:@"%@/apple/game-center/signin?playerId=%@&signature=%@&bundleid=%@&publicKeyUrl=%@&salt=%@&timestamp=%@"
                                         , [RobloxInfo getApiBaseUrl]
                                         , [localPlayer.playerID urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                         , [[signature base64EncodedStringWithOptions:0] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                         , [[[NSBundle mainBundle] bundleIdentifier] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                         , publicKeyUrl
                                         , [[salt base64EncodedStringWithOptions:0] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                         , [NSString stringWithFormat:@"%llu", timestamp]
                                         ];
                    
                   
                   
#if 0
                    NSLog(@" URL: %@", finalURL);
                    NSLog(@"UserAgent: %@", [RobloxInfo getUserAgentString]);
#endif
     
                    NSURL *url = [NSURL URLWithString: finalURL];
                    
                    NSMutableURLRequest *theRequest = [NSMutableURLRequest  requestWithURL:url
                                                                               cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                                           timeoutInterval:60*7];
                    
                    
                    
                    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
                    [theRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
                    [theRequest setHTTPMethod:@"POST"];
                    
                    // Make the signin request with Roblox servers
                    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
                    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
                     ^(NSURLResponse *response, NSData *receiptResponseData, NSError *error)
                     {
                         // Got the response
                         NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
                         if ([urlResponse statusCode] == 200) // Successful 200 Status Code
                         {
                             NSDictionary* dict = [[NSDictionary alloc] init];
                             NSError* error = nil;
                             dict = [NSJSONSerialization JSONObjectWithData:receiptResponseData options:kNilOptions error:&error];
                             if (!error) // No Error processsing json data from our servers
                             {

                                 NSNumber * n = [dict valueForKey:@"success"];
                                 BOOL success = [n boolValue];
                                 
                                 if (success) // GC User exists on Roblox Server, do the Login
                                 {
#if 0
                                     NSArray * all = [NSHTTPCookie cookiesWithResponseHeaderFields:[urlResponse allHeaderFields] forURL:[NSURL URLWithString:[RobloxInfo getBaseUrl]]];
                                     NSLog(@"\n\n\n****Cookies Recd Start****\n");
                                     for (NSHTTPCookie *each in all)
                                     {
                                         NSLog(@"Name: %@ : Value: %@ Date: %@ Path: %@ Domain: %@ HTTP Only: %@\n\n", each.name, each.value, each.expiresDate, each.path, each.domain, (each.isHTTPOnly ? @"True" : @"False"));
                                     }
                                     NSLog(@"\n****Cookies Recd End****\n\n\n");
#endif
                                     [[LoginManager sharedInstance] doGameCenterLogin];
                                 } // GC User exists on Roblox Server, do the Login
                                 else // GC User do not exist exists on Roblox Server, do a sign up on Roblox servers by creating account
                                 {
                                     // Try doing a signup now
                                     NSString* finalURL =[NSString stringWithFormat:@"%@/apple/game-center/signup?playerId=%@&isUnderAge=%@&signature=%@&bundleid=%@&publicKeyUrl=%@&salt=%@&timestamp=%@"
                                                          , [RobloxInfo getApiBaseUrl]
                                                          , [localPlayer.playerID urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                                          , localPlayer.isUnderage ? @"true" : @"false"
                                                          , [[signature base64EncodedStringWithOptions:0] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                                          , [[[NSBundle mainBundle] bundleIdentifier] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                                          , publicKeyUrl
                                                          , [[salt base64EncodedStringWithOptions:0] urlEncodeUsingEncoding:NSUTF8StringEncoding]
                                                          , [NSString stringWithFormat:@"%llu", timestamp]
                                                          ];
                                     
                                     
                                     
#if 0
                                     NSLog(@" URL: %@", finalURL);
                                     NSLog(@"UserAgent: %@", [RobloxInfo getUserAgentString]);
#endif
                                     
                                     NSURL *url = [NSURL URLWithString: finalURL];
                                     
                                     NSMutableURLRequest *theRequest = [NSMutableURLRequest  requestWithURL:url
                                                                                                cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                                                            timeoutInterval:60*7];
                                     
                                     
                                     [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
                                     [theRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
                                     [theRequest setHTTPMethod:@"POST"];
                                     
                                     NSOperationQueue *queue = [[NSOperationQueue alloc] init];
                                     [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
                                      ^(NSURLResponse *response, NSData *receiptResponseData, NSError *error)
                                      { // Got response
                                          NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
                                          if ([urlResponse statusCode] == 200) // successful response
                                          {
                                              
                                              NSDictionary* dict = [[NSDictionary alloc] init];
                                              NSError* error = nil;
                                              dict = [NSJSONSerialization JSONObjectWithData:receiptResponseData options:kNilOptions error:&error];
                                              if (!error) // No Error processsing json data from our servers
                                              {
                                                  
                                                  NSNumber * n = [dict valueForKey:@"success"];
                                                  BOOL success = [n boolValue];
                                                  
                                                  if (success) // successful sign up, do a login
                                                  {
#if 0
                                                      NSArray * all = [NSHTTPCookie cookiesWithResponseHeaderFields:[urlResponse allHeaderFields] forURL:[NSURL URLWithString:[RobloxInfo getBaseUrl]]];
                                                      NSLog(@"\n\n\n****Cookies Recd Start****\n");
                                                      for (NSHTTPCookie *each in all)
                                                      {
                                                          NSLog(@"Name: %@ : Value: %@ Date: %@ Path: %@ Domain: %@ HTTP Only: %@\n\n", each.name, each.value, each.expiresDate, each.path, each.domain, (each.isHTTPOnly ? @"True" : @"False"));
                                                      }
                                                      NSLog(@"\n****Cookies Recd End****\n\n\n");
#endif
                                                      [[LoginManager sharedInstance] doGameCenterLogin];
                                                  } // successful sign up, do a login
                                                  else // unsuccessful with signup
                                                  {
                                                      [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                                                      [[LoginManager sharedInstance] logoutRobloxUser];
                                                  } // unsuccessful with signup
                                              }
                                              else // Error processsing json data from our servers
                                              {
                                                  NSLog(@"Error in GC JSON Data with Roblox Signup: %@", [error description]);
                                                  [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                                                  [[LoginManager sharedInstance] logoutRobloxUser];
                                              } // Error processsing json data from our servers
                                              
                                          } // successful response
                                          else // non successful response
                                          {
                                              NSLog(@"GC Failed Response Code %d", [urlResponse statusCode]);
                                              [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                                              [[LoginManager sharedInstance] logoutRobloxUser];
                                          }
                                      }]; // Got response
                                 } // GC User do not exist exists on Roblox Server, do a sign up on Roblox servers by creating account
                             } // No Error processsing json data from our servers
                             else // Error processsing json data from our servers
                             {
                                 NSLog(@"Error in GC JSON Data from Roblox Servers: %@", [error description]);
                                 [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                                 [[LoginManager sharedInstance] logoutRobloxUser];
                             } // Error processsing json data from our servers
                             
                         } // Successful 200 Status Code
                         else
                         {
                             NSLog(@"GC Failed Response Code %d", [urlResponse statusCode]);
                             [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
                             [[LoginManager sharedInstance] logoutRobloxUser];
                         }
                     }]; // Make the signin request with Roblox servers

                } // Apple Server cert generation successful
                
            }]; // Ask Apple to generate the signature, certificate for our server verification if he is authenticated Game Center with the player id
            
        }        // Apple has auntheticated the GC User
        else if(viewController) // Apple is going to ask the user to enter the user name and passord for GC
        {
            [[LoginManager sharedInstance] logoutRobloxUser];
            NSLog(@"Ask to login with Game Center");
            [self presentViewController:viewController];
        } else // Apple has decided not to show the GC login after user has dismissed three times with cancel
        {
            NSLog(@"Disable Game Center");
            [[NSNotificationCenter defaultCenter] postNotificationName:gameCenterDisabledNotification object:self userInfo:nil];
        }
    };
}


-(void)presentViewController:(UIViewController*)vc
{
    [[[[[UIApplication sharedApplication] delegate] window] rootViewController] presentViewController:vc animated:YES completion:nil];
}

@end