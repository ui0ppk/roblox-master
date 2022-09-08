//
//  InfoScreenController.m
//  RobloxMobile
//
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "InfoScreenController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "TermsAgreementController.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 540, 360)

@interface InfoScreenController ()
@end

@implementation InfoScreenController
{
    IBOutlet UINavigationBar *_navBar;
    IBOutlet UIWebView *_finePrint;
}

////////////////////////////////////////////////////////////////
// View delegates
////////////////////////////////////////////////////////////////
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Stylize elements
	[RobloxTheme applyToModalPopupNavBar:_navBar];
    
    // Localize strings
    _navBar.topItem.title = NSLocalizedString(@"InfoWord", nil);
    
    [_txtRobloxInfo setText:NSLocalizedString(@"InfoDisclaimerPhrase", nil)];
    
    //get the device info
    NSString* urlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/reference/deviceinfo"];
    NSURL *url = [NSURL URLWithString: urlString];
    NSMutableURLRequest *theRequest = [NSMutableURLRequest  requestWithURL:url
                                                               cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                           timeoutInterval:60*7];
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"GET"];
    
    NSData *receivedData = [NSURLConnection sendSynchronousRequest:theRequest returningResponse:nil error:NULL];
    if (receivedData)
    {
        NSString *deviceInfo = [[NSString alloc] initWithData:receivedData encoding:NSUTF8StringEncoding];
        
        NSCharacterSet *trim = [NSCharacterSet characterSetWithCharactersInString:@"{}\""];
        
        deviceInfo = [[deviceInfo componentsSeparatedByCharactersInSet:trim] componentsJoinedByString:@" "];
        deviceInfo = [deviceInfo stringByReplacingOccurrencesOfString:@"Type :" withString:@":"];
        deviceInfo = [deviceInfo stringByReplacingOccurrencesOfString:@"," withString:@"\n"];
        _txtDeviceInfo.text = deviceInfo;
    }
    else
    {
        _txtDeviceInfo.text = @"";
    }
    
    // Fine print
    NSString *htmlFile = [[NSBundle mainBundle] pathForResource:@"SignUpDisclamer" ofType:@"html" inDirectory:nil];
    if(htmlFile)
    {
        NSString* htmlContents = [NSString stringWithContentsOfFile:htmlFile encoding:NSUTF8StringEncoding error:NULL];
        htmlContents = [htmlContents stringByReplacingOccurrencesOfString:@"textPlaceholder" withString:NSLocalizedString(@"HomeFinePrintWords", nil)];
        [_finePrint loadData:[htmlContents dataUsingEncoding:NSUTF8StringEncoding] MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:[NSURL URLWithString:@""]];
    }
}


- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    if ([RobloxInfo thisDeviceIsATablet])
        self.view.superview.bounds = DEFAULT_VIEW_SIZE;
}
- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [_finePrint stopLoading];
}

- (BOOL)disablesAutomaticKeyboardDismissal
{
    return NO;
}



////////////////////////////////////////////////////////////////
// Action delegates
////////////////////////////////////////////////////////////////
- (IBAction)closeController:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

////////////////////////////////////////////////////////////////
// Fine print
////////////////////////////////////////////////////////////////
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    NSString* urlRequestString = [[request URL] absoluteString];
    NSRange finePrintInit = [urlRequestString rangeOfString:@"file"];
    if(finePrintInit.location != NSNotFound)
        return YES;
    
    [self performSegueWithIdentifier:@"FinePrintSegue" sender:urlRequestString];
    //[[UIApplication sharedApplication] openURL:request.URL];
    
    return NO;
}
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([segue.identifier isEqualToString:@"FinePrintSegue"])
    {
        TermsAgreementController *controller = (TermsAgreementController *)segue.destinationViewController;
        controller.url = sender;
    }
}

@end
