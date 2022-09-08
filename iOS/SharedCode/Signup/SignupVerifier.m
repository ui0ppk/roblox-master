//
//  SignupVerifier.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 2/5/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "SignupVerifier.h"

#import <AdSupport/AdSupport.h>
#import <CommonCrypto/CommonHMAC.h>

#import "iOSSettingsService.h"
#import "LoginManager.h"
#import "NSDictionary+Parsing.h"
#import "RBXEventReporter.h"
#import "RBXFunctions.h"
#import "RobloxData.h"
#import "RobloxInfo.h"
#import "RobloxWebUtility.h"
#import "SessionReporter.h"
#import "UserInfo.h"

DYNAMIC_FASTFLAGVARIABLE(EnableXBOXSignupRules, false);

@interface SignupVerifier ()

@property (retain, nonatomic) NSString *signUpUrlString;
@property (retain, nonatomic) NSString *signUpArgs;

@end

@implementation SignupVerifier

NSString* s3 = @"Q,v?KZ^#q";
NSString* s1 = @"Fu.*mJ";
NSString* s4 = @"l%=f~RIWh";
NSString* s2 = @"L65H";
NSString* s5 = @"C39$";
NSString* s8 = @"Av=MHZ";
NSString* s6 = @"jEda0J~iq";
NSString* s9 = @"mfcyG9,F";
NSString* s7 = @"b@Wl";
NSString* s10 = @"B7YpO";

#pragma mark Constructors
+(id) sharedInstance
{
    static dispatch_once_t rbxSignUpVerifierManPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxSignUpVerifierManPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}
-(id) init
{
    if(self = [super init])
    {
        // have to use www base url
        NSString* baseUrl = [[RobloxInfo getBaseUrl] stringByReplacingOccurrencesOfString:@"://m." withString:@"://www."];
        baseUrl = [baseUrl stringByReplacingOccurrencesOfString:@"http" withString:@"https"];
        
        iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
        bool bSignUpWithHash = iosSettings->GetValueSignUpWithHash();
        
        if(bSignUpWithHash)
            self.signUpUrlString = [baseUrl stringByAppendingString:@"mobileapi/securesignup"];
        else
             self.signUpUrlString = [baseUrl stringByAppendingString:@"mobileapi/signup"];
            
        self.signUpArgs = @"userName=%@&password=%@&gender=%@&dateOfBirth=%@&advertiserID=%@";
    }
    return self;
}

#pragma mark Flag Settings
-(bool) usingNewSignupRules { return DFFlag::EnableXBOXSignupRules; }


#pragma mark - Email
-(NSString *)obfuscateEmail:(NSString *)emailAddress
{
    NSRange rangeOfAtChar = [emailAddress rangeOfString:@"@"];
    
    if (rangeOfAtChar.location != NSNotFound)
    {
        NSRange emailNameRange = NSMakeRange(0, rangeOfAtChar.location);
        
        NSMutableString* hiddenCharacters = [NSMutableString stringWithString:@""];
        for (int i = 0; i < emailNameRange.length; i++)
            [hiddenCharacters appendString:@"*"];
        
        // return the obscured email
        return [emailAddress stringByReplacingCharactersInRange:emailNameRange withString:hiddenCharacters];
    }
    
    //doesn't look like a real email address to me, so just return the string
    return emailAddress;
}
-(bool) isValidEmail:(NSString*)email
{
    NSString *expression = @"[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}";
    NSError *error = nil;
    
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:expression options:NSRegularExpressionCaseInsensitive error:&error];
    
    if(error)
        return NO;
    
    NSTextCheckingResult *isEmail = [regex firstMatchInString:email options:0 range:NSMakeRange(0, [email length])];
    
    return isEmail;
}
- (void) checkIfValidEmail:(NSString*)email completion:(SignupVerifierCompletionHandler)handler
{
    if (![self isValidEmail:email])
    {
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldEmail
                                                         withContext:RBXAContextSignup
                                                           withError:RBXAErrorTooShort];
        handler(NO, NSLocalizedString(@"EmailInvalid", nil));
    }
    else
    {
        //check the server if the username is not blacklisted
        [RobloxData checkIfBlacklistedEmail:email
                                 completion:^(BOOL isBlacklisted)
        {
            if (!isBlacklisted)
            {
                //looks like the user has a good email!
                handler(YES, nil);
            }
            else
            {
                handler(NO, NSLocalizedString(@"EmailBlacklisted", nil));
            }
         }];
    }
}



#pragma mark - Username
- (void) checkIfValidUsername:(NSString*)username completion:(SignupVerifierCompletionHandler)handler
{
    //is the username within the acceptable length?
    if (username.length == 0)
    {
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldUsername
                                                         withContext:RBXAContextSignup
                                                           withError:RBXAErrorMissingRequiredField];
        if (handler)
            handler(NO, NSLocalizedString(@"UsernameMissing", nil));
        return;
    }
    
    if (username.length < 3)
    {
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldUsername
                                                         withContext:RBXAContextSignup
                                                           withError:RBXAErrorTooShort];
        if (handler)
            handler(NO, NSLocalizedString(@"UsernameInvalidLength", nil));
        return;
    }
    
    if (username.length > 20) // we limit usernames to 20 characters
    {
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldUsername
                                                         withContext:RBXAContextSignup
                                                           withError:RBXAErrorTooLong];
        if (handler)
            handler(NO, NSLocalizedString(@"UsernameInvalidLength", nil));
        return;
    }
    
    //allow only one underscore, so long as it is not at the beginning or end of the name
    //^_|\w+__|_$
    NSRange underscoreLocation = [username rangeOfString:@"_"];
    if (underscoreLocation.location != NSNotFound)
    {
        if (underscoreLocation.location == 0 || underscoreLocation.location == username.length-1)
        {
            if (handler)
                handler(NO, NSLocalizedString(@"UsernameInvalidUnderscore", nil));
            return;
        }
        
        //are there two underscores? that's not okay
        NSString* afterScore = [username substringFromIndex:underscoreLocation.location+1];
        NSRange otherUnderscore = [afterScore rangeOfString:@"_"];
        if (otherUnderscore.length == 1)
        {
            if (handler)
                handler(NO, NSLocalizedString(@"UsernameTooManyUnderscores", nil));
            return;
        }
    }

    //do we have any disallowed characters?
    // NOTE - "_" is not picked up as a non alphanumeric character
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:@"\\W" options:NSMatchingProgress error:nil];
    NSTextCheckingResult* regexResult = [regex firstMatchInString:username options:NSMatchingCompleted range:NSMakeRange(0, username.length)];
    if (regexResult)
    {
        if (handler)
            handler(NO, NSLocalizedString(@"UsernameInvalidCharacters", nil));
        return;
    }
    
    //create a callback to call once we hit the endpoint
    void (^callbackBlock)(BOOL, NSString*) = ^(BOOL success, NSString *message)
    {
        if (success)
        {
            //if this endpoint fails for some reason, it will return true but with a message
            //this will prevent sign-ups from being blocked by an erroneous endpoint
            //if (message && message.length > 0)
            //{
                //an error has occurred with this endpoint, something should probably report this error
                /*[[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldUsername
                 withContext:RBXAContextSignup
                 withError:message];*/
            //}
            
            //looks like the user has a good name!
            if (handler)
                handler(YES, nil);
        }
        else
        {
            NSString* errMessage = message.length > 0 ? message : NSLocalizedString(@"UsernameCommon", nil);
            
            //for the new endpoint...
            if ([errMessage isEqualToString:@"This username is already in use"] || [errMessage isEqualToString:@"Already Taken"])
                errMessage = NSLocalizedString(@"UsernameCommon", nil); //this triggers username suggestions
            
            if (handler)
                handler(NO, errMessage);
        }
    };
    
    
    //check the server if the username is valid
    if ([self usingNewSignupRules])
    {
        //call the new endpoint
        [RobloxData checkIfValidUsername:username
                              completion:callbackBlock];
    }
    else
    {
        //call the old endpoint
        [RobloxData oldCheckIfValidUsername:username
                                 completion:callbackBlock];
    }
}
- (void) getAlternateUsername:(NSString *)username completion:(SignupVerifierCompletionHandler)handler
{
    [RobloxData recommendUsername:username
                       completion:^(BOOL success, NSString *newUsername)
     {
         if (success)
         {
             //Why the heck would we get back the same name back?!
             if ([newUsername isEqualToString:username])
                 handler(NO, NSLocalizedString(@"UsernameCommon", nil));
             else
                 handler(YES, newUsername);
             
         }
         else
             //the user has a name that is taken, but we could not find a replacement
             //let the user know that they need a new username
             handler(NO, NSLocalizedString(@"UsernameCommon", nil));
     }];
}




#pragma mark - Passwords
- (void) checkIfValidPassword:(NSString*)password withUsername:(NSString*)username completion:(SignupVerifierCompletionHandler)handler
{
    if (password.length == 0)
    {
        handler(NO, NSLocalizedString(@"PasswordMissing", nil));
        return;
    }

    if (password.length < 8)
    {
        handler(NO, NSLocalizedString([RobloxInfo thisDeviceIsATablet] ? @"PasswordWrong" : @"PasswordWrongShort", nil));
        return;
    }
    
    if (password.length > 20)
    {
        handler(NO, NSLocalizedString([RobloxInfo thisDeviceIsATablet] ? @"PasswordWrong" : @"PasswordWrongLong", nil));
        return;
    }
    
    if ([password isEqualToString:username])
    {
        handler(NO, NSLocalizedString(@"PasswordMatchesUsername", nil));
        return;
    }
    
    
    //everything looks goood
    //create a callback block for the password endpoint
    void (^callbackBlock)(BOOL, NSString*) = ^(BOOL success, NSString *message)
    {
        if (success)
            handler(YES, @"");
        else
        {
            //if there is no message, then there has been a server error.
            //Do not block the client's sign up if this is the case
            if (!message)
                handler(YES, NSLocalizedString(@"PasswordServerError", nil));
            else
                handler(NO, message);
        }
    };
    
    //check the server if we have a valid password
    if ([self usingNewSignupRules])
    {
        [RobloxData checkIfValidPassword:password
                                username:username
                              completion:callbackBlock];
    }
    else
    {
        [RobloxData oldCheckIfValidPassword:password
                                   username:username
                                 completion:callbackBlock];
    }
}
- (void) checkIfPasswordsMatch:(NSString*)password withVerification:(NSString*)verify completion:(SignupVerifierCompletionHandler)handler
{
    //this is a foolish implementation, but it does come with the benefit of providing a message with the success boolean
    if (password.length == 0)
        handler(NO, NSLocalizedString(@"PasswordMissing", nil));
    
    else if (verify.length == 0)
        handler(NO, NSLocalizedString(@"VerifyMissing", nil));
    
    else if (![password isEqualToString:verify])
        handler(NO, NSLocalizedString(@"PasswordNoMatch", nil));
    
    else
        handler(YES, @"");
}

#pragma mark - Newer / Shinier Implementation of Signup
- (NSString *)hashString:(NSString *)input
{
    const char *cstr = [input cStringUsingEncoding:NSUTF8StringEncoding];
    NSData *data = [NSData dataWithBytes:cstr length:input.length];
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    
    // This is an iOS5-specific method.
    // It takes in the data, how much data, and then output format, which in this case is an int array.
    CC_SHA256(data.bytes, data.length, digest);
    
    NSMutableString* output = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
    
    // Parse through the CC_SHA256 results (stored inside of digest[]).
    for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        [output appendFormat:@"%02x", digest[i]];
    }
    
    return output;
}

- (void) signUpV2WithUsername:(NSString *)username
                     password:(NSString *)password
                  birthString:(NSString *)birthString
                       gender:(Gender)gender
              completionBlock:(void (^)(NSError *signUpError))completionBlock
{
    NSDate* startTime = [NSDate date];
    
    // need to format for particular special characters
    NSString *escapedPassword = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                                      (CFStringRef)password,
                                                                                                      NULL,
                                                                                                      CFSTR("!*'();:@&=+$,/?%#[]"),
                                                                                                      kCFStringEncodingUTF8));
    
    NSString *escapedBirthString = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                                         (CFStringRef)birthString,
                                                                                                         NULL,
                                                                                                         CFSTR("!*'();:@&=+$,/?%#[]"),
                                                                                                         kCFStringEncodingUTF8));

    NSString* formattedSignUpArgs = [NSString stringWithFormat:@"username=%@&password=%@&gender=%@&birthday=%@",
                                     username,
                                     escapedPassword,
                                     gender == GENDER_GIRL ? @"female" : @"male",
                                     escapedBirthString];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/signup/v1/"]]];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[formattedSignUpArgs dataUsingEncoding:NSUTF8StringEncoding]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    bool bSignUpWithHash = iosSettings->GetValueSignUpWithHash();
    
    if(bSignUpWithHash)
    {
        NSString* s;
        if([RobloxInfo isTestSite])
            s = [NSString stringWithFormat:@"%@%@%@%@%@%@", s6, s7, s8, s9, s10, username];
        else
            s = [NSString stringWithFormat:@"%@%@%@%@%@%@", s1, s2, s3, s4, s5, username];
        
        
        NSString* h = [self hashString:s];
        [request setValue:h forHTTPHeaderField:@"X-RBXUSER-TOKEN"];
    }

    [[[NSURLSession sharedSession] dataTaskWithRequest:request
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                                         RBXAnalyticsResult analyticsResult = RBXAResultFailure;
                                         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;

                                         if ([RBXFunctions isEmpty:error]) {
                                             if (![RBXFunctions isEmpty:data]) {
                                                 NSDictionary *dataDict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:&error];
                                                 if ([RBXFunctions isEmpty:error]) {
                                                     if ([dataDict objectForKey:@"userId"])
                                                     {
                                                         analyticsResult = RBXAResultSuccess;
                                                     }
                                                     else if ([dataDict objectForKey:@"reasons"])
                                                     {
                                                         analyticsResult = RBXAResultFailure;
                                                         
                                                         NSArray *reasons = [dataDict arrayForKey:@"reasons"];
                                                         NSString *primaryReason = reasons.firstObject;
                                                         
                                                         if ([primaryReason.lowercaseString isEqualToString:@"BirthdayInvalid".lowercaseString]) {
                                                             analyticsError = RBXAErrorBirthdayInvalid;
                                                         } else if ([primaryReason.lowercaseString isEqualToString:@"Captcha".lowercaseString]) {
                                                             analyticsError = RBXAErrorCaptcha;
                                                             primaryReason = NSLocalizedString(@"TooManyAttempts", nil); //make sure that the captcha gets triggered
                                                         } else if ([primaryReason.lowercaseString isEqualToString:@"GenderInvalid".lowercaseString]) {
                                                             analyticsError = RBXAErrorGenderInvalid;
                                                         } else if ([primaryReason.lowercaseString isEqualToString:@"PasswordInvalid".lowercaseString]) {
                                                             analyticsError = RBXAErrorPasswordInvalid;
                                                         } else if ([primaryReason.lowercaseString isEqualToString:@"UsernameInvalid".lowercaseString]) {
                                                             analyticsError = RBXAErrorUsernameInvalidWeb;
                                                         } else if ([primaryReason.lowercaseString isEqualToString:@"UsernameTaken".lowercaseString]) {
                                                             analyticsError = RBXAErrorUsernameTaken;
                                                         }
                                                         
                                                         //save the reason into the error to be displayed to the user
                                                         error = [NSError errorWithDomain:primaryReason code:httpResponse.statusCode userInfo:@{@"request":request}];
                                                     }
                                                     else
                                                     {
                                                         error = [NSError errorWithDomain:@"Response data could not be handled" code:httpResponse.statusCode userInfo:dataDict];
                                                         analyticsResult = RBXAResultUnknown;
                                                         analyticsError = RBXAErrorBadResponse;
                                                     }
                                                 }
                                                 else
                                                 {
                                                     // There was some kind of json parsing error; lets convert the data into a string and send that back to RBXHQ
                                                     NSString *unkError = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                                                     //NSLog(@"unkError: %@", unkError);
                                                     NSDictionary *originalErrorInfo = error.userInfo;
                                                     error = [NSError errorWithDomain:unkError code:httpResponse.statusCode userInfo:originalErrorInfo];
                                                 }
                                             }
                                             else
                                             {
                                                 error = [NSError errorWithDomain:@"Data length is 0" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                 analyticsResult = RBXAResultFailure;
                                                 analyticsError = RBXAErrorCannotParseData;
                                             }
                                         }
                                         
                                         if (nil != completionBlock) {
                                             completionBlock(error);
                                         }
                                         
                                         [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSignup
                                                                                                   result:analyticsResult
                                                                                                 rbxError:analyticsError
                                                                                             startingTime:startTime
                                                                                        attemptedUsername:username
                                                                                               URLRequest:request
                                                                                         HTTPResponseCode:httpResponse ? httpResponse.statusCode : 0
                                                                                             responseData:data
                                                                                           additionalData:nil];
                                     }] resume];
}

- (void) signUpWithUsername:(NSString *)username
                   password:(NSString *)password
                birthString:(NSString *)birthString
                     gender:(Gender)gender
                      email:(NSString *)email
            completionBlock:(void (^)(NSError *signUpError))completionBlock {
    
    if ([LoginManager apiProxyEnabled]) {
        [self signUpV2WithUsername:username password:password birthString:birthString gender:gender completionBlock:completionBlock];
        return;
    }
    
    NSDate* startTime = [NSDate date];
    
    //it is assumed that all input has been verified before calling this function
    NSString* genderString = @"Unknown";
    if (gender == GENDER_GIRL)
        genderString = @"Female";
    else if (gender == GENDER_BOY)
        genderString = @"Male";
    
    
    // need to format for particular special characters
    NSString *escapedPassword = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                                      (CFStringRef)password,
                                                                                                      NULL,
                                                                                                      CFSTR("!*'();:@&=+$,/?%#[]"),
                                                                                                      kCFStringEncodingUTF8));
    
    NSString *escapedBirthString = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                                         (CFStringRef)birthString,
                                                                                                         NULL,
                                                                                                         CFSTR("!*'();:@&=+$,/?%#[]"),
                                                                                                         kCFStringEncodingUTF8));
    
    
    NSString* formattedSignUpArgs = [NSString stringWithFormat:self.signUpArgs,
                                     username,
                                     escapedPassword,
                                     genderString,
                                     escapedBirthString,
                                     [[[ASIdentifierManager sharedManager] advertisingIdentifier] UUIDString]];
    
    if (email && email.length > 0)
    {
        NSString *escapedEmail = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                                       (CFStringRef)email,
                                                                                                       NULL,
                                                                                                       CFSTR("!*'();:@&=+$,/?%#[]"),
                                                                                                       kCFStringEncodingUTF8));
        formattedSignUpArgs = [[formattedSignUpArgs stringByAppendingString:@"&email="] stringByAppendingString:escapedEmail];
    }
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:self.signUpUrlString]];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[formattedSignUpArgs dataUsingEncoding:NSUTF8StringEncoding]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    bool bSignUpWithHash = iosSettings->GetValueSignUpWithHash();
    
    if(bSignUpWithHash)
    {
        NSString* s;
        if([RobloxInfo isTestSite])
            s = [NSString stringWithFormat:@"%@%@%@%@%@%@", s6, s7, s8, s9, s10, username];
        else
            s = [NSString stringWithFormat:@"%@%@%@%@%@%@", s1, s2, s3, s4, s5, username];
        
        
        NSString* h = [self hashString:s];
        [request setValue:h forHTTPHeaderField:@"X-RBXUSER-TOKEN"];
    }
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:request queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *responseData, NSError *responseError)
     {
         RBXAnalyticsResult analyticsResult = RBXAResultFailure;
         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
         
         // parse the response
         NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
         NSError *signupError = nil;
         if(httpResponse)
         {
             NSDictionary* responseDict = [[NSDictionary alloc] init];
             NSError* dictError = nil;
             responseDict = [NSJSONSerialization JSONObjectWithData:responseData options:kNilOptions error:&dictError];
             if(!dictError && responseDict)
             {
                 NSString* statusResponse = [responseDict objectForKey:@"Status"];
                 if ([statusResponse isEqualToString:@"OK"] && [responseDict objectForKey:@"UserInfo"])
                 {
                     analyticsResult = RBXAResultSuccess;
                 }
                 else
                 {
                     if ([statusResponse isEqualToString:@"AccountCreationFloodcheck"])
                     {
                         signupError = [NSError errorWithDomain:NSLocalizedString(@"TooManyAttempts", nil) code:httpResponse.statusCode userInfo:responseDict];
                         analyticsError = RBXAErrorFloodcheckAccountCreate;
                     }
                     else
                     {
                         //might be that we haven't caught a case
                         //TO DO - CATCH OTHER CASES! 10/23/2015 Kyler
                         //server JSON formatting error
                         signupError = [NSError errorWithDomain:NSLocalizedString(@"JSONFormatError", nil) code:httpResponse.statusCode userInfo:responseDict];
                         analyticsError = RBXAErrorUnknownError;
                     }
                 }
             }
             else
             {
                 //server bad response
                 signupError = [NSError errorWithDomain:NSLocalizedString(@"ServerBadResponse", nil) code:httpResponse.statusCode userInfo:responseDict];
                 analyticsError = dictError ? RBXAErrorJSONParseFailure : RBXAErrorUnknownError;
             }
         }
         else
         {
             //server no response
             signupError = [NSError errorWithDomain:NSLocalizedString(@"SocialGigyaErrorNotifyLoginNoResponse", nil) code:httpResponse.statusCode userInfo:nil];
             analyticsError = RBXAErrorNoHTTPResponse;
         }
         
         if (nil != completionBlock)
         {
             completionBlock(signupError);
         }
         
         [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSignup
                                                                   result:analyticsResult
                                                                 rbxError:analyticsError
                                                             startingTime:startTime
                                                        attemptedUsername:username
                                                               URLRequest:request
                                                         HTTPResponseCode:httpResponse ? httpResponse.statusCode : 0
                                                             responseData:responseData
                                                           additionalData:nil];
     }];
    
}

@end
