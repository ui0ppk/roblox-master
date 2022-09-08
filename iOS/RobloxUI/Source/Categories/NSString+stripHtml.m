//
//  NSString+stripHtml.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 7/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "NSString+stripHtml.h"

@interface NSString_stripHtml_XMLParsee : NSObject<NSXMLParserDelegate>
{
    NSMutableArray* strings;
}
- (NSString*)getCharsFound;
@end

@implementation NSString_stripHtml_XMLParsee

- (id)init
{
    if((self = [super init]))
    {
        strings = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)parser:(NSXMLParser*)parser foundCharacters:(NSString*)string
{
    [strings addObject:string];
}

- (NSString*)getCharsFound
{
    return [strings componentsJoinedByString:@""];
}

@end

@implementation NSString (stripHtml)

- (NSString*)stringByStrippingHTML
{
    // take this string obj and wrap it in a root element to ensure only a single root element exists
    NSString* string = [NSString stringWithFormat:@"<root>%@</root>", self];
    
    // add the string to the xml parser
    NSStringEncoding encoding = string.fastestEncoding;
    NSData* data = [string dataUsingEncoding:encoding];
    NSXMLParser* parser = [[NSXMLParser alloc] initWithData:data];
    
    // parse the content keeping track of any chars found outside tags (this will be the stripped content)
    NSString_stripHtml_XMLParsee* parsee = [[NSString_stripHtml_XMLParsee alloc] init];
    parser.delegate = parsee;
    [parser parse];
    
    // any chars found while parsing are the stripped content
    NSString* strippedString = [parsee getCharsFound];
    
    // get the raw text out of the parsee after parsing, and return it
    return strippedString;
}
@end