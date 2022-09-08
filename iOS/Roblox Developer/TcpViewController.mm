//
//  TcpViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/17/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "TcpViewController.h"

@interface TcpViewController ()

@end

@implementation TcpViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

-(void) dealloc
{
    [self tryToDestroyStreams];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    host = @"";
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
-(void) resetStreamsOnMainThread
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self resetStreams];
    });
}

-(void) resetStreams
{
    [self tryToDestroyStreams];
    [self listenForActions:host];
}

-(void) tryToDestroyStreams
{
    if(inputStream)
    {
        [inputStream close];
        [inputStream setDelegate:nil];
        inputStream = nil;
    }
    if(outputStream)
    {
        [outputStream close];
        [outputStream setDelegate:nil];
        outputStream = nil;
    }
}

-(BOOL) listenForActions:(NSString*) newHost
{
    if (newHost == nil)
    {
        return NO;
    }
    if ((NSNull *) newHost == [NSNull null])
    {
        return NO;
    }
    if (newHost.length <= 0)
    {
        return NO;
    }
    
    if (outputStream || inputStream)
    {
        [self tryToDestroyStreams];
    }
    
    host = newHost;
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;
    CFStreamCreatePairWithSocketToHost(kCFAllocatorDefault, (__bridge CFStringRef)host, 1315, &readStream, &writeStream);
    
    if (!writeStream || !readStream)
    {
        return NO;
    }
    
    CFWriteStreamOpen(writeStream);
    CFReadStreamOpen(readStream);
    
    inputStream = objc_unretainedObject(readStream);
    outputStream = objc_unretainedObject(writeStream);
    
    [inputStream setDelegate:self];
    [outputStream setDelegate:self];
    
    [inputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [outputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    
    [inputStream open];
    [outputStream open];
    
    CFRelease(writeStream);
    CFRelease(readStream);
    
    return YES;
}

-(NSArray*) convertStreamToArray
{
    uint8_t buffer[1024];
    
    if ([inputStream hasBytesAvailable])
    {
        int len = [inputStream read:buffer maxLength:sizeof(buffer)];
        if (len > 0)
        {
            NSString *output = [[NSString alloc] initWithBytes:buffer length:len encoding:NSASCIIStringEncoding];
            
            if (nil != output)
            {
                NSMutableCharacterSet* separatorSet = [NSMutableCharacterSet whitespaceAndNewlineCharacterSet];
                NSMutableArray *words = [[output componentsSeparatedByCharactersInSet:separatorSet] mutableCopy];
                return words;
            }
        }
    }
    
    return nil;
}

-(void) writeToOutputStream:(NSString*) str
{
    if (!outputStream)
    {
        return;
    }
    
    NSData* data = [str dataUsingEncoding:NSUTF8StringEncoding];//str is my string to send
    int byteIndex = 0;
    uint8_t *readBytes = (uint8_t *)[data bytes];
    readBytes += byteIndex; // instance variable to move pointer
    int data_len = [data length];
    unsigned int len = ((data_len - byteIndex >= 1024) ?
                        1024 : (data_len-byteIndex));
    uint8_t buf[len];
    (void)memcpy(buf, readBytes, len);
    len = [outputStream write:(const uint8_t *)buf maxLength:len];
    byteIndex += len;
}


@end
