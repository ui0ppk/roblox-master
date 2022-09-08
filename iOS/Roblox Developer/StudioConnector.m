//
//  StudioConnector.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/19/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "StudioConnector.h"

#import <CFNetwork/CFNetwork.h>
#import <netdb.h>

#define RBX_DEV_PORT 1313

@implementation StudioConnector

-(id) init
{
    if (self = [super init])
    {
        codeToUse = [[NSUserDefaults standardUserDefaults] stringForKey:@"RbxPairCode"];
        hasPaired = NO;
        udpSocket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self  delegateQueue:dispatch_get_main_queue()];
    }
    return self;
}

+ (id)sharedInstance
{
    static dispatch_once_t studioConnectorPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&studioConnectorPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(NSString*) getPairEndedNotificationString
{
    return @"RbxDevPairEndedNotification";
}

-(void) tryToPairWithStudioUsingCurrentCode
{
    codeToUse = [[NSUserDefaults standardUserDefaults] stringForKey:@"RbxPairCode"];
    if(!codeToUse)
        codeToUse = @"";
    
    [self tryToPairWithStudio:codeToUse];
}

-(void) tryToPairWithStudio:(NSString*) newCode
{
    codeToUse = newCode;
    [self doPairWithStudio];
}

-(void) checkIfPaired
{
    if (!hasPaired)
    {
        NSDictionary* dict = [[NSDictionary alloc] initWithObjectsAndKeys:@"",@"ipData",@"",@"ipString",@"false",@"didPair",@"timeout",@"errorReason", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:[self getPairEndedNotificationString] object:self userInfo:dict];
        return;
    }
}

-(void) doPairWithStudio
{
    hasPaired = NO;
    
    if (!udpSocket)
    {
        udpSocket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self  delegateQueue:dispatch_get_main_queue()];
    }
    
    
    NSError* bindErr;
    [udpSocket bindToPort:RBX_DEV_PORT error:&bindErr];
    
    if (bindErr)
    {
        [self stopPairing];
        NSDictionary* dict = [[NSDictionary alloc] initWithObjectsAndKeys:@"",@"ipData",@"",@"ipString",false,@"didPair", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:[self getPairEndedNotificationString] object:self userInfo:dict];
        return;
    }
    
    NSError* err;
    [udpSocket beginReceiving:&err];
    
    if(err)
    {
        [self stopPairing];
        NSDictionary* dict = [[NSDictionary alloc] initWithObjectsAndKeys:@"",@"ipData",@"",@"ipString",false,@"didPair", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:[self getPairEndedNotificationString] object:self userInfo:dict];
        return;
    }
}

-(void) stopPairing
{
    [udpSocket close];
    udpSocket = nil;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        hasPaired = NO;
    });
}

- (NSString *)hostnameForAddress:(NSString *)address
{
    int error;
    struct addrinfo *results = NULL;
    
    error = getaddrinfo([address cStringUsingEncoding:NSUTF8StringEncoding], NULL, NULL, &results);
    if (error != 0)
    {
        return @"";
    }
    
    for (struct addrinfo *r = results; r; r = r->ai_next)
    {
        char hostname[NI_MAXHOST] = {0};
        error = getnameinfo(r->ai_addr, r->ai_addrlen, hostname, sizeof hostname, NULL, 0 , 0);
        if (error != 0)
        {
            continue; // try next one
        }
        else
        {
            return [NSString stringWithCString:hostname encoding:NSUTF8StringEncoding];
        }
    }
    
    return @"";
}

-(bool) sendPairCodeToServer:(NSString*) messageFromServer serverAddress:(NSString*) address serverPort:(unsigned short) port
{
    if ([messageFromServer isEqualToString:@"RbxDevPairServer readyToPair"])
    {
        if(!codeToUse)
        {
            NSData *d = [@"RbxDevClient didPair false" dataUsingEncoding:NSUTF8StringEncoding];
            [udpSocket sendData:d toHost:address port:port withTimeout:-1 tag:101];
            return false;
        }
        
        NSString* pairString = [@"RbxDevClient pairCode " stringByAppendingString:codeToUse];
        NSData *d = [pairString dataUsingEncoding:NSUTF8StringEncoding];
        
        [udpSocket sendData:d toHost:address port:port withTimeout:-1 tag:11];
        return true;
    }
    
    return false;
}

-(void) clearPairCode
{
    [[NSUserDefaults standardUserDefaults] setObject:@"" forKey:@"RbxPairCode"];
}

-(bool) checkPairStatusFromServer:(NSString*) messageFromServer addressData:(NSData*) data serverAddress:(NSString*) address serverPort:(unsigned short) port
{
    NSRange isRange = [messageFromServer rangeOfString:@"RbxDevPairServer didPair" options:NSCaseInsensitiveSearch];
    if(isRange.location != NSNotFound)
    {
        NSMutableArray *array = (NSMutableArray *)[messageFromServer componentsSeparatedByString:@" "];
        [array removeObject:@""];
        
        if ([array count] == 5)
        {
            NSString* didPair = array[2];
            
            hasPaired = YES;
            
            if ([didPair isEqualToString:@"true"])
            {
                
                [[NSUserDefaults standardUserDefaults] setObject:codeToUse forKey:@"RbxPairCode"];
                
                hostIPAddressString = array[4];
                hostIPAddress = data;
                didPairToHost = @"true";
                
                NSData *d = [@"RbxDevClient didPair true" dataUsingEncoding:NSUTF8StringEncoding];
                [udpSocket sendData:d toHost:address port:port withTimeout:-1 tag:101];
            }
            else
            {
                hostIPAddressString = @"";
                hostIPAddress = nil;
                didPairToHost = @"false";
                
                NSData *d = [@"RbxDevClient didPair false" dataUsingEncoding:NSUTF8StringEncoding];
                [udpSocket sendData:d toHost:address port:port withTimeout:-1 tag:101];
            }
            return true;
        }
    }
    
    return false;
}

-(bool) checkGameStartFromServer:(NSString*) messageFromServer serverAddress:(NSString*) address serverPort:(unsigned short) port
{
    NSRange isRange = [messageFromServer rangeOfString:@"RbxDevPairServer didStartServer" options:NSCaseInsensitiveSearch];
    if(isRange.location != NSNotFound)
    {
        hasPaired = YES;
        
        NSData* d = [@"RbxDevClient didConnectToServer" dataUsingEncoding:NSUTF8StringEncoding];
        [udpSocket sendData:d toHost:address port:port withTimeout:-1 tag:101];
        return true;
    }
    
    return false;
}



/**
 * By design, UDP is a connectionless protocol, and connecting is not needed.
 * However, you may optionally choose to connect to a particular host for reasons
 * outlined in the documentation for the various connect methods listed above.
 *
 * This method is called if one of the connect methods are invoked, and the connection is successful.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didConnectToAddress:(NSData *)address
{
}

/**
 * By design, UDP is a connectionless protocol, and connecting is not needed.
 * However, you may optionally choose to connect to a particular host for reasons
 * outlined in the documentation for the various connect methods listed above.
 *
 * This method is called if one of the connect methods are invoked, and the connection fails.
 * This may happen, for example, if a domain name is given for the host and the domain name is unable to be resolved.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didNotConnect:(NSError *)error
{
}

/**
 * Called when the datagram with the given tag has been sent.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didSendDataWithTag:(long)tag
{
    if (tag == 101)
    {
        // if we actually paired somewhere, stop the pairing, otherwise keep going
        if ([didPairToHost isEqualToString:@"true"])
        {
            [self stopPairing];
        }
        
        NSDictionary* dict = [[NSDictionary alloc] initWithObjectsAndKeys:hostIPAddress,@"ipData",hostIPAddressString,@"ipString",didPairToHost,@"didPair", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:[self getPairEndedNotificationString] object:self userInfo:dict];
    }
}

/**
 * Called if an error occurs while trying to send a datagram.
 * This could be due to a timeout, or something more serious such as the data being too large to fit in a sigle packet.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didNotSendDataWithTag:(long)tag dueToError:(NSError *)error
{
}


/**
 * Called when the socket has received the requested datagram.
 **/
- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(id)filterContext
{
    if (hasPaired)
    {
        return;
    }
    
    NSString* message =  [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

    NSString *host = [GCDAsyncUdpSocket hostFromAddress:address];
    NSArray* ipComponents = [host componentsSeparatedByString:@":"];
    NSString* ipv4Address = [ipComponents lastObject];
    const unsigned short port = [GCDAsyncUdpSocket portFromAddress:address];
    
    // 1) check the broadcast we got from server, see if we should send code
    if( [self sendPairCodeToServer:message serverAddress:ipv4Address serverPort:port] )
    {
        return;
    }
    
    // 2) check if our pair code was verified from server
    if ( [self checkPairStatusFromServer:message addressData:address serverAddress:ipv4Address serverPort:port])
    {
        return;
    }
    
    // 3) check if server spun up a game instance we should connect to
    if ( [self checkGameStartFromServer:message serverAddress:ipv4Address serverPort:port] )
    {
        return;
    }
}

/**
 * Called when the socket is closed.
 **/
- (void)udpSocketDidClose:(GCDAsyncUdpSocket *)sock withError:(NSError *)error
{
}

@end
