//
//  RobloxJailbreakDetector.mm
//  RobloxMobile
//
//  Created by Martin Robaszewski on 7/16/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>
#import "RobloxJailbreakDetector.h"
#include "stdio.h"

// base64 taken from the internets
// http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
//

static const char  table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

BOOL isbase64(char c)
{
    return c && strchr(table, c) != NULL;
}

inline char value(char c)
{
    const char *p = strchr(table, c);
    if(p) {
        return p-table;
    } else {
        return 0;
    }
}

int UnBase64(unsigned char *dest, const unsigned char *src, int srclen)
{
    *dest = 0;
    if(*src == 0)
    {
        return 0;
    }
    unsigned char *p = dest;
    do
    {
        
        char a = value(src[0]);
        char b = value(src[1]);
        char c = value(src[2]);
        char d = value(src[3]);
        *p++ = (a << 2) | (b >> 4);
        *p++ = (b << 4) | (c >> 2);
        *p++ = (c << 6) | d;
        if(!isbase64(src[1]))
        {
            p -= 2;
            break;
        }
        else if(!isbase64(src[2]))
        {
            p -= 2;
            break;
        }
        else if(!isbase64(src[3]))
        {
            p--;
            break;
        }
        src += 4;
        while(*src && (*src == 13 || *src == 10)) src++;
    }
    while(srclen-= 4);
    *p = 0;
    return p-dest;
}

//
//
//

#define JB_FEATURE_BITFLIP 1
#define JB_FEATURE_BASE64 1

void jbDecodeString(const unsigned char * input, unsigned char * output)
{
    // 1. get len by reading thru aray until 0x00 is found
    int len = 0;
    while (input[len] != 0)
    {
        len++;
    }
    
    // temp buffer
    unsigned char * temp = (unsigned char *)malloc((len+1) * sizeof(unsigned char));
    memset(temp, 0, len+1);
    memset(output, 0, len+1);
    
     // 2. loop thru array
    for (int a=0; a<len; a++)
    {
        unsigned char letter = input[a];
    // flip bits if needed
#if JB_FEATURE_BITFLIP
        letter = ~letter;
#endif
        temp[a] = letter;
        //printf("[%d] %02x %02x\n", a, input[a], temp[a]);
    }
    
    // 3. base64 decode if needed
#if JB_FEATURE_BASE64
    UnBase64(output, temp, len);
#else
    for (int a=0; a<len; a++)
    {
        output[a] = temp[a];
    }
#endif
    
    printf("decoded:%s\n", output);
    
    free(temp);
}


int jbCheckAll()
{
    int jbResult = 0;

    jbResult += jbCheckCydia();
    jbResult += jbCheckBash();
    jbResult += jbCheckVarLibApt();
    
    return jbResult;
}

// use str2c.pl script to generate obfuscated strings

//unsigned char bufferCydia[24] = { 0x2f,0x41,0x70,0x70,0x6c,0x69,0x63,0x61,0x74,0x69,0x6f,0x6e,0x73,0x2f,0x43,0x79,0x64,0x69,0x61,0x2e,0x61,0x70,0x70,0x00 }; //"/Applications/Cydia.app"
//unsigned char bufferCydiaBF[24] = { 0xd0,0xbe,0x8f,0x8f,0x93,0x96,0x9c,0x9e,0x8b,0x96,0x90,0x91,0x8c,0xd0,0xbc,0x86,0x9b,0x96,0x9e,0xd1,0x9e,0x8f,0x8f,0x00 }; //"/Applications/Cydia.app" BITFLIPPED

//unsigned char bufferCydia64[33] = { 0x4c,0x30,0x46,0x77,0x63,0x47,0x78,0x70,0x59,0x32,0x46,0x30,0x61,0x57,0x39,0x75,0x63,0x79,0x39,0x44,0x65,0x57,0x52,0x70,0x59,0x53,0x35,0x68,0x63,0x48,0x41,0x3d,0x00 }; //"/Applications/Cydia.app" BASE64
unsigned char bufferCydiaBF64[33] = { 0xb3,0xcf,0xb9,0x88,0x9c,0xb8,0x87,0x8f,0xa6,0xcd,0xb9,0xcf,0x9e,0xa8,0xc6,0x8a,0x9c,0x86,0xc6,0xbb,0x9a,0xa8,0xad,0x8f,0xa6,0xac,0xca,0x97,0x9c,0xb7,0xbe,0xc2,0x00 }; //"/Applications/Cydia.app" BITFLIPPED BASE64
unsigned char bufferCydiaUrlBF64[49] = { 0xa6,0xcc,0x93,0x94,0x9e,0xa8,0xba,0xc9,0xb3,0x86,0xc6,0x88,0xa6,0xa8,0xb1,0x8d,0xa6,0xa8,0x9b,0x93,0xb3,0xcd,0xb1,0x89,0x9d,0xac,0xca,0x93,0x9a,0xb8,0xb9,0x8b,0x9c,0xb8,0x87,0x93,0xb3,0x91,0xbd,0x97,0xa6,0xcd,0x8b,0x97,0xa5,0xcd,0xaa,0xc2,0x00 }; //"cydia://package/com.example.package" BITFLIPPED BASE64


int jbCheckCydia()
{
    unsigned char bufferOutput[256];
    memset(&bufferOutput, 0, 256);
    jbDecodeString((const unsigned char *)&bufferCydiaBF64, (unsigned char *)&bufferOutput);
    
    FILE *f = fopen((const char *)&bufferOutput, "r");
    if (f != NULL)
    {
        fclose(f);
        return JAILBREAK_IOS_CYDIA;
    }
    
    memset(&bufferOutput, 0, 256);
    jbDecodeString((const unsigned char *)&bufferCydiaUrlBF64, (unsigned char *)&bufferOutput);
    
    NSString * urlString = [NSString stringWithCString:(const char *)&bufferOutput encoding:[NSString defaultCStringEncoding]];
    
    // test 2
    NSURL* url = [NSURL URLWithString:urlString];
    if ([[UIApplication sharedApplication] canOpenURL:url])
    {
        return JAILBREAK_IOS_CYDIA;
    }
    
    return 0;
}

unsigned char bufferBashBF64[13] = { 0xb3,0xcd,0xb5,0x8f,0x9d,0x96,0xc6,0x96,0xa6,0xa7,0xb1,0x90,0x00 }; //"/bin/bash" BITFLIPPED BASE64

int jbCheckBash()
{
    unsigned char bufferOutput[256];
    memset(&bufferOutput, 0, 256);
    jbDecodeString((const unsigned char *)&bufferBashBF64, (unsigned char *)&bufferOutput);
    
    FILE *f = fopen((const char *)&bufferOutput, "r");
    if (f != NULL)
    {
        fclose(f);
        return JAILBREAK_IOS_BASH;
    }
    return 0;
}

unsigned char bufferVarLibAptBF64[29] = { 0xb3,0xcc,0xbd,0x86,0x9e,0xa7,0xa5,0x97,0x9b,0xb8,0xaa,0x89,0x9b,0x92,0xb9,0x86,0xb3,0xcd,0x87,0x8f,0xa6,0x96,0xc6,0x97,0x9c,0xb7,0xae,0xc2,0x00 }; //"/private/var/lib/apt" BITFLIPPED BASE64

int jbCheckVarLibApt()
{
    unsigned char bufferOutput[256];
    memset(&bufferOutput, 0, 256);
    jbDecodeString((const unsigned char *)&bufferVarLibAptBF64, (unsigned char *)&bufferOutput);
    
    FILE *f = fopen((const char *)&bufferOutput, "r");
    if (f != NULL)
    {
        fclose(f);
        return JAILBREAK_IOS_VARLIBAPT;
    }
    return 0;
}

/*
 run unsigned code
 has Cydia installed
 has jailbreak files
 full r/w access to the whole filesystem
 some system files will have been modified (content and so sha1 doesn't match with original files)
 stuck to specific version (jailbreakable version)
 */

/*
 - (void)applicationWillTerminate:(UIApplication *)application {
 NSString* path = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test"];
 int result = execlp([path UTF8String], [path UTF8String], NULL);
 // if we got here, then the attempt to run the external process failed, so make a note of this:
 [[NSUserDefaults standardUserDefaults] setValue: [NSNumber numberWithBool: NO] forKey: @"is_jailbroken"];
 NSLog(@"test returns %d", result);
 }
 */
