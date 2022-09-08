//
//  RBCaptchaV2ViewController.m
//  RobloxMobile
//
//  Created by Ashish Jain on 12/17/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "RBCaptchaV2ViewController.h"

#import "LoginManager.h"
#import "RobloxInfo.h"
#import "RobloxTheme.h"
#import "RBXFunctions.h"
#import "UIView+Position.h"

@interface RBCaptchaV2ViewController ()
typedef NS_ENUM(NSInteger, RBCaptchaType)
{
    RBCaptchaLogin,
    RBCaptchaSignup,
    RBCaptchaSocialSignup
};

@property UITapGestureRecognizer* touches;

@property (nonatomic, strong) NSString *challenge;
@property (nonatomic, strong) NSString *imageToken;
@property (nonatomic, strong) UIImage *image;

@property (nonatomic, strong) UILabel *instructions;
@property (nonatomic, strong) UIImageView *imageView;
@property (nonatomic, strong) UIButton *submitButton;
@property (nonatomic, strong) UIButton *reloadButton;
@property (nonatomic, strong) UITextField *userInput;

@property (nonatomic, strong) NSString *username;

@property RBCaptchaType cType;

@property (nonatomic, copy) CaptchaCompletionHandler captchaCompletionHandler;

@end

static NSString *publicKey = @"6Le88gcTAAAAALG04IFgQDENWlQmc_hy_3tdF1yY";
static NSInteger captchaAttempts = 0;

#define MARGIN_WIDTH 20


#define VIEW_BACKGROUND_COLOR [UIColor colorWithRed:(238.0/255.0) green:(238.0/255.0) blue:(238.0/255.0) alpha:1.0]

@implementation RBCaptchaV2ViewController

+ (instancetype) CaptchaV2ForLoginWithUsername:(NSString*)username
                             completionHandler:(CaptchaCompletionHandler)completionHandler
{
    RBCaptchaV2ViewController *vc = [[RBCaptchaV2ViewController alloc] init];
    
    vc.username = username;
    vc.captchaCompletionHandler = completionHandler;
    
    vc.cType = RBCaptchaLogin;
    return vc;
}

+ (instancetype) CaptchaV2ForSignupWithUsername:(NSString *)username
                                  andCompletion:(CaptchaCompletionHandler)completionHandler
{
    RBCaptchaV2ViewController *vc = [[RBCaptchaV2ViewController alloc] init];
    
    vc.username = username;
    vc.captchaCompletionHandler = completionHandler;
    
    vc.cType = RBCaptchaSignup;
    return vc;
}

+ (instancetype) CaptchaV2ForSocialSignupWithUsername:(NSString *)username
                                        andCompletion:(CaptchaCompletionHandler)completionHandler
{
    RBCaptchaV2ViewController *vc = [[RBCaptchaV2ViewController alloc] init];
    
    vc.username = username;
    vc.captchaCompletionHandler = completionHandler;
    
    vc.cType = RBCaptchaSocialSignup;
    return vc;
}

#pragma mark Lifecycle functions
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    self.touches = [[UITapGestureRecognizer alloc] init];
    [self.touches setNumberOfTouchesRequired:1];
    [self.touches setNumberOfTapsRequired:1];
    [self.touches setDelegate:self];
    [self.touches setEnabled:YES];
    [self.view addGestureRecognizer:self.touches];
    
    
    self.title = NSLocalizedString(@"CaptchaTitleQuestion", nil);
    
    [self.view setBackgroundColor:VIEW_BACKGROUND_COLOR];
    
    self.instructions = [[UILabel alloc] initWithFrame:CGRectZero];
    [self.instructions setTextColor:[UIColor grayColor]];
    [self.instructions setFont:[RobloxTheme fontBodyLarge]];
    [self.instructions setNumberOfLines:0];
    [self.instructions setText:NSLocalizedString(@"CaptchaVerifyHuman", nil)];
    [self.instructions setTextAlignment:NSTextAlignmentCenter];
    [self.view addSubview:self.instructions];
    
    self.imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    [self.imageView setBackgroundColor:[UIColor whiteColor]];
    [self.imageView setContentMode:UIViewContentModeScaleAspectFit];
    [self.imageView.layer setBorderColor:[UIColor lightGrayColor].CGColor];
    [self.imageView.layer setBorderWidth:0.5];
    [self.view addSubview:self.imageView];
    
    self.userInput = [[UITextField alloc] initWithFrame:CGRectZero];
    [self.userInput setDelegate:self];
    [self.userInput setBackgroundColor:[UIColor whiteColor]];
    [self.userInput.layer setBorderColor:[UIColor lightGrayColor].CGColor];
    [self.userInput.layer setBorderWidth:0.5];
    [self.userInput setTextAlignment:NSTextAlignmentCenter];
    [self.userInput setPlaceholder:NSLocalizedString(@"CaptchaTextPlaceholder", nil)];
    [self.userInput setReturnKeyType:UIReturnKeyDone];
    [self.userInput setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [self.userInput setAutocorrectionType:UITextAutocorrectionTypeNo];
    [self.view addSubview:self.userInput];

    self.submitButton = [[UIButton alloc] initWithFrame:CGRectZero];
    [self.submitButton setTitle:NSLocalizedString(@"SubmitWord", nil).uppercaseString forState:UIControlStateNormal];
    [self.submitButton.titleLabel setTextAlignment:NSTextAlignmentCenter];
    [self.submitButton addTarget:self action:@selector(submitButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    [RobloxTheme applyToModalSubmitButton:self.submitButton];
    [self.view addSubview:self.submitButton];
    
    self.reloadButton = [[UIButton alloc] initWithFrame:CGRectZero];
    [self.reloadButton setTitle:NSLocalizedString(@"ReloadWord", nil).uppercaseString forState:UIControlStateNormal];
    [self.reloadButton.titleLabel setTextAlignment:NSTextAlignmentCenter];
    [self.reloadButton addTarget:self action:@selector(reloadButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    [RobloxTheme applyToModalCancelButton:self.reloadButton];
    [self.reloadButton setBackgroundColor:[UIColor whiteColor]];
    [self.view addSubview:self.reloadButton];
    
}


- (void) viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    
    CGRect bounds = self.view.bounds;
    NSInteger navBarHeight = self.navigationController.navigationBar.frame.size.height;
    
    [self.instructions setPosition:CGPointMake(MARGIN_WIDTH, navBarHeight + MARGIN_WIDTH)];
    [self.instructions setSize:CGSizeMake(bounds.size.width - MARGIN_WIDTH - MARGIN_WIDTH, bounds.size.height / 7)];
    
    [self.imageView setPosition:CGPointMake(MARGIN_WIDTH, CGRectGetMaxY(self.instructions.frame) + MARGIN_WIDTH)];
    [self.imageView setSize:CGSizeMake(bounds.size.width - MARGIN_WIDTH - MARGIN_WIDTH, bounds.size.height / 4)];
    
    [self.userInput setPosition:CGPointMake(MARGIN_WIDTH, CGRectGetMaxY(self.imageView.frame) + MARGIN_WIDTH)];
    [self.userInput setWidth:bounds.size.width - MARGIN_WIDTH - MARGIN_WIDTH];
    [self.userInput setHeight:40];
    
    // This button width takes into account for two buttons side by side separated by MARGIN_WIDTH
    NSInteger buttonWidth = (bounds.size.width - MARGIN_WIDTH - MARGIN_WIDTH - MARGIN_WIDTH) / 2;
    
    [self.reloadButton setPosition:CGPointMake(MARGIN_WIDTH, CGRectGetMaxY(self.userInput.frame) + MARGIN_WIDTH)];
    [self.reloadButton setWidth:buttonWidth];
    [self.reloadButton setHeight:40];

    [self.submitButton setPosition:CGPointMake(CGRectGetMaxX(self.reloadButton.frame) + MARGIN_WIDTH, CGRectGetMaxY(self.userInput.frame) + MARGIN_WIDTH)];
    [self.submitButton setWidth:buttonWidth];
    [self.submitButton setHeight:40];
}

- (void) viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    [self getCaptchaChallengeImageWithCompletionHandler:^(UIImage *image, NSError *captchaError) {
        [RBXFunctions dispatchOnMainThread:^{
            [self.imageView setImage:image];
            [self.userInput becomeFirstResponder];
        }];
    }];
}

#pragma mark Accessors and Mutators
- (BOOL) disablesAutomaticKeyboardDismissal
{
    return NO;
}

#pragma mark UI Functions
- (void) submitButtonTapped:(id)sender {
    if (![RBXFunctions isEmptyString:self.userInput.text]) {
        [self verifyCaptchaForUsername:self.username challenge:self.imageToken response:self.userInput.text completionHandler:^(NSError *rbxRecaptchaError) {
            if ([RBXFunctions isEmpty:rbxRecaptchaError]) {
                // Captcha Validation Succeeded
                [self dismissAndResetCaptchaControllerWithError:rbxRecaptchaError];
            }
            else
            {
                // Captcha Validation Failed
                if (captchaAttempts < 3) {
                    [self reDisplayCaptchaAndIncrementAttempts:YES];
                }
                else
                {
                    [self dismissAndResetCaptchaControllerWithError:rbxRecaptchaError];
                }
            }
        }];
    }
}

- (void) reloadButtonTapped:(id)sender
{
    [self reDisplayCaptchaAndIncrementAttempts:NO];
}

- (void) reDisplayCaptchaAndIncrementAttempts:(BOOL)incrementAttempts {
    [self getCaptchaChallengeImageWithCompletionHandler:^(UIImage *image, NSError *captchaError) {
        [RBXFunctions dispatchOnMainThread:^{
            if (incrementAttempts) {
                captchaAttempts++;
            }
            [self.userInput setText:@""];
            [self.imageView setImage:image];
            [self.userInput becomeFirstResponder];
        }];
    }];
}

- (void) dismissAndResetCaptchaControllerWithError:(NSError *)rbxRecaptchaError
{
    captchaAttempts = 0;

    [RBXFunctions dispatchOnMainThread:^{
        [self dismissViewControllerAnimated:YES completion:nil];
        if (nil != self.captchaCompletionHandler) {
            self.captchaCompletionHandler(rbxRecaptchaError);
        }
    }];
}

- (void) getCaptchaChallengeImageWithCompletionHandler:(void(^)(UIImage *image, NSError *captchaError))completionHandler
{
    [self getChallengeWithCompletionHandler:completionHandler];
}

- (void) getChallengeWithCompletionHandler:(void(^)(UIImage *image, NSError *captchaError))completionHandler
{
    NSString *challengeUrl = [NSString stringWithFormat:@"http://www.google.com/recaptcha/api/challenge?k=%@", publicKey];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:challengeUrl]];
    [[[NSURLSession sharedSession] dataTaskWithRequest:request
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         if ([RBXFunctions isEmpty:error]) {
                                             if (![RBXFunctions isEmpty:data]) {
                                                 NSString *obj = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                                                 if ([obj isKindOfClass:[NSString class]]) {
                                                     self.challenge = [self parseChallengeFromRecaptchaState:obj];
                                                     
                                                     [self getImageTokenWithCompletionHandler:completionHandler];
                                                     return;
                                                 }
                                             }
                                         }
                                         
                                         if (nil != completionHandler) {
                                             completionHandler(nil, error);
                                         }
                                     }] resume];
}

- (NSString *) parseChallengeFromRecaptchaState:(NSString *)recaptchaState
{
    //NSLog(@"recaptcha state: %@", recaptchaState);
    NSString *challenge = nil;
    
    NSArray *components = [recaptchaState componentsSeparatedByString:@","];
    //NSLog(@"components: %@", components);
    for (NSString *component in components) {
        if ([component rangeOfString:@"challenge"].length != 0) {
            NSArray *subComponents = [component componentsSeparatedByString:@":"];
            //NSLog(@"subComponents: %@", subComponents);
            for (NSString *subComponent in subComponents) {
                if ([subComponent rangeOfString:@"challenge"].length == 0) {
                    //NSLog(@"challenge:%@", subComponent);
                    
                    challenge = [subComponent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
                    //NSLog(@"removed white space; challenge:%@", challenge);
                    
                    challenge = [challenge stringByTrimmingCharactersInSet:[NSCharacterSet punctuationCharacterSet]];
                    //NSLog(@"removed apostrophes; challenge:%@", challenge);
                    
                    break;
                }
            }
        }
    }
    
    return challenge;
}

- (void) getImageTokenWithCompletionHandler:(void(^)(UIImage *image, NSError *captchaError))completionHandler
{
    NSString *imageTokenUrl = [NSString stringWithFormat:@"http://www.google.com/recaptcha/api/reload?c=%@&k=%@&type=image", self.challenge, publicKey];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:imageTokenUrl]];
    [[[NSURLSession sharedSession] dataTaskWithRequest:request
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         if ([RBXFunctions isEmpty:error]) {
                                             if (![RBXFunctions isEmpty:data]) {
                                                 NSString *obj = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                                                 if ([obj isKindOfClass:[NSString class]]) {
                                                     self.imageToken = [self parseImageTokenFromResponse:obj];
                                                     
                                                     [self getImageWithCompletionHandler:completionHandler];
                                                     return;
                                                 }
                                             }
                                         }
                                         
                                         if (nil != completionHandler) {
                                             completionHandler(nil, error);
                                         }
                                     }] resume];
}

- (NSString *) parseImageTokenFromResponse:(NSString *)response
{
    NSString *imageToken = nil;
    NSString *substring = [response substringFromIndex:[response rangeOfString:@"('"].location];
    NSArray *components = [substring componentsSeparatedByString:@","];
    for (NSString *component in components) {
        if (component.length > 50) {
            imageToken = component;
            imageToken = [imageToken stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
            imageToken = [imageToken stringByTrimmingCharactersInSet:[NSCharacterSet punctuationCharacterSet]];
            NSLog(@"imageToken: %@", imageToken);
            
            break;
        }
    }
    
    return imageToken;
}

- (void) getImageWithCompletionHandler:(void(^)(UIImage *image, NSError *captchaError))completionHandler
{
    NSString *imageUrl = [NSString stringWithFormat:@"http://www.google.com/recaptcha/api/image?c=%@", self.imageToken];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:imageUrl]];
    [[[NSURLSession sharedSession] dataTaskWithRequest:request
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         UIImage *responseImage = nil;
                                         if ([RBXFunctions isEmpty:error]) {
                                             if (![RBXFunctions isEmpty:data]) {
                                                 responseImage = [UIImage imageWithData:data];
                                                 if ([responseImage isKindOfClass:[UIImage class]]) {
                                                     self.image = responseImage;
                                                     captchaAttempts++;
                                                 }
                                             }
                                         }
                                         
                                         if (nil != completionHandler) {
                                             completionHandler(responseImage, error);
                                         }
                                     }] resume];
}

- (void) verifyCaptchaForUsername:(NSString *)username challenge:(NSString *)challenge response:(NSString *)response completionHandler:(CaptchaCompletionHandler)completionHandler
{
    NSString* validateURL;
    switch (self.cType)
    {
        case (RBCaptchaLogin):          validateURL = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/captcha/validate/login/"];  break;
        case (RBCaptchaSignup):         validateURL = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/captcha/validate/signup/"]; break;
        case (RBCaptchaSocialSignup):   validateURL = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/captcha/validate/signup/"]; break; //maybe this will matter one day
        default:                        validateURL = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/captcha/validate/login/"];  break;
    }
    NSURL *url = [NSURL URLWithString: validateURL];
    
    //configure the request
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60 * 7];
    
    [theRequest setHTTPMethod:@"POST"];
    
    //    NSDictionary *bodyParams = @{
    //                                 @"username":username,
    //                                 @"recaptcha_challenge_field":challenge,
    //                                 @"recaptcha_response_field":response
    //                                 };
    //    [theRequest setHTTPBody:[NSKeyedArchiver archivedDataWithRootObject:bodyParams]];
    
    NSString* args = [NSString stringWithFormat:@"username=%@&recaptcha_challenge_field=%@&recaptcha_response_field=%@", username, challenge, response];
    NSLog(@"recaptcha args: %@", args);
    [theRequest setHTTPBody:[args dataUsingEncoding:NSUTF8StringEncoding]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    
    [[[NSURLSession sharedSession] dataTaskWithRequest:theRequest
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                                         NSLog(@"HTTP Response Status: %@", @(httpResponse.statusCode).stringValue);
                                         
                                         if ([RBXFunctions isEmpty:error]) {
                                             // collect user info and store in user object
                                             
                                             if (httpResponse.statusCode != 200)
                                             {
                                                 error = [NSError errorWithDomain:@"CaptchaFailed" code:httpResponse.statusCode userInfo:@{@"captchainfo":args}];
                                             }
                                         }
                                         
                                         if (nil != completionHandler) {
                                             completionHandler(error);
                                         }
                                     }] resume];
}

#pragma mark Delegate functions
- (void) resignAllResponders
{
    [self.view endEditing:YES];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    //NSLog(@"GestureRecognizer : %@", touch);
    UIView* touchedView = touch.view;
    if (touchedView == self.view)
    {
        [self resignAllResponders];
        return  YES;
    }
    return NO;
}
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [self resignAllResponders];
    
    if (textField.text.length > 0)
        [self submitButtonTapped:nil];
    
    return YES;
}

@end
