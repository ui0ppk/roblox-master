//
//  SignupVerifier.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 2/5/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum {
    GENDER_DEFAULT,
    GENDER_BOY,
    GENDER_GIRL
} Gender;

typedef void(^SignupVerifierCompletionHandler)(BOOL success, NSString *message);

@interface SignupVerifier : NSObject

+ (instancetype) sharedInstance;

// Helper functions
- (NSString *) obfuscateEmail:(NSString *)emailAddress;

- (void) checkIfValidUsername:(NSString*)username completion:(SignupVerifierCompletionHandler)handler;
- (void) checkIfValidPassword:(NSString*)password withUsername:(NSString*)username completion:(SignupVerifierCompletionHandler)handler;
- (void) checkIfPasswordsMatch:(NSString*)password withVerification:(NSString*)verify completion:(SignupVerifierCompletionHandler)handler;
- (void) checkIfValidEmail:(NSString*)email completion:(SignupVerifierCompletionHandler)handler;
- (void) getAlternateUsername:(NSString *)username completion:(SignupVerifierCompletionHandler)handler;

- (void) signUpWithUsername:(NSString *)username
                   password:(NSString *)password
                birthString:(NSString *)birthString
                     gender:(Gender)gender
                      email:(NSString *)email
            completionBlock:(void (^)(NSError *signUpError))completionBlock;

@end
